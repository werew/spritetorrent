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
 * @return a new ctask of NULL in case of error
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


int st_cwork(st_ctask ctask){
    time_t now, ltime;
    struct msg* m;
    struct pollfd pfd;

    // Init pollfd
    pfd.fd = ctask->sockfd;
    pfd.events = POLLIN;

    while (1){

        // Timeout: finish if too much time has passed
        // since the last probe 
        if ((now = time(NULL)) == -1) return -1;
        if (now - ctask->lastupdate > ctask->timeout) break;

        // Wait for incoming messages during the remaining time
        ltime = ctask->timeout - (now-ctask->lastupdate);

        int ret =  poll(&pfd, 1, ltime * 1000);
        if (ret == -1) return -1;  // error
        else if (ret == 0 ) break; // timeout

        // A message has arrived
        printf("Accept\n");
        if ((m = accept_msg(ctask->sockfd)) == NULL) return -1;
                
        if (handle_msg(ctask, m) == -1) return -1;
    }

    return 0;
}


int handle_msg(st_ctask ctask, struct msg* m){
    puts("Handle msg");
    return 0;
}

int poll_runupdate(st_ctask ctask){
    puts("Update POLL");
    ctask->lastupdate = time(NULL); 
    if (ctask->lastupdate == -1) return -1;
    return 0;
}

int ka_gen(st_ctask ctask){
    puts("KA generation");
    return 0;
}


int st_cstart(st_ctask ctask){

    // Update lastupdate timestamp
    ctask->lastupdate = time(NULL); 
    if (ctask->lastupdate == -1) return -1;

    // Main work/update loop 
    while(1){
        
        // listen/handle/reply
        printf("Wait\n");
        if (st_cwork(ctask) == -1) return -1;
        
        // Forge KAs
        ka_gen(ctask);

        // Update poll
        poll_runupdate(ctask);

    }

    return 0; 
}


int main(int argc, char* argv[]){

    st_ctask ctask = st_create_ctask(5555, 10);
    if (ctask == NULL) fail("st_create_ctask");

    return 0;
}
  
