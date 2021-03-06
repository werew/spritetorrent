#ifndef _NETWORK_H_
#define _NETWORK_H_

#define IN(p)  ((struct sockaddr_in* )p)
#define IN6(p) ((struct sockaddr_in6*)p)

int bound_socket(uint16_t port);
struct msg* accept_msg(int sockfd);
int send_msg(int sockfd, struct msg* m);
struct sockaddr* human2sockaddr(const char* addr, uint16_t port);

#endif
