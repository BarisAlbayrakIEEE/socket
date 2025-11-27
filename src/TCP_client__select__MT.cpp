// TCP_client__select__MT.cpp

#include "TCP_client__select__MT.hpp"
using namespace BA_Socket;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: TCP_client__select__MT hostname port\n");
        return 1;
    }
    
    std::string hostname = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
    int status = TCP_client__select__ST(hostname, port, AF_INET6);
    return status;
}
