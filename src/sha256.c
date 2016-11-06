#include <stdio.h>
#include <stdint.h>
#include <string.h>
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

void sha256(void* data, size_t s){


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
    char chunk[64];

    
    size_t s_left = s;
    while (s_left >= 64) {
        memcpy(chunk, data, 64);
        sha256_proc(chunk, h);
        s_left -= 64;
    };

    /* Add padding to the last chunk */
    memset(chunk, 0, 64);
    *(size_t*) (chunk + 64 - sizeof(size_t)) = s;
    memcpy(chunk, data, s_left);
    sha256_proc(chunk,h);

    // At this point the digest is ready
   
};


void sha256_proc(void* chk, uint32_t* ph){

    unsigned int i;
    uint32_t w[64];
    uint32_t a,b,c,d,e,f,g,h;
    uint32_t* m = (uint32_t*) chk;


    /* Init variables with previous hash values */    
    a = ph[0]; b = ph[1]; c = ph[2]; d = ph[3];
    e = ph[4]; f = ph[5]; g = ph[6]; h = ph[7];


    /* Prepare the message schedule */
    for (i=0; i<16; i++) w[i] = m[i];
    for (; i<64; i++) w[i] = SSIG1(w[i-2]) + w[i-7] + SSIG0(w[i-15]) + w[i-16];


    /* Compression loop */
    for (i=0; i<63; i++){
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
    ph[0] = a + ph[0]; ph[1] = b + ph[1];
    ph[2] = c + ph[2]; ph[3] = d + ph[3];
    ph[4] = e + ph[4]; ph[5] = f + ph[5];
    ph[6] = g + ph[6]; ph[7] = h + ph[7];
};


    



