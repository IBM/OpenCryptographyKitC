/* crypto/aes/aes_gcm.h */
/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/
/* GCM table driven acceleration: Aaron Cristensen November 2007. */


#ifndef HEADER_AES_GCM_H
#define HEADER_AES_GCM_H

#ifdef __cplusplus
extern "C" {
#endif

#define FAILURE 0
#define OK 1

#define AES_GCM_CTRL_SET_ACCEL 0
#define AES_GCM_CTRL_GET_ACCEL 1
#define AES_GCM_CTRL_TLS13 2

#define IVBLEN 16 /* Length of the fixed internal IV buffer */

#include "openssl/modes.h"
/*! @brief The structure of the AES_GCM context */
typedef struct AES_GCM_struct {
  EVP_CIPHER_CTX *ctx;        /*!< OpenSSL's underlying GCM struct */
  EVP_CIPHER_CTX *IVctx;      /*!< Cipher context for IV gen */
  unsigned char IVcounter[8]; /*!< 64 bit counter for IV gen */
  unsigned long count;        /*!< Number if iterations through IV gen */
  unsigned char mask[8];      /*!< Data used to obfuscate the counter */
  int iv_initialized;         /*!< Flag to indicate that the IV was initialized */
  unsigned char key[32];      /*!< Temporary storage for the key */
  unsigned int klen;          /*!< Key length */
  unsigned char *iv;          /*!< Temporary storage for the IV if > 16 bytes */
  unsigned char ivbuf[IVBLEN];    /*!< fixed buffer for sane IV's */
  unsigned int ivlen;
  const EVP_CIPHER *cipher;   /*!< cipher used */
  unsigned int init;          /*!< Initialized already */
  unsigned int enc;           /*!< 0 = decrypt */
  unsigned int flags;
} AES_GCM_CTX_t;


typedef struct AES_GCM_CTX_t AES_GCM_CTX;

int AES_GCM_GenerateIV(AES_GCM_CTX *gcm_ctx,unsigned char out[8]);
int AES_GCM_GenerateIV_NIST(AES_GCM_CTX *gcm_ctx,int ivlen,unsigned char *iv);
AES_GCM_CTX *AES_GCM_CTX_new();
void AES_GCM_CTX_free(AES_GCM_CTX *ctx);
int AES_GCM_CTX_ctrl(AES_GCM_CTX *ain, int mode, int accel, void *ptr);

int AES_GCM_Init(ICClib *pcb,AES_GCM_CTX *ain,
		 unsigned char *iv,unsigned long ivlen,
		 unsigned char *key, unsigned int klen );

int AES_GCM_EncryptUpdate(AES_GCM_CTX *ain,
			  unsigned char *aad,unsigned long aadlen,
			  unsigned char *data,unsigned long datalen,
			  unsigned char *out, unsigned long *outlen);

int AES_GCM_DecryptUpdate(AES_GCM_CTX *ain,
			  unsigned char *aad,unsigned long aadlen,
			  unsigned char *data,unsigned long datalen,
			  unsigned char *out, unsigned long *outlen);

int AES_GCM_EncryptFinal(AES_GCM_CTX *ain,
			 unsigned char *out, unsigned long *outlen,
			 unsigned char *hash);

int AES_GCM_DecryptFinal(AES_GCM_CTX *ain,
			 unsigned char *out, unsigned long *outlen,
			 unsigned char *hash,unsigned int hlen);

#ifdef __cplusplus
}
#endif
                                                                           
#endif
