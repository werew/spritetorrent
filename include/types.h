#ifndef _TYPES_H_
#define _TYPES_H_



/***** Types ******/

/* data */
#define FILE_HASH        50
#define CHUNK_HASH       51
#define CLIENT           55
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

// Size of the header of a tlv (type + length)
#define SIZE_HEADER_TLV 3
// Max size of the length of a struct tlv
#define MAX_LEN_TLV 1028



/**
 * Only the fields type and data
 * can accessed direclty by the user.
 * To set and get the length the
 * function msgset_length and
 * msgget_length must be used.
 */
struct tlv {
    char type;
    unsigned char _len0;
    unsigned char _len1;
    char data[];
};


/**
 * This struct is used as
 * return value of accept_msg
 */
struct msg {
    struct tlv* tlv;            // received msg
    ssize_t size;               // size of the data read
    socklen_t addrlen;          // size of the addr
    struct sockaddr_storage addr;   // addr of the sender
};



struct tlv* create_tlv(uint16_t max_length);
void drop_tlv(struct tlv* tlv);

struct msg* create_msg(uint16_t max_length);
void drop_msg(struct msg* m);

void tlvset_length(struct tlv* tlv, uint16_t length);
uint16_t tlvget_length(const struct tlv* tlv);


#endif
