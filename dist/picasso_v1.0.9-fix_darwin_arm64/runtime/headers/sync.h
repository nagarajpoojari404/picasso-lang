#ifndef SYNC_H
#define SYNC_H

#include "platform.h"
#include <stdalign.h>

#include "stdatomic.h"
#include "stdint.h"
#include "scheduler.h"
#include "queue.h"

#define WRITER_BIT   1
#define READER_INC   2

typedef struct __public__rwmutex {
    _Atomic int64_t state;
    pthread_mutex_t lock;      // per-mutex lock to protect enqueue + sleep
    safe_queue_t readers;
    safe_queue_t writers;
}__public__rwmutex_t;


/**
 * @brief Create a new read-write mutex
 * @return Pointer to the created read-write mutex
 */
__public__rwmutex_t* __public__sync_rwmutex_create();

/**
 * @brief Acquire a read lock on the read-write mutex
 * @param mux Pointer to the read-write mutex
 */
void __public__sync_rwmutex_rlock(__public__rwmutex_t* mux);

/**
 * @brief Acquire a write lock on the read-write mutex
 * @param mux Pointer to the read-write mutex
 */
void __public__sync_rwmutex_rwlock(__public__rwmutex_t* mux);

/**
 * @brief Release a read lock on the read-write mutex
 * @param mux Pointer to the read-write mutex
 */
void __public__sync_rwmutex_runlock(__public__rwmutex_t* mux);

/**
 * @brief Release a write lock on the read-write mutex
 * @param mux Pointer to the read-write mutex
 */
void __public__sync_rwmutex_rwunlock(__public__rwmutex_t* mux);



typedef struct __public__mutex {
    _Atomic int64_t state;
    pthread_mutex_t lock;
    safe_queue_t waiters;
}__public__mutex_t;

/**
 * @brief Create a new mutex
 * @return Pointer to the created mutex
 */
__public__mutex_t* __public__sync_mutex_create(void);

/**
 * @brief Acquire a lock on the mutex
 * @param mtx Pointer to the mutex
 */
void __public__sync_mutex_lock(__public__mutex_t* mtx);

/**
 * @brief Release a lock on the mutex
 * @param mtx Pointer to the mutex
 */
void __public__sync_mutex_unlock(__public__mutex_t* mtx);

typedef struct __public__waitgroup {
    _Atomic int64_t count;
    pthread_mutex_t lock;
    safe_queue_t waiters;
}__public__waitgroup_t;

/**
 * @brief Create a new wait group
 * @return Pointer to the created wait group
 */
__public__waitgroup_t* __public__sync_waitgroup_create();

/**
 * @brief Add delta to the WaitGroup counter
 * @param wg Pointer to the wait group
 * @param delta Value to add to the counter (can be negative)
 *
 * Add adds delta, which may be negative, to the WaitGroup counter.
 * If the counter becomes zero, all tasks blocked on Wait are released.
 * If the counter goes negative, Add panics (asserts).
 */
void __public__sync_waitgroup_add(__public__waitgroup_t* wg, int64_t delta);

/**
 * @brief Decrement the WaitGroup counter by one
 * @param wg Pointer to the wait group
 *
 * Done decrements the WaitGroup counter by one.
 * Equivalent to calling Add(-1).
 */
void __public__sync_waitgroup_done(__public__waitgroup_t* wg);

/**
 * @brief Block until the WaitGroup counter is zero
 * @param wg Pointer to the wait group
 *
 * Wait blocks until the WaitGroup counter is zero.
 * If the counter is already zero, Wait returns immediately.
 */
void __public__sync_waitgroup_wait(__public__waitgroup_t* wg);

#endif