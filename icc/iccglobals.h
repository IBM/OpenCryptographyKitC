/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/
/*
// Description: Global definitions ubiquitous across ICC
*/

/** \file iccglobals.h 
 * @brief ICC Global variable and structure definitions (ICCSDK)
 * This should only be included via icc.h
 */
 
#ifndef INCLUDED_ICCGLOBALS
#define INCLUDED_ICCGLOBALS

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#define ICC_LINKAGE __cdecl
#else
#define ICC_LINKAGE
#endif

#include<stddef.h>
/*! @brief
  These are the error codes returned from ICC API calls where the return type allows an error to be returned.
  - Note that where a function returns a pointer, ICC will return NULL if an ICC API error occurs.
  - Note the difference between ICC_OSSL_FAILURE and ICC_OSSL_FAILURE_TOO.
  - ICC_OSSL_FAILURE on a verify operation means that the verify failed.
  - ICC_OSSL_FALURE_TOO means that something prevented the verification being done - RNG wasn't initialized for example
  - ICC_FAILURE can occur if ICC wasn't initialized correctly or a FIPS self test error occurred and the API is locked.
  - ICC_NOT_IMPLEMENTED added for ICCPKG. With two ICC's under the hood, some functions may be missing
*/
typedef enum {
  ICC_NOT_IMPLEMENTED   = -3, /*!< ICCPKG only, function not available */  
  ICC_FAILURE 	        = -2, /*!< Return value for ICC internal failure,i.e. ICC not initialized, FIPS error */
  ICC_OSSL_FAILURE_TOO 	= -1, /*!< Also used by OpenSSL on failure. "operation could not complete due to internal error" */
  ICC_OSSL_FAILURE      =  0, /*!< Return value for OpenSSL failure. "operation failed" return code   */
  ICC_OSSL_SUCCESS      =  1  /*!< OpenSSL success value */
} ICC_RC_ENUM;

#define ICC_INTERNALNAME "ICC"

struct ICC_t;

/*! @brief
  This is the ICC context structure. 
  It's allocated by a call to ICC_Init, 
  initialized by a call to ICC_Attach,
  and destroyed by a call to ICC_Cleanup.
  This should not be created or destroyed in aany other way.
  It's used to hold internal state needed for ICC's function.
  Most applications need at most two ICC contexts,
  - FIPS
  - Non-FIPS
  The structure is quite large, and there is no performance gain
  in having more than these two. 
*/
typedef struct ICC_t ICC_CTX;

typedef struct ossl_param_st OSSL_PARAM;

struct ICC_PRNG_t;

/*! 
  @brief ICC_PRNG is the opaque handle to the internal PRNG definitions
 */
typedef struct ICC_PRNG_t ICC_PRNG;

struct ICC_PRNG_CTX_t;
/*!
  @brief ICC_PRNG_CTX is the opaque handle to an instance of a PRNG
*/
typedef struct ICC_PRNG_CTX_t ICC_PRNG_CTX;

/*Defines from Openssl*/

/*! @brief
    NID for and empty generic asymetric key
    Returned by EVP_CIPHER_type()
    Generally not used by applications.
*/
#define ICC_EVP_PKEY_NONE       0

/*! @brief
    NID for an RSA generic asymetric key
    Returned by EVP_CIPHER_type()
    Generally not used by applications.
*/
#define ICC_EVP_PKEY_RSA        6

/*! @brief
    NID for a DSA generic asymetric key
    Returned by EVP_CIPHER_type()
    Generally not used by applications.
*/
#define ICC_EVP_PKEY_DSA      116

/*! @brief
    NID for a DH generic asymetric key
    Returned by EVP_CIPHER_type()
    Generally not used by applications.
*/
#define ICC_EVP_PKEY_DH        28

/*! @brief
    NID for an EC generic asymetric key
    Returned by EVP_CIPHER_type()
    Generally not used by applications.
*/
#define ICC_EVP_PKEY_EC        408
            
/*! @brief 
  Maximum possible size of an ICC message digest.  
  SHA-512 is 64 bytes. So far there are no combination modes
  that need more.
*/
#define ICC_EVP_MAX_MD_SIZE  (64)

/*! @brief
  Definitions for the values passed to EVP_CIPHER_CTX_ctrl
*/
typedef enum {
  ICC_EVP_CTRL_INIT                   = 0, /*!< Init, unused */
  ICC_EVP_CTRL_SET_KEY_LENGTH         = 1, /*!< Generic set key length, unused ? */
  ICC_EVP_CTRL_GET_RC2_KEY_BITS       = 2, /*!< Get RC2 key length */
  ICC_EVP_CTRL_SET_RC2_KEY_BITS       = 3 , /*!< Set RC2 key length - only way to set 'odd' RC2 key lengths */
  ICC_EVP_CTRL_GCM_SET_IVLEN          = 0x09, /*!< Set GCM IVLength */
  ICC_EVP_CTRL_GCM_GET_TAG            = 0x10, /*!< Get the GCM tag */
  ICC_EVP_CTRL_GCM_SET_TAG            = 0x11, /*!< Set the GCM tag */
  ICC_EVP_CTRL_GCM_SET_IV_FIXED       = 0x12, /*!< Provide the IV externally */
  ICC_EVP_CTRL_AEAD_TLS1_AAD          = 0x16, /*!< Provide the AAD */
  ICC_EVP_CTRL_GCM_SET_IV_INV         = 0x18  /*!< Decrypt only */
} ICC_EVP_CTRL_ENUM;

/*! @brief
  Definitions for RSA key padding types.
  Note: Not all types are usable in all modes.
*/
typedef enum {
  ICC_RSA_PKCS1_PADDING	        =1, /*!< PKCS#1 */
  ICC_RSA_SSLV23_PADDING        =2, /*!< SSLV2/SSLV3 normally used only in SSL */
  ICC_RSA_NO_PADDING            =3, /*!< No padding */
  ICC_RSA_PKCS1_OAEP_PADDING    =4, /*!< OAEP Padding */
  ICC_RSA_X931_PADDING          =5, /*!< X931 padding */
  ICC_RSA_PKCS1_PSS_PADDING     =6  /*!< RSASA-PSS, only usable via EVP_Digest[Sign/Verify] */
} ICC_RSA_PADDING_ENUM;


/*! @brief
  Definitions for locking operations. Lock command values, num locks
  - Lock mode is specified by or'ing values together, not all combinations are valid.
  - Deprecated. While correct these definitions have not been usable by ICC API users since ICC 1.2.1
  - The calls which used them still exist, but have no effect now.
  - These are still used internally by ICC.
  - These are in here because this header gets scanned for consistency    
    with the OpenSSL version we use during BVT and it's vital    
    that these values are correct.            
*/
typedef enum {
  ICC_CRYPTO_LOCK               =1,  /*!< Lock operation */
  ICC_CRYPTO_UNLOCK             =2,  /*!< Unlock operation */
  ICC_CRYPTO_READ               =4,  /*!< READ lock */
  ICC_CRYPTO_WRITE              =8,  /*!< Write lock */
  ICC_CRYPTO_NUM_LOCKS          =41  /*!< Total number of locks needed */
} ICC_CRYPTO_LOCK_ENUM;


/*! @brief
  Definitions for locking operations. Lock numbers.
  - Deprecated. While correct these definitions have not been usable by ICC API users since ICC 1.2.1
  - The calls which used them still exist, but have no effect now.
  - These are used internally by ICC.
*/

typedef enum {
  ICC_CRYPTO_LOCK_ERR           =1,
  ICC_CRYPTO_LOCK_EX_DATA       =2,
  ICC_CRYPTO_LOCK_RSA           =9,
  ICC_CRYPTO_LOCK_EVP_PKEY      =10,
  ICC_CRYPTO_LOCK_RAND          =18,
  ICC_CRYPTO_LOCK_RAND2         =19,
  ICC_CRYPTO_LOCK_MALLOC        =20,
  ICC_CRYPTO_LOCK_MALLOC2       =27
} ICC_CRYPTO_LOCK_TYPE_ENUM;

/* Magic numbers for EVP_PKEY_CTX_ctrl
   Note these aren't enums as some are reused
   for different purposes with different underlying algorithms
   Note that these only work with EVP_Digest[Sign/Verify]
*/
#define  ICC_EVP_PKEY_ALG_CTRL 0x1000

/* DSA2 related numbers */
#define	ICC_EVP_PKEY_CTRL_DSA_PARAMGEN_BITS (ICC_EVP_PKEY_ALG_CTRL + 1)
#define	ICC_EVP_PKEY_CTRL_DSA_PARAMGEN_Q_BITS (ICC_EVP_PKEY_ALG_CTRL + 2)
#define	ICC_EVP_PKEY_CTRL_DSA_PARAMGEN_MD (ICC_EVP_PKEY_ALG_CTRL + 3)

/* RSASA-PSS related numbers */
#define ICC_EVP_PKEY_CTRL_RSA_PADDING (ICC_EVP_PKEY_ALG_CTRL + 1)
#define ICC_EVP_PKEY_CTRL_RSA_PSS_SALTLEN (ICC_EVP_PKEY_ALG_CTRL + 2)
/* Note that it's a const ICC_EVP_MD * that's passed or returned
   (return from ICC_EVP_get_digestbyname())
*/
#define ICC_EVP_PKEY_CTRL_RSA_MGF1_MD	(ICC_EVP_PKEY_ALG_CTRL + 5)
#define ICC_EVP_PKEY_CTRL_GET_RSA_PADDING (ICC_EVP_PKEY_ALG_CTRL + 6)
#define ICC_EVP_PKEY_CTRL_GET_RSA_PSS_SALTLEN (ICC_EVP_PKEY_ALG_CTRL + 7)
#define ICC_EVP_PKEY_CTRL_GET_RSA_MGF1_MD (ICC_EVP_PKEY_ALG_CTRL + 8)
/*! @brief
   Magic numbers for Diffie-Hellman key generation.
   These are the supported types.
*/

/*Param names used in OSSL_PARAMS Scraped from Openssl v3 "core_names.h"*/

#define ICC_OSSL_KDF_PARAM_THREADS           "threads"
#define ICC_OSSL_KDF_PARAM_ARGON2_LANES      "lanes"
#define ICC_OSSL_KDF_PARAM_ARGON2_MEMCOST    "memcost"
#define ICC_OSSL_KDF_PARAM_SALT              "salt"
#define ICC_OSSL_KDF_PARAM_PASSWORD          "pass"
#define ICC_OSSL_PARAM_UNSIGNED_INTEGER      2
#define ICC_OSSL_PARAM_OCTET_STRING          5
#define ICC_OSSL_KDF_PARAM_ARGON2_VERSION    "version"
#define ICC_OSSL_KDF_PARAM_MODE              "mode"
#define ICC_OSSL_PARAM_UNMODIFIED           ((size_t)-1)
#define ICC_OSSL_PARAM_END                   { NULL, 0, NULL, 0 }

typedef enum {
  ICC_DH_GENERATOR_2	  = 2, /*!< Type 2 key generator */
  ICC_DH_GENERATOR_5      = 5  /*!< Type 5 key generator */
} ICC_DH_GENERATOR_ENUM;
	
 

/*
  Major return codes found in ICC_STATUS (majRC)
*/
typedef enum {
  ICC_OK                  = 0, /*!< No error */
  ICC_WARNING             = 1, /*!< No error, but results may not be as expected. Use of a deprecated API call for example */
  ICC_ERROR               = 2, /*!< An error occurred in the ICC API */
  ICC_OPENSSL_ERROR       = 3, /*!< An error occurred in an OpenSSL call */
  ICC_OS_ERROR            = 4  /*!< An error occurred in a Operating System call */
} ICC_MAJOR_RC_ENUM;

/*! @brief
  Minor codes found in ICC_STATUS (minRC) 
*/
typedef enum {
  ICC_NULL_PARAMETER              =  0, /*!< A parameter was NULL, and was expected to have a value */
  ICC_UNSUPPORTED_VALUE_ID        =  1, /*!< An unsupported command was passed */
  ICC_UNSUPPORTED_VALUE           =  2, /*!< An invalid value was passed */
  ICC_LIBRARY_NOT_FOUND           =  3, /*!< One of the ICC shared libraries was not found */
  ICC_LIBRARY_VERIFICATION_FAILED =  4, /*!< Validation of the library signature failed */
  ICC_INCOMPATIBLE_LIBRARY        =  5, /*!< Incompatible library - wrong architecture 64 bit library, 32 bit code etc */
  ICC_INVALID_STATE               =  6, /*!< ICC found it was in an invalid state internally */
  ICC_VALUE_NOT_SET               =  7, /*!< A requested value was not set (write)/has never been set (read)*/
  ICC_VALUE_TRUNCATED             =  8, /*!< The value (string) passed in was too long and has been truncated */  
  ICC_INVALID_PARAMETER           =  9, /*!< The parameter was invalid */
  ICC_NOT_INITIALIZED             = 10, /*!< ICC has not been initialized */
  ICC_DISABLED                    = 11, /*!< A fatal error occurred (Self tests, RNG initialization) and ICC is disabled */
  ICC_MEMORY_FUNCTIONS            = 12, /*!< Problem in ICC's memory allocation */
  ICC_MUTEX_ERROR                 = 13, /*!< We expected a mutex to be set and it wasn't */
  ICC_UNABLE_TO_SET               = 14, /*!< ICC cannot set a value */
  ICC_NOT_ENOUGH_MEMORY           = 15, /*!< malloc() failed */
  ICC_ALREADY_ATTACHED            = 16  /*!< ICC_Attach called, but this call 
					  has already been made */
} ICC_MINOR_RC_ENUM ;

/*! @brief 
  FIPS mode Flags
  These are readable in ICC_STATUS, and the result may be 
  - 0
  - ICC_FIPS_FLAG
  - ICC_ERROR_FLAG 
  - (ICC_FIPS_FLAG | ICC_ERROR_FLAG)
*/
typedef enum {
  ICC_FIPS_FLAG    = 1,       /*!< FIPS mode is set */
  ICC_ERROR_FLAG   = 2        /*!< An error has been detected which invalidates FIPS mode */
} ICC_FLAGS_ENUM;

/*! @brief
   Maximun sizes of ICC internal Buffers,
   Limits on data lengths passed in.
   - Maximum path length that can be accepted. 
   - Limit on path parameter passed to ICC_Init 
   Note. Windows only: 
   ICC_InitW will accept a Unicode path up to 255 Unicode characters, but will 
   only ever return up to 255 BYTE buffer via ICC_GetValue
*/
#define ICC_MAXPATHLENGTH 256

/*! @brief
   Maximun sizes of ICC internal Buffers,
   Maximum length of an ICC error description returned
   in ICC_STATUS
*/
#define ICC_DESCLENGTH ICC_MAXPATHLENGTH

/*! @brief
   Maximun sizes of ICC internal Buffers,
   Limits on data lengths passed in.
   - Maximum string size that can be passed to SetValue 
*/
#define ICC_VALUESIZE ICC_DESCLENGTH

/*! @brief
   Self-test hashes/signatures
   Of very little interest to ICC API users
*/
typedef enum {
  ICC_HASHSIZE 	  =512,          /*!< Size of the Signatures */
  ICC_HASHANDNULL =513           /*!< Size of the Signatures with terminator */
} ICC_SELF_TEST_ENUM;

/*! @brief
  Value IDs used by SetValue/GetValue 
  - Note that the following values are unusable in production code.
  The memory callbacks can only set once per process, first to call wins, 
  and in integrated products the ICC users simply cannot guarantee that 
  they can win the races to set these as there may be multiple independently
  written users of ICC within one process space.
  - FIPS: The memory callbacks are permitted in FIPS mode provided there's only 
    a single user of ICC in the process space.
  - Reason: 
    - The process memory allocators can be hooked in other ways
    - The memory is in the same process, there are easier ways to obtain keys than by
      hooking the memory callbacks.
    - i.e. there's no reduction in security allowing these callbacks in FIPS mode.
  - <b>ICC_MEMORY_ALLOC</b>
  - <b>ICC_MEMORY_REALLOC</b>
  - <b>ICC_MEMORY_FREE</b>
  - The following values may be set, but since ICC 1.2.1, ICC will return ICC_WARNING indicating
  that they were already set. (Internally by ICC on startup to OS thread model defaults). Same problem
  as with the memory callbacks, but to ensure proper operation in threaded code
  they had to be set, and no individual application could guarantee they'd be the
  one to set them with multiple ICC users in the same process.
  - <b>ICC_LOCKING_FUNCTION</b>
  - <b>ICC_ID_FUNCTION</b>
  <p>
  - The following tags decribe the read/write capability and behaviour of the enums.
    - R/W1 value can be read but retains state once ICC_Attach() is called
    - RO value is read only
    - R/S1 value can be read (and may written only under very restricted conditions) but retains state
    once ICC_Attach() is called
    - F Fatal. As for R/W but ICC will shutdown shortly.
*/
typedef enum {
  ICC_FIPS_APPROVED_MODE = 0,  /*!< FIPS approved mode "on" or "off"
				   - Valid values: "on", "off"  (<b>R/W1</b>)
				   - FIPS: The only mode allowed by FIPS is "on"
				*/
  ICC_INSTALL_PATH       = 1,  /*!< Path to ICC libraries set by ICC_Init (<b>RO</b>) 
				 - FIPS: Allowed in FIPS mode
				    - Reason: only reads data 
			       */
  ICC_VERSION            = 2,  /*!< ICC Version (<b>RO</b>) 
				 - FIPS: Allowed in FIPS mode
				   - Reason: only reads data				 
			       */
  ICC_MEMORY_ALLOC       = 3,  /*!< Deprecated/unusable in production code (<b>R/S1</b>) 
				 - FIPS: Allowed in FIPS mode with restrictions
				   - Reason - see above
				*/
  ICC_MEMORY_REALLOC     = 4,  /*!< Deprecated/unusable in production code (<b>R/S1</b>) 
				 - FIPS: Allowed in FIPS mode with restrictions
				   - Reason - see above				 
				*/
  ICC_MEMORY_FREE        = 5,  /*!< Deprecated/unusable in production code (<b>R/S1</b>) 
				 - FIPS: Allowed in FIPS mode with restrictions
				   - Reason - see above				 
				*/
  ICC_LOCKING_FUNCTION   = 6,  /*!< Deprecated/unusable, handled internally (<b>RO</b>) 
				 - FIPS: Allowed in FIPS mode
				   - Reason - no impact on operation
				*/
  ICC_ID_FUNCTION        = 7,  /*!< Deprecated/unusable, handled internally (<b>RO</b>) 
				 - FIPS: Allowed in FIPS mode
				   - Reason - no impact on operation
				*/
  ICC_ENTROPY_ESTIMATE   = 8,  /*!< 0-100 100 = 1 bit per bit entropy from the TRNG
				 <p>ICC guarantees at least 50 (0.5 bits entropy/bit)
				 Expected values: 0-100.  (<b>RO</b>)
				 - FIPS: Allowed in FIPS mode
				   - Reason - informational only
				 - Note: <50 in FIPS mode and ICC shuts down.
			       */
  ICC_RANDOM_GENERATOR   = 9,   /*!< Change the PRNG construction ICC/OpenSSL uses by default
				   - Note that this is only a failsafe in case a flaw
				    is found in the default PRNG.
				   - Valid values:  (<b>R/W1</b>)
				     - "HMAC-SHA256" (default)
				     - "HMAC-SHA384"
				     - "HMAC-SHA512"
				     - "AES-256-ECB"
				     - "SHA256"
				     - "SHA384"
				     - "SHA512"
				   - FIPS: allowed in FIPS mode
			             - Reason: All these modes meet FIPS requirements for a 256 bit PRNG
                              */
  ICC_SEED_GENERATOR   = 10,  /*!< Change the Entropy source ICC/OpenSSL uses by default
				 - Note that this should only be used when the
				   default entropy source is unusable, most likely
				   a virtualized system or new hardware.
				 - Valid values:  (<b>R/W1</b>)
				   - "TRNG_HW" (default)
				   - "TRNG_OS"
				   - "TRNG_FIPS"
			    */
  ICC_INDUCED_FAILURE = 11,     /*!< Set to an active value (>0)
				  before ICC_Init is called for the first time 
				  this will force errors in ICC.
				  Used to prove coverage of ICC error
				  paths. 
				  - Valid values: 1-999 (sparse) (<b>F</b>) 				 
				  - <b>DO NOT USE in production environments. </b>
				  - FIPS: Not allowed in FIPS mode
				    - Reason: This is test function which should result in ICC shutting down
				  - \sa INDUCED

				*/
  ICC_ALLOW_INDUCED = 12,       /*!< Set to 1 to allow ICC_INDUCED_FAILURE
				     to be set at times other than at startup.
				     Used ONLY for FIPS compliance testing
				     of error paths
				     - Valid values: 1 or 0  (<b>R/W1</b>)
				     - <b>DO NOT USE in production environments. </b>
				     - FIPS: Not allowed in FIPS mode
				       - Reason: This is test function which may result in ICC shutting down
				*/
  ICC_CLEAN_AT_EXIT = 13,       /*!< Set to 1 to get the ICC static stub to register an atexit() handler 
				     to scrub state at process exit. Not available on some 
				     OS's (AIX/HPUX) where shared libraries are
				     unmapped immediately on dlclose()
				     - Valid values: 1 or 0  (<b>R/W1</b>)
				*/
  ICC_CPU_CAPABILITY_MASK =14,  /*!< Mask down CPU capabilities
				     <p>Note: While the interface will remain, the capabilities
				     that can be dropped should be considered deprecated.
				     This interface only serves as a temporary way to disable
				     new features - while they are new and relatively untested.
				     Once the code is considered stable the ability to mask out
				     the new CPU feature may be removed.
				     - Valid values: Unset, "????????????????" (<b>R/W1</b>)
				     - This is passed as a 16 byte hexadecimal representation
				       of a 64 bit unsigned number
				     - Some platforms may have no CPU capability mask
				     - FIPS: Valid settings are unset (default) and "0000000000000000"
				       - Reason. We do not test the crypto. code during FIPS
				          with any other settings than the default or "0000000000000000"
				     - Calling ICC_GetValue before ICC_Attach has been called will not 
				       work - the crypto. code which provides this function
               has not been loaded. So you can't read the state, mask
               a single bit and write it back
				     - Impact is global and the value is write-once,
				       this must be set before ICC_Init() is called
				*/
  ICC_RNG_INSTANCES = 15,       /*!< Set the number of RNG's in the pool 
				        ICC > 8.0 uses pools of PRNGS/TRNG's
				        to reduce impacts of locking on the system RNG's
				        Default is 7
				     - Valid values 1-255. (<b>R/W1</b>)
				     - FIPS: Allowed in FIPS mode
				       - Reason. PRNGS/TRNGS are still approved design
				*/
  ICC_RNG_TUNER = 16,            /*!< Algorithm used to setup the entropy gathering
                This shouldn't be touched in normal 
                use. Allows for a safe transition to 'better' algorithms.
				      - Valid values 0,1,2 (<b>R/W1</b>)
				      - FIPS: Allowed in FIPS mode
				        - Reason. Impacts initialization of TRNG's, not function
				*/
  ICC_ALT_ALLOCATOR = 17,        /*!< Set to "on" to enable.
				     Affects GSkit-Crypto and all underlying ICC's.
				     AIX only, use an alternate allocation strategy 
                                     on AIX to work around problems  with the Watson 
				     allocator. Memory use will increase.
				     - Valid values "on" "ON" or unset. (<b>R/W1</b>)
				     - FIPS: Allowed in FIPS mode
				       - Reason. Changes underly allocator used. Out of scope.
				     - Note: Environment variable. "yes" or "1".
				*/
  ICC_SHIFT = 18,   /*!< Manual RNG tuning. ONLY use after direct
                         instruction from GSkit L3. This disables
                         the auto-tuning of the RNG. Used to work
                         around pathological cases when virtualized.
                      - valid values 1-15
                      - FIPS: Allowed but not recommended in FIPS mode
                        - Reason: Can degrade entropy levels if not used
                          with extreme care.
                                 */      				
  ICC_LOOPS = 19,   /*!< Manual RNG tuning. ONLY use after direct
                                     instruction from GSkit L3. This disables
                                     the auto-tuning of the RNG. Used to work
                                     around pathological cases when virtualized/new hardware etc.
                                     - valid values: Positive integer.
                                     - FIPS: Allowed but not recommended in FIPS mode
                                     - Reason: Can degrade entropy levels if not used
                                       with extreme care.
                                 */
  ICC_FIPS_CALLBACK = 20,       /*!< Set the FIPS callback in THIS context to flag
                                      whether FIPS approved algorithms are being used
                                    - Note that using this may impose a considerable 
                                      performance penalty
                                    - This can only be set once, done to avoid locking
                                      overhead. This is known thread safe but thread safety
                                      checkers will complain. 
                                      To clear the callback, close the context and
                                      create a new one.*/
									  
  ICC_TRACE_CALLBACK = 21,       /*!< Set the Trace callback in THIS context */

  GSK_ICC_ACTIVE_LIBS = 52     /*!< Integer bit mask, the low two bits are used.
                                     Bit 0 = 1 the FIPS library is loadable
                                     Bit 1 = 1 the non-FIPS library is loadable
                                     Default value is both bits on (3). 
                                     - FIPS: Out of scope for FIPS.
                                     - Note: turning both bits off disables crypto. entirely.
                                     - The reason for having this here is that we have
                                       been run on new hardware where the crypto. code hangs. 
				                               As self test happens on library load we need some 
				                               way to specify that one of the libraries is not usable 
				                               at levels above the FIPS code so we can avoid loading
                                       the offending library at all.
                                     - Note: ICC_GetValue() does not work for this value.
				                        */         

} ICC_VALUE_IDS_ENUM;


/* Prototype for the FIPS callback function (ICC_FIPS_CALLBACK)
  func is provided for disambiguation in threaded code but using one ICC context/thread is recommended
  Note that func is generated by the C compiler __FUNC__ macro and will not exactly match the called API entry point
  nid is the nid (OpenSSL internal algorithm id) for the function or the related nid for more complex functions (i.e. HMAC, KDF)
  status is 1 is the operation was using a FIPS allowed algorithm correctly 
            0 otherwise - Allowed algorithm and allowed key size are all that's checked
  Generally we aim to call this once and once only in normal algorithm lifecycles to minimize the considerable performance impacts           
*/


/*! @brief NID's for EC function. These generally define types
    rather than individual curves.
    @note These values shouldn't change, but we can't guarantee that 
    so always use these via the enum's not via hardwired constants.
    @note The NID's for the EC curves can be obtained using 
    ICC_OBJ_txt2nid(ctx,"curvename"); 
    Known curve names can be found in OpenSSL documentation, 
    or in the ICC user guide.
*/
typedef enum {
  /* Enum for ansi X9_62 */
    ICC_NID_ansi_X9_62 = 405,                        /*!< ANSI X9.62 */
    /* Enum's for EC field types */
    ICC_NID_X9_62_prime_field = 406,                 /*!< prime field */
    ICC_NID_X9_62_characteristic_two_field = 407,    /*!< characteristic two field */
    ICC_NID_X9_62_id_characteristic_two_basis = 680, /*!< characteristic-two-basis */
    ICC_NID_X9_62_onBasis = 681,                     /*!< onBasis */
    ICC_NID_X9_62_tpBasis = 682,                     /*!< tpBasis */
    ICC_NID_X9_62_ppBasis = 683,                     /*!< ppBasis */
    /* EC Key identifier */
    ICC_NID_X9_62_id_ecPublicKey = 408 /*!< EC Public key */  	
} ICC_EC_NID_ENUM ;

  /*! @brief mode for setting AES_GCM acceleration level, no longer has an effect */ 
#define ICC_AES_GCM_CTRL_SET_ACCEL 0
  /*! @brief mode for getting AES_GCM acceleration level, always 1 */
#define ICC_AES_GCM_CTRL_GET_ACCEL 1
 /*! @brief Force check of TLSV1.3 compatible GCM IV rollover, set only, no parameters */ 
#define ICC_AES_GCM_CTRL_TLS13 2


/*!
    @brief The level of acceleration to use; a space-time trade-off
           Has no effect now as all platforms use asm
 */
typedef enum ICC_GCM_ACCEL {
  ICC_GCM_ACCEL_noaccel = 0,    /*!< No acceleration and no additional space used */
  ICC_GCM_ACCEL_level1,         /*!< Uses a 256 byte table to speed up GHASH computation */
  ICC_GCM_ACCEL_level2,         /*!< Uses a 4 Kbyte table to speed up GHASH computation */
  ICC_GCM_ACCEL_level3,         /*!< Uses an 8 Kbyte table to speed up GHASH computation */
  ICC_GCM_ACCEL_level4          /*!< Uses a 64 Kbyte table to speed up GHASH computation */
} ICC_GCM_ACCEL;

/*! 
  @brief State values returned by the PRNG API
*/
typedef enum {
  SP800_90UNINIT = 0, /*!< Not initialized */
  SP800_90INIT,       /*!< Initialized */
  SP800_90RUN,        /*!< Running */
  SP800_90SHUT,       /*!< Normal cleanup */
  SP800_90RESEED,     /*!< PRNG needs a reseed */
  SP800_90PARAM,      /*!< Error in data passed in, this will become CRIT next call */
  SP800_90ERROR,      /*!< Error, need to shutdown and reinitialize */
  SP800_90CRIT        /*!< Critical error, it's going to stay dead */
} SP800_90STATE;

/*! 
  @brief Commands for PRNG_CTX_ctrl 
*/
typedef enum {
  SP800_90_GET_PARANOID=0,   /*!< Get prediction resistance mode, continually reseed (slow) if !0 */
  SP800_90_GETMAXRESEED,/*!< Get maximum calls left before reseeding is needed */
  SP800_90_GETMAXAAD,   /*!< Get maximum AAD for this mode */
  SP800_90_GETMAXPER,   /*!< Get maximum personalization string */
  SP800_90_GETMINSEED,  /*!< Minimum seeding data required */
  SP800_90_GETMAXSEED,  /*!< Maximum seed length (entropy + anything else) */
  SP800_90_GETSTRENGTH, /*!< Effective security strength of the generator */
  SP800_90_SETRESEED,   /*!< Set reseed point (< PRNG maximum) in calls to the PRNG */
  SP800_90_GETRESEED,   /*!< Get reseed point in in calls to the PRNG Generate func  */
  SP800_90_DORESEED,    /*!< Force a reseed operation to occur before the next data is generated */
  SP800_90_SELFTEST,    /*!< Reset the PRNG and run the self test routines, needs reinit after */
  SP800_90_GETENTROPY,   /*!< Return the % (0-100) entropy estimate for the TRNG
			   used for seeding this RNG, 
			   or in the case of a TRNG, 
			   it's own entropy estimate */ 
  SP800_90_GETLASTERROR, /*!< Returns a pointer to non-internationalized text
			   with the reason for the last RNG error, or NULL
			   The pointed to data is static const, don't free it.
			 */
  SP800_90_GETTESTCOUNT,/*!< Returns the health check counter for the 
			  PRNG method used by this context. This is 
			  decremented with each new instance of a PRNG
			  and self test is re-run when it hits zero.
			  (Largely useful for formal verification of the API).
                         */
  SP800_90_GETMAXDATA,   /*!< Returns the maximum amount of data (bytes) that can be 
                             returned on each call.
                         */     
  SP800_90_GETMAXNONCE,   /*!< Returns the maximum nonce allowed in this mode */
  SP800_90_SETAUTO,       /*!< Set the autoreseed status - defaults to on (!0) */
  SP800_90_GETAUTO,       /*! Get the autoreseed status 0 == off !0 == on */
  SP800_90_SET_PARANOID   /*!< Set prediction resistance mode, continually reseed. (slow) */
} SP800_90CTRL;

/*
  Flags passed to the SP800-38F Key wrap/unwrap function
*/
#define ICC_KW_WRAP 1            /*!< If set key wrap, unset unwrap */ 
#define ICC_KW_FORWARD_DECRYPT 2 /*!< If set wrap uses decrypt, if uset wrap uses encrypt. (recommend unset) */
#define ICC_KW_PAD 4            /*!< If set we use the padded variant, if unset padded (and input data must be correctly blocked) */

typedef enum {
  SP800_38F_PARAM = 0,  /*!< Parameter error, invalid key length, invalid flags */
  SP800_38F_OK     = 1, /*!< No error */
  SP800_38F_MAC    = 2, /*!< Unwrap failed (MAC mismatch) */
  SP800_38F_DATA   = 3, /*!< Data range error, not blocked in unpadded mode, too long etc */
  SP800_38F_MEM    = 4  /*!< Memory/object allocation failure */
} SP800_38F_ERR;

/*! @brief
  ICC_STATUS is the ONLY ICC data type that the ICC 
  user can/is expected to allocate.
  @see ICC_MAJOR_RC_ENUM ICC_MINOR_RC_ENUM  
*/
struct ICC_STATUS_t
{
  int majRC;                  /*!< Major return code @see ICC_MAJOR_RC_ENUM*/
  int minRC;                  /*!< Minor return code @see ICC_MINOR_RC_ENUM*/
  char desc[ICC_DESCLENGTH];  /*!< Text description of the problem, not i18n */
  int mode;                   /*!< Mode flags - @see ICC_FLAGS_ENUM */
}; 

typedef struct ICC_STATUS_t ICC_STATUS;

/* ossl_param_st is sraped from ossl v3*/
struct ICC_OSSL_PARAM_t {
	const char* key;             /* the name of the parameter */
	unsigned int data_type;      /* declare what kind of content is in data */
	void* data;                  /* value being passed in or out */
	size_t data_size;            /* data size */
	size_t return_size;          /* returned size */
};
typedef struct ICC_OSSL_PARAM_t ICC_OSSL_PARAM;

#ifdef __cplusplus
}
#endif

#endif /*INCLUDED_ICCGLOBALS*/
