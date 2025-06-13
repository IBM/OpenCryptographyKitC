/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Data tables for SP800-90 TRNG
//
*************************************************************************/


#include "icclib.h"
#include "fips.h"
#include "TRNG/entropy_to_NRBGi.h"
#include "fips-prng/SP800-90.h"
#include "fips-prng/SP800-90i.h"

extern TRNG_ERRORS SetRNGError(const char *msg,const char *file, int line);

/*
  This file provides access to NRBG's using the same
  API as the SP800-90 PRNG code
  It's not intended that this necessarilly provide a specific FIPS 
  certified mode of operation, though the ICC NRBG should meet FIPS
  requirements as an entropy source.
  This code is here simply to "make the NRBG work via this API"

  Also note that all the "is this a valid state" questions have been asked
  before we got to this point - so there's little state or critical error
  checking.

*/
SP800_90ReSeed      TRNG_ReSeed;
SP800_90Generate    TRNG_Generate;
SP800_90Cleanup     TRNG_CleanupX;

SP800_90Instantiate TRNG_FIPS_Instantiate;
SP800_90Instantiate TRNG_ALT_Instantiate;
SP800_90Instantiate TRNG_ALT4_Instantiate;
/*!
  @brief Instantiate function for the ICC NRBG's
  @param ctx a partially initialized PRNG context
  @param ein a pointer to the entropy input buffer. (May be NULL)
  @param einl a pointer to the length of the provided entropy
  @param nonce additional entropy
  @param nonl a pointer to the length of the provided entropy
  @param person a pointer to the personalization data
  @param perl a pointer to the length of the personalization data
  @param type the type of NRBG to instantiate
  @note this is provided simply to provide an API compatable 
  interface to the RNG sources. It doesn't behave like a PRNG
 */
SP800_90STATE TRNG_Inst_Type(PRNG_CTX *ctx,
				    unsigned char *ein, unsigned int einl,
				    unsigned char *nonce, unsigned int nonl,
				    unsigned char *person,unsigned int perl,
				    TRNG_TYPE type
				    )
{
  SP800_90PRNG_Data_t *tctx = (SP800_90PRNG_Data_t *)ctx;

  tctx->state = SP800_90INIT;

  if(NULL != tctx->trng) {
    TRNG_free(tctx->trng);
    tctx->trng = NULL;
  }
  tctx->trng = TRNG_new(type);
  if((NULL ==  tctx->trng) ) {
    tctx->state = SP800_90CRIT;
    tctx->error_reason = ERRAT(SP800_90_NOT_INIT);
  }
  tctx->Auto = 1; /* Automatically reseeds, by definition */
  return tctx->state;
}

SP800_90STATE TRNG_FIPS_Instantiate(PRNG_CTX *ctx,
				unsigned char *ein, unsigned int einl,
				unsigned char *nonce, unsigned int nonl,
				unsigned char *person,unsigned int perl
				)
{
  return TRNG_Inst_Type(ctx,ein,einl,nonce,nonl,person,perl,TRNG_FIPS);
}

SP800_90STATE TRNG_ALT_Instantiate(PRNG_CTX *ctx,
				unsigned char *ein, unsigned int einl,
				unsigned char *nonce, unsigned int nonl,
				unsigned char *person,unsigned int perl
				)
{
  return TRNG_Inst_Type(ctx,ein,einl,nonce,nonl,person,perl,TRNG_OS);
}


SP800_90STATE TRNG_ALT4_Instantiate(PRNG_CTX *ctx,
				unsigned char *ein, unsigned int einl,
				unsigned char *nonce, unsigned int nonl,
				unsigned char *person,unsigned int perl
				)
{
  SP800_90STATE rv =  SP800_90UNINIT;
  SP800_90PRNG_Data_t *tctx = (SP800_90PRNG_Data_t *)ctx;
  if(! ALT4_Avail() ) {
    tctx->state = SP800_90ERROR;
    tctx->error_reason = ERRAT("This mode requires a Hardware RNG which was not detected");
  } else {
    rv = TRNG_Inst_Type(ctx,ein,einl,nonce,nonl,person,perl,TRNG_HW);
  }
  return rv;
}


SP800_90STATE TRNG_ReSeed(PRNG_CTX *ctx,
			 unsigned char *ein, unsigned int einl,
			 unsigned char *adata,unsigned int adatal)
{
  SP800_90PRNG_Data_t *tctx = (SP800_90PRNG_Data_t *)ctx;

  if(NULL == tctx->trng) {
    tctx->state = SP800_90ERROR;
    tctx->error_reason = ERRAT("NRBG has not been initialised");
  } else {
    tctx->state = SP800_90RUN;
  }
  return tctx->state;
}

/*!
  @brief output from an ICC NRBG
*/
SP800_90STATE TRNG_Generate(PRNG_CTX *ctx,
			   unsigned char *buffer,unsigned int blen,
			   unsigned char *adata,unsigned int adatal)
{
 
  SP800_90PRNG_Data_t *tctx = (SP800_90PRNG_Data_t *)ctx;

  if(NULL == tctx->trng) {
    tctx->state = SP800_90ERROR;
    tctx->error_reason = ERRAT("NRBG has not been initialised");
  } else {
    if( TRNG_OK == TRNG_GenerateRandomSeed(tctx->trng,blen,buffer) ) {
      tctx->state = SP800_90RUN;
    } else {
      tctx->state = SP800_90CRIT;
      tctx->error_reason = ERRAT("NRBG entropy fell below limits");
    }
  }
  return tctx->state;
}


SP800_90STATE TRNG_ETAP_Generate(PRNG_CTX *ctx,
			   unsigned char *buffer,unsigned int blen,
			   unsigned char *adata,unsigned int adatal)
{
 
  SP800_90PRNG_Data_t *tctx = (SP800_90PRNG_Data_t *)ctx;
  TRNG *trng = tctx->trng;
  TRNG_ERRORS rv = TRNG_OK;

  if(NULL == trng) {
    tctx->state = SP800_90ERROR;
    tctx->error_reason = ERRAT("NRBG has not been initialised");
  } else {
    if( 0 != trng_raw(&(trng->econd),buffer,blen) ) {
      rv = SetRNGError("Repeated health test fails",__FILE__,__LINE__);
      if(TRNG_OK != rv) {
        tctx->state = SP800_90ERROR;
        tctx->error_reason = ERRAT("Repeated health test fails");
      }  
    }
  }
  return tctx->state;
}
SP800_90STATE TRNG_NOISE_Generate(PRNG_CTX *ctx,
			   unsigned char *buffer,unsigned int blen,
			   unsigned char *adata,unsigned int adatal)
{
 
  SP800_90PRNG_Data_t *tctx = (SP800_90PRNG_Data_t *)ctx;
  TRNG *trng = tctx->trng;


  if(NULL == tctx->trng) {
    tctx->state = SP800_90ERROR;
    tctx->error_reason = ERRAT("NRBG has not been initialised");
  } else {
    trng->econd.impl.gb(&(trng->econd),buffer,blen);     
  }
  return tctx->state;
}

/*!
  @brief Cleanup function for TRNG based PRNG's
  @param ctx The PRNG_CTX to cleanup 
  all allocated data is scrubbed and released. 
*/
SP800_90STATE TRNG_CleanupX(PRNG_CTX *ctx)
{
  SP800_90PRNG_Data_t *tctx = (SP800_90PRNG_Data_t *)ctx;
  char *reason = tctx->error_reason;
  if(NULL != tctx->trng) {
    TRNG_free(tctx->trng);
    tctx->trng = NULL;
  }
  tctx->state = SP800_90UNINIT;
  tctx->error_reason = reason;
  return tctx->state;
}

/*!
  @brief return the type of a TRNG 
  @return the TRNG type
*/
TRNG_TYPE TRNG_type(TRNG *T)
{
  return T->type;
}

SP800_90PRNG_t SPTRNG_FIPS = {
  SP800_TRNG_FIPS,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "TRNG_FIPS",
  "TRNG_FIPS",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_FIPS_Instantiate,
  TRNG_ReSeed,
  TRNG_Generate,
  TRNG_CleanupX, 
  SP800_IS_FIPS, 
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};

SP800_90PRNG_t SPTRNG_ALT = {
  SP800_TRNG_ALT,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "TRNG_ALT",
  "TRNG_ALT",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_ALT_Instantiate,
  TRNG_ReSeed,
  TRNG_Generate,
  TRNG_CleanupX, 
  SP800_NON_FIPS, /* Not for use as a PRNG in FIPS mode, 
		   it's NOT a PRNG so that's fine */
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};

SP800_90PRNG_t SPTRNG_OS = {
  SP800_TRNG_OS,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "TRNG_OS",
  "TRNG_OS",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_ALT_Instantiate,
  TRNG_ReSeed,
  TRNG_Generate,
  TRNG_CleanupX, 
  SP800_NON_FIPS, /* Not for use as a PRNG in FIPS mode, 
		   it's NOT a PRNG so that's fine */
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};


SP800_90PRNG_t SPTRNG_ALT4 = {
  SP800_TRNG_ALT4,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "TRNG_ALT4",
  "TRNG_ALT4",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_ALT4_Instantiate,
  TRNG_ReSeed,
  TRNG_Generate,
  TRNG_CleanupX, 
  SP800_NON_FIPS, 
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};

SP800_90PRNG_t SPTRNG_HW = {
  SP800_TRNG_HW,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "TRNG_HW",
  "TRNG_HW",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_ALT4_Instantiate,
  TRNG_ReSeed,
  TRNG_Generate,
  TRNG_CleanupX, 
  SP800_NON_FIPS, 
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};

SP800_90PRNG_t SPTRNG_FIPS_ETAP = {
  SP800_TRNG_FIPS_ETAP,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "ETAP_FIPS",
  "ETAP_FIPS",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_FIPS_Instantiate,
  TRNG_ReSeed,
  TRNG_ETAP_Generate,
  TRNG_CleanupX,
  SP800_NON_FIPS, /* Test tap */
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};

SP800_90PRNG_t SPTRNG_ALT_ETAP = {
  SP800_TRNG_ALT_ETAP,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "ETAP_ALT",
  "ETAP_ALT",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_ALT_Instantiate,
  TRNG_ReSeed,
  TRNG_ETAP_Generate,
  TRNG_CleanupX, 
  SP800_NON_FIPS, /* Test tap for external test */
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};

SP800_90PRNG_t SPTRNG_OS_ETAP = {
  SP800_TRNG_OS_ETAP,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "ETAP_OS",
  "ETAP_OS",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_ALT_Instantiate,
  TRNG_ReSeed,
  TRNG_ETAP_Generate,
  TRNG_CleanupX, 
  SP800_NON_FIPS, /* Test tap for external test */
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};

SP800_90PRNG_t SPTRNG_ALT4_ETAP = {
  SP800_TRNG_ALT4_ETAP,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "ETAP_ALT4",
  "ETAP_ALT4",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_ALT4_Instantiate,
  TRNG_ReSeed,
  TRNG_ETAP_Generate,
  TRNG_CleanupX, 
  SP800_NON_FIPS, /* Test tap only */
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};

SP800_90PRNG_t SPTRNG_HW_ETAP = {
  SP800_TRNG_HW_ETAP,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "ETAP_HW",
  "ETAP_HW",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_ALT4_Instantiate,
  TRNG_ReSeed,
  TRNG_ETAP_Generate,
  TRNG_CleanupX, 
  SP800_NON_FIPS, /* Test tap only */
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};


SP800_90PRNG_t SPTRNG_FIPS_NOISE = {
  SP800_TRNG_FIPS_NOISE,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "NOISE_FIPS",
  "NOISE_FIPS",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_FIPS_Instantiate,
  TRNG_ReSeed,
  TRNG_NOISE_Generate,
  TRNG_CleanupX, 
  SP800_NON_FIPS, /* Test tap, raw noise */
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};

SP800_90PRNG_t SPTRNG_ALT_NOISE = {
  SP800_TRNG_ALT_NOISE,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "NOISE_ALT",
  "NOISE_ALT",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_ALT_Instantiate,
  TRNG_ReSeed,
  TRNG_NOISE_Generate,
  TRNG_CleanupX, 
  SP800_NON_FIPS, /* Tap for raw noise source */
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};

SP800_90PRNG_t SPTRNG_OS_NOISE = {
  SP800_TRNG_OS_NOISE,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "NOISE_OS",
  "NOISE_OS",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_ALT_Instantiate,
  TRNG_ReSeed,
  TRNG_NOISE_Generate,
  TRNG_CleanupX, 
  SP800_NON_FIPS, /* Tap for raw noise source */
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};

SP800_90PRNG_t SPTRNG_ALT4_NOISE = {
  SP800_TRNG_ALT4_NOISE,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "NOISE_ALT4",
  "NOISE_ALT4",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_ALT4_Instantiate,
  TRNG_ReSeed,
  TRNG_NOISE_Generate,
  TRNG_CleanupX, 
  SP800_NON_FIPS, /* Test tap */
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};

SP800_90PRNG_t SPTRNG_HW_NOISE = {
  SP800_TRNG_HW_NOISE,
  20,          /* retained seedlen */
  0,           /* max nonce - unused */
  0,           /* max aad - unused */
  (1<<11),     /* max bytes in one request */
  0xFFFFFFFFL, /* Max calls before reseeding- always self reseeds */
  20,          /* Internal block size */
  0,           /* max entropy in - unused */ 
  256,         /* max personalization data */
  {256,
   0,
   0,
   0,
  },
  "NOISE_HW",
  "NOISE_HW",
  0,              /*!< Has a derivation function */
  Inst,
  Res,
  Gen,
  Cln,
  TRNG_ALT4_Instantiate,
  TRNG_ReSeed,
  TRNG_NOISE_Generate,
  TRNG_CleanupX, 
  SP800_NON_FIPS, /* Test tap */
  -1,             /* Retest interval */
  -1,             /* Not self tested via this interface */         
  {
    { 
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
    {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    },
  },
};
