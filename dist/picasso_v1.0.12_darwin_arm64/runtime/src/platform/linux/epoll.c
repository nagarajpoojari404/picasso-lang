#include "netpoll.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <assert.h>

static uint32_t to_epoll_events(netpoll_eventflag_t ev){
    uint32_t e = 0;

    if (ev & NETPOLL_IN)  e |= EPOLLIN;
    if (ev & NETPOLL_OUT) e |= EPOLLOUT;
    if (ev & NETPOLL_ERR) e |= EPOLLERR;
    if (ev & NETPOLL_HUP) e |= EPOLLHUP;
    if (ev & NETPOLL_ONESHOT) e |= EPOLLONESHOT;

    return e;
}

static netpoll_eventflag_t from_epoll_events(uint32_t ev){
    netpoll_eventflag_t e = 0;

    if (ev & EPOLLIN)  e |= NETPOLL_IN;
    if (ev & EPOLLOUT) e |= NETPOLL_OUT;
    if (ev & EPOLLERR) e |= NETPOLL_ERR;
    if (ev & EPOLLHUP) e |= NETPOLL_HUP;

    return e;
}

/**
 * @brief Create a new network poller instance
 * @return Pointer to the created netpoll_t structure, or NULL on failure
 */
netpoll_t* netpoll_create(void) {
    netpoll_t* p = calloc(1, sizeof(*p));
    if (!p)
        return NULL;

    p->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (p->epfd < 0) {
        free(p);
        return NULL;
    }

    return p;
}

/**
 * @brief Destroy a network poller instance and free its resources
 * @param p Pointer to the netpoll_t structure to destroy
 */
void netpoll_destroy(netpoll_t* p) {
    if (!p) return;

    close(p->epfd);
    free(p);
}

/**
 * @brief Add a file descriptor to an epoll instance.
 *
 * Registers the file descriptor with the given epoll instance using
 * EPOLLONESHOT and associates the provided task pointer with the event.
 *
 * @param epfd Epoll instance file descriptor.
 * @param fd File descriptor to add.
 * @param t Task associated with this epoll entry.
 * @param events Epoll events to monitor (e.g., EPOLLIN, EPOLLOUT).
 *
 * @return 0 on success, -1 on failure (errno set).
 */
int netpoll_add( netpoll_t* p, int fd, netpoll_eventflag_t events, netpoll_ud_t ud) {
    struct epoll_event ev = {
        .events   = to_epoll_events(events),
        .data.ptr = ud,
    };

    assert(p != NULL);
    return epoll_ctl(p->epfd, EPOLL_CTL_ADD, fd, &ev);
}

/**
 * @brief Modify an existing epoll registration.
 *
 * Updates the event mask for a previously registered file descriptor and
 * re-arms it as EPOLLONESHOT, keeping the associated task pointer.
 *
 * @param epfd Epoll instance file descriptor.
 * @param fd File descriptor to modify.
 * @param t Task associated with this epoll entry.
 * @param events New epoll events to monitor.
 *
 * @return 0 on success, -1 on failure (errno set).
 */
int netpoll_mod( netpoll_t* p, int fd, netpoll_eventflag_t events, netpoll_ud_t ud) {
    struct epoll_event ev = {
        .events   = to_epoll_events(events),
        .data.ptr = ud,
    };

    return epoll_ctl(p->epfd, EPOLL_CTL_MOD, fd, &ev);
}

/**
 * @brief Remove a file descriptor from an epoll instance.
 *
 * Unregisters the file descriptor from the epoll instance. Any pending
 * events for the descriptor are discarded.
 *
 * @param epfd Epoll instance file descriptor.
 * @param fd File descriptor to remove.
 */
int netpoll_del(netpoll_t* p, int fd){
    return epoll_ctl(p->epfd, EPOLL_CTL_DEL, fd, NULL);
}

/**
 * @brief Wait for events on monitored file descriptors
 * @param p Pointer to the netpoll_t structure
 * @param events Array to store triggered events
 * @param max_events Maximum number of events to return
 * @param timeout_ms Timeout in milliseconds (-1 for infinite)
 * @return Number of events triggered, or -1 on error
 */
int netpoll_wait( netpoll_t* p, netpoll_event_t* events, int max_events, int timeout_ms) {
    struct epoll_event evs[128];

    if (max_events > (int)(sizeof(evs) / sizeof(evs[0])))
        max_events = (int)(sizeof(evs) / sizeof(evs[0]));

    int n = epoll_wait(p->epfd, evs, max_events, timeout_ms);
    if (n <= 0)
        return n;

    for (int i = 0; i < n; i++) {
        events[i].fd     = -1;
        events[i].events = from_epoll_events(evs[i].events);
        events[i].ud     = evs[i].data.ptr;
    }

    return n;
}

