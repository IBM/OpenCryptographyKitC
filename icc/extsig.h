/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*
// Description: Header for shared functions
*/

#if defined(_WIN32)
#define EOL "\r\n"
#else
#define EOL "\n"
#endif

int CheckSig(FILE *fin,FILE *targ,EVP_PKEY *rsaPKey,int SigFileOnly);
int ReadConfigItems(FILE *fin, char *tweaks[], int n);
