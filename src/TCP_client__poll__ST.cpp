// TCP_client__poll__ST.cpp

#include "TCP_client__poll__ST.hpp"
using namespace BA_Socket;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: TCP_client__poll__ST hostname port\n");
        return 1;
    }
    
    std::string hostname = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
    int status = TCP_client__poll__ST(hostname, port, AF_INET6);
    return status;
}
