/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Header for the icclib shared library
//
*************************************************************************/

#ifndef INCLUDED_ICCLIB
#define INCLUDED_ICCLIB
#ifdef  __cplusplus
extern "C" {
#endif

#if defined(__MVS__) /* must precede stdio inclusion on MVS... */
#ifndef pthread_included
#define pthread_included
#define _OPEN_THREADS 1 /* well... that remains to be seen; it redefines _OPEN_THREADS under z/OS 1.9 */
#include <pthread.h>
typedef pthread_mutex_t ICC_Mutex;
#endif
#endif

#include "iccversion.h"
#include "iccglobals.h"   /* global definitions */


#include "openssl/rand.h" /* Wrong order for Windows */
#include "openssl/evp.h"

#include "openssl/rc2.h"
#include "openssl/hmac.h"
#include "openssl/des.h"
#include "openssl/dsa.h"
#include "openssl/dh.h"
#include "openssl/sha.h"
#include "openssl/aes.h"
#include "openssl/ec.h"
#include "openssl/rsa.h"
#include "openssl/ecdh.h"
#include "openssl/ecdsa.h"
#include "openssl/pkcs12.h"
#include "openssl/err.h"
#include "openssl/cmac.h"
#include "openssl/kdf.h"


#define HMAC_CTX_cleanup(ctx) HMAC_CTX_reset(ctx)



#include "platform.h"
#include "icc_cdefs.h"
/* Pick up PRNG PRNG_CTX types */
#include "fips-prng/SP800-90.h"

typedef void (*CALLBACK_T)(const char *,int,int);

#include "icclib_a.h"
/* Pick up KDF_CTX type */ 
#include "SP800_108/SP800-108.h"
/* Pick UP KeyWrap */
#include "SP800_38F/SP80038F.h" 

/* ATSec advised this setting for 8.8.2 FIPS-140-3 */
#define ICC_TRNG_FIPS_OSR_MIN 5

/* OpenSSL's crypto callbacks don't include calloc
   and we have our own
  The parameter types for nmemb and size SHOULD be size_t
  but openssl uses int.
  This only limits the size of very large objects that could be created
*/
extern void * CRYPTO_calloc(int nmemb,int size,const char *file, int line);
extern int my_EVP_MD_CTX_free (EVP_MD_CTX * x);
int my_EVP_ENCODE_CTX_free(EVP_ENCODE_CTX * a);
int my_CMAC_Init(CMAC_CTX *cmac_ctx,const EVP_CIPHER *cipher,unsigned char *key,unsigned int keylen);

int my_CMAC_Final(CMAC_CTX *cmac_ctx,unsigned char *md,unsigned int maclen);

void GenerateRandomSeed (ICClib *pcb,ICC_STATUS *status,int num,unsigned char * buf);
int SetRNGInstances(int i);
int GenRNGInstances(void);
int Set_default_tuner(int alg);
int Get_default_tuner(void);
int fips_rand_bytes(unsigned char *buffer,int num);
/* OpenSSL function prototypes not available in headers */
extern int OPENSSL_cpuid_override(long long mask);
extern int OPENSSL_cpuid(long long *id);


extern unsigned long OPENSSL_rdtscX();
extern int OPENSSL_HW_rand(unsigned char *buf);

int RSA_X931_derive_ex(RSA *rsa, BIGNUM *p1, BIGNUM *p2, BIGNUM *q1, BIGNUM *q2,
                       const BIGNUM *Xp1, const BIGNUM *Xp2, const BIGNUM *Xp,
                       const BIGNUM *Xq1, const BIGNUM *Xq2, const BIGNUM *Xq,
                       const BIGNUM *e, BN_GENCB *cb);
                       
                       
int RSA_X931_generate_key_ex(RSA *rsa, int bits, const BIGNUM *e, BN_GENCB *cb);
int GetStatus(ICClib *pcb, ICC_STATUS *status);
int GetValue(ICClib *pcb,ICC_STATUS* status,ICC_VALUE_IDS_ENUM valueID,void* value,int valueLength);
int SetValue(ICClib *pcb,ICC_STATUS* status,ICC_VALUE_IDS_ENUM valueID,const void* value);
int lib_attach (ICClib * pcb, ICC_STATUS * status);
int lib_cleanup (ICClib *pcb,ICC_STATUS * status);
void *lib_init (ICClib * pcb, ICC_STATUS * status, const char *iccpath,const char *a, const char *b);
int SelfTest (ICClib *pcb,ICC_STATUS * status);


const BIGNUM *DH_get_PublicKey (const DH * dh);
RSA * my_RSA_new();

int my_HMAC_Init(HMAC_CTX *ctx, const void *key, int key_len,const EVP_MD *md);
unsigned char* my_HKDF_Expand(const EVP_MD* evp_md,
    const unsigned char* prk, size_t prk_len,
    const unsigned char* info, size_t info_len,
    unsigned char* okm, size_t okm_len);
unsigned char* my_HKDF_Extract(const EVP_MD* evp_md,
    const unsigned char* salt, size_t salt_len,
    const unsigned char* key, size_t key_len,
    unsigned char* prk, size_t* prk_len);
int my_EVP_DecryptInit(EVP_CIPHER_CTX *ctx,const EVP_CIPHER *type,unsigned char *key, unsigned char *iv);
int my_EVP_EncryptInit(EVP_CIPHER_CTX *ctx,const EVP_CIPHER *type,unsigned char *key, unsigned char *iv);
int my_EVP_PKEY_encrypt(unsigned char *enc_key,unsigned char *key,int key_len,EVP_PKEY *pub_key);
int my_EVP_PKEY_decrypt(unsigned char *dec_key,unsigned char *enc_key,int enc_key_len,EVP_PKEY *private_key);
int my_HMAC_Init(HMAC_CTX *ctx, const void *key, int key_len,const EVP_MD *md);
const FUNC *OS_helpers();

#include "platform_api.h"

#include "aes_gcm.h"
#include "aes_ccm.h"

#ifdef  __cplusplus
}
#endif


#endif /*INCLUDED_ICC*/
