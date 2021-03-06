#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <endian.h>
#include <arpa/inet.h>
#include "sha256.h"


static const uint32_t K[] = {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
};


void sha256_to_string(char* dest, const char* hash){

    size_t count = 0;
    for(count = 0; count < 32; count++) {
        sprintf(dest, "%02hhx", hash[count]);
        dest += 2;
    }
}



void string_to_sha256(unsigned char* dest, const char* hexstring){

    size_t count = 0;
    for(count = 0; count < 64; count++) {
        sscanf(hexstring, "%02hhx", &dest[count]);
        hexstring += 2;
    }
}


int fsha256(char dest[SHA256_HASH_SIZE], FILE* f,
            long offset, ssize_t size){

    if (fseek(f, offset, SEEK_SET) == -1) return -1;

    
    uint32_t h[8] = { 
        0x6a09e667,
        0xbb67ae85,
        0x3c6ef372,
        0xa54ff53a,
        0x510e527f,
        0x9b05688c,
        0x1f83d9ab,
        0x5be0cd19
    };

    /* 512-bits chunk */
    unsigned char chunk[64];

   
    /* Compute hash for all the full 512-bits chunks */ 
    size_t size_left = size;
    size_t size_chunk = 0;
    size_t size_read = 0;
    while (size == -1 || size_left >= 64) {

        size_chunk = fread(chunk, 1, 64, f);

        size_read += size_chunk;

        if (size_chunk < 64){
            if (feof(f) == 0) return -1;
            size_left = 0;
            break;
        }
        
        sha256_proc(chunk, h);
        size_left -= 64;
    }

    // Read remaining data
    if (size_left != 0){
        size_chunk = fread(chunk, 1, size_left, f);
        size_read += size_chunk;
        if (size_chunk < size_left){
            if (feof(f) == 0) return -1;
            size_left = 0;
        }
    }

    /* Padding */

    /* Append 1-bit */ 
    chunk[size_chunk++] = 0x80;

    /* Need at least 8 bytes of space */
    if (size_chunk > 56){
        /* Size is too big */
        while (size_chunk < 64) chunk[size_chunk++] = 0x00;
        sha256_proc(chunk,h);
        
        /* Use a new empty chunk */ 
        size_chunk = 0;
    } 

    while(size_chunk < 56) chunk[size_chunk++] = 0x00;

    /* Append 8-bytes full data size in bits */
    *(uint64_t*) (chunk + 56) = htobe64((uint64_t) size_read * 8);

    sha256_proc(chunk,h);

    int i;
    for (i=0; i<8; i++) h[i] = htonl(h[i]);
      
    memcpy(dest, h, SHA256_HASH_SIZE);

    return 0;
}

/**
 * Forge the hash of a file.
 * @param dest Destination buffer
 * @param filename File to read
 * @param offset Offset from where to read
 * @param size How much data (-1 means all)
 * @return 0 in case of success, -1 otherwise
 */
int sha256(char dest[SHA256_HASH_SIZE], 
const char* filename, long offset, ssize_t size){

    // Open and set the good position on the file
    FILE* f = fopen(filename, "r");
    if (f == NULL) return -1;

    int ret = fsha256(dest,f,offset,size);
    
    fclose(f);

    return ret;

}


void sha256_proc(void* chk, uint32_t* ph){

    unsigned int i;
    uint32_t w[64];
    uint32_t a,b,c,d,e,f,g,h;
    uint32_t* m = (uint32_t*) chk;


    /* Init variables with previous hash values */    
    a = ph[0]; b = ph[1]; 
    c = ph[2]; d = ph[3];
    e = ph[4]; f = ph[5]; 
    g = ph[6]; h = ph[7];


    /* Prepare the message schedule */
    for (i=0; i<16; i++) 
        w[i] = htobe32(m[i]);

    for (i=16; i<64; i++) 
        w[i] = SSIG1(w[i-2]) + w[i-7] + SSIG0(w[i-15]) + w[i-16];

    /* Compression loop */
    for (i=0; i<64; i++){
        uint32_t t1 =  h + SIG1(e) + CH(e,f,g) + K[i] + w[i];
        uint32_t t2 =  SIG0(a) + MAJ(a,b,c);

        h = g; 
        g = f; 
        f = e;
        e = d + t1;
        d = c; 
        c = b; 
        b = a; 
        a = t1 + t2;
    }

    /* Compute next hash values */
    ph[0] += a; ph[1] += b;
    ph[2] += c; ph[3] += d;
    ph[4] += e; ph[5] += f;
    ph[6] += g; ph[7] += h;

}


    



