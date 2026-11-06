/* crypto/aes/aes_ccm.h */
/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

#ifndef HEADER_AES_CCM_H
#define HEADER_AES_CCM_H


#ifdef __cplusplus
extern "C" {
#endif

int AES_CCM_Encrypt(ICClib *pcb,unsigned char *iv,unsigned int ivlen,
                    unsigned char *key,unsigned int keylen,
                    unsigned char *aad, unsigned long aadlen,
                    unsigned char *data,unsigned long datalen,
                    unsigned char *out, unsigned long *outlen,
                    unsigned int taglen
		      );


int AES_CCM_Decrypt(ICClib *pcb,unsigned char *iv,unsigned int ivlen,
                    unsigned char *key,unsigned int keylen,
                    unsigned char *aad, unsigned long aadlen,
                    unsigned char *data,unsigned long datalen,
                    unsigned char *out, unsigned long *outlen,
                    unsigned int taglen
                    );


#ifdef __cplusplus
}
#endif
                                                                           
#endif
