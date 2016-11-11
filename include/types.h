#ifndef _TYPES_H_
#define _TYPES_H_

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
    char[SHA256_HASH_SIZE] hash; // Hash of the file
};


/*******     Messages     ********/

/* Types */
/* client <--> client */
#define PUT_C           100
#define REP_PUT         101
#define LIST            102
#define REP_LIST        103
/* client <--> tracker */
#define PUT_T           110 
#define ACK_PUT         111
#define GET             112
#define ACK_GET         113
#define KEEP_ALIVE      114
#define ACK_KEEP_ALIVE  115
/* debug */
#define PRINT           150

typedef char msg_type;

/**
 * Only the fields type and data
 * can accessed direclty by the user.
 * To set and get the length the
 * function msgset_length and
 * msgget_length must be used.
 */
struct msg {
    msg_type type;
    unsigned char _len0;
    unsigned char _len1;
    char data[];
};


struct msg* create_msg(uint16_t max_length);
void drop_msg(struct msg* msg);

void msgset_length(struct msg* msg, uint16_t length);
uint16_t msgget_length(struct msg* msg);





#endif
