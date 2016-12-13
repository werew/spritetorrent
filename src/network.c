#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "types.h"
#include "network.h"

struct sockaddr* human2sockaddr(const char* addr, uint16_t port){

	struct sockaddr* sockaddr;
    unsigned long s_addr;
    
    sockaddr = calloc(1,sizeof(struct sockaddr_in));
    if (sockaddr == NULL) return NULL;
    IN(sockaddr)->sin_family = AF_INET;
    if (inet_pton(AF_INET ,addr, &s_addr) != -1 || errno != EAFNOSUPPORT){
			IN(sockaddr)->sin_port = htons(port);
			IN(sockaddr)->sin_addr.s_addr = s_addr;
    } else {
			sockaddr = calloc(1,sizeof(struct sockaddr_in6));
			if (sockaddr == NULL) return NULL;
			IN6(sockaddr)->sin6_family = AF_INET6;
			IN6(sockaddr)->sin6_port = htons(port);
			if (inet_pton(AF_INET6, addr,
				&IN6(sockaddr)->sin6_addr.s6_addr) == -1){
                free(sockaddr);
                return NULL;
            }
    }

    return sockaddr;
}

/**
 * Accept a message and deals with it
 * @param sockfd A socket file descriptor
 *        from where receive the message
 * @return a pointer to a struct msg or 
 *         NULL in case of error
 */
struct msg* accept_msg(int sockfd){

    struct msg* m = calloc(1,sizeof (struct msg)); 
    if (m == NULL) return NULL;

    m->tlv = create_tlv(MAX_LEN_TLV); 
    if (m->tlv == NULL) goto error_1;


    // Receive a message
    m->addrlen = sizeof(struct sockaddr_storage);
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
 * Sends a message
 * @param sockfd The socket trough which the message must
 *        be sent
 * @param m A pointer to a struct msg containing the message
 * @return 0 in case of success, -1 otherwise
 */
int send_msg(int sockfd, struct msg* m){
    size_t sizemsg = SIZE_HEADER_TLV + tlvget_length(m->tlv);
    ssize_t s = sendto(sockfd, m->tlv, sizemsg, 0, 
            (struct sockaddr*) &m->addr, m->addrlen);

    puts("sending");
    if (s < sizemsg) return -1;

    puts("sent");

    return 0;

}








/**
 * Creates a socket and binds it to the given address
 * and port.
 * @param addr A string containing an IPv4 or IPv6
 *        address
 * @param port The port to use. If 0 a random port
 *        will be used
 * @return The fd of the socket bound to the given
 *        port and address or -1 in case of error
 */
int init_connection(char* addr, uint16_t port){	

	int sockfd;
	struct sockaddr_in sockaddr;

    // TODO
	int ip_v = AF_INET;

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



/**
 * Creates a new socket bound at the given port
 * and sets the address to in6addr_any, which (by default) 
 * allows connections to be established from any IPv4 or IPv6 client
 * @param port The port to which this socket must be bound
 * @return The file descriptor of the socket, or -1 in case of error
 */
int bound_socket(uint16_t port){	
	int sockfd;
	struct sockaddr_in6 sockaddr;

	if ((sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) == -1)
        return -1;

	sockaddr.sin6_family = AF_INET6;
	sockaddr.sin6_port   = htons(port);
    sockaddr.sin6_addr   = in6addr_any;

	if (bind(sockfd,(struct sockaddr *)&sockaddr,
             sizeof(struct sockaddr_in6)) == -1) {
        int e_bkp = errno;
        close(sockfd);
        errno = e_bkp;
        return -1;
    }

    return sockfd;
}




