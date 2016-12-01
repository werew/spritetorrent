#ifndef _TRACKER_H_
#define _TRACKER_H_

#include "sha256.h"

/*******    Tracker Task   ********/

#define SIZE_HTABLE 100
typedef struct ttask {
    int sockfd;                 // Socket from where to listen
    unsigned int hosts_count;   // Number of connected hosts
    unsigned int hosts_max;     // Max number of hosts allowed
    time_t timeout;             // Probe frequency
    time_t last_probe;          // Timestamp of the last probe
    void* htable[SIZE_HTABLE];  // Hash table
} *st_ttask;
    
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


st_ttask st_create_ttask
(uint16_t port, unsigned int max_hosts, time_t timeout);
struct seeder* create_seeder(struct sockaddr* addr);
void drop_seeder(struct seeder* s);



#endif
