#include "common.h"
#include "server.h"

int main(int argc, char *argv[]) {
    int port = 6379;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    server_start(port);
    
    return 0;
}