/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Implementation of the upper API levels of SP800-90 RNG modes
//
*************************************************************************/


/*!
   Thread safety. The PRNG "object" is not safe to share
   across threads as it contains retained state. 
   Use one/thread.
*/

#if 0
/* debug enabled */
#define debug(x) x
#else
#define debug(x)
#endif

/*
#define DEBUG_PRNG 1
*/
#ifdef _WIN32
#    if _MSC_VER<1600
#include <stddef.h>
#else
#include <stdint.h>
#endif
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#endif
#include "icclib.h"
#include "ds.h"
#include "fips.h"
#include "SP800-90.h"
#include "SP800-90i.h"
#include "status.h"
#include "utils.h"
#include "TRNG/ICC_NRBG.h"
#include "induced.h"


extern SP800_90STATE TRNG_Inst_Type(PRNG_CTX *ctx,
                                    unsigned char *ein, unsigned int einl,
                                    unsigned char *nonce, unsigned int nonl,
                                    unsigned char *person,unsigned int perl,
                                    TRNG_TYPE type
                                    );

static const char C01[4] = {0x00,0x00,0x00,0x01};

/*! \IMPLEMENT SP800-90 Implementation
  The SP800_90 code is an API for RNG's, not just an
  implementation of the SP800_90 standard.
  <p>
  Also exposed via this API are the older X9.31 RNG's and
  the ICC TRNG.
  <p>
  The API is designed for usability, so in normal use by
  ICC consumers features such as self test/health checks and
  reseeding are transparent.
  <p>
  SP800_90.c contains the API wrapper code, self test code,
  state transition logic and parameter checking which is common
  to all underlying algorithms.
  <p>
  The "methods" in the other files contain only the bare
  cryptograhic code necessary for function.
  <ul>
  <li>SP800-90HashData.c (SHA PRNG's)
  <li>SP800-90HMAC.c (HMAC PRNG's)
  <li>SP800-90Cipher.c (Cipher based PRNG's)
  <li>SP800-90TRNG.c (TRNG)
  <li>SP800-90_X931.c (Older X9.31 PRNG's)
  </ul>
  Conformance testing of the SP800-90 code is 
  in two parts.
  <p>
  SP800_90_test.c contains behavoural tests, i.e. are
  range checks on input data performed correctly,
  do the RNG's reseed when they should.
  <p>
  SP800_90VS.c contains the formal NIST known answer 
  code. These tests are driven via the low level methods and 
  as these don't handle range checking of data they can only 
  be used for validation of "correct" data.
  <p> Note that ICC allows the RBG's used by OpenSSL to
  be changed, see ICC_SetValue and "ICC_RANDOM_GENERATOR", 
  but only before ICC is first instantiated.
  The mode chosen must be one of the FIPS approved modes,
  and muust provide 256 bits of security strength.
  <p> This option is NOT intended to be used in normal operation,
  it's only there so we have a fallback at the application level
  without needing recertification if the chosen default RBG is shown
  to have security problems.
  <p> Note also that ICC_get_RNGbyname() will only return FIPS approved
  PRNG types (of any security strength) when used with a FIPS mode content.
  <p> ICC_get_RNGbyname() will only return RBG's that are functional, i.e.
  passed self test. If hardware is required, it's present.
  <p> Note that by default the PRNG's run in Auto mode, 
  i.e. reseeding from the TRNG is automatic, and large requests are 
  satisfied by breaking them up into a sucession of allowable sized requests. 
  This can be changed between Instantiate and the first call to the PRNG
*/

/*!
  @brief
  Forward declarations for the algorithm methods supported by ICC
  Implementations are elsewhere
*/
extern SP800_90PRNG_t 
  sha1PRNG,        /*!< SP800-90 SHA1 */
  sha224PRNG,      /*!< SP800-90 SHA224 */
  sha256PRNG,      /*!< SP800-90 SHA256 */
  sha384PRNG,      /*!< SP800-90 SHA384 */
  sha512PRNG,      /*!< SP800-90 SHA512 */
  aes128PRNG,      /*!< SP800-90 AES-128 */
  aes192PRNG,      /*!< SP900-90 AES-192 */
  aes256PRNG,      /*!< SP900-90 AES-192 */
  HMACsha1PRNG,    /*!< SP800-90 HMAC SHA1 */
  HMACsha224PRNG,  /*!< SP800-90 HMAC SHA224 */
  HMACsha256PRNG,  /*!< SP800-90 HMAC SHA256 */
  HMACsha384PRNG,  /*!< SP800-90 HMAC SHA384 */
  HMACsha512PRNG,  /*!< SP800-90 HMAC SHA512 */
  SPTRNG_FIPS,     /*!< FIPS TRNG */
  SPTRNG_FIPS_NOISE,  /*!< FIPS noise source test tap */ 
  SPTRNG_FIPS_ETAP,   /*!< FIPS entropy test tap */
  SPTRNG_ALT,       /*!< ALT RBG */
  SPTRNG_ALT_NOISE,  /*!< ALT nouse test tap */
  SPTRNG_ALT_ETAP,   /*!< ALT Entropy test tap */
  SPTRNG_OS,
  SPTRNG_OS_NOISE,
  SPTRNG_OS_ETAP,
  SPTRNG_ALT4,       /*!< ALT4 RBG */
  SPTRNG_ALT4_NOISE, /*!< ALT4 noise source test tap */
  SPTRNG_ALT4_ETAP,  /*!< ALT4 Entropy test tap */
  SPTRNG_HW,
  SPTRNG_HW_NOISE,
  SPTRNG_HW_ETAP
  ;
/*!
  @brief the list of RNG's supported by this API
  Used by ICC_get_RNGbyname()
*/

static SP800_90PRNG_t *PRNG_list[] =
    {
        &SPTRNG_FIPS,
        &SPTRNG_FIPS_NOISE,
        &SPTRNG_FIPS_ETAP,
        &sha1PRNG, &sha224PRNG, &sha256PRNG, &sha384PRNG, &sha512PRNG,
        &aes128PRNG, &aes192PRNG, &aes256PRNG,
        &HMACsha1PRNG, &HMACsha224PRNG, &HMACsha256PRNG, &HMACsha384PRNG, &HMACsha512PRNG,
        &SPTRNG_ALT,
        &SPTRNG_OS,
        &SPTRNG_ALT4,
        &SPTRNG_HW,
        &SPTRNG_ALT_ETAP,
        &SPTRNG_OS_ETAP,
        &SPTRNG_ALT4_ETAP,
        &SPTRNG_HW_ETAP,
        &SPTRNG_ALT_NOISE,
        &SPTRNG_OS_NOISE,
        &SPTRNG_ALT4_NOISE, /* Note, ALT4 requires hardware */
        &SPTRNG_HW_NOISE,
        NULL
    };


static char *exclude_list = "";

static TRNG_TYPE typeofTRNG(SP800_90PRNG_mode mode)
{
  TRNG_TYPE rv;

  switch (mode)  {
  case SP800_TRNG_FIPS:
  case SP800_TRNG_FIPS_NOISE:
  case SP800_TRNG_FIPS_ETAP:
    rv = TRNG_FIPS;
    break;
  case SP800_TRNG_ALT:
  case SP800_TRNG_ALT_NOISE:
  case SP800_TRNG_ALT_ETAP:
  case SP800_TRNG_OS:
  case SP800_TRNG_OS_NOISE:
  case SP800_TRNG_OS_ETAP:
    rv = TRNG_OS;
    break;
  case SP800_TRNG_ALT4:
  case SP800_TRNG_ALT4_NOISE:
  case SP800_TRNG_ALT4_ETAP:
  case SP800_TRNG_HW:
  case SP800_TRNG_HW_NOISE:
  case SP800_TRNG_HW_ETAP:
    rv = TRNG_HW;
    break;
  default:
    rv = GetDefaultTrng();
    break;
  }
  return rv;
}

/*! @brief
   We need a scratch buffer for the induced failure tests,
   Attempting to modify static const data causes a segv so we need
   to move some data to a modifable data area
   Since these tests result in ICC termination, 
   there's no need for this to be thread safe
*/

static unsigned char kabuf[1024];

extern int error_state;



/*!
  @brief Set up a list of RNG's to NOT use
  @param lst a pointer to a comma delimitted string list of modes
  @note lst MUST be a persistant string, and it's up to the caller 
  to free it if it needs to be freed at library shutdown.
*/
void Set_rng_exclude(char *lst)
{
  if(NULL != lst) {
    exclude_list = lst;
  }
}

static int matchstr(char *one, char *two, char delim) {
  int matched = 0;
  while (0 == matched) {
    matched = (*one) - (*two);
    one++;
    two++;
    if ((*one == '\0' || *one == delim) && (*two == '\0' || *two == delim))
      break;
    if ((*one == '\0' || *one == delim) || (*two == '\0' || *two == delim)) {
      matched = -1;
    }
  }
  return matched;
}

/*! @brief return the list of FIPS compliant supported RNGS.
  @return a pointer to the internal (text) list of RNG's
*/
const char **get_SP800_90FIPS(void) {
  static int initialized = 0;
  static char *FIPS_rng_list[sizeof(PRNG_list) / sizeof(SP800_90PRNG_t *)];

  int i = 0;
  int j = 0;
  int exclude = 0;
  char *ptr = NULL;

  if (!initialized) {
    memset(FIPS_rng_list, 0, sizeof(FIPS_rng_list));
    for (i = 0; NULL != PRNG_list[i]; i++) {
      exclude = 0;
      if (NULL != exclude_list) {
        ptr = exclude_list;
        while ((ptr = strstr(ptr, (char *)PRNG_list[i]->prngname))) {
          if (0 == matchstr(ptr, (char *)PRNG_list[i]->prngname, ',')) {
            exclude = 1;
            break;
          }
          if (*ptr)
            ptr++;
        }
      }

      if (exclude) {
        continue;
      }
      /* This is opportunistic, create the per-type mutex
         here when we are already single threaded
      */
      ICC_CreateMutex(&(PRNG_list[i]->mtx));
      if (SP800_IS_FIPS == PRNG_list[i]->FIPS) {
        FIPS_rng_list[j] = (char *)PRNG_list[i]->prngname;
        j++;
      }
    }
    initialized = 1;
  }
  return (const char **)FIPS_rng_list;
}
/*!
  @brief
  Generate random data using this PRNG's seed source
  @param P a pointer to the prng to use
  @param n the number of byts to extract from the TRNG
  @param buf where to store the extracted data
  @return  0 if O.K. 1 otherwise.
*/
TRNG_ERRORS PRNG_GenerateRandomSeed(PRNG_CTX *P, unsigned int n,
                                    unsigned char *buf) {
  TRNG_ERRORS rv = TRNG_OK;
  SP800_90PRNG_Data_t *prng = (SP800_90PRNG_Data_t *)P;
 
  if (0 == n) {
    prng->state = SP800_90PARAM;
    prng->error_reason = ERRAT("0 bytes is not a valid entropy request");
    rv = TRNG_REQ_SIZE;
  } else {
    rv = TRNG_GenerateRandomSeed(prng->trng, n, buf);
    /** \induced 401: Simulate a one off TRNG failure,
        force test case to fail, triggering this error path
        and recovery
        (Doing this in the entropy source itself stops us
        well before this point)
    */
    /** \induced 406: Simulate an unrecoverable RNG failure 
     * 
    */
    
    if ((TRNG_OK != rv) || (401 == icc_failure) || (406 == icc_failure) ) {
      if(401 == icc_failure) {
        icc_failure = 0; /* Simulate a transient failure of a TRNG */
      }
      /* Try again, we should have changed the TRNG now */
      /* rv = TRNG_GenerateRandomSeed(prng->trng, n, buf); */
      if((TRNG_OK != rv)  || (406 == icc_failure) ) {
         prng->state = SP800_90CRIT;
         prng->error_reason = ERRAT("TRNG failure, low entropy");
         rv = TRNG_ENTROPY;
      }
    }
  }
  return rv;
}

/*!
  @brief called during ICC shutdown to cleanup any global objects
  @note now a dummy function, we had to allocate a PRNG per TRNG to
  avoid lock contention/blocking
*/
void CleanupSP800_90()
{
  int i;
  for(i = 0;PRNG_list[i] != NULL; i++) {
    /* Destroy the mutex's associated with the PRNG types
     */
    ICC_DestroyMutex(&(PRNG_list[i]->mtx));
  }
}

/*!
  @brief return the number of blocks needed to fill N bytes of output
  @return number of blocks needed.
  @note Rounds up so it generates AT LEAST bytes of output
  
*/
unsigned BlocksReqd(unsigned int bytes,unsigned int blocksize)
{
  return (bytes + blocksize -1)/blocksize;
}

/*!
  @brief convert an unsigned int to a big endian 4 byte stream
  @param n the unsigned int to convert
  @param N the output byte stream
*/
void uint2BS(unsigned n,unsigned char N[4])
{
  int i;
  for(i = 3; i >= 0 ; i--) {
    N[i] = n % 256;
    n = n / 256;
  }
}




/* 
   Flags to tell us if we allocated buffers for the various entropy sources
   so we can remember to free them
*/
#define ALLOC_ENT 1
#define ALLOC_NONCE 2
#define ALLOC_PERSON 4

/*! @brief
  Calculate the number of bytes of entropy needed for various modes 
  @param ictx An internal PRNG instance pointer
  @return the number of bytes needed.
*/
static unsigned NeededBytes(SP800_90PRNG_Data_t *ictx)
{
  int l;
  /* If it has a derivation function, guarantee enough bytes
     to meet the entropy requirement
  */
  /* Default is suitable for cipher modes with no DF */
  l = ictx->prng->seedlen;
  if(ictx->minEnt == 0) { /* Hasn't been set up yet */
    ictx->minEnt = ictx->prng->seedlen;
  }
  if(ictx->prng->hasDF) {
    /* Pull enough bytes to compensate for the entropy 
       guarantee of the source
    */
    if(ictx->trng) {
      l = ictx->minEnt * GetDesignEntropy(ictx->trng);
    } else {
      /* Some modes use the PRNG as a conditioner
          and there's no underlying TRNG to query
      */
      l = 2;
    }  
  } else if( ictx->prng->seedlen <= (ictx->minEnt *2)) {
    /* If it has a restricted entropy range provide the maximum 
       X9.31 modes
     */
    l = ictx->prng->seedlen;
  }
  return l;
}

/*!
  @brief Utility function to release data allocated
  internally during operation
  @param ein a pointer to possibly allocated entropy buffer
  @param nonce a pointer to a possibly allocated nonce
  @param person a pointer to a possibly allocated personalization string
  @param flags A set of flags which track locally allocated parameters 
  @note The data is assumed to have been scrubbed elsewhere
*/
void PRNG_free_scratch(unsigned char **ein,
		       unsigned char **nonce, 
		       unsigned char **person,
		       unsigned int *flags)
{
  if(*flags & ALLOC_ENT) {
    ICC_Free(*ein);
    *ein = NULL;
    *flags &= ~ALLOC_ENT;
  }
  if(*flags & ALLOC_NONCE) {
    ICC_Free(*nonce);
    *nonce = NULL;
    *flags &= ~ALLOC_NONCE;
  }
  if(*flags & ALLOC_PERSON) {
    ICC_Free(*person);
    *person = NULL;
    *flags &= ~ALLOC_PERSON;
  }
}
/*!
  @brief common sanity checks/setup for PRNG Instantiation
  @param ctx a partially initialized PRNG context
  @param ein a pointer to the entropy input buffer. (May be NULL)
  @param einl a pointer to the length of the provided entropy
  @param nonce additional entropy
  @param nonl a pointer to the length of the provided entropy
  @param person a pointer to the personalization data
  @param perl a pointer to the length of the personalization data
  @param flags a pointer to the flags used to track data we've allocated
  @return SP800_90STATE , normally SP800_90INIT
  @note In normal use we'd expect to arrive here with ein,einl, nonce, nonl 
  as NULL,0,NULL,0. But during testing we call the functions embedded in the
  PRNG_CTX directly to drive the code and supply the known input data through
  here.
  Note that if we are doing that, make sure to set the TestMode flag
  to bypass the automatic fill of empty inputs

*/
SP800_90STATE PRNG_Instantiate(SP800_90PRNG_Data_t *ctx,
			       unsigned char **ein, unsigned int *einl,
			       unsigned char **nonce, unsigned int *nonl,
			       unsigned char **person,unsigned int *perl,
			       unsigned int *flags)
{
  switch(ctx->state) {
  case SP800_90INIT:
  case SP800_90UNINIT:
    ctx->state = SP800_90INIT;
    break;
  default:
    break;
  }
  *flags = 0;
  /* 
     Only do the automatic stuff below if we aren't in test mode 
     There's no point trying to seed entropy sources either.
     We ignore the test-tap TRNG's as they are only used for testing and 
     this doesn't matter there.
  */


  if( (ctx->TestMode == 0)  && !(IS_TRNG & ctx->prng->type)     

     ) {

    ctx->Auto = 1; /* Enable autoreseed by default */
    /* Note we DON'T set the alloc flag here, we reuse ctx->eBuf for this
       if we aren't being driven in formal test mode.
       and eBuf is erased once the data is consumed.
    */
    /*! \FIPS  VE07.09.01 
      - Entropy and Nonce ("Seed key" and "Seed") are obtained
      from separate calls to the TRNG.
      - Our TRNG is an entropy source not a PRNG construction
      and we assert that separate outputs from the TRNG are independent
      and assert that the TRNG's are resistant to forward and backward
      attacks. (
      - For the SP800-90 PRNG's the Entropy and nonce are the same length      
      - IF the Entropy input and Nonce are the same length, 
      the TRNG construction also assures that the two inputs are also different
      - Note that the entropy extracted from the TRNG on each call is
      scaled to compensate for the minimum entropy guarantee of the TRNG.
    */

    /*!
      \FIPS SP800-90, Entropy input: Section 8.6.3
    */
    if (NULL == *ein) {
      *ein = ctx->eBuf;
      *einl = NeededBytes(ctx);
      if (TRNG_OK != PRNG_GenerateRandomSeed((PRNG_CTX *)ctx, *einl, *ein)) {
        ctx->state = SP800_90ERROR;
        ctx->error_reason = ERRAT("TRNG failure, low entropy");
      }
    } else {
      if (*einl < ctx->minEnt) {
        ctx->state = SP800_90PARAM;
        ctx->error_reason = ERRAT(SP800_90_MIN_ENT);
      }
      if (ctx->prng->hasDF && (*einl != ctx->prng->seedlen)) {
        ctx->state = SP800_90PARAM;
        ctx->error_reason = ERRAT(SP800_90_DF_ENT);
      }
    }
    /* A nonce isn't used for functions with no
       derivation function or TRNG test taps
    */
    if ((SP800_90INIT == ctx->state) && ctx->prng->hasDF) {
      /*!
        \FIPS  SP800-90, Nonce: Section 8.6.7
      */
      /* If nonce data isn't provided but is supported, supply it
         NB in the noDF modes maxNonce should be 0
      */
      if (NULL == *nonce && ctx->prng->maxNonce) {
        *flags |= ALLOC_NONCE;
        *nonl = NeededBytes(ctx);
        *nonce = ICC_Calloc(1, *nonl, __FILE__, __LINE__);
        if (TRNG_OK != PRNG_GenerateRandomSeed((PRNG_CTX *)ctx, *nonl, *nonce)) {
          ctx->state = SP800_90ERROR;
          ctx->error_reason = ERRAT("TRNG failure, low entropy");
        }
      } else {
        if (*nonl > ctx->prng->maxNonce) {
          ctx->state = SP800_90PARAM;
          ctx->error_reason = ERRAT(SP800_90_EXCESS_NONCE);
        }
      }
    }
    if (SP800_90INIT == ctx->state) {
      /* If personalization data isn't provided but is supported,
         supply it, if maxPers is 0, then obviously it can't be used
      */
      if (NULL == *person && ctx->prng->maxPers) {
        *flags |= ALLOC_PERSON;
        /* NULL returns the maximum length of the internal personalization
         * string */
        *perl = Personalize(NULL);
        *person = ICC_Calloc(1, *perl, __FILE__, __LINE__);
        Personalize(*person); /* Fill in the allocated buffer */
        if (ctx->prng->maxPers < *perl) {
          *perl = ctx->prng->maxPers; /* But lie about the length ... */
        }
      } else {
        if (*perl > ctx->prng->maxPers) {
          ctx->state = SP800_90PARAM;
          ctx->error_reason = ERRAT(SP800_90_EXCESS_PERS);
        }
      }
    }
  }

  /* Finally, check to ensure we don't have too much data
     This check is only valid for modes that provide this information
  */
  if (ctx->prng->maxEnt &&
      (((long)*einl + (long)*nonl + (long)*perl) > (long)(ctx->prng->maxEnt))) {
    ctx->state = SP800_90PARAM;
    ctx->error_reason = ERRAT(SP800_90_EXCESS_TOTAL);
  }
  if (SP800_90PARAM == ctx->state) {
    PRNG_free_scratch(ein, nonce, person, flags);
  }

  return ctx->state;
}

/*! 
  @brief check that a memory area is zero'd 
  @param where The location to check
  @param n the number of bytes to check
  @return 0 if zero'd, 1 otherwise
*/
static int NotZero(unsigned char *where, int n)
{
  int i;
  int rv = 0;
  for(i = 0; i < n ; i++) {
    if('\0' != where[i]) {
      rv = 1;
      break;
    }
  }
  return rv;
}
 /*! 
  @brief Internal "Instantiate" function which encapsulates
  the necessary state transition logic common to all modes
  Calls the method specific Instantiate() to to the algorithmic heavy hauling
  @param ctx A PRNG ctx pointer
  @param Ein Entropy input
  @param Einl length of entropy input
  @param Non nonce data
  @param Nonl nonce data length
  @param Per personalization data
  @param Perl personalization data length
*/
void Inst(PRNG_CTX *ctx,
	  unsigned char *Ein,unsigned int Einl,
	  unsigned char *Non,unsigned int Nonl,
	  unsigned char *Per, unsigned int Perl
	  ) 
{
  SP800_90PRNG_Data_t *ictx = (SP800_90PRNG_Data_t *)ctx;
  uint32_t t;
  switch(ictx->state) {
  case SP800_90UNINIT:
  case SP800_90INIT:
    ictx->state = SP800_90INIT;
    ictx->prng->Init(ctx,Ein,Einl,Non,Nonl,Per,Perl);
    ictx->ReseedAt = ictx->prng->maxReseed;
    t = 1;
    ictx->CallCount.u = htonl(t);
    switch(ictx->state) {
    case SP800_90INIT:
      ictx->state = SP800_90INIT;
      break;
    case SP800_90ERROR:
    case SP800_90CRIT:
    case SP800_90PARAM:
      break;
    default:
      ictx->state = SP800_90CRIT;
      ictx->error_reason = ERRAT("Invalid state transition in Instantiate");
      break;
    }
    break;
  case SP800_90ERROR:
  case SP800_90CRIT:
  case SP800_90PARAM:   
    break;
  default:
      ictx->state = SP800_90CRIT;
      ictx->error_reason = ERRAT("Invalid state on entry to Instantiate");    
   break;
  }
}
/*! 
  @brief Internal "ReSeed" function which encapsulates
  the necessary state transition logic common to all modes
  Calls the method specific ReSeed() to to the algorithmic heavy hauling
  @param ctx A PRNG ctx pointer
  @param Ein Entropy input
  @param Einl length of entropy input
  @param Adata Additional data
  @param Adatal Additional data length
*/
void Res(PRNG_CTX *ctx,
		unsigned char *Ein,unsigned int Einl,
		unsigned char *Adata,unsigned int Adatal)
{
  SP800_90PRNG_Data_t *ictx = (SP800_90PRNG_Data_t *)ctx;
  uint32_t t;
  switch(ictx->state) {
  case SP800_90RESEED:
  case SP800_90INIT:
  case SP800_90RUN:
    ictx->state = SP800_90RESEED;
    ictx->prng->ReSeed(ctx,Ein,Einl,Adata,Adatal);
    t = 1;
    ictx->CallCount.u = htonl(t);
    switch(ictx->state) {
    case SP800_90INIT:
    case SP800_90RUN:
    case SP800_90RESEED:
      ictx->state = SP800_90RUN;
      break;
    case SP800_90ERROR:
    case SP800_90CRIT:
    case SP800_90PARAM:
      break;
    default:
      ictx->state = SP800_90CRIT;
      ictx->error_reason = ERRAT("Invalid state transition in ReSeed");
      break;
    }
    break;
  case SP800_90ERROR:
  case SP800_90CRIT:
  case SP800_90PARAM:
    break;
  default:
    ictx->state = SP800_90CRIT;
    ictx->error_reason = ERRAT("Invalid state on entry to ReSeed");
    break;
  }    
}

/*! @brief Internal "Generate" function which encapsulates
  the necessary state transition logic common to all modes
  Calls the method specific Generate() to to the algorithmic heavy hauling
  @param ctx A PRNG ctx pointer
  @param out The output buffer
  @param outl The number of bytes to return
  @param Adata Additional data
  @param Adatal Additional data length
*/
void Gen(PRNG_CTX *ctx,
		unsigned char *out,unsigned int outl,
		unsigned char *Adata,unsigned int Adatal)
{
  SP800_90PRNG_Data_t *ictx = (SP800_90PRNG_Data_t *)ctx;
  uint32_t t = 0;
  unsigned char tmp[CNT_SZ];
  
  memset(tmp,0,CNT_SZ);

  switch(ictx->state) {
  case SP800_90RESEED:
    ictx->state = SP800_90ERROR;
    ictx->error_reason = ERRAT("PRNG needed reseeding");
    break;
  case SP800_90INIT:
    /*! \FIPS SP800-90 
      Create the first round of data for the continuous test 
    */
    if( 0 == ictx->TestMode ) {
      ictx->prng->Generate(ctx,
			   ictx->lastdata,
			   CNT_SZ,
			   NULL,0
			   );
    }
    /* Fall through */
  case SP800_90RUN:
    ictx->state = SP800_90RUN;
    if ((0 == ictx->TestMode) && (outl < CNT_SZ)) {
      ictx->prng->Generate(ctx, tmp, CNT_SZ, Adata, Adatal);
      memcpy(out, tmp, outl);

    } else {
      ictx->prng->Generate(ctx, out, outl, Adata, Adatal);
      if (0 == ictx->TestMode) {
        memcpy(tmp, out, CNT_SZ);
        /** \induced 405: Simulate a failure in the PRNG continuous tests.
            Force the current data to be the same as the saved sample
        */
        if (405 == icc_failure) {
          memcpy(tmp, ictx->lastdata, CNT_SZ);
        }
      }
    }
    /*! \FIPS SP800-90
      Continuous test. If we aren't in a test mode, check that the first 8 bytes
      of data from the last and current blocks don't match
      @note We always pull out at least 8 bytes/call unless in a test mode
    */
    if( (0 == ictx->TestMode) && (0 == memcmp(tmp,ictx->lastdata,CNT_SZ))) {
      ictx->state = SP800_90CRIT;
      ictx->error_reason = ERRAT(SP800_90_CONTINUOUS);
      SetFatalError("Health test failed on seed source",__FILE__,__LINE__);
      break;
    }
    /*
    Add_BE(ictx->CallCount,ictx->CallCount,4,(unsigned char *)C01,4);
    */
    t = ntohl(ictx->CallCount.u);
    t++;
    ictx->CallCount.u = htonl(t);      
    if(t >= ictx->ReseedAt) {
      ictx->state = SP800_90RESEED;
    }    
    switch(ictx->state) {
    case SP800_90RUN:
    case SP800_90RESEED:
    case SP800_90ERROR:
    case SP800_90CRIT:
    case SP800_90PARAM:
      break;	
    default:
      ictx->state = SP800_90CRIT;
      ictx->error_reason = ERRAT("Invalid state transition in Generate");
      break;

    }
    break;
  case SP800_90ERROR:
  case SP800_90CRIT:
  case SP800_90PARAM:
    break;
  default:
    ictx->state = SP800_90CRIT;
    ictx->error_reason = ERRAT("Invalid state on entry to Generate");
    break;     
  }	
}
/*! 
  @brief Internal "Cleanup" function which encapsulates
  the necessary state transition logic common to all modes
  @param ctx a PPRNG_CTX pointer
*/
void Cln(PRNG_CTX *ctx)
{
  SP800_90PRNG_Data_t *ictx = (SP800_90PRNG_Data_t *)ctx;
  SP800_90PRNG_t *prng = ictx->prng;
  TRNG *trng = ictx->trng;
  ictx->trng = NULL;
  prng->Cleanup(ctx);
  memset(ictx,0,sizeof(SP800_90PRNG_Data_t));
  ictx->prng = prng;
  ictx->trng = trng;
  ictx->state = SP800_90UNINIT;
}
/*!
  @brief PRNG self test function. run self test on all usable strengths for this algorithm
  Error reporting - critical errors are persistant and result in a not usable context
  In non-debug modes errors are treated as FIPS
  critical.
  @param ctx a PRNG context
  @param alg a PRNG algorithm
*/
#define TEST_OUT_SIZE 1024
void PRNG_self_test(PRNG_CTX *ctx, PRNG *alg)
{
  int i;
  unsigned char *out = NULL;
  SP800_90PRNG_Data_t *ictx = (SP800_90PRNG_Data_t *)ctx;
  SP800_90PRNG_t *prng = NULL;
  SP800_90PRNG_t *ialg = (SP800_90PRNG_t *)alg;
  SP800_90_test *data = NULL;
  unsigned char *ein = NULL;
  char *reason = NULL;

  prng = ictx->prng;

  out = (unsigned char *)CRYPTO_malloc(TEST_OUT_SIZE, __FILE__, __LINE__);
  if (ictx->prng->type & IS_TRNG) /* Meaningless for one of the TRNG test taps */
  {
    ictx->state = SP800_90RUN;
  }  else {
    if (NULL != out)
    {
      /* Flag we are doing self test, otherwise we'll try to scrub
       the test data
    */
      ictx->TestMode = 1;
      for (i = 0; i < 4; i++)
      {
        memset(out, 0, TEST_OUT_SIZE);
        data = &prng->TestData[i];
        if (NULL == data->InitEin)
          break;
        ein = (unsigned char *)data->InitEin->buf;
        /** \induced 402: Simulate a self test failure in a PRNG Instantiate.
          Change the known "entropy" input
      */
        if (402 == icc_failure)
        {
          memcpy(kabuf, ein, data->InitEin->len);
          ein = kabuf;
          kabuf[0] = ~kabuf[0];
        }
        ictx->prng->Inst(
            ctx, ein, data->InitEin->len, (unsigned char *)data->InitNonce->buf,
            data->InitNonce->len, (unsigned char *)data->InitPerson->buf,
            data->InitPerson->len);
        /* if we have entropy input here we are in a prediction resistance enabled
         mode - we reseed with the provided data then call generate
      */
        if ((NULL != data->GenEin->buf) && data->GenEin->len)
        {
          /** \induced 403: Simulate a self test failure in a PRNG ReSeed call.
            Change the known "entropy" input
        */
          ein = (unsigned char *)data->GenEin->buf;
          if (403 == icc_failure)
          {
            memcpy(kabuf, ein, data->GenEin->len);
            ein = kabuf;
            kabuf[0] = ~kabuf[0];
          }
          ictx->prng->Res(ctx, ein, data->GenEin->len,
                          (unsigned char *)data->GenAAD->buf, data->GenAAD->len);
          ictx->prng->Gen(ctx, out, data->GenRes->len, NULL, 0);
        }
        else
        {
          memset(out, 0, 1024);
          ictx->prng->Gen(ctx, out, data->GenRes->len,
                          (unsigned char *)data->GenAAD->buf, data->GenAAD->len);
        }
        if (0 != memcmp(out, data->GenRes->buf, data->GenRes->len))
        {
#if defined(DEBUG_PRNG)
          printf("PRNG failure %s strength %d\n", prng->prngname, prng->secS[i]);
          printf("Expected\n");
          iccPrintBytes((unsigned char *)data->GenRes->buf,
                        (int)data->GenRes->len);
          printf("Actual\n");
          iccPrintBytes(out, (int)data->GenRes->len);
#endif
          ictx->state = SP800_90CRIT;
          ictx->error_reason = ERRAT("Known answer test failed");
          SetFatalError("PRNG Known answer test failed", __FILE__, __LINE__);
        }
        /* Don't loose track of the fact it failed a self test */
        if (SP800_90CRIT == ictx->state)
        {
          reason = ictx->error_reason;
          ictx->prng->Cln(ctx);
          ictx->state = SP800_90CRIT;
          ictx->error_reason = reason;
          ictx->prng->error = 1; /* 40770 make self test errors sticky */
        }
        else
        {
          /* Cleanup the internal PRNG state */
          ictx->prng->Cln(ctx);
          /* Now check that the cleanup happened */
          /** \induced 404: Simulate a failure of the PRNG cleanup function,
            make some of the data that should have been zero'd non-zero
        */
          if (404 == icc_failure)
          {
            ictx->eBuf[5] = 0x42;
          }
          if (NotZero(ictx->K, MAX_K) || NotZero(ictx->V, MAX_V) ||
              NotZero(ictx->C, MAX_C) || NotZero(ictx->T, MAX_T) ||
              NotZero(ictx->eBuf, EBUF_SIZE) || (ictx->SecStr != 0) ||
              (ictx->ReseedAt != 0) || (ictx->Paranoid != 0) ||
              (ictx->minEnt != 0) || (ictx->CallCount.u != 0) ||
              (ictx->ctx.cctx != NULL))
          {
            SetFatalError("PRNG context cleanup failed", __FILE__, __LINE__);
            ictx->state = SP800_90CRIT;
            ictx->error_reason = ERRAT("PRNG context cleanup filed");
            ictx->prng->error = 1; /* 40770 make self test errors sticky */
          }
          else
          {
            ictx->state = SP800_90UNINIT;
          }
        }
        ictx->TestMode = 1;
        ictx->prng = prng;
      }
      /* O.K. out of test mode */
      ictx->TestMode = 0;
      ICC_LockMutex(&(ialg->mtx));
      ialg->last_tested_at = ialg->test_at;
      ICC_UnlockMutex(&(ialg->mtx));
      ICC_Free(out);
      out = NULL;
    }
    else
    {
      ictx->state = SP800_90CRIT;
      ictx->error_reason = ERRAT("Could not allocate memory for self test.");
    }
  }
}
/* Note, the reason for the ef functions here is to create an 
   OpenSSL callable set of interfaces. We delegate ICC/FIPS 
   error handling to the META layer as we want to plug these PRNG's
   into OpenSSL as well. 

   Error conditions will still be triggered at the lower layers, but 
   may not be handled as FIPS critical until a FIPS operation takes place.
*/

/*!
  @brief Get PRNG method by name
  @param algname algorithm name
  @param fips are we in FIPS mode ?. If so algorithms known to be
  non-FIPS compliant won't return a PRNG
  @return a PRNG method for the provided algorithm
*/
PRNG *get_RNGbyname(const char *algname, int fips) {
  int i = 0;
  PRNG *rv = NULL;
  char *ptr = NULL;

  static int hw_available_checked = 0;
  static int hw_available = 1;
  if (!hw_available_checked) {
     /* perform static hw availability check */
     hw_available_checked = 1;
     hw_available = ALT4_Avail();
  }

  for (i = 0; PRNG_list[i] != NULL; i++) {
    /* printf("(%s) PRNG %s\n",algname,PRNG_list[i]->prngname); */
    if (strcasecmp(algname, PRNG_list[i]->prngname) == 0) {
      /* Prune out the modes which rely solely on hardware for entropy
         if hardware isn't present. It'd be cheaper to modify the lists
         but unlike hashes etc, these shouldn't be ephemeral objects
         and the hit should be tolerable.
      */
       if (!hw_available && ((NULL != strstr(PRNG_list[i]->prngname, "HW")) ||
                            (NULL != strstr(PRNG_list[i]->prngname, "ALT4")))) {
          break;
       }

      /*
         In FIPS modes, restrict the algorithms we'd return
         to those that could pass testing
         D40770, don't allow PRNG TYPES which failed self test
      */
      if ((!fips || (SP800_IS_FIPS == PRNG_list[i]->FIPS)) &&
          (0 == PRNG_list[i]->error)) {
        rv = (PRNG *)PRNG_list[i];
        break;
      }
    }
  }

  if (NULL != exclude_list && NULL != rv) {
    ptr = exclude_list;
    while ((ptr = strstr(ptr, (char *)((SP800_90PRNG_t *)rv)->prngname))) {
      if (0 == matchstr(ptr, (char *)((SP800_90PRNG_t *)rv)->prngname, ',')) {
        rv = NULL;
        break;
      }
      if (*ptr)
        ptr++;
    }
  }
  return rv;
}
/*! @brief Public entry point for get_RNGbyname() 
  

*/
PRNG * my_get_RNGbyname(ICClib *pcb,const char *algname)
{
  PRNG *rv = NULL;
  SP800_90PRNG_t *trv = NULL;
  const EVP_MD *md = NULL;
  int nid;
  char *ptr = NULL;
  const EVP_CIPHER *cipher = NULL;
  if(NULL != pcb) {
    rv = get_RNGbyname(algname,(0 != (pcb->flags & ICC_FIPS_FLAG)));
    trv = (SP800_90PRNG_t *)rv;
    if((NULL != rv) && (pcb->callback)) {
      if((ptr = strstr(trv->prngname,"SHA"))) {
        md = EVP_get_digestbyname(ptr);
        nid = EVP_MD_type(md);
      } else {
        cipher = EVP_get_cipherbyname(trv->prngname);
        nid = EVP_CIPHER_type(cipher);
      }
      (*pcb->callback)("ICC_get_RNGbyname",nid,trv->FIPS);
    }
  } 
  return rv;
}
/*!
  @brief allocate a new PRNG_CTX
  @return a PRNG_CTX or NULL
  @note needed to cope with creation of ALT2 TRNG's
*/

PRNG_CTX *RNG_CTX_new_no_TRNG() 
{
  SP800_90PRNG_Data_t  * ctx = NULL;
  
  ctx = CRYPTO_calloc(1,sizeof(SP800_90PRNG_Data_t),__FILE__,__LINE__);
  return (PRNG_CTX *)ctx; 
}

/*!
  @brief allocate a new PRNG_CTX
  @return a PRNG_CTX or NULL
*/

PRNG_CTX *RNG_CTX_new() 
{
  SP800_90PRNG_Data_t  * ctx = NULL;
  ctx = (SP800_90PRNG_Data_t  *)RNG_CTX_new_no_TRNG();
  if(NULL != ctx) {
    /* And attach it to it's own TRNG of the default type */
    if(NULL != ctx->trng) {
      TRNG_free(ctx->trng);
    }
    ctx->trng = TRNG_new(GetDefaultTrng());
    if(NULL == ctx->trng) {
      ICC_Free(ctx);
      ctx = NULL;
    }
  }
  return (PRNG_CTX *)ctx; 
}

/*! 
  @brief Initialize an ICC PRNG channel - internal version
  @param ctx The PRNG context
  @param alg The PRNG algorithm, as returned from get_RNGbyname
  @param person Unique Personalization data (recommened NULL in which case 
  machine ID, PID, TID, and date/time is used)
  @param personal Number of bytes of personalization data
  @param strength The FIPS security strength. This must be <= the capability
  of the PRNG speficified as alg. 
  Valid values are 0 -> 256.
  0 is the recommended value for this API, in which case the HIGHEST 
  strength possible with the chosen generator is selected. 
  The only real difference is in the amount of data required to seed the PRNG 
  and with ICC there's no great gain in using less than the maximum PRNG capability. .
  @param prediction_resistance The PRNG is reseeded on each use. We recommend leaving
  this off. 
  @return SP800_90RUN or SP800_90PARAM (parameter error) SP800_90ERROR (NULL ctx or alg)
  are the most likely return values.
  @note Recomended usage is to pass NULL,0 for the personalization string 
  @note By default, ICC will initialize a PRNG in non-prediction resistant 
  mode with automatic reseeding enabled. 
  PRNG_CTX_ctrl() may be used to change those settings only between
  Init() and the first call to Generate()
  @note TRNG test taps are treated differently internally, reseed is a noop as is SelfTest etc
  but return codes should be appropriate.
*/
SP800_90STATE RNG_CTX_Init(PRNG_CTX *ctx, PRNG *alg, unsigned char *person,
                           unsigned int personal, unsigned int strength,
                           int prediction_resistance) {
  SP800_90PRNG_Data_t *ictx = (SP800_90PRNG_Data_t *)ctx;
  SP800_90PRNG_t *ialg = (SP800_90PRNG_t *)alg;
  unsigned int flags;
  unsigned char *ein = NULL;
  unsigned int einl = 0;
  unsigned char *nonce = NULL;
  unsigned int nonl = 0;
  int i;
  TRNG_TYPE T;
  TRNG_ERRORS e;
  SP800_90STATE state = SP800_90CRIT;
  /* Allow reinitialization at any point,
     provided there wasn't a critical failure
  */

  if(NULL != ictx && NULL != ialg && ( IS_TRNG & ialg->type) ) { /* If it's a TRNG test tap, just create it */         
    ictx->prng = ialg;
    if (SP800_90INIT == PRNG_Instantiate(ictx, &ein, &einl, &nonce, &nonl,
                                               &person, &personal, &flags)) {
            /* Call the Instantiate function */
      ictx->prng->Inst(ctx, ein, einl, nonce, nonl, person, personal);
      T = typeofTRNG(ialg->type);
      /* TRNG_Inst_Type(ctx,NULL,0,NULL,0,NULL,0,T); */
      e = TRNG_TRNG_Init(ictx->trng,T);
      if(TRNG_OK != e) {
        debug(printf("RNG_CTX_INIT: Error TRNG_TRNG_Init(t,%d) = %d\n", T, e));
      }
      else {
        state = ictx->state = SP800_90INIT;
      }
    }
  
  } else if (NULL != ictx) {

    switch (ictx->state) {
    case SP800_90CRIT:
      break;
    default:
      if ((NULL != ctx) && (NULL != alg)) {
        ictx->prng = (SP800_90PRNG_t *)alg;
        if (personal > ictx->prng->maxPers) {
          ictx->state = SP800_90PARAM;
          ictx->error_reason = ERRAT(SP800_90_EXCESS_TOTAL);
          break;
        }
        /*!
          \FIPS SP800-90 automated retest after N instantiations
          The retest counter is initialized to zero, so we hit this
          the first time through.
        */
        ICC_LockMutex(&(ialg->mtx));
        ialg->last_tested_at--; /* Update the self test counter */
        i = ialg->last_tested_at;
        ICC_UnlockMutex(&(ialg->mtx));

        if (i <= 0) {
          PRNG_self_test(ctx, alg);
        }

        /* If self test failed, we came out with a critical error
           just give up
        */
        if (ictx->state != SP800_90CRIT) {
          ictx->prng->Cln(ctx);
          switch (strength) {
          case 0: /* Pick highest available strength */
            for (i = 3; i >= 0; i--) {
              if (0 != ictx->prng->secS[i]) {
                ictx->SecStr = ictx->prng->secS[i];
                break;
              }
            }
            break;
          default: /* Pick minimum strength >= specified */
            for (i = 0; i < 4; i++) {
              if (ictx->prng->secS[i] >= strength) {
                ictx->SecStr = ictx->prng->secS[i];
                break;
              }
              if (i > 3) { /* Essentially too high a strength requested */
                ictx->state = SP800_90PARAM;
                ictx->error_reason = ERRAT(SP800_90_REQUESTED_STRENGTH);
              }
            }
            break;
          }
        }
        if (SP800_90UNINIT == ictx->state) {
          ictx->minEnt = (ictx->SecStr / 8);
          if (0 != prediction_resistance)
            ictx->Paranoid = 1;
          /* PRNG_Instantiate:
             Creates any entropy filled buffers that weren't supplied
             Checks the buffer lengths here if they were passed in ..
          */
          if (SP800_90INIT == PRNG_Instantiate(ictx, &ein, &einl, &nonce, &nonl,
                                               &person, &personal, &flags)) {
            /* Call the Instantiate function */
            ictx->prng->Inst(ctx, ein, einl, nonce, nonl, person, personal);
          }
          /* Clean up any buffers we provided in PRNG_Instantiate */
          PRNG_free_scratch(&ein, &nonce, &person, &flags);

          if ((SP800_90INIT == ictx->state) && (ictx->Paranoid)) {
            ictx->state = SP800_90RESEED;
          }
        }
      } else { /* Invalid, NULL, parameter */
        ictx->state = SP800_90PARAM;
        ictx->error_reason = ERRAT("Invalid (NULL) parameter");
      }
    }
#if !defined(_WIN32)    
    ictx->lastPID = getpid();
#endif
    state = ictx->state;
  }
  return state;
}

/*!
  @brief The ICC SP800-90 API Reseed function. 
  Any generic parameter and state validation is done at this point. 
  The actual reseeding is done by the algorithm specific methods
  @param ctx an initialized PRNG_CTX
  @param adata additional data to mix in (may be NULL)
  @param adatal length of the additional data
  @return SP800_90STATE - which should be SP800_90RUN

*/

SP800_90STATE RNG_ReSeed(PRNG_CTX *ctx, unsigned char *adata,
                         unsigned int adatal)
{
  SP800_90PRNG_Data_t *ictx = (SP800_90PRNG_Data_t *)ctx;
  SP800_90STATE state = SP800_90CRIT;
  int einl;
  int type = 0;

  unsigned long l;

  if ((NULL != ictx) && (NULL != ictx->prng))
  {
    if (IS_TRNG == (IS_TRNG & ictx->prng->type))
    {
      ictx->state = SP800_90RUN; /* TRNG test tap, doesn't need reseed */
    }  else {
      switch (ictx->state)
      {
      case SP800_90INIT:
      case SP800_90RUN:
      case SP800_90RESEED:
        if (adatal)
        {
          if (adatal > ictx->prng->maxAAD)
          {
            ictx->state = SP800_90PARAM;
            ictx->error_reason = ERRAT(SP800_90_EXCESS_AAD);
            break;
          }
          l = NeededBytes(ictx);
          if ((adatal + l) > ictx->prng->maxEnt)
          {
            ictx->state = SP800_90PARAM;
            ictx->error_reason = ERRAT(SP800_90_EXCESS_ENT);
            break;
          }
        }

        /*
          check that the global TRNG type hasn't changed
        */
        type = TRNG_type(ictx->trng);
        if (type != GetDefaultTrng())
        {       
          /* printf("Changing TRNG, default %d, current %d\n", GetDefaultTrng(),type); */
          /* It has so change our seed source to match the global one */
          TRNG_free(ictx->trng);
          ictx->trng = NULL;
          ictx->trng = TRNG_new(GetDefaultTrng());
          /* printf("TRNG is of type %d\n",TRNG_type(ictx->trng)); */
 
          if (NULL == ictx->trng)
          {
            ictx->state = SP800_90CRIT;
            ictx->error_reason = ERRAT("TRNG change, no usable TRNG");
          }
        }
        if (SP800_90CRIT != ictx->state)
        {
          /* Provide an appropriate amount of entropy */
          einl = NeededBytes(ictx);
          if (TRNG_OK != PRNG_GenerateRandomSeed(ctx, einl, ictx->eBuf))
          {
            ictx->state = SP800_90ERROR;
            ictx->error_reason = ERRAT("TRNG failure, low entropy");
          }
          else
          {
            ictx->prng->Res(ctx, ictx->eBuf, einl, adata, adatal);
            memset(ictx->eBuf, 0, einl);
          }
        }
        break;
      case SP800_90ERROR:
      case SP800_90CRIT:
      case SP800_90PARAM:
        break;
      default:
        ictx->state = SP800_90ERROR;
        ictx->error_reason = ERRAT(SP800_90_BAD_STATE);
        break;
      }
    }
  }
  else
  {
    ictx->state = SP800_90ERROR;
    ictx->error_reason = ERRAT(SP800_90_NOT_INIT);
  }

  state = ictx->state;

  return state;
}

/*!
  @brief extract data from the PRNG, 
  this layer perform generic state checks
  but the actual work is done by the algorithm methods
  @param ctx The PRNG context
  @param buffer where to place the returned bytes
  @param n number of bytes to extract
  @param adata additional input data to mix in (NULL is acceptable)
  @param adatal the length of the adata (0 is acceptable)
*/
static SP800_90STATE OldefRNG_Generate(PRNG_CTX *ctx, unsigned char *buffer,
                                       unsigned int n, unsigned char *adata,
                                       unsigned int adatal)
{
  SP800_90PRNG_Data_t *ictx = (SP800_90PRNG_Data_t *)ctx;
  SP800_90STATE state = SP800_90CRIT;
  unsigned int l;
  unsigned char tbuf[512];
#if !defined(_WIN32)

  pid_t pid = -1;
#endif
  if (NULL != ictx)
  {
    if (NULL != ictx->prng)
    {
#if !defined(_WIN32)
      /* Fork protection, ensure each process has unique state after fork() */
      pid = getpid();
      if(ictx->lastPID != pid) {
        TRNG_GenerateRandomSeed(ictx->trng,512,tbuf); /* We cache the noise input, so pull enough data to ensure that's cleared */
        ictx->state = SP800_90RESEED; /* Force a reseed to fix the PRNG states */
        ictx->lastPID = pid;
      }

#endif  
      if (n > ictx->prng->maxBytes)
      {
        ictx->state = SP800_90PARAM;
        ictx->error_reason = ERRAT(SP800_90_EXCESS_DATA);
      }
    
      if ((ictx->Paranoid) || (SP800_90RESEED == ictx->state) )
      {
        if( IS_TRNG == (IS_TRNG & ictx->prng->type))
        {
          ictx->state = SP800_90RUN;
        } else {  
          RNG_ReSeed(ctx, adata, adatal);
          adata = NULL;
          adatal = 0;
        }
      }
        switch (ictx->state)
        {
        case SP800_90RESEED:
        case SP800_90INIT:
        case SP800_90RUN:
          if (adatal)
          {
            if (adatal > ictx->prng->maxAAD)
            {
              ictx->state = SP800_90PARAM;
              ictx->error_reason = ERRAT(SP800_90_EXCESS_AAD);
              break;
            }
            l = NeededBytes(ictx);
            if ((adatal + l) > ictx->prng->maxEnt)
            {
              ictx->state = SP800_90PARAM;
              ictx->error_reason = ERRAT(SP800_90_EXCESS_ENT);
              break;
            }
          }
          /* Do the generate operation, state transition as necessary */
          ictx->prng->Gen(ctx, buffer, n, adata, adatal);


        if (ictx->Paranoid)
        {
          switch (ictx->state)
          {
          case SP800_90RESEED:
          case SP800_90RUN:
            ictx->state = SP800_90RESEED;
            break;
          default:
            break;

          }
          break;
        case SP800_90CRIT:
        case SP800_90ERROR:
        case SP800_90PARAM:
          break;
        default:
          ictx->state = SP800_90ERROR;
          ictx->error_reason = ERRAT(SP800_90_BAD_STATE);
          break;
        }
      }
    }
    else
    {
      ictx->state = SP800_90ERROR;
      ictx->error_reason = ERRAT(SP800_90_NOT_INIT);
    }
    state = ictx->state;
  }
  return state;
}
  /*!
  @brief extract data from the PRNG, 
  @param ctx The PRNG context
  @param buffer where to place the returned bytes
  @param n number of bytes to extract
  @param adata additional input data to mix in (NULL is acceptable)
  @param adatal the length of the adata (0 is acceptable)
  @note This will satisfy arbitrary length data requests,
        reseeding as necessary.
*/
  SP800_90STATE RNG_Generate(PRNG_CTX * ctx, unsigned char *buffer, unsigned int n,
                             unsigned char *adata, unsigned int adatal)
  {
    SP800_90PRNG_Data_t *ictx = (SP800_90PRNG_Data_t *)ctx;
    SP800_90STATE state = SP800_90CRIT;
    unsigned int chunksize = 0, req = 0;

    if (NULL != ictx)
    {
      if (0 != ictx->Auto)
      {
        chunksize = ictx->prng->maxBytes;
        while (((SP800_90RUN == ictx->state) || (SP800_90RESEED == ictx->state) ||
                (SP800_90INIT == ictx->state)) &&
               (n > 0))
        {
          req = (n > chunksize) ? chunksize : n;
          OldefRNG_Generate(ctx, buffer, req, adata, adatal);
          adata = NULL;
          adatal = 0;
          n -= req;
          buffer += req;
        }
      } else {
      OldefRNG_Generate(ctx, buffer, n, adata, adatal);
    }
    state = ictx->state;
  }
  return state;
}

/*! 
  @brief perform some operation on an initialized PRNG CTX
  @param ctx The PRNG context
  @param type The type of operation to perform. See enum SP800_90CTRL for possible operations
  @param arg input data
  @param ptr pointer to output data
  @return The state of the PRNG_CTX after the operation. 
  This will be SP800_90UNINIT after a self test cycle.
*/
SP800_90STATE RNG_CTX_ctrl(PRNG_CTX *ctx, SP800_90CTRL type, int arg,
                           void *ptr) {
  SP800_90PRNG_Data_t *ictx = (SP800_90PRNG_Data_t *)ctx;
  SP800_90STATE rv = SP800_90ERROR;
  if ((NULL != ictx) && (NULL != ictx->prng)) {
    switch (type) {
    case SP800_90_GET_PARANOID:
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->Paranoid;
        rv = ictx->state;
      } else {
        rv = SP800_90PARAM;
      }
      break;
    case SP800_90_SET_PARANOID:
      /* Only allowed after Init & before first call */
      if (SP800_90INIT == ictx->state) {
        ictx->Paranoid = arg;
      }
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->Paranoid;
      }
      rv = ictx->state;
      break;
    case SP800_90_GETMAXAAD:
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->prng->maxAAD;
        rv = ictx->state;
      } else {
        rv = SP800_90PARAM;
      }
      break;
    case SP800_90_GETMAXNONCE:
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->prng->maxNonce;
        rv = ictx->state;
      } else {
        rv = SP800_90PARAM;
      }
      break;
    case SP800_90_GETMAXPER:
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->prng->maxPers;
        rv = ictx->state;
      } else {
        rv = SP800_90PARAM;
      }
      break;
    case SP800_90_GETMINSEED:
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->minEnt;
        rv = ictx->state;
      } else {
        rv = SP800_90PARAM;
      }
      break;
    case SP800_90_GETMAXSEED:
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->prng->maxEnt;
        rv = ictx->state;
      } else {
        rv = SP800_90PARAM;
      }
      break;
    case SP800_90_SELFTEST:
      /* Only allowed before first call for a PRNG
         and left in a damaged state after ...
         because it really messes it up
       */
      if (IS_TRNG == (IS_TRNG & ictx->prng->type)) /* TRNG, so fake this */
      {
        ictx->prng->Cln(ctx);
        rv = ictx->state = SP800_90UNINIT;
      }
      else
      {
        if (SP800_90INIT == ictx->state)
        {

          PRNG_self_test(ctx, (PRNG *)ictx->prng);
          /* Save the state after self test */
          rv = ictx->state;
          ictx->prng->Cln(ctx);
          /* Were we broken ? */
          if (SP800_90CRIT != rv)
          {
            /* Not broken , so set to unititialized */
            rv = ictx->state = SP800_90UNINIT;
            if (NULL != ptr)
            {
              *(SP800_90STATE *)ptr = ictx->state;
            }
          }
        }
      }
      break;
    case SP800_90_GETMAXRESEED:
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->prng->maxReseed;
        rv = ictx->state;
      } else {
        rv = SP800_90PARAM;
      }
      break;
    case SP800_90_GETRESEED:
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->ReseedAt;
        rv = ictx->state;
      } else {
        rv = SP800_90PARAM;
      }
      break;
    case SP800_90_GETSTRENGTH:
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->SecStr;
        rv = ictx->state;
      } else {
        rv = SP800_90PARAM;
      }
      break;
    case SP800_90_SETRESEED:
      /* Only allowed after Init & before first call */
      if (SP800_90INIT == ictx->state) {
        if ((arg > 0) && (arg < (int)ictx->prng->maxReseed)) {
          ictx->ReseedAt = arg;
        }
      }
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->ReseedAt;
      }
      rv = ictx->state;
      break;
    case SP800_90_DORESEED:
      if ((SP800_90RUN == ictx->state) || (SP800_90INIT == ictx->state)) {
        ictx->state = SP800_90RESEED;
        rv = ictx->state;
      }
      break;
    case SP800_90_GETENTROPY:
      *(unsigned int *)ptr = GetEntropy(ictx->trng);
      rv = ictx->state;
      break;
    case SP800_90_GETLASTERROR:
      if (NULL != ptr) {
        *(char **)ptr = ictx->error_reason;
        rv = ictx->state;
      } else {
        rv = SP800_90PARAM;
      }
      break;
    case SP800_90_GETTESTCOUNT:
      if (NULL != ptr) {
        ICC_LockMutex(&(ictx->prng->mtx));
        *(unsigned int *)ptr = ictx->prng->last_tested_at;
        ICC_UnlockMutex(&(ictx->prng->mtx));
        rv = ictx->state;
      } else {
        rv = SP800_90PARAM;
      }
      break;
    case SP800_90_GETMAXDATA:
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->prng->maxBytes;
        rv = ictx->state;
      } else {
        rv = SP800_90PARAM;
      }
      break;
    case SP800_90_SETAUTO:
      /* Only allowed after Init & before first call */
      if (SP800_90INIT == ictx->state) {
        ictx->Auto = arg;
      }
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->Auto;
      }
      rv = ictx->state;
      break;
    case SP800_90_GETAUTO:
      if (NULL != ptr) {
        *(unsigned int *)ptr = ictx->Auto;
        rv = ictx->state;
      } else {
        rv = SP800_90PARAM;
      }
      break;
    default:
      break;
    }
  }
  return rv;
}

/*!
  @brief Scrub and deallocate a PRNG_CTX
  @param ctx The PRNG_CTX to free
*/
void RNG_CTX_free(PRNG_CTX *ctx)
{
  SP800_90PRNG_Data_t *ictx = (SP800_90PRNG_Data_t *)ctx;
  if(NULL != ictx) {
     if(NULL != ictx->trng) {
      TRNG_free(ictx->trng);
      ictx->trng = NULL;
    }
    if(NULL != ictx->prng) {
      ictx->prng->Cln(ctx);
      ictx->prng = NULL;
    }
    memset(ictx,0,sizeof(SP800_90PRNG_Data_t));
    ICC_Free(ictx);
  }
}

