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
    void* htable[SIZE_HTABLE];  // Hash table
} *st_ctask;

    
/******* Seeds, seeders and chunks *******/

/* A cell of a list of hosts  */
struct host {
    struct sockaddr* addr;   // Net infos (addr, port, ...)
    struct host* next;   // Next seeder of the list
};


struct chunk {
    char hash[SHA256_HASH_SIZE]; // Hash of the chunk
    int status;                  // ready/busy/empty
    int index;                   // Position into the file
    struct host* seeders;       // Pointers to the seeders 
};


struct c_seed {
    char hash[SHA256_HASH_SIZE]; // Hash of the file
    char filename[256];          // Path to the file
    struct chunk* chunks;        // List of the chunks
    struct host* seeders;    // List of seeders sharing the file
    struct c_seed* next;         // Next hash of the list
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

#endif
