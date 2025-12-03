// TCP__Client__Mid__ST.cpp

#include "TCP__Client.hpp"

using namespace BA_Socket;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: TCP__Client__Mid__ST hostname port\n");
        return 1;
    }
    
    using EL_t = Event_Loop__Mid__ST_t;
    std::string hostname = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
    int status = TCP__Client<EL_t>(hostname, port, AF_INET6);
    return status;
}
