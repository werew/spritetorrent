// Here goes the code of the tracker
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "types.h"
#include "network.h"
#include "client.h"
#include "debug.h"


void fail(const char* emsg){
    perror(emsg);
    exit(1);
}


/**
 * Creates a new ctask suitable to use a client
 * @param port Port to use
 * @param timeout Frequency of the keep alive
 * @return a new ttask of NULL in case of error
 */
st_ctask st_create_ctask(uint16_t port, time_t timeout){
    st_ctask ctask = calloc(1,sizeof(struct ctask));
    if (ctask == NULL) return NULL;

    ctask->sockfd = bound_socket(port);
    if (ctask->sockfd == -1) {
        free(ctask);
        return NULL;
    }

    ctask->timeout = timeout;

    return ctask;
}


int main(int argc, char* argv[]){

    st_ctask ctask = st_create_ctask(5555, 10);
    if (ctask == NULL) fail("st_create_ttask");

    return 0;
}
  
