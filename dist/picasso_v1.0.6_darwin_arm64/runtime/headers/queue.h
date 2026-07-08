#ifndef QUEUE_H 
#define QUEUE_H
#include "platform.h"
#include <pthread.h>
#include "task.h"

/* Node in the task queue */
typedef struct task_node { 
    task_t *t;                 /* Pointer to the task */
    struct task_node *next;    /* Next node in the queue */
} task_node_t;

/* thread safe FIFO queue for tasks  */
typedef struct {
    int size_limit;            /* Maximum number of tasks in the queue */
    
    task_node_t *head, *tail;  /* Queue head and tail pointers */
    pthread_mutex_t lock;       /* Mutex protecting queue operations */
    pthread_cond_t cond;        /* Condition variable for waiting threads */
} safe_queue_t;

/**
 * @brief Initialize a thread-safe queue.
 * 
 * Sets head/tail to NULL, initializes mutex and condition variable,
 * and sets the maximum size of the queue.
 * 
 * @param q Pointer to the queue to initialize.
 * @param size Maximum number of elements allowed in the queue.
 */
void safe_q_init(safe_queue_t *q, int size);

/**
 * @brief Push a task onto the queue in a thread-safe manner.
 * 
 * Wakes up any threads waiting for a task. If the queue has a size limit,
 * the caller should handle blocking or dropping tasks as needed.
 * 
 * @param q Pointer to the queue.
 * @param t Task to push.
 */
void safe_q_push(safe_queue_t *q, task_t *t);

/**
 * @brief Pop a task from the queue in a non-blocking way.
 * 
 * If the queue is empty, returns NULL immediately.
 * 
 * @param q Pointer to the queue.
 * @return Pointer to the task, or NULL if queue is empty.
 */
task_t *safe_q_pop(safe_queue_t *q);

/**
 * @brief Pop a task from the queue in a blocking way.
 * 
 * If the queue is empty, the calling thread waits until a task becomes available.
 * Never returns NULL.
 * 
 * @param q Pointer to the queue.
 * @return Pointer to the next task in the queue.
 */
task_t *safe_q_pop_wait(safe_queue_t *q);


/* thread unsafe FIFO queue for tasks  */
typedef struct {
    int size_limit;        
    wait_q_metadata_t *head; 
} unsafe_queue_t;

/**
 * @brief Initialize a thread-unsafe queue.
 * 
 * @param q Pointer to the queue to initialize.
 * @param size Maximum number of elements allowed in the queue.
 */
void unsafe_ioq_init(unsafe_queue_t *q, int size);

/**
 * @brief Insert a task into an unsafe intrusive wait queue.
 *
 * Allocates and attaches wait-queue metadata for the given task and inserts it
 * into the queue. The queue is implemented as a circular doubly linked list,
 * and the new element becomes the head of the queue.
 *
 * This operation is non-blocking and performs no synchronization; the caller
 * must ensure external safety (e.g., single-threaded execution or proper
 * locking). This function does not enforce any queue size limits and does not
 * perform any wake-up or notification logic by itself.
 *
 * The allocated wait-queue metadata must be freed when the task is removed
 * from the queue.
 *
 * @param q Pointer to the queue.
 * @param t Pointer to the task to insert.
 */
void unsafe_ioq_push(unsafe_queue_t *q, task_t *t);

/**
 * @brief Remove a specific task from an unsafe wait queue.
 *
 * Removes the given task's wait-queue metadata from the queue if present.
 * This operation is non-blocking and performs no synchronization; the caller
 * must ensure external safety (e.g., single-threaded access or proper locking).
 *
 * If the task is not associated with a wait queue or the queue is empty,
 * the function does nothing.
 *
 * @param q Pointer to the queue from which the task should be removed.
 * @param t Pointer to the task to remove.
 *
 * @return 1 if the task was successfully removed, 0 otherwise.
 */
int unsafe_ioq_remove(unsafe_queue_t *q, task_t *t);

/* thread unsafe FIFO queue for tasks  */
typedef struct {
    int size_limit;        
    wait_q_metadata_t *head; 

    pthread_mutex_t lock;       /* Mutex protecting queue operations */
    pthread_cond_t cond;        /* Condition variable for waiting threads */
} safe_gcqueue_t;

/**
 * @brief Initialize a thread-safe queue.
 * 
 * @param q Pointer to the queue to initialize.
 * @param size Maximum number of elements allowed in the queue.
 */
void safe_gcq_init(safe_gcqueue_t *q, int size);


/**
 * @brief Insert a task into an safe intrusive wait queue.
 *
 * Allocates and attaches wait-queue metadata for the given task and inserts it
 * into the queue. The queue is implemented as a circular doubly linked list,
 * and the new element becomes the head of the queue.
 *
 * This operation is non-blocking and performs no synchronization; the caller
 * must ensure external safety (e.g., single-threaded execution or proper
 * locking). This function does not enforce any queue size limits and does not
 * perform any wake-up or notification logic by itself.
 *
 * The allocated wait-queue metadata must be freed when the task is removed
 * from the queue.
 *
 * @param q Pointer to the queue.
 * @param t Pointer to the task to insert.
 */
void safe_gcq_push(safe_gcqueue_t *q, task_t *t);

/**
 * @brief Remove a specific task from an safe wait queue.
 *
 * Removes the given task's wait-queue metadata from the queue if present.
 * This operation is non-blocking and performs no synchronization; the caller
 * must ensure external safety (e.g., single-threaded access or proper locking).
 *
 * If the task is not associated with a wait queue or the queue is empty,
 * the function does nothing.
 *
 * @param q Pointer to the queue from which the task should be removed.
 * @param t Pointer to the task to remove.
 *
 * @return 1 if the task was successfully removed, 0 otherwise.
 */
int safe_gcq_remove(safe_gcqueue_t *q, task_t *t);
#endif
