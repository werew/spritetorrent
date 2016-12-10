#ifndef _SHA256_H_
#define _SHA256_H_

#include <stdint.h>

/* Size of an hash in bytes */
#define SHA256_HASH_SIZE 32

/* SHA-256 Operations (on 32 bits words) */
// TODO move to sha256.c (no need to have them here)
#define CH(x,y,z)  ( ((x) & (y)) ^ ((~(x)) & (z) ) )
#define MAJ(x,y,z) ( ((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)) )
#define ROTL(x,n)  ( ((x) << (n)) | ((x) >> (32-(n))) )
#define ROTR(x,n)  ( ((x) >> (n)) | ((x) << (32-(n))) )
#define SIG0(x)   ( ROTR(x,2) ^ ROTR(x,13) ^ ROTR(x,22) )
#define SIG1(x)   ( ROTR(x,6) ^ ROTR(x,11) ^ ROTR(x,25) )
#define SSIG0(x)  ( ROTR(x,7) ^ ROTR(x,18) ^ ((x) >> 3) )
#define SSIG1(x)  ( ROTR(x,17) ^ ROTR(x,19) ^ ((x) >> 10) )

int sha256(char dest[SHA256_HASH_SIZE], const char* filename, long offset, ssize_t size);
void sha256_proc(void* chk, uint32_t* ph);
void string_to_sha256(unsigned char* dest, const char* string);
void sha256_to_string(char* dest, const char* hash);

#endif
