// TCP__Client__Low__HP.cpp

#include "TCP__Client.hpp"
#include "Concurrent_Queue__Blocking.hpp"
#include "Thread_Pool__Blocking.hpp"

using namespace BA_Socket;
using namespace BA_Concurrency;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: TCP__Client__Low__HP hostname port\n");
        return 1;
    }
    
    using EL_t = Event_Loop__Low__HP_t<Concurrent_Queue__Blocking, Thread_Pool__Blocking>;
    std::string hostname = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
    int status = TCP__Client<EL_t>(hostname, port, AF_INET6);
    return status;
}
