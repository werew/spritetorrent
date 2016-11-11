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
 * Handle a received message
 * @param msg A pointer to the received message
 * @param src_addr A struct giving some infos 
 *        about the sender
 * @param addrlen The actual size of the struct 
 *        pointed by src_addr
 * @return 0 in case of success, -1 otherwise
 */
int handle_msg
(struct msg* msg, struct sockaddr* src_addr, socklen_t addrlen){

    // Create seeder
    struct seeder* peer = create_seeder(src_addr,addrlen);
    if (peer == NULL) return -1;
    

    //TODO complete
    switch (msg->type){
        case PUT_T: puts("PUT");
            break;
        case GET: puts("GET");
            break;
        case KEEP_ALIVE: puts("KEEP_ALIVE");
            break;
        default: puts("INVALID");
    }
    
    return 0;
}


/**
 * Accept a message and deals with it
 * @param sockfd A socket file descriptor
 *        from where receive the message
 * @return 0 in case of success, -1 otherwise
 */
int accept_msg(int sockfd){

    struct sockaddr_storage src_addr;
    socklen_t addrlen = sizeof src_addr;

    // Receive a message
    ssize_t size_msg;
    struct msg* msg = create_msg(MAX_MSG); 
    if (msg == NULL) return -1;
    size_msg = recvfrom(sockfd, msg, MAX_MSG, 0, 
                   (struct sockaddr*) &src_addr, 
                   &addrlen);
    if (size_msg == -1) return -1;

    // Check valid length
    uint16_t mlenght = msgget_length(msg);
    if (mlenght > size_msg) {
        drop_msg(msg);
        printf("Message dropped: bad length\n"); 
        return 0;
    }

    // Deal with the msg
    if (handle_msg(msg, (struct sockaddr*) &src_addr, addrlen) == -1){
        drop_msg(msg);
        return -1;
    }

    return 0;
}

int main(int argc, char* argv[]){
    // Listen on localhost port 5555
    // XXX those values are just a test
    int sockfd = init_connection("127.0.0.1",5555);

    accept_msg(sockfd);

    return 0;
}

