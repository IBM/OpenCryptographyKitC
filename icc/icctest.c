/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Unit test for ICC
//
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(JGSK_WRAP)
#  include "jcc_a.h"
#endif
#include "icc.h"

/* Consider using the --tool=massif stacks=yes option to Valgrind instead
#define STACK_DEBUG
#define STACK_DEBUG_DEEP
*/
/*
#define MEMTRACE
*/
/* 
   This code provides leak detection facilities for the main ICC
   test executable.
   It's special purpose code and relies on OpenSSL/ICC memory callbacks
   passing in file/line# info when malloc is called

   We allocate a trace buffer large enough to hold the deepest allocation sequence
   add allocated blocks to that when malloc() requests are made
   and remove them again when free() requests for block are made
   Anything left at program exit is a leak ...

   Valgrind is better, but this will find leaks in the ICC code very directly

*/

/* Prototypes for ICC memory callback functions compatible with OpenSSL
 */
void *TestMalloc(int sz,const char *file,int line);
void *TestRealloc(void *ptr,int sz,char *file, int line);
void TestFree(void *ptr);
static unsigned long nallocs;
static size_t allocsz;
static unsigned long nreallocs;
static unsigned long nfrees;

#define N_RNGS 9




/* Routine to report any unfree'd buffers */

void x_memdump();

void clean(void **marray, int n) 
{
  int i;
  for(i = 0 ; i < n ; i++) {
    if(NULL != marray[i] ) {
      TestFree(marray[i]);
      marray[i] = NULL;
    }
  }
}


void x_free(void **marray,int i) 
{
  TestFree(marray[i]);
  marray[i] = 0;
}


/*!
  Code to track memory leaks
  Keep a buffer of allocated entries, remove them
  when we free() them
  Warn and make the buffer bigger next time if we run out of slots
  this is special purpose code.
*/
#define MEM_T_SZ 4096

#if defined(MEMCHK_ON)


typedef struct {
  void *ptr;
  char *file;
  int line;
  size_t size;
  int alloc_status; /* Increment when allocated, decrement on free */
} MEMTRACK;

static MEMTRACK memt[MEM_T_SZ];
int last_slot = 0;
int max_slot = 0;
/*! @brief compare code for the memory lookup array searches
  @param p1 is an array entry
  @param p2 is the passed parameter
  @return 0 if the pointers are equal, 1 if p2 > p1, -1 otherwise
  \DEBUG Debug code omitted from production builds
 */
int x_memcmp(void *p1, void *p2)
{
  int rv = -1;
  if(p1 == p2) { 
    rv = 0;
  } else if(p2 > p1) {
    return 1;
  }
  return rv;
}
/*! @brief add an entry to the array
  @param ptr the newly allocated pointer
  @param the file the allocation occured in
  @param the line #
  \DEBUG Debug code omitted from production builds
*/
void x_memadd(void *ptr,char *file, int line,size_t size)
{
  int i = 0;
  int r = 0;
  /* Finding the correct slot could be optimized */
  for(i = 0; i <= last_slot; i++) {
    r = x_memcmp(memt[i].ptr,ptr);
    if( 0 == r) {
      fprintf(stdout,"\nDuplicated alloc orig file %s, line %d,new file %s, line %d\n",memt[i].file,memt[i].line,file,line);
      break;
    } else if (1 == r) {
      if(i != last_slot) {
        memmove(&memt[i+1],&memt[i],sizeof(MEMTRACK) * (last_slot-i));
      }
      memt[i].ptr = ptr;
      memt[i].file = file;
      memt[i].line = line;
      memt[i].size = size;
      last_slot++;
      if(last_slot> max_slot) {
        max_slot++;
      }
      if(last_slot >= MEM_T_SZ) {
        fprintf(stdout,"\nToo many slots used, aborting\n");
        exit(1);
      } 
      break;
    }
  }
}
/*! @brief remove an entry from the array
  @param ptr the entry to remove
  \DEBUG Debug code omitted from production builds
*/
void x_memdel(void *ptr) 
{
  int i = 0;
  int r = 0;
   /* Finding the correct slot could be optimized */
  for(i = 0; i < last_slot; i++) {
    r = x_memcmp(memt[i].ptr,ptr);
    if( 0 == r) {
      if(i != last_slot) {
        memmove(&memt[i],&memt[i+1],sizeof(MEMTRACK) * (last_slot-i));
      }
      memt[last_slot].ptr = NULL;
      memt[last_slot].file = NULL;
      memt[last_slot].line = 0;
      memt[last_slot].size = 0;
      last_slot--;
    }
  }
  if(last_slot <0 ) {
    fprintf(stdout,"\nUnderrun, aborting\n");
    exit(1);
  }
}
/*! @brief Dump any residual information
  and, for convenience in testing, reset the tracking array
  \DEBUG Debug code omitted from production builds
*/
void x_memdump() {
  int i;
  fprintf(stdout,"\nChecking for non-free'd allocated memory\n  (%d slots used in tracking)\n",max_slot);
  if(last_slot > 0) {
    for( i = 0; i < last_slot ; i++) {
      printf("File %s, line %d size %lu\n",memt[i].file,memt[i].line,(unsigned long)memt[i].size);
    }
  }
  memset(memt,0,sizeof(memt));
  last_slot = 0;
  max_slot = 0;
}
#else
/*!
  @brief production code memory leak debug reporting code
  does nothing when disabled.
*/
void x_memdump()
{
}
#endif
/*!
  @brief OpenSSL compatable malloc callback
  @param sz request size
  @param file file the request came from
  @param line line # of the request call
  @return a pointer to a newly allocated buffer or NULL
*/
void *TestMalloc(int sz,const char *file,int line)
{
  void *rv = NULL;
  nallocs ++;
  allocsz += sz;

#if 0 && defined(_AIX) 
  if( 0 != posix_memalign(&rv,16,sz)) {
    rv = NULL;
  }
#else
  rv = malloc(sz);
#endif
#if defined(MEMCHK_ON)
  if((NULL != rv) && (0 != sz)) {
    x_memadd(rv,(char *)file,line,sz);
  }
#endif
#if defined(MEMTRACE)
  fprintf(stderr,"\nmalloc %d %0llx %s %d\n",sz,(unsigned long long)rv,file,line);
#endif
  return rv;
}

void *TestRealloc(void *ptr,int sz,char *file, int line)
{
  void *rv = NULL;
  nreallocs ++;
#if defined(MEMCHK_ON)
  x_memdel(ptr);
#endif
  rv = realloc(ptr,sz);
#if defined(MEMCHK_ON)
  if((NULL != rv) && (0 != sz)) {
    x_memadd(rv,file,line,sz);
  }
#endif
#if defined(MEMTRACE)
  fprintf(stderr,"\nrealloc %0llx %d %0llx %s %d\n",(unsigned long long)ptr,sz,(unsigned long long)rv,file,line);
#endif  
  return rv;
}

void TestFree(void *ptr)
{
  nfrees ++;
#if defined(MEMTRACE)
  fprintf(stderr,"\nfree %0llx\n",(unsigned long long)ptr);
#endif  
#if defined(MEMCHK_ON)
  x_memdel(ptr);
#endif
  free(ptr);
}


#if defined(_WIN32)
#define strcasecmp(a,b) _stricmp(a,b)

#endif
typedef struct {
  const char *alg;
  const int keylen;
} KDF_TEST;

float version = 0.0; /* Used to compare ICC version numbers */
char env_trng[256] = ""; /* Set if ICC_TRNG is set in the environment */
int entropy1 = 0;
int entropy2 = 0;

int is_unicode = 0; /*<! contains unicode init state */

/* Assume we never use > 50k of stack */
#define CHECK_RANGE 50000
#define MARKER 0xa5

/*! 
  @brief Change the sizes of some keys used in testing 
  We need to test that small keys do work in non-FIPS mode
  for interop.
*/
static int fips_mode = 0;

/*!
  @brief If set ICC_INDUCED_FAILUE is set after ICC_ATTACH
  - proves these code paths work
  - allows us to check some failure points for post-startup
  failures (add 1000 to the normal failure value)
*/  
static unsigned int allow_at_runtime = 0;

/*! 
  @brief The induced failure to trip if set
*/
static unsigned int icc_failure = 0;
/*! @brief prototype for a pointer to OpenSSL style malloc() 
  The last two parameters are expected to be __FILE__, __LINE__
*/
typedef void *(*MallocFunc) (int, const char *,int);

/*! @brief prototype for a pointer to realloc() used by OpenSSL*/
typedef void *(*ReallocFunc) (void *,int, const char *, int);

/*! @brief prototype for a pointer to free() used by OpenSSL */
typedef void (*FreeFunc) (void *);


/* 
   ICC isn't threaded when we use these, so we create these scratch buffers
   once as statics
*/

static  unsigned char buf1[4096];
static  unsigned char buf2[4096];

static  int tuner = 0; /*! RNG tuning algorithm, 0 = unset, 1 = heuristic, 2 = estimate */
static int exclude = 0;
/*
  uncomment to turn on fine grained stack checks so that you can 
  find the deep spots
  #define STACK_DEBUG
*/

/**
   @brief crude stack use checker.
   @param z if z != 0 then zero the check buffer only
   @note this code is very likely architecture dependent !!!!
*/
#if defined(STACK_DEBUG)

static int max_stack = 0;
void check_stack(int z)
{
  static int first = 1;
  unsigned char zero[CHECK_RANGE];
  int i = 0;
  if( first ) {
    first = 0;
  } else if( 0 != z) {
    for( i = 0; i < CHECK_RANGE ; i++ ) {
      if(MARKER != zero[i]) { /* i will be the number of untouched bytes */
	break;
      }
    }
    i = CHECK_RANGE - i;
    if( i > max_stack) max_stack = i;
#if defined(STACK_DEBUG_DEEP)
    printf("Stack used %d, max %d\n",i,max_stack);
#endif
  }
  memset(zero,MARKER,CHECK_RANGE);
}
#else
#define check_stack(z)
#endif

/* 
   Used to track errors resulting from OpenSSL API calls 
*/
#define OSSLE(x) OpenSSLError(x,__LINE__);

int OpenSSLError(ICC_CTX *ICC_ctx,const int line)
{

  unsigned long retcode = 0;
  retcode = ICC_ERR_get_error(ICC_ctx);
  /* While because we want to drain the swamp */
  while(0 != retcode && ((unsigned long)-2L) != retcode) { 
    ICC_ERR_error_string(ICC_ctx,retcode,(char *)buf1);
    printf("OpenSSL error line %d [%s]\n",line,buf1);
    retcode = ICC_ERR_get_error(ICC_ctx);
  }
  return retcode;
}
   
int check_status( ICC_STATUS *status, const char *file, int line )                
{                              
  const char *sev = "UNKNOWN ERROR TYPE";
  int rv = ICC_OK;

  if((status->majRC) != ICC_OK) {
    switch(status->majRC) {
    case ICC_ERROR:
      sev = "ICC_ERROR";
      break;
    case ICC_WARNING:
      sev = "ICC_WARNING";  
      break;
    case ICC_FAILURE:
      sev = "ICC_FAILURE";  
      break;
    case ICC_OPENSSL_ERROR:
      sev = "ICC_OPENSSL_ERROR";
      break;
    case ICC_OS_ERROR:
      sev = "ICC_OS_ERROR";
      break;
    default:
      rv = ICC_ERROR;
      break;
    }
    switch(status->majRC) {
    case ICC_ERROR:
    case ICC_FAILURE:
    case ICC_WARNING:
    case ICC_OPENSSL_ERROR:
    case ICC_OS_ERROR:  
      printf("Line %d: Status Check (%s): majRC: %d minRC: %d Error string: %s\n",line, sev,status->majRC, status->minRC , status->desc);
      rv = status->majRC;
      break;
    default:
      printf("Line %d: Status Check (%s): majRC: %d minRC: %d  \"Something bad happened\"\n",line,sev,status->majRC,status->minRC);
      break;
    }
  }                      
  return rv;
}

/*! @brief print the state of a context, FIPS, errors, 
    TRNG, PRNG used
    @param ctx an initialized ICC context
    @param prefix padding so the print can be offset, printed every line
*/
static void print_cfg(ICC_CTX *ctx, char *prefix)
{
  static char *buf; 
  ICC_STATUS *stat;
  
  buf = calloc(1,ICC_VALUESIZE);
  stat = calloc(1,sizeof(ICC_STATUS));
  if((NULL != buf) && (NULL != stat)) {
    buf[0] = 0;
    if(NULL == prefix) prefix = "";

    ICC_GetValue(ctx,stat,ICC_SEED_GENERATOR,buf,20);
    printf("%sFIPS %s\n",prefix,(stat->mode & ICC_FIPS_FLAG) ? "Yes":"No");
    printf("%sERR  %s\n",prefix,(stat->mode & ICC_ERROR_FLAG) ? "Yes":"No");

    printf("%sTRNG %s\n",prefix,buf);
    ICC_GetValue(ctx,stat,ICC_RANDOM_GENERATOR,buf,20);
    printf("%sPRNG %s\n",prefix,buf); 
  }
  free(stat);
  free(buf);
}

/*!
  @brief convert the string ICC version to a number
  that can be used in comparisons
  @param vin The input string
  @return a float version of the number i.e. 1.405 1.0902030
  @note
  We allow a 100 range between digits so "1.4.5.0" -> 1.0405000
  NOT 1.450
*/
float VString2(char *vin)
{
  double rv = 0.0;
  double mult = 0.01;
  rv = (double)atoi(vin); /* major - left of '.' */
  for( ; *vin ; vin++) {
    while(*vin && *vin != '.') vin++;
    if(*vin) vin++;
    rv = rv + (atoi(vin) * mult);
    mult *= 0.01;
  }
  return (float)rv;
}

int doEVPDigestUnitTest(ICC_CTX *ICC_ctx)
{
  int rv = ICC_OSSL_SUCCESS;
  int retcode = 0;
  ICC_EVP_MD_CTX *md_ctx = NULL;
  ICC_EVP_MD_CTX *md_ctx2 = NULL;
  const ICC_EVP_MD *md = NULL;

  printf("Starting EVP Digest unit test...\n");
	
  check_stack(0);

  md_ctx = ICC_EVP_MD_CTX_new(ICC_ctx);
  md_ctx2 = ICC_EVP_MD_CTX_new(ICC_ctx);
  if( (NULL == md_ctx) || (NULL == md_ctx2) ) {
    printf("EVP Digest test aborted, could not allocate digest structures!\n");
    rv = ICC_ERROR;
  } else {
    md = ICC_EVP_get_digestbyname(ICC_ctx,"SHA1");
    ICC_EVP_DigestInit(ICC_ctx,md_ctx,md);
	
    retcode = ICC_EVP_MD_CTX_copy(ICC_ctx,md_ctx2,md_ctx);
    ICC_EVP_DigestUpdate(ICC_ctx,md_ctx,buf1,20);
    md = ICC_EVP_MD_CTX_md(ICC_ctx,md_ctx);
    ICC_EVP_DigestFinal(ICC_ctx,md_ctx,buf1,NULL);



    ICC_EVP_DigestUpdate(ICC_ctx,md_ctx2,buf1,20);
    ICC_EVP_DigestFinal(ICC_ctx,md_ctx2,buf1,NULL);

    retcode = ICC_EVP_MD_type(ICC_ctx,md);
    
    ICC_OBJ_nid2sn(ICC_ctx,retcode);
    
    retcode = ICC_EVP_MD_size(ICC_ctx,md);
    retcode = ICC_EVP_MD_block_size(ICC_ctx,md);
    
    ICC_EVP_MD_CTX_cleanup(ICC_ctx,md_ctx2);
    ICC_EVP_MD_CTX_free(ICC_ctx,md_ctx2);
    ICC_EVP_MD_CTX_cleanup(ICC_ctx,md_ctx); 
#if 0
   /* SHAKE/XOF */
 
    md = ICC_EVP_get_digestbyname(ICC_ctx,"SHAKE128");
    ICC_EVP_DigestInit(ICC_ctx,md_ctx,md);
    ICC_EVP_DigestUpdate(ICC_ctx,md_ctx,buf1,20);
    ICC_EVP_DigestFinalXOF(ICC_ctx,md_ctx,buf1,65);
#endif

 
    ICC_EVP_MD_CTX_cleanup(ICC_ctx,md_ctx); 
    ICC_EVP_MD_CTX_free(ICC_ctx,md_ctx);
    check_stack(1);
    printf("EVP Digest Unit test successfully completed!\n");
  }
  return rv;
}

int doEVPCipherUnitTest(ICC_CTX *ICC_ctx)
{
  int rv = ICC_OSSL_SUCCESS;
  int retcode = 0;
  ICC_EVP_CIPHER_CTX *cipher_ctx = NULL;
  const ICC_EVP_CIPHER *cipher = NULL;
  unsigned char key[128];
  unsigned char iv[64];
  int int1 = 0;
  int i = 0;
  int key_bits = 64;
	
  memset(key,1,128);
  memset(iv,0,64);
  memset(buf1,0,128);
  memset(buf2,0,128);
	
  printf("Starting EVP Cipher unit test...\n");
  check_stack(0);

  cipher_ctx = ICC_EVP_CIPHER_CTX_new(ICC_ctx);
  if( NULL == cipher_ctx) {
    printf("EVP Cipher test aborting, could not allocate cipher context!\n");
    rv = ICC_ERROR;
  } else {
    cipher = ICC_EVP_get_cipherbyname(ICC_ctx,"AES128");

    retcode = ICC_EVP_CIPHER_type(ICC_ctx,cipher);
  
    ICC_EVP_CIPHER_CTX_init(ICC_ctx,cipher_ctx);

    retcode = ICC_EVP_EncryptInit(ICC_ctx,cipher_ctx,cipher,key,iv);
    retcode = ICC_EVP_EncryptUpdate(ICC_ctx,cipher_ctx,buf2,&i,buf1,64);
    retcode = ICC_EVP_EncryptFinal(ICC_ctx,cipher_ctx,buf2+i,&int1);
    i += int1;
    OSSLE(ICC_ctx);
    retcode = ICC_EVP_CIPHER_CTX_cleanup(ICC_ctx,cipher_ctx);
    retcode = ICC_EVP_CIPHER_CTX_free(ICC_ctx,cipher_ctx);

    cipher_ctx = ICC_EVP_CIPHER_CTX_new(ICC_ctx);

    retcode = ICC_EVP_DecryptInit(ICC_ctx,cipher_ctx,cipher,key,iv);
    retcode = ICC_EVP_DecryptUpdate(ICC_ctx,cipher_ctx,buf1,&int1,buf2,i);
    retcode = ICC_EVP_DecryptFinal(ICC_ctx,cipher_ctx,buf1+int1,&int1);	
    OSSLE(ICC_ctx);
    retcode = ICC_EVP_CIPHER_CTX_cleanup(ICC_ctx,cipher_ctx);
    retcode = ICC_EVP_CIPHER_CTX_free(ICC_ctx,cipher_ctx);
    /********** Now the stuff which may leak, but which gives us API coverage  ********/
    cipher_ctx = ICC_EVP_CIPHER_CTX_new(ICC_ctx);

    cipher = ICC_EVP_get_cipherbyname(ICC_ctx,"RC4");

    ICC_EVP_CIPHER_CTX_init(ICC_ctx,cipher_ctx);
    retcode = ICC_EVP_EncryptInit(ICC_ctx,cipher_ctx,cipher,NULL,NULL);
    retcode = ICC_EVP_CIPHER_CTX_set_key_length(ICC_ctx,cipher_ctx,128);
    retcode = ICC_EVP_CIPHER_CTX_set_padding(ICC_ctx,cipher_ctx,0);
    retcode = ICC_EVP_EncryptInit(ICC_ctx,cipher_ctx,cipher,key,NULL);
    OSSLE(ICC_ctx);

    retcode = ICC_EVP_CIPHER_block_size(ICC_ctx,cipher);
    retcode = ICC_EVP_CIPHER_key_length(ICC_ctx,cipher);
    retcode = ICC_EVP_CIPHER_iv_length(ICC_ctx,cipher);
    retcode = ICC_EVP_EncryptUpdate(ICC_ctx,cipher_ctx,buf1,&int1,buf2,64);
    retcode = ICC_EVP_EncryptFinal(ICC_ctx,cipher_ctx,buf1,&int1);

    retcode = ICC_EVP_CIPHER_CTX_cleanup(ICC_ctx,cipher_ctx);	
    retcode = ICC_EVP_CIPHER_CTX_free(ICC_ctx,cipher_ctx);
    OSSLE(ICC_ctx);
	
    /* 
       Now the 'fixup' function added for DB2 
    */
    cipher_ctx = ICC_EVP_CIPHER_CTX_new(ICC_ctx);
    cipher = ICC_EVP_get_cipherbyname(ICC_ctx,"RC2");
    ICC_EVP_CIPHER_CTX_init(ICC_ctx,cipher_ctx);
    retcode = ICC_EVP_EncryptInit(ICC_ctx,cipher_ctx,cipher,NULL,NULL);
    OSSLE(ICC_ctx);
    retcode = ICC_EVP_CIPHER_CTX_ctrl(ICC_ctx,cipher_ctx, ICC_EVP_CTRL_SET_RC2_KEY_BITS, key_bits, NULL);
    retcode = ICC_EVP_CIPHER_CTX_ctrl(ICC_ctx,cipher_ctx, ICC_EVP_CTRL_GET_RC2_KEY_BITS, 0, &key_bits);
	
    retcode = ICC_EVP_EncryptInit(ICC_ctx,cipher_ctx,cipher,key,NULL);
    retcode = ICC_EVP_EncryptUpdate(ICC_ctx,cipher_ctx,buf1,&int1,buf2,128);
    retcode = ICC_EVP_EncryptFinal(ICC_ctx,cipher_ctx,buf1,&int1);
    OSSLE(ICC_ctx);
    ICC_EVP_CIPHER_CTX_cipher(ICC_ctx,cipher_ctx);

    retcode = ICC_EVP_CIPHER_CTX_cleanup(ICC_ctx,cipher_ctx);
    /* Void return */
    ICC_EVP_CIPHER_CTX_free(ICC_ctx,cipher_ctx);
    if(retcode != 1) {
      rv = retcode;
    }
	
    check_stack(1);


    printf("EVP Cipher Unit test successfully completed!\n");	
  }
      
  return rv;
}

int doEVPEnvelopeAndSignatureUnitTest(ICC_CTX *ICC_ctx)
{

  int retcode = 0;
  ICC_STATUS *status = NULL;
  ICC_RSA *rsa = NULL;
  ICC_EVP_PKEY *pkey[2] = {NULL,NULL};
  ICC_EVP_PKEY_CTX *pctx = NULL;
  ICC_EVP_MD_CTX *md_ctx = NULL;
  const ICC_EVP_MD *md = NULL;
  ICC_EVP_CIPHER_CTX *cipher_ctx = NULL;
  const ICC_EVP_CIPHER *cipher = NULL;
  unsigned char key[512];	
  unsigned char iv[64];	
  unsigned char *keyp[2] = {NULL,NULL};
  int keylen = 2048;
  int int1 = 0;
  unsigned int uint1 = 0;
  size_t uint2 = 0;
  int ekeylen = 0;
  int i1 = 0;
  int rv = ICC_OSSL_SUCCESS;
  int nid = 0;

  memset(key,'1',64);
  memset(iv,'1',64);
  memset(buf1,0,sizeof(buf1));
  memset(buf2,0,sizeof(buf2));
	
  keyp[0] = key;

  printf("Starting EVP Envelope And Signature unit test...\n");

  check_stack(0);

  status = calloc(1,sizeof(ICC_STATUS));

  
  if(!fips_mode) {
    keylen = 512;
  }
  rsa = ICC_RSA_generate_key(ICC_ctx,keylen,0x10001,NULL,NULL);
  if( (NULL == rsa) || (rv != ICC_OSSL_SUCCESS) ) {
    printf("EVP Envelope And Signature abort. Could not create RSA key.\n");
    rv = ICC_ERROR;
  } else {
    printf("\tRSA key size %d\n",keylen);
    check_stack(1);
	                        
    md_ctx = ICC_EVP_MD_CTX_new(ICC_ctx);
    check_stack(1);
    ICC_EVP_MD_CTX_init(ICC_ctx,md_ctx);
    check_stack(1);
    md = ICC_EVP_get_digestbyname(ICC_ctx,"SHA1");

    check_stack(1);
    cipher_ctx = ICC_EVP_CIPHER_CTX_new(ICC_ctx);
    ICC_EVP_CIPHER_CTX_init(ICC_ctx,cipher_ctx);
    cipher = ICC_EVP_get_cipherbyname(ICC_ctx,"AES-128-CBC");
    check_stack(1);
    OSSLE(ICC_ctx);
    pkey[0] = ICC_EVP_PKEY_new(ICC_ctx);
    ICC_EVP_PKEY_set1_RSA(ICC_ctx,pkey[0],rsa);
    check_stack(1);
    retcode = ICC_EVP_SealInit(ICC_ctx,cipher_ctx,cipher,keyp,&ekeylen,iv,pkey,1);
    OSSLE(ICC_ctx);   
    retcode = ICC_EVP_SealUpdate(ICC_ctx,cipher_ctx,buf2,&int1,buf1,20);
    i1 = int1;
    retcode = ICC_EVP_SealFinal(ICC_ctx,cipher_ctx,buf2+int1,&int1);
    i1 += int1;
    ICC_EVP_CIPHER_CTX_cleanup(ICC_ctx,cipher_ctx);
    OSSLE(ICC_ctx);
    check_stack(1);
    retcode = ICC_EVP_OpenInit(ICC_ctx,cipher_ctx,cipher,key,ekeylen,iv,pkey[0]);
    retcode = ICC_EVP_OpenUpdate(ICC_ctx,cipher_ctx,buf1,&int1,buf2,i1);
    retcode = ICC_EVP_OpenFinal(ICC_ctx,cipher_ctx,buf1+int1,&int1);
    OSSLE(ICC_ctx);
    check_stack(1);
    ICC_EVP_PKEY_id(ICC_ctx,pkey[0]);
    ICC_EVP_CIPHER_CTX_cleanup(ICC_ctx,cipher_ctx);
    retcode = ICC_EVP_CIPHER_CTX_free(ICC_ctx,cipher_ctx);
    check_stack(1);   
    ICC_EVP_SignInit(ICC_ctx,md_ctx,md);
    ICC_EVP_SignUpdate(ICC_ctx,md_ctx,NULL,0);
    retcode = ICC_EVP_SignFinal(ICC_ctx,md_ctx,buf1,(unsigned int *)&int1,pkey[0]);
    OSSLE(ICC_ctx);
    ICC_EVP_VerifyInit(ICC_ctx,md_ctx,md);
    ICC_EVP_VerifyUpdate(ICC_ctx,md_ctx,NULL,0);
    retcode = ICC_EVP_VerifyFinal(ICC_ctx,md_ctx,buf1,(unsigned int)int1,pkey[0]);
    OSSLE(ICC_ctx);
    ICC_EVP_MD_CTX_cleanup(ICC_ctx,md_ctx);
    check_stack(1);       

    OSSLE(ICC_ctx);

    check_stack(1);
    nid = ICC_OBJ_txt2nid(ICC_ctx,"SHA1");
    ICC_RSA_sign(ICC_ctx,nid,buf2,20,buf1,&uint1,rsa);
    ICC_RSA_verify(ICC_ctx,nid,buf2,20,buf1,uint1,rsa);
    check_stack(1);
    ICC_EVP_PKEY_free(ICC_ctx,pkey[0]);
    retcode = ICC_EVP_MD_CTX_cleanup(ICC_ctx,md_ctx);
    retcode = ICC_EVP_MD_CTX_free(ICC_ctx,md_ctx);


    check_stack(1);
    /* RSA PSS section */
    md_ctx = ICC_EVP_MD_CTX_new(ICC_ctx);
    pkey[0] = ICC_EVP_PKEY_new(ICC_ctx);
    ICC_EVP_PKEY_set1_RSA(ICC_ctx,pkey[0],rsa);
    printf("\tEVP_Digest[Sign/Verify] ");
    if(ICC_NOT_IMPLEMENTED != ICC_EVP_DigestSignInit(ICC_ctx,md_ctx,&pctx,md,NULL,pkey[0])) {
      (void)ICC_EVP_PKEY_CTX_get0_pkey(ICC_ctx,pctx); /* Just code coverage */
      ICC_EVP_PKEY_CTX_ctrl(ICC_ctx,pctx,ICC_EVP_PKEY_RSA,-1,ICC_EVP_PKEY_CTRL_RSA_PADDING,ICC_RSA_PKCS1_PSS_PADDING,NULL);
      ICC_EVP_SignUpdate(ICC_ctx,md_ctx,NULL,0);
      ICC_EVP_DigestSignFinal(ICC_ctx,md_ctx,NULL,&uint2);
      ICC_EVP_DigestSignFinal(ICC_ctx,md_ctx,buf2,&uint2);
      OSSLE(ICC_ctx);
      ICC_EVP_DigestVerifyInit(ICC_ctx,md_ctx,&pctx,md,NULL,pkey[0]);
      ICC_EVP_PKEY_CTX_ctrl(ICC_ctx,pctx,ICC_EVP_PKEY_RSA,-1,ICC_EVP_PKEY_CTRL_RSA_PADDING,ICC_RSA_PKCS1_PSS_PADDING,NULL);
      ICC_EVP_VerifyUpdate(ICC_ctx,md_ctx,NULL,0);
      ICC_EVP_DigestVerifyFinal(ICC_ctx,md_ctx,buf2,uint2);
      OSSLE(ICC_ctx);
      printf("\n");
    } else {
      printf("N/A\n");
    }

    ICC_EVP_MD_CTX_free(ICC_ctx,md_ctx);
    ICC_EVP_PKEY_free(ICC_ctx,pkey[0]);
    if(retcode != 1) {
      rv = retcode;
    }
    ICC_RSA_free(ICC_ctx,rsa);
    printf("EVP Envelope And Signature Unit test successfully completed!\n");
  }
  if(NULL != status) {
    free(status);
    status = NULL;
  }
  return rv;
}

int doEVPEncodeAndDecodeUnitTest(ICC_CTX *ICC_ctx)
{
  ICC_EVP_ENCODE_CTX *encode_ctx = NULL;	
  int int1 = 0;
  int int2 = 0;
  int temp = 0;
  int retcode = 0;

  check_stack(0);
  /* Avoid "uninitialized data" type wornings from purify/valgrind */
  memset(buf1,0,sizeof(buf1));
  memset(buf2,0,sizeof(buf2));

  printf("Starting EVP Encode And Decode unit test...\n");

  encode_ctx = ICC_EVP_ENCODE_CTX_new(ICC_ctx);

  ICC_EVP_EncodeInit(ICC_ctx,encode_ctx);
  ICC_EVP_EncodeUpdate(ICC_ctx,encode_ctx,buf2,&int1,buf1,16);

  ICC_EVP_EncodeFinal(ICC_ctx,encode_ctx,buf2+int1,&temp);
  temp += int1;
  retcode = ICC_EVP_ENCODE_CTX_free(ICC_ctx,encode_ctx);

  encode_ctx = ICC_EVP_ENCODE_CTX_new(ICC_ctx);
	
  ICC_EVP_DecodeInit(ICC_ctx,encode_ctx);
  retcode = ICC_EVP_DecodeUpdate(ICC_ctx,encode_ctx,buf2,&int2,buf1,temp);
  temp = 0;
  retcode = ICC_EVP_DecodeFinal(ICC_ctx,encode_ctx,buf2+int2,&temp);

  retcode = ICC_EVP_ENCODE_CTX_free(ICC_ctx,encode_ctx);
  OSSLE(ICC_ctx);
  check_stack(1);
  printf("EVP Encode And Decode Unit test successfully completed!\n");
 
  return retcode;
}

int doEVPUnitTest(ICC_CTX *ICC_ctx)
{
  int rv = ICC_OSSL_SUCCESS;
  int testnum;

  printf("Starting EVP unit test...\n");
  testnum = 1;
  while(testnum>0)
    {
      switch (testnum)
	{
		  
	case 1:
	  if(doEVPDigestUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
	    {
	      printf("EVP Digest unit test failed!\n");
	      testnum = -1;		
	    }
	  else testnum++;
	  break;
		  
	case 2:
	  if(doEVPCipherUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
	    {
	      printf("EVP Cipher unit test failed!\n");
	      testnum = -1;		
	    }
	  else testnum++;
	  break;
				
	case 3:
		  
	  if(doEVPEnvelopeAndSignatureUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
	    {
	      printf("EVP Envelope And Signature unit test failed!\n");
	      testnum = -1;		
	    }
	  else testnum++;
	  break;
				
	case 4:
	  if(doEVPEncodeAndDecodeUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
	    {
	      printf("EVP Encode And Decode unit test failed!\n");
	      testnum = -1;		
	    }
	  else testnum++;
	  break;
				
	default:
	  testnum = 0;
	  break;
	}
    }
  if (testnum == 0) printf("EVP Unit test successfully completed!\n");
  else /*error occurred*/
    {
      printf("Error occurred in EVP unit test!\n");
      rv = ICC_ERROR;
    }
  return rv;
}


int doRandUnitTest(ICC_CTX *ICC_ctx)
{
  int rv = ICC_OSSL_SUCCESS;
  int retcode;
  static unsigned char ZERO[8] = {0,0,0,0,0,0,0,0};
  unsigned char buf[8] = {0,0,0,0,0,0,0,0};

  printf("Starting Rand unit test...\n");
  check_stack(0);
  retcode = ICC_RAND_bytes(ICC_ctx,buf,sizeof(buf));
  if(version < 1.09 ) {
    if((retcode < 0) || (0 == memcmp(ZERO,buf,sizeof(ZERO)))) {
      rv = ICC_ERROR;
    }
  } else {
    if((retcode < 1) || (0 == memcmp(ZERO,buf,sizeof(ZERO)))) {
      rv = ICC_ERROR;
    }
  }
  ICC_RAND_seed(ICC_ctx,buf,1);
  check_stack(1);
  if( ICC_OSSL_SUCCESS == rv) {
    printf("Rand Unit test successfully completed!\n");
  }
  return rv;
}


int doCryptoUnitTest(ICC_CTX *ICC_ctx)
{
  printf("Starting Crypto unit test...\n");

  printf("Crypto Unit test successfully completed!\n");

  return ICC_OSSL_SUCCESS;
}

int doKeyUnitTest(ICC_CTX *ICC_ctx) {
  int retcode = 0;
  int rv = ICC_OSSL_SUCCESS;
  ICC_EVP_PKEY *pkey = NULL;
  ICC_RSA *rsa = NULL;
  ICC_DSA *dsa = NULL, *dsa1 = NULL;
  ICC_DH *dh = NULL;
  ICC_DH *dh1 = NULL;

  unsigned char *dhcvtbuf = NULL, *ptr = NULL;
  int blen = 0, len = 0;
  const unsigned char *tptr = NULL;
  unsigned char *tptr1 = NULL;
  static int intbuf1[1000];
  static unsigned char bufbn[1000];
  int keylen = 512;
  ICC_BIGNUM *bn = NULL;
  int counter = 0;
  int i = 0;
  int l1 = 0;
  unsigned long h = 0;
  int dsa_len[] = {keylen, 0};

  printf("Starting key unit test...\n");

  printf("Starting RSA subsection...\n");

  if (fips_mode) {
    keylen = 2048;
    dsa_len[0] = keylen;
  }
  memset(buf1, 0, sizeof(buf1));
  memset(buf2, 0, sizeof(buf2));
  check_stack(0);
  rsa = ICC_RSA_new(ICC_ctx);
  if (NULL == rsa) {
    printf("RSA subsection abort - could not allocate new RSA key!\n");
    rv = ICC_ERROR;
  } else {
    /* Only an API test, leaves a security exposure */
    ICC_RSA_blinding_off(ICC_ctx, rsa);
    ICC_RSA_free(ICC_ctx, rsa);
    rsa = NULL;
    check_stack(1);
    rsa = ICC_RSA_generate_key(ICC_ctx, keylen, 0x10001, NULL, NULL);

    check_stack(1);
    if (rsa == NULL) {
      printf("RSA_generate_key failed!\n");
      rv = ICC_ERROR;
    } else {
      tptr1 = buf1;
      l1 = retcode = ICC_i2d_RSAPrivateKey(ICC_ctx, rsa, &tptr1);

      tptr1 = buf1;
      l1 = retcode = ICC_i2d_RSAPublicKey(ICC_ctx, rsa, &tptr1);

      retcode = ICC_RSA_check_key(ICC_ctx, rsa);
      memset(buf2, 0, sizeof(buf2));
      retcode = ICC_RSA_private_encrypt(ICC_ctx, (keylen / 8), buf1, buf2, rsa,
                                        ICC_RSA_NO_PADDING);
      OSSLE(ICC_ctx);
      retcode = ICC_RSA_private_decrypt(ICC_ctx, retcode, buf2, buf1, rsa,
                                        ICC_RSA_NO_PADDING);
      OSSLE(ICC_ctx);
      retcode = ICC_RSA_public_encrypt(ICC_ctx, (keylen / 8), buf1, buf2, rsa,
                                       ICC_RSA_NO_PADDING);
      OSSLE(ICC_ctx);
      retcode = ICC_RSA_public_decrypt(ICC_ctx, retcode, buf2, buf1, rsa,
                                       ICC_RSA_NO_PADDING);
      /* This section was added solely to get test coverage of an unused function */
      tptr1 = buf1;
      pkey = ICC_EVP_PKEY_new(ICC_ctx);
      retcode = ICC_EVP_PKEY_set1_RSA(ICC_ctx, pkey, rsa);
      /* pkey can be null if we have induced a trng failure */
      if (NULL != pkey) { 
      l1 = retcode = ICC_i2d_PublicKey(ICC_ctx,pkey,&tptr1);
      ICC_EVP_PKEY_free(ICC_ctx,pkey);
      }
      tptr1 = buf1;
      pkey = ICC_d2i_PublicKey(ICC_ctx,ICC_EVP_PKEY_RSA,NULL,&tptr1,l1);
      if (NULL != pkey) {
          ICC_EVP_PKEY_free(ICC_ctx,pkey);
      }   
      /* End unused API */ 
      check_stack(1);
    }
    printf("RSA Subsection Completed!\n");
  }
  OSSLE(ICC_ctx);
  if (ICC_OSSL_SUCCESS == rv) {
    printf("Starting DSA subsection...\n");
    dsa = ICC_DSA_new(ICC_ctx);
    if (NULL == dsa) {
      rv = ICC_ERROR;
      printf("DSA subsection abort - could not allocate new DSA key!\n");
    } else {
      ICC_DSA_free(ICC_ctx, dsa);
      dsa = NULL;
      for (i = 0; dsa_len[i]; i++) {
        printf("\tDSA key size %d\n", dsa_len[i]);

        counter = 0;
        h = 0;

        dsa = ICC_DSA_generate_parameters(ICC_ctx, dsa_len[i],
                                          (unsigned char *)buf1,dsa_len[i]/4, &counter,
                                          &h, NULL, NULL);
        OSSLE(ICC_ctx);
        if (dsa == NULL) {
          printf("Fail ICC_DSA_generate_parameters %d\n", dsa_len[i]);
          rv = ICC_ERROR;
        } else {
          retcode = ICC_DSA_generate_key(ICC_ctx, dsa);
          if (retcode != 1) {
            printf("Fail ICC_DSA_generate_key %d\n", dsa_len[i]);
            rv = ICC_ERROR;
          } else {
            retcode = ICC_DSA_size(ICC_ctx, dsa);

            if (retcode <= 0) {
              printf("Fail ICC_DSA_size %d\n", dsa_len[i]);
              rv = ICC_ERROR;
            }
            tptr1 = buf1;
            /* Alternate (Standard) public key format */
            l1 = retcode = ICC_i2d_DSA_PUBKEY(ICC_ctx, dsa, &tptr1);
            tptr = buf1;
            dsa1 = ICC_d2i_DSA_PUBKEY(ICC_ctx, NULL, &tptr, l1);
            if (dsa1) {
              ICC_DSA_free(ICC_ctx, dsa1);
              dsa1 = NULL;
            }
            OSSLE(ICC_ctx);
            tptr1 = buf1;
            l1 = retcode = ICC_i2d_DSAPublicKey(ICC_ctx, dsa, &tptr1);
            tptr = buf1;
            dsa1 = ICC_d2i_DSAPublicKey(ICC_ctx, NULL, &tptr, l1);
            if (dsa1) {
              ICC_DSA_free(ICC_ctx, dsa1);
              dsa1 = NULL;
            }
            OSSLE(ICC_ctx);
            tptr1 = buf1;
            l1 = retcode = ICC_i2d_DSAPrivateKey(ICC_ctx, dsa, &tptr1);
            tptr = buf1;
            dsa1 = ICC_d2i_DSAPrivateKey(ICC_ctx, NULL, &tptr, l1);
            if (dsa1) {
              ICC_DSA_free(ICC_ctx, dsa1);
              dsa1 = NULL;
            }
            OSSLE(ICC_ctx);

            tptr1 = buf1;
            l1 = ICC_i2d_DSAparams(ICC_ctx, dsa, &tptr1);
            OSSLE(ICC_ctx);
            tptr1 = buf1;
            dsa1 = ICC_d2i_DSAparams(ICC_ctx, NULL,
                                     (const unsigned char **)&tptr1, (long)l1);
            OSSLE(ICC_ctx);
            if (dsa1) {
              ICC_DSA_free(ICC_ctx, dsa1);
              dsa1 = NULL;
            }
            if (dsa) {
              ICC_DSA_free(ICC_ctx, dsa);
              dsa = NULL;
            }
          }
        }
      }
      printf("DSA Subsection Completed!\n");
    }
  }
  OSSLE(ICC_ctx);
  if (ICC_OSSL_SUCCESS == rv) {
    printf("Starting DH subsection...\n");
    check_stack(0);
    dh = ICC_DH_new(ICC_ctx);
    if (NULL == dh) {
      rv = ICC_ERROR;
      printf("DH subsection abort - could not allocate new DH key!\n");
    } else {
      ICC_DH_free(ICC_ctx, dh);
      dh = NULL;
      dh = ICC_DH_generate_parameters(ICC_ctx, 64, ICC_DH_GENERATOR_5, NULL,
                                      NULL);
      if (NULL == dh) {
        printf("Cannot generate DH key\n");
        rv = ICC_ERROR;
      } else {
        ICC_DH_free(ICC_ctx, dh);
        dh = NULL;
        dh = ICC_DH_generate_parameters(ICC_ctx, 64, ICC_DH_GENERATOR_5, NULL,
                                        NULL);

        retcode = ICC_DH_generate_key(ICC_ctx, dh);
        retcode = ICC_DH_size(ICC_ctx, dh);
        retcode = ICC_DH_check(ICC_ctx, dh, intbuf1);
        retcode = ICC_DH_compute_key(
            ICC_ctx, (unsigned char *)buf1,
            (ICC_BIGNUM *)ICC_DH_get_PublicKey(ICC_ctx, dh), dh);

        ICC_DH_free(ICC_ctx, dh);
        dh = NULL;
        dh = ICC_DH_generate_parameters(ICC_ctx, 64, ICC_DH_GENERATOR_5, NULL,
                                        NULL);
        retcode = ICC_DH_generate_key(ICC_ctx, dh);

        pkey = ICC_EVP_PKEY_new(ICC_ctx);
        retcode = ICC_EVP_PKEY_set1_DH(ICC_ctx, pkey, dh);
        dh1 = ICC_EVP_PKEY_get1_DH(ICC_ctx, pkey);
        ICC_DH_free(ICC_ctx, dh1);
        dh1 = NULL;
        ICC_EVP_PKEY_free(ICC_ctx, pkey);
        pkey = NULL;
        blen = ICC_i2d_DHparams(ICC_ctx, dh, NULL); /* get length */
        dhcvtbuf = (unsigned char *)malloc(blen);
        ptr = dhcvtbuf;
        ICC_i2d_DHparams(ICC_ctx, dh, &ptr);
        ICC_DH_free(ICC_ctx, dh);
        dh = NULL;
        tptr = dhcvtbuf;
        dh1 = ICC_d2i_DHparams(ICC_ctx, NULL, &tptr, blen);

        retcode = ICC_DH_generate_key(ICC_ctx, dh1);
        retcode = ICC_DH_size(ICC_ctx, dh1);
        retcode = ICC_DH_check(ICC_ctx, dh1, intbuf1);

        /* Now fake transport of the public key */
        bn = (ICC_BIGNUM *)ICC_DH_get_PublicKey(
            ICC_ctx, dh1); /* get a pointer to it's key */

        len = ICC_BN_num_bytes(ICC_ctx, bn); /* Find the length */
        ICC_BN_bn2bin(ICC_ctx, bn, bufbn);   /* Convert to binary */

        /* ......... transport the key .............*/
        /* convert the (portable) representation to binary ,
           You DID remember to transport the length as well ?*/
        bn = ICC_BN_bin2bn(ICC_ctx, bufbn, len, NULL);

        retcode = ICC_DH_compute_key(ICC_ctx, (unsigned char *)buf1, bn, dh1);
        ICC_BN_clear_free(ICC_ctx, bn);
        bn = NULL;

        ICC_DH_free(ICC_ctx, dh1);
        dh1 = NULL;
        check_stack(1);
      }
      printf("DH Subsection Completed!\n");
    }
  }
  OSSLE(ICC_ctx);
  if (ICC_OSSL_SUCCESS == rv) {
    printf("Starting PKEY subsection...\n");
    check_stack(0);
    pkey = ICC_EVP_PKEY_new(ICC_ctx);
    if (NULL == pkey || NULL == rsa) {
      rv = ICC_ERROR;
      printf("PKEY subsection abort - could not create keys!\n");
    } else {
      retcode = ICC_EVP_PKEY_set1_RSA(ICC_ctx, pkey, rsa);
      retcode = ICC_EVP_PKEY_encrypt(ICC_ctx, buf1, buf2, 20, pkey);
      retcode = ICC_EVP_PKEY_decrypt(ICC_ctx, buf2, buf1, retcode, pkey);
      retcode = ICC_EVP_PKEY_bits(ICC_ctx, pkey);
      retcode = ICC_EVP_PKEY_size(ICC_ctx, pkey);

      retcode = ICC_i2d_PublicKey(ICC_ctx, pkey, NULL);

      ICC_RSA_free(ICC_ctx, rsa);
      rsa = NULL;
      rsa = ICC_EVP_PKEY_get1_RSA(ICC_ctx, pkey);
      ICC_RSA_free(ICC_ctx, rsa);
      rsa = NULL;
    }
    if (NULL != pkey) {
      ICC_EVP_PKEY_free(ICC_ctx, pkey);
      pkey = NULL;
    }
    if (ICC_OK == rv) {
      pkey = ICC_EVP_PKEY_new(ICC_ctx);
      /* Generate a DSA key */

      dsa = ICC_DSA_generate_parameters(ICC_ctx, 256, (unsigned char *)buf1, 20,
                                        &counter, &h, NULL, NULL);

      retcode = ICC_DSA_generate_key(ICC_ctx, dsa);
      /* Stick it into a pkey */
      retcode = ICC_EVP_PKEY_set1_DSA(ICC_ctx, pkey, dsa);
      /* free the DSA structures */
      ICC_DSA_free(ICC_ctx, dsa);
      dsa = NULL;
      /* Create a DSA structure from the pkey */
      dsa = ICC_EVP_PKEY_get1_DSA(ICC_ctx, pkey);
      /* Free the DSA */
      if (dsa != NULL) {
        ICC_DSA_free(ICC_ctx, dsa);
        dsa = NULL;
      }
      /* free the pkey */
      ICC_EVP_PKEY_free(ICC_ctx, pkey);
      pkey = NULL;

      pkey = ICC_EVP_PKEY_new(ICC_ctx);
      tptr1 = buf1;
      ICC_d2i_PrivateKey(ICC_ctx, 0, &pkey, &tptr1, 0);
      /* free the pkey */
      ICC_EVP_PKEY_free(ICC_ctx, pkey);
      pkey = NULL;
      /* Note that we do need to free and re-allocate
              as the d2i overwrites some fields otherwise
      */
      pkey = ICC_EVP_PKEY_new(ICC_ctx);
      tptr1 = (unsigned char *)buf1;
      ICC_d2i_PublicKey(ICC_ctx, 0, &pkey, &tptr1, 0);
      /* free the pkey */
      ICC_EVP_PKEY_free(ICC_ctx, pkey);
      pkey = NULL;
      check_stack(1);

      printf("PKEY Subsection Completed!\n");
    }
    printf("Freeing key structures...\n");

    free(dhcvtbuf);

    printf("Key structures successfully Freed!\n");

    if (ICC_OSSL_SUCCESS == rv) {
      printf("Key unit test successfully completed!\n");
    }
  }
  OSSLE(ICC_ctx);
  if (ICC_OSSL_SUCCESS == rv) {
    if (NULL != rsa)
      ICC_RSA_free(ICC_ctx, rsa);
    if (NULL != dsa)
      ICC_DSA_free(ICC_ctx, dsa);
    if (NULL != dsa1)
      ICC_DSA_free(ICC_ctx, dsa1);
    if (NULL != dh)
      ICC_DH_free(ICC_ctx, dh);
    if (NULL != dh1)
      ICC_DH_free(ICC_ctx, dh1);
    if (NULL != pkey)
      ICC_EVP_PKEY_free(ICC_ctx, pkey);
  }
  return rv;
}

int doBNUnitTest(ICC_CTX *ICC_ctx)
{
  int len = 0;
  int retcode = 0;
  ICC_BIGNUM *bn = NULL;
  ICC_BIGNUM *r = NULL;
  ICC_BIGNUM *p = NULL;
  ICC_BIGNUM *m = NULL;
  ICC_BN_CTX *bn_ctx = NULL;
  const unsigned char bin[] = {0,0,0,1};
  const unsigned char eight[] = {0,0,0,8};

  printf("Starting BIGNUM unit test...\n");	


  bn = ICC_BN_new(ICC_ctx);
  len = ICC_BN_num_bytes(ICC_ctx,bn);
  if( len != 0 ) {
    printf("Expected 0 size BIGNUM, got %d\n",len);
  }
  check_stack(0);
  ICC_BN_clear_free(ICC_ctx,bn);

  /* 
     BN_CTX BN_mod_exp tests
  */
  bn_ctx = ICC_BN_CTX_new(ICC_ctx);
  bn = ICC_BN_new(ICC_ctx);
  ICC_BN_clear_free(ICC_ctx,bn);
  bn = ICC_BN_bin2bn(ICC_ctx,bin,4,NULL);
  r = ICC_BN_bin2bn(ICC_ctx,bin,4,NULL);
  p = ICC_BN_bin2bn(ICC_ctx,bin,4,NULL);
  m = ICC_BN_bin2bn(ICC_ctx,bin,4,NULL);

  retcode = ICC_BN_div(ICC_ctx,r,m,bn,p,bn_ctx);
  ICC_BN_clear_free(ICC_ctx,r);
  ICC_BN_clear_free(ICC_ctx,m);
  ICC_BN_clear_free(ICC_ctx,bn);
  ICC_BN_clear_free(ICC_ctx,p);

  bn = ICC_BN_bin2bn(ICC_ctx,bin,4,NULL);
  r = ICC_BN_bin2bn(ICC_ctx,bin,4,NULL);
  p = ICC_BN_bin2bn(ICC_ctx,bin,4,NULL);
  m = ICC_BN_bin2bn(ICC_ctx,bin,4,NULL);

  retcode = ICC_BN_mod_exp(ICC_ctx,r,bn,p,m,bn_ctx);

  ICC_BN_clear_free(ICC_ctx,r);
  ICC_BN_clear_free(ICC_ctx,m);
  ICC_BN_clear_free(ICC_ctx,bn);
  ICC_BN_clear_free(ICC_ctx,p);

  bn = ICC_BN_bin2bn(ICC_ctx,bin,4,NULL);
  r = ICC_BN_bin2bn(ICC_ctx,bin,4,NULL);
  p = ICC_BN_bin2bn(ICC_ctx,bin,4,NULL);
  m = ICC_BN_bin2bn(ICC_ctx,eight,4,NULL);

  if(ICC_NOT_IMPLEMENTED == ICC_BN_add(ICC_ctx,r,p,m) ) {
    printf("\tICC_BN_add not implemented\n");
  }
  if(ICC_NOT_IMPLEMENTED == ICC_BN_sub(ICC_ctx,r,p,m) ) {
    printf("\tICC_BN_sub not implemented\n");
  }
  if(ICC_NOT_IMPLEMENTED == ICC_BN_cmp(ICC_ctx,r,p) ) {
    printf("\tICC_BN_cmp not implemented\n");
  }

  ICC_BN_mod_mul(ICC_ctx,r,bn,p,(const ICC_BIGNUM *)m,bn_ctx);
  
  ICC_BN_clear_free(ICC_ctx,r);
  ICC_BN_clear_free(ICC_ctx,m);
  ICC_BN_clear_free(ICC_ctx,bn);
  ICC_BN_clear_free(ICC_ctx,p);

  ICC_BN_CTX_free(ICC_ctx,bn_ctx);
  check_stack(1);
 
  printf("BIGNUM structures successfully Freed!\n");

  printf("BIGNUM test successfully completed!\n");

  return retcode;	
}

int doErrorUnitTest(ICC_CTX *ICC_ctx)
{
  const char *retstring = NULL;
  int retcode = ICC_OSSL_SUCCESS;

  printf("Starting Error unit test...\n");

  check_stack(0);
  retcode = ICC_ERR_get_error(ICC_ctx);
  retcode = ICC_ERR_peek_error(ICC_ctx);
  retcode = ICC_ERR_peek_last_error(ICC_ctx);
  retstring = ICC_ERR_error_string(ICC_ctx,retcode,(char *)buf1);
  ICC_ERR_error_string_n(ICC_ctx,retcode,(char *)buf1,1);
  retstring = ICC_ERR_lib_error_string(ICC_ctx,retcode);
  retstring = ICC_ERR_func_error_string(ICC_ctx,retcode);
  retstring = ICC_ERR_reason_error_string(ICC_ctx,retcode);
  ICC_ERR_clear_error(ICC_ctx);
  ICC_ERR_remove_state(ICC_ctx,0);
  check_stack(1);
  if((NULL == retstring) || strlen(retstring) > 0 ) {
    retcode = ICC_OSSL_SUCCESS;
  }
  printf("Error Unit test successfully completed!\n");

  return retcode;
}

int doCMACUnitTest(ICC_CTX *ICC_ctx)
{

  static unsigned char cmac_ka_key[] = {
    0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c
  };
  

  ICC_CMAC_CTX *cmac_ctx = NULL;
  const ICC_EVP_CIPHER *cip = NULL;
  unsigned char Result[16];
  int maclen = 16;
  printf("Starting CMAC unit test...\n");
  check_stack(0);
  cip = ICC_EVP_get_cipherbyname(ICC_ctx,"AES-128-CBC");
  cmac_ctx = ICC_CMAC_CTX_new(ICC_ctx);
  if(NULL != cmac_ctx) {
    ICC_CMAC_Init(ICC_ctx,cmac_ctx,cip,cmac_ka_key,16);
    ICC_CMAC_Update(ICC_ctx,cmac_ctx,NULL,0);
    ICC_CMAC_Final(ICC_ctx,cmac_ctx,Result,maclen);
    ICC_CMAC_CTX_free(ICC_ctx,cmac_ctx);
    check_stack(1);
    OSSLE(ICC_ctx);
    printf("CMAC Unit test successfully completed!\n");
  } else {
    printf("CMAC not implemented\n");
  }
  return ICC_OSSL_SUCCESS;

}

int doHMACUnitTest(ICC_CTX *ICC_ctx)
{

  static const unsigned char  hmac_ka_key[] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f, 
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17, 
    0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27, 
    0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
    0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f
  };
  static const unsigned char hmac_ka_data[] = {
    0x53,0x61,0x6d,0x70,0x6c,0x65,0x20,0x23,
    0x31
  };

  ICC_HMAC_CTX *hmac_ctx = NULL;
  const ICC_EVP_MD *digest = NULL;
  unsigned char Result[20];
  unsigned int outlen = 0;

  printf("Starting HMAC unit test...\n");
  check_stack(0);
  digest = ICC_EVP_get_digestbyname(ICC_ctx,"SHA1");
  hmac_ctx = ICC_HMAC_CTX_new(ICC_ctx);
  if(NULL != hmac_ctx) {
    ICC_HMAC_Init(ICC_ctx,hmac_ctx,hmac_ka_key,sizeof(hmac_ka_key),digest);
    ICC_HMAC_Update(ICC_ctx,hmac_ctx,hmac_ka_data,sizeof(hmac_ka_data));
    ICC_HMAC_Final(ICC_ctx,hmac_ctx,Result,&outlen);
    ICC_HMAC_CTX_free(ICC_ctx,hmac_ctx);
    check_stack(1);
    printf("HMAC Unit test successfully completed!\n");
  } else {
    printf("HAMC Not implemented\n");
  }
  return ICC_OSSL_SUCCESS;

}


int doAES_GCMUnitTest(ICC_CTX *ICC_ctx)
{

  /* NIST draft test vector 4 */
  static unsigned char  gcm_ka_key[] = {
    0xfe,0xff,0xe9,0x92,0x86,0x65,0x73,0x1c,
    0x6d,0x6a,0x8f,0x94,0x67,0x30,0x83,0x08
  };

  static unsigned char gcm_ka_plaintext[] = {
    0xd9,0x31,0x32,0x25,0xf8,0x84,0x06,0xe5,
    0xa5,0x59,0x09,0xc5,0xaf,0xf5,0x26,0x9a,
    0x86,0xa7,0xa9,0x53,0x15,0x34,0xf7,0xda,
    0x2e,0x4c,0x30,0x3d,0x8a,0x31,0x8a,0x72,
    0x1c,0x3c,0x0c,0x95,0x95,0x68,0x09,0x53,
    0x2f,0xcf,0x0e,0x24,0x49,0xa6,0xb5,0x25,
    0xb1,0x6a,0xed,0xf5,0xaa,0x0d,0xe6,0x57,
    0xba,0x63,0x7b,0x39
  };
  static unsigned char gcm_ka_iv[] = {
    0xca,0xfe,0xba,0xbe,0xfa,0xce,0xdb,0xad,
    0xde,0xca,0xf8,0x88
  };
  static unsigned char gcm_ka_aad[] = {
    0xfe,0xed,0xfa,0xce,0xde,0xad,0xbe,0xef,
    0xfe,0xed,0xfa,0xce,0xde,0xad,0xbe,0xef,
    0xab,0xad,0xda,0xd2
  };
  static unsigned char gcm_ka_ciphertext[] = {
    0x42,0x83,0x1e,0xc2,0x21,0x77,0x74,0x24,
    0x4b,0x72,0x21,0xb7,0x84,0xd0,0xd4,0x9c,
    0xe3,0xaa,0x21,0x2f,0x2c,0x02,0xa4,0xe0,
    0x35,0xc1,0x7e,0x23,0x29,0xac,0xa1,0x2e,
    0x21,0xd5,0x14,0xb2,0x54,0x66,0x93,0x1c,
    0x7d,0x8f,0x6a,0x5a,0xac,0x84,0xaa,0x05,
    0x1b,0xa3,0x0b,0x39,0x6a,0x0a,0xac,0x97,
    0x3d,0x58,0xe0,0x91
  };
  static unsigned char gcm_ka_authtag[] = {
    0x5b,0xc9,0x4f,0xbc,0x32,0x21,0xa5,0xdb,
    0x94,0xfa,0xe9,0x5a,0xe7,0x12,0x1a,0x47
  };
  /* We don't run the full test during BVT, takes too long , uses insane memory
     but it's useful to do it again and again and ...
  */
#define TEST_IVS 6666
  int rv = ICC_OSSL_SUCCESS;
  unsigned char *ivbuf = NULL;
  unsigned char Result[16];
  unsigned long outlen = 0;
  unsigned long toutlen = 0;
  unsigned char ciphertext[sizeof(gcm_ka_ciphertext)+32];
  unsigned char pt[sizeof(gcm_ka_plaintext)];
  ICC_AES_GCM_CTX *gcm_ctx = NULL;
  static  unsigned char tmp1[16] = {
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
  };
  static unsigned char tmp2[16];
  int i,j;
  int err = 0;
  memset(Result,0,sizeof(Result));
  memset(ciphertext,0,sizeof(ciphertext));
  memset(pt,0,sizeof(pt));


  printf("Starting AES_GCM unit test...\n");
  check_stack(0);
  ivbuf = calloc(8,TEST_IVS);

  gcm_ctx = ICC_AES_GCM_CTX_new(ICC_ctx);
  if(NULL != gcm_ctx) {
    printf("\tTesting AES_GCM IV generation...\n");
    for(i = 0; i < TEST_IVS; i++) {
      if( 0 == ICC_AES_GCM_GenerateIV(ICC_ctx,gcm_ctx,&ivbuf[i*8]) ) {
	err ++;
      }
    }
    if(err) {
      printf("Bug: AES_GCM IV generation failed due to TRNG seed failure\n");
    }
    for(i = 0; i < TEST_IVS; i++) {
      for(j = i+1; j < TEST_IVS; j++) {
	if(memcmp(&ivbuf[i*8],&ivbuf[j*8],8) == 0) {
	  printf("Bug: AES_GCM IV gen returned identical IV's in %d sample test\n",TEST_IVS);
	  err++;
	}
      }
    }

    if(err) { 
      printf("\tAES_GCM IV generation failed.\n");
    } else {
      printf("\tAES_CGM IV, no repeats in %u samples\n",TEST_IVS);
    }
    printf("\tStarting GCM tests\n");

    toutlen = 0;
    ICC_AES_GCM_Init(ICC_ctx,gcm_ctx,gcm_ka_iv,sizeof(gcm_ka_iv),gcm_ka_key,sizeof(gcm_ka_key));

    ICC_AES_GCM_EncryptUpdate(ICC_ctx,gcm_ctx,gcm_ka_aad,sizeof(gcm_ka_aad),gcm_ka_plaintext,sizeof(gcm_ka_plaintext),ciphertext,&outlen);  
    toutlen += outlen;
    ICC_AES_GCM_EncryptFinal(ICC_ctx,gcm_ctx,ciphertext+outlen,&outlen,Result);
    toutlen += outlen;
    if(0 != memcmp(Result,gcm_ka_authtag,sizeof(gcm_ka_authtag)) ) {
      printf("\t\tGCM Encrypt failed (authtag mismatch)\n");
      rv = ICC_OPENSSL_ERROR;
    }
    /* IV MUST be supplied */
    ICC_AES_GCM_Init(ICC_ctx,gcm_ctx,gcm_ka_iv,sizeof(gcm_ka_iv),gcm_ka_key,sizeof(gcm_ka_key));
    outlen = 0;
    ICC_AES_GCM_DecryptUpdate(ICC_ctx,gcm_ctx,gcm_ka_aad,sizeof(gcm_ka_aad),ciphertext,toutlen,pt,&outlen);
    if(1 != ICC_AES_GCM_DecryptFinal(ICC_ctx,gcm_ctx,pt+outlen,&outlen,Result,16)) {
      printf("\t\tGCM Decrypt failed (authtag mismatch)\n");
      rv = ICC_OPENSSL_ERROR;
    }

    /* And now - a couple of tests just for completeness
       all these really do is put a tick in the box WRT test coverage

       GHASH is there are a convenience while testing
    */
    ICC_AES_GCM_CTX_ctrl(ICC_ctx,gcm_ctx,ICC_AES_GCM_CTRL_SET_ACCEL,0,NULL);
    ICC_GHASH(ICC_ctx,gcm_ctx,tmp1,tmp2,gcm_ka_plaintext,sizeof(gcm_ka_plaintext));
    check_stack(1);
    if(ICC_OSSL_SUCCESS == rv ) {
      printf("AES_GCM Unit test successfully completed!\n");
    } 
    ICC_AES_GCM_CTX_free(ICC_ctx,gcm_ctx);
  } else {
    printf("AES_GCM not implemented\n");
  }
  if(NULL != ivbuf) free(ivbuf);
  ivbuf = NULL;
  return rv;
}
int doAES_CCMUnitTest(ICC_CTX *ICC_ctx)
{

  /* NIST SP800-38C example vector 2 */
  static unsigned char  Key[] = {
    0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
    0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f
  };

  static unsigned char nonce[] = {
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17
  };
  static unsigned char aad[] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
  };
  static unsigned char pt[] = {
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
    0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f
  };
  static unsigned char ct[] = {
    0xd2,0xa1,0xf0,0xe0,0x51,0xea,0x5f,0x62,
    0x08,0x1a,0x77,0x92,0x07,0x3d,0x59,0x3d,
    0x1f,0xc6,0x4f,0xbf,0xac,0xcd
  };


  int rv = ICC_OSSL_SUCCESS;
  unsigned char *out = NULL;
  unsigned long outlen = 0;
  int err = 0;

  printf("Starting AES_CCM unit test...\n");
  check_stack(0);
  out = malloc(sizeof(pt)+64);
  if(ICC_NOT_IMPLEMENTED == 
     ICC_AES_CCM_Encrypt(ICC_ctx,nonce,sizeof(nonce),Key,16,aad,sizeof(aad),
			 pt,sizeof(pt),out,&outlen,6) 
     ) {
    printf("AES_CCM not implemented\n");
  } else {
    /* make sure known answer is correct for encrypt  */
    if(memcmp(out,ct,sizeof(ct)) != 0) {
      rv = ICC_FAILURE;
    }

    err = ICC_AES_CCM_Decrypt(ICC_ctx,nonce,sizeof(nonce),Key,16,aad,sizeof(aad),
			      ct,sizeof(ct),out,&outlen,6);
    if( err != 1 ) { /* Verification failed */
      rv = ICC_FAILURE;
    } else if(memcmp(out,pt,sizeof(pt)) != 0) {
      rv = ICC_FAILURE;
    }
    
    check_stack(1);
    if(ICC_OSSL_SUCCESS == rv ) {
      printf("AES_CCM Unit test successfully completed!\n");
    }
  }
  if(out) free(out);
  return rv;

}



int doDESUnitTest(ICC_CTX *ICC_ctx)
{
  ICC_DES_cblock cblock;

  printf("Starting DES unit test...\n");
  check_stack(0);
  ICC_DES_random_key(ICC_ctx,&cblock);
  ICC_DES_set_odd_parity(ICC_ctx,&cblock);
  check_stack(1);
  printf("DES Unit test successfully completed!\n");
  return ICC_OSSL_SUCCESS;
}

/* Conditional on the EC API being present */
int doEC_KEYTest(ICC_CTX *ICC_ctx)
{
  int retcode = ICC_OSSL_SUCCESS;
  ICC_EVP_PKEY *pkey = NULL;
  ICC_EC_KEY *ec_key = NULL,*ec_key1 = NULL;
  const ICC_EC_GROUP *group = NULL;
  const ICC_EC_GROUP *group1 = NULL;
  const ICC_BIGNUM *ec_priv = NULL;
  const ICC_EC_POINT *ec_pub = NULL;

  unsigned char *ptr = NULL;
  unsigned char *buf = NULL;
  int len = 0;
  int nid = 0;
 
  printf("Starting EC_KEY unit test...\n");

  nid = ICC_OBJ_txt2nid(ICC_ctx,(char *)"secp384r1");
  ec_key = ICC_EC_KEY_new_by_curve_name(ICC_ctx,nid);
  if(ec_key == NULL) {
    printf("EC_KEY function not implemented\n");
  } else {
    /* For test coverage */
    group = ICC_EC_KEY_get0_group(ICC_ctx,ec_key);
    ec_key1 = ICC_EC_KEY_new(ICC_ctx);
    ICC_EC_KEY_set_group(ICC_ctx,ec_key1,group);
    ICC_EC_KEY_free(ICC_ctx,ec_key1);
    ec_key1 = NULL;

    ec_key1 = ICC_EC_KEY_dup(ICC_ctx,ec_key);
    group1 = ICC_EC_KEY_get0_group(ICC_ctx,ec_key);
    if(group != group1) {
      retcode = ICC_OSSL_FAILURE;
    }
    ICC_EC_KEY_free(ICC_ctx,ec_key1);
    ec_key1 = NULL;
    /* Generate a key from this curve */
    group = NULL;
    group1 = NULL;
    if( ICC_EC_KEY_generate_key(ICC_ctx,ec_key) == 0) {
      printf("ICC_EC_KEY_generate_key() failed!\n");
      retcode = ICC_OSSL_FAILURE;
    }
    if(ICC_OSSL_SUCCESS == retcode) {   
      check_stack(0);
      /* Key extract/restore */
      ec_priv = ICC_EC_KEY_get0_private_key(ICC_ctx,ec_key);
      ec_key1 = ICC_EC_KEY_new_by_curve_name(ICC_ctx,nid);

      if(NULL == ec_key1) {
	retcode = ICC_OSSL_FAILURE;
      } else {
	ICC_EC_KEY_set_private_key(ICC_ctx,ec_key1,ec_priv); 
	ICC_EC_KEY_free(ICC_ctx,ec_key1);
      }
      ec_key1 = NULL;
      OSSLE(ICC_ctx);
      ec_pub = ICC_EC_KEY_get0_public_key(ICC_ctx,ec_key);
      ec_key1 = ICC_EC_KEY_new_by_curve_name(ICC_ctx,nid);
 
      OSSLE(ICC_ctx);
      if(NULL == ec_key1) {
	retcode = ICC_OSSL_FAILURE;
      } else {
	ICC_EC_KEY_set_public_key(ICC_ctx,ec_key1,ec_pub);
	ICC_EC_KEY_free(ICC_ctx,ec_key1);
	ec_key1 = NULL;
	OSSLE(ICC_ctx);	
	/* Now Stuff it into an EVP_PKEY and extract again */
	pkey = ICC_EVP_PKEY_new(ICC_ctx);
      }
      OSSLE(ICC_ctx);
      if(NULL == pkey) {
	retcode = ICC_OSSL_FAILURE;
      } 
      if(ICC_OSSL_SUCCESS == retcode) {
	ICC_EVP_PKEY_set1_EC_KEY(ICC_ctx,pkey,ec_key);  
	ec_key1 = ICC_EVP_PKEY_get1_EC_KEY(ICC_ctx,pkey);
	ICC_EC_KEY_free(ICC_ctx,ec_key1);
	ec_key1 = NULL;

	ICC_EVP_PKEY_free(ICC_ctx,pkey);
	pkey = NULL;   
      }
      OSSLE(ICC_ctx);
      if(ICC_OSSL_SUCCESS == retcode) {
	/* i2d/d2i function */

      /* EC Private Key export/import */
	len = ICC_i2d_ECPrivateKey(ICC_ctx,ec_key,NULL);
	buf = (unsigned char *)calloc(len,1);
	ptr = buf;
	len = ICC_i2d_ECPrivateKey(ICC_ctx,ec_key,&ptr);
	
	ptr = buf;
	ec_key1 = ICC_d2i_ECPrivateKey(ICC_ctx,NULL,(const unsigned char **)&ptr,len);
	if(NULL == ec_key1) {
	  printf("d2i_ECPrivateKey failed, key is NULL\n");
	} else {
	  ICC_EC_KEY_free(ICC_ctx,ec_key1);
        }
	ec_key1 = NULL;    
	free(buf);
	buf = ptr = NULL;

	/* EC parameter export/import */
	len = ICC_i2d_ECParameters(ICC_ctx,ec_key,NULL);
	buf = (unsigned char *)calloc(len,1);
	ptr = buf;
	
	ICC_i2d_ECParameters(ICC_ctx,ec_key,&ptr);
	
	ptr = buf;
	ec_key1 = ICC_d2i_ECParameters(ICC_ctx,NULL,(const unsigned char **)&ptr,len);
      
	ICC_EC_KEY_free(ICC_ctx,ec_key1);
	ec_key1 = NULL;        
	free(buf);
	buf = ptr = NULL;

	/* ANSI X9.62 EC parameter export/import */
	group = ICC_EC_KEY_get0_group(ICC_ctx,ec_key);
	ICC_EC_GROUP_set_asn1_flag(ICC_ctx,(ICC_EC_GROUP *)group,1);
	len = ICC_i2d_ECPKParameters(ICC_ctx,group,NULL);
	buf = (unsigned char *)calloc(len,1);
	ptr = buf;
	
	ICC_i2d_ECPKParameters(ICC_ctx,group,&ptr);
	ICC_EC_GROUP_set_asn1_flag(ICC_ctx,(ICC_EC_GROUP *)group,0);
	
	ptr = buf;
	group = ICC_d2i_ECPKParameters(ICC_ctx,NULL,&ptr,len);
	OSSLE(ICC_ctx);
	ICC_EC_GROUP_free(ICC_ctx,(ICC_EC_GROUP *)group);
	group = NULL;
	free(buf);
	buf = ptr = NULL;
	
	/* EC public key export/import */
	len = ICC_i2o_ECPublicKey(ICC_ctx,ec_key,NULL);
	buf = (unsigned char *)calloc(len,1);
	ptr = buf;
	
	ICC_i2o_ECPublicKey(ICC_ctx,ec_key,&ptr);
	OSSLE(ICC_ctx);
	ptr = buf;
	ec_key1 = ICC_EC_KEY_new_by_curve_name(ICC_ctx,nid);
	ICC_o2i_ECPublicKey(ICC_ctx,&ec_key1,&ptr,len);
	OSSLE(ICC_ctx);
	free(buf);
	ptr = buf = NULL;
      }
      /* End export/import */
      check_stack(1);
    }
  }
  if(NULL != buf) free(buf);
  if(ec_key != NULL) ICC_EC_KEY_free(ICC_ctx,ec_key);
  if(ec_key1 != NULL) ICC_EC_KEY_free(ICC_ctx,ec_key1);
  if(pkey != NULL) ICC_EVP_PKEY_free(ICC_ctx,pkey);
  if(retcode == ICC_OSSL_SUCCESS) {
    printf("EC_KEY Unit test successfully completed!\n");
  }
  return retcode;
}

#define SHA1_len 20
static ICC_CTX *ctx = NULL; /* Need to set this in ECDHTest before using the KDF callback */ 

static void *KDF(const void *in, size_t inlen, void *out, size_t *outlen)
{
  ICC_EVP_MD_CTX *md_ctx;
  const ICC_EVP_MD *md;

  check_stack(0);
  md_ctx = ICC_EVP_MD_CTX_new(ctx);
  md = ICC_EVP_get_digestbyname(ctx,"SHA1");
  ICC_EVP_DigestInit(ctx,md_ctx,md);
  ICC_EVP_DigestUpdate(ctx,md_ctx,in,(int)inlen);
  ICC_EVP_DigestFinal(ctx,md_ctx,(unsigned char *)out,NULL);
  ICC_EVP_MD_CTX_free(ctx,md_ctx);
  *outlen = SHA1_len;
  check_stack(1);
  return out;
}


int doECDHTest(ICC_CTX *ICC_ctx)
{
  int retcode = ICC_OSSL_SUCCESS;
  
  ICC_EC_KEY *a=NULL;
  ICC_EC_KEY *b=NULL;
  const ICC_EC_GROUP *group = NULL;

  ICC_BN_CTX *bn_ctx = NULL;
  ICC_BIGNUM *x_a = NULL;
  ICC_BIGNUM *x_b = NULL;
  ICC_BIGNUM *y_a = NULL;
  ICC_BIGNUM *y_b = NULL;
  ICC_BIGNUM *order = NULL;

  const ICC_EC_METHOD *method = NULL;
  ICC_EC_POINT *p1 = NULL, *p2 = NULL;

  unsigned char *hash_bufa[SHA1_len];
  unsigned char *hash_bufb[SHA1_len];

  int lena = 0;
  int lenb = 0;
  int nid = 0;

  ctx = ICC_ctx; /* needed for the KDF callback above to function */
  printf("Starting ECDH unit test...\n");
  check_stack(0);

  nid = ICC_OBJ_txt2nid(ICC_ctx,(char *)"secp384r1");
  a = ICC_EC_KEY_new_by_curve_name(ICC_ctx,nid);
  b = ICC_EC_KEY_new_by_curve_name(ICC_ctx,nid);
  if( (NULL == a) || (NULL == b) ) {
    printf("ECDH test skipped, no EC function\n");
  } else {
    bn_ctx = ICC_BN_CTX_new(ICC_ctx);
    x_a = ICC_BN_new(ICC_ctx);
    x_b = ICC_BN_new(ICC_ctx);
    y_a = ICC_BN_new(ICC_ctx);
    y_b = ICC_BN_new(ICC_ctx);
    order =  ICC_BN_new(ICC_ctx);
    group = ICC_EC_KEY_get0_group(ICC_ctx,a);
    method = ICC_EC_GROUP_method_of(ICC_ctx,group);
    
    
    retcode = ICC_EC_KEY_generate_key(ICC_ctx,a) ;
    if( ICC_OSSL_SUCCESS == retcode) {
      /* For coverage only ... */
      ICC_EC_GROUP_get0_generator(ICC_ctx,(ICC_EC_GROUP *)group);
      /* For coverage only ... , the call should fail */
      ICC_EC_GROUP_get_curve_GF2m(ICC_ctx,group,NULL,x_a,x_b,bn_ctx);
      /* For coverage only ... */
      ICC_EC_GROUP_get_degree(ICC_ctx,group);
      /* For coverage only ... */
      ICC_EC_GROUP_get_curve_GFp(ICC_ctx,group,NULL,x_a,x_b,bn_ctx);
      
      p1 = ICC_EC_POINT_new(ICC_ctx,group);
    }
    if( ICC_OSSL_SUCCESS != retcode) {
      printf("ECDH test aborted, could not generate EC key!\n");
      retcode = ICC_OSSL_FAILURE;
    } 
    if( ICC_OSSL_SUCCESS == retcode) {
      if( ICC_EC_METHOD_get_field_type(ICC_ctx,method) == ICC_NID_X9_62_prime_field) {
	ICC_EC_POINT_get_affine_coordinates_GFp(ICC_ctx,group,ICC_EC_KEY_get0_public_key(ICC_ctx,a),x_a,y_a,bn_ctx);
	
	ICC_EC_POINT_set_affine_coordinates_GFp(ICC_ctx,group,p1,x_a,y_a,bn_ctx);
	
      } else {
	ICC_EC_POINT_get_affine_coordinates_GF2m(ICC_ctx,group,ICC_EC_KEY_get0_public_key(ICC_ctx,a),x_a,y_a,bn_ctx);
	
	ICC_EC_POINT_set_affine_coordinates_GF2m(ICC_ctx,group,p1,x_a,y_a,bn_ctx);
	
      }
      
      p2 = ICC_EC_POINT_dup(ICC_ctx,p1,group);
      ICC_EC_POINT_is_at_infinity(ICC_ctx,group,p1);
      ICC_EC_POINT_mul(ICC_ctx,group,p1,x_a,NULL,NULL,bn_ctx);
      ICC_EC_GROUP_get_order(ICC_ctx,group,order,bn_ctx);
      ICC_BN_clear_free(ICC_ctx,order);
      order = NULL;
      ICC_EC_POINT_free(ICC_ctx,p2);
      p2 = NULL;
      ICC_EC_POINT_free(ICC_ctx,p1);
      p1 = NULL;
      ICC_EC_KEY_generate_key(ICC_ctx,b);
  
      if( ICC_EC_METHOD_get_field_type(ICC_ctx,method) == ICC_NID_X9_62_prime_field) {
	ICC_EC_POINT_get_affine_coordinates_GFp(ICC_ctx,group,ICC_EC_KEY_get0_public_key(ICC_ctx,b),x_b,y_b,bn_ctx);
      } else {
	ICC_EC_POINT_get_affine_coordinates_GF2m(ICC_ctx,group,ICC_EC_KEY_get0_public_key(ICC_ctx,b),x_b,y_b,bn_ctx);
      }
  
      retcode = ICC_EC_POINT_is_on_curve(ICC_ctx,group,ICC_EC_KEY_get0_public_key(ICC_ctx,b),bn_ctx);

      lena = ICC_ECDH_compute_key(ICC_ctx,hash_bufa,SHA1_len,ICC_EC_KEY_get0_public_key(ICC_ctx,b),a,KDF);
      lenb = ICC_ECDH_compute_key(ICC_ctx,hash_bufb,SHA1_len,ICC_EC_KEY_get0_public_key(ICC_ctx,a),b,KDF);
      check_stack(1);
      if( (lena < 4) || (lena != lenb) || (memcmp(hash_bufa,hash_bufb,lena) != 0)) {
	retcode = ICC_OSSL_FAILURE;
	printf("    ECDH key agreement failed!\n");
      }
  

    }

    if(retcode == ICC_OSSL_SUCCESS) {
      printf("ECDH Unit test successfully completed!\n");
    }
  }
  if(NULL != order) ICC_BN_clear_free(ICC_ctx,order);
  if(NULL != p2)  ICC_EC_POINT_free(ICC_ctx,p2);
  if(NULL != p1)  ICC_EC_POINT_free(ICC_ctx,p1);
  if(NULL != y_b) ICC_BN_clear_free(ICC_ctx,y_b);
  if(NULL != y_a) ICC_BN_clear_free(ICC_ctx,y_a);
  if(NULL != x_b) ICC_BN_clear_free(ICC_ctx,x_b);
  if(NULL != x_a) ICC_BN_clear_free(ICC_ctx,x_a);
  if(NULL != bn_ctx) ICC_BN_CTX_free(ICC_ctx,bn_ctx);
  if(NULL != a)   ICC_EC_KEY_free(ICC_ctx,a);
  if(NULL != b)   ICC_EC_KEY_free(ICC_ctx,b);
  ctx = NULL;
  return retcode;

}
int doECDSATest(ICC_CTX *ICC_ctx)
{
  int retcode = ICC_OSSL_SUCCESS;
  int len = 0;
  ICC_ECDSA_SIG *sig = NULL;
  ICC_EC_KEY *ec_key = NULL;
  /* We only use 20 bytes, but some compiler complain we don't have space
     for the '\0' terminator */
  unsigned char fake_hash[SHA1_len+1] = "01234567890123456789";
  unsigned char *sigbuf = NULL;
  unsigned int siglen = 0;
  unsigned char *buf = NULL;
  unsigned char *tmp = NULL;
  unsigned int bufl = 0;
  int nid = 0;

  printf("Starting ECDSA unit test...\n");
  nid = ICC_OBJ_txt2nid(ICC_ctx,(char *)"secp521r1");
  ec_key = ICC_EC_KEY_new_by_curve_name(ICC_ctx,nid);
  if(ICC_NOT_IMPLEMENTED == ICC_EC_KEY_generate_key(ICC_ctx,ec_key)) {
    printf("ECDSA test skipped, no EC function\n");
  } else {
    /* Now check sign/verify */
    printf("   Testing ECDSA sign/verify...\n");
    len = ICC_ECDSA_size(ICC_ctx,ec_key);
    sigbuf = (unsigned char *)calloc(len,1);
    if( ICC_ECDSA_sign(ICC_ctx,0,fake_hash,20,sigbuf,&siglen,ec_key) 
	!= ICC_OSSL_SUCCESS ){
      retcode = ICC_OSSL_FAILURE;
    } else {
      if( ICC_ECDSA_verify(ICC_ctx,0,fake_hash,20,sigbuf,siglen,ec_key) 
	  != ICC_OSSL_SUCCESS) {
	retcode = ICC_OSSL_FAILURE;
      }
    }
    if(retcode == ICC_OSSL_SUCCESS) {
      printf("   ECDSA sign/verify O.K.!\n");
    } else {
      printf("   ECDSA sign/verify failed!\n");
    }
    /* Now check i2d/d2i functions 
       In theory we have a DER encoded signature in sigbuf from the previous test
       It should be "len" long
    */
    tmp = sigbuf;
    ICC_d2i_ECDSA_SIG(ICC_ctx,&sig,(const unsigned char **)&tmp,(long)siglen);
    if(sig == NULL) {
      printf("   ECDSA d2i failed!\n");
      retcode = ICC_OSSL_FAILURE;
    } else {
      bufl = ICC_i2d_ECDSA_SIG(ICC_ctx,sig,NULL);
      if((bufl != siglen) ) {
      	retcode = ICC_OSSL_FAILURE;
      }
      if(sig != NULL) ICC_ECDSA_SIG_free(ICC_ctx,sig);
    }
    if(sigbuf != NULL) free(sigbuf);
    if(buf != NULL) free(buf);
    /* Simple allocator/deallocator check */
    sig = ICC_ECDSA_SIG_new(ICC_ctx);
    ICC_ECDSA_SIG_free(ICC_ctx,sig);

    /* Now add some code just to cover the parts of the EC API
       that we never actually needed to use in real test cases
    */
    if(NULL != ec_key) {
      ICC_EC_KEY_check_key(ICC_ctx,ec_key); 
    }    
    if(retcode == ICC_OSSL_SUCCESS) {
      printf("ECDSA Unit test successfully completed!\n");
    } 
  }
  if(NULL != ec_key) ICC_EC_KEY_free(ICC_ctx,ec_key);
  return retcode;
}


int doTRNGTest(ICC_CTX *ICC_ctx,ICC_STATUS *status)
{
  int rv = ICC_OSSL_SUCCESS;
  char *buffer = NULL;
  int entropy = 0;
  int i;
  printf("Starting TRNG unit test...\n");
  ICC_GetValue(ICC_ctx,status,ICC_ENTROPY_ESTIMATE,&entropy,sizeof(entropy));
  if( ICC_ERROR != status->majRC) {
    buffer = malloc(1024); /* NOT ICC_VALUESIZE */
    for(i = 0 ; i < 1024*10; i += 1024) {
      ICC_GenerateRandomSeed(ICC_ctx,status,1024,buffer);
      rv |= check_status(status,__FILE__,__LINE__);
      entropy = 0;
      ICC_GetValue(ICC_ctx,status,ICC_ENTROPY_ESTIMATE,&entropy,sizeof(entropy));
      
      printf("%d ",entropy);
      if(entropy < 60) rv |= ICC_ERROR;
    }
    if(ICC_OK == rv) rv = ICC_OSSL_SUCCESS;
    printf("\n");
    if(rv == ICC_OSSL_SUCCESS) {
      printf("TRNG Unit test successfully completed!\n");
    } 
    free(buffer);
  } else {
    printf("TRNG entropy check functionality not implemented\n");
    memset(status,0,sizeof(ICC_STATUS));
  }
  return rv;
}

typedef struct  {
  const SP800_90STATE state;
  const char *txt;
} SP800_90Err2Txt;

SP800_90Err2Txt SPErrs[] = {
  {  SP800_90UNINIT, "Uninitialized" },
  {  SP800_90INIT,   "Initialized" },
  {  SP800_90RUN,    "Running" },
  {  SP800_90SHUT,   "Shutdown" },
  {  SP800_90RESEED, "Needs reseeding" },
  {  SP800_90PARAM,  "Parameter error" },
  {  SP800_90ERROR,  "Error occurred" },
  {  SP800_90CRIT,   "Critical error" }
};

/*! 
  @brief convert an SP800-90 error state to text
  @param state the state to convert to text
  @return text corresponding to the error state
*/
static const char * SPErr2Txt(SP800_90STATE state)
{
  static const char *rv =  "Unknown state";
  if((state >=  SP800_90UNINIT) && (state <= SP800_90CRIT) ) {
    rv = SPErrs[state].txt;
  }
  return rv;
}

/*!
  @brief Test one SP800-90 mode
  @param ICC_ctx an initialized ICC context
  @param algname The name of the algorithm to test
  @return ICC_OSSL_SUCCESS or ICC_OSSL_FAILURE
*/

static int TestOneRNG(ICC_CTX *ICC_ctx, const char *algname)
{
  unsigned char *buffer = NULL;
  SP800_90STATE srv;
  ICC_PRNG *alg = NULL;
  ICC_PRNG_CTX *ctx = NULL;
  char *errtxt = NULL;
  int i = 0;
  int rv = ICC_OSSL_SUCCESS;

  buffer = calloc(1,64);

  printf("\tRNG %-20s ", algname);
  if(NULL == buffer) {
    printf("\tCould not allocate memory.\n");
    rv = ICC_OSSL_FAILURE;
  }
  if(ICC_OSSL_SUCCESS == rv) {
    ctx = ICC_RNG_CTX_new(ICC_ctx);
  }
  if (NULL == ctx)
  {
    printf("\tCould not allocate PRNG context.\n");
    rv = ICC_OSSL_FAILURE;
  }
  alg = ICC_get_RNGbyname(ICC_ctx, algname);
  if (NULL == alg)
  {
    printf("N/A\n");
  }
  else
  {
    if (ICC_OSSL_SUCCESS == rv)
    {
      srv = ICC_RNG_CTX_Init(ICC_ctx, ctx, alg, NULL, 0, 0, 0);
      if (srv != SP800_90INIT)
      {
        ICC_RNG_CTX_ctrl(ICC_ctx, ctx, SP800_90_GETLASTERROR, 0, &errtxt);
        printf("\tInit unexpected state [%s] - [%s]\n", SPErr2Txt(srv), (errtxt ? errtxt : "None"));
        rv = ICC_OSSL_FAILURE;
      }
    }
    if (ICC_OSSL_SUCCESS == rv)
    {
      for (i = 0; i < (2048 / 64); i += 64)
      {
        srv = ICC_RNG_Generate(ICC_ctx, ctx, buffer, 64, NULL, 0);
        if ((srv != SP800_90RUN) && (srv != SP800_90RESEED))
        {
          ICC_RNG_CTX_ctrl(ICC_ctx, ctx, SP800_90_GETLASTERROR, 0, &errtxt);
          printf("\tGenerate unexpected state [%s] [%s]\n", SPErr2Txt(srv), (errtxt ? errtxt : "None"));
          rv = ICC_OSSL_FAILURE;
          break;
        }
      }
    }
    if (ICC_OSSL_SUCCESS == rv)
    {
      srv = ICC_RNG_ReSeed(ICC_ctx, ctx, NULL, 0);
      if ((srv != SP800_90RUN) && (srv != SP800_90RESEED))
      {
        ICC_RNG_CTX_ctrl(ICC_ctx, ctx, SP800_90_GETLASTERROR, 0, &errtxt);
        printf("\tReSeed unexpected state [%s] [%s]\n", SPErr2Txt(srv), (errtxt ? errtxt : "None"));
        rv = ICC_OSSL_FAILURE;
      }
    }
    /* Need to run the self test manually to get sufficient FVT coverage 
       It's run during startup which means it won't
       run automatically again for a long time and
       errors only lock the API persistantly in FIPS mode 
    */
    if (ICC_OSSL_SUCCESS == rv)
    {
      srv = ICC_RNG_CTX_Init(ICC_ctx, ctx, alg, NULL, 0, 0, 0);
      srv = ICC_RNG_CTX_ctrl(ICC_ctx, ctx, SP800_90_SELFTEST, 0, NULL);
      if ((srv == SP800_90ERROR) || (srv == SP800_90CRIT))
      {
        printf("\tSelfTest failed [%s]\n", SPErr2Txt(srv));
        rv = ICC_OSSL_FAILURE;
      }
    }
    printf("\n");
  }
  if (NULL != ctx)
  {
    ICC_RNG_CTX_free(ICC_ctx, ctx);
  }
  if(NULL != buffer) {
    free(buffer);
  }
  return rv;
}
/*! @brief cycle theough the available RNG's
  @param ICC_ctx an initialized ICC Context
  @param status a pointer to an ICC _STATUS struct
  @note 
  - we exclude TRNG_ALT on i5/OS as there's
    no /dev/(u)random here. It'll always fail
  - we run TRNG_ALT as a special case on other
    OS's as it can fail if /dev/urandom is missing
    for example - so - soft fails rather than 
    terminating the test run
*/
int doSP800_90_UnitTest(ICC_CTX *ICC_ctx,ICC_STATUS *status)
{
  int rv = ICC_OSSL_SUCCESS;
  int tv = ICC_OSSL_SUCCESS;

  static const char *alglist[] = 
    {
      "AES-128-ECB","AES-192-ECB","AES-256-ECB",
      "SHA1","SHA224","SHA256","SHA384","SHA512",
      "HMAC-SHA1","HMAC-SHA224","HMAC-SHA256","HMAC-SHA384","HMAC-SHA512",
      "TRNG_FIPS","ETAP_TRNG_FIPS","NOISE_TRNG_FIPS",
      /* No /dev/(u)random on OS400, so skip this on that OS */

      "TRNG_ALT","ETAP_ALT","NOISE_ALT",


      "TRNG_ALT4","ETAP_ALT4","NOISE_ALT4",

      NULL
    };
  int i = 0;


  printf("Starting SP800-90 PRNG unit tests...\n");
  for(i = 0; NULL != alglist[i]; i++) {
    tv =  TestOneRNG(ICC_ctx,alglist[i]);
    if(ICC_OSSL_SUCCESS != tv) {
      rv = tv;
      printf("SP800-90 PRNG unit test failed %s!\n",alglist[i]);
      break;
    }
  }

  if(rv == ICC_OSSL_SUCCESS) {
    printf("SP800-90 PRNG tests successfully completed!\n");
  } 
  return rv;
}

int doKDFTest(ICC_CTX *ICC_ctx)
{
  int rv = ICC_OSSL_SUCCESS;
  ICC_STATUS status;

  static const KDF_TEST alglist[] = 
    {
      {"SHA1-CTR",16},
      {"SHA224-CTR",16},
      {"SHA256-CTR",16},
      {"SHA384-CTR",16},
      {"SHA512-CTR",16},
      {"SHA1-FB",16},
      {"SHA224-FB",16},
      {"SHA256-FB",16},
      {"SHA384-FB",16},
      {"SHA512-FB",16},
      {"SHA1-DP",16},
      {"SHA224-DP",16},
      {"SHA256-DP",16},
      {"SHA384-DP",16},
      {"SHA512-DP",16},
      {"AES-128-CTR",16},
      {"AES-192-CTR",24},
      {"AES-256-CTR",32},
      {"AES-128-FB",16},
      {"AES-192-FB",24},
      {"AES-256-FB",32},
      {"AES-128-DP",16},
      {"AES-192-DP",24},
      {"AES-256-DP",32},
      {"CAMELLIA-128-CTR",16},
      {"CAMELLIA-192-CTR",24},
      {"CAMELLIA-256-CTR",32},
      {"CAMELLIA-128-FB",16},
      {"CAMELLIA-192-FB",24},
      {"CAMELLIA-256-FB",32},
      {"CAMELLIA-128-DP",16},
      {"CAMELLIA-192-DP",24},
      {"CAMELLIA-256-DP",32},
      {NULL,0}
    };
  int i = 0,j = 0, k = 0;
  const ICC_KDF *kdf = NULL;
#define BUFSZ (64*64)
  unsigned char *buffer = NULL;
  int srv = 0;
  int err = 0;
  static const char skey[64] = "0123456789001234567890012345678900123456789001234567890";

  buffer = malloc(BUFSZ);
  printf("Starting SP800-108 KDF unit tests...\n");
  kdf = ICC_SP800_108_get_KDFbyname(ICC_ctx,(char *)"SHA512-CTR"); /* Should ALWAYS be present */
  if(NULL != kdf) {
    for(i = 0; NULL != alglist[i].alg; i++) {
      memset(buffer,0,BUFSZ);
      kdf = ICC_SP800_108_get_KDFbyname(ICC_ctx,(char *)alglist[i].alg);
      if(NULL == kdf) {
	printf("\tKDF %s N/A\n",alglist[i].alg);
	continue;
      } 
      printf("\tKDF %s\n",alglist[i].alg);
      memcpy(buffer,skey,alglist[i].keylen);
      for(j = 0; j < 63; j++) {
	srv = ICC_SP800_108_KDF(ICC_ctx,kdf,
				buffer+(j*alglist[i].keylen),alglist[i].keylen,
				(unsigned char *)"ICC BVT",8,
				(unsigned char *)"ABCDEFG",8,
				buffer + ((j+1)*alglist[i].keylen),
				alglist[i].keylen
				); 
	if(srv != 1 ) {
	  printf("SP800-90 KDF failed for algorithm %s\n",alglist[i].alg);
	  rv = ICC_OSSL_FAILURE;
	  break;
	}
      }
      for(j = 0, err = 0;(err == 0) &&  (j < 64); j++) {
	for(k = j+1;(err == 0) &&  (k < 64) ; k++) {
	  if(memcmp(buffer+(j*alglist[i].keylen),
		    buffer+(k*alglist[i].keylen),
		    alglist[i].keylen) == 0) {
	    printf("SP800-90 KDF failed for algorithm %s (Key repeat)\n",alglist[i].alg);
	    rv = ICC_OSSL_FAILURE;
	    err = 1;
	  }
	}
      }
    }
    if(rv == ICC_OSSL_SUCCESS) {
      printf("SP800-108 PRNG tests successfully completed!\n");
    } 

  } else {
    printf("SP800-108 KDF API not implemented\n");
  }
  /* This is present for induced failure testing */
  if( ICC_GetStatus(ICC_ctx,&status) != ICC_OSSL_SUCCESS) {
    if(status.majRC != ICC_OK) {
      rv = ICC_OSSL_FAILURE;
    }
  }
  if(NULL != buffer) {
    free(buffer);
  }
  return rv;
}

/*!
  @brief do a common subset of the PKCS#8 operations
  - convert an ICC_EVP_PKEY to ICC_PKCS8_PRIV_KEY_INFO
  - convert ICC_PKCS8_PRIV_KEY_INFO to DER
  - convert DER to ICC_PKCS8_PRIV_KEY_INFO
  - convert ICC_PKCS8_PRIV_KEY_INFO to PKEY
  Sanity check where possible
  @param ICC_ctx the ICC context to use
  @param pkey the starting state
  @return 0 on sucess, 1 something bad happened
*/
static int do_P8_subset(ICC_CTX *ICC_ctx,ICC_EVP_PKEY *pkey)
{
  int rv = 0;
  unsigned char *buf = NULL;
  unsigned char *tmp = NULL;
  int bufl = 0;
  ICC_PKCS8_PRIV_KEY_INFO *p8info = NULL;
  ICC_PKCS8_PRIV_KEY_INFO *p8info1 = NULL;
  ICC_EVP_PKEY *pkey1 = NULL;
  
  p8info = ICC_EVP_PKEY2PKCS8(ICC_ctx,pkey);
  if(NULL == p8info) {
    rv = 1;
  }
  if( 0 == rv) {
    bufl =  ICC_i2d_PKCS8_PRIV_KEY_INFO(ICC_ctx,p8info,NULL);
    if( bufl <= 0) {
      rv = 1;
    }
  }
  if( 0 == rv) {
    buf = calloc(1,bufl);
    tmp = buf;
    bufl =  ICC_i2d_PKCS8_PRIV_KEY_INFO(ICC_ctx,p8info,&buf);
  }

  if( 0 == rv ) {
    buf = tmp;
    p8info1 = ICC_d2i_PKCS8_PRIV_KEY_INFO(ICC_ctx,NULL,&buf,bufl);
    if(NULL == p8info1) {
      rv = 1;
    }
  }
  if( 0 == rv ) {
    pkey1 = ICC_EVP_PKCS82PKEY(ICC_ctx,p8info1);
    if(NULL == pkey1) {
      rv = 1;
    }
  }
  if(NULL != p8info) {
    ICC_PKCS8_PRIV_KEY_INFO_free(ICC_ctx,p8info);
  }
  if(NULL != p8info1) {
    ICC_PKCS8_PRIV_KEY_INFO_free(ICC_ctx,p8info1);
  }
  if( NULL != pkey1) {
    ICC_EVP_PKEY_free(ICC_ctx,pkey1);
  }
  if(NULL != tmp) {
    free(tmp);
  }
  return rv;
}
/*!
  @brief
  Test the PKCS#8 private key import/export API
  @param ICC_ctx An ICC context
  @return ICC_OSSL_SUCCESS on success, ICC_OSSL_FAILURE on fail
*/
int doPKCS8Test(ICC_CTX *ICC_ctx)
{
  int rc = ICC_OSSL_SUCCESS;
  ICC_EC_KEY *ec_key = NULL;
  ICC_RSA *rsa = NULL;
  ICC_DSA *dsa = NULL;
  ICC_EVP_PKEY *pkey = NULL;
  ICC_PKCS8_PRIV_KEY_INFO *p8info = NULL;
  int nid = 0;
  unsigned long h = 0;
  int counter = 0;

  /* RSA */
  printf("Starting PKCS#8 unit test...\n");

  rsa = ICC_RSA_generate_key(ICC_ctx,2048,0x10001,NULL,NULL);

  pkey = ICC_EVP_PKEY_new(ICC_ctx);
  ICC_EVP_PKEY_set1_RSA(ICC_ctx,pkey,rsa);
  p8info = ICC_EVP_PKEY2PKCS8(ICC_ctx,pkey);
  if(NULL == p8info) {
    printf("PKCS#8 tests skipped, no PKCS#8 capability\n");
    ICC_RSA_free(ICC_ctx,rsa);
    rsa = NULL;
    ICC_EVP_PKEY_free(ICC_ctx,pkey);
    pkey = NULL;
  } else {
    ICC_PKCS8_PRIV_KEY_INFO_free(ICC_ctx,p8info);
    printf("\tPKCS#8 with RSA\n");
    if(0 != do_P8_subset(ICC_ctx,pkey) ) {
      rc = ICC_OSSL_FAILURE;
      printf("\tPKCS#8 with RSA failed !\n");
    }
    ICC_RSA_free(ICC_ctx,rsa);
    rsa = NULL;
    ICC_EVP_PKEY_free(ICC_ctx,pkey);
    pkey = NULL;

    /* DSA */
    printf("\tPKCS#8 with DSA\n");
    pkey = ICC_EVP_PKEY_new(ICC_ctx);

    dsa = ICC_DSA_generate_parameters(ICC_ctx,1024,(unsigned char *)buf1,20,&counter,&h,NULL,NULL);
   
    ICC_DSA_generate_key(ICC_ctx,dsa);
    ICC_EVP_PKEY_set1_DSA(ICC_ctx,pkey,dsa);
    if(0 != do_P8_subset(ICC_ctx,pkey)) {
      rc = ICC_OSSL_FAILURE;
      printf("\tPKCS#8 with DSA failed !\n");
    }
    ICC_DSA_free(ICC_ctx,dsa);
    dsa = NULL;
    ICC_EVP_PKEY_free(ICC_ctx,pkey);
    pkey = NULL;
    /* EC */
    nid = ICC_OBJ_txt2nid(ICC_ctx,(char *)"secp521r1");
    ec_key = ICC_EC_KEY_new_by_curve_name(ICC_ctx,nid);
    if(NULL != ec_key) {
      printf("\tPKCS#8 with EC\n");
      ICC_EC_KEY_generate_key(ICC_ctx,ec_key);
      pkey = ICC_EVP_PKEY_new(ICC_ctx);
      ICC_EVP_PKEY_set1_EC_KEY(ICC_ctx,pkey,ec_key);
      if(0 != do_P8_subset(ICC_ctx,pkey) ) {
	rc = ICC_OSSL_FAILURE;
	printf("\tPKCS#8 with EC failed !\n");
      }
      ICC_EC_KEY_free(ICC_ctx,ec_key);
      ec_key = NULL;
      ICC_EVP_PKEY_free(ICC_ctx,pkey);
      pkey = NULL;   
    }
  }
  if(rc == ICC_OSSL_SUCCESS) {
    printf("PKCS#8 tests successfully completed!\n");
  } 
  
  return rc;
}
/*!
  @brief
  Test the behaviour of ICC_GenerateRandomSeed() when an error is forced
  This is here to pick up corner case behaviour of GenerateRandomSeed
  WRT the status parameter
  @param ICC_ctx An ICC context
  @return ICC_OSSL_SUCCESS on success, ICC_OSSL_FAILURE on fail
*/
int doGenerateRandomTest(ICC_CTX *ICC_ctx)
{
  int rc = ICC_OSSL_SUCCESS;
  int retcode = ICC_OSSL_SUCCESS;
  char buffer[80];
  char buffer1[80];
  ICC_STATUS status;
  printf("ICC_GenerateRandomSeed() test\n");
  memset(&status,0,sizeof(status));
  memset(buffer,0,sizeof(buffer));
  memset(buffer1,0,sizeof(buffer1));
  retcode = ICC_SelfTest(ICC_ctx,&status);
  if(ICC_OSSL_SUCCESS != retcode) {
    rc = ICC_OSSL_FAILURE;
  }
  check_status(&status,__FILE__,__LINE__);
  ICC_GenerateRandomSeed(ICC_ctx,&status,sizeof(buffer),buffer);
  check_status(&status,__FILE__,__LINE__);
  if((0 == memcmp(buffer,buffer1,sizeof(buffer))) ) {
    rc = ICC_OSSL_FAILURE;
  }
  return rc;
}

/*!
  @brief
  Test for the SP800-38F Key Wrap code
  @param ICC_ctx An ICC context
  @return ICC_OSSL_SUCCESS on success, ICC_OSSL_FAILURE on fail
*/
int doKWTest(ICC_CTX *ICC_ctx)
{
  int rv = ICC_OSSL_SUCCESS;

  unsigned char test[32] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,19,30,31,32};
  unsigned char key[16] =  {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
  unsigned char out[48];
  unsigned char test1[32];
  int outl = 0;



  if(ICC_NOT_IMPLEMENTED !=  ICC_SP800_38F_KW(ICC_ctx,test,32,out,&outl,key,128,ICC_KW_WRAP) ) {
    printf("ICC_SP800_38F_KW() tests\n");
    if(1 != ICC_SP800_38F_KW(ICC_ctx,test,32,out,&outl,key,128,ICC_KW_WRAP)) {
      rv = ICC_OSSL_FAILURE;
    }
    if(1 != ICC_SP800_38F_KW(ICC_ctx,out,40,test1,&outl,key,128,0)) {
      rv = ICC_OSSL_FAILURE;
    }
    if(0 != memcmp(test,test1,sizeof(test)) ) {
      printf("KWE/KUE error\n");
      rv = ICC_OSSL_FAILURE;
    }
    if(1 != ICC_SP800_38F_KW(ICC_ctx,test,32,out,&outl,key,128,ICC_KW_WRAP | ICC_KW_FORWARD_DECRYPT)) {
      rv = ICC_OSSL_FAILURE;
    }
    if(1 != ICC_SP800_38F_KW(ICC_ctx,out,40,test1,&outl,key,128,ICC_KW_FORWARD_DECRYPT)) {
      rv = ICC_OSSL_FAILURE;
    }
    if(0 != memcmp(test,test1,sizeof(test)) ) {
      printf("KWD/KUD error\n");
      rv = ICC_OSSL_FAILURE;
    }


    if(1 != ICC_SP800_38F_KW(ICC_ctx,test,2,out,&outl,key,128,ICC_KW_WRAP | ICC_KW_PAD)) {
      rv = ICC_OSSL_FAILURE;
    }
    if(1 != ICC_SP800_38F_KW(ICC_ctx,out,outl,test1,&outl,key,128,ICC_KW_PAD)) {
      rv = ICC_OSSL_FAILURE;
    }
    if(0 != memcmp(test,test1,2) ) {
      printf("KWEP/KUEP error\n");
      rv = ICC_OSSL_FAILURE;
    }
    if(1 != ICC_SP800_38F_KW(ICC_ctx,test,5,out,&outl,key,128,ICC_KW_WRAP | ICC_KW_FORWARD_DECRYPT | ICC_KW_PAD)) {
      rv = 1;
    }
    if(1 != ICC_SP800_38F_KW(ICC_ctx,out,outl,test1,&outl,key,128,ICC_KW_FORWARD_DECRYPT | ICC_KW_PAD)) {
      rv = 1;
    }
    if(0 != memcmp(test,test1,sizeof(test)) ) {
      printf("KWDP/KUDP error\n");
      rv = 1;
    }

  
    if(rv == ICC_OSSL_SUCCESS) {
      printf("ICC_SP800_38F_KW() tests successfully completed!\n");
    }
  } else {
    printf("ICC_SP800_38F_KW() N/A\n");
  }
  return rv;
}

int ProbeInt(ICC_CTX *icc_ctx,ICC_VALUE_IDS_ENUM e)
{
  unsigned int i = (unsigned int)-1;
  ICC_STATUS sts,*status = &sts;
  int retcode = 0;
  char *tag = "Unknown";
  switch(e) {
  case ICC_RNG_INSTANCES:
    tag = "ICC_RNG_INSTANCES";
    break;
  case ICC_RNG_TUNER:
    tag = "ICC_RNG_TUNER";
    break;
 case ICC_INDUCED_FAILURE:
    tag = "ICC_INDUCED_FAILURE";
    break;
  case ICC_ALLOW_INDUCED:
    tag = "ICC_ALLOW_INDUCED";
    break;
  case ICC_LOOPS:
    tag = "ICC_LOOPS";
    break;
  case ICC_SHIFT:
    tag = "ICC_SHIFT";
    break;
  default:
    break;
  }

  retcode = ICC_GetValue(icc_ctx,status,e,(void *)&i,sizeof(i));
  if(ICC_OK != retcode) {
    printf("CONFIG:%s=FAILED\n",tag);
  } else {
    printf("CONFIG:%s=%u\n",tag,i); 
  }
  return retcode;
}
int ProbeStr(ICC_CTX *icc_ctx,ICC_VALUE_IDS_ENUM e)
{
  char buffer[ICC_VALUESIZE];
  ICC_STATUS sts,*status = &sts;
  int retcode = 0;
  char *tag = "Unknown";

  buffer[0] = 0;

  switch(e) {
  case ICC_RANDOM_GENERATOR:
    tag = "ICC_RANDOM_GENERATOR";
    break;
  case ICC_SEED_GENERATOR:
    tag = "ICC_TRNG";
    break;
  case ICC_CPU_CAPABILITY_MASK:
    tag = "ICC_CAP_MASK";
    break;
  case ICC_INSTALL_PATH:
    tag = "ICC_INSTALL_PATH";
    break;
  default:
    break;
  }

  retcode = ICC_GetValue(icc_ctx,status,e,(void *)buffer,ICC_VALUESIZE);
  if(ICC_OK != retcode) {
    printf("CONFIG:%s=FAILED\n",tag);
  } else {
    printf("CONFIG:%s=%s\n",tag,buffer); 
  }
  return retcode;

}
int doConfigProbes(ICC_CTX *icc_ctx)
{
  printf("Global configuration\n");
  ProbeInt(icc_ctx,ICC_RNG_TUNER);
  ProbeInt(icc_ctx,ICC_RNG_INSTANCES);
  ProbeInt(icc_ctx,ICC_INDUCED_FAILURE);
  ProbeInt(icc_ctx,ICC_ALLOW_INDUCED);
  ProbeStr(icc_ctx,ICC_SEED_GENERATOR);
  ProbeStr(icc_ctx,ICC_RANDOM_GENERATOR);
  ProbeStr(icc_ctx,ICC_CPU_CAPABILITY_MASK);
  ProbeStr(icc_ctx,ICC_INSTALL_PATH);
  if(version > 8.05) {
    ProbeInt(icc_ctx,ICC_SHIFT);
    ProbeInt(icc_ctx,ICC_LOOPS);
  }
  return ICC_OSSL_SUCCESS;
}

/*! @brief test coverage of API added specifically for Java security 
     PKEY section
    @param icc_ctx a valid ICC context
    @return ICC_OSSL_SUCCESS, ICC_OSSL_FAILURE
*/
static int doJavaSecPKEYAPITests(ICC_CTX *icc_ctx)
{
  int rv = ICC_OSSL_SUCCESS;
  int retcode = 0;
  ICC_RSA *rsa = NULL;
  ICC_EVP_PKEY *pkey = NULL;
  ICC_EVP_PKEY_CTX *pctx = NULL;
  size_t outlen = 0;
  int keylen =2048;
  unsigned char *out = NULL;
  memset(buf1,0,sizeof(buf1));
  memset(buf2,0,sizeof(buf2));

  if(version > 8.05) { 
    printf("Aux PKEY function tests\n");
    rsa = ICC_RSA_generate_key(icc_ctx,keylen,0x10001,NULL,NULL);
    pkey = ICC_EVP_PKEY_new(icc_ctx);  
    if( NULL == pkey || NULL == rsa) {
      rv = ICC_OSSL_FAILURE;
      printf("Aux function subsection abort - could not create keys!\n");
    } else {
      retcode = ICC_EVP_PKEY_set1_RSA(icc_ctx,pkey,rsa);
      pctx = ICC_EVP_PKEY_CTX_new(icc_ctx,pkey,NULL);
      retcode = ICC_EVP_PKEY_encrypt_init(icc_ctx,pctx);
      /* EVP_PKEY_CTX_set_rsa_padding(ctx, pad); Is a macro */
      ICC_EVP_PKEY_CTX_ctrl(icc_ctx,pctx,ICC_EVP_PKEY_RSA,-1,ICC_EVP_PKEY_CTRL_RSA_PADDING,ICC_RSA_NO_PADDING,NULL);
      OSSLE(icc_ctx);
      retcode = ICC_EVP_PKEY_encrypt_new(icc_ctx,pctx,NULL,&outlen,buf2,keylen/8);
      out = malloc(outlen);
      retcode = ICC_EVP_PKEY_encrypt_new(icc_ctx,pctx,out,&outlen,buf2,keylen/8);

      OSSLE(icc_ctx);
      retcode = ICC_EVP_PKEY_decrypt_init(icc_ctx,pctx);
      ICC_EVP_PKEY_CTX_ctrl(icc_ctx,pctx,ICC_EVP_PKEY_RSA,-1,ICC_EVP_PKEY_CTRL_RSA_PADDING,ICC_RSA_NO_PADDING,NULL);

      OSSLE(icc_ctx);
      retcode = ICC_EVP_PKEY_decrypt_new(icc_ctx,pctx,out,&outlen,buf1,outlen);
      OSSLE(icc_ctx); 
    }
    if(NULL != out) {
      free(out);
    }
    if(NULL != pctx) {
      ICC_EVP_PKEY_CTX_free(icc_ctx,pctx);
    }
    if(NULL != pkey) {
      ICC_EVP_PKEY_free(icc_ctx,pkey);
    }
    if(NULL != rsa) {
      ICC_RSA_free(icc_ctx,rsa);
    }
  }
  /* Muzzle the compiler warning that retcode is unused */
  if(0 == retcode) {
    rv |= retcode;
  }
  return rv;
}
/*! @brief test coverage of API added specifically for Java security
     ECDSA section
    @param icc_ctx a valid ICC context
    @return ICC_OSSL_SUCCESS, ICC_OSSL_FAILURE
*/
int doJavaSecECDSAAPITests(ICC_CTX *icc_ctx)
{
  int rv = ICC_OSSL_SUCCESS;
  int retcode = 0;
  ICC_EC_KEY *ec_key = NULL;
  ICC_ECDSA_SIG *ec_sig = NULL;
  ICC_BIGNUM *bn1 = NULL;
  ICC_BIGNUM *bn2 = NULL;
  ICC_BN_CTX *bn_ctx = NULL;
  static const unsigned char dgst[20] = {1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,1};
  int nid = -1;


  if(version > 8.05) {
    printf("Aux ECDSA function tests\n");
    if(ICC_OSSL_SUCCESS == rv) {
      bn_ctx = ICC_BN_CTX_new(icc_ctx);
      nid = ICC_OBJ_txt2nid(icc_ctx,(char *)"secp521r1");
      ec_key = ICC_EC_KEY_new_by_curve_name(icc_ctx,nid);
      if(NULL == ec_key || NULL == bn_ctx) {
	rv = ICC_OSSL_FAILURE;
      } else {
	do {
	  retcode = OpenSSLError(icc_ctx,__LINE__);
	} while(0 != retcode);
	retcode = ICC_EC_KEY_generate_key(icc_ctx,ec_key);  

	retcode = ICC_ECDSA_sign_setup(icc_ctx,ec_key,bn_ctx,&bn1,&bn2);
	OSSLE(icc_ctx);

	ec_sig = ICC_ECDSA_do_sign_ex(icc_ctx,dgst,sizeof(dgst),bn1,bn2,ec_key);
	OSSLE(icc_ctx);
	retcode = ICC_ECDSA_do_verify(icc_ctx,dgst,sizeof(dgst),(const ICC_ECDSA_SIG *)ec_sig,ec_key);

	OSSLE(icc_ctx);
      }
      if(retcode != ICC_OSSL_SUCCESS) {
	rv = ICC_OSSL_FAILURE;
      }
    }
    if(NULL != bn_ctx) {
      ICC_BN_CTX_free(icc_ctx,bn_ctx);
    }
    if(NULL != bn1) {
      ICC_BN_clear_free(icc_ctx,bn1);

    }
    if(NULL != bn2) {
      ICC_BN_clear_free(icc_ctx,bn2);
    } 
    if(NULL != ec_sig) {
      ICC_ECDSA_SIG_free(icc_ctx,ec_sig);
    }
    if(NULL != ec_key) {
      ICC_EC_KEY_free(icc_ctx,ec_key);
    }
  }
  return rv;
}
/*!
  @brief test ICC with simultaneous FIPS/non-FIPS contexts open
  @return ICC_OSSL_SUCCESS, ICC_OSSL_FAILURE
*/
int doDualTest() {
  ICC_STATUS * status = NULL;
  ICC_CTX *ICC_ctx = NULL;
  ICC_STATUS * status1 = NULL;
  ICC_CTX *ICC_ctx1 = NULL;
  int retcode = 0;
  static char *path = NULL;
  char value[ICC_VALUESIZE];

#if !defined(ICCPKG)
  path = "../package";
#endif   
  printf("Simultaneous context Test\n");
  

  status = (ICC_STATUS*)calloc(1,sizeof(ICC_STATUS));
  status1 = (ICC_STATUS*)calloc(1,sizeof(ICC_STATUS));
  ICC_ctx = ICC_Init(status,path);
  retcode = ICC_SetValue(ICC_ctx,status,ICC_FIPS_APPROVED_MODE,"on");  
  ICC_ctx1 = ICC_Init(status1,path);
  retcode = ICC_SetValue(ICC_ctx1,status,ICC_FIPS_APPROVED_MODE,"off");  
  retcode = ICC_Attach(ICC_ctx,status);
  retcode = ICC_Attach(ICC_ctx1,status1);

  retcode = ICC_GetValue(ICC_ctx,status,ICC_VERSION,value,ICC_VALUESIZE);
  printf("ICC #1 version %s\n",value);
  print_cfg(ICC_ctx,"    ");
  retcode = ICC_GetValue(ICC_ctx1,status,ICC_VERSION,value,ICC_VALUESIZE);
  printf("ICC #2 version %s\n",value);
  print_cfg(ICC_ctx1,"    ");
  ICC_Cleanup(ICC_ctx,status);
  ICC_Cleanup(ICC_ctx1,status1);

  free(status);
  free(status1);
  if(ICC_OK == retcode) {
    retcode = ICC_OSSL_SUCCESS;
  }
  return retcode;
}
int runTest(ICC_CTX *ICC_ctx,ICC_STATUS *status,int testnum)
{
  switch (testnum) {
  case -1:
    break;
  case 1:
    if(doEVPUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
      {
	printf("EVP unit test failed!\n");
	testnum = -1;
      }
    else testnum++;
    break;
     
  case 2:
    if(doCryptoUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
      {
	printf("Crypto unit test failed!\n");
	testnum = -1;		
      }
    else testnum++;
    break;
	  
  case 3:
    if(doRandUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
      {
	printf("Rand unit test failed!\n");
	testnum = -1;		
      }
    else testnum++;
    break;
  case 4:
    if(doKeyUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
      {
	printf("Key unit test failed!\n");
	testnum = -1;		
      }
    else testnum++;
    break;
  case 5:
    if(doErrorUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
      {
	printf("Error unit test failed!\n");
	testnum = -1;		
      }
    else testnum++;
    break;
  case 6:
    if(doDESUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
      {
	printf("DES unit test failed!\n");
	testnum = -1;		
      }
    else testnum++;
    break;
  case 7:
	
    if(doBNUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
      {
	printf("BIGNUM unit test failed!\n");
	testnum = -1;		
      }
    else testnum++;
    break;
	
  case 8:
    if(doHMACUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
      {
	printf("HMAC unit test failed!\n");
	testnum = -1;		
      }
    else testnum++;
    break;
	  
  case 9:
    if(doCMACUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
      {
	printf("CMAC unit test failed!\n");
	testnum = -1;		
      }
    else testnum++;
    break;
  case 10:
    if(doAES_CCMUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
      {
	printf("AES_CCM unit test failed!\n");
	testnum = -1;		
      }
    else testnum++;
    break;
  case 11:
    if(doTRNGTest(ICC_ctx,status) != ICC_OSSL_SUCCESS) 
      {
	printf("TRNG unit test failed!\n");
	testnum = -1;
      } 
    else testnum ++;
    break;
  case 12:
    if(doAES_GCMUnitTest(ICC_ctx) != ICC_OSSL_SUCCESS)
      {
	printf("AES_GCM unit test failed!\n");
	testnum = -1;		
      }
    else testnum++;
    break;
  case 13:
    if(doSP800_90_UnitTest(ICC_ctx,status) != ICC_OSSL_SUCCESS)
      {
	printf("SP800-90 PRNG unit test failed!\n");
	testnum = -1;		
      }
    else testnum++;
    break;	  
  case 14:
    if(doEC_KEYTest(ICC_ctx) != ICC_OSSL_SUCCESS) {
      printf("EC_KEY unit test failed!\n");
      testnum = -1;
    } else testnum ++;
    break;	
  case 15:
    if(doECDHTest(ICC_ctx) != ICC_OSSL_SUCCESS) {
      printf("ECDH unit test failed!\n");
      testnum = -1;
    } else testnum ++;
    break;
  case 16:
    if(doECDSATest(ICC_ctx) != ICC_OSSL_SUCCESS) {
      printf("ECDSA unit test failed!\n");
      testnum = -1;
    } else testnum ++;
    break;

  case 17:
    if(doPKCS8Test(ICC_ctx) != ICC_OSSL_SUCCESS) {
      printf("PKCS#8 unit test failed!\n");
      testnum = -1;
    } else testnum ++;
    break;

  case 18:
    if(doDualTest() != ICC_OSSL_SUCCESS) {
      printf("Simultaneous context Test failed!\n");
      testnum = -1;
    } else testnum ++;
    break;
  case 19:
    if(doKDFTest(ICC_ctx) != ICC_OSSL_SUCCESS) {
      printf("SP800-108 key derivation Test failed!\n");
      testnum = -1;
    } else testnum ++;
  case 20:
    if(doGenerateRandomTest(ICC_ctx) != ICC_OSSL_SUCCESS) {
      printf("ICC_GenerateRandom() Test failed!\n");
      testnum = -1;
    } else testnum ++;
    break;
  case 21:
    if(doKWTest(ICC_ctx) != ICC_OSSL_SUCCESS) {
      printf("ICC_SP800_38F_KW() Test failed!\n");
      testnum = -1;
    } else testnum ++;
    break;
  case 22:
    if(doConfigProbes(ICC_ctx) != ICC_OSSL_SUCCESS) {
      printf("Config probes failed!\n");
      testnum = -1;
    } else testnum ++;
    break;
 case 23:
    if(doJavaSecPKEYAPITests(ICC_ctx) != ICC_OSSL_SUCCESS) {
      printf("Aux PKEY function test failed!\n");
      testnum = -1;
    } else testnum++;
    break;
  case 24:
    if(doJavaSecECDSAAPITests(ICC_ctx) != ICC_OSSL_SUCCESS) {
      printf("Aux ECDSA function test failed!\n");
      testnum = -1;
    } else testnum++;
    break;
  default:
    testnum = 0;
    break;
  }
  return testnum;
}


/* This is also 256 bits, and passes the NIST KA tests */
#define TEST_PRNG_NAME "SHA256"
/* This one passes KA tests, but is too weak for FIPS 201 use */
#define BAD_TEST_PRNG_NAME "DSS_3_1_SHA"

/* Fallback TRNG method */
#define TEST_TRNG_NAME "TRNG_ALT"

/*! @brief
  Initialization for the ICC1.9+ pre-initialization 
  setup modes. Memory callbacks, RNG's, induced failure tests
  @note this is done unconditionally and returns no errors
  we can't get the ICC version this early ....
  so this WILL fail against ICCPKG
*/
#if defined(BUILD_AS_LIB)
#if defined(_WIN32)
_declspec(dllexport)
#endif
#endif
int doPreInit() {
  ICC_STATUS stat, *status = &stat;
  int rv = ICC_OK;
  static const int rngs = N_RNGS;

  char *env = NULL;

  env = getenv("ICC_INDUCED_FAILURE");
  if (NULL != env) {
    icc_failure = (unsigned int)atoi(env);
    printf("Testing ICC failure paths [%d]\n",
           ((icc_failure < 1000) ? icc_failure : icc_failure - 1000));

    /* For the test case code,setting values  with 1000 added with
       set the failure flag post-startup
    */
    if (icc_failure > 1000) {
      allow_at_runtime = 1;
    }
  }

  env = getenv("ICC_TRNG");
  if ((NULL != env) && (0 == strcasecmp("TRNG_HW", env))) {
    printf("Testing with TRNG_HW\n");
    strcpy(env_trng, "TRNG_HW");
  } else if ((NULL != env) && (0 == strcasecmp("TRNG_OS", env))) {
    printf("Testing with TRNG_OS\n");
    strcpy(env_trng, "TRNG_OS");
  } else if ((NULL != env) && (0 == strcasecmp("TRNG_FIPS", env))) {
    printf("Testing with TRNG_FIPS\n");
    strcpy(env_trng, "TRNG_FIPS");
  }
  check_stack(0);
  if (0 != strcmp(env_trng, "")) {
    ICC_SetValue(NULL, status, ICC_SEED_GENERATOR, (const void *)env_trng);
  }
  ICC_SetValue(NULL, status, ICC_RANDOM_GENERATOR,
               (const void *)TEST_PRNG_NAME);
  ICC_SetValue(NULL, status, ICC_RNG_INSTANCES, (const void *)&rngs);
  if (0 != tuner) {
    ICC_SetValue(NULL, status, ICC_RNG_TUNER, (const void *)&tuner);
  }
  check_stack(1);
  return rv;
}

int doPostStartupTest(ICC_CTX *ICC_ctx, ICC_STATUS *status) {
  int rv = ICC_OK;
  int retcode;
  char value[ICC_VALUESIZE];
  char value1[9]; /* Deliberately broken */

  value1[0] = '\0';
  check_stack(0);
  /* This SHOULD return an error, but caused a segv on earlier ICC's */
  retcode = ICC_GetValue(ICC_ctx, status, ICC_INSTALL_PATH, (void *)value1, 9);
  if (retcode == ICC_OK) {
    printf("ICC vulnerable to buffer overrun in ICC_GetValue() - expect a "
           "crash [%s]\n",
           value1);
    rv = ICC_ERROR;
  }
  retcode = ICC_SetValue(ICC_ctx, status, ICC_INSTALL_PATH, (void *)value1);
#if 0
  if( retcode == ICC_OK) {
    printf("ICC vulnerable to invalid ICC_SetValue() - expect a crash [%s]\n",value1);
    rv = ICC_ERROR;
  }
#endif
  value[0] = '\0';
  retcode = ICC_GetValue(ICC_ctx, status, ICC_INSTALL_PATH, (void *)value,
                         ICC_VALUESIZE);
  if (retcode != ICC_ERROR) {
#if defined(_WIN32)
    if (is_unicode) {
      wprintf(L"ICC loaded from: [%s]\n", (value ? ((wchar_t*)value) : L"NULL"));
    } else
#endif
    {
      printf("ICC loaded from: [%s]\n", value);
    }
  } else {
    printf("Could not get ICC load path\n");
    rv = ICC_ERROR;
  }
  check_stack(1);
  check_stack(0);
  if ((ICC_OK == rv) && (version >= 1.09)) {
    entropy1 = 0;
    ICC_GetValue(ICC_ctx, status, ICC_ENTROPY_ESTIMATE, &entropy1,
                 sizeof(entropy1));
    check_status(status, __FILE__, __LINE__);
    retcode = ICC_SelfTest(ICC_ctx, status);
    if (ICC_OSSL_SUCCESS != retcode) {
      printf("SelfTest failed\n");
      rv = ICC_ERROR;
    }
    if (ICC_OK == rv) {
      retcode = ICC_SetValue(ICC_ctx, status, ICC_RANDOM_GENERATOR,
                             (char *)"HMAC-SHA256");
      if (retcode == ICC_OSSL_SUCCESS) {
        printf("Attempted to set the PRNG with ICC in an invalid state - "
               "didn't fail\n");
      }
      if (0 != strcmp(env_trng, "")) {
        retcode = ICC_SetValue(NULL, status, ICC_SEED_GENERATOR, (const void *)env_trng);
        if (retcode == ICC_OSSL_SUCCESS) {
          printf("Attempted to set the TRNG with ICC in an invalid state - "
                 "didn't fail\n");
        }
      }
    }
    check_stack(1);
    check_stack(0);
    if (ICC_OK == rv) {
      value[0] = '\0';

      ICC_GetValue(ICC_ctx, status, ICC_RANDOM_GENERATOR, value, 20);
      rv |= check_status(status, __FILE__, __LINE__);
      if (0 != strcmp(value, TEST_PRNG_NAME)) {
        printf("PRNG expected %s, got %s\n", TEST_PRNG_NAME, value);
      }

      printf("Check PRNG %s\n", (rv == ICC_OK) ? "pass" : "fail");
    }
    check_stack(1);
    check_stack(0);
    if (ICC_OK == rv) {
      value[0] = '\0';
      check_stack(0);
      ICC_GetValue(ICC_ctx, status, ICC_SEED_GENERATOR, value, 20);
      check_stack(1);
      check_status(status, __FILE__, __LINE__);
      printf("Check TRNG %s\n", (rv == ICC_OK) ? "pass" : "fail");
      if (0 != strcmp(env_trng, "")) {
        if (0 != strcmp(value, env_trng)) {
          printf("TRNG expected %s, got %s\n", env_trng, value);
        }
      }
    }
    check_stack(1);
    check_stack(0);
  }
  /* Attempt to print out the CPU capability flags */
  if ((ICC_OK == rv) && (version > 8.0)) {
    value[0] = '\0';
    /* This one should fail */
    if (ICC_OK ==
        ICC_GetValue(ICC_ctx, status, ICC_CPU_CAPABILITY_MASK, value, 8)) {
      rv = ICC_ERROR;
      printf("Attempted to read CPU capability mask into too small a field - "
             "suceeded - this is bad!\n");
    } else {
      ICC_GetValue(ICC_ctx, status, ICC_CPU_CAPABILITY_MASK, value, 17);
      check_status(status, __FILE__, __LINE__);
      if (ICC_OK == rv && strlen(value)) {
        printf("CPU flags %s\n", value);
      } else {
        printf("CPU flags not available\n");
      }
    }
  }
  check_stack(1);
  if ((ICC_OK == rv) && (version > 8.02)) {
    int nrngs = 1;
    ICC_GetValue(ICC_ctx, status, ICC_RNG_INSTANCES, (void *)&nrngs,
                 sizeof(nrngs));
    printf("RNG instances %d\n", nrngs ? nrngs : 1);
  }
  check_stack(1);
  if ((ICC_OK == rv) && (version > 8.02)) {
    int nrngs = 1;
    ICC_GetValue(ICC_ctx, status, ICC_RNG_TUNER, (void *)&nrngs, sizeof(nrngs));
    printf("RNG tuner %s\n", (nrngs == 2) ? "Estimator" : "Heuristic");
  }
  check_stack(1);
  return rv;
}

#if defined(BUILD_AS_LIB)
#if defined(_WIN32)
_declspec(dllexport)
#endif
#endif
int doUnitTest(int test,char *fips, int unicode)
{
  int rv = ICC_OSSL_SUCCESS;
  int error = 0;
  ICC_STATUS * status = NULL;
  ICC_CTX *ICC_ctx = NULL;
  ICC_STATUS * status1 = NULL;
  ICC_CTX *ICC_ctx1 = NULL;
  int retcode = 0, testnum = 1;
  char* value = NULL;
  static char *path = NULL;
  static char tmp[ICC_VALUESIZE];
#if defined(_WIN32)
  static wchar_t *wpath = NULL;
#endif

#if !defined(ICCPKG)
  path = "../package";
#   if defined(_WIN32)
  wpath = L"../package";
#   endif
#endif
  fips_mode = 0;
  status = (ICC_STATUS*)calloc(1,sizeof(ICC_STATUS));
  status1 = (ICC_STATUS*)calloc(1,sizeof(ICC_STATUS));
  value = (char *)malloc(ICC_VALUESIZE);
  if ((NULL != status) && (NULL != status1) && (NULL != value))
  {
    ICC_ctx1 = ICC_Init(status1, path);

    /*Initialize ICC*/
#if defined(_WIN32)
    if (1 == unicode)
    {
      wprintf(L"Loading and Initializing ICC from unicode path: [%s]\n", wpath ? wpath : L"NULL (ICCPKG)");
      check_stack(0);
      is_unicode = 1;
      ICC_ctx = ICC_InitW(status, wpath);
    }
    else
#endif
    {
      printf("Loading and Initializing ICC with: [%s]\n", path ? path : "NULL (ICCPKG)");
      check_stack(0);
      ICC_ctx = ICC_Init(status, path);
    }

    check_stack(1);
    check_status(status, __FILE__, __LINE__);

    if (NULL == ICC_ctx)
    {
      error = 1;
      printf("ICC_Init failed - NULL returned - ICC shared library missing or not loadable ?\n");
      rv = ICC_OSSL_FAILURE;
    }

    if (!error && (0 == strcmp(fips, "on")))
    {

      retcode = ICC_SetValue(ICC_ctx, status, ICC_FIPS_APPROVED_MODE, fips);
    }

    if (!error)
    {
      check_stack(0);

      retcode = ICC_Attach(ICC_ctx, status);
      check_status(status, __FILE__, __LINE__);
      switch (retcode)
      {
      case ICC_OSSL_FAILURE:
      case ICC_FAILURE:
        rv = ICC_OSSL_FAILURE;
        error = 1;
        break;
      default:
        break;
      }
    }
    check_stack(1);
    if (!error)
    {
      retcode = ICC_GetValue(ICC_ctx, status, ICC_FIPS_APPROVED_MODE, tmp, ICC_VALUESIZE);
      if ((0 == strcmp(tmp, "on")))
      {
        fips_mode = 1;
      }
    }
    /* counter-intuitive ICC_Init is a macro in non-FIPS versions 
       If we run through check_status() here we produce an error 
       which the build scanning scripts pick up when this is supposed
       to fail.
  */
    if (!error)
    {
      retcode = ICC_GetStatus(ICC_ctx, status);
      if (fips_mode && !(status->mode & ICC_FIPS_FLAG))
      {
        printf("Couldn't enter FIPS mode. Is this is a non-FIPS build [%d]?\n", status->mode);
        error = 1;
      }
      if (retcode != ICC_OSSL_SUCCESS)
      {
        error = 1;
        rv = ICC_OSSL_FAILURE;
        ICC_Cleanup(ICC_ctx, status);
        printf("\tCheck handling of errors in ICC_Attach()\n");
        ICC_ctx = ICC_Init(status, path);
        if (NULL != ICC_ctx)
        {
          printf("\t\tError. ICC_Attach() failure should be persistant (return code from ICC_Init()) \n");
        }
        if (ICC_OK == status->majRC)
        {
          printf("\t\tError. ICC_Attach() failure should be persistant (status)\n");
        }
        check_status(status, __FILE__, __LINE__);
      }
      else
      {
        value[0] = '\0';
        retcode = ICC_GetValue(ICC_ctx, status, ICC_VERSION, value, ICC_VALUESIZE);
        check_stack(1);
        version = VString2(value);
        printf("ICC version %s\n", value);
        print_cfg(ICC_ctx, "    ");
      }
      check_stack(0);
      check_stack(1);
    }
    if (!error && (ICC_OK != doPostStartupTest(ICC_ctx, status)))
    {
      error = 1;
      rv = ICC_OSSL_FAILURE;
      printf("Post Startup state invalid!\n");
    }
    /* If we are doing extended - post startup testing, we've already
     set the flag to allow this post ICC_Attach()
     so set the appropriate flag late 
  */
    if (!error)
    {
      if (allow_at_runtime && (icc_failure > 1000))
      {
        icc_failure = icc_failure - 1000;
        if (ICC_OK != ICC_SetValue(ICC_ctx, status, ICC_INDUCED_FAILURE, (const void *)&icc_failure))
        {
          rv = ICC_OSSL_FAILURE;
        }
        check_status(status, __FILE__, __LINE__);
      }

      printf("Starting ICC unit test...FIPS %s\n", fips);

      /*Start Module Testing*/
      if (test == 0)
      {
        testnum = 1;
        while (testnum > 0)
        {
          if(testnum == exclude) {
            testnum = testnum + 1;
          } else {
          OSSLE(ICC_ctx);
          testnum = runTest(ICC_ctx, status, testnum);
        }
      }
      }
      else
      {
        testnum = 0;
        runTest(ICC_ctx, status, test);
      }

      if (testnum == 0)
      {
        printf("ICC Unit test successfully completed FIPS %s!\n", fips);
      }
      else
      {
        error = 1;
        rv = ICC_OSSL_FAILURE;
        printf("Error occurred in ICC unit test!\n");
      }

      if (version >= 1.09)
      {
        entropy2 = 0;
        ICC_GetValue(ICC_ctx, status, ICC_ENTROPY_ESTIMATE, &entropy2, sizeof(entropy2));
        check_status(status, __FILE__, __LINE__);
      }
    }

    if (version >= 1.09)
    {
      printf("Entropy estimates: Initial %d, final %d\n", entropy1, entropy2);
    }
    if (!error && (ICC_OSSL_SUCCESS != ICC_Attach(ICC_ctx1, status1)))
    {
      error = 1;
    }
    /* Perform the FIPS integrity check */
    if(NULL != ICC_ctx) {
      ICC_IntegrityCheck(ICC_ctx,status);
    }
    check_status(status, __FILE__, __LINE__);
  }
  else
  {
    rv = ICC_FAILURE;
  }
  /* Cleanup test artifacts from early ICC 1.9 multiple Init crash test */
  if(NULL != ICC_ctx1) {
    ICC_Cleanup(ICC_ctx1,status1);
    ICC_ctx1 = NULL;
  }
  if(NULL != status1) {
    free(status1);
    status1 = NULL;
  }

  /* clean up leaks caused by errors being tripped during testing */
  if(NULL != ICC_ctx) {
    ICC_Cleanup(ICC_ctx,status);
    ICC_ctx = NULL;
  }
#if defined(MEMCK_ON)
  if(vers >= 1.09) {
    printf("Memory status %ld malloc %ld realloc %ld free, total malloced %ld\n",nallocs,nreallocs,nfrees,(unsigned long)allocsz);
  }
#endif
  if(NULL != status) {
    free(status);
  }
  if(NULL != value) {
    free(value);
  }

  return rv;
}

/* from iccversion.h*/
#ifndef ICC_GIT_BRANCH
#define ICC_GIT_BRANCH         "n/a"
#endif
#ifndef ICC_GIT_HASH
#define ICC_GIT_HASH           "n/a"
#endif

static void usage(char *prgname,char *text)
{
   static const char* ICC_vinfo =
   {
     "@(#)GIT_BRANCH:       " ICC_GIT_BRANCH "\n"
     "@(#)GIT_HASH :        " ICC_GIT_HASH "\n"
   };

  printf("%s\n", ICC_vinfo);  printf("Usage: %s [n]/[-u]/[-h]\n",prgname);
  printf("       %s runs the ICC BVT tests, this covers most of the API\n",prgname
	 );
  printf("           note that correct usage of the ICC API is not guaranteed\n");
  printf("       n = a single test number to run\n");
  printf("       -t <num> RNG tuning algorithm, 0 = unset, 1 = heuristic, 2 = estimate\n");
  printf("       -x <num> a single test to exclude\n");
  printf("       -u<nicode> start ICC with a Unicode path (Windows only)\n");
  printf("       -h this text\n");
  if(NULL != text) {
    printf("\n%s\n",text);
  }
}
/* Also used as a shared library during FVT 
   that's simpler in setup and doesn't take the 
   same arguments
*/
#if !defined(BUILD_AS_LIB)  
int main(int argc, char *argv[])
{
  int test = 0;
  int unicode = 0;
  int argi = 1;
  while(argc > argi  ) {
    if(strncmp("-u",argv[argi],2) == 0) {
      unicode = 1;;
    } else if(strncmp("-h",argv[argi],2) == 0) {
      usage(argv[0],NULL);
      exit(0);
    } else if(strncmp("-t",argv[argi],2) == 0) {
      if(argc > (argi+1)) {
	tuner = atoi(argv[argi+1]);
	argi++;
      } else {
	tuner = 2;
      }
    } else if(strncmp("-x",argv[argi],2) == 0) {
      if(argc > (argi+1)) {
	      exclude = atoi(argv[argi+1]);
	      argi++;
      }
    } else {
      test = atoi(argv[argi]);
    }
    argi++;
  }
  if(!unicode) {
    if( ICC_OK != doPreInit()) {
      printf("ICC pre-initialization tests failed!\n");
      return 1;
    }
  }
  if (ICC_OSSL_SUCCESS != doUnitTest(test,"on",unicode)) {
    printf("ICC unit test failed - FIPS mode!\n");
    return 1;
  }
  printf("\n\n\n");
  if ( ICC_OSSL_SUCCESS != doUnitTest(test,"off",unicode)) {
    printf("ICC unit test failed - non-FIPS mode!\n");
    return 1;
  }


#if defined(STACK_DEBUG)
  printf("Stack usage ~ %d bytes\n",max_stack);
#endif
  printf("ICC unit test program exiting successfully !\n");
  x_memdump();
  return 0;
}
#endif