/*----------------------------------------------------------------------------
// Copyright IBM Corp. 2023
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
//
// Description:                                                                
//    Define flag for induced failure testing of ICC's error paths
//
//----------------------------------------------------------------------------------*/

#if !defined(INDUCED_H)
#define INDUCED_H
 /*! @brief Trigger for induced failure tests 
   i.e. trigger error conditions
   - export ICC_INDUCED_FAILURE=number to trip the relevant test
   - export ICC_ALLOW_INDUCED=yes to allow post POST triggering
   - This is formal test function only and should not be used by products unless
   they are also using it in testing.
  */
extern unsigned int icc_failure;

#endif
