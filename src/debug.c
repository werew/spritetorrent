#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "debug.h"
#include "sha256.h"

void printhash(char* hash){
    int i;
    for (i=0;i<SHA256_HASH_SIZE;i++){
        printf("%x",hash[i]);
    }
    printf("\n");
}

void printsockaddr(struct sockaddr* s){
    char* addr = calloc(50);
    uint16_t port;

    switch (s->sa_family){
        case AF_INET: 
            inet_ntop(AF_INET,&IN(s)->sin_addr.s_addr, addr, 50);
            port = ntohs(IN(s)->sin_port);
            break;
        case AF_INET6:
            inet_ntop(AF_INET6,&IN6(s)->sin6_addr.s6_addr, addr, 50);
            port = ntohs(IN(s)->sin6_port);
            break;
        default: puts("Invalid sockaddr type");
    }

    printf("--> Addr: %s Port: %d\n",addr,port);
    free(addr);
}
