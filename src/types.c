#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "types.h"
#include "network.h"


int sockaddr_cmp(struct sockaddr* addr1, struct sockaddr* addr2){

    if (addr1->sa_family == AF_INET){

        if (IN(addr1)->sin_port != IN(addr2)->sin_port ||
            IN(addr1)->sin_addr.s_addr != IN(addr2)->sin_addr.s_addr 
           ) return 0;

    } else if (addr1->sa_family == AF_INET6){

        if (IN6(addr1)->sin6_port != IN6(addr2)->sin6_port) return 0;
        if (memcmp(&IN6(addr1)->sin6_addr, &IN6(addr2)->sin6_addr,
            sizeof(struct in6_addr)) != 0) return 0;

    } else return -1;

    return 1;
}    

int tlv_cmp(struct tlv* t1, struct tlv* t2){

    if (t1->type != t2->type) return 0;

    uint16_t len1 = tlvget_length(t1);
    uint16_t len2 = tlvget_length(t2);

    if (len1 != len2) return 0;

    if (memcmp(t1->data, t2->data, len1) != 0) return 0;


    return 1;
}

    
/**
 * Allocates a struct msg of the good size
 * @param size_data The size of the data portion of the tlv
 * @return A pointer to the newly allocated struct tlv
 */
struct tlv* create_tlv(uint16_t size_data){
    struct tlv* tlv = malloc(SIZE_HEADER_TLV + size_data);
    if (tlv == NULL) return NULL;

    tlvset_length(tlv, size_data);

    return tlv;
}

/**
 * Drops a struct msg
 * @param msg Pointer to the struct msg to drop
 */
void drop_tlv(struct tlv* tlv){
    free(tlv);
}


/**
 * Creates an new struct msg with the given addr
 * and the tlv allocated to the given length
 * @param size_data  Size of the data into the msg
 * @param s A struct sockaddr containing the address
 *        relative to this message
 */
struct msg* create_msg(uint16_t size_data, const struct sockaddr* s){
    struct msg* m = calloc(1,sizeof(struct msg));
    if (m == NULL) return NULL;

    if ((m->tlv = create_tlv(size_data)) == NULL) {
        free(m);
        return NULL;
    }

    m->size = SIZE_HEADER_TLV + size_data;
    m->addrlen = (s->sa_family == AF_INET)? 
            sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
    memcpy(&m->addr, s, m->addrlen);
    
    return m;
}

void drop_msg(struct msg* m){
    drop_tlv(m->tlv);
    free(m);
}



/**
 * Read the length of a message
 * @param msg A pointer to the message
 * @return The length of the message
 */
uint16_t tlvget_length(const struct tlv* tlv){
    uint16_t length;

    unsigned char* pl = (unsigned char*) &length;
    pl[0] = tlv->_len0;
    pl[1] = tlv->_len1;

    return ntohs(length);
}

/**
 * Set the length of a message
 * @param msg A pointer to the message
 * @param length The length to set
 */
void tlvset_length(struct tlv* tlv, uint16_t length){
    length = htons(length);
    unsigned char* pl = (unsigned char*) &length;
    tlv->_len0 = pl[0];
    tlv->_len1 = pl[1];
}



struct msg* msg_dup(const struct msg* m){

    struct msg* dup = create_msg(m->size-SIZE_HEADER_TLV, 
            (struct sockaddr*) &m->addr);
    if (dup == NULL) return NULL;

    memcpy(dup->tlv,m->tlv, m->size);

    return dup;
}



/**
 * Check the content of a message against errors and
 * inconstistencies. For instance: check if the global 
 * length is compatible with the lengths of the contents
 * and if the message contains the expected types
 * @param msg A pointer to the struct tlv containing the
 *        message to validate
 * @param nargs The number of struct tlv (args) contained 
 *        inside the message (without considering the 
 *        message itself
 * @param types The expected types of the args 
 * @return 0 in case of success, -1 otherwise
 */
int validate_tlv(struct tlv* msg, unsigned int nargs){
    unsigned int expected_size, nheaders, summ_lengths;
    expected_size = nheaders = summ_lengths = 0;

    uint16_t total_length = tlvget_length(msg);

    while (nheaders < nargs){
        nheaders++; 
        expected_size = nheaders*SIZE_HEADER_TLV + summ_lengths;    
        if (total_length < expected_size) return -1; 

        summ_lengths += tlvget_length
            ((struct tlv*)&msg->data[expected_size-SIZE_HEADER_TLV]);
    }

    expected_size = nheaders*SIZE_HEADER_TLV + summ_lengths;    
    if (total_length < expected_size) return -1;

    return 0;
}



/**
 * Converts a struct tlv of type CLIENT to a
 * struct sockaddr
 * @param c A pointer to the struct tlv to convert
 * @return A pointer to a dynamic allocated struct
 *         sockaddr
 */
struct sockaddr* client2sockaddr(const struct tlv* c){
    if (c->type != CLIENT) return NULL;
    uint16_t len = tlvget_length(c);

    struct sockaddr* s;

    switch (len) {
        case 6:
                s = calloc(1,sizeof(struct sockaddr_in));
                if (s == NULL) return NULL;
                IN(s)->sin_family = AF_INET;
                memcpy(&IN(s)->sin_port,c->data,2);
                memcpy(&IN(s)->sin_addr.s_addr,&c->data[2],4);
            break;
        case 18:
                s = calloc(1,sizeof(struct sockaddr_in6));
                if (s == NULL) return NULL;
                IN6(s)->sin6_family = AF_INET6;
                memcpy(&IN6(s)->sin6_port,c->data,2);
                memcpy(&IN6(s)->sin6_addr.s6_addr,&c->data[2],16);
            break;
        default: // Invalid addr type
                return NULL;
    }

    return s;
}


/**
 * Converts a struct tlv of type CLIENT to a
 * struct sockaddr
 * @param c A pointer to the struct tlv to convert
 * @return A pointer to a dynamic allocated struct
 *         sockaddr
 */
struct tlv* sockaddr2client(const struct sockaddr* s){

    struct tlv* c;

    switch (s->sa_family){
        case AF_INET: 
                if((c = create_tlv(2+4)) == NULL) return NULL;
                memcpy(c->data, &IN(s)->sin_port,2);
                memcpy(&c->data[2], &IN(s)->sin_addr.s_addr,4);
            break;
        case AF_INET6: 
                if((c = create_tlv(2+16)) == NULL) return NULL;
                memcpy(c->data, &IN6(s)->sin6_port,2);
                memcpy(&c->data[2], &IN6(s)->sin6_addr.s6_addr,16);
            break;
        default: // Invalid addr type
                return NULL;
    }

    c->type = CLIENT;
    return c;
}


/* FNV-1a hash */
unsigned int htable_index(const char* data, size_t length){
    unsigned int hash = 2166136261 % SIZE_HTABLE;
    size_t i;
    for (i = 0; i < length; i++) {
        hash ^= data[i];
        hash *= 16777619;
        hash %= SIZE_HTABLE;
    }
    return hash;
}

