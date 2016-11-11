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
#include <unistd.h>
#include <string.h>
#include "../include/types.h"

// Max size of a msg
#define MAX_MSG 1028

int main(int argc, char* argv[]){
    // Listen on localhost port 5555
    // XXX those values are just a test
    int socketfd = init_connection("localhost",5555);

    char buf[MAX_MSG];
    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof addrlen;
   
    // Receive a msg  
    recvfrom(socketfd, buf, MAX_MSG, 0, 
             (struct sockaddr*) &src_addr, &addrlen);
    
    return 0;
}
