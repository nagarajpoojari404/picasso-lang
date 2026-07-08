#include "platform.h"
#include <stdatomic.h>
#include <assert.h>
#include <pthread.h>
#include "sync.h"
#include "scheduler.h"
#include "sync.h"
#include "alloc.h"

extern __thread arena_t* __arena__;
           
/**
 * @brief Create a new read-write mutex
 * @return Pointer to the created read-write mutex
 */
__public__rwmutex_t* __public__sync_rwmutex_create(void) {
    assert(__arena__ != NULL);
    __public__rwmutex_t* mux = (__public__rwmutex_t*)allocate(__arena__, sizeof(__public__rwmutex_t));
    safe_q_init(&mux->readers, SCHEDULER_LOCAL_QUEUE_SIZE);
    safe_q_init(&mux->writers, SCHEDULER_LOCAL_QUEUE_SIZE);

    pthread_mutex_init(&mux->lock, NULL);
    atomic_store_explicit(&mux->state, 0, memory_order_relaxed);

    return mux;
}

/**
 * @brief Acquire a read lock on the read-write mutex
 * @param mux Pointer to the read-write mutex
 */
void __public__sync_rwmutex_rlock(__public__rwmutex_t* mux) {
    assert(mux != NULL);
    int64_t old;

    for (;;) {
        old = atomic_load_explicit(&mux->state, memory_order_seq_cst);

        if (old & WRITER_BIT) {
            pthread_mutex_lock(&mux->lock);
            if (atomic_load_explicit(&mux->state, memory_order_seq_cst) & WRITER_BIT) {
                safe_q_push(&mux->readers, current_task);
                pthread_mutex_unlock(&mux->lock);
                task_yield(kernel_thread_map[current_task->sched_id]);
                continue;
            }
            pthread_mutex_unlock(&mux->lock);
            continue;
        }

        if (atomic_compare_exchange_weak_explicit(
                &mux->state,
                &old,
                old + READER_INC,
                memory_order_seq_cst,
                memory_order_relaxed)) {
            return;
        }
    }
}

/**
 * @brief Release a read lock on the read-write mutex
 * @param mux Pointer to the read-write mutex
 */
void __public__sync_rwmutex_runlock(__public__rwmutex_t* mux) {
    assert(mux != NULL);
    int64_t prev = atomic_fetch_sub_explicit(
        &mux->state,
        READER_INC,
        memory_order_release
    );

    // Last reader and a writer waiting
    if ((prev - READER_INC) == WRITER_BIT) {
        pthread_mutex_lock(&mux->lock);
        task_t *w = safe_q_pop(&mux->writers);
        if (w) {
            safe_q_push(&kernel_thread_map[w->sched_id]->ready_q, w);
        }
        pthread_mutex_unlock(&mux->lock);
    }
}

/**
 * @brief Acquire a write lock on the read-write mutex
 * @param mux Pointer to the read-write mutex
 */
void __public__sync_rwmutex_rwlock(__public__rwmutex_t* mux) {
    assert(mux != NULL);
    int64_t old;

    for (;;) {
        old = atomic_load_explicit(&mux->state, memory_order_seq_cst);
        if (old & WRITER_BIT) {
            pthread_mutex_lock(&mux->lock);
            if (atomic_load_explicit(&mux->state, memory_order_seq_cst) & WRITER_BIT) {

                safe_q_push(&mux->writers, current_task);
                pthread_mutex_unlock(&mux->lock);

                task_yield(kernel_thread_map[current_task->sched_id]);
                continue;
            }

            pthread_mutex_unlock(&mux->lock);
            continue;
        }

        if (atomic_compare_exchange_weak_explicit(
                &mux->state,
                &old,
                old | WRITER_BIT,
                memory_order_seq_cst,
                memory_order_relaxed)) {
            break;
        }
    }

    // FLAW: If this task has higher priority than the remaining readers, 
    // task_yield might return here immediately, and readers will never finish.
    while (atomic_load_explicit(&mux->state, memory_order_seq_cst) != WRITER_BIT) {
        task_yield(kernel_thread_map[current_task->sched_id]);
    }
}

/**
 * @brief Release a write lock on the read-write mutex
 * @param mux Pointer to the read-write mutex
 */
void __public__sync_rwmutex_rwunlock(__public__rwmutex_t* mux) {
    assert(mux != NULL);
    pthread_mutex_lock(&mux->lock);
    
    // FLAW: Setting state to 0 here allows new readers to "barge in" 
    // before the woken writer below even gets a chance to run.
    atomic_store_explicit(&mux->state, 0, memory_order_release);
    
    task_t *w = safe_q_pop(&mux->writers);
    if (w) {
        safe_q_push(&kernel_thread_map[w->sched_id]->ready_q, w);
        pthread_mutex_unlock(&mux->lock);
        return;
    }
    
    task_t *r;
    while ((r = safe_q_pop(&mux->readers)) != NULL) {
        safe_q_push(&kernel_thread_map[r->sched_id]->ready_q, r);
    }
    
    pthread_mutex_unlock(&mux->lock);
}

/**
 * @brief Create a new mutex
 * @return Pointer to the created mutex
 */
__public__mutex_t* __public__sync_mutex_create(void) {
    __public__mutex_t* mux = (__public__mutex_t*)allocate(__arena__, sizeof(__public__mutex_t));
    pthread_mutex_init(&mux->lock, NULL);
    safe_q_init(&mux->waiters, SCHEDULER_LOCAL_QUEUE_SIZE);
    atomic_store_explicit(&mux->state, 0, memory_order_relaxed);
    return mux;
}

/**
 * @brief Acquire a lock on the mutex
 * @param mtx Pointer to the mutex
 */
void __public__sync_mutex_lock(__public__mutex_t* mtx) {
    assert(mtx != NULL);
    int64_t expected = 0;

    // Fast path: try to grab lock immediately
    if (atomic_compare_exchange_strong_explicit(
            &mtx->state,
            &expected,
            1,
            memory_order_seq_cst,
            memory_order_relaxed)) {
        return; // acquired lock
    }

    // Slow path: lock is held, must enqueue and sleep
    for (;;) {
        // Mark ourselves waiting and sleep atomically
        pthread_mutex_lock(&mtx->lock);

        expected = atomic_load_explicit(&mtx->state, memory_order_seq_cst);
        if (expected == 0) {
            // Lock became free while we acquired mtx->lock
            atomic_store_explicit(&mtx->state, 1, memory_order_seq_cst);
            pthread_mutex_unlock(&mtx->lock);
            return;
        }

        // Enqueue and yield to scheduler
        safe_q_push(&mtx->waiters, current_task);
        pthread_mutex_unlock(&mtx->lock);
        task_yield(kernel_thread_map[current_task->sched_id]);
    }
}

/**
 * @brief Release a lock on the mutex
 * @param mtx Pointer to the mutex
 */
void __public__sync_mutex_unlock(__public__mutex_t* mtx) {
    assert(mtx != NULL);
    pthread_mutex_lock(&mtx->lock);

    // Mark the mutex as free
    atomic_store_explicit(&mtx->state, 0, memory_order_release);

    // Wake one waiting task, if any
    task_t *t = safe_q_pop(&mtx->waiters);
    if (t) {
        // Give the lock to this task immediately
        safe_q_push(&kernel_thread_map[t->sched_id]->ready_q, t);
    }

    pthread_mutex_unlock(&mtx->lock);
}


/**
 * @brief Create a new wait group
 * @return Pointer to the created wait group
 */
__public__waitgroup_t* __public__sync_waitgroup_create(void) {
    assert(__arena__ != NULL);
    __public__waitgroup_t* wg = (__public__waitgroup_t*)allocate(__arena__, sizeof(__public__waitgroup_t));
    safe_q_init(&wg->waiters, SCHEDULER_LOCAL_QUEUE_SIZE);

    pthread_mutex_init(&wg->lock, NULL);
    atomic_store_explicit(&wg->count, 0, memory_order_relaxed);

    return wg;
}

/**
 * @brief Add delta to the WaitGroup counter
 * @param wg Pointer to the wait group
 * @param delta Value to add to the counter (can be negative)
 *
 * If the counter becomes zero, all waiting tasks are woken up.
 * If the counter becomes negative, the program will assert.
 */
void __public__sync_waitgroup_add(__public__waitgroup_t* wg, int64_t delta) {
    assert(wg != NULL);
    
    int64_t new_count = atomic_fetch_add_explicit(&wg->count, delta, memory_order_acq_rel);
    new_count += delta;
    
    // counter must not go negative
    assert(new_count >= 0 && "WaitGroup counter cannot be negative");
    
    // if counter reached zero, wake up all waiting tasks
    if (new_count == 0 && delta < 0) {
        pthread_mutex_lock(&wg->lock);
        
        task_t *waiter;
        while ((waiter = safe_q_pop(&wg->waiters)) != NULL) {
            safe_q_push(&kernel_thread_map[waiter->sched_id]->ready_q, waiter);
        }
        
        pthread_mutex_unlock(&wg->lock);
    }
}

/**
 * @brief Decrement the WaitGroup counter by one
 * @param wg Pointer to the wait group
 *
 * Equivalent to calling Add(-1). When the counter reaches zero,
 * all tasks blocked in Wait are released.
 */
void __public__sync_waitgroup_done(__public__waitgroup_t* wg) {
    assert(wg != NULL);
    __public__sync_waitgroup_add(wg, -1);
}

/**
 * @brief Block until the WaitGroup counter is zero
 * @param wg Pointer to the wait group
 *
 * Wait blocks the calling task until the WaitGroup counter is zero.
 * If the counter is already zero, Wait returns immediately.
 */
void __public__sync_waitgroup_wait(__public__waitgroup_t* wg) {
    assert(wg != NULL);

    for (;;) {
        // Check if counter is already zero (fast path)
        int64_t count = atomic_load_explicit(&wg->count, memory_order_seq_cst);
        if (count == 0) {
            return;
        }
        
        // slow path: enqueue ourselves and yield
        pthread_mutex_lock(&wg->lock);
        
        // double-check after acquiring lock
        count = atomic_load_explicit(&wg->count, memory_order_seq_cst);
        if (count == 0) {
            pthread_mutex_unlock(&wg->lock);
            return;
        }
        
        safe_q_push(&wg->waiters, current_task);
        pthread_mutex_unlock(&wg->lock);
        
        task_yield(kernel_thread_map[current_task->sched_id]);
    }
}
