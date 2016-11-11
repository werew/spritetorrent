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
 * Determine the using version of IP adress
 * passing in argument
 * @param addr: Address to determine the IP version
 * @return AF_INET or AF_INET6
 */

int ip_version(char* addr)
{
	//TODO faire fonction
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



/**
 * Create and use a connection for send a PUT, GET or KEEP ALIVE
 * message to the tracker and receive its answer
 * 
 * @param addr_tracker: address to contact the tracker
 * @param port_tracker: port to contact the tracker
 * @param type: type of message send to the tracker
 * @param hash: hash send the tracker
 */
struct msg* tracker_exchange(char* addr_tracker,int port_tracker,char* type, char* hash)
{
	//Determine the version of IP
	int ip_v=ip_version(addr_tracker);

	//Création du socket de communication avec le tracker
	if((sockfd_tracker=socket(ip_v,SOCK_DGRAM,0))==-1)
	{
		//TODO traitement erreur
	}

	struct sockaddr_in sockaddr_tracker;

	//Setting the sockaddr tracker
	tracker.sin_family = ip_v;
	tracker.sin_port = port_tracker;
	inet_pton(ip_v,addr_tracker,&tracker.sin_addr.s_addr);

	//Create message for the tracker
	struct msg* tracker_msg=create_msg(1027);
	//TODO mise du hash et du client
	//TODO mise de la longueur

	if(strcmp(type,"put")==0)
	{
		tracker_msg->type=PUT_T;
	}
	else if(strcmp(type,"get")==0)
	{
		tracker_msg->type=GET;
	}
	//TODO message keep alive
	else
	{
		//TODO erreur, action incorrect
	}

	//Send message to the tracker
	if(sendto(sockfd_tracker,tracker_msg,(msgget_length(msg)+3),(struct sockaddr*)&tracker,sizeof(sockaddr_in)) == -1)
	{
		//TODO erreur
	}

	//XXX boucle de recv si la liste ne tient pas en un message
	//Wait the answer of the tracker
	if(rcvfrom(sockfd_tracker,tracker_msg,1027,0,NULL,NULL) == -1)
	{
		//TODO erreur
	}

	close(sockfd_tracker);

	return tracker_msg;
}
