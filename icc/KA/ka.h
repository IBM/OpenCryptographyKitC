/*
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*/
#if defined(STANDALONE)
/* KA tests only need ICC_STATUS and openssl headers, so this provides that for debug without dragging
  in piles of other stuff
*/
#include <string.h>

#include "openssl/evp.h"
#include "openssl/rand.h"
#include "openssl/rc2.h"
#include "openssl/hmac.h"
#include "openssl/des.h"
#include "openssl/dsa.h"
#include "openssl/sha.h"
#include "openssl/aes.h"
#include "openssl/ec.h"
#include "openssl/rsa.h"
#include "openssl/ecdh.h"
#include "openssl/ecdsa.h"
#include "openssl/pkcs12.h"
#include "openssl/err.h"
#include "openssl/cmac.h"

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
  ICC_STATUS is the ONLY ICC data type that the ICC 
  user can/is expected to allocate.
  @see ICC_MAJOR_RC_ENUM ICC_MINOR_RC_ENUM  
*/
typedef struct ICC_STATUS_t
{
  int majRC;                  /*!< Major return code @see ICC_MAJOR_RC_ENUM*/
  int minRC;                  /*!< Minor return code @see ICC_MINOR_RC_ENUM*/
  char desc[ICC_DESCLENGTH];  /*!< Text description of the problem, not i18n */
  int mode;                   /*!< Mode flags - @see ICC_FLAGS_ENUM */
} ICC_STATUS; 

extern int icc_failure;
extern unsigned char ibuf[1024];

#endif
