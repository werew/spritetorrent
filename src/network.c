


void init_connection(char* addr, uint16_t port){

	int sockfd;
	struct sockaddr_in sockaddr;

	/* TODO  AF_INET changes depending on the type
 	   of address (ipv4 or ipv6) */

	//Création socket
	if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) == -1){
		perror("Erreur socket tracker");
		//TODO traitement de l'erreur
	}

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port   = htons(port);
	sockaddr.sin_addr.s_addr = INADDR_ANY;

	//Configuration du socket
	if(bind(sockfd,(struct sockaddr *)&sockaddr,sizeof(struct sockaddr_in)) == -1){
		//TODO traitement de l'erreur
	}
}
