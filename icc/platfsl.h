/*****************************************************************************
// Copyright IBM Corp. 2023
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
//
// Description: Provides a layer of abstraction/indirection for platform
//              specific code.
//
******************************************************************************/

#ifndef INCLUDED_PLATFSL
#define INCLUDED_PLATFSL

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * ICC Platform Shared Library Function Documentation
 *
 * ICCLoad and ICCUnload are two functions that must be implemented by
 * the application using platfsl.x
 */

/*!
  @brief When a dynamic module is loaded this function
  will be called.
  @return OS dependent
*/  
int   ICCLoad(void);

/*!
  @brief When a dynamic module is unloaded this function
  will be called.
  @return OS dependent
*/  
int   ICCUnload(void);

#ifdef  __cplusplus
}
#endif


#endif  /* INCLUDED_PLATFSL */
