#ifndef DISKIO_H
#define DISKIO_H

#include "platform.h"
#include "stdio.h"

#include "queue.h"
#include "str.h"
#include "task.h"
#include "array.h"


/** Number of I/O worker threads in the pool, must be kept equal to number of scheduler threads */
#define DISKIO_THREAD_POOL_SIZE 4

/**  @depricated: Maximum number of events returned by epoll_wait at once */
#define MAX_EVENTS 16

/**  @depricated: Maximum number of tasks in the I/O queue */
#define DISKIO_QUEUE_SIZE 256

/** Queue depth */
#define DISKIO_QUEUE_DEPTH  256

/** @deprecated: io_uring submission done when req hits this threshold */
#define SUBMIT_THRESHOLD (256/2)

#if defined(__linux__)
extern struct io_uring **diskio_ring_map;

/**
 * @brief Worker thread that waits for completed I/O events from io_uring.
 *
 * This thread continuously polls for completion events from the io_uring
 * submission queue. When an event completes, it retrieves the associated
 * task (stored in CQE user data), updates its `nread` field with the result,
 * and pushes it back into the scheduler's ready queue.
 *
 * @param arg Unused.
 * @return void* Always returns NULL.
 */
void *diskio_worker(void *arg);

/**
 * @brief Submit an asynchronous STDIN read for the current task.
 *
 * Prepares and submits an io_uring read request on STDIN for the
 * current task’s buffer. The read follows normal read(2) semantics:
 * it may complete with fewer bytes than requested and may block
 * internally if STDIN is a TTY.
 *
 * This function does NOT modify STDIN file descriptor flags
 * (e.g. O_NONBLOCK) and does not provide true asynchronous behavior
 * for terminal input. The calling task explicitly yields and will be
 * resumed by the scheduler once the io_uring completion is processed
 * by the I/O worker thread.
 *
 * Completion status (bytes read or error) is stored in the task
 * structure and must be examined after the task resumes.
 *
 * @return void
 */
void async_stdin_read(task_t *t) ;

/**
 * @brief Submit an io_uring write request to STDOUT for the current task.
 *
 * Prepares and submits a write request for the current task’s buffer
 * to STDOUT at the current file offset. The task yields execution
 * to allow other tasks to run while the I/O completes.
 *
 * Notes:
 *  - Partial writes are possible; actual bytes written are stored in
 *    current_task->done_n after completion.
 *  - I/O errors are reported via current_task->io_err.
 *  - The task will be resumed by the scheduler once the I/O worker
 *    processes the completion.
 *  - True asynchronous non-blocking behavior depends on the underlying file descriptor
 *    (TTY writes may still block internally).
 *  - The function does **not** exit the process on SQE allocation or submission failure;
 *    errors are propagated to the task structure instead.
 *
 * @return void
 */
void async_stdout_write();

/**
 * @brief Submit an io_uring read request on a file descriptor for the current task.
 *
 * Prepares and submits a read request for the current task’s buffer
 * from the specified file descriptor at the given offset. The task
 * yields execution to allow other tasks to run while the I/O completes.
 *
 * Notes:
 *  - Partial reads are possible; the actual number of bytes read is
 *    stored in current_task->done_n after completion.
 *  - I/O errors are reported via current_task->io_err.
 *  - The task will be resumed by the scheduler once the I/O worker
 *    processes the completion.
 *
 * @return void
 */
void async_file_read(void);

/**
 * @brief Submit an io_uring write request on a file descriptor for the current task.
 *
 * Prepares and submits a write request for the current task’s buffer
 * to the specified file descriptor at the given offset. The task
 * yields execution to allow other tasks to run while the I/O completes.
 *
 * Notes:
 *  - Partial writes may occur; actual bytes written are stored in
 *    current_task->done_n.
 *  - I/O errors are reported via current_task->io_err.
 *  - The task will be resumed by the scheduler once the I/O worker
 *    processes the completion.
 *  - True asynchronous non-blocking behavior depends on the file type.
 *
 * @return void
 */
void async_file_write();

/**
 * @brief Read up to n bytes from STDIN and suspend the current task.
 *
 * Allocates a buffer and submits an io_uring read request on STDIN for the
 * current task. The task yields execution and is resumed by the scheduler
 * once the I/O completion is processed by the I/O worker thread.
 *
 * The read follows read(2) semantics: it may complete with fewer than n
 * bytes and does not guarantee true asynchronous behavior for terminal
 * input. The returned buffer is NUL-terminated based on the number of
 * bytes actually read.
 *
 * @param n Maximum number of bytes to read.
 *
 * @return Pointer to the allocated buffer on success, or NULL on error.
 */
__public__array_t* __public__asyncio_scan(int n);

/**
 * @brief Format and write output to STDOUT, suspending the current task until done.
 *
 * Formats the input string and arguments into a dynamically allocated buffer,
 * sets up the current task for an io_uring write to STDOUT, and submits it.
 * The task yields execution and will be resumed by the scheduler once the
 * write completes.
 *
 * Note:
 *  - The write may be partial; actual bytes written are stored in
 *    current_task->done_n.
 *  - The returned buffer is owned by the task/runtime; do not free manually.
 *  - True asynchronous non-blocking behavior is only possible for
 *    non-TTY fds.
 *
 * @param fmt Format string (printf-style).
 * @param ... Arguments matching the format string.
 *
 * @return Number of bytes successfully written on success, or -1 on error.
 */
ssize_t __public__asyncio_printf(__public__string_t* fmt, ...);

/**
 * @brief Read up to n bytes from a file at a given offset, suspending the current task.
 *
 * Configures the current task with the file descriptor, buffer, byte count,
 * and offset, then submits an io_uring read request. The task yields execution
 * and will be resumed by the scheduler once the read completes.
 *
 * Notes:
 *  - Partial reads are possible; actual bytes read are stored in
 *    current_task->done_n.
 *  - I/O errors are reported via current_task->io_err.
 *  - True asynchronous non-blocking behavior depends on the file type.
 *
 * @param f      FILE pointer to read from.
 * @param buf    Buffer to store read data.
 * @param n      Maximum number of bytes to read.
 * @param offset File offset to start reading from.
 *
 * @return Number of bytes read on success (ssize_t), or -1 on error.
 */
ssize_t __public__asyncio_fread(__public__string_t* f, __public__array_t* buf, int n, int offset);

/**
 * @brief Write up to n bytes to a file at a given offset, suspending the current task.
 *
 * Configures the current task with the file descriptor, buffer, byte count,
 * and offset, then submits an io_uring write request. The task yields execution
 * and will be resumed by the scheduler once the write completes.
 *
 * Notes:
 *  - Partial writes are possible; actual bytes written are stored in
 *    current_task->done_n.
 *  - I/O errors are reported via current_task->io_err.
 *  - True asynchronous non-blocking behavior depends on the file type.
 *
 * @param f      FILE pointer to write to.
 * @param buf    Buffer containing data to write.
 * @param n      Maximum number of bytes to write.
 * @param offset File offset to start writing from.
 *
 * @return Number of bytes successfully written on success, or -1 on error.
 */
ssize_t __public__asyncio_fwrite(__public__string_t* f, __public__array_t* buf, int n, int offset);
#endif
/**
 * @brief Synchronously read up to n bytes from STDIN.
 *
 * Allocates a buffer of size n+1 and performs a blocking read() from STDIN.
 * Notes on behavior:
 *  - When reading from a TTY, input is typically line-buffered. The read() syscall
 *    may return fewer than n bytes as soon as a line is available.
 *  - If read() is interrupted by a signal (errno == EINTR), the function retries
 *    the read automatically.
 *  - The returned buffer is NUL-terminated at the position corresponding to
 *    the number of bytes actually read.
 *
 * @param n Maximum number of bytes to read.
 *
 * @return Pointer to the allocated buffer containing the read data on success,
 *         or NULL on allocation failure or read error.
 */
__public__array_t* __public__syncio_scan(int n);

/**
 * @brief Synchronously write formatted output to STDOUT.
 *
 * Formats the input string and arguments into a dynamically allocated buffer,
 * then writes the entire result to STDOUT using the write() syscall. The function
 * ensures that all bytes are written, retrying as needed in case of partial writes
 * or EINTR interruptions.
 *
 * Notes:
 *  - The buffer is NUL-terminated internally for formatting purposes,
 *    but the NUL byte is not written to STDOUT.
 *  - The function may perform multiple write() syscalls to complete the output.
 *
 * @param fmt Format string (printf-style).
 * @param ... Arguments matching the format string.
 *
 * @return Number of bytes successfully written on success, or -1 on error.
 */
ssize_t __public__syncio_printf(__public__string_t* fmt, ...);

/**
 * @brief Synchronously read up to n bytes from a file at a given offset.
 *
 * Reads from the specified FILE pointer into the provided buffer using the
 * pread() syscall. The function handles partial reads and EINTR interruptions,
 * and continues reading until either n bytes are read or end-of-file is reached.
 *
 * Notes:
 *  - The file descriptor offset is updated internally after each read.
 *  - Partial reads are handled transparently; the function loops until
 *    the requested byte count is satisfied or EOF occurs.
 *  - The buffer is not NUL-terminated; it contains raw file data.
 *
 * @param f      FILE pointer to read from.
 * @param buf    Buffer to store read data.
 * @param n      Maximum number of bytes to read.
 * @param offset File offset to start reading from.
 *
 * @return Number of bytes actually read on success (0 indicates EOF),
 *         or -1 on error.
 */
ssize_t __public__syncio_fread(__public__string_t* f, __public__array_t* buf, int n, int offset);

/**
 * @brief Synchronously write up to n bytes to a file at a given offset.
 *
 * Writes data from the provided buffer to the specified FILE pointer using
 * the pwrite() syscall. The function handles partial writes and EINTR
 * interruptions, looping until either all n bytes are written or an error occurs.
 *
 * Notes:
 *  - The file offset is updated internally for each pwrite() call.
 *  - Partial writes are handled transparently; the function ensures that
 *    the full requested byte count is written unless an error occurs.
 *  - The buffer is not modified by the function.
 *
 * @param f      FILE pointer to write to.
 * @param buf    Buffer containing data to write.
 * @param n      Number of bytes to write.
 * @param offset File offset to start writing from.
 *
 * @return Number of bytes actually written on success,
 *         or -1 on error.
 */
ssize_t __public__syncio_fwrite(__public__string_t* f, __public__array_t* buf, int n, int offset);

__public__string_t* __public__syncio_get_stdout();

/**
 * @brief Synchronously open a file.
 *
 * Opens the specified file with the given mode using fopen().
 *
 * @param filename Path to the file to open.
 * @param mode     File access mode (e.g., "r", "w", "a", "rb", "wb").
 *
 * @return Pointer to the opened FILE on success, or NULL on error.
 */
__public__string_t* __public__syncio_fopen(__public__string_t* filename, __public__string_t* mode);

/**
 * @brief Synchronously close a file.
 *
 * Closes the specified FILE pointer using fclose().
 *
 * @param f FILE pointer to close.
 *
 * @return 0 on success, or -1 on error.
 */
int64_t __public__syncio_fclose(__public__string_t* f);

/**
 * @brief Synchronously seek to a position in a file.
 *
 * Sets the file position indicator for the specified FILE pointer using fseek().
 *
 * @param f      FILE pointer to seek in.
 * @param offset Number of bytes to offset from whence.
 * @param whence Position to offset from (SEEK_SET, SEEK_CUR, or SEEK_END).
 *
 * @return 0 on success, or -1 on error.
 */
int64_t __public__syncio_fseek(__public__string_t* f, int64_t offset, int whence);

/**
 * @brief Synchronously flush a file's output buffer.
 *
 * Flushes any buffered output data for the specified FILE pointer using fflush().
 *
 * @param f FILE pointer to flush.
 *
 * @return 0 on success, or -1 on error.
 */
int64_t __public__syncio_fflush(__public__string_t* f);
#endif