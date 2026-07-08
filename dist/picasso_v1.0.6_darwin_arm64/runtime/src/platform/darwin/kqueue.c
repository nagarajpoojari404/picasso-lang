#include "netpoll.h"

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/event.h>
#include <sys/time.h>

static int add_event( int kq, int fd, int16_t filter, uint16_t flags, netpoll_ud_t ud) {
    struct kevent kev;
    EV_SET(&kev, fd, filter, flags, 0, 0, ud);
    return kevent(kq, &kev, 1, NULL, 0, NULL);
}

/* convert kevent -> netpoll flags */
static netpoll_eventflag_t from_kqueue_events(const struct kevent* kev){
    netpoll_eventflag_t ev = 0;

    if (kev->filter == EVFILT_READ)
        ev |= NETPOLL_IN;
    if (kev->filter == EVFILT_WRITE)
        ev |= NETPOLL_OUT;

    if (kev->flags & EV_EOF)
        ev |= NETPOLL_HUP;
    if (kev->flags & EV_ERROR)
        ev |= NETPOLL_ERR;

    return ev;
}

static uint16_t to_kqueue_flags(netpoll_eventflag_t events){
    uint16_t flags = EV_ADD | EV_ENABLE;

    if (events & NETPOLL_ONESHOT)
        flags |= EV_ONESHOT;

    return flags;
}

/**
 * @brief Create a new network poller instance
 * @return Pointer to the created netpoll_t structure, or NULL on failure
 */
netpoll_t* netpoll_create(void){
    netpoll_t* p = calloc(1, sizeof(*p));
    if (!p)
        return NULL;

    p->kq = kqueue();
    if (p->kq < 0) {
        free(p);
        return NULL;
    }

    return p;
}

/**
 * @brief Destroy a network poller instance and free its resources
 * @param p Pointer to the netpoll_t structure to destroy
 */
void netpoll_destroy(netpoll_t* p){
    if (!p) return;

    close(p->kq);
    free(p);
}

/**
 * @brief Add a file descriptor to the network poller
 * @param p Pointer to the netpoll_t structure
 * @param fd File descriptor to add
 * @param events Event flags to monitor
 * @param ud User data associated with this file descriptor
 * @return 0 on success, non-zero on failure
 */
int netpoll_add( netpoll_t* p, int fd, netpoll_eventflag_t events, netpoll_ud_t ud) {
    uint16_t flags = to_kqueue_flags(events);

    if (events & NETPOLL_IN) {
        if (add_event(p->kq, fd, EVFILT_READ, flags, ud) < 0)
            return -1;
    }

    if (events & NETPOLL_OUT) {
        if (add_event(p->kq, fd, EVFILT_WRITE, flags, ud) < 0)
            return -1;
    }

    return 0;
}

/**
 * @brief Modify the events monitored for a file descriptor
 * @param p Pointer to the netpoll_t structure
 * @param fd File descriptor to modify
 * @param events New event flags to monitor
 * @param ud New user data associated with this file descriptor
 * @return 0 on success, non-zero on failure
 */
int netpoll_mod( netpoll_t* p, int fd, netpoll_eventflag_t events, netpoll_ud_t ud) {
    /* kqueue has no real MOD -> delete + add */
    netpoll_del(p, fd);
    return netpoll_add(p, fd, events, ud);
}

/**
 * @brief Remove a file descriptor from the network poller
 * @param p Pointer to the netpoll_t structure
 * @param fd File descriptor to remove
 * @return 0 on success, non-zero on failure
 */
int netpoll_del(netpoll_t* p, int fd){
    struct kevent kev[2];
    int n = 0;

    EV_SET(&kev[n++], fd, EVFILT_READ,  EV_DELETE, 0, 0, NULL);
    EV_SET(&kev[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);

    kevent(p->kq, kev, n, NULL, 0, NULL);
    return 0;
}

/**
 * @brief Wait for events on monitored file descriptors
 * @param p Pointer to the netpoll_t structure
 * @param events Array to store triggered events
 * @param max_events Maximum number of events to return
 * @param timeout_ms Timeout in milliseconds (-1 for infinite)
 * @return Number of events triggered, or -1 on error
 */
int netpoll_wait( netpoll_t* p, netpoll_event_t* out, int max_events, int timeout_ms) {
    struct kevent kev[128];

    if (max_events > (int)(sizeof(kev) / sizeof(kev[0])))
        max_events = (int)(sizeof(kev) / sizeof(kev[0]));

    struct timespec ts;
    struct timespec* tsp = NULL;

    if (timeout_ms >= 0) {
        ts.tv_sec  = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000L;
        tsp = &ts;
    }

    int n = kevent(p->kq, NULL, 0, kev, max_events, tsp);
    if (n <= 0)
        return n;

    for (int i = 0; i < n; i++) {
        out[i].fd     = (int)kev[i].ident;
        out[i].events = from_kqueue_events(&kev[i]);
        out[i].ud     = kev[i].udata;
    }

    return n;
}
