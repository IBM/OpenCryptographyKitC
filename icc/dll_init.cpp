/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

#include <stdio.h>

extern "C" {
    int ICCLoad(void);
    int ICCUnload(void);
/* This is VERY dubious being the C++ destructor
   we may have to link as C++ instead
*/   
void __dl__FPv(void *targ)
{

}

};

class DLL_Load_Init {
  public:
    DLL_Load_Init() {
     /* printf("*******************Loading DLL!\n"); */
      ICCLoad();
    }
    ~DLL_Load_Init() {
      /* printf("Unloading DLL!******************\n"); */
      ICCUnload();
    }
};

static DLL_Load_Init dllLoadInitVar;
