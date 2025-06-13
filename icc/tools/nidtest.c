/*
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*/

/*
    Small test to weed out some quirks in the name/nid caches    
*/

#include <stdio.h>
#include <stdlib.h>
#include "icc.h"
const char *alglist[] = {
    "AES-128-CFB1",
    "AES-128-CFB8",
    "AES-128-CFB",
    NULL
};

int main(int argc, char *argv[])
{
    ICC_CTX *ctx = NULL;
    int nid,nidc;
    const ICC_EVP_CIPHER *cip = NULL;
    const char *alg = NULL;
    const char **ptr = NULL;
    const char *name = NULL;
    ICC_STATUS status;
    
    ctx = ICC_Init(&status,"../package");
    if(NULL == ctx) {
        printf("Could not initialize ICC\n");
        exit(1);
    } else {
        ICC_Attach(ctx,&status);
        for(ptr = alglist; NULL != *ptr; ptr++)
        {
            cip = ICC_EVP_get_cipherbyname(ctx,*ptr);
            nidc = ICC_EVP_CIPHER_type(ctx,cip);
            nid = ICC_OBJ_txt2nid(ctx,(char *)*ptr);
            name = ICC_OBJ_nid2sn(ctx,nid);
            printf("alg = %s, nid = %d, nidc = %d, name for nid = %s\n",*ptr,nid,nidc,name);
        }

        ICC_Cleanup(ctx,&status);
    }
}
