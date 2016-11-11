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
