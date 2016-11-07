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

int main(int argc, char **argv)
{
	int sockfd_track,sockfd_peer;
	socklen_t addrlen;
	struct sockaddr_in tracker, peer;

	// check the number of args on command line
	if(argc != 6)
	{
		printf("USAGE: %s @tracker tracker_port listen_port mode hash\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// socket factory
	if((sockfd_track = socket(AF_INET,SOCK_DGRAM,0)) == -1)
	{
		perror("Erreur socket tracker");
		exit(EXIT_FAILURE);
	}

	// init remote addr structure and other params
	tracker.sin_family = AF_INET;
	tracker.sin_port   = htons(atoi(argv[2]));
	addrlen         = sizeof(struct sockaddr_in);

	// get addr from command line and convert it
	if(inet_pton(AF_INET,argv[1],&tracker.sin_addr.s_addr) != 1)
	{
		perror("Erreur inet_pton");
		close(sockfd_track);
		exit(EXIT_FAILURE);
	}

/******************************************************************/

	if((sockfd_peer = socket(AF_INET,SOCK_DGRAM,0)) == -1)
	{
		perror("Erreur socket peer");
		close(sockfd_track);
		exit(EXIT_FAILURE);
	}

	// init remote addr structure and other params
	peer.sin_family = AF_INET;
	peer.sin_port   = htons(atoi(argv[3]));
	peer.sin_addr.s_addr=INADDR_ANY;

	if(bind(sockfd_peer,(struct sockaddr *)&peer,addrlen)==-1)
	{
		perror("Erreur bind");
		close(sockfd_track);
		close(sockfd_peer);
		exit(EXIT_FAILURE);
	}

/***********************************************************************/

	struct msg query, answer;

	if(strcmp(argv[4],"put")==0)
	{
		query.type=110;
	}
	else if(strcmp(argv[4],"get")==0)
	{
		query.type=112;
	}
	else
	{
		printf("Erreur mode");
		close(sockfd_track);
		exit(EXIT_FAILURE);
	}

	query.length=strlen(argv[5]);

	query.data=malloc(query.length);

	memcpy(query.data,argv[5],query.length);

	// send query to tracker
	if(sendto(sockfd_track,(void*)&query,sizeof(struct msg),0,(const struct sockaddr *) &tracker,addrlen) == -1)
	{
		perror("Erreur send");
		close(sockfd_track);
		exit(EXIT_FAILURE);
	}

	answer.data=malloc(1024);

	if(recvfrom(sockfd_track,&answer,1027,0,NULL,NULL) == -1)	//TODO mettre info tracker dans sockaddr
   	{
		perror("Erreur recvfrom");
		close(sockfd);
		exit(EXIT_FAILURE);
	}


	// close the socket
	close(sockfd_track);
	close(sockfd_peer);

	return 0;
}
