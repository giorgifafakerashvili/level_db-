#ifndef _FDEVENT_H_
#define _FDEVENT_H_

struct server; /* declaration */

typedef hadler_t (*fdevent_handler)(struct server* srv, void* ctx, int revents);

/**
 * These are the POLL* values from <bits/poll.h> (linux poll)
 */

#define FDEVENT_IN     BV(0)
#define FDEVENT_PRI    BV(1)
#define FDEVENT_OUT    BV(2)
#define FDEVENT_ERR    BV(3)
#define FDEVENT_HUP    BV(4)
#define FDEVENT_NVAL   BV(5)

#define FDEVENT_STREAM_REQUEST          BV(0)
#define FDEVENT_STREAM_REQUEST_BUFMIN   BV(1)
#define FDEVENT_STREAM_REQUEST_POLLIN   BV(15)

#define FDEVENT_STREAM_RESPONSE         BV(0)
#define FDEVENT_STREAM_RESPONSE_BUFMIN  BV(1)

typedef enum {
    FD_EVENT_TYPE_UNSET = -1,
    FD_EVENT_TYPE_CONNECTION,
    FD_EVENT_TYPE_FCGI_CONNECTION,
    FD_EVENT_TYPE_DIRWATCH,
    FD_EVENT_TYPE_CGI_CONNECTION
} fd_event_t;

typedef enum { FDEVENT_HANDLER_UNSET,
    FDEVENT_HANDLER_SELECT,
    FDEVENT_HANDLER_POLL,
    FDEVENT_HANDLER_LINUX_SYSEPOLL,
    FDEVENT_HANDLER_SOLARIS_DEVPOLL,
    FDEVENT_HANDLER_SOLARIS_PORT,
    FDEVENT_HANDLER_FREEBSD_KQUEUE,
    FDEVENT_HANDLER_LIBEV
} fdevent_handler_t

typedef struct _fdnode {
    fdevent_handler handler;
    void* ctx;
    void* handler_ctx;
    int fd;
    int events;
} fdnode;

/**
 * Array of unused fd's
 */
 typedef struct {
     int* ptr;

     size_t used;
     size_t size;
 } buffer_int;

/**
 * fd_event handler for select(), poll() and rt-signals on Linux
 */
 typedef struct fdevents {
     struct server* srv;
     fdevent_handler_t type;

     fdnode** fdarray;
     size_t maxfds;
     int highfd;

#ifdef USE_LINUX_POLL
     int epoll_fd;
     struct epoll_event* epoll_events;
#endif // USE_LINUX_POLL

#ifdef USE_POLL
    struct pollfd* pollfds;

    size_t size;
    size_t used;

    buffer_int unused;

#endif

#ifdef USE_SELECT
    fd_set select_read;
    fd_set select_write;
    fd_set select_error;

    fd_set select_set_read;
    fd_set select_set_write;
    fd_set select_set_error;

#endif

#ifdef USER_SOLARIS_DEVPOLL
    int devpoll_fd;
    struct pollfd* devpollfds;
#endif

#ifdef USE_FREEBSD_KQUEUE
    int kq_fd;
    struct kevent* kq_results;
#endif

#ifdef USE_SOLARIS_PORT
    int port_fd;
#endif

    int (*reset)(struct fdevents* ev);
    void (*free)(struct fdevents* ev);

    int (*event_set)(struct fdevents* ev, int fd_ndx, int fd, int events);
    int (*event_del)(struct fdevents *ev, int fde_ndx, int fd);
    int (*event_get_revent)(struct fdevents* ev, size_t ndx);
    int (*event_get_fd)(struct fdevents* ev, size_t ndx);

    int (*event_next_fdndx)(struct fdevents* ev, int ndx);

    int (*poll)(struct fdevents* ev, int timeout_ms);

    int (*fcntl_set)(struct fdevents* ev, int fd);


 } fdevents;


fdevents* fdevent_init(struct server* srv, size_t maxfds, int type);
int fdevent_reset(struct fdevents* ev); /* intit after fork() */
void fdevent_free(fdevents* ev);

#define fdevent_event_get_interest(ev, fd) \
    ((fd) >= 0 ? (ev)->fdarray[(fd)]->events : 0)

void fdevent_event_set(fdevents* ev, int* fd_ndx, int fd, int events);
void fdevent_event_add(fdevents *ev, int *fde_ndx, int fd, int event); /* events can be FDEVENT_IN or FDEVENT_OUT */
void fdevent_event_clr(fdevents *ev, int *fde_ndx, int fd, int event); /* events can be FDEVENT_IN or FDEVENT_OUT */
void fdevent_event_del(fdevents *ev, int *fde_ndx, int fd);
int fdevent_event_get_revent(fdevents *ev, size_t ndx);
int fdevent_event_get_fd(fdevents *ev, size_t ndx);
fdevent_handler fdevent_get_handler(fdevents *ev, int fd);
void * fdevent_get_context(fdevents *ev, int fd);


int fdevent_event_next_fdndx(fdevents *ev, int ndx);
int fdevent_poll(fdevents *ev, int timeout_ms);


int fdevent_register(fdevents *ev, int fd, fdevent_handler handler, void *ctx);
int fdevent_unregister(fdevents *ev, int fd);
void fdevent_sched_close(fdevents *ev, int fd, int issock);
void fdevent_sched_run(struct server *srv, fdevents *ev);



#define fd_close_on_exec(fd) fdevent_setfd_cloexec(fd)
void fdevent_setfd_cloexec(int fd);
void fdevent_clrfd_cloexec(int fd);
int fdevent_fcntl_set(fdevents *ev, int fd);
int fdevent_fcntl_set_nb(fdevents *ev, int fd);
int fdevent_fcntl_set_nb_cloexec(fdevents *ev, int fd);
int fdevent_fcntl_set_nb_cloexec_sock(fdevents *ev, int fd);
int fdevent_socket_cloexec(int domain, int type, int protocol);
int fdevent_socket_nb_cloexec(int domain, int type, int protocol);
int fdevent_open_cloexec(const char *pathname, int flags, mode_t mode);


struct sockaddr;
int fdevent_accept_listenfd(int listenfd, struct sockaddr *addr, size_t *addrlen);

int fdevent_select_init(fdevents *ev);
int fdevent_poll_init(fdevents *ev);
int fdevent_linux_sysepoll_init(fdevents *ev);
int fdevent_solaris_devpoll_init(fdevents *ev);
int fdevent_solaris_port_init(fdevents *ev);
int fdevent_freebsd_kqueue_init(fdevents *ev);
int fdevent_libev_init(fdevents *ev);


int fdevent_ioctl_fionread (int fd, int fdfmt, int *toread);

int fdevent_is_tcp_half_closed(int fd);


#endif // _FD_EVENT_H_