#include "platform.h"


#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <netinet/tcp.h>  
#include <netdb.h>         // addrinfo, getaddrinfo, AI_PASSIVE
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "netpoll.h"

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <signal.h>

#include "netio.h"
#include "task.h"
#include "scheduler.h"
#include "initutils.h"
#include "queue.h"

extern __thread task_t *current_task;
extern netpoll_t* netpoller;
extern __thread arena_t* __arena__;

/**
 * @brief Asynchronously accept a new connection on a listening socket.
 *
 * Registers the listening file descriptor with netpoll for read events and
 * suspends the current task until an incoming connection is ready to be
 * accepted. The task is resumed by the scheduler once the accept operation
 * completes or fails.
 *
 * This function integrates with the task scheduler and netpoll using
 * EPOLLONESHOT semantics. It may yield execution and resume later.
 *
 * @param listen_fd Listening socket file descriptor.
 *
 * @return Accepted socket file descriptor on success, or -1 on error
 *         (with errno set accordingly).
 */
ssize_t __public__net_accept(int64_t listen_fd) {
    task_t *t = current_task;

    t->io = (io_metadata_t){
        .fd = (int)listen_fd,
        .op = IO_ACCEPT,
        .io_done = 0,
        .io_err = 0,
    };

    if (netpoll_add(netpoller, (int)listen_fd, NETPOLL_IN | NETPOLL_ONESHOT, t) < 0) {
        if (errno != EEXIST)
            return -1;
        netpoll_mod(netpoller, (int)listen_fd, NETPOLL_IN | NETPOLL_ONESHOT, t);
    }

    unsafe_ioq_push(&kernel_thread_map[t->sched_id]->wait_q, t);

    task_yield(kernel_thread_map[t->sched_id]);
    errno = t->io.io_err;
    return t->io.done_n;
}

/**
 * @brief Asynchronously read data from a file descriptor.
 *
 * Registers the file descriptor with netpoll for read readiness and suspends
 * the current task until data is available or an error occurs. The read may
 * complete partially; the actual number of bytes read is returned when the
 * task resumes.
 *
 * This function cooperates with the scheduler and netpoll using EPOLLONESHOT
 * semantics and may yield execution until the I/O operation completes.
 *
 * @param fd File descriptor to read from.
 * @param buf Destination buffer.
 * @param len Maximum number of bytes to read.
 *
 * @return Number of bytes read on success, or -1 on error
 *         (with errno set accordingly).
 */
ssize_t __public__net_read(int64_t fd, __public__array_t *buf, size_t len) {
    task_t *t = current_task;

    t->io = (io_metadata_t){
        .fd = (int)fd,
        .buf = buf->data,
        .req_n = (ssize_t)len,
        .offset = 0,
        .op = IO_READ,
        .io_done = 0,
        .io_err = 0,
    };

    if (netpoll_add(netpoller, (int)fd, NETPOLL_IN | NETPOLL_ONESHOT, t) < 0) {
        if (errno != EEXIST)
            return -1;
        netpoll_mod(netpoller, (int)fd, NETPOLL_IN | NETPOLL_ONESHOT, t);
    }

    unsafe_ioq_push(&kernel_thread_map[t->sched_id]->wait_q, t);

    task_yield(kernel_thread_map[t->sched_id]);
    errno = t->io.io_err;
    return t->io.done_n;
}

/**
 * @brief Asynchronously write data to a file descriptor.
 *
 * Registers the file descriptor with netpoll for write readiness and suspends
 * the current task until the descriptor becomes writable or an error occurs.
 * The write may complete partially; the actual number of bytes written is
 * returned when the task resumes.
 *
 * This function integrates with the scheduler and netpoll using EPOLLONESHOT
 * semantics and may yield execution until the I/O operation completes.
 *
 * @param fd File descriptor to write to.
 * @param buf Source buffer.
 * @param len Number of bytes to write.
 *
 * @return Number of bytes written on success, or -1 on error
 *         (with errno set accordingly).
 */
ssize_t __public__net_write(int64_t fd, __public__array_t *buf, size_t len) {
    task_t *t = current_task;

    t->io = (io_metadata_t){
        .fd = (int)fd,
        .buf = buf->data,
        .req_n = (ssize_t)len,
        .offset = 0,
        .op = IO_WRITE,
        .io_done = 0,
        .io_err = 0,
    };

    if (netpoll_add(netpoller, (int)fd, NETPOLL_OUT | NETPOLL_ONESHOT, t) < 0) {
        if (errno != EEXIST)
            return -1;
        netpoll_mod(netpoller, (int)fd, NETPOLL_OUT | NETPOLL_ONESHOT, t);
    }

    unsafe_ioq_push(&kernel_thread_map[t->sched_id]->wait_q, t);

    task_yield(kernel_thread_map[t->sched_id]);
    errno = t->io.io_err;
    return t->io.done_n;
}

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
    int tcp_defer_accept __attribute__((unused)),
    int tcp_fastopen,
    int keepalive,
    int rcvbuf,
    int sndbuf,
    int ipv6_only
) {
    char host[128] = {0};

    if (addr && addr->data && addr->size) {
        size_t n = addr->size < sizeof(host) - 1 ? addr->size : sizeof(host) - 1;
        memcpy(host, addr->data, n);
    } else {
        strcpy(host, "0.0.0.0");
    }

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%u", port);

    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    if (getaddrinfo(host, port_str, &hints, &res) != 0)
        return -1;

    int fd = -1;

    for (struct addrinfo *ai = res; ai; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0)
            continue;

        int f = fcntl(fd, F_GETFL, 0);
        if (f < 0 || fcntl(fd, F_SETFL, f | O_NONBLOCK) < 0)
            goto fail;

        if (close_on_exec) {
            int fd_flags = fcntl(fd, F_GETFD, 0);
            if (fd_flags < 0 || fcntl(fd, F_SETFD, fd_flags | FD_CLOEXEC) < 0)
                goto fail;
        }

        if (reuse_addr) {
            int yes = 1;
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        }

#ifdef SO_REUSEPORT
        if (reuse_port) {
            int yes = 1;
            setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
        }
#endif

        if (rcvbuf > 0)
            setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

        if (sndbuf > 0)
            setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));

        if (keepalive) {
            int yes = 1;
            setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));
        }

        if (ai->ai_family == AF_INET6 && ipv6_only) {
#ifdef IPV6_V6ONLY
            int yes = 1;
            setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes));
#endif
        }

        if (bind(fd, ai->ai_addr, ai->ai_addrlen) == 0)
            break;

    fail:
        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);

    if (fd < 0)
        return -1;

    /* TCP options AFTER bind, BEFORE listen */
    if (tcp_nodelay) {
        int yes = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
    }

/* supported only in linux ig*/
#ifdef TCP_DEFER_ACCEPT
if (tcp_defer_accept > 0) {
    setsockopt(fd, IPPROTO_TCP,
        TCP_DEFER_ACCEPT,
        &tcp_defer_accept,
        sizeof(tcp_defer_accept));
    }
#endif
    
/* supported only in linux ig*/
#ifdef TCP_FASTOPEN
    if (tcp_fastopen > 0) {
        setsockopt(fd, IPPROTO_TCP,
                   TCP_FASTOPEN,
                   &tcp_fastopen,
                   sizeof(tcp_fastopen));
    }
#endif

    int bl = backlog > 0 ? backlog : SOMAXCONN;
    if (listen(fd, bl) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

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
ssize_t __public__net_dial(__public__string_t *addr, uint16_t port) {
    int fd = -1;
    struct addrinfo hints = {0}, *res = NULL, *ai;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%u", port);

    hints.ai_family   = AF_UNSPEC;      // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(addr->data, port_str, &hints, &res) != 0)
        return -1;

    for (ai = res; ai; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0)
            continue;

        /* non-blocking */
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
            goto fail;

        /* close-on-exec */
        if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0)
            goto fail;

        int r = connect(fd, ai->ai_addr, ai->ai_addrlen);
        if (r == 0) {
            /* connected immediately */
            freeaddrinfo(res);
            return fd;
        }

        if (errno != EINPROGRESS)
            goto fail;

        /* async connect */
        task_t *t = current_task;

        struct sockaddr *sa = allocate(__arena__, ai->ai_addrlen);
        socklen_t *salen = allocate(__arena__, sizeof(socklen_t));

        if (!sa || !salen) {
            errno = ENOMEM;
            goto fail;
        }

        memcpy(sa, ai->ai_addr, ai->ai_addrlen);
        *salen = ai->ai_addrlen;

        t->io = (io_metadata_t){
            .fd       = fd,
            .buf      = NULL,
            .req_n    = 0,
            .offset   = 0,
            .op       = IO_CONNECT,
            .io_done  = 0,
            .io_err   = 0,
            .addr     = sa,
            .addrlen  = salen,
        };

        if (netpoll_add(netpoller, fd,
                        NETPOLL_OUT | NETPOLL_ONESHOT, t) < 0) {
            if (errno != EEXIST)
                goto fail;
            netpoll_mod(netpoller, fd,
                        NETPOLL_OUT | NETPOLL_ONESHOT, t);
        }

        unsafe_ioq_push(&kernel_thread_map[t->sched_id]->wait_q, t);
        task_yield(kernel_thread_map[t->sched_id]);

        /* connect completion check */
        int err = 0;
        socklen_t len = sizeof(err);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
            errno = err ? err : errno;
            goto fail;
        }

        freeaddrinfo(res);
        return fd;

    fail:
        if (fd >= 0) close(fd);
        fd = -1;
    }

    freeaddrinfo(res);
    return -1;
}


/**
 * @brief Network I/O worker thread event loop.
 *
 * Runs an netpoll-based event loop that drives asynchronous network I/O for
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
void *netio_worker(void *arg __attribute__((unused))) {
    /* prevent SIGPIPE from killing process */
    signal(SIGPIPE, SIG_IGN);

    netpoll_t* npoll = netpoll_create();
    netpoller = npoll;
    netpoll_event_t events[128];

    for (;;) {
        int n = netpoll_wait(npoll, events, 128, -1);
        if (n < 0)
            continue;

        for (int i = 0; i < n; i++) {
            task_t *t = (task_t*)events[i].ud;
            t->io.io_err = 0;

            switch (t->io.op) {
            case IO_LISTEN:
                /* IO_LISTEN should not reach here in normal flow */
                break;

            case IO_CONNECT: {
                int err = 0;
                socklen_t len = sizeof(err);

                if (getsockopt(t->io.fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
                    t->io.io_err = errno;
                    t->io.done_n = -1;
                    netpoll_del(npoll, t->io.fd);
                } else if (err != 0) {
                    t->io.io_err = err;
                    t->io.done_n = -1;
                    netpoll_del(npoll, t->io.fd);
                } else {
                    /* Connected successfully */
                    t->io.done_n = t->io.fd;
                }

                t->io.io_done = 1;
                safe_q_push(&kernel_thread_map[t->sched_id]->ready_q, t);
                break;
            }


            case IO_ACCEPT: {
                // int cfd = accept4(
                //     t->io.fd,
                //     t->io.addr,
                //     t->io.addrlen,
                //     SOCK_NONBLOCK | SOCK_CLOEXEC
                // );

                int cfd = accept(t->io.fd, t->io.addr, t->io.addrlen);
                if (cfd >= 0) {
                    // 1. Set SOCK_NONBLOCK equivalent (O_NONBLOCK)
                    int flags = fcntl(cfd, F_GETFL, 0);
                    fcntl(cfd, F_SETFL, flags | O_NONBLOCK);

                    // 2. Set SOCK_CLOEXEC equivalent (FD_CLOEXEC)
                    fcntl(cfd, F_SETFD, FD_CLOEXEC);
                }

                if (cfd < 0) {
                    if (errno == EAGAIN) {
                        netpoll_mod(npoll, t->io.fd, NETPOLL_IN | NETPOLL_ONESHOT, t);
                        break;
                    }
                    t->io.io_err = errno;
                    t->io.done_n = -1;
                } else {
                    t->io.done_n = cfd;
                }

                t->io.io_done = 1;
                safe_q_push(&kernel_thread_map[t->sched_id]->ready_q, t);
                break;
            }

            case IO_READ: {
                ssize_t r = recv(
                    t->io.fd,
                    (char *)t->io.buf + t->io.offset,
                    (size_t)(t->io.req_n - t->io.offset),
                    0   // no flags: identical to read() for TCP
                );

                if (r > 0) {
                    t->io.offset += r;
                    t->io.done_n = t->io.offset;
                } else if (r == 0) {
                    /* Peer performed orderly shutdown (TCP FIN) */
                    t->io.done_n = t->io.offset;
                    netpoll_del(npoll, t->io.fd);
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        netpoll_mod(npoll, t->io.fd, NETPOLL_IN | NETPOLL_ONESHOT, t);
                        break;
                    }
                    t->io.io_err = errno;
                    t->io.done_n = -1;
                    netpoll_del(npoll, t->io.fd);
                }

                assert(t->io.req_n >= t->io.done_n);

                t->io.io_done = 1;
                safe_q_push(&kernel_thread_map[t->sched_id]->ready_q, t);
                break;
            }

            case IO_WRITE: {
                ssize_t w = send(
                    t->io.fd,
                    (char *)t->io.buf + t->io.offset,
                    (size_t)(t->io.req_n - t->io.offset),
                    MSG_NOSIGNAL
                );

                if (w < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        netpoll_mod(npoll, t->io.fd, NETPOLL_OUT | NETPOLL_ONESHOT, t);
                        break;
                    }
                    t->io.io_err = errno;
                    t->io.done_n = -1;
                    netpoll_del(npoll, t->io.fd);
                } else {
                    t->io.offset += w;
                    t->io.done_n = t->io.offset;
                    if (t->io.offset < t->io.req_n) {
                        netpoll_mod(npoll, t->io.fd, NETPOLL_OUT | NETPOLL_ONESHOT, t);
                        break;
                    }
                }

                t->io.io_done = 1;
                safe_q_push(&kernel_thread_map[t->sched_id]->ready_q, t);
                break;
            }
            }
        }
    }
    netpoll_destroy(npoll);
}
