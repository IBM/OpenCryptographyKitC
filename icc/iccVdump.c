/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

#include <stdio.h>
#include "iccversion.h"

/* This file exists so we can pass information about the ICC version
  into the ICC code generator. 
  Parsing nexted C headers from Java didn't look like much fun at
  all and doing it this way, all Java has to do is open a file
  read it and use the information within. The C-compiler having done
  all the heavy lifting.
  usage ./iccVdump >ICC_ver.txt
*/  

int main(int argc, char *argv[])
{
  printf("%1d_%1d_%d_%d\n",ICC_VERSION_VER,ICC_VERSION_REL,ICC_VERSION_MOD,ICC_VERSION_FIX);
  return 0;
}
