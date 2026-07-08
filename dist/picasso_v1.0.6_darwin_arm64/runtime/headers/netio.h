#ifndef NETIO_H
#define NETIO_H

#include "platform.h"
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h> 

#include "str.h"
#include "array.h"

/** Number of I/O worker threads in the pool, must be kept equal to number of scheduler threads */
#define NETIO_THREAD_POOL_SIZE 1

/** Queue depth */
#define NETIO_QUEUE_DEPTH  256

/**
 * @brief Network I/O worker thread event loop.
 *
 * Runs an epoll-based event loop that drives asynchronous network I/O for
 * tasks. The worker waits for I/O readiness events, performs the requested
 * operation (accept, read, or write), and reschedules tasks back onto their
 * originating scheduler’s ready queue upon completion.
 *
 * Epoll is used with EPOLLONESHOT semantics; file descriptors are explicitly
 * re-armed on EAGAIN or partial progress. This function runs indefinitely and
 * is expected to be executed by a dedicated kernel / I/O worker thread.
 *
 * @param arg Worker identifier (cast from intptr_t).
 *
 * @return Never returns.
 */
void *netio_worker(void *arg);

/**
 * @brief Asynchronously accept a new connection on a listening socket.
 *
 * Registers the listening file descriptor with epoll for read events and
 * suspends the current task until an incoming connection is ready to be
 * accepted. The task is resumed by the scheduler once the accept operation
 * completes or fails.
 *
 * This function integrates with the task scheduler and epoll using
 * EPOLLONESHOT semantics. It may yield execution and resume later.
 *
 * @param listen_fd Listening socket file descriptor.
 *
 * @return Accepted socket file descriptor on success, or -1 on error
 *         (with errno set accordingly).
 */
ssize_t __public__net_accept(int64_t epfd);

/**
 * @brief Asynchronously read data from a file descriptor.
 *
 * Registers the file descriptor with epoll for read readiness and suspends
 * the current task until data is available or an error occurs. The read may
 * complete partially; the actual number of bytes read is returned when the
 * task resumes.
 *
 * This function cooperates with the scheduler and epoll using EPOLLONESHOT
 * semantics and may yield execution until the I/O operation completes.
 *
 * @param fd File descriptor to read from.
 * @param buf Destination buffer.
 * @param len Maximum number of bytes to read.
 *
 * @return Number of bytes read on success, or -1 on error
 *         (with errno set accordingly).
 */
ssize_t __public__net_read(int64_t fd, __public__array_t *buf, size_t len);

/**
 * @brief Asynchronously write data to a file descriptor.
 *
 * Registers the file descriptor with epoll for write readiness and suspends
 * the current task until the descriptor becomes writable or an error occurs.
 * The write may complete partially; the actual number of bytes written is
 * returned when the task resumes.
 *
 * This function integrates with the scheduler and epoll using EPOLLONESHOT
 * semantics and may yield execution until the I/O operation completes.
 *
 * @param fd File descriptor to write to.
 * @param buf Source buffer.
 * @param len Number of bytes to write.
 *
 * @return Number of bytes written on success, or -1 on error
 *         (with errno set accordingly).
 */
ssize_t __public__net_write(int64_t fd, __public__array_t *buf, size_t len) ;

/**
 * @brief Create, bind, and listen on a TCP socket.
 *
 * Creates a non-blocking IPv4 TCP socket, binds it to the specified address
 * and port, and starts listening for incoming connections.
 *
 * The socket is created with SOCK_NONBLOCK and SOCK_CLOEXEC. SO_REUSEADDR
 * and SO_REUSEPORT are enabled by default.
 *
 * @param addr IPv4 address to bind to (e.g., "127.0.0.1" or "0.0.0.0").
 *             If NULL or "0.0.0.0", binds to INADDR_ANY.
 * @param port TCP port number (host byte order).
 * @param backlog Maximum length of the pending connection queue.
 *
 * @return Listening socket file descriptor on success, or -1 on error
 *         (with errno set accordingly).
 */
ssize_t __public__net_listen(
    __public__string_t *addr,
    uint16_t port,
    int backlog,
    int close_on_exec,
    int reuse_addr,
    int reuse_port,
    int tcp_nodelay,
    int tcp_defer_accept,
    int tcp_fastopen,
    int keepalive,
    int rcvbuf,
    int sndbuf,
    int ipv6_only
);


/**
 * @brief Connect to a remote TCP server.
 *
 * Creates a non-blocking IPv4 TCP socket and initiates a connection to the
 * specified address and port.
 *
 * The socket is created with SOCK_NONBLOCK and SOCK_CLOEXEC. The connection
 * is performed asynchronously and may complete immediately or require waiting
 * for writability.
 *
 * @param addr IPv4 address to connect to (e.g., "127.0.0.1").
 * @param port TCP port number (host byte order).
 *
 * @return Connected socket file descriptor on success, or -1 on error
 *         (with errno set accordingly).
 */
ssize_t __public__net_dial(__public__string_t *addr, uint16_t port);
#endif