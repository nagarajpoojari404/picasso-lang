#include "platform.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

#include "queue.h"
#include "task.h"
#include "alloc.h"
#include "gc.h"

extern arena_t* __global__arena__;
/**
 * @brief Initialize a thread-safe queue.
 * 
 * Sets head/tail to NULL, initializes mutex and condition variable,
 * and sets the maximum size of the queue.
 * 
 * @param q Pointer to the queue to initialize.
 * @param size Maximum number of elements allowed in the queue.
 */
void safe_q_init(safe_queue_t *q, int size) {
    q->head = q->tail = NULL;
    q->size_limit = size;
    // initialize lock
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);
}

/**
 * @brief Push a task onto the queue in a thread-safe manner.
 * 
 * Wakes up any threads waiting for a task. If the queue has a size limit,
 * the caller should handle blocking or dropping tasks as needed.
 * 
 * @param q Pointer to the queue.
 * @param t Task to push.
 */
void safe_q_push(safe_queue_t *q, task_t *t) {
    task_node_t *n = allocate(__global__arena__, sizeof(task_node_t)); /* need to be freed */
    n->t = t; n->next = NULL;

    pthread_mutex_lock(&q->lock);
    
    if (!q->tail)
        q->head = q->tail = n;
    else {
        q->tail->next = n;
        q->tail = n;
    }

    // signal only one consumer thread about the availability
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
}

/**
 * @brief Pop a task from the queue in a non-blocking way.
 * 
 * If the queue is empty, returns NULL immediately.
 * 
 * @param q Pointer to the queue.
 * @return Pointer to the task, or NULL if queue is empty.
 */
task_t *safe_q_pop(safe_queue_t *q) {
    pthread_mutex_lock(&q->lock);

    task_node_t *n = q->head;
    if (!n) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }
    q->head = n->next;
    if (!q->head)
        q->tail = NULL;
    pthread_mutex_unlock(&q->lock);

    task_t *t = n->t;

    release(__global__arena__,n);
    return t;
}

/**
 * @brief Pop a task from the queue in a blocking way.
 * 
 * If the queue is empty, the calling thread waits until a task becomes available.
 * Never returns NULL.
 * 
 * @param q Pointer to the queue.
 * @return Pointer to the next task in the queue.
 */
task_t *safe_q_pop_wait(safe_queue_t *q) {
    pthread_mutex_lock(&q->lock);

    // suspend if queue is empty
    // release lock & wait for signal from push
    while (!q->head) 
        pthread_cond_wait(&q->cond, &q->lock);


    task_node_t *n = q->head;
    q->head = n->next;
    if (!q->head)
        q->tail = NULL;

    pthread_mutex_unlock(&q->lock);
    task_t *t = n->t;

    release(__global__arena__, n);
    return t;
}

/**
 * @brief Initialize a thread-unsafe queue.
 * 
 * @param q Pointer to the queue to initialize.
 * @param size Maximum number of elements allowed in the queue.
 */
void unsafe_ioq_init(unsafe_queue_t *q, int size) {
    q->head = NULL;
    q->size_limit = size;
}

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
void unsafe_ioq_push(unsafe_queue_t *q, task_t *t) {
    wait_q_metadata_t *n = allocate(__global__arena__, sizeof(wait_q_metadata_t)); /* must be freed later */
    if (!n) abort();   /* optional but sane */
    n->t = t;
    t->wq = n; 

    /* empty queue */
    if (!q->head) {
        n->fd = n;
        n->bk = n;
        q->head = n;
        return;
    }

    wait_q_metadata_t *head = q->head;

    n->fd = head;        
    n->bk = head->bk;    

    head->bk->fd = n;     
    head->bk = n;        
    q->head = n; 
    
}

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
int unsafe_ioq_remove(unsafe_queue_t *q, task_t *t) {
    wait_q_metadata_t *wq = t->wq;
    if (!wq || !q->head)
        return 0;

    if (wq->fd == wq) {
        /* single element */
        q->head = NULL;
    } else {

        wq->fd->bk = wq->bk;
        wq->bk->fd = wq->fd;

        if (q->head == wq)
            q->head = wq->fd;
    }

    wq->fd = NULL;
    wq->bk = NULL;
    t->wq = NULL;

    release(__global__arena__, wq);
    return 1;
}

/**
 * @brief Initialize a thread-safe queue.
 * 
 * @param q Pointer to the queue to initialize.
 * @param size Maximum number of elements allowed in the queue.
 */
void safe_gcq_init(safe_gcqueue_t *q, int size) {
    q->head = NULL;
    q->size_limit = size;

    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);
}

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
void safe_gcq_push(safe_gcqueue_t *q, task_t *t) {
    wait_q_metadata_t *n = allocate(__global__arena__, sizeof(wait_q_metadata_t)); /* must be freed later */
    if (!n) abort();   /* optional but sane */
    n->t = t;
    t->gcq = n; 

    pthread_mutex_lock(&q->lock);
    /* empty queue */
    if (!q->head) {
        n->fd = n;
        n->bk = n;
        q->head = n;

        // signal only one consumer thread about the availability
        pthread_cond_signal(&q->cond);
        pthread_mutex_unlock(&q->lock);
        return;
    }

    wait_q_metadata_t *head = q->head;

    n->fd = head;        
    n->bk = head->bk;    

    head->bk->fd = n;     
    head->bk = n;        
    q->head = n; 

    // signal only one consumer thread about the availability
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
}

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
int safe_gcq_remove(safe_gcqueue_t *q, task_t *t) {
    pthread_mutex_lock(&q->lock);

    wait_q_metadata_t *gcq = t->gcq;
    if (!gcq || !q->head)
        return 0;

    if (gcq->fd == gcq) {
        /* single element */
        q->head = NULL;
        pthread_mutex_unlock(&q->lock);
    } else {

        gcq->fd->bk = gcq->bk;
        gcq->bk->fd = gcq->fd;

        if (q->head == gcq)
            q->head = gcq->fd;
    }

    gcq->fd = NULL;
    gcq->bk = NULL;
    t->gcq = NULL;

    pthread_mutex_unlock(&q->lock);

    release(__global__arena__, gcq);
    return 1;
}

