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
int tracker_exchange
(char* addr_tracker,int port_tracker,char* type, char* hash){

	int sockfd_tracker;


	struct sockaddr* socktrack = human2sockaddr(addr_tracker,port_tracker);
	if (socktrack == NULL) fail("human2sockaddr");

	socklen_t addrlen;

	//Création du socket de communication avec le tracker
	if((sockfd_tracker=socket(ip_v,SOCK_DGRAM,0))==-1){
		fail("socket");
	}

	//TODO faire data
	
	uint16_t length = strlen(hash);
	struct tlv* hash = create_tlv(length);
	hash->type = FILE_HASH; 
	tlvset_length(hash,length);

	struct tlv* client = create_tlv(...);//TODO

	struct tlv data[] = { hash, client};
	//Create message for the tracker
	struct msg* tracker_msg = create_msg(type, data, 2, socktrack);


	//Send message to the tracker
	if(sendto(sockfd_tracker,tracker_msg,(msgget_length(tracker_msg)+3),0,(struct sockaddr*)&sockaddr_tracker,addrlen) == -1)
	{
		//TODO erreur
	}
	else
	{
		printf("%s %s to %s port %d",type,hash,addr_tracker,port_tracker);	//XXX
	}

	drop_msg(tracker_msg);

	//XXX boucle de recv si la liste ne tient pas en un message
	//Wait the answer of the tracker

	struct acpt_msg* tracker_answer = accept_msg(sockfd_tracker);

	close(sockfd_tracker);

	return tracker_answer;
}


/**
 * Execution when client put a hash
 * @param sockfd_listen_peer: socket where client listen entering connection with an other client
 * @param tracker_answer: answer of tracker after the client has declare put hash
 */
void put_execution(int sockfd_listen_peer, struct acpt_msg* tracker_answer)
{
	//TODO
}

/**
 * Execution when client get hash
 * @param tracker_answer: anwser of tracker after the client has declare get hash
 */
void get_execution(struct acpt_msg* tracker_answer)
{
	//TODO
}

void fail(char* emsg){
	perror(emsg);
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv){
	if(argc!=6){
		printf("Usage: %s tracker_addr tracker_port "
		       "listen_port action hash",argv[0]);
		exit(EXIT_FAILURE);
	}

	//Initialisation du socket d'écoute des peer
	int sockfd_peer = init_connection(NULL, atoi(argv[3]));
	if (sockfd_peer == -1) fail("init_connection");
	printf("Socket peer créé\n");

	//Communicate with tracker and return its answer
	char type_exchange;
	//Choose different execution depending if client put or get hash
	if(strcmp(argv[4],"put")){
		type_exchange = PUT_T;
	} else if(strcmp(argv[4],"get")){
		type_exchange = GET;
	} else {
		fail("Invalid action");
	}


	if (tracker_exchange(argv[1],atoi(argv[2]),
		type_exchange,argv[5]) == -1) fail("tracker_exchange");

/*
	if(strcmp(argv[4],"put")){
		printf("Listenning on port %s",argv[3]);	//XXX
		tracker_answer = tracker_exchange
			(argv[1],atoi(argv[2]),PUT_T,argv[5]);
		//put_execution(sockfd_peer,tracker_answer);
	} else if(strcmp(argv[4],"get")){
		tracker_answer = tracker_exchange
			(argv[1],atoi(argv[2]),GET_T,argv[5]);
		//get_execution(tracker_answer);
	} else {
		fail("Invalid action");
	}

	if (tracker_answer == NULL) fail("tracker_exchange");
*/
	
	return 0;
}
