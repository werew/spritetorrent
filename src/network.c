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
 * Determine the using version of IP adress
 * passing in argument
 * @param addr: Address to determine the IP version
 * @return AF_INET or AF_INET6
 */

int ip_version(char* addr)
{
	//TODO faire fonction
    return AF_INET; // XXX just not to break the program
}

/**
 * Accept a message and deals with it
 * @param sockfd A socket file descriptor
 *        from where receive the message
 * @return a pointer to a struct acpt_msg or 
 *         NULL in case of error
 */
struct acpt_msg* accept_msg(int sockfd){

    struct acpt_msg* am = malloc(sizeof (struct acpt_msg)); 
    if (am == NULL) return NULL;

    am->msg = create_msg(MAX_MSG); 
    if (am->msg == NULL) goto error_1;


    // Receive a message
    am->addrlen = sizeof am->addrlen;
    am->size = recvfrom(sockfd, am->msg, MAX_MSG, 0, 
                   (struct sockaddr*) &(am->src_addr),
                   &(am->addrlen));
    if (am->size == -1) goto error_2;

    return am;

error_2:
    drop_msg(am->msg);
error_1:
    free(am);
    return NULL;
}

/**
 * Allocates a struct msg of the good size
 * @param max_length The maximum size of the
 *        data that can be contained
 * @return A pointer to the newly allocated
 *        struct msg
 */
struct msg* create_msg(uint16_t max_length){
    struct msg* msg = malloc(max_length+4);
    return msg;
}

/**
 * Drops a struct msg
 * @param msg Pointer to the struct msg to drop
 */
void drop_msg(struct msg* msg){
    free(msg);
}

void drop_acpt_msg(struct acpt_msg* am){
    drop_msg(am->msg);
    free(am);
}

/**
 * Read the length of a message
 * @param msg A pointer to the message
 * @return The length of the message
 */
uint16_t msgget_length(struct msg* msg){
    uint16_t length;

    unsigned char* pl = (unsigned char*) &length;
    pl[0] = msg->_len0;
    pl[1] = msg->_len1;

    return ntohs(length);
}

/**
 * Set the length of a message
 * @param msg A pointer to the message
 * @param length The length to set
 */
void msgset_length(struct msg* msg, uint16_t length){
    length = htons(length);
    unsigned char* pl = (unsigned char*) &length;
    msg->_len0 = pl[0];
    msg->_len1 = pl[1];
}

/**
 * Creates a struct seeder from a generic
 * struct sockaddr and its length
 * @param addr Pointer to the struct sockaddr
 * @param length Actual size of the address
 */
struct seeder* create_seeder(struct sockaddr* addr, socklen_t length){

    // Allocate a struct seeder
    struct seeder* s = malloc(sizeof(struct seeder));
    if (s == NULL) return NULL;
   
    // Copy the address 
    s->addr = malloc(length);
    if (s->addr == NULL) goto error_1;
    memcpy(s->addr,addr, length);

    // Add a timestamp
    s->lastseen = time();
    if (s->lastseen == -1) goto error_2;
    
    return s;

error_2:
    free(s->addr);
error_1:
    free(s);
    return NULL;
}

/**
 * Drop a given seeder freeing
 * the allocated memory
 * @param s A pointer to the seeder to drop
 */
void drop_seeder(struct seeder* s){
    free(s->addr);
    free(s);
}

/**
 * Creates a socket and binds it to the given address
 * and port.
 * @param addr A string containing an IPv4 or IPv6
 *        address
 * @param port The port to use. If 0 a random port
 *        will be used
 * @return The fd of the socket binded to the given
 *        port and address or -1 in case of error
 */
int init_connection(char* addr, uint16_t port){	

	int sockfd;
	struct sockaddr_in sockaddr;

	int ip_v=ip_version(addr);

	//Création socket
	if ((sockfd = socket(ip_v,SOCK_DGRAM,0)) == -1){
		perror("Erreur socket tracker");
		//TODO traitement de l'erreur
	}

	sockaddr.sin_family = ip_v;
	sockaddr.sin_port   = htons(port);

	if(addr==NULL){
		sockaddr.sin_addr.s_addr=INADDR_ANY;
	}else{
		inet_pton(ip_v,addr,&sockaddr.sin_addr.s_addr);
	}
	

	//Configuration du socket
	if (bind(sockfd,(struct sockaddr *)&sockaddr,
             sizeof(struct sockaddr_in)) == -1){
        perror("bind");
		//TODO traitement de l'erreur
	}

    return sockfd;
}




