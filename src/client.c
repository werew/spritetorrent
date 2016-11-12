// Here goes the code of the client

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
#include "types.h"
#include "network.h"

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

	int sockfd_tracker;

	//Determine the version of IP
	int ip_v=ip_version(addr_tracker);

	//Création du socket de communication avec le tracker
	if((sockfd_tracker=socket(ip_v,SOCK_DGRAM,0))==-1)
	{
		//TODO traitement erreur
	}

	struct sockaddr_in sockaddr_tracker;

	//Setting the sockaddr tracker
	sockaddr_tracker.sin_family = ip_v;
	sockaddr_tracker.sin_port = port_tracker;
	inet_pton(ip_v,addr_tracker,&sockaddr_tracker.sin_addr.s_addr);
	socklen_t addrlen=sizeof(struct sockaddr_in);

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
	if(sendto(sockfd_tracker,tracker_msg,(msgget_length(tracker_msg)+3),0,(struct sockaddr*)&sockaddr_tracker,addrlen) == -1)
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






int main(int argc, char* argv)
{
	if(argc!=6){
		printf("Usage: %s tracker_addr tracker_port listen_port action hash",argv[0]);
		exit(EXIT_FAILURE);
	}

	int sockfd_peer;

	//Initialisation du socket d'écoute des peer
	sockfd_peer=init_connection(NULL,argv[3]);
	printf("Socket peer créé\n");

	

	return 0;
}
