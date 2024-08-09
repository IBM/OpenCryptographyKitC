/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Internal (ICC use only) Structure definitions 
// for  SP800-90 RNG modes
// These are shared with the ICC test cases.
//
*************************************************************************/


#if !defined(SP800_90I_H)
#include "TRNG/ICC_NRBG.h"
#if OPENSSL_VERSION_NUMBER <  0x10100000L
#  include "openssl/hmac.h"
#endif
#include "fips-prng-err.h"

/*! @brief Macros to improve error handling */

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT  " :" __FILE__ ":" TOSTRING(__LINE__)

#define ERRAT(x) x AT

/* common error strings */

#define SP800_90_REQUESTED_STRENGTH "SP800-90 (DRBG) requested security strength is too high for the chosen algorithm"
#define SP800_90_BAD_STATE "The RNG was in an unexpected state"
#define SP800_90_NOT_INIT "The RNG is not initialized"
#define SP800_90_EXCESS_AAD "More than the allowed additional data was provided"
#define SP800_90_EXCESS_PERS "More than the allowed personalization data was provided"
#define SP800_90_EXCESS_ENT "Supplied data + internal entropy exceeds allowed input limits"
#define SP800_90_MIN_ENT "Less than the required minimum entropy was supplied"
#define SP800_90_DF_ENT "For no-df modes exactly seedlen bytes of entropy are required"
#define SP800_90_EXCESS_NONCE "More than the allowed nonce data was supplied"
#define SP800_90_EXCESS_TOTAL "The total of entropy + nonce + personalization data was greater than permitted"
#define SP800_90_EXCESS_DATA "The data request was greater than allowed in this mode"
#define SP800_90_CONTINUOUS "The DRBG returned the same data twice"

#define ICC_GUARANTEED_ENTROPY 2   /* 1 bit of entropy/2 bits in */

/*! @brief enum defining the ICC SP800_90 DRBG usability
  _COND means that although the mode passes the KAT tests 
  the ICC DRBG isn't good enough in theory for it to be usable 
  internally.
*/
typedef enum {
  SP800_NON_FIPS = 0,
  SP800_IS_FIPS = 1,
  SP800_IS_FIPS_COND = 2 
} SP800_FIPS;

/* Key, note that this has to cope with HAMC keys as well as AES
 */
#define MAX_K 64
/* IV */
#define MAX_IV 16
/* Max retained data in any mode */
#define MAX_V (888/8)
/* Max retained data in any mode */
#define MAX_C MAX_V
/* max key length + max block size, or max hash size 
   or scratch space, i.e. MAX_V
*/
#define MAX_T (MAX_V)

/* Highest strength possible */
#define MAX_STRENGTH 256
/* Maximum number of bytes we'll have to extract from ICC's NRBG
   to guarantee meeting the entropy requirements
*/
#define EBUF_SIZE (MAX_STRENGTH * ICC_GUARANTEED_ENTROPY)

/*! \FIPS The number of times the NRBG can be instantiated 
  before self test is re-run
  @note Self test is run before the first usage.
*/
#define SELF_TEST_AT 1024

/*! Defines the amount of retained data for the continuous tests */
#define CNT_SZ 8

/*! \IMPLEMENT SP800 DRBG's  
  Note that the functions hidden inside the PRNG_CTX structure can
  accept more input than the public outer level functions that call them.
  This is done so that we can grab a context then call these functions 
  directly during formal testing.
  In use, the upper layer functions pass "NULL,0" for the normally
  unused (internally provided) data.
  @note There's no separate DRBG cleanup call in the ICC API. 
  free calls the cleanup methods before releasing the context.

  
*/




/* These are typedefs for the low level algorithm specific methods 
 */					  
typedef SP800_90STATE SP800_90Instantiate(PRNG_CTX *pctx,
					  unsigned char *ein,unsigned einl,
					  unsigned char *nonce, unsigned nonl,
					  unsigned char *person,unsigned perl);

typedef SP800_90STATE SP800_90ReSeed(PRNG_CTX *pctx,
				     unsigned char *ein,unsigned einl,
				     unsigned char *adata,unsigned adatal);

typedef SP800_90STATE SP800_90Generate(PRNG_CTX *pctx, 
				       unsigned char *buffer,unsigned blen,
				       unsigned char *adata,unsigned adatal);

typedef SP800_90STATE SP800_90Cleanup(PRNG_CTX *pctx); 

/* Typedefs for the common high level generic access points which 
   handle state transitions etc and call the lower level
   methods
   We expose these via the SP800_90 structure so we can drive everything
   "properly" during formal testing
*/
typedef void SP800_90Inst(PRNG_CTX *pctx,
			  unsigned char *ein,unsigned einl,
			  unsigned char *nonce, unsigned nonl,
			  unsigned char *person,unsigned perl);


typedef void SP800_90Res(PRNG_CTX *pctx,
			 unsigned char *ein,unsigned einl,
			 unsigned char *adata,unsigned adatal);

typedef void SP800_90Gen(PRNG_CTX *pctx, 
			 unsigned char *buffer,unsigned blen,
			 unsigned char *adata,unsigned adatal);

typedef void SP800_90Cln(PRNG_CTX *pctx); 


/*!
  @brief Data structure to handle variable length NIST known answer vectors
  @note Because ICC uses known answer vectors that are missing data for
  some fields, and the normal operating modes would internally generate data
  for the missing elements, a 0 length data item with a non-NULL pointer is 
  provided in test cases where this data is to be left out.
  i.e. to have data generated 0,NULL would be specified
  to avoid this 0,{0x00} is used. A zero length string, with a 
  valid pointer - pointing to a 0 byte. 
  (Any string COULD be used).
  @note HPUX/i5 compilers can't handle initialization of structures
  containing indefinate arrays.
  @note SUN compilers generate the correct code, but whine about it

*/
typedef struct {
  const unsigned char term;
  const unsigned char len;
  const unsigned char buf[256]; 
} StringBuf;

/*! @brief defines the self test data structures for the DRBG's
    Use the NIST test data where available,
    prediction resistance is enabled as that
    tests all the input paths
    Only one round of output - this is only a self test to 
    ensure we are still working, not a full formal test suite.
 */
typedef struct {
  const StringBuf *InitEin;    /*!< Initialization entropy  */
  const StringBuf *InitNonce;  /*!< Initialization Nonce */
  const StringBuf *InitPerson; /*!< Initialization Personalization */
  const StringBuf *GenAAD;     /*!< Generate additional data */ 
  const StringBuf *GenEin;     /*!< Generate entropy - data for reseed */
  const StringBuf *GenRes;     /*!< Generate result */
} SP800_90_test;

/*!
  @brief defines the algorithm specific data for a DRBG 
*/
typedef struct  {
  const SP800_90PRNG_mode type;  /*!< The DRBG type, used for searching during setup */
  const unsigned seedlen;        /*!< The retained seedlen for this mode (bytes) */
  const unsigned maxNonce;       /*!< Maximum length of a Nonce (bytes) */
  const unsigned maxPers;        /*!< Maximum personalization data allowed (bytes) */
  const unsigned maxAAD;         /*!< Maximum additional data length (bytes)*/ 
  const unsigned maxBytes;       /*!< Maximum bytes in one request (bytes) */
  const unsigned maxReseed;      /*!< Maximum CALLS to the DRBG before reseeding (NOT BYTES) */
  const unsigned OBL;            /*!< Output block length (bytes) */
  const unsigned maxEnt;         /*!< Maximum entropy in (bytes) */
  const unsigned secS[4];        /*!< Supported security strengths, either the value or 0, BITS */
  const char *specific;          /*!< Algorithm specific id, generally algorithm name "SHA1" etc */
  const char *prngname;          /*!< The algorithm name, as passed to get_DRBGbyname() */
  const int hasDF;               /*!< The algorithm implements a derivation function */
  SP800_90Inst *Inst;           /*!< The common high level instantiation function */
  SP800_90Res *Res;             /*!< The common high level reseeding function */
  SP800_90Gen *Gen;             /*!< The common high level generate function */  
  SP800_90Cln *Cln;             /*!< The common high level cleanup function */
  SP800_90Instantiate *Init;    /*!< The method specific instantiation function */
  SP800_90ReSeed *ReSeed;       /*!< The method specific reseeding function */
  SP800_90Generate *Generate;   /*!< The method specific DRBG generator */
  SP800_90Cleanup *Cleanup;     /*!< The method specific cleanup function, erases all internal state */
  const SP800_FIPS FIPS;        /*!< Set to SP800_IS_FIPS for modes that can pass the FIPS tests,
				     we may end up with these apparently functional, but
				     with bugs that mean they either fail known answer tests
				     or can't be compliant (The no-df Cipher modes for example)
				*/
  const int test_at;            /*!< Self test again after this many instantiations */
  int last_tested_at;           /*!< Self test requirements. Self test at count 0 and reload */
  SP800_90_test TestData[4];    /*!< Self test vectors */
  int error;                    /*!< 40770 , error in self test */
  ICC_Mutex mtx;                /*!< 41221 Need locking on the type */
} SP800_90PRNG_t;      


/*!
  @brief defines the instance specific data for the DRBG
*/
typedef struct {
  unsigned char K[MAX_K];  /*!< Working key for cipher modes*/
  unsigned char V[MAX_V];  /*!< Working space/restained data for digest mode */
  unsigned char C[MAX_C];  /*!< Working space/restained data for digest mode
			        Working space in cipher modes */
  unsigned char T[MAX_T];  /*!< Scratch space */
  unsigned char eBuf[EBUF_SIZE]; /*!< A scratch buffer used to collect NRBG input,
				   and DRBG output. It's scrubbed after each use */
  unsigned int TestMode;       /*!< Set if we are testing the DRBG, supresses scrubbing if fed in (const) data buffers */
  unsigned int SecStr;         /*!< The desired security strength in bits 112, 128,192,256 */
  unsigned int ReseedAt;       /*!< Number of CALLS (not bytes) before we Reseed */
  unsigned int Paranoid;       /*!< Enable Prediction resistance, continual reseed   */ 
  unsigned int minEnt;         /*!< minimum entropy for instantiate, reseed or input (worst case) */
  unsigned int Auto;            /*!< Automatic, auto-reseed, auto-chunk large requests*/
  union {
   unsigned char c[4];         /*!< the number of times the Generate function was called since last seeded */
   uint32_t u;                 /* it's merged in the PRNG's */
  } CallCount;
  SP800_90STATE state;         /*!< The DRBG state - includes extra self test states */
  SP800_90PRNG_t *prng;        /*!< A pointer to the instance of this class of DRBG */
  union {
    const EVP_MD *md;          /*!< ICC Message digest specific to this instance */
    const EVP_CIPHER *cipher;  /*!< ICC Cipher specific to this instance */
  } alg;
  union {
    EVP_MD_CTX *md_ctx;        /*!< Message digest context associated with this instance */
    EVP_CIPHER_CTX *cctx;      /*!< Cipher context associated with this instaance */
    HMAC_CTX *hmac_ctx;        /*!< HMAC ctx used in HMAC modes */
  } ctx;
  char * error_reason;         /*!< A short reason for a failure */
  TRNG *trng;                  /*!< The seed source for this DRBG instance */
  unsigned char lastdata[CNT_SZ];   /*!< The first 8 bytes of the last data request */
#if !defined(_WIN32)
  pid_t lastPID;               /* The PID on the last call to generate, auto-reseed on fork() - lacking on Windows */
#endif
} SP800_90PRNG_Data_t;


/*!
  @brief common sanity checks/setup for DRBG Instantiation
  @param ctx a partially initialized DRBG context
  @param ein a pointer to the entropy input buffer. (May be NULL)
  @param einl a pointer to the length of the provided entropy
  @param nonce additional entropy
  @param nonl a pointer to the length of the provided entropy
  @param person a pointer to the personalization data
  @param perl a pointer to the length of the personalization data
  @param flags a pointer to the flags used to track data we've allocated
  @note In normal use we'd expect to arrive here with ein,einl, nonce, nonl 
  as NULL,0,NULL,0. But during testing we call the functions embedded in the
  PRNG_CTX directly to drive the code and supply the known input data through
  here.
*/
SP800_90STATE PRNG_Instantiate(SP800_90PRNG_Data_t *ctx,
			       unsigned char **ein, unsigned int *einl,
			       unsigned char **nonce, unsigned int *nonl,
			       unsigned char **person,unsigned int *perl,
			       unsigned int *flags);
/*!
  @brief Utility function to release data allocated
  internally during operation
  @param ein a pointer to possibly allocated entropy buffer
  @param nonce a pointer to a possibly allocated nonce
  @param person a pointer to a possibly allocated personalization string
  @param flags A set of flags which trackwhich of the parameters were allocated.
  @note The data is assumed to have been scrubbed elsewhere
*/
void PRNG_free_scratch(unsigned char **ein,
		       unsigned char **nonce, 
		       unsigned char **person,
		       unsigned int *flags);

/*!
  @brief A common NRBG shared with all the DRBG's
  @param P the prng to use
  @param n the number of bytes to extract from the NRBG
  @param buf where to store the extracted data
  @return NRBG state
*/
TRNG_ERRORS PRNG_GenerateRandomSeed(PRNG_CTX *P,unsigned int n, unsigned char *buf);

/*!
  @brief return the number of blocks needed to fill N bytes of output
  @return number of blocks needed.
  @note Rounds up so it generates AT LEAST bytes of output
  
*/
unsigned BlocksReqd(unsigned int bytes,unsigned int blocksize);

/*!
  @brief convert an unsigned int to a big endian 4 byte byte stream
  @param n the unsigned int to convert
  @param N the output byte stream
*/
void uint2BS(unsigned n,unsigned char N[4]);


/** @brief xor two buffers into a destination. 
    (Which may be one of the source buffers )
    @param dest the destination buffer
    @param s1 first source buffer
    @param s2 second source buffer
    @param blen buffer length
*/

void xor(unsigned char *dest, unsigned char *s1, unsigned char *s2, unsigned blen);

/*!
  @brief  Binary add src1 + src2 + cyin result in dest
  Numbers are assumed little endian
  @param dest destination,may be the same as src1 or src2
  @param src1 source data 1
  @param s1 length of s1 and dest
  @param src2 source data 2
  @param s2 length of src2
  @note s2 may be set to 0, in which case s1 is used for s2.
  s2 = 1 and src->0, A single "1" byte is typical for an increment.
*/
void Add(unsigned char *dest,
	 unsigned char *src1,unsigned int s1, 
	 unsigned char *src2,unsigned int s2);

/*!
  @brief DRBG self test function. run self test on all usable strengths 
  for this algorithm
  Error reporting - critical errors are persistant and result in a 
  not usable context
  In non-debug modes errors are treated as FIPS
  critical.
  @param ctx a DRBG context
  @param alg a DRBG algorithm
  @note This code doesn't (currently) self test the NRBG or X9.31 modes
*/
void PRNG_self_test(PRNG_CTX *ctx,PRNG *alg);

/*! 
  @brief Internal "Instantiate" function which encapsulates
  the necessary state transition logic common to all modes
  Calls the method specific Instantiate() to to the algorithmic heavy hauling
  @param ctx A DRBG ctx pointer
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
	  );

/*! 
  @brief Internal "ReSeed" function which encapsulates
  the necessary state transition logic common to all modes
  Calls the method specific ReSeed() to to the algorithmic heavy hauling
  @param ctx A DRBG ctx pointer
  @param Ein Entropy input
  @param Einl length of entropy input
  @param Adata Additional data
  @param Adatal Additional data length
*/
void Res(PRNG_CTX *ctx,
	 unsigned char *Ein,unsigned int Einl,
	 unsigned char *Adata,unsigned int Adatal);

/*! @brief Internal "Generate" function which encapsulates
  the necessary state transition logic common to all modes
  Calls the method specific Generate() to to the algorithmic heavy hauling
  @param ctx A DRBG ctx pointer
  @param out The output buffer
  @param outl The number of bytes to return
  @param Adata Additional data
  @param Adatal Additional data length
*/
void Gen(PRNG_CTX *ctx,
	 unsigned char *out,unsigned int outl,
	 unsigned char *Adata,unsigned int Adatal);

/*! 
  @brief Internal "Cleanup" function which encapsulates
  the necessary state transition logic common to all modes
  @param ctx a PDRBG_CTX pointer
*/
void Cln(PRNG_CTX *ctx);



          
#endif
