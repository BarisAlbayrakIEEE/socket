// TCP__Server__Mid__ST.cpp

#include "TCP__Server.hpp"

using namespace BA_Socket;

int main(int argc, char *argv[]) {
    using EL_t = Event_Loop__Mid__ST_t;
    int status = TCP__Server<EL_t>();
    return status;
}
