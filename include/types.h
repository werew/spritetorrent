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


struct seed {
    struct seeder seeder;
    struct seeder* next;
};


typedef msg_type char;


// I think this struct needs to be packed
// or we should use bitfields: http://www.catb.org/esr/structure-packing/
struct msg {
    msg_type type;
    uint16_t length;
    void data[];
};
    









#endif
