#include "platform.h"
#include <string.h>
#include "platform/context.h"
#include "platform/netpoll.h"
#include "start.h"
#include "array.h"
#include "ggc.h"
#include "diskio.h"
#include "netio.h"
#include "queue.h"
#include "scheduler.h"
#include "task.h"
#include "crypto.h"
#include "str.h"
#include "alloc.h"
#include "gc.h"


/* kernel_thread_map holds map of scheduler id to its kernel_thread_st instance */
kernel_thread_t **kernel_thread_map;

/* diskio_ring_map holds map of diskio_worker id to its io_uring ring instance */
#if defined(__linux__)
struct io_uring **diskio_ring_map = NULL;
#endif

/* single netpoller file descriptor */
netpoll_t* netpoller;

/* global arena for runtime memory allocation.
   this must be used by c runtime itself, not for language.
   global_arena will not be garbage collected for safety.
   it's my responsibilty to release it after usage.
*/
arena_t* __global__arena__;

/* current task_count which is still need to be completed */
atomic_int task_count;

/* shceudler thread instance */
pthread_t sched_threads[SCHEDULER_THREAD_POOL_SIZE];

/**
 * @brief Create and schedule a new task on a random scheduler thread.
 * 
 * Allocates a task with its own stack and context, assigns it a random
 * ID, and pushes it onto a scheduler thread's ready queue.
 * Each task is blocking in nature, main loop waits for its completion before
 * terminating program.
 * 
 * @param fn   Function pointer for the task to execute.
 * @param this Argument to pass to the task function.
 */
void thread(void (*fn)(void), int nargs, ...) {
    task_payload_t *payload = allocate(__global__arena__, sizeof(task_payload_t));

    payload->fn = fn;
    payload->nargs = nargs;

    // Allocate arrays for FFI types and pointers to values
    payload->arg_types = allocate(__global__arena__, sizeof(ffi_type*) * (size_t)nargs);
    payload->arg_values = allocate(__global__arena__, sizeof(void*) * (size_t)nargs);

    va_list ap;
    va_start(ap, nargs);
    for (int i = 0; i < nargs; i++) {
        // All args are pointers
        payload->arg_types[i] = &ffi_type_pointer;

        void **val = allocate(__global__arena__, sizeof(void*));
        *val = va_arg(ap, void*);

        payload->arg_values[i] = val;
    }
    va_end(ap);

    // Return type = void
    if (ffi_prep_cif(
            &payload->cif,
            FFI_DEFAULT_ABI,
            (unsigned int)nargs,
            &ffi_type_void,
            payload->arg_types
        ) != FFI_OK) {
        fprintf(stderr, "ffi_prep_cif failed\n");
        abort();
    }

    int kid = rand() % SCHEDULER_THREAD_POOL_SIZE;
    task_t *t = task_create(fn, payload, kernel_thread_map[kid]);

    t->id = rand();
    atomic_fetch_add(&task_count, 1);
    safe_q_push(&kernel_thread_map[kid]->ready_q, t);
}

/**
 * @brief Create and schedule a main task on a random scheduler thread.
 * 
 * Allocates a task with its own stack and context, assigns it a random
 * ID, and pushes it onto a scheduler thread's ready queue.
 * daemon is not blocking in nature. life ends with main loop.
 * 
 * @param fn   Function pointer for the task to execute.
 * @param this Argument to pass to the task function.
 */
 void orphan(void*(*fn)(void*) __attribute__((unused)), void *this __attribute__((unused))) {
    // @depricated
    // int kernel_thread_id = rand() % SCHEDULER_THREAD_POOL_SIZE;
    // task_t *t1 = task_create(fn, this, kernel_thread_map[kernel_thread_id]);
    // t1->id = rand();
    // safe_q_push(&(kernel_thread_map[kernel_thread_id]->ready_q), t1);
}

/**
 * @brief Initialize the I/O subsystem.
 * 
 * - Initializes the global I/O queue.
 * - Creates an epoll instance for monitoring file descriptors.
 * - Launches a pool of I/O worker threads.
 * 
 * @return 0 on success, 1 on failure.
 */
int init_io(void) {
#if defined(__linux__)
    diskio_ring_map = allocate(__global__arena__, DISKIO_THREAD_POOL_SIZE * sizeof(struct io_uring*));
    if (!diskio_ring_map) {
        perror("calloc diskio_ring_map");
        exit(1);
    }

    pthread_t diskio_threads[DISKIO_THREAD_POOL_SIZE];
    
    for (int i = 0; i < DISKIO_THREAD_POOL_SIZE; i++) {
        diskio_ring_map[i] = allocate(__global__arena__, 1 * sizeof(struct io_uring));
        if (!diskio_ring_map[i]) {
            perror("calloc ring");
            exit(1);
        }
    }

    for (int i = 0; i < DISKIO_THREAD_POOL_SIZE; i++) {
        struct io_uring *ring = allocate(__global__arena__, sizeof(*ring));
        if (!ring) abort();

        int ret = io_uring_queue_init(DISKIO_QUEUE_DEPTH, ring, 0);
        if (ret < 0) {
            char buf[128];
            int n = snprintf(buf, sizeof(buf), "io_uring_queue_init failed: %d\n", ret);
            write(2, buf, n);
            abort();
        }

        diskio_ring_map[i] = ring; // now safe
        
        int rc = pthread_create(&diskio_threads[i], NULL, diskio_worker, (void*)(intptr_t)i);
        if (rc != 0) {
            fprintf(stderr, "pthread_create(%d) failed: %s\n", i, strerror(rc));
            exit(1);
        }
    }
#endif
    pthread_t netio_threads[NETIO_THREAD_POOL_SIZE];
    for (int i = 0; i < NETIO_THREAD_POOL_SIZE; i++) {
        int rc = pthread_create(&netio_threads[i], NULL, netio_worker, (void*)(intptr_t)i);
        if (rc != 0) {
            fprintf(stderr, "pthread_create(%d) failed: %s\n", i, strerror(rc));
            exit(1);
        }
    }
    return 0;
}

/**
 * @brief Initialize scheduler threads.
 * 
 * - Allocates and initializes kernel_thread_t structures.
 * - Initializes each scheduler's local ready queue.
 * - Creates threads running the scheduler_run() loop.
 * 
 * @return 0 on success.
 */
int init_scheduler(void) {
    atomic_init(&task_count, 0);

    kernel_thread_map = allocate(__global__arena__, SCHEDULER_THREAD_POOL_SIZE * sizeof(kernel_thread_t*));
    for (int i=0;i<SCHEDULER_THREAD_POOL_SIZE;i++) {
        kernel_thread_map[i] = allocate(__global__arena__, 1 * sizeof(kernel_thread_t));
        platform_ctx_init(&kernel_thread_map[i]->sched_ctx);
        kernel_thread_map[i]->id = i;
        kernel_thread_map[i]->current = NULL;
        safe_q_init(&kernel_thread_map[i]->ready_q, SCHEDULER_LOCAL_QUEUE_SIZE);
        unsafe_ioq_init(&kernel_thread_map[i]->wait_q, SCHEDULER_LOCAL_QUEUE_SIZE);

        pthread_create(&sched_threads[i], NULL, scheduler_run, kernel_thread_map[i]);
    }

    return 0;
}

/**
 * @brief Cleanup resources used by the scheduler.
 * 
 * Currently frees only the first kernel thread. In production, all
 * threads and queues should be properly deallocated.
 */
void clean_scheduler(void) {
    // free(kernel_thread_map[0]);
}

/**
 * @brief wait for all schedulers to join.
 */
int wait_for_schedulers(void) {
    for (int i = 0; i < SCHEDULER_THREAD_POOL_SIZE; i++) {
        pthread_join(sched_threads[i], NULL);
    }

    return 0;
}
