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
#include "client.h"
#include "sha256.h"
#include "debug.h"


void fail(const char* emsg){
    perror(emsg);
    exit(1);
}


/**
 * Creates a new ctask suitable to use a client
 * @param port Port to use
 * @param timeout Frequency of the keep alive
 * @return a new ctask of NULL in case of error
 */
st_ctask st_create_ctask(uint16_t port, time_t timeout){
    st_ctask ctask = calloc(1,sizeof(struct ctask));
    if (ctask == NULL) return NULL;

    ctask->sockfd = bound_socket(port);
    if (ctask->sockfd == -1) {
        free(ctask);
        return NULL;
    }

    ctask->timeout = timeout;

    return ctask;
}

/**
 * Launch a client on a given ctask. This 
 * function is blocking.
 * @param ctask
 * @return 0 in case of success, -1 otherwise
 */
int st_cstart(st_ctask ctask){

    // Update lastupdate timestamp
    ctask->lastupdate = time(NULL); 
    if (ctask->lastupdate == -1) return -1;

    // Main work/update loop 
    while(1){
        
        // listen/handle/reply
        printf("Wait\n");
        if (st_cwork(ctask) == -1) return -1;
        
        // Forge KAs
        ka_gen(ctask);

        // Update poll
        poll_runupdate(ctask);

    }

    return 0; 
}




/**
 * Client worker: accept message -> handle msg
 * @param ctask 
 */
int st_cwork(st_ctask ctask){
    time_t now, ltime;
    struct msg* m;
    struct pollfd pfd;

    // Init pollfd
    pfd.fd = ctask->sockfd;
    pfd.events = POLLIN;

    while (1){

        // Timeout: finish if too much time has passed
        // since the last probe 
        if ((now = time(NULL)) == -1) return -1;
        if (now - ctask->lastupdate > ctask->timeout) break;

        // Wait for incoming messages during the remaining time
        ltime = ctask->timeout - (now-ctask->lastupdate);

        int ret =  poll(&pfd, 1, ltime * 1000);
        if (ret == -1) return -1;  // error
        else if (ret == 0 ) break; // timeout

        // A message has arrived
        printf("Accept\n");
        if ((m = accept_msg(ctask->sockfd)) == NULL) return -1;
                
        if (handle_msg(ctask, m) == -1) return -1;
    }

    return 0;
}



int poll_runupdate(st_ctask ctask){
    puts("Update POLL");
    ctask->lastupdate = time(NULL); 
    if (ctask->lastupdate == -1) return -1;
    return 0;
}

int ka_gen(st_ctask ctask){
    puts("KA generation");
    return 0;
}



/**
 * Handle an incoming msg from the worker.
 * As this function is related to the worker it does not
 * handle msgs reserved to the trasmissions threads as:
 * REP_LIST and REP_GET. This function will launch a new
 * transmission wheter the message received is a GET_C message.
 * @param ctask
 * @param m Message received
 * @return -1 in case of error, 0 otherwise
 */
int handle_msg(st_ctask ctask, struct msg* m){
    puts("Handle msg");

    switch (m->tlv->type){
        case GET_C: puts("received GET_C: new transmission");
            break;

        case LIST: puts("rceived LIST");
            break;

        case ACK_PUT: 
        case ACK_GET: 
        case ACK_KEEP_ALIVE: puts("received ACK"); 
                rm_answered(ctask, m);
            break;
        default: puts("Bad msg type");
    }
    return 0;
}










/**
 * Scan the poll for the request which triggered the
 * given message as answer remove it from the poll.
 * @param ctask
 * @param m Answer message
 */
void rm_answered(st_ctask ctask, struct msg* m){
    // Look at the corresponding type of request
    char req_type;
    switch (m->tlv->type){
        case REP_LIST: req_type = LIST;
            break;
        case ACK_PUT:  req_type = PUT_T;
            break;
        case ACK_GET:  req_type = GET_T;
            break;
        case ACK_KEEP_ALIVE: req_type = KEEP_ALIVE;
            break;
        default: puts("Bad msg type");
            return;
    }

    // Scan the poll for the corresponding request
    struct request* prev = NULL;
    struct request* current = ctask->req_poll;
     
    while (current != NULL){

        if (req_type == current->msg->tlv->type){

            // Debug    
            printf("Compare addr\n");
            printsockaddr((struct sockaddr*) &m->addr);
            printsockaddr((struct sockaddr*) &current->msg->addr);

            if (sockaddr_cmp((struct sockaddr*) &m->addr,        
               (struct sockaddr*) &current->msg->addr) == 1){

                printf("Matched addr\n");
                if (tlv_cmp((struct tlv*) current->msg->tlv->data,
                        (struct tlv*) m->tlv->data) == 1){
                    
                    printf("Matched data\n");

                    if (prev == NULL) ctask->req_poll = current->next;
                    else  prev->next = current->next;

                    // Request fond ! We can drop it !
                    drop_msg(current->msg);
                    free(current);
                }

            }
        }

        prev = current;
        current = current->next;
    }
}




/**
 * Sends a request and update its fields
 * @param ctask Used to get the socket
 * @param req Request to send
 * @return 0 in case of success, -1 otherwise
 */
int send_req(st_ctask ctask, struct request* req){
    time_t t = time(NULL);
    if (t == -1 || send_msg(ctask->sockfd, req->msg) == -1) return -1;
    req->timestamp = t;
    req->attempts++;
    return 0;
}



/**
 * Push a request inside the poll
 * @param ctask 
 * @param req
 */
void push_req(st_ctask ctask, struct request* req){
    req->next = ctask->req_poll;
    ctask->req_poll = req;
}




/**
 * Sends a message PUT_T to a tracker. Doesn't wait for an 
 * answer, the worker whould deal with it.
 * @param ctask
 * @param tracker Tracker to which send the msg
 * @param hash  Hash to put
 * @param local Address to declare
 * @return 0 in case of success, -1 otherwise
 */
int _put_t(st_ctask ctask, const struct sockaddr* tracker, 
           const char* hash, struct sockaddr* local){

    uint16_t size_addr = (local->sa_family == AF_INET)? 4:16; 

    // Create msg of type PUT: 
    // [FILE_HASH LEN HASH][CLIENT LEN PORT ADDR]
    struct msg* m = create_msg(SIZE_HEADER_TLV*2 + 
                               SHA256_HASH_SIZE  + 
                               2 + size_addr , tracker);
    if (m == NULL) return -1;

    /** Fill message **/
    m->tlv->type = PUT_T;

    // hash
    struct tlv* tlv_hash = (struct tlv*) m->tlv->data;
    tlv_hash->type = FILE_HASH;
    tlvset_length(tlv_hash, SHA256_HASH_SIZE);
    memcpy(tlv_hash->data, hash, SHA256_HASH_SIZE);
    
    // client
    struct tlv* tlv_client = (struct tlv*)
                             &tlv_hash->data[SHA256_HASH_SIZE];
    struct tlv* tmp = sockaddr2client(local);
    if (tmp == NULL) goto err_1;
    memcpy(tlv_client, tmp, SIZE_HEADER_TLV+2+size_addr);
    drop_tlv(tmp);
   

    // Build and send request 
    struct request* put_req = calloc(1,sizeof(struct request));
    if (put_req == NULL) goto err_1;
    put_req->msg = m;

    // Push req into the poll
    push_req(ctask, put_req);

    if (send_req(ctask, put_req) == -1) return -1;

    return 0;

err_1:
    drop_msg(m);
    return -1;
}


/**
 * Create a list of hash-chunks from the given file
 * @param filename
 * @return A linked list of struct chunk or NULL in case of error
 */
struct chunk* make_chunkslist(const char* filename){

    FILE* f = fopen(filename, "r");
    if (f == NULL) return NULL;

    int index = 0;
    struct chunk* list = calloc(1,sizeof(struct chunk));
    struct chunk* chunk = list;
    
    while (1) {

        if (chunk == NULL) return NULL; // TODO free other chunks
        
        if (fsha256(chunk->hash, f, 
            CHUNK_SIZE*index, CHUNK_SIZE) == -1) return NULL; // XXX

        printhash(chunk->hash);

        chunk->index = index++; 
        chunk->status = AVAILABLE;

        if (feof(f) != 0) break;

        chunk->next = calloc(1,sizeof(struct chunk));
        chunk = chunk->next;

    }

    fclose(f);

    return list;
}





/**
 * Declares a file to all the trackers
 * @param ctask
 * @param filename
 * @return 0 or -1 in case of error
 */
int st_put(st_ctask ctask, const char* filename){
    /********* Store locally: make a seed **********/

    struct c_seed* s = calloc(1,sizeof (struct c_seed));
    if (s == NULL) return -1;

    // File hash
    if (sha256(s->hash,filename,0,-1) == -1) goto err_1;

    // File name
    s->filename = strdup(filename);
    if (s->filename == NULL) goto err_1;

    // Chunks
    s->chunks = make_chunkslist(filename);
    if (s->filename == NULL) goto err_2;

    // Push into htable
    unsigned int hti = htable_index(s->hash,SHA256_HASH_SIZE);
    s->next = ctask->htable[hti];
    ctask->htable[hti] = s;


    /*************** Send message PUT_T ***********/

    struct host* local   = NULL;
    struct host* tracker = ctask->trackers;

    while (tracker != NULL){
        local = ctask->locals; 
        while (local != NULL){
            if (_put_t(ctask, tracker->addr, 
                       s->hash, local->addr) == -1){
               return -1; 
            }
            local = local->next;
        }
        tracker = tracker->next;
    }

    return 0;    

err_2:
    free(s->filename);
err_1:
    free(s);
    return -1;
}



/********************* IN TRANSMISSION ***********************/




int handle_ack_get(struct in_trasmission* it, struct msg* m){
    // Check msg type
    if (m->tlv->type != ACK_GET) return -1;
   
    // Check hash 
    struct tlv* hash = (struct tlv*) m->tlv->data;
    if (hash->type != FILE_HASH                 ||
        tlvget_length(hash) != SHA256_HASH_SIZE ||
        memcmp(hash->data, it->hash, SHA256_HASH_SIZE) != 0 ){
        return -1;
    }

    // Get clients
    uint16_t length_hash = SIZE_HEADER_TLV + SHA256_HASH_SIZE;
    uint16_t length_clients = tlvget_length(m->tlv) - length_hash;    
    struct tlv* client = (struct tlv*) &m->tlv->data[length_hash];

    while (length_clients > 0){

        struct sockaddr* addr_seeder = client2sockaddr(client);
        if (addr_seeder == NULL) return -1;

        printsockaddr(addr_seeder);
        
        struct host* s = malloc(sizeof(struct host));
        if (s == NULL) {
            free(addr_seeder);
            return -1;
        }
    
        s->addr = addr_seeder;
        s->next = it->seeders;
        it->seeders = s;

        uint16_t c_size =  tlvget_length(client);
        length_clients -= SIZE_HEADER_TLV + c_size;
        client = (struct tlv*) &client->data[c_size];
    }

    return 0;
}



int get_t(struct sockaddr* tracker, struct in_trasmission* it){

    // Create GET_T msg
    struct msg* m = create_msg
        (SIZE_HEADER_TLV+SHA256_HASH_SIZE,tracker);
    if (m == NULL) return -1;

    // Fill message
    m->tlv->type = GET_T;
    struct tlv* tlv_hash = (struct tlv*) m->tlv->data;
    tlv_hash->type = FILE_HASH;
    tlvset_length(tlv_hash,SHA256_HASH_SIZE);
    memcpy(tlv_hash->data, it->hash, SHA256_HASH_SIZE);
   
    // Init pollfd
    struct pollfd pfd;
    pfd.fd = it->sockfd;
    pfd.events = POLLIN;

    int tries = 0; 
    struct msg* answer = NULL;
    while (tries++ < 3){

        if (send_msg(it->sockfd, m) == -1) return -1;
        
        // Wait answer for max 3 secs
        int ret =  poll(&pfd, 1, 3000);

        if (ret == -1) return -1;  // error
        else if (ret == 0 ) continue; // timeout

        // A message has arrived
        printf("---> Accept ACK GET\n");
        if ((answer = accept_msg(it->sockfd)) == NULL) return -1;

        ret = handle_ack_get(it, answer);
        drop_msg(m);
        drop_msg(answer);

        return ret;
    }

    drop_msg(m);
    if (answer != NULL) drop_msg(answer);

    return -1;
}


int list(struct in_trasmission* it){
    if (it->seeders == NULL) return 0;

    struct sockaddr* seeder = it->seeders->addr;
    struct msg* m = create_msg(SIZE_HEADER_TLV+
                    SHA256_HASH_SIZE,seeder);
    if (m == NULL) return -1;

    // Fill msg body
    m->tlv->type = LIST;
    struct tlv* hash = (struct tlv*) m->tlv->data;
    hash->type = FILE_HASH;
    memcpy(hash->data, it->hash, SHA256_HASH_SIZE);

    // Send LIST msg
    if (send_msg(it->sockfd, m) == -1){
        drop_msg(m);
        return -1;
    }
    

    // Handle answer
    struct msg* answer = accept_msg(it->sockfd);
    if (answer ==  NULL) return -1;

    return 0;
}



int st_get(st_ctask ctask, const char hash[SHA256_HASH_SIZE]){
    // Init trasmission
    struct in_trasmission it;
    it.sockfd = bound_socket(0); 
    if (it.sockfd == -1) return -1;

    memcpy(it.hash,hash,SHA256_HASH_SIZE);

    // Get clients
    struct host* tracker = ctask->trackers; // XXX use all
    if (get_t(tracker->addr, &it) == -1) return -1;

    // Get chunks
    list(&it);
    

    return 0;
}



/********************** CONFIG *******************************/

int st_addtracker(st_ctask ctask, const char* addr, uint16_t port){
    
    struct sockaddr* sockaddr = human2sockaddr(addr, port);
    if (sockaddr == NULL) return -1;
    
    struct host* tracker = malloc(sizeof(struct host));
    if (tracker == NULL) {
        free(sockaddr);
        return -1;
    }

    tracker->addr = sockaddr;
    tracker->next = ctask->trackers;
    ctask->trackers = tracker;

    return 0; 
}


int st_addlocal(st_ctask ctask, const char* addr, uint16_t port){
    
    struct sockaddr* sockaddr = human2sockaddr(addr, port);
    if (sockaddr == NULL) return -1;
    
    struct host* localaddr = malloc(sizeof(struct host));
    if (localaddr == NULL) {
        free(sockaddr);
        return -1;
    }

    localaddr->addr = sockaddr;
    localaddr->next = ctask->locals;
    ctask->locals = localaddr;
    return 0; 
}



int main(int argc, char* argv[]){

    st_ctask ctask = st_create_ctask(2222, 10);
    if (ctask == NULL) fail("st_create_ctask");

    st_addtracker(ctask,"127.0.0.1",5555);
    st_addlocal(ctask,"127.0.0.1",2222);

    st_put(ctask, "/etc/passwd");

    char hash[SHA256_HASH_SIZE];
    string_to_sha256((unsigned char*) hash,
"0d3e2e56a2d3cd9ed109d842d9e4aed3df34465c3bc38f59da6cdb18a7121d32");

    st_get(ctask, hash);

    st_cstart(ctask);

    return 0;
}
  
