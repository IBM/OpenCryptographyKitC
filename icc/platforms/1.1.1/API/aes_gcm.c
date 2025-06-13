/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/


#ifndef AES_DEBUG
# ifndef NDEBUG
#  define NDEBUG
# endif
#endif



#include <string.h>


#include "openssl/evp.h" 
#include "openssl/aes.h"
#include "openssl/rand.h"

#include "icclib.h"
/*
#include "aes_gcm.h"
*/

/* Note we need to look these up
   because the accelerated and non-acclerated objects are different
   and this has the capability probes done 
   */
static const EVP_CIPHER *gcm_128 = NULL;
static const EVP_CIPHER *gcm_192 = NULL;
static const EVP_CIPHER *gcm_256 = NULL;

#if __BIG_ENDIAN__
# define htonll(x) (x)
# define ntohll(x) (x)
#else
# define htonll(x) ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
# define ntohll(x) ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

/*
\debug Define: Uncomment to enable debugging of the AES-GCM code
#define AES_GCM_DEBUG
*/

/*! @brief increment a big endian counter
  @param counter pointer to the base of the counter block
  @param n number of bytes in the counter
  @note: the IV/counter CTR mode is big-endian.  The rest of the AES code
  is endian-neutral.
  increment counter (32 bit unsigned int
  rightmost 32 bits of a 128 bit block ) by 1 
*/
static void AES_gcm_inc(unsigned char *counter, int n) {
  int i;

  for(i = (n - 1); i >= 0; i--) {
    counter[i]++;
    if(0 != counter[i]) break;
  }
}

/** 
    @brief IV generator for AES_GCM IV's. This variant is complaint with the FIPS 140-2 IG
    @param gcm_ctx An AES_GCM context
    @param ivlen length of the IV
    @param iv a byte buffer which should contain reasonably non-repeating data 
    ideally at least 2^32 between repeats, and not predictable in the last 8 bytes.
    @return 1 if O.K. 0 if ivlen is less than 8 bytes
    @note Modified to meet the requirements of the FIPS 140-2 IG -
    - If the IV is 12 bytes or larger the tail 8 bytes are a randomly initialized and 
    masked counter which is regenerated every 2^32 bytes. 
    The 4 bytes above the 8 byte counter are initialized to "IBM " on the first call, 
    this should be overwritten by the application with an application specific tag after the first call to 
    AES_GCM_GenerateIV_NIST() returns
    - Note after 2^32 iterations the GCM context must be recreated with 
      new keys. The IV generation shuts down at 2^32 to enforce this.
 */

int AES_GCM_GenerateIV_NIST(AES_GCM_CTX *gcm_ctx, int ivlen,
                            unsigned char *iv) {
  int rv = 1; /* O.K. */
  AES_GCM_CTX_t *ctx = NULL;
  int i = 0;

  if (ivlen >= 12) {
    ctx = (AES_GCM_CTX_t *)gcm_ctx;
    /* Initialize from scratch the first time,
     */
    if (0 == ctx->count) {
      if (ctx->iv_initialized) {
        rv = 0; /* We've wrapped around, signal a failure */
      }
      /* Initialize the counter to some random starting value
         Note that data from RAND_bytes() comes from an SP800-90 PRNG.
      */
      RAND_bytes(ctx->IVcounter, sizeof(ctx->IVcounter));
      /* get a new seed from the TRNG for the mask
         This *is* weak, but slightly better than just a counter
      */
      RAND_bytes(ctx->mask, sizeof(ctx->mask));
    }
    if (!ctx->iv_initialized) {
      if (ivlen >= 12) {
        iv[ivlen - 12 + 0] = 'I';
        iv[ivlen - 12 + 1] = 'B';
        iv[ivlen - 12 + 2] = 'M';
        iv[ivlen - 12 + 3] = ' ';
      }
      ctx->iv_initialized = 1;
    }

    /* Increment the counter, 8 bytes of it in this case */
    AES_gcm_inc(ctx->IVcounter, 8);
    /* xor our randomly initialized counter with the random mask */
    for (i = 0; i < sizeof(ctx->IVcounter); i++) {
      iv[ivlen - sizeof(ctx->IVcounter) + i] = ctx->IVcounter[i] ^ ctx->mask[i];
    }
    /* Increment the number of cycles, and ensure we'll restart again at 2^32 */
    ctx->count = (ctx->count + 1) & 0xffffffff;
  } else {
    rv = 0; /* IV field is too short for us to do anything sensible with */
  }
  return rv;
}
/** 
    @brief IV generator for AES_GCM IV's
    @param gcm_ctx An AES_GCM context
    @param out 8 byte IV buffer
    @return 1 if O.K. 0 if the RNG found two adjacent repeated values
    @note
    - There are probably a lot of ways to do this, but the simplest to ensure: 
    - a) non-repitition over 2^32 calls
    - b) unpredictability of the data stream
    seems to be to encrypt a counter with (any) cipher with an 8 byte block size and
    a randomly generated key. 
    - To guarantee no repeats within our 8 byte output
    we need a cipher with an 8 byte blocksize so AES isn't usable.
    The 8 byte cipher blocksize guarantees a unqiue 1:1 transform
    between our input (counter) and the output and no repeats for the block size.
    - To avoid attacks, we generate a new random key every 2^32 iterations.
    - The choice of cipher itself shouldn't matter as we are only relying on this one 
    property (key specific 1:1 mapping of input to output over 64 bits),
    not it's strength, and over one block, all ciphers will be equally strong or weak
    WRT this. We chose to use blowfish because of it's speed.
    DES, 3DES or CAST could also be used, but offer no functional advantages
    The weak keys in DES cause problems they require reducing the key space  
    - We believe blowfish is usable here, even in FIPS approved code, because we 
    are NOT using it as an encryption function, simply as a way to guarantee 
    the unique 1:1 input/output mapping.
*/
   
int AES_GCM_GenerateIV(AES_GCM_CTX *gcm_ctx,unsigned char out[8]) 
{
  int rv = 1;
  AES_GCM_CTX_t *ctx = NULL;
  const EVP_CIPHER *cipher = NULL; 
  unsigned char IVkeybuf[16];  /*!< Transient encrypt key */
  int outl = 0;

  ctx = (AES_GCM_CTX_t *)gcm_ctx;
  
  /* Obtain the block cipher */
  if(NULL == cipher) {
    cipher = EVP_get_cipherbyname("blowfish");
  }
  /* Have we used this for IV generation before ? */
  if(NULL == ctx->IVctx) {
    /* Create an encryption context for this IV generator */
    ctx->IVctx = EVP_CIPHER_CTX_new();
  }
  /* Initialize from scratch the first time, 
     and every 2^32'th time round the block 
  */
  if(0 == ctx->count) {
    /* Initialize the counter to some random starting value */    
    RAND_bytes(ctx->IVcounter,8);
    /* get a new seed from the TRNG for the transform key*/
    RAND_bytes(IVkeybuf,16);
    EVP_CIPHER_CTX_cleanup(ctx->IVctx);
    EVP_CIPHER_CTX_set_padding(ctx->IVctx,0);
    EVP_EncryptInit(ctx->IVctx,cipher,IVkeybuf,ctx->IVcounter);
  }
  /* Here we go, encrypt our randomly initialized counter with a random key */
  EVP_EncryptUpdate(ctx->IVctx,out,&outl,ctx->IVcounter,8);
  /* Increment the counter, 8 bytes of it in this case */
  AES_gcm_inc(ctx->IVcounter,8);
  /* Increment the number of cycles, and ensure we'll restart again at 2^32 */
  ctx->count = (ctx->count +1) & 0xffffffff;  
  return rv;
}





int AES_GCM_CTX_ctrl(AES_GCM_CTX *ain, int mode, int accel, void *ptr)
{
  int rv = 1;
  AES_GCM_CTX_t *a = (AES_GCM_CTX_t *)ain;

  switch(mode) {
  case AES_GCM_CTRL_SET_ACCEL:
    break;
  case AES_GCM_CTRL_GET_ACCEL:
    *(int *)ptr = 1; /* Stuck at 4bit tables, implemented in assembler */
    break;
  case AES_GCM_CTRL_TLS12: /* TLS 1.2 IV rollover */
     a->flags |= AES_GCM_CTRL_TLS12;
     break;
  case AES_GCM_CTRL_TLS13: /* TLS 1.3 IV rollover */
    a->flags |= AES_GCM_CTRL_TLS13;
    break;  
  default:
    rv = 0;
    break;
  }
  return rv;
}

AES_GCM_CTX *AES_GCM_CTX_new()
{
  AES_GCM_CTX_t *ctx = NULL;
  ctx = OPENSSL_malloc(sizeof(AES_GCM_CTX_t));
  if(NULL != ctx) {
    memset(ctx,0,sizeof(AES_GCM_CTX_t));
  }
  return (AES_GCM_CTX *)ctx;
}

void AES_GCM_CTX_free(AES_GCM_CTX *ctx)
{
  AES_GCM_CTX_t *a = (AES_GCM_CTX_t *)ctx;
  if((a->ivlen > IVBLEN) && (NULL != a->iv)) {
    OPENSSL_free(a->iv);
  }
  if(NULL != a->ctx) {
    EVP_CIPHER_CTX_cleanup(a->ctx);
    EVP_CIPHER_CTX_free(a->ctx);
 
  }
  if(NULL != a->IVctx) {
    EVP_CIPHER_CTX_cleanup(a->IVctx);
    EVP_CIPHER_CTX_free(a->IVctx);
  }
  memset(a,0,sizeof(AES_GCM_CTX_t));
  OPENSSL_free(ctx);
}
static void XOR(unsigned char *a,unsigned char *b,int l)
{
  int i = 0;
    for(i = 0; i < l; i++) {
        a[i] ^= b[i];
    }
} 

/** @brief Calculate the GHASH of an arbitrary data stream
    @param gcm_ctx an AES GSM context
    @param H the hash key
    @param Hash is the input/output, (AES_BLOCK_SIZE long)
    @param X the input data
    @param Xlen is the length of the input data 0 <= X <= 2^64 BITS
    @return 1 if O.K. 0 on error (X is too long)
    @note The last block of X will be 0 padded - so no partial blocks
    unless you want this.
    @note Y should be initialized to 0 if this is the first pass
    otherwise, Y is the output from the previous invocation and
    the GHASH can be chained.   
*/
void GHASH(AES_GCM_CTX *gcm_ctx,unsigned char *H, unsigned char *Hash, unsigned char *X,unsigned long Xlen)
{

}

/** @brief
    Initialize an AES GCM operation, provide the initialization data and
    key
    @param pcb The internal ICC_CTX
    @param ain an AES_GCM_CTX context
    @param iv The IV, can be 1-2^56 bytes long, 12 bytes is best
    @param ivlen the length of the IV
    @param key an aes key 16,24 or 32 bytes long
    @param klen the length of the aes key
    @return 1 if O.K., 0 otherwise
    @note Checks for IV change on rollovers (FIPS). Can be called multiple times to set 'iv' and 'key' separately
*/

int AES_GCM_Init(ICClib *pcb, AES_GCM_CTX *ain, unsigned char *iv, unsigned long ivlen,
                 unsigned char *key, unsigned int klen ) {

  AES_GCM_CTX_t *a = (AES_GCM_CTX_t *)ain;
  int rv = 1;

  if (iv) {
     if ((ivlen > IVBLEN) && (ivlen > a->ivlen)) {
        if (NULL != a->iv) {
           if (a->ivlen > IVBLEN) {
              OPENSSL_free(a->iv);
           }
           a->iv = NULL;
        }
     }
     if (NULL == a->iv) {
        if (ivlen > IVBLEN) {
           a->iv = OPENSSL_malloc(ivlen);
        }
        else {
           a->iv = &(a->ivbuf[0]);
        }
        a->ivlen = ivlen;
     }

     if ((NULL != a->ctx) && (NULL == key)) {

        /* This context has been used before and it's an IV rollover with the same mask, ensure that the IV has changed accordingly */
        if (a->flags & AES_GCM_CTRL_TLS13) {
           /* Specific check for TLS V1.3, masked iv is incremented by 1 */
           if (ivlen == a->ivlen && ivlen == 12) {
              unsigned char tmp[8];
              const unsigned int countLen = sizeof(tmp);
              if (0 != memcmp(a->iv, iv, 4)) {
                 /* the first 4 bytes are not part of counter so must not change */
                 rv = -1;
              }
              memcpy(tmp, a->mask, countLen);
              XOR(tmp, &(iv[4]), countLen);
              if (0 != memcmp(tmp, a->IVcounter, countLen)) {
                 rv = -1;
              }
              AES_gcm_inc(a->IVcounter, countLen);
           }
           else {
              /* the iv length is not supposed to change! */
              rv = -1;
           }
        }
        else if (a->flags & AES_GCM_CTRL_TLS12) {
           /* Specific check for TLS V1.2, iv is incremented by 1 */
           if (0 != memcmp(a->iv, iv, 4)) {
              /* the first 4 bytes are not part of counter so must not change */
              rv = -1;
           }
           if (0 != memcmp(&(iv[4]), a->IVcounter, 8)) {
              rv = -1;
           }
           AES_gcm_inc(a->IVcounter, 8);
        }
        else if (a->ivlen != ivlen || 0 == memcmp(a->iv, iv, ivlen)) {
           /* default to IV has changed */
           rv = -1;
        }
     }
     memcpy(a->iv, iv, ivlen);
     a->ivlen = ivlen;

     if (!a->ctx) {
        if (a->flags & AES_GCM_CTRL_TLS13) {
           /* Save the IV for later */
           memcpy(a->mask, iv + 4, 8);
        }
        else if (a->flags & AES_GCM_CTRL_TLS12) {
           /* These stay fixed for the session */
           memcpy(a->mask, iv, 4);
        }
     }
  }

  if (klen > sizeof(a->key)) {
     /* truncate */
     klen = sizeof(a->key);
  }

  if ((NULL != key) && (klen > 0)) {
     a->klen = klen;
     memcpy(a->key, key, klen);
  }
  
  /* we need to have an iv before we create a context for the first time */
  /* if we dont have an iv then caller will call again */
  if (a->iv && !a->ctx) {
     a->ctx = EVP_CIPHER_CTX_new();
     if (!a->ctx) {
        rv = -1;
     }
  }
  if( 1 == rv) {
    switch (a->klen) {
    case 16:
      if (NULL == gcm_128) {
        gcm_128 = EVP_get_cipherbyname("aes-128-gcm");
      }
      a->cipher = gcm_128;
      break;
    case 24:
      if (NULL == gcm_192) {
        gcm_192 = EVP_get_cipherbyname("aes-192-gcm");
      }
      a->cipher = gcm_192;
    break;
    case 32:
      if (NULL == gcm_256) {
        gcm_256 = EVP_get_cipherbyname("aes-256-gcm");
      }
      a->cipher = gcm_256;
      break;
    default:
      break;
    }
    if (NULL == a->cipher) {
      rv = -1;
    }
  }
  /* We already know if it's a decrypt or encrypt
     we do this to allow the iv to be changed without
     performing a new key expansion
  */
  if( 1 == rv) {
    if (a->iv && (1 == a->init)) {
      if (a->enc) {
        EVP_EncryptInit_ex(a->ctx, NULL, NULL, a->key, a->iv);
      } else {
        EVP_DecryptInit_ex(a->ctx, NULL, NULL, a->key, a->iv);
     }
    } else {
      /* EVP_EncryptInit_ex will happen in the AES_GCM_??cryptUpdate */
      a->init = 0;
    }
  }
  if((1 == rv) && pcb && pcb->callback) {
    int nid = 0;
    nid = EVP_CIPHER_type(a->cipher);
    pcb->callback("AES_GCM_Init",nid,1);
  }
  return rv;
}

/** @brief Perform an AES_GCM_CTX "updateEncrypt" operation
    @param ain the (opaque) AES_GCM_CTX context
    @param aad additional authentication data (hashed, not encrypted)
    @param aadlen the length of the aad 0-2^56 bytes long
      @param data data to encrypt
    @param datalen the length of the encrypted data
    @param out the output buffer, should be at least "data" long
    @param outlen a place to store the length of the output data
    @return 1 if O.K., 0 otherwise
    @note aad may only be specified up to the point where input data is being
   supplied we do support streaming aad up until that point.
    - i.e. we allow multiple chunks of aad to be specified provided data is
   NULL until the last aad chunk
    - multiple chunks of data, but aad must be NULL after the first data chunk
    - There's very little difference between encrypt and decrypt in AES_GCM
    so we've rolled the code into one routine with a wrapper to simplify
    maintenance.
*/
int AES_GCM_EncryptUpdate(AES_GCM_CTX *ain, unsigned char *aad,
                          unsigned long aadlen, unsigned char *data,
                          unsigned long datalen, unsigned char *out,
                          unsigned long *outlen) {
  AES_GCM_CTX_t *a = (AES_GCM_CTX_t *)ain;
  int outl = 0;
  int rv = 1;
  if (outlen) {
    *outlen = 0;
  }
  if (0 == a->init) {
    if (NULL == EVP_CIPHER_CTX_cipher(a->ctx)) {
      rv = EVP_EncryptInit_ex(a->ctx, a->cipher, NULL, NULL, NULL);
    }
    EVP_CIPHER_CTX_ctrl(a->ctx, EVP_CTRL_GCM_SET_IVLEN, a->ivlen, NULL);
    rv = EVP_EncryptInit_ex(a->ctx, NULL, NULL, a->key, a->iv);
    a->init = 1;
    a->enc = 1;
  }
  if (1 == rv) {
    if (NULL != aad) {
      rv = EVP_EncryptUpdate(a->ctx, NULL, &outl, aad, aadlen);
    }
    if (NULL != data) {
      rv = EVP_EncryptUpdate(a->ctx, out, &outl, data, datalen);
      if (outlen) {
      *outlen = outl;
    }
  }
  }
  return rv;
  }

  /** @brief Perform an AES_GCM_CTX "updateDecrypt" operation
      @param ain the (opaque) AES_GCM_CTX context
      @param aad additional authentication data (hashed, not encrypted)
      @param aadlen the length of the aad 0-2^56 bytes long
      @param data data to encrypt
      @param datalen the length of the encrypted data
      @param out the output buffer, should be at least "data" long
      @param outlen a place to store the length of the output data
      @return 1 if O.K., 0 otherwise
      @note aad may only be specified up to the point where input data is being
     supplied we do support streaming aad up until that point.
      - i.e. we allow multiple chunks of aad to be specified provided data is
     NULL until the last aad chunk
      - multiple chunks of data, but aad must be NULL after the first chunk
      - There's very little difference between encrypt and decrypt in AES_GCM
      so we've rolled the code into one routine with a wrapper to simplify
      maintenance.
  */
  int AES_GCM_DecryptUpdate(AES_GCM_CTX * ain, unsigned char *aad,
                            unsigned long aadlen, unsigned char *data,
                            unsigned long datalen, unsigned char *out,
                            unsigned long *outlen) {
    int rv = 1;
    AES_GCM_CTX_t *a = (AES_GCM_CTX_t *)ain;
    int outl = 0;

    if (outlen) {
      *outlen = 0;
    }
    if (0 == a->init) {
      if (NULL == EVP_CIPHER_CTX_cipher(a->ctx)) {
        rv = EVP_DecryptInit_ex(a->ctx, a->cipher, NULL, NULL, NULL);
      }
      EVP_CIPHER_CTX_ctrl(a->ctx, EVP_CTRL_GCM_SET_IVLEN, a->ivlen, NULL);
      rv = EVP_DecryptInit_ex(a->ctx, NULL, NULL, a->key, a->iv);
      a->init = 1;
    }
    if (1 == rv) {
      if (NULL != aad) {
        rv = EVP_DecryptUpdate(a->ctx, NULL, &outl, aad, aadlen);
      }
      if (NULL != data) {
        rv = EVP_DecryptUpdate(a->ctx, out, &outl, data, datalen);
        if (outlen) {
        *outlen = outl;
        }
      }
    }
    return rv;
  }

  /** @brief
      The final phase of an AES GCM operation
      @param ain an AES_GCM_CTX pointer
      @param out the buffer to hold any residual encrypted data
      @param outlen a place to hold the length of any residual data
      @param hash a place to store AES_BLOCK_SIZE bytes of the
      authentication tag
      @return 1 if O.K., 0 otherwise
  */
  int AES_GCM_EncryptFinal(AES_GCM_CTX * ain, unsigned char *out,
                           unsigned long *outlen, unsigned char *hash) {
    int rv = 1;
    AES_GCM_CTX_t *a = (AES_GCM_CTX_t *)ain;
    int outl = 0;

    if (0 == a->init) {
      if (NULL == EVP_CIPHER_CTX_cipher(a->ctx)) {
        rv = EVP_EncryptInit_ex(a->ctx, a->cipher, NULL, NULL, NULL);
      }
      EVP_CIPHER_CTX_ctrl(a->ctx, EVP_CTRL_GCM_SET_IVLEN, a->ivlen, NULL);
      rv = EVP_EncryptInit_ex(a->ctx, NULL, NULL, a->key, a->iv);

      a->init = 1;
      a->enc = 1;
    }
    outl = *outlen;
    EVP_EncryptFinal_ex(a->ctx, out, &outl);
    *outlen = outl;
    EVP_CIPHER_CTX_ctrl(a->ctx, EVP_CTRL_GCM_GET_TAG, 16, hash);

    a->init = 2;
    return rv;
  }

  /** @brief The final phase of an AES GCM decrypt operation
      @param ain an AES_GCM_CTX pointer
      @param out the buffer to hold any residual encrypted data
      @param outlen a place to hold the length of any residual data
      @param hash a place to store AES_BLOCK_SIZE bytes of the
      authentication tag
      @param hlen the length of the auth tag
      @return 1 if the operation completed with the correct hash, 0 otherwise
  */
  int AES_GCM_DecryptFinal(AES_GCM_CTX * ain, unsigned char *out,
                           unsigned long *outlen, unsigned char *hash,
                           unsigned int hlen) {
    AES_GCM_CTX_t *a = (AES_GCM_CTX_t *)ain;
    int outl = 0;
    int rv = 1;

    if (0 == a->init) {
      if (NULL == EVP_CIPHER_CTX_cipher(a->ctx)) {
        rv = EVP_DecryptInit_ex(a->ctx, a->cipher, NULL, NULL, NULL);
      }
      EVP_CIPHER_CTX_ctrl(a->ctx, EVP_CTRL_GCM_SET_IVLEN, a->ivlen, NULL);
      rv = EVP_DecryptInit_ex(a->ctx, NULL, NULL, a->key, a->iv);
      a->enc = 0;
      a->init = 1;
    }
    EVP_CIPHER_CTX_ctrl(a->ctx, EVP_CTRL_AEAD_SET_TAG, hlen, hash);
    outl = *outlen;
    rv = EVP_DecryptFinal_ex(a->ctx, out, &outl);
    *outlen = outl;
    a->init = 2;
    return rv;
  }
