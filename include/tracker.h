#ifndef _TRACKER_H_
#define _TRACKER_H_

#include "sha256.h"

/******* Seeds and seeders ********/

/* A cell of a list of seeders */
struct seeder {
    struct sockaddr* addr; // Net infos (addr, port, ...)
    time_t lastseen;       // Last contact 
    struct seeder* next;   // Next seeder of the list
};


/* Associates an hash to a list of seeders */
struct seed {
    struct seeder* seeders;      // List of seeders sharing the file
    char hash[SHA256_HASH_SIZE]; // Hash of the file
    struct seed* next;           // Next hash of the list
};

#define SIZE_HTABLE 100

struct seeder* create_seeder(struct sockaddr* addr);
void drop_seeder(struct seeder* s);




#endif
