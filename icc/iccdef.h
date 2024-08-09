/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

#if defined(_WIN32)
#include "BaseTsd.h"
#elif defined(__sun)
#include <inttypes.h>
#else
#include <stdint.h>
#endif


#if defined(_WIN32)
#define ICC_INT32   INT32
#define ICC_UINT32  UINT32
#else
#define ICC_INT32   int32_t
#define ICC_UINT32  uint32_t
#endif

/*  Can't trust long, which is 4 bytes on windows, 8 on linux
    stdint.h should be available everywhere. */

#if defined(_WIN32)
#define ICC_INT64   INT64
#define ICC_UINT64  UINT64
#else
#define ICC_INT64   int64_t
#define ICC_UINT64  uint64_t
#endif
