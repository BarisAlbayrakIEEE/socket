// TCP__Server__Low__HP.cpp

#include "TCP__Server.hpp"
#include "Concurrent_Queue__Blocking.hpp"
#include "Thread_Pool__Blocking.hpp"

using namespace BA_Socket;
using namespace BA_Concurrency;

int main(int argc, char *argv[]) {
    using EL_t = Event_Loop__Low__HP_t<Concurrent_Queue__Blocking, Thread_Pool__Blocking>;
    int status = TCP__Server<EL_t>();
    return status;
}
