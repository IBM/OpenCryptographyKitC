/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: ICC specific TRNG initialization
//
*************************************************************************/

#define ICC_HEADER
#include <stdio.h>

#if defined(_MSC_VER)
#define snprintf _snprintf
#endif

#include "icc.h"

#include "platform.h"
#include "platform_api.h"
#include "tracer.h"
#include "openssl/evp.h"
#include "ICC_NRBG.h"
#include "entropy_estimator.h"
#include "fips-prng/fips-prng-err.h"
#include "induced.h"

/* Circular dependency */
unsigned int nonfips_loops();
unsigned int fips_loops();

#if defined FAST_TEST
#define ONE_WEEK 600.0 
#define ONE_HOUR 36.0   
#else
#define ONE_WEEK 604800.0
#define ONE_HOUR 3600.0
#endif

#include "TRNG/entropy_to_NRBGi.h"
/* Wrong prototype but it should cope with NULL,NULL inputs */
extern void iccDoKnownAnswer(void *iccLib, void *icc_stat);
extern void SetFatalError(const char *msg, const char *file, int line);
extern int FIPS_mode(); /* In OpenSSL, gets set by ICC during startup */

/*! \FIPS This is the number of times we'll retry getting data from the TRNG
  if the continuous test fails. It's only a number >1 because the API allows
  very short requests, and we store/compare a hash, not the data.
*/
#define TRNG_RETRIES 5
 /* Lookup for number of bits/byte (Index 0-8) to required entropy level, note that we are more conservative than NIST 
   0 is a throwaway value simple to avoid the whole (-1) thing
 */
static const unsigned int E_GuarTo_Ein[9] = {100,100,50,50,25,25,25,25,25};
static const char * TRNG_IMPLtag = "TRNG_IMPL";
static const char * TRNG_CONDtag = "TRNG_COND";
static const char * TRNGtag = "TRNG_t";
static const char * TRNG_ESRCtag = "E_SOURCE";

/*
  List of usable NRBG types. This is used in the resiliance code
*/
typedef struct NRBG_type_t {
  const char *name; /*!< NRBG name, NULL if unused */
  time_t timestamp; /*!< Time it was initialized */
  int initialized;  /*!< Flag to say it was used */
} NRBG_type;

/*
  List of NRBG's, the last being unusable is convienent
*/


static TRNG_TYPE global_trng_type = TRNG_ALT;

int rerunSelfTest = 0;

static void TRNG_LocalCleanup(TRNG *T);
int fips_rand_bytes(unsigned char *buffer, int num);

static ICC_Mutex GlobalRNGMutex;

/* 
  
  There's some awkwardness here because although we say we need 4x the data TRNG does our entropy test doesn't
  have the resolution to cope with 12.5% so that's set at 25%. i.e. E_GuarTo_Ein[]

  NOTE: This array MUST match exactly with the definitions of TRNG_TYPE in noise_to_entropy.h

*/  
ENTROPY_IMPL TRNG_ARRAY[] = {
  {
    "TRNG_ALT",
    TRNG_ALT,
    2,
    ALT_getbytes,
    ALT_Init,
    ALT_Cleanup,
    ALT_preinit,
    ALT_Avail,
    NULL,
    0
  },
  {
    "TRNG_ALT4", 
    TRNG_ALT4, 
    2, 
    ALT4_getbytes, 
    ALT4_Init, 
    ALT4_Cleanup,
    ALT4_preinit,
    ALT4_Avail,
    NULL,
    0
  },
   {
    "TRNG_FIPS",       /*!< Common name */
    TRNG_FIPS,          /*!< Enum used internally */  
    4, /*!< Number of bits needed to produce nominally one bit of entropy after compression */           
    TRNG_FIPS_getbytes, /*!< Callback to a buffer of entropy data */
    TRNG_FIPS_Init,    /*!< Callback for TRNG Initialization */
    TRNG_FIPS_Cleanup, /*!< Callback for TRNG Cleanup */
    TRNG_FIPS_preinit, /*!< Callback for (global) setup for this TYPE of entropy source */
    TRNG_FIPS_Avail,   /*!< availability */
    NULL,
    1
  },
};

#define NTRNGS  (sizeof(TRNG_ARRAY)/sizeof(ENTROPY_IMPL))

static NRBG_type NRBG_list[NTRNGS];
/*!< 
  @brief Re-run POST because our RBG configuration changed.
  @param file trace information, source file for call
  @param line trace information, line number of call
  @note Self Test will be triggered by the next call to the either of the RBG thread pools
*/
void ReRunSelfTest(const char *file, int line) {

  if (rerunSelfTest == 1) {
    IN();
    MARK("Asked to rerun SelfTest",file);
    iccDoKnownAnswer(NULL, NULL);
    OUTRC(line);
  }

}
/*!
  @brief Error recovery from hardware migration/virtualisation 
  @param msg The error message
  @param file the file the error originated from
  @param line the line the error was triggered from  
*/
void SetRNGError(const char *msg,const char *file, int line)
{
  int rc = 0;
  ICC_LockMutex(&GlobalRNGMutex);
  MARK("TRNG Error ", TRNG_ARRAY[global_trng_type].name);
  rc = NextNRBGinList();
  /* Mark that we need to re-run self test without locks held */
  if(0 == rerunSelfTest) {
    rerunSelfTest = 1; 
  }
  ICC_UnlockMutex(&GlobalRNGMutex);
  if (-1 == rc) {
    MARK("Unrecoverable TRNG Error ", TRNG_ARRAY[global_trng_type].name);
    SetFatalError(msg,file,line);
  } else {
    MARK("TRNG recovery with ", TRNG_ARRAY[global_trng_type].name);   
  }
}




/*! @brief get the name of the indexed NRBG
  @param trng TRNG_TYPE
  @return The TRNG name
*/

const char * GetTRNGNameR(TRNG_TYPE trng) {
  const char *rv = "Invalid";
  if(trng >= 0 && trng < NTRNGS) {
    rv = (const char *)TRNG_ARRAY[trng].name;
  }
  return rv;
}


/*! @brief Return the number of available NRBG's
    @return the number of available NRBG's
*/
int TRNG_count() { return (int)NTRNGS; }


/*! @brief return the FIPS status of a trng
  @param trng TRNG_TYPE
  @return 1 if FIPS allowed
*/
int isFipsTrng(TRNG_TYPE trng)
{
  int rv = 0;
  rv = TRNG_ARRAY[trng].fips;
  return rv;
}
/*! 
  @brief return the name of TRNG ICC uses
  @return The name of TRNG ICC uses
*/
const char *GetTRNGName()
{
  /* printf("GetTRNGName: global type %d, name %s\n",global_trng_type,GetTRNGNameR(global_trng_type)); */
  return GetTRNGNameR(global_trng_type);
}

/*!< type of the global TRNG used by OpenSSL via callbacks
  This can only be set before the global TRNG is actually instantiated
*/

extern unsigned icc_failure; /*!< Trigger for induced failure tests */
int SetTRNGName(char *trngname)
{
  int rv = 0;
  int i = 0;
  for (i = 1; i < TRNG_count(); i++)
  {
    if (0 == strcasecmp(trngname,TRNG_ARRAY[i].name))
    {
      SetDefaultTrng(TRNG_ARRAY[i].type);
      if (TRNG_ARRAY[i].type == (int)GetDefaultTrng()) {
         rv = 1;
      }
      break;
    }
  }

  return rv;
}
/*!
  @brief Set the default TRNG
  @return the TRNG in use
  @note this can only be done internally and does change the TRNG's seeding any PRNG's in play
        BEFORE they are next used.
*/
TRNG_TYPE SetDefaultTrng(TRNG_TYPE trng) {
  switch (trng) {
  case TRNG_ALT:
  case TRNG_ALT4:
  case TRNG_FIPS:
    if(TRNG_ARRAY[trng].avail()) {
      global_trng_type = trng;
    }
    break;
  default:
    global_trng_type = TRNG_ALT;
    break;
  }
  /* printf("SetDefaultTRNG asked for %s to %s\n",GetTRNGNameR(trng), GetTRNGName()); */
  return global_trng_type;
}
/* 
  Create a TRNG conditioner (entropy copmpression) context. Key, HMAC CTX, working buffer
  Note that this assumes the key has been set up 
*/
static TRNG_ERRORS TRNG_CondInit(TRNG_COND *c,const EVP_MD *md)
{
  TRNG_ERRORS rv = TRNG_INIT;
  unsigned int mlen = SHA_DIGEST_SIZE;

  if (NULL != c) {
    if (NULL == c->hctx) {
      c->hctx = HMAC_CTX_new();
    }
    if (NULL != c->hctx) {
      /* Create the conditioning key from the random data fed in earlier */
      HMAC_Init_ex(c->hctx, c->key, sizeof(c->key),md, NULL);
      HMAC_Update(c->hctx, c->rdata, sizeof(c->rdata));
      HMAC_Final(c->hctx, c->key, &mlen);
      HMAC_CTX_reset(c->hctx);
      c->id = TRNG_CONDtag;
      rv = TRNG_OK;
    }
  }
  return rv;
}

static void TRNG_CondCleanup(TRNG_COND *c)
{
  if (NULL != c) {
    if (NULL != c->hctx) {
      HMAC_CTX_free(c->hctx);
      c->hctx = NULL;
      c->id = NULL;
    }
  }
}
/* 
  Initialise a TRNG entropy testing structure
*/
static TRNG_ERRORS TRNG_ESourceInit(E_SOURCE *es,int e_exp)
{
  TRNG_ERRORS rv = TRNG_OK;
  if(NULL != es) {
    memset(es->nbuf,0,sizeof(es->nbuf));
    es->cnt = 0;
    if(NULL != es->impl.avail) {
      if( 0 == (es->impl.avail())) {
        rv = TRNG_INIT;
      }
    }
    if(TRNG_OK == rv) {
      if (1 != ht_Init(&(es->hti),e_exp)) {
        rv = TRNG_INIT;
      }
    }
    if (TRNG_OK == rv) {
      /* Do any global initialization required for this TRNG type
         any 'do it once' code is below this as there may be multiple
         TRNG instances of this type
       */
      if ((NULL != es->impl.preinit)) {
        (es->impl.preinit)(0);
      }
    }
    if (TRNG_OK == rv) {
      /* Run the optional per-type initialization */
      if (NULL != es->impl.init) {
        rv = (es->impl.init)(es,NULL,0);
      } else {
        rv = TRNG_INIT;
      }    
    }
    if (TRNG_OK == rv) {
      es->id = TRNG_ESRCtag;
    }
  }
  return rv;
}

static void TRNG_ESourceCleanup(E_SOURCE *es)
{
  if(NULL != es) {
    if(NULL != es->impl.cleanup) {
      (es->impl.cleanup)(es);
    }
    ICC_securezero(es,sizeof(E_SOURCE));
  }
}
/*! @brief return the NRBG type that's the default within ICC and OpenSSL
    @return The global NRBG type
 */
         
TRNG_TYPE GetDefaultTrng() { return global_trng_type; }
/*!
  @brief return a TRNG context
  @return an uninitialized TRNG context or NULL if one couldn't be allocated
*/
TRNG *TRNG_new(TRNG_TYPE type) {
  TRNG *t = NULL;
  /* printf("Requesting new TRNG of type %d\n",type); */
  t = (TRNG *)ICC_Calloc(1, sizeof(TRNG), __FILE__, __LINE__);
  if(NULL != t) {
    if(TRNG_OK != TRNG_TRNG_Init(t,type)) {
      TRNG_LocalCleanup(t);
      t = NULL;
    }

  }
  return t;
}
/* Return the entropy guarantee, actually the reciprocal of
   how many bytes are required to produce on byte of entropy
*/
unsigned int TRNG_guarantee(TRNG *T)
{
  unsigned int rv = 2;
  if(T && (T->type < NTRNGS)) {
    rv =  TRNG_ARRAY[T->type].e_guarantee;
  }
  return rv;
}

/*!
  @brief Clean up a TRNG context.
    - Free associated data structures (which scrub their state)
    - Scrub internal state
    - free data structure
  @param T The TRNG context to scrub
  @note Checking that state is in fact scrubbed is part of the Self Test of these objects
*/ 
void TRNG_LocalCleanup(TRNG *T) 
{

  if(NULL != T) {
    TRNG_ESourceCleanup(&(T->econd));
    if (NULL != T->md_ctx) {
      EVP_MD_CTX_free(T->md_ctx);
      T->md_ctx = NULL;
    }
    /* Clean up the conditioner */
    TRNG_CondCleanup(&(T->cond));
    /* Clean up the long term test on the TRNG health */
    CleanupEntropyEstimator(T);
    /* Erase it all */
    ICC_securezero(T, sizeof(TRNG));
  }
}
/*!
  @brief Initialize a TRNG of the specified type
  @param T an allocated TRNG data block
  @param type the type of the TRNG
  - TRNG conditioned event counter. The number of loops required to give high
         jitter in event counter samples is used and the delta's in event
         counter timing is are collected, compressed and used directly as an
  entropy source
  - TRNG_FIPS conditioned event counter. The number of loops required to give high
         jitter in event counter samples is used and the delta's are filtered with a histogram sort
         to ensure they came only from a noise source then filtered.
  - TRNG_ALT An external PRNG (/dev/(u)random) is used as a well distributed
          stream of data and mixed with event counter samples to provide resistaance to attacks 
          on that source
          Used when:
         - tuning is difficult due to the machine architecture
         - low intrinsic entropy (virtualization)
  - TRNG_ALT4 Hardware RBG with normal tests, useful when virtualized because you may as well
    trust the hardware source here.      
  @return 1 on success, 0 on failure
*/
TRNG_ERRORS TRNG_TRNG_Init(TRNG *T, TRNG_TYPE type) {
  TRNG_ERRORS rv = TRNG_OK;
  unsigned char *tmp = NULL;
  int tmpl = 0;

  unsigned int e_exp = 0; /* % entropy in the noise at the INPUT of the TRNG core, calced from the bits/byte in TRNG_TYPE */

  if( (type < 0 ) || (type >= NTRNGS) ){
    type = global_trng_type;
  }

  if (NULL != T) {
    TRNG_LocalCleanup(T);
    e_exp = E_GuarTo_Ein[TRNG_ARRAY[type].e_guarantee];


    /* Set up the implementation from the templates we have
     */
    if (TRNG_OK == rv) {
      memcpy(&(T->econd.impl), &(TRNG_ARRAY[type]), sizeof(T->econd.impl));
      T->econd.impl.id = TRNG_IMPLtag;
    }

    /* Setup and initialize the method with whatever input entropy it specifies */
    rv = TRNG_ESourceInit(&(T->econd),e_exp);
    /* 
      and the 50% entropy we guarantee at output.       
    */
    if (TRNG_OK == rv) {
      if (1 != ht_Init(&(T->ht),50)) {
        rv = TRNG_INIT;
      }
    }
  } else {
    rv = TRNG_INIT;
  }
  if (TRNG_OK == rv) {
    ICC_securezero(T->lastdigest,sizeof(T->lastdigest));
    if (NULL == T->md) {
      T->md = EVP_get_digestbyname(TRNG_DIGEST);
    }
    if (NULL == T->md) {
      rv = TRNG_INIT;
    }
  }
  if (TRNG_OK == rv) {
    if (NULL == T->md_ctx) {
      T->md_ctx = EVP_MD_CTX_new();
    }
    if (NULL == T->md_ctx) {
      rv = TRNG_INIT;
    }
  }
  if(TRNG_OK == rv) {
    rv = InitEntropyEstimator(T);
  }
  /* \FIPS Initialize the HMAC key with whatever noise we 
    have available. We can't use the TRNG itself as that's what we
    are initializing.
  */
  if (TRNG_OK == rv) {
    getbytes(T->cond.key,sizeof(T->cond.key));
  }
  if(TRNG_OK == rv) {
    /* Initialize the TRNG compressor */
    rv = TRNG_CondInit(&(T->cond),T->md);
  }
  /** \FIPS Initialize the TRNG with a time/date
      based personalization string.
      - We reuse the SP800-90 personalization routine
      the time/date fields PID/TID are early in the data so we do pick those up
  */
  /* Set up personalization data */
  if (TRNG_OK == rv) {

    tmpl = Personalize(NULL);
    tmp = ICC_Calloc(1, tmpl, __FILE__, __LINE__);
    if (NULL == tmp) {
      rv = TRNG_INIT;
    } else {
    /* TRNG retained data */
    Personalize(tmp);
    xcompress(T, T->cond.rdata, tmp, tmpl);
      ICC_securezero(tmp, tmpl);
    ICC_Free(tmp);
    tmp = NULL;
  }
  }
  if (TRNG_OK == rv) {
    T->initialized = 1;
    T->id = TRNGtag;
    T->type = type;
  } else {
    TRNG_LocalCleanup(T);
  }
  return rv;
}

/*!
  @brief
  Clear/free a TRNG context
  @param T a context to free
*/
void TRNG_free(TRNG *T) {
  if (NULL != T) {
    TRNG_LocalCleanup(T);
    ICC_Free(T);
  }
}

/*!
 @brief
  This is the code path ICC uses INTERNALLY for seeds, i.e. ones
  fed into the SP800-90 PRNG's
  @param T TRNG context
  @param seedLength requested seeding data
  @param seed pointer to the buffer to hold the seed data
  @return TRNG_OK on sucess, error indication otherwise
*/

TRNG_ERRORS TRNG_GenerateRandomSeed(TRNG *T, int seedLength, void *seed) {
  TRNG_ERRORS rv = TRNG_OK;
  rv = Entropy_to_TRNG(T, seed, seedLength);
  return rv;
}

/* @brief ICC_GenerateRandsomSeed calls this
  @param num number of bytes requested
  @buf the buffer to copy the random data to
  @return Number of bytes (== num) on sucess
  @note We bypass OpenSSL here to avoid calling down
  into OpenSSL then back out to our thread pool. 
  Performance basically.
*/
int my_GenerateRandomSeed(int num, unsigned char *buf) {
  return fips_rand_bytes((void *)buf, num);
}

/*! \FIPS
  We now have to cope with hot migration in virtualized environments.
  Data for our NRBG recovery strategy.
  Create a circular list of available modes (i.e. if
  there's no hardware TRNG_ALT4 can't be used)
  On failure we check elapsed time. >=1 week we assume it's
  an infrequent recoverable problem and simply lock 
  re-run the initialization (tuning) 
  and  reinstantiate the system RNG's with the same NRBG type.
  <1 week we move to the next mode on the list, if it hasn't been
  used before as above. Otherwise if it was last used less
  than an hour ago we are in deep trouble and flag a fatal error.

  Basically if we have an option that's usable it will end up
  selected, but we aren't going to continually thrash either.
*/



/*!
  @brief Initialize the list of available
  NRBG's. This is part of the error recovery strategy
  @note, locks are assumed held elsewhere when this is
  called. (During library load via RAND_FIPS_init())
*/
void InitNRBGList() 
{
  int i = 0;

  ICC_CreateMutex(&GlobalRNGMutex);

  for (i = 0; i < NTRNGS; i++) { /* Last NRBG type is unusable */
    NRBG_list[i].name = NULL;
    if (0 == TRNG_ARRAY[i].avail()) { /* Check that it's available  */
      continue;
    }  
    if( FIPS_mode() ) { /* Actually set down in OpenSSL */
      if(!isFipsTrng((TRNG_ARRAY[i].type))) {
        continue;
      }
    }
    /* Note, we compare pointers not strings
       So NRBG_list[i].name be static const strings
    */
    NRBG_list[i].name = (const char *)TRNG_ARRAY[i].name;
  }
  NRBG_list[global_trng_type].timestamp = time(NULL);
}

/*! @brief Return the NRBG to use next
     reinitialize RNG pool
     re-run known answer tests
  Called in the event of an NRBG error
  @return the current global trng type or -1 on error
  @note Locks must be held before calling this

*/
int NextNRBGinList() 
{
  time_t ct;
  int i = 0;

  ct = time(NULL);
  i = global_trng_type;

  /* Were we working properly for > 1 week ? */
  if (difftime(ct, NRBG_list[global_trng_type].timestamp) > ONE_WEEK) {
    /* Yes so simply reinitialize the pool in the same mode */   
  } else {
    i++;
    i %= NTRNGS;
    if(0 == i) {
      i = 1;
    }
    if (NULL == NRBG_list[i].name) { /* End of usable TRNG's in list */
      i = 0;
    }
    if (!NRBG_list[i].initialized) {
        /* No, so just mark it as used */
      NRBG_list[i].initialized = 1;
    } else {
      /* Yes, how recently ? */
      if (difftime(ct, NRBG_list[i].timestamp) < ONE_HOUR) {
        MARK("NRBG types cycling unacceptably quickly, gave up on ",NRBG_list[i].name);
        i = -1;
      }
    }
  }
  if(i >= 0) {
    /* Make sure any system wide setup is done (force tuning etc) */
    TRNG_ARRAY[i].preinit(1);
    global_trng_type = i;
    /* Set the timestamp */ 
    NRBG_list[global_trng_type].timestamp = ct;
  }
  return i;
}
/*! @brief
  Cleanup any residual state from the NRBG resilience code
  All that needs cleaning up is the mutex protecting the data
  structures as they don't hold actual RBG data, just types
  (Called at exit via fips_rand_cleanup())
*/
void CleanupNRBGList(void) 
{ 
  ICC_DestroyMutex(&GlobalRNGMutex);
}
/*! @brief Return the loop count used by the default TRNG 
    @return loop count
*/

unsigned int Loops()
{
  TRNG_TYPE x;
  unsigned int l = 0;
  x = GetDefaultTrng();
  switch(x) {
    case TRNG_FIPS:
      l = fips_loops();
      break;
    default:
      l = nonfips_loops();
      break;
  }
  return l;
}
