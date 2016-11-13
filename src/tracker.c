// Here goes the code of the tracker
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
#include "../include/network.h"
#include "../include/tracker.h"

/******************** DEBUG functions ***********************/
void printhash(char* hash){
    int i;
    for (i=0;i<SHA256_HASH_SIZE;i++){
        printf("%x",hash[i]);
    }
    printf("\n");
}

/******************** END DEBUG functions ******************/

struct seed* htable[SIZE_HTABLE];

/**
 * Creates a struct seeder from a struct sockaddr
 * @param addr Pointer to the struct sockaddr
 */
struct seeder* create_seeder(struct sockaddr* addr){

    // Allocate a struct seeder
    struct seeder* s = malloc(sizeof(struct seeder));
    if (s == NULL) return NULL;

    // Attach address   
    s->addr = addr;

    // Add a timestamp
    s->lastseen = time();
    if (s->lastseen == -1){
        free(s);
        return NULL;
    }
    
    return s;
}

/**
 * Drop a given seeder freeing
 * the allocated memory
 * @param s A pointer to the seeder to drop
 */
void drop_seeder(struct seeder* s){
    free(s->addr);
    free(s);
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


/**
 * Search a given hash in a list of seeds
 * @param list A pointer to the first element of the list
 * @param hash The hash to search
 * @return A pointer to the seed containing the given hash
 *         or NULL if the hash has not been found
 */
struct seed* search_hash
(struct seed* list, const char* hash){
    puts("Start search hash");//
    while (list != NULL){
        printf("Compare: "); //
        printhash(list->hash);//
        if (memcmp(list->hash,hash,SHA256_HASH_SIZE) == 0){
            puts("Found");//
            return list;
        }
        list = list->next;
    }
    puts("Not found");//
    return NULL;
}



/**
 * Handle a message of type PUT_T storing 
 * the seeder and also the hash if necessary
 * @param m Message of type PUT_T
 * @return 0 in case of success, -1 otherwise
 */
int h_put_t(struct tlv* m){

    if (validate_tlv(m,2) != 0){
        puts("Message dropped: invalid length");
        return -1;
    }

    
    uint16_t mlength = tlvget_length(m);

    struct tlv* hash = (struct tlv*) m->data;
    uint16_t hlength = tlvget_length(hash);
    if (hlength != SHA256_HASH_SIZE) {
        puts("Drop msg: wrong hash type");
        return -1;
    }

    struct tlv* client = (struct tlv*) 
        m->data + SIZE_HEADER_TLV + hlength;
    uint16_t clength = tlvget_length(client);


    // Get the htable index and then search the seed
    unsigned int hti = htable_index(hash->data,hlength);
    struct seed* s = search_hash(htable[hti],hash->data);

    // If no seed corresponds to the hash create a new one
    if (s == NULL){
        puts("Create seed");//
        s = malloc(sizeof (struct seed));
        if (s == NULL) return -1;

        memcpy(s->hash,hash->data,hlength);
        s->seeders = NULL;
        s->next = htable[hti];
        htable[hti] = s;
    } 
   
    struct sockaddr* cl = client2sockaddr(client);  
    if (cl == NULL) return -1;
    
    struct seeder* seeder = create_seeder(cl);
    if (seeder == NULL) return -1;

    seeder->next = s->seeders;
    s->seeders = seeder;

    return 0;
}

/**
 * Handle a received msg
 * @param msg A pointer to the received msg
 * @param src_addr A struct giving some infos 
 *        about the sender
 * @param addrlen The actual size of the struct 
 *        pointed by src_addr
 * @return 0 in case of success, -1 otherwise
 */
int handle_msg(struct msg* m){

    // Check valid length
    uint16_t mlenght = tlvget_length(m->tlv);
    if (mlenght > m->size) {
        printf("Message dropped: bad length\n"); 
        return 0;
    }


    //TODO complete
    switch (m->tlv->type){
        case PUT_T: h_put_t(m->tlv);
            break;
        case GET: puts("GET");
            break;
        case KEEP_ALIVE: puts("KEEP_ALIVE");
            break;
        default: puts("INVALID");
    }
    
    return 0;
}



int main(int argc, char* argv[]){
    // Listen on localhost port 5555
    // XXX those values are just a test
    int sockfd = init_connection("127.0.0.1",5555);

    struct msg* m = accept_msg(sockfd);

    // Deal with the msg
    if (handle_msg(m) == -1) perror("");

    drop_msg(m);
    return 0;
}
  
