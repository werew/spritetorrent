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


int handle_msg(st_ctask ctask, struct msg* m){
    puts("Handle msg");
    switch (m->tlv->type){
        case GET: puts("Get chunk");
            break;
        case LIST: puts("List");
            break;
        case REP_LIST:
        case ACK_PUT: 
        case ACK_GET: 
        case ACK_KEEP_ALIVE: rm_answered(ctask, m);
            break;
        default: puts("Bad msg type");
    }
    return 0;
}




void rm_answered(st_ctask ctask, struct msg* m){
    // Look at the corresponding type of request
    char req_type;
    switch (m->tlv->type){
        case REP_LIST: req_type = LIST;
            break;
        case ACK_PUT:  req_type = PUT_T;
            break;
        case ACK_GET:  req_type = GET;
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


int send_req(st_ctask ctask, struct request* req){
    time_t t = time(NULL);
    if (t == -1 || send_msg(ctask->sockfd, req->msg) == -1) return -1;
    req->timestamp = t;
    req->attempts++;
    return 0;
}

void push_req(st_ctask ctask, struct request* req){
    req->next = ctask->req_poll;
    ctask->req_poll = req;
}

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



struct chunk* make_chunkslist(const char* filename){
    return NULL;
}






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









int st_get(st_ctask ctask, const char hash[SHA256_HASH_SIZE], 
const char* filename){
    // Get from tracker
    // Get from clients
    return 0;
}

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

    st_cstart(ctask);

    return 0;
}
  
