#ifndef _TRACKER_H_
#define _TRACKER_H_

#include "sha256.h"


struct request {
    struct msg* msg;            // The body of the request
    time_t timestamp;           // When was it last done
    int nb_deliv;               // Number of times it was sent
    struct request* next;       
};


#define SIZE_HTABLE 100
typedef struct ctask {
    int sockfd;                 // Socket from where to listen
    time_t timeout;             // Keep alive frequency
    struct request* req_poll;   // All the running requests
    void* htable[SIZE_HTABLE];  // Hash table
} *st_ctask;

    
/******* Seeds, seeders and chunks *******/

/* A cell of a list of seeders */
struct c_seeder {
    struct sockaddr* addr; // Net infos (addr, port, ...)
    struct seeder* next;   // Next seeder of the list
};


struct chunk {
    char hash[SHA256_HASH_SIZE]; // Hash of the chunk
    int status;                  // ready/busy/empty
    int index;                   // Position into the file
    struct seeder* seeders;       // Pointers to the seeders 
};


struct c_seed {
    char hash[SHA256_HASH_SIZE]; // Hash of the file
    char filename[256];          // Path to the file
    struct chunk* chunks;        // List of the chunks
    struct c_seeder* seeders;    // List of seeders sharing the file
    struct c_seed* next;         // Next hash of the list
};


st_ctask st_create_ctask(uint16_t port, time_t timeout);

#endif
