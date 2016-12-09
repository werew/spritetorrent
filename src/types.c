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



/**
 * Allocates a struct msg of the good size
 * @param max_length The maximum size of the
 *        data that can be contained
 * @return A pointer to the newly allocated
 *        struct msg
 */
struct tlv* create_tlv(uint16_t max_length){
    struct tlv* tlv = malloc(max_length+4);
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
 * @param size Size of the data into the msg
 * @param s A struct sockaddr containing the address
 *        relative to this message
 */
struct msg* create_msg(size_t size, struct sockaddr s){
    struct msg* m = calloc(sizeof(struct msg));
    if (m == NULL) return NULL;

    if ((m->tlv = calloc(1,size)) == NULL) {
        free(m);
        return NULL;
    }

    m->size = size;
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
 * @param types The expected types of the args //TODO 
 * @return 0 in case of success, -1 otherwise
 */
int validate_tlv(struct tlv* msg, unsigned int nargs){
    puts("Validate tlv");//
    unsigned int expected_size, nheaders, summ_lengths;
    expected_size = nheaders = summ_lengths = 0;

    uint16_t total_length = tlvget_length(msg);

    while (nheaders < nargs){
        nheaders++; 
        expected_size = nheaders*SIZE_HEADER_TLV + summ_lengths;    
        if (total_length < expected_size) return -1; 

        // TODO check also the types

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
                s = calloc(sizeof(struct sockaddr_in));
                if (s == NULL) return NULL;
                IN(s)->sin_family = AF_INET;
                memcpy(&IN(s)->sin_port,c->data,2);
                memcpy(&IN(s)->sin_addr.s_addr,&c->data[2],4);
            break;
        case 18:
                s = calloc(sizeof(struct sockaddr_in6));
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
                tlvset_length(c, 2+4);
                memcpy(c->data, &IN(s)->sin_port,2);
                memcpy(&c->data[2], &IN(s)->sin_addr.s_addr,4);
            break;
        case AF_INET6: 
                if((c = create_tlv(2+16)) == NULL) return NULL;
                tlvset_length(c, 2+16);
                memcpy(c->data, &IN6(s)->sin6_port,2);
                memcpy(&c->data[2], &IN(s)->sin6_addr.s6_addr,16);
            break;
        default: // Invalid addr type
                return NULL;
    }

    c->type = CLIENT;
    return c;
}
