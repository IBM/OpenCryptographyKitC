/*-----------------------------------------------------------------------------
// Copyright IBM Corp. 2023
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*/
/* Description: Common data shared between the static stub and shared lib
*/

#if !defined(DEFINE_FUNC)
#define DEFINE_FUNC

typedef int (*PFI)();

typedef struct {
  char *name;
  PFI func;
} FUNC;

#endif
