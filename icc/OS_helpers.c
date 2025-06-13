/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*
  Helpers for Java.
  Only relevant to zSeries currently but lets
  keep the future in mind here
*/
#include <stdio.h>
#include "../icc/icc_cdefs.h"

extern int OPENSSL_cpuid(unsigned long long *id);
/* Won't compile on z/OS */
#if defined(__s390__) || defined(__MVS__)
/* Constants from s390x_arch.h */
#define I_S390X_SHA3_224 0x00000001
#define I_S390X_SHA3_256 0x00000002
#define I_S390X_SHA3_384 0x00000004
#define I_S390X_SHA3_512 0x00000008
#define I_S390X_SHAKE_128 0x00000010
#define I_S390X_SHAKE_256 0x00000020
#define I_S390X_GHASH     0x00000040


/* reserved                     0x00000080 */

/* km/kmc/kmac/kmctr/kmo/kmf/kma */
#define I_S390X_AES_128 0x00000100
#define I_S390X_AES_192 0x00000200
#define I_S390X_AES_256 0x00000400

/* reserved                     0x00000800 */
/* prno */
#define I_S390X_TRNG    0x00001000
/* AES 128,192,256 GCM are ALL available via KMA */
#define I_S390X_KMA_GCM 0x00002000

/* XLC Metal has a richer asm syntax for Register Dependency Specification. For
 * IBM XLC Metal, let the compiler decide on the best register to use (there
 * isn't that many options for code this thin, but lets do the proper thing) For
 * GCC, the only option is to hardcode the registers, there is no gcc syntax to
 * specify the register interdependencies required by the crypto instructions.
 */

#if defined(__GNUC__)

#include "stdlib.h"

typedef size_t UDATA;
typedef signed char *arr;

/* gcc zcipher_gcc.c -c -S -o zcipher_gcc.s && gcc zcipher_gcc.c -c -o
 * zcipher_gcc.o
 */
/* gcc default compilation flags include "-mtune=z10 -march=z9-109 -mzarch"
  which excludes the rest of crypto instructions. Hence, hardcoding in hex
  (registers hardcoded!)*/

/*__asm__ with __volatile__ included to work around gcc bug, optimizing away asm
   at -O3: https://bugs.launchpad.net/ubuntu/+source/gcc-4.6/+bug/1029454 */

__asm(".macro kmc out in\n"
      ".byte 0xB9\n"
      ".byte 0x2F\n"
      ".byte 0x00\n"
      ".byte 0x46\n" /*out=R1=GPR4, in=R2=GPR6*/
      ".endm");

#define k_wrapper2R(I)                                                         \
  void s390_##I##_native(arr in, arr out, void *len, arr parmBlock,            \
                         void *mode) {                                         \
    register arr r_in asm("r6") = in;                                          \
    register arr r_out asm("r4") = out;                                        \
    register long r_len asm("r7") = (long)(*((UDATA *)len));                   \
    register long r_mode asm("r0") = (long)(*((UDATA *)mode));                 \
    register arr r_parmBlock asm("r1") = parmBlock;                            \
    __asm__ __volatile__("REDO" #I ": " #I " %r4,%r6\n"                        \
                         "BRC 3,REDO" #I "\n"                                  \
                         : "=d"(r_mode), "=d"(r_in), "=d"(r_len),              \
                           "=d"(r_parmBlock), "=d"(r_out)                      \
                         : "d"(r_mode), "d"(r_in), "d"(r_len),                 \
                           "d"(r_parmBlock), "d"(r_out));                      \
  }

__asm(".macro kmgcm out in aad\n"
      ".byte 0xB9\n"
      ".byte 0x29\n"
      ".byte 0x80\n" /*aad=R3=GPR8*/
      ".byte 0x46\n" /*out=R1=GPR4, in=R2=GPR6*/
      ".endm");

void s390_kmgcm_native(arr in, arr out, arr aad, void *len, void *aadLen,
                       arr parmBlock, void *mode) {
  register arr r_in asm("r6") = in;
  register arr r_out asm("r4") = out;
  register arr r_aad asm("r8") = aad;
  register long r_len asm("r7") = (long)(*((UDATA *)len));
  register long r_aadLen asm("r9") = (long)(*((UDATA *)aadLen));
  register long r_mode asm("r0") = (long)(*((UDATA *)mode));
  register arr r_parmBlock asm("r1") = parmBlock;
  __asm__ __volatile__("REDOkmgcm: kmgcm %r4,%r8,%r6\n"
                       "BRC 3,REDOkmgcm\n"
                       : "=d"(r_mode), "=d"(r_in), "=d"(r_len), "=d"(r_aadLen),
                         "=d"(r_parmBlock), "=d"(r_out), "=d"(r_aad)
                       : "d"(r_mode), "d"(r_in), "d"(r_len), "d"(r_aadLen),
                         "d"(r_parmBlock), "d"(r_out), "d"(r_aad));
}

__asm(".macro km out in\n"
      ".byte 0xB9\n"
      ".byte 0x2E\n"
      ".byte 0x00\n"
      ".byte 0x46\n" /*out=R1=GPR4, in=R2=GPR6*/
      ".endm");

void s390_km_native(arr in, arr out, void *len, arr parmBlock, void *mode) {
  register arr r_in asm("r6") = in;
  register arr r_out asm("r4") = out;
  register long r_len asm("r7") = (long)(*((UDATA *)len));
  register long r_mode asm("r0") = (long)(*((UDATA *)mode));
  register arr r_parmBlock asm("r1") = parmBlock;
  __asm__ __volatile__(
      "REDOkm: km %r4,%r6\n"
      "BRC 3,REDOkm\n"
      : "=d"(r_mode), "=d"(r_in), "=d"(r_len), "=d"(r_parmBlock), "=d"(r_out)
      : "d"(r_mode), "d"(r_in), "d"(r_len), "d"(r_parmBlock), "d"(r_out));
}

__asm(".macro kimd out in\n"
      ".byte 0xB9\n"
      ".byte 0x3E\n"
      ".byte 0x00\n"
      ".byte 0x46\n" /*out=R1=GPR4, in=R2=GPR6*/
      ".endm");

void s390_kimd_native(arr in, void *len, arr parmBlock, void *mode) {
  register arr r_in asm("r6") = in;
  register long r_len asm("r7") = (long)(*((UDATA *)len));
  register long r_mode asm("r0") = (long)(*((UDATA *)mode));
  register arr r_parmBlock asm("r1") = parmBlock;
  __asm__ __volatile__("REDOkimd: kimd 0,%r6\n"
                       "BRC 3,REDOkimd\n"
                       : "=d"(r_mode), "=d"(r_in), "=d"(r_len),
                         "=d"(r_parmBlock)
                       : "d"(r_mode), "d"(r_in), "d"(r_len), "d"(r_parmBlock));
}


k_wrapper2R(kmc)
#else 

/* zOS does this with external asm */
#define k_wrapper2R(I)

#endif




#endif

static int presence()
{
  return 42;
}
#if defined(__s390__) || defined(__MVS__) 

#define S390_GCM 1
#define S390_CBC 2
#define S390_ECB 3
#define S390_GHASH 4

#if    !defined(__MVS__)


static FUNC flist[6] = {
  {"presence",(PFI)presence},
  {"AES-GCM",(PFI)s390_kmgcm_native},
  {"AES-CBC",(PFI)s390_kmc_native},
  {"AES-ECB",(PFI)s390_km_native},
  {"GHASH",(PFI)s390_kimd_native},
  {NULL,(PFI)NULL}
};
#else /* __MVS__ */
extern void s390_km_native(char *in, char *out, long *len, char *parmBlock, long *mode);
extern void s390_kmgcm_native(const unsigned char *in, unsigned char *out, const unsigned char *aad, void *len, void *aadLen,
                             void *parmBlock, unsigned int mode);
extern void s390_kmc_native (signed char *in, signed char* out, void* len, 
                             signed char* parm,
                 void * mode);
extern void s390_kimd_native(char *in, long *len, char *parmBlock, long *mode);


static FUNC flist[6] = {
  {"presence",(PFI)presence},
  {"AES-GCM",(PFI)s390_kmgcm_native},
  {"AES-CBC",(PFI)s390_kmc_native},
  {"AES-ECB",(PFI)s390_km_native},
  {"GHASH",(PFI)s390_kimd_native},
  {NULL,(PFI)NULL}
};

#endif
#else
  static FUNC flist[2] = {
    {"presence",(PFI)presence},
    {NULL,(PFI)NULL}
  };
#endif

const FUNC *  OS_helpers() 
{
/* Now do the capabilities check and switch off anything 
   that won't be there
   */
#if defined(__s390__) || defined(__MVS__)
#define ALL_AES (I_S390X_AES_128 | I_S390X_AES_192 | I_S390X_AES_256 )

  unsigned long long cap = 0LL;
  /* Add probe code and if GCM capability is missing, set ptr to NULL
     Note that we insist that all of 128/192/256 are present as the functions
     provided can do any of these.
   */
  OPENSSL_cpuid(&cap);
  if(!((cap & I_S390X_KMA_GCM) && ((cap & ALL_AES) == ALL_AES))) {
    flist[S390_GCM].func = NULL;
  }
  if(!(cap & I_S390X_GHASH)) {
    flist[S390_GHASH].func = NULL;
  }
  if( ! ((cap & ALL_AES) == ALL_AES) ) {
    flist[S390_CBC].func = NULL;
    flist[S390_ECB].func = NULL;
  }


#endif

  return &flist[0];
}
