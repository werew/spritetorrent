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
#include <getopt.h>
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
    int ka_switch = 1;
    while(1){
        
        // listen/handle/reply
        if (st_cwork(ctask) == -1) return -1;

        // Update poll
        poll_runupdate(ctask);

        // Forge KAs
        if (ka_switch == 0){
            ka_gen(ctask);
            ka_switch = 1;
        } else ka_switch = 0;

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
        if ((m = accept_msg(ctask->sockfd)) == NULL) return -1;
                
        if (handle_msg(ctask, m) == -1) return -1;
    }

    return 0;
}



int poll_runupdate(st_ctask ctask){
    ctask->lastupdate = time(NULL); 
    if (ctask->lastupdate == -1) return -1;

    // Scan the poll
    struct request* prev = NULL;
    struct request* current = ctask->req_poll;
     
    while (current != NULL){
        if (current->attempts > MAX_ATTEMPTS){
            // Drop old request

            if (prev == NULL) ctask->req_poll = current->next;
            else  prev->next = current->next;
            
            struct request* next = current->next;

            drop_msg(current->msg);
            free(current);

            current = next;

        } else {
            // Update request 
            send_req(ctask, current);
            prev = current;
            current = current->next;
        }

    }

    return 0;
}

int ka_gen(st_ctask ctask){
    int i;
    for (i=0; i<SIZE_HTABLE; i++){
        struct c_seed* seed = ctask->htable[i];
        while (seed != NULL){
            
            struct host* tracker = ctask->trackers;
            while (tracker != NULL){
                struct request* req = calloc(1,
                        sizeof(struct request));
                if (req == NULL) return -1;
    
                req->msg = create_msg(SIZE_HEADER_TLV+
                            SHA256_HASH_SIZE, tracker->addr);
                if (req->msg == NULL) {free(req); return -1;}

                req->msg->tlv->type = KEEP_ALIVE;

                struct tlv* filehash=(struct tlv*)req->msg->tlv->data;
                filehash->type = FILE_HASH;
                tlvset_length(filehash,SHA256_HASH_SIZE);
                memcpy(filehash->data, seed->hash, SHA256_HASH_SIZE);

                send_req(ctask, req);
                push_req(ctask, req);

                tracker = tracker->next;
            }
            seed = seed->next; 
        }
    }
    
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

    switch (m->tlv->type){
        case GET_C: rep_get(ctask,m);
            break;
        case LIST: rep_list(ctask, m);
            break;
        case ACK_PUT: 
        case ACK_GET: 
        case ACK_KEEP_ALIVE: 
                rm_answered(ctask, m);
            break;
        default: puts("Bad msg type");
    }
    return 0;
}


int rep_get(struct ctask* ctask, struct msg* m){

    // Get file hash
    struct tlv* hash = (struct tlv*) m->tlv->data;
    if (hash->type != FILE_HASH) return -1;

    // Search into the htable 
    int idx = htable_index(hash->data, SHA256_HASH_SIZE);
    struct c_seed* s = search_hash_c(ctask->htable[idx],hash->data);
    if (s == NULL) return 0; // File not found

    hash = (struct tlv*) &hash->data[SHA256_HASH_SIZE];

    // Find chunk
    struct chunk* c = s->chunks;
    while (c != NULL){
        if (memcmp(hash->data, c->hash,SHA256_HASH_SIZE) == 0 &&
            c->status == AVAILABLE ) break;
        c = c->next;
    }
    if (c == NULL) return 0;    // Chunk not found


    // Log
    char shash[SHA256_HASH_SIZE*2];
    sha256_to_string(shash,c->hash);
    printf(BLUE"Transmitting %s of \"%s\"\n"CRESET,shash,s->filename);



    transmit_chunk(ctask, m, s, c);

    
    return 0;
}



int transmit_chunk(struct ctask* ctask, struct msg* m,
    struct c_seed* seed, struct chunk* c){

    FILE* file = fopen(seed->filename, "r");
    if (file == NULL) return -1;

    // Calculate size chunk
    fseek(file, 0, SEEK_END);
    size_t file_size  = ftell(file);
    size_t offset  = c->index*CHUNK_SIZE;
    size_t size_chunk = (file_size - offset < CHUNK_SIZE)?
           file_size - offset : CHUNK_SIZE;

    // The result is: max_frags = ceil(size_chunk/FRAG_SIZE)-1;
    uint16_t max_frags = (size_chunk+FRAG_SIZE-1)/FRAG_SIZE-1;
    uint16_t index  = 0;
    
    // Move to the chunk
    fseek(file, offset, SEEK_SET);    

    // Prepare message
    struct msg* asw = create_msg(tlvget_length(m->tlv) + 
                        SIZE_HEADER_TLV+4+FRAG_SIZE, 
                        (struct sockaddr*) &m->addr);
    if (m == NULL) { fclose(file); return -1; }

    // Base data
    asw->tlv->type = REP_GET;
    memcpy(asw->tlv->data, m->tlv->data, tlvget_length(m->tlv));
    size_t fixedlength = tlvget_length(m->tlv)+SIZE_HEADER_TLV+4;
    struct tlv* frag = (struct tlv*)&asw->tlv->data
                        [tlvget_length(m->tlv)];
    frag->type = CHUNK_FRAG;
    memcpy(&frag->data[2], &max_frags, 2);
    

    // Start transmission
    while (index <= max_frags){
        size_t frag_size = (size_chunk < FRAG_SIZE)?
                            size_chunk : FRAG_SIZE;
        size_chunk -= frag_size;

        tlvset_length(frag, frag_size+4);
        asw->size = fixedlength + frag_size;
        memcpy(frag->data, &index, 2);

        fread(&frag->data[4], 1, frag_size, file); //!= frag_size
        sleep(1);
            send_msg(ctask->sockfd, asw);/* == -1){
            drop_msg(asw);
            fclose(file);
            return -1;
        }
*/
        
        printf(CYAN"---> Fragment %d of %d. Size %ld\n"CRESET,
                   index, max_frags, frag_size);
        index++;
    }

    drop_msg(asw);
    fclose(file);
    return 0;
}



struct c_seed* search_hash_c
(struct c_seed* list, const char* hash){
    while (list != NULL){
        if (memcmp(list->hash,hash,SHA256_HASH_SIZE) == 0){
            return list;
        }
        list = list->next;
    }
    return NULL;
}

int rep_list(struct ctask* ctask, struct msg* m){

    // Get file hash
    struct tlv* hash = (struct tlv*) m->tlv->data;
    if (hash->type != FILE_HASH) return -1;

        // Log
        char shash[SHA256_HASH_SIZE*2];
        sha256_to_string(shash,hash->data);
        printf(YELLOW"\t %s\n"CRESET, shash);

    // Search into the htable 
    int idx = htable_index(hash->data, SHA256_HASH_SIZE);
    struct c_seed* s = search_hash_c(ctask->htable[idx],hash->data);
    if (s == NULL) return -1;
    
    // Chunks (they are stored inside the seed_c)
    void* chunklist;
    ssize_t size_chunklist = chunks2chunklist(s->chunks, &chunklist); 
    if (size_chunklist == -1) return -1;

    // Prepare REP_LIST
    size_t size_filehash = SIZE_HEADER_TLV+SHA256_HASH_SIZE;
    struct msg* asw = create_msg(size_filehash+size_chunklist, 
                      (struct sockaddr*) &m->addr);
    asw->tlv->type = REP_LIST;
    tlvset_length(asw->tlv,SIZE_HEADER_TLV+
                 SHA256_HASH_SIZE+size_chunklist);
    memcpy(asw->tlv->data, hash, size_filehash);
    memcpy(&asw->tlv->data[size_filehash], chunklist, size_chunklist);

    // Send answer
    int ret = send_msg(ctask->sockfd, asw);

    free(chunklist);
    drop_msg(asw);

    return ret;
}




ssize_t chunks2chunklist (struct chunk* c, void** dest){
    
    size_t mem_size = 512;
    size_t used_size = 0;
    void* mem = malloc(mem_size);
    if (mem == NULL) return -1;

    struct tlv* hash= create_tlv(SHA256_HASH_SIZE+2);
    hash->type = CHUNK_HASH;
    size_t size_chunk = SIZE_HEADER_TLV+SHA256_HASH_SIZE+2;

    while (c != NULL){

        // Only chunks that are actually availables
        if (c->status != AVAILABLE){
            c = c->next;
            continue;
        }

        // Copy chunk's data
        memcpy(hash->data, c->hash, SHA256_HASH_SIZE+2); 

        // Resize buf if necessary 
        if (used_size + size_chunk > mem_size){
            mem_size += size_chunk + 512;
            void* tmp = realloc(mem, mem_size);
            if (tmp == NULL) { free(mem); return -1;}
            mem = tmp;
        }

        // Copy chunk
        memcpy((char*) mem+used_size, hash, size_chunk);
        used_size += size_chunk;

        c = c->next;
    }

    *dest = mem;
    return used_size;
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

            if (sockaddr_cmp((struct sockaddr*) &m->addr,        
               (struct sockaddr*) &current->msg->addr) == 1){

                if (tlv_cmp((struct tlv*) current->msg->tlv->data,
                        (struct tlv*) m->tlv->data) == 1){
                    
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

    // Log
    char addr[100];
    if (req->msg->addr.ss_family == AF_INET){
        inet_ntop(AF_INET, &IN(&req->msg->addr)->sin_addr, 
                  addr, 100);
    } else {
        inet_ntop(AF_INET6, &IN6(&req->msg->addr)->sin6_addr, 
                  addr, 100);
    }
    printf(CYAN"Sent %d to %s : %d attempts\n"CRESET,
           req->msg->tlv->type, addr, req->attempts);

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

        if (chunk == NULL) return NULL;
        
        if (fsha256(chunk->hash, f, 
            CHUNK_SIZE*index, CHUNK_SIZE) == -1) return NULL; 

        // Log
        char hash[SHA256_HASH_SIZE*2];
        sha256_to_string(hash,chunk->hash);
        printf(YELLOW"\t  chunk%d: %s\n"CRESET, index, hash);

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

    // Log
    char hash[SHA256_HASH_SIZE*2];
    sha256_to_string(hash,s->hash);
    printf(YELLOW"PUT file \"%s\"\n\tFILEHASH: %s\n"CRESET,
          filename , hash);

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

    // Log
    char addr[100];
    if (m->addr.ss_family == AF_INET){
        inet_ntop(AF_INET, &IN(&m->addr)->sin_addr, 
                  addr, 100);
    } else {
        inet_ntop(AF_INET6, &IN6(&m->addr)->sin6_addr, 
                  addr, 100);
    }
    printf(YELLOW"\tTry seeder %s\n"CRESET,addr);


    // Send LIST msg
    if (send_msg(it->sockfd, m) == -1){
        drop_msg(m);
        return -1;
    }
    
    // Handle answer
    struct msg* answer = accept_msg(it->sockfd);
    if (answer ==  NULL) return -1;

    handle_rep_list(it, answer);

    return 0;
}





int handle_rep_list(struct in_trasmission* it, struct msg* m){
    if (m->tlv->type != REP_LIST) return -1;

    size_t length = tlvget_length(m->tlv);

    struct tlv* hash = (struct tlv*) m->tlv->data;

    // Is it the correct file hash ?
    if (hash->type != FILE_HASH ||
        memcmp(hash->data, it->hash, 
        SHA256_HASH_SIZE) != 0){
        return -1;
    }

    hash = (struct tlv*) &hash->data[SHA256_HASH_SIZE];
    length -= SIZE_HEADER_TLV + SHA256_HASH_SIZE;
    while (length > 0){

               
        if (hash->type != CHUNK_HASH) return -1;
        
        struct chunk* c = calloc(1,sizeof(struct chunk));
        if (c == NULL) return -1;

        
        memcpy(c->hash,hash->data,SHA256_HASH_SIZE+2);

        // Log
        char shash[SHA256_HASH_SIZE*2];
        sha256_to_string(shash,c->hash);
        printf(YELLOW"\tGot chunk%d: %s\n"CRESET, c->index, shash);

        c->next = it->chunks;
        it->chunks = c;
        
        length -= SIZE_HEADER_TLV+SHA256_HASH_SIZE+2;
        hash = (struct tlv*) &hash->data[SHA256_HASH_SIZE+2];
    }

    

   return 0; 

}



int st_get(st_ctask ctask, 
const char hash[SHA256_HASH_SIZE], char* filename){
    // Init trasmission
    struct in_trasmission it;
    memset(&it,0,sizeof(struct in_trasmission));
    it.sockfd = bound_socket(0); 
    if (it.sockfd == -1) return -1;
    memcpy(it.hash,hash,SHA256_HASH_SIZE);

    // Log
    char shash[SHA256_HASH_SIZE*2];
    sha256_to_string(shash,hash);
    printf(YELLOW"GET: %s\n"CRESET, shash);

    // Get clients
    struct host* tracker = ctask->trackers;
    if (get_t(tracker->addr, &it) == -1) return -1;

    // Get chunks
    list(&it);
    struct chunk* c = it.chunks;
    while (c != NULL){
        int tries = 0;
        while (tries < 3){ 
            struct msg* req = get_c(&it, c, it.seeders);
            if (req == NULL) return -1;
            int ret = receive_chunk(&it, c, req, filename);
            if (ret == 0) break;
            else if (ret == 1) tries++;
        }
        c = c->next;
    }

    return 0;
}




int receive_chunk(struct in_trasmission* it, 
    struct chunk* c, struct msg* req, char* filename){

    
    struct msg* asw;
    uint16_t max_frags, index, read = 0;
    struct pollfd pfd;
    pfd.fd = it->sockfd;
    pfd.events = POLLIN;

    // Get first message
    int ret =  poll(&pfd, 1, 3000);
    if (ret == -1) return -1;  // error
    else if (ret == 0 ) return 1; // timeout
    if ((asw = accept_msg(it->sockfd)) == NULL) return -1;

    // Validate msg (the prelude should correspond to the req)
    size_t size_prelude = (SIZE_HEADER_TLV+SHA256_HASH_SIZE)*2+2;
    if (memcmp(req->tlv->data, asw->tlv->data, 
        size_prelude) != 0 ) goto error_1;

    // Read index and max_frags
    struct tlv* frag = (struct tlv*) &asw->tlv->data[size_prelude];
    memcpy(&max_frags, &frag->data[2], 2);
    memcpy(&index, frag->data, 2);

    char* received = calloc(1,max_frags);
    if (received == NULL) goto error_1;

    int f = open(filename,O_WRONLY|O_CREAT,0660);
    if (f == -1) goto error_2;

    while (1){

        // Write to file
        lseek(f, CHUNK_SIZE*c->index + FRAG_SIZE*index, SEEK_SET);
        size_t size_frag = tlvget_length(frag)-4;
        if (write(f, &frag->data[4], size_frag) < size_frag){
            goto error_2;
        }

        // Count fragment 
        if (received[index] == 0) {
            received[index] = 1;
            read++;
            if (read > max_frags) break;
        }

        // Get a new message
        int ret =  poll(&pfd, 1, 3000);
        if (ret == -1) return -1;  // error
        else if (ret == 0 ) {      // timeout
            free(received);
            drop_msg(asw);
            close(f);
            return 1;
        }

        if ((asw = accept_msg(it->sockfd)) == NULL) goto error_3;

        // Validate msg (the prelude should correspond to the req)
        size_t size_prelude = (SIZE_HEADER_TLV+SHA256_HASH_SIZE)*2+2;
        if (memcmp(req->tlv->data, asw->tlv->data, 
            size_prelude) != 0 ) goto error_1;

        // Read fragment
        frag = (struct tlv*) &asw->tlv->data[size_prelude];
        memcpy(&index, frag->data, 2);
    }
 
    drop_msg(asw); 
    free(received); 
    close(f); 
    return 0;

error_3:
    close(f);
error_2:
    free(received);
error_1:
    drop_msg(asw);
    return -1;
}

struct msg* get_c(struct in_trasmission* it,
                  struct chunk* c,struct host* h){

    
    // LOG
    char shash[SHA256_HASH_SIZE*2];
    sha256_to_string(shash,c->hash);
    printf(CYAN"\tGET_C %s\n"CRESET, shash);


    // Craft GET_C
    struct msg* m = create_msg(SIZE_HEADER_TLV*2+
                    SHA256_HASH_SIZE*2+2,h->addr);
    if (m == NULL) return NULL;

    m->tlv->type = GET_C;
    struct tlv* hash = (struct tlv*) m->tlv->data;
    hash->type = FILE_HASH;
    tlvset_length(hash, SHA256_HASH_SIZE);
    memcpy(hash->data, it->hash, SHA256_HASH_SIZE);

    hash = (struct tlv*) &hash->data[SHA256_HASH_SIZE];
    hash->type = CHUNK_HASH;
    tlvset_length(hash, SHA256_HASH_SIZE+2);
    memcpy(hash->data, c->hash, SHA256_HASH_SIZE+2);


    // Send GET_C msg
    if (send_msg(it->sockfd, m) == -1){
        drop_msg(m);
        return NULL;
    }
    
        
    return m;
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

    printf(CYAN"Using tracker: %s %d\n"CRESET,addr, port);
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

    printf(CYAN"Declare local addr: %s %d\n"CRESET,addr, port);
    return 0; 
}


/**
 * Prints some information about the usage of the program
 * @param name Name of the executable
 * @param exit_value Exit value of the program
 * @param help Print of not the help
 */
void usage(const char* name, int exit_value, int help){
    printf("usage: %s [-h] [-g hash] [-p file] [-l localaddr]"
           "[-t trackeraddr port] <port>\n",name);
    if (help == 1) {
        printf( "\n"
                "-h                     Shows this help\n"
                "-g hash                Get a file     \n"
                "-p file                Put a file     \n"
                "-l localaddr           Declare local addr\n"
                "-t trackeraddr port    Add a tracker  \n"
                "\n"
        );
    }
        
    exit(exit_value);
}


int main(int argc, char* argv[]){


    // Option pointers
    char* get[10][2];     int iget = 0;
    char* put[10];        int iput = 0;
    char* local[10];      int iloc   = 0;
    char* track[10][2];   int itrack = 0;

    // Get options
    int opt;
    while ((opt = getopt (argc, argv, "hg:p:l:t:")) != -1){

        if (iget > 10 || iput > 10 || iloc > 10 || itrack > 10){
            fprintf(stderr, "Way too many options\n");
        }

        switch (opt){
            case 'h': usage(argv[0], 0, 1);
                break;
            case 'g': get[iget][0]    = optarg;
                      get[iget++][1]  = argv[optind];
                break ;
            case 'p': put[iput++]     = optarg;
                break ;
            case 'l': local[iloc++]   = optarg;
                break ;
            case 't': track[itrack][0] = optarg;
                      track[itrack++][1] = argv[optind];
                break ;
            default: usage(argv[0], 1, 0);
        }
    }

    if (argc - optind < 1) usage(argv[0], 1, 0);
    
    uint16_t port = atoi(argv[optind+1]);

    st_ctask ctask = st_create_ctask(port, 10);
    if (ctask == NULL) fail("st_create_ctask");

    // Add local addr
    while (iloc-- > 0) {
        st_addlocal(ctask,local[iloc],port);
    }
    
    // Add trackers
    while (itrack-- > 0) {
        st_addtracker(ctask,track[itrack][0],
                atoi(track[itrack][1]));
    }


    // Get and push files
    int i;
    for (i = 0; i < 10; i++){
        if (i < iget) {
            unsigned char hash[SHA256_HASH_SIZE];
            string_to_sha256(hash, get[i][0]);
            st_get(ctask, (char*) hash, get[i][1]);
        }
        if (i < iput) {
            st_put(ctask, put[i]);
        }
    }

    // Main worker
    st_cstart(ctask);

    return 0;
}
  
