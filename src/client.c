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

	//Initialisation du socket d'écoute des peer
	sockfd_peer=init_connection(NULL,argv[3]);
	printf("Socket peer créé\n");

	return 0;
}
