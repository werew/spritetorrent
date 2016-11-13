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
 * @return a pointer to a struct msg or 
 *         NULL in case of error
 */
struct msg* accept_msg(int sockfd){

    struct msg* m = malloc(sizeof (struct msg)); 
    if (m == NULL) return NULL;

    m->tlv = create_tlv(MAX_LEN_TLV); 
    if (m->tlv == NULL) goto error_1;


    // Receive a message
    m->addrlen = sizeof m->addrlen;
    m->size = recvfrom(sockfd, m->tlv, MAX_LEN_TLV, 0, 
                   (struct sockaddr*) &(m->addr),
                   &(m->addrlen));
    if (m->size == -1) goto error_2;

    return m;

error_2:
    drop_tlv(m->tlv);
error_1:
    free(m);
    return NULL;
}

/**
 * Allocates a struct msg of the good size
 * @param max_length The maximum size of the
 *        data that can be contained
 * @return A pointer to the newly allocated
 *        struct msg
 */
struct tlv* create_tlv(uint16_t max_length){
    struct tlv* tlv = malloc(max_length+4);
    return tlv;
}

/**
 * Drops a struct msg
 * @param msg Pointer to the struct msg to drop
 */
void drop_tlv(struct tlv* tlv){
    free(tlv);
}

void drop_msg(struct msg* m){
    drop_tlv(m->tlv);
    free(m);
}

/**
 * Read the length of a message
 * @param msg A pointer to the message
 * @return The length of the message
 */
uint16_t tlvget_length(const struct tlv* tlv){
    uint16_t length;

    unsigned char* pl = (unsigned char*) &length;
    pl[0] = tlv->_len0;
    pl[1] = tlv->_len1;

    return ntohs(length);
}

/**
 * Set the length of a message
 * @param msg A pointer to the message
 * @param length The length to set
 */
void tlvset_length(struct tlv* tlv, uint16_t length){
    length = htons(length);
    unsigned char* pl = (unsigned char*) &length;
    tlv->_len0 = pl[0];
    tlv->_len1 = pl[1];
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




