#ifndef INITUTILS_H
#define INITUTILS_H

#include "platform.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <string.h> 


#include "start.h"
#include "array.h"
#include "ggc.h"
#include "diskio.h"
#include "queue.h"
#include "scheduler.h"
#include "task.h"
#include "crypto.h"
#include "str.h"
#include "alloc.h"
#include "gc.h"
#include "netio.h"


/* kernel_thread_map holds map of scheduler id to its kernel_thread_st instance */
extern kernel_thread_t **kernel_thread_map;

/* diskio_ring_map holds map of diskio_worker id to its io_uring ring instance */
#if defined(__linux__)
extern struct io_uring **diskio_ring_map;
#endif
/* single netpoller file descriptor */
extern int netio_epoll_id;

/* global arena for runtime memory allocation.
   this must be used by c runtime itself, not for language.
   global_arena will not be garbage collected for safety.
   it's my responsibilty to release it after usage.
*/
extern arena_t* __global__arena__;

/* current task_count which is still need to be completed */
extern atomic_int task_count;

/* shceudler thread instance */
extern pthread_t sched_threads[SCHEDULER_THREAD_POOL_SIZE];

/**
 * @brief Create and schedule a new task on a random scheduler thread.
 * 
 * Allocates a task with its own stack and context, assigns it a random
 * ID, and pushes it onto a scheduler thread's ready queue.
 * 
 * @param fn   Function pointer for the task to execute.
 * @param this Argument to pass to the task function.
 */
void thread(void (*fn)(), int nargs, ...);

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
 void orphan(void*(*fn)(void*), void *this);

/**
 * @brief Initialize the I/O subsystem.
 * 
 * - Initializes the global I/O queue.
 * - Creates an epoll instance for monitoring file descriptors.
 * - Launches a pool of I/O worker threads.
 * 
 * @return 0 on success, 1 on failure.
 */
int init_io();

/**
 * @brief Initialize scheduler threads.
 * 
 * - Allocates and initializes kernel_thread_t structures.
 * - Initializes each scheduler's local ready queue.
 * - Creates threads running the scheduler_run() loop.
 * 
 * @return 0 on success.
 */
int init_scheduler();


/**
 * @brief Cleanup resources used by the scheduler.
 * 
 * Currently frees only the first kernel thread. In production, all
 * threads and queues should be properly deallocated.
 */
void clean_scheduler();

/**
 * @brief wait for all schedulers to join.
 */
int wait_for_schedulers();

#endif