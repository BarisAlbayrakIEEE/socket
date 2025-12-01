// TCP_server__poll__MT.cpp

#include "TCP_server__poll__MT.hpp"
#include "Concurrent_Queue__Blocking.hpp"
#include "Thread_Pool__Blocking.hpp"

using namespace BA_Socket;
using namespace BA_Concurrency;

int main() {
    int status = TCP_server__poll__MT<Concurrent_Queue__Blocking, Thread_Pool__Blocking>();
    return status;
}
