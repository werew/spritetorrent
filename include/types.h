#ifndef _TYPES_H_
#define _TYPES_H_



/* TODO need to define the correct types
   this will depend on the implementation
struct seeder {
    addr;
    port;
    lastseen;
};
*/

/*
struct seed {
    struct seeder seeder;
    struct seeder* next;
};
*/

typedef char msg_type;

/**
 * This struct is aligned on a 1-byte
 * boundary so that it can be casted
 * directly at the beginning of the 
 * message
 */
#pragma pack(1)
struct msg {
    msg_type type;
    uint16_t length;
    char data[];
};
    









#endif
