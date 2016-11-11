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


int main(int argc, char* argv)
{
	if(argc!=6){
		printf("Usage: %s tracker_addr tracker_port listen_port action hash",argv[0]);
		exit(EXIT_FAILURE);
	}

	int sockfd_tracker,sockfd_peer;
	int ip_v=ip_version(argv[1]);

	//Initialisation du socket d'écoute des peer
	sockfd_peer=init_connection(NULL,argv[3]);
	printf("Socket peer créé\n");

	//Création du socket de communication avec le tracker
	if((sockfd_tracker=socket(ip_v,SOCK_DGRAM,0))==-1)
	{
		//TODO traitement erreur
	}

	struct sockaddr_in sockaddr_tracker;

	tracker.sin_family = ip_v;
	tracker.sin_port = argv[2];
	inet_pton(AF_INET6,argv[1],&tracker.sin_addr.s_addr);

	struct msg* tracker_sollicitation=create_msg(/*TODO taille*/);
	//TODO mise du hash et du client
	//TODO mise de la longueur

	if(strcmp(argv[4],"put")==0)
	{
		msg->type=PUT_T;
	}
	else if(strcmp(argv[4],"get")==0)
	{
		msg->type=GET;
	}
	else
	{
		//TODO erreur, action incorrect
	}

	

	return 0;
}
