#ifndef _SHA256_H_
#define _SHA256_H_

/* Size of an hash in bytes */
#define SHA256_HASH_SIZE 64

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

void sha256(void* data, size_t s);
void sha256_proc(void* chk, uint32_t* ph);

#endif
