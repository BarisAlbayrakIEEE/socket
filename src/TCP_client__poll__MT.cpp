// TCP_client__poll__MT.cpp

#include "TCP_client__poll__MT.hpp"
#include "Concurrent_Queue__Blocking.hpp"
#include "Thread_Pool__Blocking.hpp"

using namespace BA_Socket;
using namespace BA_Concurrency;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: TCP_client__poll__MT hostname port\n");
        return 1;
    }
    
    std::string hostname = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
    int status = TCP_client__poll__MT<Concurrent_Queue__Blocking, Thread_Pool__Blocking>(hostname, port, AF_INET6);
    return status;
}
