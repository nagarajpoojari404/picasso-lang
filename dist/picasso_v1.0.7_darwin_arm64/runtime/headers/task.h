#ifndef TASK_H
#define TASK_H

#include "platform.h"

#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/socket.h>    
#include <netinet/in.h>   
#include <arpa/inet.h>      
#include <pthread.h>      
#include <stdint.h>        
#include <sys/types.h> 

#include "platform/context.h"

/* Size of per-task I/O buffer (bytes) */
#define TASK_IO_BUFFER 256

typedef enum {
    IO_LISTEN,
    IO_ACCEPT,
    IO_READ,
    IO_WRITE,
    IO_CONNECT,
} netio_op_t;

typedef struct {
    /* File descriptor if the task performs I/O (otherwise -1) */
    int fd;

    /* Buffer for I/O operations */
    char *buf;

    /* Number of bytes requested to read/write */
    ssize_t req_n;

    /* Number of bytes actually read or written */
    ssize_t done_n;

    /* seek offset */
    ssize_t offset;

    /* error number if io fails */
    int io_err;

    volatile int io_done;

    /* netio operation enum */
    netio_op_t op;
    
    /* socket address */
    struct sockaddr *addr;

    /* socket address length */
    socklen_t *addrlen;
    
} io_metadata_t;

typedef struct {
    void (*fn)();
    int nargs;
    ffi_cif cif;
    ffi_type **arg_types;
    void **arg_values;
} task_payload_t;

typedef enum {
    TASK_RUNNING, 
    TASK_YIELDED,
    TASK_FINISHED
} task_state_t;

// forward declaration;
struct task;

typedef struct wait_q_metadata {
    struct wait_q_metadata* fd;
    struct wait_q_metadata* bk;
    struct task* t;
} wait_q_metadata_t;

/**
 * @struct task_t
 * @brief Represents a single task/coroutine managed by the scheduler.
 * 
 * Tasks have their own stack, CPU context, and optionally perform I/O operations.
 * They are scheduled cooperatively or preemptively on a kernel thread.
 */
typedef struct task {

    /* Unique identifier for the task */
    int id;

    /* CPU context used for saving/restoring execution state */
    platform_ctx_t ctx;

    /* Size of the private stack (usable bytes, excluding guard page) */
    size_t stack_size;

    /* Scheduler/kernel thread ID that owns this task */
    int sched_id;

    /* Function to execute when task is scheduled */
    void (*fn)(void *);

    /* Pointer to the task's private stack (after guard page) */
    char *stack;

    task_state_t state;

    io_metadata_t io;

    wait_q_metadata_t* wq;

    wait_q_metadata_t* gcq;
} task_t;

#endif