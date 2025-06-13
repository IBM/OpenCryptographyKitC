/* Copyright IBM Corp. 2023

 Licensed under the Apache License 2.0 (the "License"). You may not use
 this file except in compliance with the License. You can obtain a copy
 in the file LICENSE in the source distribution.
 */
 
 /*
  kemtest.c
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

#   include "icc.h"

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

/* Shared secret */
typedef kbuf ssbuf;

int pubEncode(ICC_CTX* ctx, const ICC_EVP_PKEY* pa, kbuf* der, enum ed encdec)
{
   int rv = 0;
   int len;
   unsigned char* pp = NULL;
   if (encdec & pkcs1) {
      /* PKCS1 */
      /* this variant encodes the OID for the key type rather than a raw encoding */
      len = ICC_i2d_PUBKEY(ctx, pa, NULL);
      if (len <= 0) {
         return 5;
      }
      der->len = len;
      pp = der->data = malloc(len);
      rv = ICC_i2d_PUBKEY(ctx, pa, &pp);
   }
   else {
      /* binary */
      len = ICC_i2d_PublicKey(ctx, pa, NULL);
      if (len <= 0) {
         return 5; /* Invalid public key length */
      }
      der->len = len;
      pp = der->data = malloc(len);
      rv = ICC_i2d_PublicKey(ctx, pa, &pp);
   }
   if (rv <= 0) {
      return 6; /* Failed to encode public key */
   }

   return rv;
}

ICC_EVP_PKEY* pubDecode(ICC_CTX* ctx, int nid, const kbuf* der, enum ed encdec)
{
   ICC_EVP_PKEY* npa = NULL; /* For decoded key */
   const unsigned char* pp = NULL;
   int len;

   pp = der->data;
   len = (int)der->len;
   if (encdec & pkcs1) {
      /* reconstruct key from encoding */
      npa = ICC_d2i_PUBKEY(ctx, &npa, &pp, len);
   }
   else {
      /* Reconstruct public key from encoding and type */
      npa = ICC_d2i_PublicKey(ctx, nid, &npa, &pp, len);
   }
   return npa;
}

int priEncode(ICC_CTX* ctx, const ICC_EVP_PKEY* pa, kbuf* der, enum ed encdec)
{
   int rv = 0;
   int len;
   unsigned char* pp = NULL;
   if (encdec & pkcs8) {
      ICC_PKCS8_PRIV_KEY_INFO* p8 = ICC_EVP_PKEY2PKCS8(ctx, pa);
      if (!p8) {
         return 11;
      }
      len = ICC_i2d_PKCS8_PRIV_KEY_INFO(ctx, p8, NULL);
      der->len = len;
      pp = der->data = malloc(len);
      rv = ICC_i2d_PKCS8_PRIV_KEY_INFO(ctx, p8, &pp);
      if (rv <= 0) {
         return 7; /* Failed to encode private key */
      }
   }
   else if (encdec & raw) {
      len = ICC_i2d_PrivateKey(ctx, pa, NULL);
      der->len = len;
      pp = der->data = malloc(len);
      rv = ICC_i2d_PrivateKey(ctx, pa, &pp);
      if (rv <= 0) {
         return 7; /* Failed to encode private key */
      }
   }
   return rv;
}

ICC_EVP_PKEY* priDecode(ICC_CTX* ctx, int nid, const kbuf* der, enum ed encdec)
{
   ICC_EVP_PKEY* npa = NULL; /* For decoded key */
   const unsigned char* pp = NULL;
   int len;

   pp = der->data;
   len = (int)der->len;
   if (encdec & pkcs8) {
      ICC_PKCS8_PRIV_KEY_INFO* p8 = NULL;
      p8 = ICC_d2i_PKCS8_PRIV_KEY_INFO(ctx, NULL, &pp, len);
      if (!p8) {
         return NULL;
      }
      npa = ICC_EVP_PKCS82PKEY(ctx, p8);
      ICC_PKCS8_PRIV_KEY_INFO_free(ctx, p8);
      if (!npa) {
         return NULL;
      }
   }
   else if (encdec & raw) {
      npa = ICC_d2i_PrivateKey(ctx, nid, &npa, &pp, len);
      if (!npa) {
         return NULL;
      }
   }
   /* else no encoding - not an error */
   return npa;
}

/* Generate KEM key pair - return 0 for success */

int
KEMEVP_gen(ICC_CTX* ctx, const char* nm, pkbuf* p_pkc, skbuf* p_skc, enum ed encdec)
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
   pubEncode(ctx, pa, &p_pkc->der, encdec);

   /* private key */
   p_skc->nid = nid;
   /* encode (optional) */
   priEncode(ctx, pa, &p_skc->der, encdec);
   p_skc->ctx = evp_sp;
   p_skc->key = pa;

   /* verify encodings */
   /* reconstruct keys from encoding */
   {
      ICC_EVP_PKEY* npa = NULL; /* For decoded key */

      /* public */
      npa = pubDecode(ctx, p_pkc->nid, &p_pkc->der, encdec);
      if (!npa) {
         printf("Error pubDecode failed\n");
         return 1;
      }

      if (1 != ICC_EVP_PKEY_cmp(ctx, pa, npa)) { /*compare pubkey and decoded key */
         kbuf pb = { 0 };
         printf("warning - public key encode/decode missmatch\n");
         fprintBstr(stdout, "public key old\n", p_pkc->der.data, p_pkc->der.len);
         pubEncode(ctx, npa, &pb, encdec);
         if (p_pkc->der.len != pb.len  || memcmp(p_pkc->der.data, pb.data, pb.len)) {
            printf("warning - public key encode/decode/encode missmatch\n");
            fprintBstr(stdout, "public key new\n", pb.data, pb.len);
         }
         return 20;
      }
      ICC_EVP_PKEY_free(ctx, npa);
      npa = NULL;

      /* private */
      npa = priDecode(ctx, p_skc->nid, &p_skc->der, encdec);
      if (npa) {
         size_t keylen = ICC_EVP_PKEY_size(ctx, pa);
         size_t kl = ICC_EVP_PKEY_size(ctx, npa);
         if (keylen == 0 || kl != keylen) {
            printf("warning - key size missmatch %d != %d\n", (int)keylen, (int)kl);
            return 22;
         }
         if (1 != ICC_EVP_PKEY_cmp(ctx, pa, npa)) {
            kbuf sb = { 0 };
            printf("warning - private key encode/decode missmatch\n");
            fprintBstr(stdout, "private key old\n", p_skc->der.data, p_skc->der.len);
            priEncode(ctx, npa, &sb, encdec);
            if (p_skc->der.len != sb.len || memcmp(p_skc->der.data, sb.data, sb.len)) {
               printf("warning - private key encode/decode/encode missmatch\n");
               fprintBstr(stdout, "private key new\n", sb.data, sb.len);
            }
            return 21;
         }

         ICC_EVP_PKEY_free(ctx, npa);
         npa = NULL;
      }
   }

   return 0;
}

/* KEM encapsulation - return 0 for success */

int
KEMEVP_enc(ICC_CTX* ctx, kbuf* pks, kbuf* ss, const pkbuf* p_pkc, int argc, const char * argv[], enum ed encdec)
{
   ICC_EVP_PKEY* pa = NULL;
   const unsigned char* pp;
   pp = p_pkc->der.data;
   size_t len = p_pkc->der.len;

   pa = pubDecode(ctx, p_pkc->nid, &p_pkc->der, encdec);
   if (!pa) {
      printf("KEMEVP_enc: Error pubDecode failed\n");
      return 1;
   }
   
   ICC_EVP_PKEY_CTX* evp_pk;
   evp_pk = ICC_EVP_PKEY_CTX_new_from_pkey(ctx, NULL, pa, NULL);
   if (!evp_pk) {
       return 2;
   }
    
#if 1
   /* ICC doesn't support ICC_EVP_PKEY_CTX_set_kem_op and only RSA appears to use it */
   /* make any kem operational settings */
   if (argv && argc) {
      int i = 0;
      for (i = 1; i < argc; i++) {
         if (NULL == strstr(argv[i], "-op") || i+1 == argc) {
            continue;
         }
         i++;

         /* Set the mode - only 'RSASVE' is currently supported */
         if (ICC_EVP_PKEY_CTX_set_kem_op(ctx, evp_pk, argv[i]) <= 0) {
            return 7;
         }
      }
   }
#endif

   int rc = -1;

   rc = ICC_EVP_PKEY_encapsulate_init(ctx, NULL, NULL);
   if (rc != ICC_OSSL_SUCCESS) {
      return 3;
   }

   size_t wrappedkeylen = 0;
   size_t genkeylen = 0;

   rc = ICC_EVP_PKEY_encapsulate(ctx, evp_pk, NULL, &wrappedkeylen, NULL, &genkeylen);
   if (rc != ICC_OSSL_SUCCESS) {
      return 4;
   }

   kbuf wk;
   wk.len = wrappedkeylen;
   wk.data = malloc(wrappedkeylen);
   unsigned char* wrappedkey = wk.data;
   kbuf gk;
   gk.len = genkeylen;
   gk.data = malloc(genkeylen);
   unsigned char* genkey = gk.data;

   rc = ICC_EVP_PKEY_encapsulate(ctx, evp_pk, wrappedkey, &wrappedkeylen, genkey, &genkeylen);
   if (rc != ICC_OSSL_SUCCESS) {
      return 5;
   }

   ICC_EVP_PKEY_free(ctx, pa);
   ICC_EVP_PKEY_CTX_free(ctx, evp_pk);
   *ss = gk;
   /* public key to share with peer is just the ss encrypted with peer's public key */
   *pks = wk;

   return 0;
}

/* KEM decapsulation - return 0 for success */

int
KEMEVP_dec(ICC_CTX* ctx, const skbuf* p_skc, kbuf* ss, const kbuf* p_pks, enum ed encdec)
{
   ICC_EVP_PKEY_CTX* skc = p_skc->ctx; /* private key context */
   int rc;

   if (!skc) {
      /* make a temporary context */
      ICC_EVP_PKEY* skey = p_skc->key; /* private key */

      if (!skey) {
         /* reconstruct key from encoding */
         const unsigned char* pp = p_skc->der.data;
         if (!pp) {
            /* no key or encoding present - give up */
            return 10;
         }
         skey = priDecode(ctx, p_skc->nid, &p_skc->der, encdec);
         if (!skey) {
             return 9;
         }
      }

      /* load a context with this key */
      skc = ICC_EVP_PKEY_CTX_new_from_pkey(ctx, NULL, skey, NULL);
      if (!skc) {
         return 2;
      }

      if (!p_skc->key) {
         /* the key is reference counted in the context so we can delete this temporary reference */
         ICC_EVP_PKEY_free(ctx, skey);
      }
   }

   rc = ICC_EVP_PKEY_decapsulate_init(ctx, NULL, NULL);
   if (rc != ICC_OSSL_SUCCESS) {
      return 1;
   }

   {
      /* peer's public key is just the ss encrypted (by peer) with our public key */
      size_t wrappedkeylen = 0;
      wrappedkeylen = p_pks->len;
      unsigned char* wrappedkey = p_pks->data;
      size_t genkeylen = 0;

      rc = ICC_EVP_PKEY_decapsulate(ctx, skc, NULL, &genkeylen, NULL, wrappedkeylen);
      if (rc != ICC_OSSL_SUCCESS) {
         return 2;
      }

      {
         kbuf gk;
         unsigned char* genkey;
         gk.len = genkeylen;
         genkey = gk.data = malloc(genkeylen);
         /*unsigned char* unwrapped, size_t* unwrappedlen, const unsigned char* wrapped, size_t wrappedlen */
         rc = ICC_EVP_PKEY_decapsulate(ctx, skc, genkey, &genkeylen, wrappedkey, wrappedkeylen);
         if (rc != ICC_OSSL_SUCCESS) {
            return 3;
         }

         *ss = gk;
      }
   }

   if (!p_skc->ctx) {
      ICC_EVP_PKEY_CTX_free(ctx, skc);
   }
   return 0;
}

/* Signature test function - generates keys, signs a message, and verifies the signature */
/* return 0 for success */
int
PQC_KEM_test(ICC_CTX* ctx, const char* algname, int verbose, int argc, const char* argv[], enum ed encdec)
{
   FILE* fp_rsp = stdout;
   int ret_val;

   {
      pkbuf pk = {0}; /*initialise these*/
      skbuf sk = {0};
      kbuf ct = {0};
      kbuf ss = {0}, ss1 = {0};

      if (verbose) {
         printf("PQC_KEM_test: Algorithm : %s)\n", algname);
      }
      /* Peer 1 generates the public/private keypair */
      if (verbose) {
         printf("keygen\n");
      }
      if ((ret_val = KEMEVP_gen(ctx, algname, &pk, &sk, encdec)) != 0) {
         printf("Error: KEMEVP_gen returned <%d>\n", ret_val);
         return 1;
      }
      if (verbose) {
         fprintf(fp_rsp, "pk encoding length = %d\n", (int)pk.der.len);
         fprintf(fp_rsp, "pk %s\n", (encdec & pkcs1) ? "pkcs1" : "raw");
         fprintBstr(fp_rsp, "", pk.der.data, pk.der.len);

         fprintf(fp_rsp, "sk encoding length = %d\n", (int)sk.der.len);
         if (encdec & (raw | pkcs8)) {
            fprintf(fp_rsp, "sk %s\n", (encdec & pkcs8) ? "pkcs8" : "raw");
            fprintBstr(fp_rsp, "", sk.der.data, sk.der.len);
         }
      }
      {
         int keylen = 0;
         keylen = ICC_EVP_PKEY_size(ctx, sk.key);
         if (verbose) {
            fprintf(fp_rsp, "key size = %d\n", keylen);
         }
      }

      /* get rid of gen context */
      if (sk.ctx) {
         ICC_EVP_PKEY_CTX_free(ctx, sk.ctx);
         sk.ctx = NULL;
      }

      if (encdec & (raw | pkcs8)) {
         /* delete ICC key and context so we use private encoding */
         if (sk.key) {
            ICC_EVP_PKEY_free(ctx, sk.key);
            sk.key = NULL;
         }
      }

      /* Typical usage: Send public key (pk) to peer 2, keep private/secret key (sk) */

      if (verbose) {
         printf("encrypt\t");
      }

      /* peer 2 generates shared secret (ss) and uses pk to encapsulate ss (ct) */
      if ((ret_val = KEMEVP_enc(ctx, &ct, &ss, &pk, argc, argv,  encdec)) != 0) {
         printf("kem encapsulate returned <%d>\n", ret_val);
         return 2;
      }
      fprintBstr(fp_rsp, "ct = ", ct.data, ct.len);
      fprintBstr(fp_rsp, "ss = ", ss.data, ss.len);
      /* send ct to peer 1 */

      /* peer 1 uses sk to decapsulate (decrypt) ct to get ss */
      fprintf(fp_rsp, "\n");
      if (verbose) {
         printf("decrypt\n");
      }
      if ((ret_val = KEMEVP_dec(ctx, &sk, &ss1, &ct, encdec)) != 0) {
         printf("kem decapsulate returned <%d>\n", ret_val);
         return 3;
      }

      /* ss's should match */
      if (ss.len != ss1.len || memcmp(ss.data, ss1.data, ss.len)) {
         printf("kem decapsulate returned bad 'ss' value\n");
         fprintBstr(fp_rsp, "ss = ", ss1.data, ss1.len);
         return 4;
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
      }
      if (ct.data) {
         free(ct.data);
      }
      if (ss.data) free(ss.data);
      if (ss1.data) free(ss1.data);
   }
   return 0;
}


/* map command line arguments to ICC parameters */
/* these algorithm names are self explanitory but will come from icc.h */

static
char* algs[] =
{
   "Kyber_512",
   "ML-KEM-768",
   "Kyber_1024",
   NULL
};

/* Map command line arguments to Key Encapsulation algorithm names */
static
const char* to_KEM_ALGNAME(int k)
{
   /* k is 1 based so adjust for 0 based index */
   if (k > sizeof(algs) / sizeof(algs[0]))
      return NULL;
   return algs[k-1];
}

static
void fcb(const char* a, int b, int c)
{
   printf("KemTest fcb:%s, %d, %d\n", a, b, c);
}

static
void tcb(const char* val1, const char* val2)
{
   printf("KemTest Tcb:%s, %s \n", val1, val2);
}


/* Main function to parse arguments and execute KEM tests */
int main(int argc, const char* argv[])
{
   const char* algname = NULL; 
   const char* iccPath = NULL;
   bool isFips = false, wantFips = false, verbose = false;
   bool wantTraceCB = false; /* Trace callback */
   bool wantFipsCB = false; /* FIPS callback */
   enum ed encdec = none;
   int rv = 0;
   size_t iterations = 1;

   /* Parse command-line arguments */
   if (argc > 1) {
      const char* arg;
      int i;
      for ( i = 1; i < argc; i++) {
         arg = argv[i];
         if (NULL != strstr(arg, "-?")) {
            int j;
            printf("Usage: KemTest [-v] [-fips] [-fcb] [-tcb] [-alg <algorithm name>] [-ed <none|raw|pkcs[1|8]>] [<number (see list below)>]\n");
            printf("     -fips    Request FIPS mode ICC\n");
            printf("     -fcb     Install a FIPS callback routine (prints message 'fcb:...')\n");
            printf("     -tcb     Install a TRACE callback routine (prints message 'tcb:...')\n");
            printf("     -ed      Encode/Decode keys, pkcs=pkcs1|pkcs8\n");
            printf("     -alg     Refer following table...\n");
            for ( j = 1; to_KEM_ALGNAME(j); j++) {
               printf(" %d    %s\n", j, to_KEM_ALGNAME(j));
            }
         }
         else if (NULL != strstr(arg, "-fips")) {
            wantFips = true;
         }
         else if (NULL != strstr(arg, "-fcb")) {
            wantFips = true;
            wantFipsCB = true;
         }
         else if (NULL != strstr(arg, "-tcb")) {
            wantTraceCB = true;
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
         else if (NULL != strstr(arg, "-i")) {
            i++;
            iterations = atoi(argv[i]);
         }
         else if (*arg == '-') {
            /* another setting - pass it on */
            i++;
         }
         else {
            int k = 0;
            k = (int)atoi(arg);
            algname = to_KEM_ALGNAME(k);
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
       char iccversion[ICC_VALUESIZE + 1];
       iccversion[ICC_VALUESIZE] = '\0';

       /* Initialize ICC context */
       icc_ctx = ICC_Init(&status, iccPath);
       if (NULL == icc_ctx) {
           printf("ICC not initialized, exiting\n");
           if (iccPath)
              printf("icc path was: %s\n", iccPath);
           exit(1);
       }

       /* check and report the ICC step version we found (before attach) */
       {
          iccversion[0] = '\0';
          if (ICC_ERROR == ICC_GetValue(icc_ctx, &status, ICC_VERSION, (void*)iccversion, ICC_VALUESIZE)) {
             printf("ICC_GetValue() failed, exiting\n");
             ICC_Cleanup(icc_ctx, &status);
             exit(1);
          }
          iccversion[ICC_VALUESIZE] = '\0';
          printf("ICC_Version step: %s\n", iccversion);
       }

       /*
       * Set FIPS only work before the attach
       */
       if (wantFips) {
           isFips = wantFips;
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

       /* check and report the ICC module version we found */
       {
          iccversion[0] = '\0';
          if (ICC_ERROR == ICC_GetValue(icc_ctx, &status, ICC_VERSION, (void*)iccversion, ICC_VALUESIZE)) {
             printf("ICC_GetValue() failed, exiting\n");
             ICC_Cleanup(icc_ctx, &status);
             exit(1);
          }
          iccversion[ICC_VALUESIZE] = '\0';
          printf("ICC_Version module: %s\n", iccversion);
          //if (strstr(iccversion, "8.6") != NULL) {
          //   printf("Error: %s\n", "ICC 8.6 not supported");
          //   ICC_Cleanup(icc_ctx, &status);
          //   exit(1);
          //}
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

       if (!algname) {
           /* default */
           algname = to_KEM_ALGNAME(2); /* Kyber_768 */
           printf("algname = %s\n", algname?algname:"NULL");
       }
       /* Execute the KemTest test */
       {
          size_t iteration;
          for (iteration = 0; iteration < iterations; iteration++) {
             rv = PQC_KEM_test(icc_ctx, algname, verbose, argc - 1, argv + 1, encdec);
             if (iterations > 1 && verbose) {
                printf("iteration %d\n", (int)iteration);
             }
             if (rv) {
                if (iterations > 1 && !verbose) {
                   printf("iteration %d\n", (int)iteration);
                }
                printf("%s: Error %d, try -? to get help\n", algname, rv);
             }
          }
       }

       /* Clean up ICC context */
       ICC_Cleanup(icc_ctx, &status);
   }

   return rv;
}
