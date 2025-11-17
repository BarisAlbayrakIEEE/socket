/*
#include "print_addrs.h"
using namespace ba_socket;
int main(int argc, char *argv[]) {
    return print_addrs();
}
*/

/*
#include "server_with_timer__jthread.h"
using namespace ba_socket;
int main(int argc, char *argv[]) {
    return server_with_timer__jthread();
}
*/

//*
#include "TCP_client.h"
using namespace ba_socket;

int main(int argc, char *argv[]) {
    /*
    if (argc < 3) {
        fprintf(stderr, "usage: tcp_client hostname port\n");
        return 1;
    }
    */
    
    int status = TCP_client("example.com", 80);
    return status;
}
//*/
