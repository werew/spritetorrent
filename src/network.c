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

struct sockaddr* human2sockaddr(char* addr, uint16_t port){
	//Determine the version of IP
	int ip_v = ip_version(addr_tracker);

	struct sockaddr* sockaddr;

	//Setting the sockaddr tracker
	switch (ip_v) {
		case AF_INET: 
			sockaddr = malloc(sizeof(struct sockaddr_in));
			if (sockaddr == NULL) return NULL;
			IN(&sockaddr)->sin_family = ip_v;
			IN(&sockaddr)->sin_port = htons(port);
			if (inet_pton(ip_v,addr,
				&IN(&sockaddr)->sin_addr.s_addr) == -1)
				return NULL;
			break;
		case AF_INET6:
			sockaddr = malloc(sizeof(struct sockaddr_in6));
			if (sockaddr == NULL) return NULL;
			IN6(&sockaddr)->sin6_family = ip_v;
			IN6(&sockaddr)->sin6_port = htons(port);
			if (inet_pton(ip_v,addr,
				&IN6(&sockaddr)->sin6_addr.s6_addr) == -1)
				return NULL;
			break;
		default: return NULL;
	}
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
 * Check the content of a message against errors and
 * inconstistencies. For instance: check if the global 
 * length is compatible with the lengths of the contents
 * and if the message contains the expected types
 * @param msg A pointer to the struct tlv containing the
 *        message to validate
 * @param nargs The number of struct tlv (args) contained 
 *        inside the message (without considering the 
 *        message itself
 * @param types The expected types of the args //TODO 
 * @return 0 in case of success, -1 otherwise
 */
int validate_tlv(struct tlv* msg, unsigned int nargs){
    puts("Validate tlv");//
    unsigned int expected_size, nheaders, summ_lengths;
    expected_size = nheaders = summ_lengths = 0;

    uint16_t total_length = tlvget_length(msg);

    while (nheaders < nargs){
        nheaders++; 
        expected_size = nheaders*SIZE_HEADER_TLV + summ_lengths;    
        if (total_length < expected_size) return -1; 

        // TODO check also the types

        summ_lengths += tlvget_length
            ((struct tlv*)&msg->data[expected_size-SIZE_HEADER_TLV]);
    }

    expected_size = nheaders*SIZE_HEADER_TLV + summ_lengths;    
    if (total_length < expected_size) return -1;

    return 0;
}


/**
 * Converts a struct tlv of type CLIENT to a
 * struct sockaddr
 * @param c A pointer to the struct tlv to convert
 * @return A pointer to a dynamic allocated struct
 *         sockaddr
 */
struct sockaddr* client2sockaddr(const struct tlv* c){
    if (c->type != CLIENT) return NULL;
    uint16_t len = tlvget_length(c);

    struct sockaddr* s;

    switch (len) {
        case 6:
                s = malloc(sizeof(struct sockaddr_in));
                memset(s,0,sizeof(struct sockaddr_in));
                IN(s)->sin_family = AF_INET;
                memcpy(&IN(s)->sin_port,c->data,2);
                memcpy(&IN(s)->sin_addr.s_addr,&c->data[2],4);
            break;
        case 18:
                s = malloc(sizeof(struct sockaddr_in6));
                memset(s,0,sizeof(struct sockaddr_in6));
                IN6(s)->sin6_family = AF_INET6;
                memcpy(&IN6(s)->sin6_port,c->data,2);
                memcpy(&IN6(s)->sin6_addr.s6_addr,&c->data[2],16);
            break;
        default: // Invalid addr type
                return NULL;
    }

    return s;
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




