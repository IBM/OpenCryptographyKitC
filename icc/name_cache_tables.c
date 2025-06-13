/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/* Common code for the name/nid caches
   name cache is used for performance, NID cache for the FIPS algorithm callback
   defines, typedefs, data
   This is used differently at different levels
*/    
#if defined(_WIN32) && !defined(strcasecmp)
#  define strcasecmp(a,b) _stricmp(a,b)
#endif


/* Note we split the name cache as lookup time
   is proportional to some polynomal(entries)
  even when using sorted lists and binary search
  Also note we only cache "important" algorithms
  for the same reason.

  Note the block flag, as yet unused. That's a 'just in case' we are
  told to blacklist weak algorithms at some point.

  Ideally whatever would be done down in the lower level code but
  it gets expensive touching the FIPS code, so we make life difficult
  for ourselves and do this in the step library instead.

  That does have the dubious advantage of avoiding a couple of calls & if's as
  well and may make the cache more effective.

*/

#if defined(ICCPKG)
typedef struct {
    const char *name;
    const ICC_EVP_MD *md;
    int block; /* Blacklisted */
    int fips;  /* FIPS allowed alg */
    int nid;   /* NID */
} MD_CACHE;

typedef struct {
    const char *name;
    const ICC_EVP_CIPHER *cip;
    int block; /* Blacklisted */
    int fips;  /* FIPS allowed alg */
    int nid;   /* NID */
} CIP_CACHE;
#else
typedef struct {
    const char *name;
    const EVP_MD *md;
    int block; /* Blacklisted */
    int fips;  /* FIPS allowed alg */
    int nid;   /* NID */
} MD_CACHE;

typedef struct {
    const char *name;
    const EVP_CIPHER *cip;
    int block; /* Blacklisted */
    int fips;  /* FIPS allowed alg */
    int nid;   /* NID */
} CIP_CACHE;
#endif

#if defined(HAVE_C_ICC)
/* FIPS digests and ones important enough to get a fast path lookup. 
  Any other digest will come up as non-FIPS */
static MD_CACHE C_diglist[] = {
    {"SHA1",NULL,0,0},
    {"SHA256",NULL,0,1},
    {"SHA224",NULL,0,1},
    {"SHA384",NULL,0,1},
    {"SHA512",NULL,0,1},
    {"SHA512-224",NULL,0,1},
    {"SHA512-256",NULL,0,1},
    {"SHA3-224",NULL,0,1},
    {"SHA3-256",NULL,0,1},
    {"SHA3-384",NULL,0,1},
    {"SHA3-512",NULL,0,1},
    {"SHAKE128",NULL,0,1},
    {"SHAKE256",NULL,0,1},
    {"MD5",NULL,0,0}
    };
/* FIPS algs and ones consdered important enough to get a fast path lookup 
  Any other cipher will come up as non-FIPS
*/
static CIP_CACHE C_ciplist[] = {
    {"AES-128-ECB",NULL,0,1},
    {"AES-128-CBC",NULL,0,1},
    {"AES-128-GCM",NULL,0,1},
    {"AES-128-CCM",NULL,0,1},
    {"AES-128-CTR",NULL,0,1},
    {"AES-128-XTS",NULL,0,1},
    {"AES-128-CFB1",NULL,0,1},
    {"AES-128-CFB8",NULL,0,1},
    {"AES-128-CFB",NULL,0,1},
    {"AES-128-OFB",NULL,0,1},    
    {"AES-192-ECB",NULL,0,1},
    {"AES-192-CBC",NULL,0,1},
    {"AES-192-CFB1",NULL,0,1},
    {"AES-192-CFB8",NULL,0,1},
    {"AES-192-CFB",NULL,0,1},
    {"AES-192-OFB",NULL,0,1},
    {"AES-192-GCM",NULL,0,1},
    {"AES-192-CCM",NULL,0,1},
    {"AES-192-CTR",NULL,0,1},
    {"AES-256-ECB",NULL,0,1},
    {"AES-256-CBC",NULL,0,1},  
    {"AES-256-GCM",NULL,0,1},
    {"AES-256-CCM",NULL,0,1},
    {"AES-256-CTR",NULL,0,1},
    {"AES-256-XTS",NULL,0,1},
    {"AES-256-CFB1",NULL,0,1},
    {"AES-256-CFB8",NULL,0,1},
    {"AES-256-OFB",NULL,0,1}, 
    {"AES-256-CFB",NULL,0,1},
    {"id-aes128-wrap",NULL,0,1},
    {"id-aes192-wrap",NULL,0,1},
    {"id-aes256-wrap",NULL,0,1},
    {"id-aes128-wrap-pad",NULL,0,1},
    {"id-aes192-wrap-pad",NULL,0,1},
    {"id-aes256-wrap-pad",NULL,0,1},
    {"DES-EDE3-CBC",NULL,0,0},
    {"CHACHA20-POLY1305",NULL,0,0},
    {"RC4",NULL,0,0},
    };
 #endif
 #if defined(HAVE_N_ICC)   
/* Digests considered important enough to get a fast path lookup */
static MD_CACHE N_diglist[] = {
    {"SHA1",NULL,0,0},
    {"SHA256",NULL,0,1},
    {"SHA224",NULL,0,1},
    {"SHA384",NULL,0,1},
    {"SHA512",NULL,0,1},
    {"SHA512-224",NULL,0,1},
    {"SHA512-256",NULL,0,1},
    {"SHA3-224",NULL,0,1},
    {"SHA3-256",NULL,0,1},
    {"SHA3-384",NULL,0,1},
    {"SHA3-512",NULL,0,1},
    {"SHAKE128",NULL,0,1},
    {"SHAKE256",NULL,0,1},
    {"MD5",NULL,0,0}
    };
/* Ciphers consdered important enough to get a fast path lookup */
static CIP_CACHE N_ciplist[] = {
    {"AES-128-ECB",NULL,0,1},
    {"AES-128-CBC",NULL,0,1},
    {"AES-128-GCM",NULL,0,1},
    {"AES-128-CCM",NULL,0,1},
    {"AES-128-CTR",NULL,0,1},
    {"AES-128-XTS",NULL,0,1},
    {"AES-128-CFB1",NULL,0,1},
    {"AES-128-CFB8",NULL,0,1},
    {"AES-128-CFB",NULL,0,1},
    {"AES-128-OFB",NULL,0,1},    
    {"AES-192-ECB",NULL,0,1},
    {"AES-192-CBC",NULL,0,1},
    {"AES-192-CFB1",NULL,0,1},
    {"AES-192-CFB8",NULL,0,1},
    {"AES-192-CFB",NULL,0,1},
    {"AES-192-OFB",NULL,0,1},
    {"AES-192-GCM",NULL,0,1},
    {"AES-192-CCM",NULL,0,1},
    {"AES-192-CTR",NULL,0,1},
    {"AES-256-ECB",NULL,0,1},
    {"AES-256-CBC",NULL,0,1},  
    {"AES-256-GCM",NULL,0,1},
    {"AES-256-CCM",NULL,0,1},
    {"AES-256-CTR",NULL,0,1},
    {"AES-256-XTS",NULL,0,1},
    {"AES-256-CFB1",NULL,0,1},
    {"AES-256-CFB8",NULL,0,1},
    {"AES-256-CFB",NULL,0,1},
    {"AES-256-OFB",NULL,0,1},    
    {"DES-EDE3-CBC",NULL,0,0},
    {"CHACHA20-POLY1305",NULL,0,0},
    {"RC4",NULL,0,0},
    };

#endif
