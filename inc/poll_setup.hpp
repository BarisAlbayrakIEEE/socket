// poll_setup.hpp

#ifndef POLL_SETUP_HPP
#define POLL_SETUP_HPP

#include "socket_setup.hpp"
#if defined(_WIN32)
    #include <mstcpip.h>
#else
    #include <poll.h>
#endif
#include <vector>

// uniform event flags
#if defined(_WIN32)
    #define POLL_IN   POLLRDNORM
    #define POLL_OUT  POLLWRNORM
    #define POLL_ERR  POLLERR
    #define POLL_HUP  POLLHUP
    #define POLL_NVAL POLLNVAL

    using pollfd_t = WSAPOLLFD;
    inline int poll_execute(pollfd_t* fds, ULONG nfds, INT timeout) {
        return ::WSAPoll(fds, nfds, timeout);
    }
#else
    #define POLL_IN   POLLIN
    #define POLL_OUT  POLLOUT
    #define POLL_ERR  POLLERR
    #define POLL_HUP  POLLHUP
    #define POLL_NVAL POLLNVAL

    using pollfd_t = struct pollfd;
    inline int poll_execute(pollfd_t* fds, nfds_t nfds, int timeout) {
        return ::poll(fds, nfds, timeout);
    }
#endif

#endif // POLL_SETUP_HPP
