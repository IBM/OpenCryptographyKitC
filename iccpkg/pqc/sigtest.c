/* Copyright IBM Corp. 2023

 Licensed under the Apache License 2.0 (the "License"). You may not use
 this file except in compliance with the License. You can obtain a copy
 in the file LICENSE in the source distribution.
 */
 
 /*
  sigtest.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if 0
#include <stdbool.h>
#else
#define bool int
#define false 0
#define true  1
#endif

#if defined(_WIN32)
#   include <windows.h>
#else
#   include <stdlib.h>
#endif

# include "icc.h"

/* We want to use PKCS1 and PKCS8 encodings for i2d/d2i */
/* these are all independent bits that can be combined */
/* public is pkcs1 or binary (default) */
/* private is none, raw or pkcs8 */
enum ed { none = 0, raw = 1, pkcs1 = 2, pkcs8 = 4 };

/* Helper function to print byte arrays in hexadecimal */
static
void fprintBstr(FILE* fp, const char* S, const unsigned char* A, size_t L)
{
   size_t i;

   fprintf(fp, "%s", S);

   for (i = 0; i < L; i++)
      fprintf(fp, "%02X", A[i]);

   if (L == 0)
      fprintf(fp, "00");

   fprintf(fp, "\n");
}

static
void
count_up(unsigned char* b, size_t n)
{
   size_t i;
   for (i = 0; i < n; i++) {
      b[i] = (unsigned char)i;
   }
}


/* Key buffer definitions */
struct kbuf_s {
   unsigned char* data;
   size_t len;
};
typedef struct kbuf_s kbuf;

/* Public key - encoded */
struct pkbuf_s {
   int nid; /* ICC key id */
   kbuf der;
};
typedef struct pkbuf_s pkbuf;

/* Private key - holds context */
struct skbuf_s {
   ICC_EVP_PKEY_CTX* ctx; /* ICC context for the key */
   ICC_EVP_PKEY* key; /* the key */
   int nid; /* ICC key id */
   kbuf der;
};
typedef struct skbuf_s skbuf;

/* Signature buffer */
typedef kbuf sbuf;

/* Generate signature key pair - return 0 for success */

int
SignatureEVP_gen(ICC_CTX* ctx, const char* nm, pkbuf* p_pkc, skbuf* p_skc, enum ed encdec)
{
   ICC_EVP_PKEY_CTX* evp_sp; /* key context */
   ICC_EVP_PKEY* pa = NULL; /* key pair */
   int rv = ICC_OSSL_SUCCESS;

   const int nid = ICC_OBJ_txt2nid(ctx, nm);
   if (!nid) {
      return 1; /* Unsupported algorithm */
   }

   evp_sp = ICC_EVP_PKEY_CTX_new_id(ctx, nid, NULL);
   if (!evp_sp) {
      /* try newer API */
      evp_sp = ICC_EVP_PKEY_CTX_new_from_name(ctx, NULL, nm, NULL);
      if (!evp_sp) {
         return 2; /* Failed to create key context */
      }
   }
   rv = ICC_EVP_PKEY_keygen_init(ctx, evp_sp);
   if (rv != ICC_OSSL_SUCCESS) {
      if (evp_sp) {
         ICC_EVP_PKEY_CTX_free(ctx, evp_sp);
      }
      return 3; /* Keygen initialization failed */
   }

   rv = ICC_EVP_PKEY_keygen(ctx, evp_sp, &pa);
   if (rv != ICC_OSSL_SUCCESS) {
      if (evp_sp) {
         ICC_EVP_PKEY_CTX_free(ctx, evp_sp);
      }
      if (pa) {
         ICC_EVP_PKEY_free(ctx, pa);
      }
      return 4; /* Key generation failed */
   }

   /* public key */

   p_pkc->nid = nid;

   /* encode (always) */
   {
      int len;
      unsigned char* pp = NULL;
      if (encdec & pkcs1) {
         /* PKCS1 */
         /* this variant encodes the OID for the key type rather than a raw encoding */
         len = ICC_i2d_PUBKEY(ctx, pa, NULL);
         if (len <= 0) {
            return 5;
         }
         p_pkc->der.len = len;
         pp = p_pkc->der.data = malloc(len);
         rv = ICC_i2d_PUBKEY(ctx, pa, &pp);
      }
      else {
         /* binary */
         len = ICC_i2d_PublicKey(ctx, pa, NULL);
         if (len <= 0) {
            return 5; /* Invalid public key length */
         }
         p_pkc->der.len = len;
         pp = p_pkc->der.data = malloc(len);
         rv = ICC_i2d_PublicKey(ctx, pa, &pp);
      }
      if (rv <= 0) {
         return 6; /* Failed to encode public key */
      }
   }

   /* private key */

   p_skc->nid = nid;

   /* encode (optional) */

   {
      int len;
      unsigned char* pp = NULL;
      if (encdec & pkcs8) {
         ICC_PKCS8_PRIV_KEY_INFO* p8 = ICC_EVP_PKEY2PKCS8(ctx, pa);
         if (!p8) {
            return 11;
         }
         len = ICC_i2d_PKCS8_PRIV_KEY_INFO(ctx, p8, NULL);
         p_skc->der.len = len;
         pp = p_skc->der.data = malloc(len);
         rv = ICC_i2d_PKCS8_PRIV_KEY_INFO(ctx, p8, &pp);
         if (rv <= 0) {
            return 7; /* Failed to encode private key */
         }
      }
      else if (encdec & raw) {
         len = ICC_i2d_PrivateKey(ctx, pa, NULL);
         p_skc->der.len = len;
         pp = p_skc->der.data = malloc(len);
         rv = ICC_i2d_PrivateKey(ctx, pa, &pp);
         if (rv <= 0) {
            return 7; /* Failed to encode private key */
         }
      }
   }

   p_skc->ctx = evp_sp;
   p_skc->key = pa;

   /* verify encodings */
   /* reconstruct keys from encoding */
   {
      int len;
      ICC_EVP_PKEY* npa = NULL; /* For decoded key */
      const unsigned char* pp = NULL;

      /* public */
      pp = p_pkc->der.data;
      len = (int)p_pkc->der.len;
      if (encdec & pkcs1) {
         /* reconstruct key from encoding */
         npa = ICC_d2i_PUBKEY(ctx, &npa, &pp, len);
      }
      else {
         /* Reconstruct public key from encoding and type */
         npa = ICC_d2i_PublicKey(ctx, p_pkc->nid, &npa, &pp, len);
      }
      if (!npa) {
         return 1; /* Failed to reconstruct public key */
      }

      if (1 != ICC_EVP_PKEY_cmp(ctx, pa, npa)) { /*compare pubkey and decoded key */
         printf("warning - public key encode/decode missmatch\n");
          /* return 20; */
      }
      ICC_EVP_PKEY_free(ctx, npa);
      npa = NULL;

      /* private */
      pp = p_skc->der.data;
      len = (int)p_skc->der.len;
      if (encdec & pkcs8) {
         ICC_PKCS8_PRIV_KEY_INFO* p8 = NULL;
         p8 = ICC_d2i_PKCS8_PRIV_KEY_INFO(ctx, NULL, &pp, len);
         if (!p8) {
            return 11;
         }
         npa = ICC_EVP_PKCS82PKEY(ctx, p8);
         ICC_PKCS8_PRIV_KEY_INFO_free(ctx, p8);
         if (!npa) {
            return 9;
         }
      }
      else if (encdec & raw) {
         npa = ICC_d2i_PrivateKey(ctx, p_skc->nid, &npa, &pp, len);
         if (!npa) {
            return 9;
         }
      }

      if(npa) {
         size_t keylen = ICC_EVP_PKEY_size(ctx, pa);
         size_t kl = ICC_EVP_PKEY_size(ctx, npa);
         if (keylen == 0 || kl != keylen) {
            printf("warning - key size missmatch %d != %d\n", (int)keylen, (int)kl);
            return 22;
         }
         if (1 != ICC_EVP_PKEY_cmp(ctx, pa, npa)) {
            printf("warning - private key encode/decode missmatch\n");
            /* return 21; */
         }
      }

      ICC_EVP_PKEY_free(ctx, npa);
   }

   return 0;
}

/* Sign a message - return 0 for success, non-zero otherwise */

int
SignatureEVP_sign(ICC_CTX* ctx, sbuf* sig, const skbuf* p_skc, const unsigned char* msg, size_t msg_len, enum ed encdec, const char* hash)
{
   int rc;
   size_t siglen = 0;
   unsigned char* signature;

   ICC_EVP_PKEY* skey = p_skc->key;
   if (!skey) {
      /* reconstruct key from encoding */
      const unsigned char* pp = p_skc->der.data;
      if (encdec & pkcs8) {
         ICC_PKCS8_PRIV_KEY_INFO* p8 = NULL;
         p8 = ICC_d2i_PKCS8_PRIV_KEY_INFO(ctx, NULL, &pp, (long)p_skc->der.len);
         if (!p8) {
            return 11;
         }
         skey = ICC_EVP_PKCS82PKEY(ctx, p8);
         ICC_PKCS8_PRIV_KEY_INFO_free(ctx, p8);
      }
      else {
         skey = ICC_d2i_PrivateKey(ctx, p_skc->nid, &skey, &pp, (long)p_skc->der.len);
      }
      if (!skey) {
         return 9;
      }
   }

   /*
   * May need to hash if sign alg is limited in size.
   * Note that PQC hash internally so no hashing reqired.
   */
   if (!hash) {
      ICC_EVP_PKEY_CTX* skc = p_skc->ctx; /* Private key context */

      if (!skc) {
         /* create context from key */
         skc = ICC_EVP_PKEY_CTX_new(ctx, skey, NULL);
         if (!skc) {
            return 10; /* no context */
         }
      }

      rc = ICC_EVP_PKEY_sign_init(ctx, skc);
      if (rc != ICC_OSSL_SUCCESS) {
         return 1; /* Sign initialization failed */
      }

      rc = ICC_EVP_PKEY_sign(ctx, skc, NULL, &siglen, msg, msg_len);
      if (rc != ICC_OSSL_SUCCESS) {
         return 2; /* Failed to get signature length */
      }

      signature = malloc(siglen);
      if (!signature) {
         return 3; /* Memory allocation failed */
      }

      rc = ICC_EVP_PKEY_sign(ctx, skc, signature, &siglen, msg, msg_len);
      if (rc != ICC_OSSL_SUCCESS) {
         free(signature);
         return 4; /* Signing failed */
      }

      if (!p_skc->ctx) {
         /* free temp ctx */
         ICC_EVP_PKEY_CTX_free(ctx, skc);
      }
   }
   else {
      /* need to hash / sign */
      unsigned int slen;
      ICC_EVP_MD_CTX* md = NULL;
      md = ICC_EVP_MD_CTX_new(ctx);
      ICC_EVP_MD_CTX_init(ctx, md);
      {
         const ICC_EVP_MD* mdt = NULL; /* does not need to be freed */
         mdt = ICC_EVP_get_digestbyname(ctx, hash);
         rc = ICC_EVP_SignInit(ctx, md, mdt);
         if (rc != ICC_OSSL_SUCCESS) {
            return 5;
         }
      }
      rc = ICC_EVP_SignUpdate(ctx, md, msg, (unsigned int)msg_len);
      if (rc != ICC_OSSL_SUCCESS) {
         return 6;
      }

      rc = ICC_EVP_SignFinal(ctx, md, NULL, &slen, skey);
      if (rc != ICC_OSSL_SUCCESS) {
         if (!p_skc->key) {
            /* clean up temporary */
            ICC_EVP_PKEY_free(ctx, skey);
         }
         return 7;
      }
      signature = malloc(slen);
      if (!signature) {
         if (!p_skc->key) {
            /* clean up temporary */
            ICC_EVP_PKEY_free(ctx, skey);
         }
         return 3; /* Memory allocation failed */
      }
      rc = ICC_EVP_SignFinal(ctx, md, signature, &slen, skey);
      if (rc != ICC_OSSL_SUCCESS) {
         free(signature);
         if (!p_skc->key) {
            /* clean up temporary */
            ICC_EVP_PKEY_free(ctx, skey);
         }
         return 4; /* Signing failed */
      }
      siglen = slen;

      ICC_EVP_MD_CTX_free(ctx, md);
   }

   sig->data = signature;
   sig->len = siglen;

   if (!p_skc->key) {
      /* clean up temporary */
      ICC_EVP_PKEY_free(ctx, skey);
   }

   return 0; /* Success */
}

/* Verify a signature - return 0 for success, non-zero for failure */
int
SignatureEVP_verify(ICC_CTX* ctx, const pkbuf* p_pkc, const unsigned char* msg, size_t msg_len, const sbuf* sig, enum ed encdec, const char* hash)
{
   int rc = -1;
   ICC_EVP_PKEY* pa = NULL;
   ICC_EVP_PKEY_CTX* evp_pk = NULL;

   /* public key */
   {
      const unsigned char* pp = p_pkc->der.data;
      long len = (long)p_pkc->der.len;

      if (encdec & pkcs1) {
         /* reconstruct key from encoding */
         pa = ICC_d2i_PUBKEY(ctx, &pa, &pp, (long)len);
      }
      else {
         /* Reconstruct public key from encoding and type */
         pa = ICC_d2i_PublicKey(ctx, p_pkc->nid, &pa, &pp, len);
      }
      if (!pa) {
         return 1; /* Failed to reconstruct public key */
      }
   }

   /* EVP context */
   evp_pk = ICC_EVP_PKEY_CTX_new(ctx, pa, NULL);
   if (!evp_pk) {
      /* try new API */
      evp_pk = ICC_EVP_PKEY_CTX_new_from_pkey(ctx, NULL, pa, NULL);
      if (!evp_pk) {
         ICC_EVP_PKEY_free(ctx, pa);
         return 2; /* Failed to create public key context */
      }
   }

   if (!hash) {
      rc = ICC_EVP_PKEY_verify_init(ctx, evp_pk);
      if (rc != ICC_OSSL_SUCCESS) {
         ICC_EVP_PKEY_free(ctx, pa);
         ICC_EVP_PKEY_CTX_free(ctx, evp_pk);
         return 3; /* Verification initialization failed */
      }

      rc = ICC_EVP_PKEY_verify(ctx, evp_pk, sig->data, sig->len, msg, msg_len);
   }
   else {
      /* need to hash / verify */
      
      ICC_EVP_MD_CTX* md = NULL;
      md = ICC_EVP_MD_CTX_new(ctx);
      ICC_EVP_MD_CTX_init(ctx, md);
      {
         const ICC_EVP_MD* mdt = NULL; /* does not need to be freed */
         mdt = ICC_EVP_get_digestbyname(ctx, hash);
         rc = ICC_EVP_VerifyInit(ctx, md, mdt);
         if (rc != ICC_OSSL_SUCCESS) {
            ICC_EVP_MD_CTX_free(ctx, md);
            ICC_EVP_PKEY_free(ctx, pa);
            ICC_EVP_PKEY_CTX_free(ctx, evp_pk);
            return 5;
         }
      }
      rc = ICC_EVP_VerifyUpdate(ctx, md, msg, (unsigned int)msg_len);
      if (rc != ICC_OSSL_SUCCESS) {
         ICC_EVP_MD_CTX_free(ctx, md);
         ICC_EVP_PKEY_free(ctx, pa);
         ICC_EVP_PKEY_CTX_free(ctx, evp_pk);
         return 6;
      }

      rc = ICC_EVP_VerifyFinal(ctx, md, sig->data, (unsigned int)sig->len, pa);
      ICC_EVP_MD_CTX_free(ctx, md);
   }

   /* clean up */
   ICC_EVP_PKEY_free(ctx, pa);
   ICC_EVP_PKEY_CTX_free(ctx, evp_pk);
 
   /* final status check of the verification call */
   if (rc < 0) {
      return 4; /* Verification error */
   }
   if (rc == 0) {
      return 5; /* Verification failed */
   }

   return 0; /* Verification succeeded */
}

/* Signature test function - generates keys, signs a message, and verifies the signature */
int
PQC_sign_test(ICC_CTX* ctx, const char* algname, const char* hash, size_t msg_len, int verbose, int encdec)
{
   FILE* fp_rsp = stdout;
   int ret_val;

   pkbuf pk = { 0 }; /* public key */
   skbuf sk = { 0 }; /* private/secret key */

   if (verbose) {
      printf("Algorithm : %s\n", algname);
      printf("Hash : %s\n", hash? hash:"NULL");
      printf("Data length : %u\n", (unsigned)msg_len);
   }

   {
      /* Generate the public/private keypair */
      if (verbose) {
         printf("keygen\t");
      }
      if ((ret_val = SignatureEVP_gen(ctx, algname, &pk, &sk, encdec)) != 0) {
         printf("Error: SignatureEVP_gen(ctx, %s, &pk, &sk, %d) returned <%d>\n", algname, encdec, ret_val);
         return 1;
      }
      if (verbose) {
         fprintBstr(fp_rsp, "pk = ", pk.der.data, pk.der.len);
         if (encdec & (raw | pkcs8)) {
            fprintBstr(fp_rsp, "sk = ", sk.der.data, sk.der.len);
         }
      }
   }

   {
      int keylen = 0;
      keylen = ICC_EVP_PKEY_size(ctx, sk.key);
      if (verbose) {
         fprintf(fp_rsp, "key size = %d", keylen);
      }
   }

   /* get rid of gen context */
   if (sk.ctx) {
      ICC_EVP_PKEY_CTX_free(ctx, sk.ctx);
      sk.ctx = NULL;
   }

   if (encdec & (raw|pkcs8)) {
      /* delete ICC key and context so we use private encoding */
      if (sk.key) {
         ICC_EVP_PKEY_free(ctx, sk.key);
         sk.key = NULL;
      }
   }

   {
      sbuf signature;
      unsigned char* message = malloc(msg_len);
      if (message) {
         count_up(message, msg_len);
      }

      if (verbose) {
         printf("sign\t");
      }
      if ((ret_val = SignatureEVP_sign(ctx, &signature, &sk, message, msg_len, encdec, hash)) != 0) {
         printf("SignatureEVP_sign returned <%d>\n", ret_val);
         return 2;
      }
      if (verbose) {
         fprintBstr(fp_rsp, "signature = ", signature.data, signature.len);
      }

      fprintf(fp_rsp, "\n");
      if (verbose) {
         printf("verify\t");
      }
      {
         ret_val = SignatureEVP_verify(ctx, &pk, message, msg_len, &signature, encdec, hash);
         if (ret_val != 0) {
            printf("SignatureEVP_verify failed with code <%d>\n", ret_val);
            return 3;
         }
         else {
            if (verbose)
               printf("Signature verification succeeded.\n");
         }
      }
      if (signature.data) {
         free(signature.data);
         signature.data = NULL;
      }
      free(message);
   }

   /* Clean up allocated resources */
   if (pk.der.data) {
      free(pk.der.data);
      pk.der.data = NULL;
   }
   if (sk.ctx) {
      ICC_EVP_PKEY_CTX_free(ctx, sk.ctx);
      sk.ctx = NULL;
   }
   if (sk.key) {
      ICC_EVP_PKEY_free(ctx, sk.key);
      sk.key = NULL;
   }
   if (sk.der.data) {
      free(sk.der.data);
      sk.der.data = NULL;
   }

   return 0;
}

static
char* algs[] =
{
   "rsaEncryption",
   "ML_DSA_44", /* "Dilithium_512",*/
   "ML_DSA_65", /* "Dilithium_768",*/
   "ML_DSA_87", /* "Dilithium_1024",*/
   "SLH_DSA_SHAKE_128s", /* sphincs */
   /*
   "SLH_DSA_SHAKE_192s",
   "SLH_DSA_SHAKE_256s",
   */
   NULL
};

/* Map command line arguments to signature algorithm names */
static
const char* to_SIGNATURE_ALGNAME(int k)
{
   /* k is 1 based so adjust for 0 based index */
   if (k > sizeof(algs) / sizeof(algs[0]))
      return NULL;
   return algs[k-1];
}

static
void fcb(const char* a, int b, int c)
{
   printf("fcb:%s, %d, %d\n", a, b, c);
}

static
void tcb(const char* val1, const char* val2)
{
   printf("Tcb:%s, %s \n", val1, val2);
}

static
int OpenSSLError(ICC_CTX* ctx)
{
   unsigned long retcode = -1;
   unsigned max = 5;
   /* may be more than one error recorded so print them all */
   while (retcode) {
      retcode = ICC_ERR_get_error(ctx);
      if (retcode) {
         static char buf[4096];
         ICC_ERR_error_string(ctx, retcode, buf);
         printf("OpenSSL error %d [%s]\n", retcode, buf);
      }
      /* infinite loop breaker */
      if (max == 0) break;
      max--;
   }
   return retcode;
}

/* Main function to parse arguments and execute signature tests */
int main(int argc, const char *argv[])
{
   const char* algname = NULL;
   const char* hash = NULL; /*eg, "SHA256"*/
   const char* iccPath = NULL;
   bool isFips = false, wantFips = false, verbose = false ;
   bool wantTraceCB = false; /* Trace callback */
   bool wantFipsCB = false; /* FIPS callback */
   size_t dataSize = 100;
   enum ed encdec = none;
   int rv = 0;

   /* Parse command-line arguments */
   if(argc > 1) {
      const char* arg;
      int i;
      for( i = 1; i < argc; i++) {
         arg = argv[i];
         if (NULL != strstr(arg, "-?")) {
            int j;
            printf("Usage: sigtest [-v] [-fips] [-fcb] [-tcb] [-alg <algorithm name>] [-h <hash algorithm name>] [-l <message data length>] [-ed <none|raw|pkcs[1|8]>] [<number (see list below)>]\n");
            printf("     -fips    Request FIPS mode ICC\n");
            printf("     -fcb     Install a FIPS callback routine (prints message 'fcb:...')\n");
            printf("     -tcb     Install a TRACE callback routine (prints message 'tcb:...')\n");
            printf("     -alg     Refer following table...\n");
            for ( j = 1; to_SIGNATURE_ALGNAME(j); j++) {
               printf(" %d    %s\n", j, to_SIGNATURE_ALGNAME(j));
            }
            printf("     -hash    OpenSSL/ICC hash function (e.g. SHA256)\n");
            printf("     -ed      Key Encoding\n");
            return 0;
         }
         else if (NULL != strstr(arg, "-fips")) {
            wantFips = true;
         }
         else if (NULL != strstr(arg, "-fcb")) {
            wantFipsCB = true;
            wantFips = true;
         }
         else if (NULL != strstr(arg, "-tcb")) {
            wantTraceCB = true;
         }
         else if (NULL != strstr(arg, "-h")) {
            i++;
            hash = argv[i];
         }
         else if (NULL != strstr(arg, "-l")) {
            i++;
            dataSize = atoi(argv[i]);
         }
         else if (NULL != strstr(arg, "-p")) {
            i++;
            iccPath = argv[i];
         }
         else if (NULL != strstr(arg, "-v")) {
            verbose = true;
         }
         else if (NULL != strstr(arg, "-ed")) {
            i++;
            arg = argv[i];
            if (!strcmp(arg, "none"))
               encdec = none;
            else if (!strcmp(arg, "raw"))
               encdec = raw;
            else if (!strcmp(arg, "pkcs1"))
               encdec = pkcs1;
            else if (!strcmp(arg, "pkcs8"))
               encdec = pkcs8;
            else if (!strcmp(arg, "pkcs"))
               encdec = pkcs1 | pkcs8;
            else {
               printf("%s: bad encoding, try -? to get help\n", arg);
               return -1;
            }
         }
         else if (NULL != strstr(arg, "-alg")) {
            i++;
            algname = argv[i];
         }
         else if (*arg == '-') {
            /* another setting - pass it on */
            i++;
         }
         else {
            int k = 0;
            k = (int)atoi(arg);
            algname = to_SIGNATURE_ALGNAME(k);
            if (k == 0 || !algname) {
               printf("%s: bad argument, try -? to get help\n", arg);
               return -1;
            }
         }
      }
   }

   {
      ICC_STATUS status;
      ICC_CTX* icc_ctx = NULL;

      /* Initialize ICC context */
      icc_ctx = ICC_Init(&status, iccPath);
      if (NULL == icc_ctx) {
         printf("ICC not initialized, exiting\n");
         if (iccPath)
            printf("icc path was: %s\n", iccPath);
         exit(1);
      }

      /*
      * Set FIPS only work before the attach
      */
      if (wantFips) {
         isFips = true;
         ICC_SetValue(icc_ctx, &status, ICC_FIPS_APPROVED_MODE, wantFips ? "on" : "off");
         if (ICC_OK != status.majRC)
         {
            printf("ICC_SetValue(ICC_FIPS_APPROVED_MODE) failed %d\n", rv);
            isFips = false;
         }
      }
      printf("FIPS %s.\n", isFips ? "on" : "off");

      /* Attach to ICC */
      if (ICC_ERROR == ICC_Attach(icc_ctx, &status)) {
         printf("ICC_Attach() failed, exiting\n");
         ICC_Cleanup(icc_ctx, &status);
         exit(1);
      }

      /* check and report the ICC version we found */
      {
         char iccversion[ICC_VALUESIZE+1];
         if (ICC_ERROR == ICC_GetValue(icc_ctx, &status, ICC_VERSION, (void*)iccversion, ICC_VALUESIZE)) {
            printf("ICC_GetValue() failed, exiting\n");
            ICC_Cleanup(icc_ctx, &status);
            exit(1);
         }
         iccversion[ICC_VALUESIZE] = '\0';
         printf("ICC_Version: %s\n", iccversion);
         if (strstr(iccversion, "8.6") != NULL) {
            printf("sigtest: %s\n", "ICC 8.6 not supported");
            exit(1);
         }
      }

      /*
      * Callbacks only work after the attach
      * Also, FIPS callback only works in FIPS mode
      */
      if (wantFipsCB) {
         typedef void (*CALLBACK_T)(const char*, int, int);
         CALLBACK_T x = fcb;

         rv = ICC_SetValue(icc_ctx, &status, ICC_FIPS_CALLBACK, &x);
         if (ICC_OK != status.majRC && isFips)
         {
            printf("ICC_SetValue(ICC_FIPS_CALLBACK) failed %d\n", rv);
            isFips = false;
         }
      }

      if (wantTraceCB) {
         typedef void (*TRACE_CALLBACK_T)(const char*, const char*);
         TRACE_CALLBACK_T x = tcb;
      
         rv = ICC_SetValue(icc_ctx, &status, ICC_TRACE_CALLBACK, &x);
         if (ICC_OK != status.majRC)
         {
            printf("ICC_SetValue(ICC_TRACE_CALLBACK) failed %d\n", rv);
         }
         ICC_GetValue(icc_ctx, &status, ICC_TRACE_CALLBACK, &x, sizeof(TRACE_CALLBACK_T));
      }
  
#if 0
      /* ICC has no DRGB so we can't do KAT on ICC */
      {
         unsigned char entropy_input[48];
         count_up(entropy_input, sizeof(entropy_input));
         randombytes_init(entropy_input, NULL, 256);
      }
#endif

      if (!algname) {
         /* default */
         algname = to_SIGNATURE_ALGNAME(3); /* Dilithium 768 */
         printf("algname = %s\n", algname?algname:"NULL");
      }
      /* Execute the signature test */
      rv = PQC_sign_test(icc_ctx, algname, hash, dataSize, verbose, encdec);
      if (rv) {
         OpenSSLError(icc_ctx);
         printf("%s: Error %d, try -? to get help\n", algname, rv);
      }

      /* Clean up ICC context */
      ICC_Cleanup(icc_ctx, &status);
   }

   return rv;
}
