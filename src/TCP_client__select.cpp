// TCP_client__select.cpp

#include "TCP_client__select.hpp"
using namespace BA_Socket;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: tcp_client__select hostname port\n");
        return 1;
    }
    
    std::string hostname = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
    int status = TCP_client__select(hostname, port, AF_INET6);
    return status;
}
