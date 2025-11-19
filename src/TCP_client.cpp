// TCP_client.cpp

#include "TCP_client.h"
using namespace ba_socket;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: tcp_client hostname port\n");
        return 1;
    }
    
    std::string hostname = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
    int status = TCP_client(hostname, port, AF_INET6);
    return status;
}
