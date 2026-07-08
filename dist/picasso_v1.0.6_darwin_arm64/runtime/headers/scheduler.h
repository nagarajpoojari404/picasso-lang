#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "platform.h"
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include "platform/context.h"

#include "task.h"
#include "queue.h"


/** 
 * Initial stack size allocated per task (in bytes).
 * Includes only the usable stack, not the guard page.
 */
#define STACK_SIZE (1024*1024)

/** 
 * Number of scheduler threads (kernel threads) in the pool.
 * Each thread manages its own local ready queue and executes tasks.
 */
#define SCHEDULER_THREAD_POOL_SIZE 12

/**
 * Maximum number of tasks in a scheduler thread's local queue.
 * Tasks pushed beyond this may need to go to a global queue or block.
 * @todo: use this
 */
#define SCHEDULER_LOCAL_QUEUE_SIZE 40240

/** 
 * Size of the guard page used for stack overflow detection (in bytes).
 */
#define GUARD_SIZE (4096)

/**
 * System page size (in bytes). Typically used for memory mapping and guard pages.
 * @warning: sysconf(_SC_PAGESIZE) is a runtime value, not a compile-time constant.
 */
#define PAGE_SIZE  sysconf(_SC_PAGESIZE)

/**
 * @struct kernel_thread
 * @brief Represents a kernel-level scheduler thread that manages a set of tasks.
 *
 * Each kernel_thread_t instance corresponds to one scheduler thread
 * that maintains its own ready queue and scheduling context.
 */
typedef struct kernel_thread {
    int id;                  /** Unique ID assigned to this scheduler thread. */
    platform_ctx_t sched_ctx;    /** Scheduler context for switching between tasks. */
    task_t *current;         /** Pointer to the currently running task (if any). */
    safe_queue_t ready_q;    /** Queue of ready tasks waiting to be scheduled. */
    unsafe_queue_t wait_q;  /** Queue of tasks which are waiting for IO */
} kernel_thread_t;


/** @fix: extern makes tight coupling */

/**
 * @owner: scheduler.c
 */
extern __thread task_t* current_task;
/**
 * @owner: main.c
 */
extern kernel_thread_t **kernel_thread_map;

/**
 * @brief Create a new task with its own protected stack and context.
 * 
 * @param fn   The function to run in the new task.
 * @param this Pointer argument passed to the task function.
 * @param kt   Pointer to the owning kernel thread (scheduler worker).
 * @return Pointer to the created task structure.
 */
task_t* task_create(void (*fn)(), void* payload, kernel_thread_t* kt);

/**
 * @brief Clean up a task and release its resources.
 * 
 * @param t Task to destroy.
 */
void task_destroy(task_t *t);

/**
 * @brief Entry trampoline used by tasks to invoke their function and clean up.
 * 
 * @param t Pointer to the current task.
 * @return Always returns NULL after task exits.
 */
void task_trampoline(uintptr_t _t, uintptr_t _p);


/**
 * @brief Yield execution from current task back to its scheduler thread.
 * 
 * @param kt The kernel thread running the task.
 */
void task_yield(kernel_thread_t* kt);

/**
 * @brief Resume a specific task on the given scheduler thread.
 * 
 * @param t  Task to resume.
 * @param kt The kernel thread executing the task.
 */
void task_resume(task_t *t, kernel_thread_t* kt);

/**
 * @brief Cooperative preemption check.
 * 
 * Called periodically (e.g., via timer) to allow preemptive multitasking.
 * If the current task’s preempt flag is set, it yields control.
 */
void self_yield(void);


/**
 * @brief Signal handler for SIGSEGV that grows task stacks dynamically.
 * 
 * Triggered when the task overflows its current stack and accesses the guard page.
 * Allocates a new, larger stack, copies old stack data, and updates the context.
 * 
 * @param sig     Signal number (SIGSEGV).
 * @param si      Signal info.
 * @param unused  Pointer to ucontext_t (processor state at time of fault).
 */
void more_stack(int sig, siginfo_t *si, void *unused);

/**
 * @brief Initialize alternate stack and install SIGSEGV handler.
 * 
 * This ensures the signal handler has a safe stack to run on if the current
 * task stack is corrupted or overflown.
 */
void init_stack_signal_handler(void);


/**
 * @brief Timer callback that forces preemption of a scheduler thread.
 * 
 * Sets the corresponding thread’s preempt flag to 1.
 * 
 * @param sv Signal value passed by the POSIX timer.
 */
void force_preempt(int sig, siginfo_t *si, void *uc);

/**
 * @brief Initialize per-thread timer signal handler.
 * 
 * Creates a periodic POSIX timer (SIGEV_THREAD) that triggers preemption
 * at fixed intervals for the current scheduler thread.
 * 
 * @param arg Pointer to the scheduler thread ID (int*).
 */
void init_timer_signal_handler(void *arg);

/**
 * @brief Main run loop for a scheduler worker thread.
 * 
 * Continuously executes ready tasks from the queue, handles preemption,
 * and waits for epoll I/O events to resume blocked tasks.
 * 
 * @param arg Pointer to the kernel_thread_t structure for this thread.
 * @return Never returns under normal operation.
 */
void* scheduler_run(void* arg);
#endif