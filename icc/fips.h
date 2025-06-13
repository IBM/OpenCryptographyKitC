/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description:
//        This is the header file that declares the functions
//        which will implement operations to conform
//        to the FIPS 140-2 startup and self test for a cryptographic
//        module.
//
*************************************************************************/

#ifndef INCLUDED_FIPS
#define INCLUDED_FIPS

#include "icclib.h"
#include "status.h"




#define ICC_INDUCE "ICC_INDUCED_FAILURE"
extern unsigned int icc_failure;



/*! @brief FIPS errors */
typedef enum
{       FIPS_STATUS_OK,                        /*!< No error */
	FIPS_CONTINUOUS_RNG_ERROR,             /*!< RNG consistancy test failed */
        FIPS_KNOWN_ANSWER_ERROR,               /*!< Known answer test failed */
        FIPS_CONTINUOUS_KEY_CONSISTENCY_ERROR, /*!< And RSA or DSA test failed it's consistancy check */
        FIPS_TRNG_ENTROPY                      /*!< The TRNG entropy fell below acceptable limits */   
} ICC_FIPS_MODULE_STATUS_ENUM;


int iccFipsError(int status);






int iccSetRNG(ICClib *iccLib, ICC_STATUS *icc_stat, void * seedB, int seedL);


void iccCleanupRNG();


void iccDoKnownAnswer(ICClib *iccLib, ICC_STATUS *icc_stat);
  



#if defined(UNICODE)
int iccSignatureTest(ICClib *iccLib, ICC_STATUS* icc_stat, char* signS, wchar_t * path);
#else
int iccSignatureTest(ICClib *iccLib, ICC_STATUS* icc_stat, char* signS, char* path);
#endif

/* Called after key creation if a FIPS mode context is being used */
int iccVerifyRSAKey(ICClib *iccLib, RSA* rsaKey);

int iccDSAPairTest(ICClib *icclib, DSA *dsa);

int iccECKEYPairTest(ICClib *icclib, EC_KEY *eckey);


#endif /*INCLUDED_FIPS*/
