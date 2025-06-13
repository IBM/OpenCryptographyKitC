/*
// Copyright IBM Corp. 2023
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*/
/* Description: Common data shared between the static stub and shared lib
*/

/*! @brief This structure holds the ICC shared library information
  It holds the name/function pointer data for
  the ICC shared library, plus in icclib a link to 
  more detailed context and the name/pointer array for 
  the OpenSSL library 
  This ended up not needing to be so common after all.
*/

#if defined(ICC)
static struct ICCGlobal_t
{
  void *hICCLib;		            /*!< handle to the ICC shared library */
  FUNC **funcs;	                  /*!< An array of name/pointer pairs */
  int initialized;               /*!< We've been and done this */
  ICC_Mutex mtx;
  int mutexInit;
  int refCount;
  ICC_STATUS status;             /*!< Preserve errors from lib_init */
} ICCGlobal;
#endif
