#ifndef GC_H
#define GC_H

#include "platform.h"
#include <pthread.h>
#include <stdatomic.h>
#include "scheduler.h" // external: runtime


#define GC_TIMEPERIOD (100 * 1000000)
#define MAX_ARENAS 12
#define MAX_SCHEDULERS 12 

#ifndef GC_PTR_ALIGNMENT
#define GC_PTR_ALIGNMENT 8
#endif
#define GC_ALIGN_MASK (GC_PTR_ALIGNMENT - 1)

    
typedef struct gc_state {
    atomic_int       world_stopped;      // 0 = running, 1 = requested stop
    atomic_int       stopped_count;      // number of threads that have stopped
    pthread_mutex_t  lock;
    pthread_cond_t   cv_mutators_stopped;
    pthread_cond_t   cv_world_resumed;

    // add_lock protects adding total_threads
    pthread_mutex_t add_lock;
    atomic_int        total_threads;      // mutators only
} gc_state_t;

/**
 * @brief Create the global arena for garbage collection.
 * @return Pointer to the created global arena.
 */
arena_t* gc_create_global_arena();

/**
 * @brief Create a new arena for garbage collection.
 * @return Pointer to the created arena.
 */
arena_t* gc_create_arena();

/**
 * @brief Register a task as a GC root.
 * @param t Task to register as root.
 */
void gc_register_root(task_t* t);

/**
 * @brief Unregister a task as a GC root.
 * @param t Task to unregister as root.
 */
void gc_unregister_root(task_t* t);

/**
 * @brief Initialize the garbage collector.
 */
void gc_init();

/**
 * @brief Start the garbage collector.
 */
void gc_start();

/**
 * @brief Stop all mutator threads for garbage collection.
 */
void gc_stop_the_world();

/**
 * @brief Resume all mutator threads after garbage collection.
 */
void gc_resume_world();
#endif