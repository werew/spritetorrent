#ifndef _TRACKER_H_
#define _TRACKER_H_

#include "sha256.h"


struct request {
    struct msg* msg;            // The body of the request
    time_t timestamp;           // When was it last done
    int attempts;               // Number of times it was sent
    struct request* next;       
};


#define SIZE_HTABLE 100
typedef struct ctask {
    int sockfd;                 // Socket from where to listen
    time_t timeout;             // Update frequency
    time_t lastupdate;          // Time of the last update
    struct host* locals;        // Local addresses to declare
    struct host* trackers;      // Trakers availables 
    struct request* req_poll;   // All the running requests
    struct c_seed* htable[SIZE_HTABLE];  // Hash table
} *st_ctask;

    
/******* Seeds, seeders and chunks *******/

/* A cell of a list of hosts  */
struct host {
    struct sockaddr* addr;   // Net infos (addr, port, ...)
    struct host* next;   // Next seeder of the list
};



typedef enum {
    AVAILABLE,
    BUSY,
    EMPTY
} chunk_status;


#define CHUNK_SIZE 700 //(1024*1024) // 1 Mb

struct chunk {
    char hash[SHA256_HASH_SIZE]; // Hash of the chunk
    uint16_t index;              // Position into the file (keep it
                                 //  after the hash)
    chunk_status status;         // ready/busy/empty
    struct chunk* next;
};

struct c_seed {
    char hash[SHA256_HASH_SIZE]; // Hash of the file
    char* filename;              // Path to the file
    struct chunk* chunks;        // List of the chunks
    struct c_seed* next;         // Next hash of the list
};


struct in_trasmission {
    int sockfd;
    char hash[SHA256_HASH_SIZE]; // Hash of the file
    struct chunk* chunks;        // List of the chunks
    struct host* seeders;
};


st_ctask st_create_ctask(uint16_t port, time_t timeout);

int st_cstart(st_ctask ctask);
int st_cwork(st_ctask ctask);

int ka_gen(st_ctask ctask);
int poll_runupdate(st_ctask ctask);
int handle_msg(st_ctask ctask, struct msg* m);
void rm_answered(st_ctask ctask, struct msg* m);
int st_addtracker(st_ctask ctask, const char* addr, uint16_t port);


int st_put(st_ctask ctask, const char* filename);
int _put_t(st_ctask ctask, const struct sockaddr* tracker, 
           const char* hash, struct sockaddr* local);
int send_req(st_ctask ctask, struct request* req);

ssize_t chunks2chunklist (struct chunk* c, void** dest);
int rep_list(struct ctask* ctask, struct msg* m);
struct c_seed* search_hash_c(struct c_seed* list, const char* hash);
int get_c(struct in_trasmission* it, struct chunk* c,struct host* h);
int handle_rep_list(struct in_trasmission* it, struct msg* m);

#endif
