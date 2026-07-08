#ifndef NETPOLL_H
#define NETPOLL_H

#include <stdint.h>
#include <stddef.h>

#if defined(__linux__) 
    #include <sys/epoll.h>
    
    typedef struct netpoll {
        /* epoll file descriptor */
        int epfd;
    } netpoll_t;

#elif defined(__APPLE__)
    #include <sys/event.h> 

    typedef struct netpoll {
        /* kqueue file descriptor */
        int kq;
    } netpoll_t;
#endif


typedef void* netpoll_ud_t;

typedef enum {
    NETPOLL_IN   = 1 << 0,   /* readable */
    NETPOLL_OUT  = 1 << 1,   /* writable */
    NETPOLL_ERR  = 1 << 2,   /* error */
    NETPOLL_HUP  = 1 << 3,   /* hangup / eof */
    NETPOLL_ONESHOT = 1 << 4
} netpoll_eventflag_t;

typedef struct {
    int              fd;
    netpoll_eventflag_t events;
    netpoll_ud_t     ud;     /* task_t* */
} netpoll_event_t;

/**
 * @brief Create a new network poller instance
 * @return Pointer to the created netpoll_t structure, or NULL on failure
 */
netpoll_t* netpoll_create(void);

/**
 * @brief Destroy a network poller instance and free its resources
 * @param p Pointer to the netpoll_t structure to destroy
 */
void       netpoll_destroy(netpoll_t* p);

/**
 * @brief Add a file descriptor to the network poller
 * @param p Pointer to the netpoll_t structure
 * @param fd File descriptor to add
 * @param events Event flags to monitor
 * @param ud User data associated with this file descriptor
 * @return 0 on success, non-zero on failure
 */
int netpoll_add( netpoll_t* p, int fd, netpoll_eventflag_t events, netpoll_ud_t ud );

/**
 * @brief Modify the events monitored for a file descriptor
 * @param p Pointer to the netpoll_t structure
 * @param fd File descriptor to modify
 * @param events New event flags to monitor
 * @param ud New user data associated with this file descriptor
 * @return 0 on success, non-zero on failure
 */
int netpoll_mod( netpoll_t* p, int fd, netpoll_eventflag_t events, netpoll_ud_t ud);

/**
 * @brief Remove a file descriptor from the network poller
 * @param p Pointer to the netpoll_t structure
 * @param fd File descriptor to remove
 * @return 0 on success, non-zero on failure
 */
int netpoll_del( netpoll_t* p, int fd );

/**
 * @brief Wait for events on monitored file descriptors
 * @param p Pointer to the netpoll_t structure
 * @param events Array to store triggered events
 * @param max_events Maximum number of events to return
 * @param timeout_ms Timeout in milliseconds (-1 for infinite)
 * @return Number of events triggered, or -1 on error
 */
int netpoll_wait( netpoll_t* p, netpoll_event_t* events, int max_events, int timeout_ms);
#endif 
