// TCP__Server__Low__HP.cpp

#include "TCP__Server__Low__HP.hpp"
#include "Concurrent_Queue__Blocking.hpp"
#include "Thread_Pool__Blocking.hpp"

using namespace BA_Socket;
using namespace BA_Concurrency;

int main() {
    int status = TCP__Server__Low__HP<Concurrent_Queue__Blocking, Thread_Pool__Blocking>();
    return status;
}
