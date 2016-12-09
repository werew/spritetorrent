#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "sha256.h"

void printhash(char* hash){
    int i;
    for (i=0;i<SHA256_HASH_SIZE;i++){
        printf("%x",hash[i]);
    }
    printf("\n");
}

