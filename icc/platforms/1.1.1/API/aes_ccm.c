/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/


/* Note !
   AES-CCM is defined as a one-shot encrypt/mac operation
   hence there's no AES_CCM_CTX, no Init/Update/Final
   It'll also eat a LOT of RAM for long messages.
*/
#ifndef AES_DEBUG
# ifndef NDEBUG
#  define NDEBUG
# endif
#endif
#include <string.h>
#include "osslver.h"
#include "openssl/evp.h"
#include "icclib.h"

int AES_CCM_common(ICClib *pcb,unsigned char *iv,unsigned int ivlen,
			unsigned char *key,unsigned int keylen,
			unsigned char *aad, unsigned long aadlen,
			unsigned char *data,unsigned long datalen,
			unsigned char *out, unsigned long *outlen,
		        unsigned int taglen,int enc
			)
{
  int rv = 1;
  EVP_CIPHER_CTX *ctx = NULL;
  const EVP_CIPHER *cip = NULL;
  int chunklen = 0;
  unsigned char tag[16];
  int tmplen = 0;
  
  /* AES-CCM tag length must be 4..16 bytes and even, per SP800-38C */
  if (taglen > sizeof(tag)) {
    return -1;
  }
  
  memset(tag,0,sizeof(tag));

  ctx = EVP_CIPHER_CTX_new();
  if (ctx == NULL) {
    return -1;
  }
  switch(keylen) {
  case 16:
    cip = EVP_get_cipherbyname("aes-128-ccm");
    break;
  case 24:
    cip = EVP_get_cipherbyname("aes-192-ccm");
    break;
  case 32:
    cip = EVP_get_cipherbyname("aes-256-ccm");
    break;
  default:
    rv = -1;
    break;
  }
  if(!enc) {
    if(datalen >= taglen) {
      memcpy(tag,data + datalen - taglen,taglen);
      datalen -= taglen;
    }
  }
  *outlen = 0;

  if(1 == rv) {
    EVP_CIPHER_CTX_set_flags(ctx, EVP_CIPHER_CTX_FLAG_WRAP_ALLOW);
  }

  if(1 == rv) {
    rv =  EVP_CipherInit_ex(ctx,cip,NULL,NULL,NULL,enc);
  }

  if(1 == rv) {
    rv = EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_AEAD_SET_IVLEN,ivlen,0);
  }

  if(1 == rv) {;
    rv = EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_AEAD_SET_TAG,taglen,(1 == enc) ? NULL: tag);
  }

  if(1 == rv) {
    rv = EVP_CIPHER_CTX_set_key_length(ctx,keylen);
  }

  if(1 == rv) {
    rv = EVP_CipherInit_ex(ctx, NULL, NULL, key, iv, -1);
  }

  if( 1 == rv) {
    tmplen = 0;
    rv = EVP_CipherUpdate(ctx, NULL, &tmplen, NULL,datalen);  
  }    

  if( (1 == rv) && (NULL != aad) && (aadlen > 0) ) {
    rv = EVP_CipherUpdate(ctx,NULL,&chunklen,aad,aadlen);
   
  }

  /* CCM mode is a streaming mode, but the OpenSSL implementation requires that the padding be disabled.
  This is because CCM mode does not use padding in the same way that other modes do.
  The following code is commented out because it is not necessary to disable padding in this case.
  if( 1 == rv ) {
  rv = EVP_CIPHER_CTX_set_padding(ctx, 0); 
  }
  */

  if( 1 == rv ) {
    chunklen = 0;
    rv = EVP_CipherUpdate(ctx,out+(*outlen),&chunklen,data,datalen);
    *outlen += chunklen;
  }

  if(enc && (1 == rv)) {
    rv = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG,taglen, tag);
  }

  if(1 == rv) {
    if(enc) {
      memcpy(out + (*outlen),tag,taglen);
      *outlen += taglen;
    }
  }
  if(1 == rv && pcb && pcb->callback) {
    int nid;
    nid = EVP_CIPHER_type(cip);
    pcb->callback("ICC_CCM_common",nid,1);
  }
  EVP_CIPHER_CTX_free(ctx); 

  return rv;
}

int AES_CCM_Encrypt(ICClib *pcb,unsigned char *iv,unsigned int ivlen,
		    unsigned char *key,unsigned int keylen,
		    unsigned char *aad, unsigned long aadlen,
		    unsigned char *data,unsigned long datalen,
		    unsigned char *out, unsigned long *outlen,
		    unsigned int taglen
		    )
{
  int rv = 1;
  rv = AES_CCM_common(pcb,iv,ivlen,
		      key,keylen,
		      aad,aadlen,
		      data,datalen,
		      out,outlen,
		      taglen,1);
   return rv;
}

int AES_CCM_Decrypt(ICClib *pcb,unsigned char *iv,unsigned int ivlen,
		    unsigned char *key,unsigned int keylen,
		    unsigned char *aad, unsigned long aadlen,
		    unsigned char *data,unsigned long datalen,
		    unsigned char *out, unsigned long *outlen,
		    unsigned int taglen
		    )
{
  int rv = 1;
  rv = AES_CCM_common(pcb,iv,ivlen,
		      key,keylen,
		      aad,aadlen,
		      data,datalen,
		      out,outlen,
		      taglen,0);
  return rv;
}


