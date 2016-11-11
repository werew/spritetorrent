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

	/* TODO  AF_INET changes depending on the type
 	   of address (ipv4 or ipv6) */

	//TODO verifying if AF_INET and AF_INET6 type is int
	int ip_v;

	//Création socket
	if ((sockfd = socket(ip_v,SOCK_DGRAM,0)) == -1){
		perror("Erreur socket tracker");
		//TODO traitement de l'erreur
	}

	sockaddr.sin_family = ip_v;
	sockaddr.sin_port   = htons(port);
	inet_pton(ip_v,addr,&sockaddr.sin_addr.s_addr)

	//Configuration du socket
	if (bind(sockfd,(struct sockaddr *)&sockaddr,
             sizeof(struct sockaddr_in)) == -1){
        perror("bind");
		//TODO traitement de l'erreur
	}

    return sockfd;
}
