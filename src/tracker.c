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
#include "../include/network.h"


/**
 * Handle a received message
 * @param msg A pointer to the received message
 * @param src_addr A struct giving some infos 
 *        about the sender
 * @param addrlen The actual size of the struct 
 *        pointed by src_addr
 * @return 0 in case of success, -1 otherwise
 */
int handle_msg(struct acpt_msg* am){

    // Check valid length
    uint16_t mlenght = msgget_length(am->msg);
    if (mlenght > am->size) {
        printf("Message dropped: bad length\n"); 
        return 0;
    }

    // Create seeder
    struct seeder* peer = create_seeder
            ((struct sockaddr*) &am->src_addr,am->addrlen);
    if (peer == NULL) return -1;
    

    //TODO complete
    switch (am->msg->type){
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



int main(int argc, char* argv[]){
    // Listen on localhost port 5555
    // XXX those values are just a test
    int sockfd = init_connection("127.0.0.1",5555);

    struct acpt_msg* am = accept_msg(sockfd);

    // Deal with the msg
    if (handle_msg(am) == -1) perror("");

    drop_acpt_msg(am);
    return 0;
}
  
