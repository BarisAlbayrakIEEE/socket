// poll_setup.hpp

#ifndef POLL_SETUP_HPP
#define POLL_SETUP_HPP

#include "socket_setup.hpp"

#if defined(_WIN32)
    #include <mstcpip.h>

    using pollfd_x  = WSAPOLLFD;
    using nfds_x    = ULONG;
    using timeout_x = INT;

    inline int poll_x(pollfd_x* fds, nfds_x nfds, timeout_x timeout) {
        return ::WSAPoll(fds, nfds, timeout);
    }

    // Cross-platform event flag names (same as your current aliases)
    static constexpr short POLL_X_IN   = POLLRDNORM;
    static constexpr short POLL_X_OUT  = POLLWRNORM;
    static constexpr short POLL_X_ERR  = POLLERR;
    static constexpr short POLL_X_HUP  = POLLHUP;
    static constexpr short POLL_X_NVAL = POLLNVAL;
#else
    #include <poll.h>

    using pollfd_x  = struct pollfd;
    using nfds_x    = nfds_t;
    using timeout_x = int;

    inline int poll_x(pollfd_x* fds, nfds_x nfds, timeout_x timeout) {
        return ::poll(fds, nfds, timeout);
    }

    static constexpr short POLL_X_IN   = POLLIN;
    static constexpr short POLL_X_OUT  = POLLOUT;
    static constexpr short POLL_X_ERR  = POLLERR;
    static constexpr short POLL_X_HUP  = POLLHUP;
    static constexpr short POLL_X_NVAL = POLLNVAL;
#endif // _WIN32

#endif // POLL_SETUP_HPP
