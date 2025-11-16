#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "Socket.h"
#ifdef _WIN32
    #include <conio.h>
#endif

namespace ba_socket {
    void create_TCP_client(int argc, char *argv[]) {
        SOCKET_STARTUP();





        SOCKET_CLEANUP();
    }
} // namespace ba_socket

#endif // TCP_CLIENT_H
