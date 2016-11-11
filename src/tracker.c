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


/**
 * Accept a message and deals with it
 * @param sockfd A socket file descriptor
 *        from where receive the message
 * @return 0 in case of success, -1 otherwise
 */
int accept_msg(int sockfd){

    struct msg* msg = malloc(MAX_MSG);
    struct sockaddr_in src_addr;
    socklen_t addrlen;
   
    // Receive a msg  
    // TODO test for errors
    recvfrom(sockfd, msg, MAX_MSG, 0, 
             (struct sockaddr*) &src_addr, &addrlen);

    // TODO deal with the content of the msg

}

int main(int argc, char* argv[]){
    // Listen on localhost port 5555
    // XXX those values are just a test
    int sockfd = init_connection("127.0.0.1",5555);

    accept_msg(sockfd);

    
    return 0;
}
