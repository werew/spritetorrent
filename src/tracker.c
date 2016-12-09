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
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "types.h"
#include "network.h"
#include "tracker.h"
#include "debug.h"


/**
 * Creates a new ttask suitable to start a tracker
 * @param max_hosts The number of clients manageable by this tracker
 * @param timeout The number of seconds required for an hosts to
 *        to became inactive (and be consequently removed).
 * @return a new ttask of NULL in case of error
 * @note A ttask is created so that the tracker will listen from
 *       all its IPv4 and IPv6 interfaces
 */
st_ttask st_create_ttask
(uint16_t port, unsigned int max_hosts, time_t timeout){
    st_ttask ttask = calloc(1,sizeof(struct ttask));
    if (ttask == NULL) return NULL;

    ttask->sockfd = bound_socket(port);
    if (ttask->sockfd == -1) {
        free(ttask);
        return NULL;
    }
    ttask->hosts_max = max_hosts;
    ttask->timeout = timeout;

    return ttask;
}

/**
 * This function listen, accept and handle all the incoming
 * messages for all the duration of the timeout
 * @param ttask An active st_ttask
 * @return 0 in case of success, -1 otherwise
 */
int st_twork(st_ttask ttask){
    time_t now, ltime;
    struct msg* m;
    struct pollfd pfd;

    // Init pollfd
    pfd.fd = ttask->sockfd;
    pfd.events = POLLIN;

    while (1){

        // Timeout: finish if too much time has passed
        // since the last probe 
        if ((now = time(NULL)) == -1) return -1;
        if (now - ttask->last_probe > ttask->timeout) break;

        // Wait for incoming messages during the remaining time
        ltime = ttask->timeout - (now-ttask->last_probe);

        int ret =  poll(&pfd, 1, ltime * 1000);
        if (ret == -1) return -1;  // error
        else if (ret == 0 ) break; // timeout

        // A message has arrived
        printf("Accept\n");
        if ((m = accept_msg(ttask->sockfd)) == NULL) return -1;
                
        if (handle_msg(ttask, m) == -1) return -1;
    }

    return 0;
}


/**
 * Launch a tracker
 * @param ttask A ttask previously initialized
 * @return -1 in case of error
 */
int st_tstart(st_ttask ttask){
   

    // Main work/probe loop 
    while(1){
        // Update last_probe timestamp
        ttask->last_probe = time(NULL); 
        if (ttask->last_probe == -1) return -1;
        
        // listen/handle/reply
        printf("Wait\n");
        if (st_twork(ttask) == -1) return -1;

        // TODO Probe          
        printf("Probe\n");
    }

    return 0; 
}

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
    s->lastseen = time(NULL);
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
int h_put_t(st_ttask ttask, struct msg* m){


    // Check against max number of hosts
    if (ttask->hosts_count >= ttask->hosts_max){
        puts("Limit of hosts for this server");
        return 0;
    }

    // Some tests on the format of the message
    if (validate_tlv(m->tlv,2) != 0){
        puts("Message dropped: invalid format");
        return 0;
    }


    struct tlv* hash = (struct tlv*) m->tlv->data;
    uint16_t hlength = tlvget_length(hash);
    if (hlength != SHA256_HASH_SIZE) {
        puts("Drop msg: wrong hash type");
        return -1;
    }


    // Get the htable index and then search the seed
    unsigned int hti = htable_index(hash->data,hlength);
    struct seed* s = search_hash(ttask->htable[hti],hash->data);

    // If no seed corresponds to the hash append a new one
    if (s == NULL){
        puts("Create seed");//
        s = malloc(sizeof (struct seed));
        if (s == NULL) return -1;

        memcpy(s->hash,hash->data,hlength);
        s->seeders = NULL;
        s->next = ttask->htable[hti];

        ttask->htable[hti] = s;
    } 
  
    // Append seeder (note that duplicate seeders for the
    // same seed are allowed, protection against DoS attacks
    // is garanted by the hosts_max attribute of the st_ttask
    // and the fact that KEEP_ALIVE ops are performed only on
    // the first match)
    struct tlv* client = (void*) m->tlv->data + 
                         SIZE_HEADER_TLV + hlength;

    struct sockaddr* cl = client2sockaddr(client);  
    if (cl == NULL) return -1;
    
    struct seeder* seeder = create_seeder(cl);
    if (seeder == NULL) return -1;

    seeder->next = s->seeders;
    s->seeders = seeder;

    // Keep track of the number of seeders
    ttask->hosts_count++;
    printf("%d\n",ttask->hosts_count);


    // ACK PUT
    m->tlv->type = ACK_PUT;
    if (send_msg(ttask->sockfd, m) == -1) return -1;
    
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
int handle_msg(st_ttask ttask, struct msg* m){

    // Check valid length
    uint16_t mlenght = tlvget_length(m->tlv);
    if (mlenght > m->size) {
        printf("Message dropped: bad length\n"); 
        return 0;
    }


    //TODO complete
    switch (m->tlv->type){
        case PUT_T: h_put_t(ttask, m);
            break;
        case GET:   //h_get_t(ttask, m)
            break;
        case KEEP_ALIVE: puts("KEEP_ALIVE");
            break;
        default: puts("INVALID");
    }
    
    return 0;
}


void fail(const char* emsg){
    perror(emsg);
    exit(1);
}

int main(int argc, char* argv[]){

/* OLD
    // Listen on localhost port 5555
    // XXX those values are just a test
    int sockfd = init_connection("127.0.0.1",5555);

    struct msg* m = accept_msg(sockfd);

    // Deal with the msg
    if (handle_msg(m) == -1) perror("");

    drop_msg(m);
*/

    st_ttask ttask = st_create_ttask(5555, 10, 10);
    if (ttask == NULL) fail("st_create_ttask");

    if (st_tstart(ttask) == -1) fail("st_tstart");

    return 0;
}
  
