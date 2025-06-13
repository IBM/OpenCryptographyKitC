/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description:                                                                
//    Implementation of OpenSSL RAND functions for FIPS 140-2 compliant PRNG   
//
*************************************************************************/


/* Includes */
/* ======== */
#include <stdio.h>
#include <stdlib.h>
#if !defined(_WIN32)
#include <unistd.h>
#include <time.h>
#endif

#include "openssl/rand.h"
#include "openssl/crypto.h"
#include "icclib.h"
#include "fips.h"
#include "platform.h"
#include "TRNG/entropy_estimator.h"
#include "fips-prng/SP800-90.h"
#include "fips-prng/SP800-90i.h"




/* Prototypes of internal functions */
/* ================================ */

/* standard OpenSSL required methods */
/* --------------------------------- */
static void fips_rand_cleanup(void);
static int fips_rand_seed(const void *buf, int num);
static int fips_rand_add(const void *buf, int num, double add_entropy);
int fips_rand_bytes(unsigned char *buf, int num);
static  int fips_rand_pseudo_bytes(unsigned char *buf, int num);
static  int fips_rand_status(void);
/* see 'man 3 rand' for general semantics of above and additional
   status/error codes below. Note that above functions fail if
   RAND_FIPS_init() was not successfully called beforehand! 
*/

extern int SetTRNGName(char *trngname);
extern int TRNG_count();
/* Internal global variables */
/* ========================= */


static RAND_METHOD fips_rand_meth={
  fips_rand_seed,
  fips_rand_bytes,
  fips_rand_cleanup,
  fips_rand_add,
  fips_rand_pseudo_bytes,
  fips_rand_status
};

#define MAX_rngs 256  /*!< The absolute limit on the number of RNG units in play */

/*! Why 38 bytes of AAD ?, well the strength of the PRNG is rated at
   256 bits, so more than 32 bytes is pointless. 
   However, regular sized blocks (like timestamps)  get fed in, 
   and having them xor'd neatly over one another would be a waste
   so we make the AAD buffer for added data 37 bytes long to try to 
   get regular sized inputs to not overlay.
   The first byte of AAD is the PRNG number (37 + 1 = 38 )
*/

#define AAD_SIZE 38

static enum { UNDEF, INIT, FAIL }  status = UNDEF;



/*! \FIPS DRBG underlying OpenSSL's RAND_pseudo_bytes() and ICC's 
  ICC_RAND_bytes().
  Each DRBG contains it's own NRBG instance for seeding.
   @note we use a thread pool to improve bandwidth of the RBG's, 
   and these are locked to provide thread safety.
*/
typedef struct {
  ICC_Mutex mtx;
  PRNG_CTX *rng;
} PRNG_BLOCK;

/*! \FIPS RGB underyling OpenSSL's RAND_bytes() and ICC_GenerateRandomSeed().
  
   - See A.1 Draft NIST SP800-90C

   @note we use a thread pool to improve bandwidth of the RBG's, 
   and these are locked to provide thread safety.
 */
typedef struct {
  ICC_Mutex mtx;
  PRNG_CTX *rng;
  unsigned int bytes;
  unsigned int index;
  unsigned char aad[AAD_SIZE];
} TRNG_BLOCK;

static PRNG_BLOCK *pctx = NULL;  /*!< Standard SP800-90 PRNG */
static TRNG_BLOCK *tctx = NULL; /*!< SP800-90B/C RBG used as seed source */

/* Yes, 7 is an odd number, in fact, it's a prime, so if there's some
   pattern in thread allocation, we have a better chance of distributing
   threads across RNG's well
*/
static int N_rngs = 7; /*!< The number of RNG units in play */
static char icc_global_prng_name[20] = {"SHA256"}; /*!< The type of the default PRNG */

/* Implementation of functions */
/* =========================== */

RAND_METHOD *RAND_FIPS(void) {
  return(&fips_rand_meth);
}


/*! 
  @brief return the name of PRNG ICC uses
  @return The name of PRNG ICC uses
*/
const char *GetPRNGName()
{
  return icc_global_prng_name;
}

/*! 
  @brief return the number of RNG instances in use
*/
int GetRNGInstances()
{
  return N_rngs;
}


  
/*!
  @brief set the global ICC PRNG
  @param prngname - the PRNG "name" SHA224, HMAC-256, AES-256-ECB 
  @return 1 on sucess, 0 otherwise
  @note this must be called before the first ICC_Attach() and
  must specify a PRNG that passes the NIST suite and provides
  256 bits of entropy. The default does that.
  The ONLY reason for allowing this to be changed is so we have
  a fallback at application level if one of the RNG's proves to
  have a flaw.
  
*/
int SetPRNGName(char *prngname)
{
  int rv = 0;
  PRNG *alg = NULL;
  SP800_90PRNG_t *p = NULL;
  int i = 0;

  /* The global RNG MUST be one of the FIPS SP800-90 modes and 256 bit security strength capable */
  alg = get_RNGbyname(prngname, 1);
  p = (SP800_90PRNG_t *)alg;
  if (NULL != alg)
  {
    for (i = 0; i < 4; i++)
    {
      if (256 == p->secS[i])
      { /* The highest available security strength is chosen by default */
        strncpy(icc_global_prng_name, prngname, 19);
        rv = 1;
        break;
      }
    }
  }
  return rv;
}

int SetRNGInstances(int instances)
{
  int rv = 0;
  /* Note we don't bother locking this, the actual update should be atomic 
     and locked during startup
  */
  if( (status != INIT) && (instances > 0) && (instances <= MAX_rngs) ) {
      N_rngs = instances;
      rv = 1;
  }
  return rv;
}

/*!
  @brief initialize a TRNG in the thread pool 
  @param i the TRNG to initialize
  @return RAND_R_PRNG_OK or an error
  @note   
   - i is assumed in range 
   - locks are assumed to be taken/released outside this function
   - TRNG already tested for not instantiated
*/
static int init_trng(int i) {
  int rc = RAND_R_PRNG_OK;
  PRNG *alg = NULL; 

  alg = get_RNGbyname(icc_global_prng_name,1);
  tctx[i].rng = RNG_CTX_new();

  if((alg != NULL) && (NULL != tctx[i].rng) ) {
    if(SP800_90INIT != RNG_CTX_Init(tctx[i].rng,alg,NULL,0,256,0)) {
      rc = RAND_R_PRNG_NOT_INITIALIZED;
    } 
  } else {
    rc = RAND_R_PRNG_NOT_IMPLEMENTED;
  }
  if(RAND_R_PRNG_OK != rc) {
    RNG_CTX_free(tctx[i].rng);
    tctx[i].rng = NULL;
  }
  return rc;
}
/*!
  @brief initialize a TRNG in the thread pool 
  @param i the TRNG to initialize
  @return RAND_R_PRNG_OK or an error
  @note 
   - i is assumed in range 
   - locks are assumed to be taken/released outside this function
   - rng has already been tested for not-instantiated
*/
static int init_prng(int i) {
  int rc =  RAND_R_PRNG_OK;
  PRNG *alg = NULL;

  alg = get_RNGbyname(icc_global_prng_name,1);
  pctx[i].rng = RNG_CTX_new();
   if ((alg != NULL) && (NULL != pctx[i].rng)) {
    if(SP800_90INIT != RNG_CTX_Init(pctx[i].rng,alg,NULL,0,256,0)) {
      rc = RAND_R_PRNG_NOT_INITIALIZED;
    } 
  } else {
    rc = RAND_R_PRNG_NOT_IMPLEMENTED;
  }
  if(RAND_R_PRNG_OK != rc) {
    RNG_CTX_free(pctx[i].rng);
    pctx[i].rng = NULL;
  }
  return rc;
}


/* ------------------------------------- 
   Note 08/2008 Changed to use the SP800-90 API for
   the OpenSSL RNG.
   That self tests and internally reseeds so we are no longer 
   dependent on the seed passed in here - which is now treated
   as additional data in a reseed if provided. 
   Since it comes from a separate seed source, that's
   reasonable.
   @brief Setup the global PRNG used by default by OpenSSL
   @param seed seeding data 
   @param num length of seeding data (bytes)
   @return RAND_R_PRNG_OK on sucess, something else otherwise
   @note This code has changed with time, the seed is ignored
   - The actual RNG instantiation is deferred until first 
     use so as to not penalize low thread count processes
   @note this function must be called with locks held, effectively
   that happens because it's called only during library load.
*/
int RAND_FIPS_init(const void *seed, const unsigned int num) {
  int rc = RAND_R_PRNG_OK;
  int i = 0;

  if (status != INIT) {
    
    if (status == INIT)
      goto cleanup;

    if ((status == UNDEF) && (NULL != pctx)) {
      rc = RAND_R_PRNG_INVALID_ARG;
      goto cleanup;
    }

    /*! \FIPS OpenSSL Seed source
      - This is an array of SP800-90 PRNG's in default mode
      - This is the source used for key generation
      - reseeded before every FIPS keygen (icclib.c)

     */
    rc = RAND_R_PRNG_OK;
    tctx = ICC_Malloc(sizeof(TRNG_BLOCK) * N_rngs, __FILE__, __LINE__);
    if (NULL == tctx) {
      rc = RAND_R_PRNG_NOT_INITIALIZED;
    }
    if (RAND_R_PRNG_OK == rc) {
      memset(tctx, 0, sizeof(TRNG_BLOCK) * N_rngs);
      for (i = 0; i < N_rngs; i++) {
        if (0 != ICC_CreateMutex(&(tctx[i].mtx))) {
          rc = RAND_R_PRNG_NOT_INITIALIZED;
          break;
        }

      }
    }

    /*! \FIPS OpenSSL Pseudo random source
      - This is an SP800-90 PRNG in default mode,
      a 256 bit PRNG mode autoreseed at intervals specified in the standard
      - This is the source used for padding and salts
       - Note that the PRNG construction allows for our minimum entropy
       guarantee internally
     */
    pctx = ICC_Malloc(sizeof(PRNG_BLOCK) * N_rngs, __FILE__, __LINE__);
    if (NULL == pctx) {
      rc = RAND_R_PRNG_NOT_INITIALIZED;
    }
    if (RAND_R_PRNG_OK == rc) {
      memset(pctx, 0, sizeof(PRNG_BLOCK) * N_rngs);
      for (i = 0; i < N_rngs; i++) {
        if (0 != ICC_CreateMutex(&(pctx[i].mtx))) {
          rc = RAND_R_PRNG_NOT_INITIALIZED;
          break;
        }
      }
    }

    if (RAND_R_PRNG_OK == rc) {
      status = INIT;
    }
  cleanup:
    if (rc != RAND_R_PRNG_OK) {
      if (NULL != pctx) {
        for (i = 0; i < N_rngs; i++) {
          if (NULL != pctx[i].rng) {
            RNG_CTX_free(pctx[i].rng);
            pctx[i].rng = NULL;
          }
          ICC_DestroyMutex(&(pctx[i].mtx));
        }
        ICC_Free(pctx);
        pctx = NULL;
      }
      if (NULL != tctx) {
        for (i = 0; i < N_rngs; i++) {
          if (NULL != tctx) {
            RNG_CTX_free(tctx[i].rng);
            tctx[i].rng = NULL;
          }
          ICC_DestroyMutex(&(tctx[i].mtx));
        }
        ICC_Free(tctx);
        tctx = NULL;
      }
      status = FAIL;
      ERR_put_error(ERR_LIB_RAND, RAND_F_FIPS_PRNG_RAND_INIT, rc, __FILE__,
                    __LINE__);
      SetFatalError("Could not initialize system PRNG",__FILE__,__LINE__);
    }
  }
  return rc;
}
/*!
  @brief return the entropy estimates for the core entropy sources
  @note We have three sets of potential sources, one feeding the PRNG and the other 
  the seed source, retrieve all and the 
  global TRNG state, return the minimum.
*/
int RAND_FIPS_Entropy()
{
  unsigned int eRNG = 0;
  unsigned int total = 100; /*! Underlying entropy source */
  int i = 0;

  for(i = 0 ; i < N_rngs; i++) {
    if(NULL != pctx[i].rng) {
      RNG_CTX_ctrl(pctx[i].rng,SP800_90_GETENTROPY,0,&eRNG);
      if(eRNG < total) {
	      total = eRNG;
      }
    }
    if(NULL != tctx[i].rng) {
      RNG_CTX_ctrl(tctx[i].rng,SP800_90_GETENTROPY,0,&eRNG);
      if(eRNG < total) {
	      total = eRNG;
      }
    }   
  }
  return total;
}

/*!
 @brief Manually reseed the PRNG
 @param ibuf the source seed data
 @param num the number of bytes of seed data
 @note the data supplied is nonce, primary seeding is internal
*/
int fips_rand_seed(const void *ibuf, int num){
  int rc= RAND_R_PRNG_OK;
  int tid = 0;
  unsigned char *buf = (unsigned char *)ibuf;
  SP800_90STATE state = SP800_90RUN;

  tid = ICC_GetThreadId() % N_rngs;

  ICC_LockMutex(&(tctx[tid].mtx));
  /* If it was never initialized  */
  if (NULL == tctx[tid].rng ) {
    rc = init_trng(tid);
    /* No need to reseed if it was just instantiated */
  } else { /* We allow NULL,0 because the primary seed source is internal */
    if (num >= 0) {
      state = RNG_ReSeed(tctx[tid].rng, buf, num);
    }
    switch (state)
    {
    case SP800_90RUN:
    case SP800_90RESEED:
      break;
    default:
      rc = RAND_R_PRNG_CRYPT_TEST_FAILED;
      break;
    }
  }
  ICC_UnlockMutex(&(tctx[tid].mtx));
  
  if (rc != RAND_R_PRNG_OK)
  {
    /* status = FAIL; */
    ERR_put_error(ERR_LIB_RAND, RAND_F_FIPS_PRNG_RAND_SEED, rc,__FILE__,__LINE__);
  }
  return rc;
}


/* ------------------------------------- */
static int fips_rand_add(const void *buf, int num, double add_entropy){
  /* ignore the entropy as we do not keep track of estimated entropy */
  return RAND_R_PRNG_OK; /* fips_rand_seed(buf, num);*/
}


/*!
   @brief OpenSSL callback for a TRNG
   Wired to an SP800_90 PRNG in prediction resistance mode
   @param buf the buffer in which to return the random data
   @param num the number of bytes to return
   @return 1 on success, 0 on failure
   @note Uses an enhanced RBG, as per SP800-90C
         i.e. a DRBG mixed with an NRBG (xor construction)
  @note to add resiliance a TRNG failure will just result in a new TRNG type being
    selected we have to cope with hot migration to not identical hardware now
*/
int fips_rand_bytes(unsigned char *buf, int num){
  int rc = RAND_R_PRNG_OK;
  int tid = 0;
  unsigned char *aad = NULL;
  unsigned int aadl = 0;
  SP800_90STATE state = SP800_90RUN;

  tid = ICC_GetThreadId() % N_rngs;

  if ((status != INIT) ||
      (buf==NULL) ||
      (num < 0)) {
    rc=RAND_R_PRNG_INVALID_ARG; 
    goto cleanup;
  }
  ICC_LockMutex(&(tctx[tid].mtx));
  /* If it was never initialized  */
  if (NULL == tctx[tid].rng ) {
    rc = init_trng(tid);
  }

  if (rc == RAND_R_PRNG_OK) {
    if (tctx[tid].bytes > 16) { /* Don't bother unless there's a decent amount of aad */
      aad = tctx[tid].aad;
      /* Pick up the TID as well, just to make sure the AAD fed to each
               RNG does differ */
      aadl = tctx[tid].bytes + 1;
      tctx[tid].bytes = 0; /* Reset the aad accumulator state */
      tctx[tid].index = 1;
    }
    memset(buf,0,num);
    state = RNG_Generate(tctx[tid].rng, buf, num, aad, aadl);

    switch (state) {
    case SP800_90RUN:
    case SP800_90RESEED:
      break;
    default:
      rc = RAND_R_PRNG_CRYPT_TEST_FAILED;
      break;
    }

  }
  ICC_UnlockMutex(&(tctx[tid].mtx));

cleanup:
  if (rc != RAND_R_PRNG_OK) {
    /* status = FAIL; */
    ERR_put_error(ERR_LIB_RAND, RAND_F_FIPS_PRNG_RAND_BYTES, rc, __FILE__,
                  __LINE__);
    SetFatalError("rand bytes fails", __FILE__, __LINE__);
    rc = 0;
  } else {
    rc = 1; /* OpenSSL "it worked" */
  }
  return rc;
}

/*! @brief Get random bytes from the system wide PRNG pool
  @param buf the buffer to fill
  @param num number of bytes to fill
  @return 1 on success
  @note reseed on a PID change (fork()) is done in the lower level PRNG code
  @note to add resiliance a TRNG failure will just result in a new TRNG being selected
        we have to cope with hot migration to not identical hardware now
*/
static int fips_rand_pseudo_bytes(unsigned char *buf, int num) {
  int rc = RAND_R_PRNG_OK;
  SP800_90STATE state;
  int tid = 0;
  tid = ICC_GetThreadId() % N_rngs;

  if ((status != INIT) ||
      (buf==NULL) ||
      (num < 0)) {
    rc=RAND_R_PRNG_INVALID_ARG; 
    goto cleanup;
  }

  ICC_LockMutex(&(pctx[tid].mtx));

  if( rc == RAND_R_PRNG_OK ) {
    if(NULL == pctx[tid].rng) {
      rc = init_prng(tid);
    }
  }
 
  state = RNG_Generate(pctx[tid].rng,buf,num,NULL,0);

  /* We could be in a RUN or RESEED state on return
   */
  switch(state) {
  case SP800_90RUN:
  case SP800_90RESEED:
    break;
  default:
    rc=RAND_R_PRNG_CRYPT_TEST_FAILED;
    break;
  }
  ICC_UnlockMutex(&(pctx[tid].mtx));

cleanup:  
  if (rc != RAND_R_PRNG_OK) {
      /* status = FAIL; */
      ERR_put_error(ERR_LIB_RAND, RAND_F_FIPS_PRNG_RAND_BYTES, rc,__FILE__,__LINE__);
      SetFatalError("Rand bytes fails",__FILE__,__LINE__);
      rc = -1;
    } else {
    rc = 1;
  }
  return rc;
}


/* ------------------------------------- */
static  int fips_rand_status(void){
  return (status == INIT);
}


/*! @brief cleanup the system RNG's
    Note that it's assumed locks are already held or irrelevant
    at this point.
*/
static void fips_rand_cleanup(void) {
  int rc = RAND_R_PRNG_OK;
  int i = 0;
  if (NULL != pctx) {
    for (i = 0; i < N_rngs; i++) {
      if (NULL != pctx[i].rng) {
        RNG_CTX_free(pctx[i].rng);
        pctx[i].rng = NULL;
      }
      ICC_DestroyMutex(&(pctx[i].mtx));
    }
    ICC_Free(pctx);
    pctx = NULL;
  }
  /* Free the TRNG's */
  if (NULL != tctx) {
    for (i = 0; i < N_rngs; i++) {
      if (NULL != tctx[i].rng) {
        RNG_CTX_free(tctx[i].rng);
        tctx[i].rng = NULL;
      }
      ICC_DestroyMutex(&(tctx[i].mtx));
    }
    ICC_Free(tctx);
    tctx = NULL;
  }
  status = UNDEF;

  if (rc != RAND_R_PRNG_OK) {
    ERR_put_error(ERR_LIB_RAND, RAND_F_FIPS_PRNG_RAND_CLEANUP, rc, __FILE__,
                  __LINE__);
  }
  if (rc != RAND_R_PRNG_OK)
  {
      ERR_put_error(ERR_LIB_RAND, RAND_F_FIPS_PRNG_RAND_CLEANUP, rc,__FILE__,__LINE__);
  }
}
