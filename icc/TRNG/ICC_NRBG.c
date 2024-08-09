/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
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
unsigned int fips_loops();

#if 0
/* debug enabled */
#define debug(x) x
#else
#define debug(x)
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


/*  In non-FIPS, all platforms default to TRNG_OS and upgrade to TRNG_HW at runtime if available.
    This is done to prioritise compatibility on the unpredictable range and age of the virtualisatised systems we might run on,
    while still upgrading and using TRNG_HW in most cases.
*/

#if (NON_FIPS_ICC == 1) /* Built as non-FIPS */

/* These definitions try mirror the availability of OPENSSL_HW_rand to avoid a mismatch (not relevant when we use TRNG_OS) */
/* X86 Linux and Windows, Solaris x86 */
#if     (defined(__i386)   || defined(__i386__)   || defined(_M_IX86) || \
         defined(__INTEL__) ||                                          \
         defined(__x86_64) || defined(__x86_64__) || defined(_M_AMD64) ) && ( !(defined(__SunOS) && !defined(__amd64)) \
         )
  static int global_trng_type_attempted_upgrade = 0;
  static TRNG_TYPE global_trng_type = TRNG_OS;

#elif defined(__s390__) || defined(__MVS__)
  static int global_trng_type_attempted_upgrade = 0;
  static TRNG_TYPE global_trng_type = TRNG_OS;

#elif defined(__ppc__) || defined(__powerpc__) || defined(_AIX)
  /* We will do a runtime check for cpu support for darn, present since ISA3.0, and update to TRNG_HW if so */
  static int global_trng_type_attempted_upgrade = 0;
  static TRNG_TYPE global_trng_type = TRNG_OS;

/* The default/fallback for platforms without supported hardware TRNG */
#else
  static TRNG_TYPE global_trng_type = TRNG_OS;
#endif

#else /* Built as FIPS */
  static TRNG_TYPE global_trng_type = TRNG_FIPS;
#endif

/* If a user explicitly sets a TRNG, we don't want to upgrade even if HW is available */
int global_trng_type_user_set = 0;

static void TRNG_LocalCleanup(TRNG *T);
int fips_rand_bytes(unsigned char *buffer, int num);

/* 
  TRNG works by measuring jitter between instruction execution and a CPU clock. While entropy does
  vary by CPU clock that's a second order effect (the clock rate went up because load was higher and the load
  is the entropy source). We find the fastest moving bit in a timer register then estimate how many instructions
  we need to delay between reads that it gets perturbed.

  NOTE: This array MUST match exactly with the definitions of TRNG_TYPE in noise_to_entropy.h
  NOTE: SetDefaultTrng must also be updated if adding new TRNGs

*/  
ENTROPY_IMPL TRNG_ARRAY[] = {
  {
/* used to be TRNG_ALT4 */
    "TRNG_HW",
    TRNG_HW,
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
/* used to be TRNG_ALT */
    "TRNG_OS",
    TRNG_OS,
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
    "TRNG_FIPS",        /*!< Common name */
    TRNG_FIPS,          /*!< Enum used internally */  
    4,                  /*!< Number of bits needed to produce nominally one bit of entropy after compression */           
    TRNG_FIPS_getbytes, /*!< Callback to a buffer of entropy data */
    TRNG_FIPS_Init,     /*!< Callback for TRNG Initialization */
    TRNG_FIPS_Cleanup,  /*!< Callback for TRNG Cleanup */
    TRNG_FIPS_preinit,  /*!< Callback for (global) setup for this TYPE of entropy source */
    TRNG_FIPS_Avail,    /*!< availability */
    NULL,
    1
  }
};

#define NTRNGS  (sizeof(TRNG_ARRAY)/sizeof(ENTROPY_IMPL))

char* TRNG_ALIAS[][2] = {
  {"TRNG_ALT4", "TRNG_HW"},
  {"TRNG_ALT3", "TRNG_HW"},
  {"TRNG_ALT2", "TRNG_HW"}, /* If hw not available, fallback to OS. */
  {"TRNG_ALT",  "TRNG_OS"},
  {"TRNG_TRNG", "TRNG_HW"}  /* If hw not available, fallback to OS. */
};

#define NALIAS (sizeof(TRNG_ALIAS) / sizeof(TRNG_ALIAS[0]))

/*!
  @brief Error reporting for hardware migration/virtualisation/low entropy
  @param msg The error message
  @param file the file the error originated from
  @param line the line the error was triggered from  
  @return TRNG_ENTROPY (2) indicating unrecoverable error.
*/
TRNG_ERRORS SetRNGError(const char *msg, const char *file, int line)
{
  TRNG_ERRORS rv = TRNG_ENTROPY; /* If this function has been called, it is currently considered unrecoverable */
  char buffer[256];

  snprintf(buffer,255,"%s %s:%d",TRNG_ARRAY[global_trng_type].name,file,line);
  MARK("Unrecoverable TRNG Error ", buffer);
  SetFatalError(msg,file,line);
  
  return rv;
}




/*! @brief get the name of the indexed NRBG
  @param trng TRNG_TYPE
  @return The TRNG name
*/

const char * GetTRNGNameR(TRNG_TYPE trng) {
  const char *rv = "Invalid";
  if(trng >= 0 && trng <= NTRNGS) {
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

/* Iterates TRNG_ALIAS and modifies the name of a given TRNG */
void checkTRNGAlias(char **trngname) {
  
  int i = 0;
  for(i = 0; i < NALIAS; i++) 
  {
    if(0 == strcasecmp(*trngname, TRNG_ALIAS[i][0])) 
    {
      
      *trngname = TRNG_ALIAS[i][1]; /* Set new name */

      if (0 == strcasecmp("TRNG_ALT2", TRNG_ALIAS[i][0]) || 0 == strcasecmp("TRNG_TRNG", TRNG_ALIAS[i][0]))
      {
        if(0 == strcasecmp("TRNG_HW", *trngname) && !TRNG_ARRAY[TRNG_HW].avail())
        {
          /* If it isn't available and we want to fallback for some specific TRNGs */
           *trngname = "TRNG_OS";
        }
      }
      MARK("TRNG aliased to", *trngname);
    }
  }
}

/*!< type of the global TRNG used by OpenSSL via callbacks
  This can only be set before the global TRNG is actually instantiated
*/

extern unsigned icc_failure; /*!< Trigger for induced failure tests */
int SetTRNGName(char *trngname)
{
  if (NULL != trngname) {
    MARK("Request to set TRNG to", trngname);
  }
  else {
    MARK("Request to set NULL TRNG", "");
  }
  int rv = 0;
  int i = 0;
  checkTRNGAlias(&trngname);
  for (i = 0; i < TRNG_count(); i++)
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
  debug(printf("SetDefaultTRNG(%d)\n",trng));
  switch (trng) {
  case TRNG_OS:
  case TRNG_HW:
  case TRNG_FIPS:
    if(TRNG_ARRAY[trng].avail()) {
      MARK("TRNG set to", TRNG_ARRAY[trng].name);
      global_trng_type = trng;
      global_trng_type_user_set = 1;
    } else {
      MARK("TRNG attempted to be set to", TRNG_ARRAY[trng].name);
    }
    break;
  default:
    /* The most predictable behaviour is to do nothing if the specified TRNG doesn't exist
       The source of truth for the default TRNG is how it is initialised as a static variable */
    break;
  }
  debug(printf("SetDefaultTRNG asked for %s to %s\n",GetTRNGNameR(trng), GetTRNGName()));
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
        debug(printf("TRNG_ESourceInit:avail=0\n"));
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
    memset(es,0,sizeof(E_SOURCE));
  }
}
/*! @brief return the NRBG type that's the default within ICC and OpenSSL
    will attempt to upgrade to TRNG_HW for platforms where hardware could be available but isn't always.
    I expect we continue to initialise a TRNG using the current method: `TRNG_new(GetDefaultTrng());`
    @return The global NRBG type
 */

TRNG_TYPE GetDefaultTrng()
{
#if (NON_FIPS_ICC == 1)

#if     (\
          (( defined(__i386)   || defined(__i386__)   || defined(_M_IX86) || \
            defined(__INTEL__) || \
            defined(__x86_64) || defined(__x86_64__) || defined(_M_AMD64)) && (!(defined(__SunOS) && !defined(__amd64)))) \
          || \
          ( defined(__s390__) || defined(__MVS__)) \
          || \
          ( defined(__ppc__) || defined(__powerpc__) || defined(_AIX)) \
        )
  if(!global_trng_type_attempted_upgrade) {
    MARK("Testing the availability of TRNG_HW", "");

    if(0 == global_trng_type_user_set) {
      if (TRNG_FIPS != global_trng_type) {
        if (ALT4_Avail()) {
          MARK("Found, switching to TRNG_HW", "");
    global_trng_type = TRNG_HW;
        } else {
          MARK("TRNG_HW not available, remaining with", TRNG_ARRAY[global_trng_type].name);  
        }
      } else {
        MARK("TRNG_FIPS set, remaining with", TRNG_ARRAY[global_trng_type].name);
      }
    } else {
      MARK("User TRNG set, remaining with", TRNG_ARRAY[global_trng_type].name);
  }
  global_trng_type_attempted_upgrade = 1;
  }

#endif /*x86_64, z/architecture, power */
#endif /*non-FIPS*/
  return global_trng_type;
}
/*!
  @brief return a TRNG context
  @return an uninitialized TRNG context or NULL if one couldn't be allocated
*/
TRNG *TRNG_new(TRNG_TYPE type)
{
  TRNG *t = NULL;
  TRNG_ERRORS e;
  debug(printf("Requesting new TRNG of type %d\n",type));
  t = (TRNG *)ICC_Calloc(1, sizeof(TRNG), __FILE__, __LINE__);
  if(NULL != t) {

    e = TRNG_TRNG_Init(t,type);
    if(TRNG_OK != e) {
      debug(printf("Error TRNG_TRNG_Init(t,%d) = %d\n", type, e));
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
    memset(T, 0, sizeof(TRNG));
  }
}
/*!
  @brief Initialize a TRNG of the specified type
  @param T an allocated TRNG data block
  @param type the type of the TRNG
  - TRNG_FIPS conditioned event counter. The number of loops required to give high
         jitter in event counter samples is used and the delta's are filtered with a histogram sort
         to ensure they came only from a noise source then filtered.
  - TRNG_ALT An external PRNG (/dev/(u)random) is used as a well distributed
          stream of data
          Used when:
         - tuning is difficult due to the machine architecture
         - low intrinsic entropy (virtualization)
  - TRNG_ALT4 Hardware RBG with normal tests, useful when virtualized because you may as well
    trust the hardware source here.      
  @return 1 on success, 0 on failure
*/
TRNG_ERRORS TRNG_TRNG_Init(TRNG *T, TRNG_TYPE type) {

  debug(printf("TRNG_TRNG_Init(T, %d)\n", type));
  TRNG_ERRORS rv = TRNG_OK;
  unsigned char *tmp = NULL;
  int tmpl = 0;

  unsigned int e_exp = 0; /* % entropy in the noise at the INPUT of the TRNG core, calced from the bits/byte in TRNG_TYPE */

  if( (type < 0 ) || (type > NTRNGS) ){
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
    if (rv != TRNG_OK) {
      MARK("TRNG_TRNG_Init:TRNG_ESourceInit failed:",TRNG_ARRAY[type].name);
    }

    /* 
      and the 50% entropy we guarantee at output.       
    */
    if (TRNG_OK == rv) {
      int e = ht_Init(&(T->ht),50);
      if (1 != e) {
        rv = TRNG_INIT;
      }
    }
  } else {
    rv = TRNG_INIT;
  }
  if (TRNG_OK == rv) {
    memset(T->lastdigest,0,sizeof(T->lastdigest));
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
  /* \FIPS Initialize the HMAC key, 0 is permissable.
    We can't use the TRNG itself as that's what we
    are initializing.
  */
  if (TRNG_OK == rv) {
    memset(T->cond.key, 0, sizeof(T->cond.key));
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
    /* TRNG retained data */
    Personalize(tmp);
    xcompress(T, T->cond.rdata, tmp, tmpl);
    memset(tmp, 0, tmpl);
    ICC_Free(tmp);
    tmp = NULL;
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

/*! @brief Return the loop count used by the software TRNG 
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
      break;
  }
  return l;
}
