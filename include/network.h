#ifndef _NETWORK_H_
#define _NETWORK_H_

#define IN(p)  ((struct sockaddr_in* )p)
#define IN6(p) ((struct sockaddr_in6*)p)

int init_connection(char* addr, uint16_t port);
int bound_socket(uint16_t port);
struct msg* accept_msg(int sockfd);

#endif
