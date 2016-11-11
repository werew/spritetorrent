


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
	socklen_t addrlen;
	struct sockaddr_in sockaddr;

	/* TODO  AF_INET changes depending on the type
 	   of address (ipv4 or ipv6) */

	if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) == -1){
		perror("Erreur socket tracker");
	}

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port   = htons(port);

	if(inet_pton(AF_INET,argv[1],&sockaddr.sin_addr.s_addr) != 1){
		perror("Erreur inet_pton");
		close(sockfd_track);
	}
}
