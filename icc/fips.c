/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description:                                                                
//        The functions contained within implement operations to conform       
//        to the FIPS 140-3 startup and self test for a cryptographic          
//        module. 
//
*************************************************************************/

#include "fips.h"
#include "iccerr.h"
#include "induced.h"
#include "icclib.h"
#include "fips-prng/fips-prng-RAND.h"
#include "TRNG/nist_algs.h"
#include "tracer.h"

/** @brief this is the public key that complements the private key */
/* used to sign the modules within the cryptographic       */
/* boundary at build time                                  */
/* The private key at this time is stored in a file        */
/* named icc/privkey.rsa                                   */
/* \known Data: (rsa_pub_key) RSA public key used to verify
    the ICC shared library signatures
*/
#include "pubkey.h"


#if defined(_WIN32)
#include <fcntl.h>
#endif

#if defined(OS400)
#include "getnmi.h"
#endif


struct DSA_SIG_st {
  BIGNUM *r;
  BIGNUM *s;
};
#define EVP_MD_CTX_cleanup(a) EVP_MD_CTX_reset(a)

#define FATAL_ERROR (ICC_ERROR | ICC_FATAL)

/*
  Uncomment to generate known answer data 

#define KNOWN 1
*/
/* Uncomment to regenerate keys
#define KNOWN_KEYS 1
*/
#if defined(KNOWN)

static void GenerateKAData(ICClib *iccLib,ICC_STATUS *stat);
void iccPrintBytes(unsigned char bytes[], int len);

#endif 


/** @brief return text for a detected buffer overrun */
static const char*  ICC_MEMORY_OVERRUN = "Data corruption";
/** @brief return text for a known-answer failure - data length differs */
static const char*  ICC_KA_DIFF_LENGTH = "Known answer failed - length mismatch";
/** @brief return text for a known-answer failure - data match fails */
static const char*  ICC_KA_DIFF_VALUE  = "Known answer failed";
/** @brief return text for a known-answer failure - encryption had no effect */
static const char*  ICC_ENC_DATA_SAME  = "The encrypted data was the same as the clear text";
/** @brief return text for a known-answer failure - can't find the requested algorithm */
static const char*  ICC_NO_ALG_FOUND   = "The requested algorithm was not found";

extern unsigned int icc_failure; /**< Trigger for induced failure tests */
/**
   @brief
   Standard 'min' macro
*/
#define  iccMin(a,b)    (((a) < (b)) ? (a) : (b))

/**
   @brief
   Size of buffer used for internal self tests.
*/

#define SCRATCH_SIZE 4096 

extern struct ICClibGlobal_t Global;

/**
   \known Data: (in) data used as the input vector 
   for a number of tests.
*/

static const unsigned char in[]=
  {   0x37,0x36,0x35,0x34,0x33,0x32,0x31,0x20,
      0x4E,0x6F,0x77,0x20,0x69,0x73,0x20,0x74,
      0x68,0x65,0x20,0x74,0x69,0x6D,0x65,0x20,
      0x66,0x6F,0x72,0x20,0x00,0x31,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
      0x4E,0x6F,0x77,0x20,0x69,0x73,0x20,0x74
  };


/**
 *  \known Data: Data used for ChaCha-Poly1305 KA test
*/
static const unsigned char CHAPOLY_Key[] = 
  { 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
    0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
    0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
    0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f
  };

static const unsigned char CHAPOLY_IV[] = 
  { 0x07,0x00,0x00,0x00,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47
  };

static const unsigned char CHAPOLY_AAD[] = 
  { 0x50,0x51,0x52,0x53,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7
  };

 static const unsigned char CHAPOLY_TAG[] = 
  { 0x1a,0xe1,0x0b,0x59,0x4f,0x09,0xe2,0x6a,0x7e,0x90,0x2e,0xcb,
    0xd0,0x60,0x06,0x91
  };

static const unsigned char CHAPOLY_PT[] = 
  { 0x4c,0x61,0x64,0x69,0x65,0x73,0x20,0x61,
    0x6e,0x64,0x20,0x47,0x65,0x6e,0x74,0x6c,
    0x65,0x6d,0x65,0x6e,0x20,0x6f,0x66,0x20,
    0x74,0x68,0x65,0x20,0x63,0x6c,0x61,0x73,
    0x73,0x20,0x6f,0x66,0x20,0x27,0x39,0x39,
    0x3a,0x20,0x49,0x66,0x20,0x49,0x20,0x63,
    0x6f,0x75,0x6c,0x64,0x20,0x6f,0x66,0x66,
    0x65,0x72,0x20,0x79,0x6f,0x75,0x20,0x6f,
    0x6e,0x6c,0x79,0x20,0x6f,0x6e,0x65,0x20,
    0x74,0x69,0x70,0x20,0x66,0x6f,0x72,0x20,
    0x74,0x68,0x65,0x20,0x66,0x75,0x74,0x75,
    0x72,0x65,0x2c,0x20,0x73,0x75,0x6e,0x73,
    0x63,0x72,0x65,0x65,0x6e,0x20,0x77,0x6f,
    0x75,0x6c,0x64,0x20,0x62,0x65,0x20,0x69,
    0x74,0x2e
  };

static const unsigned char CHAPOLY_CT[] =
  { 0xd3,0x1a,0x8d,0x34,0x64,0x8e,0x60,0xdb,
    0x7b,0x86,0xaf,0xbc,0x53,0xef,0x7e,0xc2,
    0xa4,0xad,0xed,0x51,0x29,0x6e,0x08,0xfe,
    0xa9,0xe2,0xb5,0xa7,0x36,0xee,0x62,0xd6,
    0x3d,0xbe,0xa4,0x5e,0x8c,0xa9,0x67,0x12,
    0x82,0xfa,0xfb,0x69,0xda,0x92,0x72,0x8b,
    0x1a,0x71,0xde,0x0a,0x9e,0x06,0x0b,0x29,
    0x05,0xd6,0xa5,0xb6,0x7e,0xcd,0x3b,0x36,
    0x92,0xdd,0xbd,0x7f,0x2d,0x77,0x8b,0x8c,
    0x98,0x03,0xae,0xe3,0x28,0x09,0x1b,0x58,
    0xfa,0xb3,0x24,0xe4,0xfa,0xd6,0x75,0x94,
    0x55,0x85,0x80,0x8b,0x48,0x31,0xd7,0xbc,
    0x3f,0xf4,0xde,0xf0,0x8e,0x4b,0x7a,0x9d,
    0xe5,0x76,0xd2,0x65,0x86,0xce,0xc6,0x4b,
    0x61,0x16
  };




/** \known Data. TLS1.3 HKDF from rfc5869
*/
#define HKDF_L 82

static const unsigned char HKDF_IKM[] = 
  { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
    0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
    0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
    0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f
  };

static const unsigned char HKDF_salt[] =
  { 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
    0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
    0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,
    0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
    0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
    0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
    0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
    0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
    0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
    0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf
  };
static const unsigned char HKDF_data[] = 
  { 0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,
    0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
    0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
    0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
    0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,
    0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
    0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,
    0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
    0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
    0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
  };

static const unsigned char HKDF_PRK[] = 
  { 0x06,0xa6,0xb8,0x8c,0x58,0x53,0x36,0x1a,
    0x06,0x10,0x4c,0x9c,0xeb,0x35,0xb4,0x5c,
    0xef,0x76,0x00,0x14,0x90,0x46,0x71,0x01,
    0x4a,0x19,0x3f,0x40,0xc1,0x5f,0xc2,0x44
  };
static const unsigned char HKDF_OKM[] = 
  { 0xb1,0x1e,0x39,0x8d,0xc8,0x03,0x27,0xa1,
    0xc8,0xe7,0xf7,0x8c,0x59,0x6a,0x49,0x34,
    0x4f,0x01,0x2e,0xda,0x2d,0x4e,0xfa,0xd8,
    0xa0,0x50,0xcc,0x4c,0x19,0xaf,0xa9,0x7c,
    0x59,0x04,0x5a,0x99,0xca,0xc7,0x82,0x72,
    0x71,0xcb,0x41,0xc6,0x5e,0x59,0x0e,0x09,
    0xda,0x32,0x75,0x60,0x0c,0x2f,0x09,0xb8,
    0x36,0x77,0x93,0xa9,0xac,0xa3,0xdb,0x71,
    0xcc,0x30,0xc5,0x81,0x79,0xec,0x3e,0x87,
    0xc1,0x4c,0x01,0xd5,0xc1,0xf3,0x43,0x4f,
    0x1d,0x87
  };

/** \known Data: SP800-38F NIST sample vector for KW_AES_128.txt 
    [PLAINTEXT LENGTH = 128], COUNT = 0
    Unwrap is checked with the reverse path
*/
static const char KW_K[] = 
  {0x75,0x75,0xda,0x3a,0x93,0x60,0x7c,0xc2,
   0xbf,0xd8,0xce,0xc7,0xaa,0xdf,0xd9,0xa6
  };
static const char KW_P[] = 
  {0x42,0x13,0x6d,0x3c,0x38,0x4a,0x3e,0xea,
   0xc9,0x5a,0x06,0x6f,0xd2,0x8f,0xed,0x3f
  };

static const char KW_C[] = 
  {0x03,0x1f,0x6b,0xd7,0xe6,0x1e,0x64,0x3d,
   0xf6,0x85,0x94,0x81,0x6f,0x64,0xca,0xa3,
   0xf5,0x6f,0xab,0xea,0x25,0x48,0xf5,0xfb
  };

/** \known Data: SP800-38F NIST sample vector for KWP_AES_128.txt
    Unwrap is checked with the reverse path
    [PLAINTEXT LENGTH = 72] COUNT = 0

*/
static const char KWP_K[] = 
  {0x6a,0x24,0x52,0x60,0xe4,0xfb,0x9c,0xec,
   0xfd,0xa7,0x0e,0xfe,0x8f,0xa6,0x02,0x79
  };
static const char KWP_P[] = 
  {0x6a,0x27,0xdc,0xbe,0xfd,0xc1,0x40,0x45,
   0x16
  };

static const char KWP_C[] = 
  {0x36,0xf2,0x01,0x23,0xef,0xda,0x28,0x30,
   0x59,0x3e,0x09,0x6d,0x7d,0xd3,0xa3,0x28,
   0x77,0xbf,0xb6,0xf4,0x5b,0x8b,0x5a,0xda
  };


/** \known Data: DSA2 P. NIST SigVer sample vectors, [mod = L=2048, N=224, SHA-256], second set */
static const char *DSA2_P = "a29b8872ce8b8423b7d5d21d4b02f57e03e9e6b8a258dc16611ba098ab543415e415f156997a3ee236658fa093260de3ad422e05e046f9ec29161a375f0eb4effcef58285c5d39ed425d7a62ca12896c4a92cb1946f2952a48133f07da364d1bdf6b0f7139983e693c80059b0eacd1479ba9f2857754ede75f112b07ebbf35348bbf3e01e02f2d473de39453f99dd2367541caca3ba01166343d7b5b58a37bd1b7521db2f13b86707132fe09f4cd09dc1618fa3401ebf9cc7b19fa94aa472088133d6cb2d35c1179c8c8ff368758d507d9f9a17d46c110fe3144ce9b022b42e419eb4f5388613bfc3e26241a432e8706bc58ef76117278deab6cf692618291b7";

/** \known Data: DSA2 Q */
static const char *DSA2_Q = "a3bfd9ab7884794e383450d5891dc18b65157bdcfcdac51518902867";

/** \known Data: DSA2 G */
static const char *DSA2_G = "6819278869c7fd3d2d7b77f77e8150d9ad433bea3ba85efc80415aa3545f78f72296f06cb19ceda06c94b0551cfe6e6f863e31d1de6eed7dab8b0c9df231e08434d1184f91d033696bb382f8455e9888f5d31d4784ec40120246f4bea61794bba5866f09746463bdf8e9e108cd9529c3d0f6df80316e2e70aaeb1b26cdb8ad97bc3d287e0b8d616c42e65b87db20deb7005bc416747a6470147a68a7820388ebf44d52e0628af9cf1b7166d03465f35acc31b6110c43dabc7c5d591e671eaf7c252c1c145336a1a4ddf13244d55e835680cab2533b82df2efe55ec18c1e6cd007bb089758bb17c2cbe14441bd093ae66e5976d53733f4fa3269701d31d23d467";

/** \known Data: DSA2 Message */
static const unsigned char DSA2_MSG[] = 
  {
    0xbb,0xf8,0x07,0x11,0xce,0xe5,0xa1,0x4f,
    0x23,0x86,0xe0,0x57,0xf3,0xe5,0x4c,0x58,
    0x77,0x6f,0x04,0x76,0xc7,0x7e,0xd6,0x6e,
    0x63,0x06,0x8e,0xfa,0xe0,0x78,0x41,0xf0,
    0xb8,0x38,0x61,0x7a,0x92,0x83,0xc4,0x98,
    0x5c,0xca,0xc8,0xc5,0xa3,0x42,0x41,0xfd,
    0x0a,0x57,0x12,0x13,0x5e,0xf1,0x41,0x32,
    0x9b,0x27,0x9c,0x08,0xa8,0xce,0x42,0x7f,
    0x5f,0x42,0x4a,0x95,0x38,0xbd,0xf0,0xe3,
    0xaa,0x11,0x01,0xa4,0x24,0x9e,0x72,0x27,
    0x3d,0xc2,0x67,0x45,0x22,0x73,0x67,0x10,
    0x04,0x0f,0xaf,0x44,0x83,0x52,0x56,0x25,
    0x3d,0x74,0x57,0xa8,0xc5,0x1b,0xb7,0xd5,
    0x46,0xef,0xc1,0xb1,0xf2,0x14,0xd2,0x15,
    0xa2,0xdf,0x7e,0x7e,0x2c,0xaa,0xe9,0x56,
    0x6f,0xa4,0x6f,0xe7,0x5a,0x9a,0x3c,0xc4
  };

/** \known Data: DSA2 Private key */
static const char *DSA2_X = "3c0417f9a8f8054c929e0436bff6976255f3940c692898d944439e5d";

/** \known Data: DSA2 Public key */
static const char *DSA2_Y = "18e86706d8e9b458b52b018e67b20248b66c98baa3b47afe3efdb45f0566d17f2c37bd8fb15755865bb46aa3c310fdf9cdc33f775e72967457518ca8205552dcb68b29bc57fc0d3bdeb5f52919aa5c31c5d26896c431120bfc4bbb925432e9a79933ece73ddbb5741386d947eaac8382ee3b3103b9a473a56b6b10400b44c1702c4f3c681709d1bf8ab6471a99f4073cc04518bc61d83ccf6ef076030e770587d8258df0b55a35c2d0ecc8a0451d20f2f423be9c0b66f74fa78d2e30cc016aa2b123861bb6a4704773c21296bac9e8944ff48dcea90a70c9fca70246e6168ae6abc457763aa617fcab791b8a3f7682f0e179220199a0c5a43003a352ac853539";

/** \known Data: DSA2 Signature (R) */

static const char *DSA2_R ="02fb5eba32c5cdeebd1b527103f955ddad1357ef4dd0b56bffdafe15";
/** \known Data: DSA2 Signature (S) */
static const char *DSA2_S ="650e22d2d44ae1b1dd18052e940ef5a0908de8cddee2c493f43160bb";



/** \known Data: DSA 2k key */
static const unsigned char DSA_key[] = {
0x30,0x82,0x03,0x56,0x02,0x01,0x00,0x02,0x82,0x01,0x01,0x00,0x86,0x93,0xE5,0xC5,
0xF5,0x25,0x4B,0xA7,0x8A,0xFE,0x39,0x64,0xA1,0x10,0x6E,0xC1,0x37,0x9F,0x84,0xAA,
0x5E,0x63,0xEA,0x0F,0x0F,0xF1,0x40,0x5D,0x18,0x4E,0xF1,0x67,0x07,0x95,0xD2,0x32,
0x09,0xD5,0x50,0x4C,0x00,0x7C,0x84,0xD6,0xDD,0x66,0xE2,0xF6,0xD1,0xEE,0xFD,0x38,
0x56,0xD3,0x26,0x07,0x07,0x0E,0x93,0x5E,0xF1,0xB6,0x4D,0xE1,0x01,0x38,0x13,0x43,
0x72,0x5A,0x52,0x98,0xC6,0x1B,0x4B,0xEB,0x07,0x01,0x1B,0x7A,0xDD,0xCD,0xDC,0x03,
0x31,0x9B,0x9B,0x31,0x6E,0x03,0xB1,0x3A,0x27,0x9D,0x24,0xD5,0x46,0x3C,0xEE,0x81,
0xA9,0x6C,0x7F,0xEA,0x74,0xED,0xCF,0xA6,0x7D,0xEF,0xF1,0x56,0xE4,0x2A,0xE8,0x18,
0x75,0xF8,0xE6,0x02,0xED,0xA7,0xB4,0x7C,0xA7,0xD7,0xE4,0xB9,0x6D,0xEA,0xA3,0x19,
0x9D,0xC6,0x21,0x10,0x8D,0x0F,0x69,0x54,0x0A,0xDF,0xC9,0x1B,0xB0,0x2E,0x42,0x9F,
0x92,0x50,0xB6,0x71,0xC0,0xC1,0xE7,0x27,0x39,0xD0,0xD3,0x19,0xFF,0x27,0xD9,0x4C,
0xE9,0x28,0x41,0xC2,0x92,0x22,0x59,0x76,0x00,0xB2,0xE0,0x27,0x01,0x18,0xB3,0x20,
0x01,0xFB,0x13,0xA6,0xCC,0x5E,0x67,0x51,0x53,0x10,0xFC,0xFE,0xAB,0xBF,0x16,0xCA,
0x3D,0x9F,0x82,0x7C,0xE4,0x79,0x6E,0x67,0xBA,0xF5,0xD9,0x0C,0x6F,0xED,0xF9,0x13,
0x90,0xDD,0xB0,0x21,0x5B,0xED,0x44,0x59,0xAC,0x8E,0xBC,0x87,0xDD,0x11,0xA8,0x35,
0x08,0x2A,0x8F,0x13,0x48,0x74,0x00,0x7D,0xD9,0x41,0x2F,0x8D,0xBB,0x51,0xB7,0x7C,
0xA2,0xCA,0xFD,0x72,0xEE,0x2A,0x96,0x2A,0x2A,0x7C,0x5F,0x35,0x02,0x21,0x00,0xB6,
0x43,0xAA,0xEB,0x89,0x0A,0xA6,0x9A,0x88,0xCA,0x8F,0x3B,0xC0,0x80,0x90,0x88,0x2D,
0x40,0xA2,0xB9,0x2C,0x31,0x8C,0xB1,0x44,0x57,0x05,0x02,0xE4,0x9C,0x0A,0xEF,0x02,
0x82,0x01,0x00,0x65,0x78,0x31,0x48,0xAA,0xAB,0x95,0xAD,0x0F,0xE5,0x42,0x46,0xCA,
0xF3,0x3A,0x7E,0x3C,0x5F,0xC6,0x7D,0xC6,0x57,0xDB,0xB4,0x11,0xF5,0x5B,0xCE,0x52,
0x5E,0x2A,0xE3,0x5C,0xC3,0xD9,0x6E,0x7F,0xF3,0xE7,0xA9,0x5F,0x96,0x6B,0x3A,0x2E,
0x3E,0x28,0xAA,0x2B,0x6B,0xED,0xC1,0x3D,0x9B,0xBE,0x04,0xD4,0x25,0x0D,0x7E,0x29,
0x08,0xEC,0x26,0xD5,0x9B,0x59,0x4A,0xE8,0x76,0x06,0x83,0x56,0x9C,0xF7,0x85,0xD8,
0x62,0x0A,0x4C,0xB8,0x6F,0x16,0x92,0xA8,0x47,0x9B,0x15,0x1B,0x4F,0x17,0x89,0xB9,
0xF8,0x96,0xD9,0x3E,0xF8,0x61,0x3A,0x2A,0xCB,0x6B,0x26,0xB4,0x51,0x29,0x08,0x0E,
0x8A,0x28,0x33,0x1D,0xF2,0x04,0xFF,0x90,0x7E,0x63,0x61,0x5F,0xC3,0xB7,0x68,0x6A,
0x70,0x8F,0x34,0x6C,0x01,0x46,0xC9,0x88,0x73,0x60,0x54,0xB7,0x41,0xEA,0x8F,0xDB,
0xE8,0xF8,0xCE,0xDF,0x1E,0xBD,0xF9,0x73,0x0E,0xA4,0x37,0x44,0xBE,0xB4,0xA7,0xFD,
0x32,0x1C,0x5E,0xB9,0x55,0x29,0x4A,0x0C,0xC9,0x85,0xAB,0xDF,0x49,0x82,0x04,0xD8,
0x7D,0x18,0x43,0x2C,0x4E,0xCC,0xC7,0xBD,0x3B,0x1F,0x4D,0x02,0x14,0x11,0xEC,0x3F,
0xAE,0x7C,0x4A,0xF1,0x7E,0x40,0xF7,0x1C,0x35,0x92,0xDA,0x7F,0x72,0x4D,0x68,0x9F,
0x95,0x00,0x21,0x67,0xB2,0x00,0x98,0x51,0x6D,0x0A,0x49,0x3C,0xAA,0x76,0x5B,0xB2,
0xBC,0xBB,0x56,0x7E,0xE6,0x99,0x1B,0x6E,0xC4,0xF2,0x76,0xCB,0x48,0xBF,0xFC,0xF5,
0xBF,0x38,0xD4,0xE1,0x2A,0x13,0x1A,0x8D,0xC7,0x65,0x38,0x44,0x0C,0xA2,0x0F,0x58,
0x95,0xE0,0x91,0x02,0x82,0x01,0x00,0x68,0x8C,0xA9,0x5E,0x69,0x71,0xCC,0x21,0xA3,
0x10,0x86,0xE8,0x8E,0x99,0xDB,0x53,0x2C,0x1D,0x03,0xAD,0xA9,0xC2,0x14,0x18,0xB8,
0xE2,0x65,0xF5,0xC0,0xE4,0x6C,0x2A,0x89,0x90,0x29,0x4E,0x4E,0x95,0x09,0x95,0x92,
0x65,0xD8,0xEB,0x8B,0xA2,0xE1,0xB4,0xF1,0x71,0x81,0x4E,0x96,0x0B,0x70,0x2F,0x48,
0x19,0x29,0x63,0xCF,0x06,0x2F,0xF6,0xB6,0x47,0x89,0x14,0xFC,0xE0,0xA1,0x56,0xAF,
0x51,0x65,0x1A,0xAC,0xA0,0x62,0x29,0x8A,0x4E,0x0A,0x9B,0x31,0x5D,0x77,0x30,0x60,
0x7D,0x05,0x77,0xF1,0xA1,0x9F,0xE9,0x59,0x51,0xF5,0xEF,0xFF,0x1A,0x46,0x01,0x58,
0x63,0x44,0x90,0xFB,0x89,0x96,0x18,0x84,0x5D,0x21,0x28,0x88,0x49,0xB0,0xA1,0x57,
0x18,0x85,0xFA,0xC6,0x74,0x41,0xAF,0xCF,0xD9,0x3F,0x7F,0xF3,0x66,0x08,0x66,0xC6,
0x1D,0x47,0xE7,0x5D,0x36,0x75,0x0A,0xD4,0x3A,0x0E,0x28,0x3A,0xC0,0x40,0x27,0x73,
0x1C,0x07,0x71,0x27,0xBF,0x3C,0x52,0x74,0x3F,0xC2,0x3C,0xC1,0xBB,0x72,0x7C,0x65,
0xAD,0xD5,0xF6,0x31,0xC6,0x6E,0xB0,0x3D,0x35,0x63,0xB3,0x34,0xF4,0x27,0xBD,0x19,
0x9A,0x6F,0x95,0x55,0x3A,0x7E,0x2C,0xE3,0xAA,0x8C,0xC1,0x24,0x63,0x68,0x79,0xB9,
0xFB,0xA3,0xBA,0x53,0x2C,0xC5,0x4C,0x53,0x72,0x43,0x76,0xC9,0xB0,0xBC,0x53,0xB9,
0x16,0x53,0x2A,0x2D,0xE2,0x37,0xDA,0xBF,0xBC,0x71,0xD0,0x82,0xCD,0xE1,0xF0,0x71,
0x6C,0x11,0x61,0x52,0xA5,0x15,0xC4,0xB8,0xEE,0xBE,0xDA,0x51,0x46,0xC6,0xDA,0x64,
0xA4,0xFA,0x99,0x06,0x08,0x5F,0x41,0x02,0x21,0x00,0x9B,0xCE,0xAD,0x57,0xFD,0x75,
0x79,0xC2,0xEB,0x18,0x9C,0xCF,0x08,0x19,0x02,0x89,0xA9,0xA1,0x52,0x09,0xFC,0xDD,
0x5A,0xAC,0x5B,0x1D,0xF0,0xD5,0x8F,0xD2,0xBD,0x4A
};
/** \known Data: DSA signature generated by DSA_key */
static const unsigned char DSA_sig[] = {
  0x30,0x45,0x02,0x20,0x7E,0x2E,0xA3,0xC9,0x04,0x76,0x02,0x15,0x57,0xA7,0xEA,0x2E,
  0x31,0x4B,0xDE,0x0D,0x1E,0x1A,0x51,0xEC,0xC2,0x34,0xD9,0xEE,0x75,0x8A,0x84,0xBB,
  0xE9,0xF2,0x51,0x11,0x02,0x21,0x00,0x92,0x5E,0xF8,0x41,0xE5,0xFD,0xE3,0xE8,0xFD,
  0x53,0x63,0xC7,0x75,0x61,0xDF,0x45,0x01,0x96,0x38,0xE2,0x65,0xEE,0x7A,0xBC,0x98,
  0x9F,0xB6,0xC4,0x72,0xBA,0x22,0x9F
};

/** \known Data: Broken DSA signature generated by DSA_key - broken RNG */
static const unsigned char DSA_sig_broken[] = {
  0x30,0x44,0x02,0x20,0x5E,0x50,0x6A,0xD7,0xAB,0x5B,0x9E,0x78,0x0C,0xB7,0x25,0x40,
  0xEB,0x04,0xB7,0x1D,0xF5,0x2C,0x1C,0x9D,0xDD,0x18,0xA4,0xBB,0x38,0x06,0xDF,0x08,
  0x0A,0x86,0x32,0xB9,0x02,0x20,0x1F,0x30,0xD4,0x4A,0xBC,0xF1,0x57,0x58,0xFF,0xB4,
  0x6D,0xC8,0x49,0x16,0x64,0x59,0x91,0x30,0xC1,0x24,0x45,0x9B,0x2E,0xD6,0xC9,0x89,
  0x33,0x2F,0x86,0x47,0x22,0xEF
};



/** \known Data: (aes_key) AES key */
static const unsigned char aes_key[]=
  {   0x01,0x31,0xD9,0x61,0x9D,0xC1,0x37,0x6E,
      0x07,0xA1,0x13,0x3E,0x4A,0x0B,0x26,0x86,
      0x4E,0x6F,0x77,0x20,0x69,0x73,0x20,0x74,
      0x38,0x49,0x67,0x4C,0x26,0x02,0x31,0x9E
  };

/** \known Data: (cbc_iv) Initial Value */
static const unsigned char cbc_iv  []=
  {
    0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10,
    0x6F,0xE3,0xC2,0xC5,0x94,0x64,0x56,0x82,
    0x14,0x59,0x15,0x51,0xC1,0x0D,0x45,0x7A,
    0x5E,0x4F,0xCF,0x02,0x41,0x00,0x99,0x44
  };

/** \known Data: ECDH 'others' public key (P-521)
 */
static const char * ECDH_pub_otherX =  "0000007bcb0ff3188f0fc3bd5305d8555d30eab06495f3156db20bbec8968ac58e0113297d7d748c5b498b343fcff3cd577e4a55fc3875bd06ba41a5eccfc1a076e513f2";

static const char * ECDH_pub_otherY =  "000001d5960c977b85d076bc9edb0c377941982b23e4fcb16ee794b7d713ad31a6467a18424b34ba7b50fde0ff208cdf3e77d92b45dff2263e20cec9c10efec023382309";


/** \known Data: ECDH my public key
 */
static const char * ECDH_pub_mineX = "000001f5b3f766604e04330427ac25b50bbd72a94acd0373f0a75d2da352ef968a6477f97173f12f82adf37cbb80080eb469a7f9c37905c8e3563a76ade900ab3572790f";
 
static const char * ECDH_pub_mineY = "000000711f8c8f62c49257d78c492a3eacdb873c05740426f70ed77f2ea5ada1937c481a59ca3c29e1433f3c73ece67eb057170091b23c6b1a1605963e52711854c3b563";


/** \known Data: ECDH my private key
 */
static const char * ECDH_priv_mine = "000000445264fb130b3f0f2c2a8b1f656643dc674075772ca389c0df78c0a77a9b0626d38a389ea8be7cee2e3aa135dd480e87afa03e9d0fefed26067bf6d9023fa0d9b1";

/** \known Data: Shared secret
 */
static const unsigned char ECDH_shared[] =
  {
    0x00,0x87,0x06,0x67,0x4a,0x8d,0xc5,0x91,
    0x96,0x2a,0x4d,0x24,0x7d,0x06,0x68,0x68,
    0x18,0x6e,0x15,0xd2,0x63,0xdd,0xf8,0x77,
    0x71,0xf9,0x86,0x64,0x5f,0x7c,0xc3,0x71,
    0x0f,0xcb,0xe5,0x6b,0xe5,0x06,0x4f,0x60,
    0xc9,0x6a,0xb3,0x89,0x14,0x53,0xfc,0x16,
    0x28,0x37,0x3d,0xbf,0x4f,0x75,0x49,0x2f,
    0x1b,0xbf,0x7c,0x06,0xad,0x30,0xc2,0x81,
    0xbb,0xc3
  };


/** \known Data:  2k RSA key
 */
static const unsigned char RSA_key[] =
    {
        0x30, 0x82, 0x04, 0xA4, 0x02, 0x01, 0x00, 0x02, 0x82, 0x01, 0x01, 0x00, 0xB7, 0x6D, 0xF0, 0x5B,
        0x3E, 0xFB, 0x58, 0xAF, 0x6B, 0x2D, 0x5A, 0xE4, 0xD8, 0x5C, 0xB4, 0x43, 0xC5, 0xAA, 0x0B, 0xD2,
        0x63, 0xB2, 0x9F, 0x6A, 0xE1, 0x5F, 0x74, 0xA7, 0x0D, 0x31, 0xAA, 0x7F, 0x97, 0x84, 0x7D, 0xD3,
        0xAE, 0x3E, 0x11, 0x7A, 0x26, 0x99, 0xF1, 0x58, 0xBE, 0x92, 0xB4, 0xEF, 0x41, 0x38, 0x65, 0xCA,
        0xD5, 0x50, 0x4B, 0x10, 0x0D, 0x18, 0x8C, 0xA4, 0xCA, 0x3F, 0x91, 0xA1, 0x72, 0x54, 0x31, 0xDA,
        0xDE, 0x38, 0x24, 0xAB, 0xAE, 0xE0, 0x49, 0x78, 0x2C, 0x0D, 0x2A, 0x21, 0xA5, 0x01, 0xAC, 0xB7,
        0x46, 0x83, 0xFC, 0xB2, 0x74, 0x3C, 0xC6, 0x62, 0x89, 0x7A, 0xF9, 0x98, 0x37, 0x9D, 0xDC, 0x74,
        0x60, 0xC5, 0xA7, 0x82, 0x0E, 0xC0, 0x39, 0x6E, 0xF4, 0x6E, 0xD3, 0x88, 0xE9, 0x46, 0x54, 0xA2,
        0x77, 0xB7, 0x10, 0xF5, 0x95, 0xDE, 0x7A, 0x6C, 0x7E, 0xBC, 0xE2, 0xAD, 0xAF, 0x7E, 0x30, 0xE0,
        0x96, 0x9A, 0x52, 0x4D, 0x4B, 0xAC, 0x81, 0x19, 0xB0, 0x89, 0xF3, 0x69, 0x80, 0xBB, 0x9C, 0x2C,
        0x28, 0x53, 0x0D, 0x9D, 0x7E, 0x7F, 0x89, 0xCF, 0x3D, 0x9F, 0xB4, 0x97, 0x67, 0x5E, 0x46, 0xAB,
        0x48, 0x95, 0xDF, 0x7C, 0x7D, 0x3B, 0x5F, 0xB5, 0x22, 0x54, 0x6A, 0x80, 0xFF, 0x77, 0x45, 0xB1,
        0xA2, 0x74, 0xA4, 0x7F, 0x41, 0x56, 0x3D, 0x53, 0x7B, 0x02, 0x45, 0x26, 0x37, 0xD6, 0xBB, 0x68,
        0x11, 0x71, 0xD4, 0x26, 0xB2, 0x77, 0x62, 0x18, 0x40, 0x84, 0x46, 0x23, 0x98, 0xB1, 0xFB, 0xB5,
        0xBD, 0xEC, 0x59, 0x35, 0xF7, 0x69, 0x1F, 0xA0, 0x82, 0x78, 0xB9, 0x8F, 0xBB, 0xB2, 0x71, 0xFD,
        0x30, 0x72, 0x80, 0x5C, 0xEF, 0xF2, 0x38, 0x52, 0xB9, 0x29, 0x62, 0xD0, 0x63, 0x23, 0xB9, 0x49,
        0xF1, 0x24, 0xA5, 0x66, 0xD5, 0x40, 0xA9, 0xDB, 0xFD, 0x3E, 0xA4, 0xFD, 0x02, 0x03, 0x01, 0x00,
        0x01, 0x02, 0x82, 0x01, 0x00, 0x15, 0xB8, 0x7A, 0x84, 0x4B, 0x8E, 0x76, 0xDF, 0x7A, 0xD9, 0x09,
        0x90, 0xDC, 0xC0, 0x09, 0x86, 0x96, 0xF2, 0xFE, 0x68, 0x60, 0xEE, 0xE7, 0x6A, 0xC0, 0x1A, 0x4E,
        0x15, 0x73, 0xFE, 0x04, 0x1B, 0x0C, 0xC5, 0x9F, 0x22, 0xC0, 0x58, 0xD0, 0x37, 0xFF, 0x37, 0x2E,
        0x79, 0x9C, 0x43, 0x82, 0x12, 0x6C, 0xCE, 0x31, 0x5B, 0x95, 0xEB, 0xE6, 0x9F, 0x95, 0x75, 0x69,
        0x3E, 0x20, 0x46, 0xEB, 0xC7, 0x4A, 0xE3, 0x06, 0x6E, 0x4C, 0xEF, 0x28, 0x04, 0x7E, 0x76, 0x47,
        0xAC, 0xE7, 0xC6, 0x7D, 0x4D, 0x33, 0x9D, 0x61, 0xA6, 0xE8, 0x3E, 0x3B, 0xDF, 0xA3, 0xDD, 0x08,
        0x7E, 0xE5, 0x99, 0xBE, 0xD1, 0x94, 0x10, 0x68, 0xE9, 0x0D, 0x06, 0x8A, 0xC1, 0xDF, 0x5E, 0x4A,
        0xCE, 0xC5, 0x3A, 0x14, 0x69, 0x7B, 0x7A, 0x1D, 0x0E, 0x8A, 0x0F, 0x8E, 0xFE, 0x0D, 0xCB, 0xA5,
        0x02, 0x90, 0xF5, 0x6C, 0x00, 0x02, 0x87, 0xCB, 0x47, 0x39, 0x72, 0x22, 0x73, 0x60, 0xB6, 0x0B,
        0xCE, 0xD5, 0x75, 0xEB, 0x5E, 0x93, 0xB6, 0xE5, 0x5C, 0x70, 0x01, 0x9F, 0xE6, 0x66, 0x5F, 0x54,
        0x6E, 0xE7, 0x1D, 0x6B, 0x14, 0xED, 0x6A, 0x5D, 0x86, 0xF3, 0x88, 0x6D, 0x5B, 0x95, 0xBB, 0x5B,
        0x7F, 0xCE, 0xBA, 0x22, 0xA5, 0xE3, 0x9A, 0x69, 0x93, 0xDC, 0xEC, 0x97, 0xAA, 0x66, 0xF7, 0x49,
        0x08, 0x3F, 0x6A, 0xED, 0xFF, 0x5B, 0x1B, 0x66, 0x4E, 0x7B, 0x9F, 0x08, 0x48, 0x57, 0xE5, 0xFF,
        0x6E, 0x31, 0x42, 0x9E, 0x46, 0x94, 0x4E, 0x57, 0x84, 0xAC, 0x0B, 0x2C, 0x38, 0xF9, 0x64, 0x8D,
        0x3E, 0x2A, 0x1B, 0x72, 0x50, 0x60, 0xF5, 0x97, 0x05, 0xFE, 0x99, 0x2F, 0xCD, 0xFA, 0x9D, 0xD3,
        0x18, 0xD8, 0x20, 0x13, 0x33, 0x47, 0xA6, 0x28, 0x3B, 0x1E, 0xF9, 0x6B, 0x16, 0xFD, 0x50, 0xD1,
        0xE2, 0x8A, 0xB3, 0x56, 0x9F, 0x02, 0x81, 0x81, 0x00, 0xF2, 0xB6, 0x01, 0x3B, 0x4D, 0x55, 0x17,
        0xDC, 0xD9, 0x86, 0xA6, 0xB0, 0x1E, 0xAC, 0xD2, 0x82, 0x0F, 0xFB, 0xAD, 0x69, 0xBB, 0x7D, 0xC5,
        0xFA, 0x83, 0x8D, 0x97, 0x1F, 0x68, 0x88, 0xCC, 0x73, 0x4E, 0xD7, 0xA8, 0xC4, 0xCD, 0x84, 0xF9,
        0x82, 0xB2, 0xFC, 0x48, 0x02, 0xB8, 0x07, 0xF5, 0x7E, 0xD3, 0x8B, 0x15, 0x37, 0x8D, 0xD9, 0xB5,
        0xC4, 0x18, 0x41, 0xE7, 0x3C, 0x79, 0xAF, 0x8C, 0x7F, 0xB2, 0x28, 0x5C, 0x54, 0xBD, 0x16, 0x5F,
        0xB0, 0x43, 0x61, 0xFB, 0x22, 0x4B, 0x25, 0xF0, 0x02, 0xD2, 0x76, 0x7E, 0xDE, 0x82, 0xEA, 0xE7,
        0x8E, 0x6E, 0x0D, 0x45, 0x0F, 0x00, 0x70, 0x0F, 0x9B, 0xFC, 0xCE, 0x3D, 0x42, 0xE5, 0xCA, 0x28,
        0xFF, 0x73, 0x6E, 0xD8, 0x85, 0x13, 0x95, 0x33, 0x0B, 0x86, 0xA8, 0xE0, 0xC7, 0xC7, 0xF8, 0x03,
        0x00, 0xC0, 0xC3, 0xD5, 0x51, 0x77, 0x4E, 0x6B, 0x73, 0x02, 0x81, 0x81, 0x00, 0xC1, 0x79, 0x01,
        0x7A, 0x9C, 0x02, 0x04, 0xFF, 0xBD, 0x10, 0xAC, 0xAD, 0xD8, 0x5D, 0xC9, 0x1F, 0xD0, 0x3B, 0xAE,
        0xD9, 0xFF, 0xC6, 0x30, 0xEC, 0x84, 0x5D, 0x1D, 0x50, 0xCB, 0x2A, 0x87, 0x25, 0x6A, 0x57, 0x83,
        0x2F, 0xBA, 0x46, 0x31, 0x80, 0x18, 0x8F, 0x93, 0x19, 0x14, 0xE1, 0x16, 0x2F, 0x56, 0xFA, 0x59,
        0xFF, 0xC7, 0x6E, 0xA9, 0x07, 0x4E, 0xD3, 0xD8, 0x7D, 0x16, 0x3B, 0x3C, 0xA8, 0x44, 0x5F, 0xB2,
        0xEA, 0xD2, 0xBB, 0x6B, 0x3F, 0x28, 0x43, 0xB8, 0xED, 0xB8, 0xC6, 0xCE, 0x8F, 0x21, 0xAD, 0x2F,
        0x62, 0xAE, 0xF2, 0x10, 0x1F, 0x31, 0xF5, 0x17, 0xD1, 0xB4, 0xE9, 0xDB, 0x28, 0xAA, 0x4A, 0xAE,
        0xB2, 0x49, 0xC2, 0xCC, 0xED, 0x69, 0xEA, 0x28, 0x0C, 0x7B, 0x13, 0xB8, 0xE7, 0x30, 0x95, 0x70,
        0x0B, 0x4C, 0xC4, 0x42, 0x75, 0x1B, 0xE6, 0x4D, 0x25, 0xF0, 0xF8, 0x71, 0xCF, 0x02, 0x81, 0x80,
        0x3D, 0xD2, 0x38, 0x2C, 0x17, 0xBD, 0x85, 0xEF, 0x7E, 0x04, 0xBB, 0x2E, 0x7F, 0x11, 0xBC, 0x28,
        0xDE, 0xD3, 0x57, 0x4F, 0x68, 0x2F, 0x58, 0x1F, 0x40, 0x24, 0xEF, 0x8A, 0x53, 0x81, 0x0C, 0xBA,
        0x8E, 0x29, 0x86, 0x56, 0x62, 0x96, 0xED, 0x4A, 0xEA, 0x36, 0x32, 0x4D, 0x66, 0xFC, 0xB7, 0xFE,
        0x4E, 0xF9, 0xCD, 0x34, 0xB2, 0x4F, 0xF2, 0xE9, 0x78, 0xD8, 0x48, 0x82, 0xF1, 0xE7, 0xD2, 0x1C,
        0xB0, 0x8F, 0x71, 0x3D, 0x30, 0x50, 0xA3, 0x9C, 0xEC, 0xFC, 0xE7, 0x0F, 0xCC, 0x1E, 0x64, 0xAD,
        0x03, 0x58, 0xA5, 0x66, 0x1D, 0xE8, 0xA1, 0x84, 0x78, 0xE9, 0xAE, 0x5E, 0x6C, 0xD9, 0x1E, 0x50,
        0xB8, 0x2A, 0xFC, 0x15, 0xAF, 0x1C, 0x38, 0x27, 0x21, 0x0A, 0xE9, 0xE1, 0xB3, 0xA4, 0x3C, 0x35,
        0x42, 0x5E, 0x7F, 0xAA, 0xC3, 0x77, 0x9B, 0xA4, 0x34, 0x75, 0x6D, 0x3F, 0x6B, 0xE9, 0x30, 0x9B,
        0x02, 0x81, 0x81, 0x00, 0x99, 0x9B, 0x7A, 0x4A, 0x1F, 0x8D, 0x16, 0xEC, 0xF8, 0xEE, 0x41, 0x3B,
        0x71, 0x7A, 0xDC, 0xD1, 0x6E, 0x61, 0xC4, 0x6C, 0x7E, 0xBF, 0x9B, 0x5E, 0x5D, 0xA5, 0x14, 0x3E,
        0x6E, 0x5F, 0xE0, 0x97, 0x1B, 0x3C, 0x4A, 0x02, 0xDD, 0xD1, 0x17, 0x42, 0x0D, 0xBE, 0x08, 0x5B,
        0x34, 0x91, 0x95, 0x2C, 0x96, 0xD1, 0x04, 0x1D, 0xA8, 0xF8, 0xBA, 0x28, 0xFC, 0x34, 0x04, 0x41,
        0x24, 0x22, 0x7A, 0x01, 0x5A, 0xEF, 0xE4, 0x3C, 0xBE, 0x7D, 0x61, 0x23, 0xEE, 0xD2, 0xFE, 0x03,
        0x77, 0xDE, 0x18, 0x67, 0xD9, 0xA4, 0x07, 0xE8, 0x40, 0xE0, 0x1D, 0x5E, 0xB7, 0x2A, 0x51, 0xF4,
        0x04, 0xC0, 0x5B, 0x69, 0x88, 0xF2, 0xEC, 0x8A, 0xCF, 0x37, 0x63, 0xBE, 0xE7, 0x85, 0xAA, 0xB0,
        0x66, 0x13, 0x7D, 0x8D, 0xC8, 0xAC, 0x0D, 0x1E, 0x5B, 0x9F, 0xC2, 0xE7, 0xF9, 0xF3, 0xBA, 0xB2,
        0xF2, 0x04, 0x3F, 0x4F, 0x02, 0x81, 0x81, 0x00, 0xBC, 0xE1, 0x66, 0x89, 0x25, 0xC0, 0x0F, 0xF3,
        0x4A, 0xEE, 0x76, 0x4B, 0x7B, 0xFC, 0xF9, 0x19, 0xAD, 0x35, 0x4C, 0xD1, 0xB6, 0x87, 0x05, 0xCB,
        0x15, 0x03, 0x69, 0x1D, 0x23, 0x33, 0x42, 0xF5, 0x3F, 0x5F, 0xA9, 0xA6, 0x80, 0xCA, 0xDC, 0x14,
        0xA4, 0x68, 0x4E, 0x19, 0x47, 0x1D, 0xCD, 0xD6, 0xE5, 0xB8, 0xBE, 0x12, 0xA6, 0x5E, 0xB7, 0xD2,
        0xBA, 0xC2, 0x30, 0xBA, 0x38, 0xCA, 0x7C, 0xFA, 0xCA, 0x3D, 0xE8, 0xC1, 0x49, 0x25, 0xE3, 0x4A,
        0x55, 0x41, 0xBB, 0x7B, 0x6A, 0x12, 0xAB, 0x07, 0xB2, 0xD2, 0x06, 0x0D, 0xD4, 0xC8, 0x4F, 0x3E,
        0x0D, 0x03, 0x69, 0xE5, 0x1A, 0xC6, 0x0E, 0x61, 0x19, 0x6E, 0x1F, 0xB4, 0x94, 0x1B, 0x9C, 0xD7,
        0x41, 0x47, 0xA4, 0x00, 0x5B, 0x7D, 0x55, 0xD8, 0x43, 0x9D, 0xFE, 0x3A, 0xCE, 0x28, 0x6C, 0xF2,
        0x59, 0x33, 0xA7, 0xE9, 0x72, 0x9D, 0x7E, 0xC1};

static const unsigned char RSA_PKCS_sig[] = {
    0xAB, 0xBC, 0x2A, 0x22, 0xA1, 0xD1, 0xFC, 0x5D, 0x66, 0xB4, 0x4B, 0x42, 0xC8, 0xE2, 0x63, 0xE6,
    0xE8, 0x3D, 0x33, 0xB9, 0x0A, 0xDF, 0xA3, 0x38, 0x8B, 0x7C, 0x64, 0x0E, 0x34, 0x41, 0x60, 0xCB,
    0x37, 0xBC, 0xB0, 0xB4, 0x0D, 0x15, 0x2D, 0x5B, 0x09, 0xEB, 0x7F, 0xD9, 0x6C, 0x70, 0x0B, 0xCE,
    0x62, 0x13, 0x3A, 0xA0, 0x7C, 0x36, 0x7C, 0x48, 0xC4, 0x64, 0x38, 0xA4, 0x98, 0x83, 0x1B, 0x3C,
    0xA0, 0x79, 0x11, 0xC4, 0x3A, 0xE1, 0x54, 0xD2, 0xD8, 0xF8, 0xF7, 0x95, 0x2D, 0x29, 0xA8, 0x98,
    0x1B, 0x56, 0x89, 0x2E, 0xAE, 0x41, 0x06, 0x2C, 0xFD, 0x6F, 0xA0, 0x05, 0xA5, 0xCE, 0xD5, 0xC3,
    0xCB, 0xC4, 0xA1, 0x4F, 0x85, 0xA8, 0xA9, 0xF3, 0x45, 0x1E, 0x28, 0xCA, 0x1D, 0xCA, 0xFF, 0x81,
    0xEE, 0x02, 0x2E, 0x82, 0xBD, 0x8F, 0x6E, 0x55, 0x23, 0x04, 0x01, 0x1E, 0xCA, 0x86, 0xC6, 0x55,
    0x06, 0xEC, 0x44, 0x91, 0x42, 0x35, 0x74, 0xBF, 0x6E, 0x95, 0x25, 0xEF, 0x53, 0xD5, 0x0C, 0x7A,
    0xC5, 0x92, 0x31, 0xB5, 0xC3, 0x70, 0xF8, 0x55, 0x91, 0x29, 0xA6, 0xBA, 0x83, 0x5B, 0x34, 0x33,
    0x9E, 0x26, 0x2E, 0x51, 0x15, 0x74, 0x95, 0x2B, 0x5E, 0xBF, 0xDA, 0x86, 0x10, 0xC1, 0xAA, 0x7B,
    0x8C, 0xBF, 0xFA, 0x63, 0x2D, 0xFA, 0x4D, 0x6C, 0x17, 0x0C, 0x13, 0xCF, 0x08, 0xB8, 0x81, 0x7C,
    0x7C, 0x5E, 0x96, 0xF1, 0x3D, 0x72, 0x82, 0xD8, 0xB4, 0x30, 0xCA, 0x58, 0x9A, 0x54, 0x48, 0x1E,
    0x2C, 0x2D, 0x15, 0x1A, 0x4F, 0xB3, 0x22, 0xB3, 0x89, 0xD1, 0xDE, 0x32, 0x97, 0x51, 0xAB, 0x28,
    0xF7, 0x6E, 0x37, 0xD1, 0xCE, 0x39, 0x53, 0xDA, 0x3D, 0x0E, 0x10, 0x56, 0x05, 0x02, 0x5B, 0xA3,
    0xFE, 0xA1, 0x0E, 0xF7, 0x15, 0x68, 0x28, 0x73, 0xBB, 0x20, 0xA0, 0xA2, 0x33, 0x30, 0x8F, 0x0C

};
static const unsigned char RSA_PSS_sig[] = {
    0x13, 0x48, 0x2D, 0x92, 0x5F, 0x3E, 0x48, 0x50, 0xDD, 0x76, 0x3F, 0x59, 0x46, 0x44, 0xC2, 0x26,
    0x07, 0xE6, 0x86, 0x24, 0x05, 0xAF, 0x35, 0x24, 0x02, 0x62, 0x54, 0xDC, 0xA7, 0xF3, 0x2C, 0x61,
    0x2D, 0x3F, 0xCC, 0xAC, 0xF1, 0x71, 0x31, 0xC7, 0x86, 0x98, 0xAA, 0xF6, 0x6F, 0x7F, 0x90, 0x75,
    0xE5, 0x13, 0xA4, 0xA6, 0x80, 0x97, 0x01, 0x9D, 0x33, 0x08, 0xD6, 0x1B, 0xA7, 0x93, 0x6E, 0x0F,
    0x0F, 0xAD, 0x9D, 0x2D, 0xA6, 0xA4, 0x78, 0x5E, 0xEA, 0x6D, 0xD2, 0xF8, 0x87, 0xB5, 0x6E, 0xB3,
    0x60, 0xC6, 0x60, 0xF0, 0xD8, 0xE6, 0xB0, 0x8F, 0x12, 0x25, 0x36, 0x87, 0xBC, 0xF4, 0x88, 0xFF,
    0xD3, 0x86, 0x05, 0x5F, 0x7B, 0x8B, 0x3A, 0xBF, 0xCA, 0xAD, 0xB9, 0xC6, 0x4E, 0x32, 0xBC, 0x0A,
    0x0B, 0xEA, 0xED, 0x28, 0x39, 0xCD, 0x34, 0xFB, 0xD6, 0xB2, 0xF4, 0x16, 0x1D, 0x71, 0x40, 0xE5,
    0x4E, 0xAD, 0xBF, 0x0D, 0x34, 0xBA, 0x1D, 0x5C, 0x2D, 0x9D, 0x60, 0x49, 0x2E, 0x8C, 0xFF, 0xA5,
    0x8C, 0xEC, 0x9F, 0x74, 0x7E, 0x6E, 0x71, 0x6C, 0x33, 0xCB, 0x9F, 0xA4, 0x19, 0x7D, 0x8E, 0xB9,
    0x74, 0x76, 0x8C, 0x6C, 0x32, 0x2B, 0xE0, 0xDD, 0x80, 0x1E, 0x8F, 0x3D, 0x9C, 0x87, 0x7D, 0xA5,
    0xD3, 0x97, 0xDF, 0x27, 0xAF, 0xEA, 0x2F, 0x25, 0x07, 0x9B, 0x60, 0x4C, 0x4F, 0x84, 0xE3, 0x7C,
    0x17, 0xB5, 0x7A, 0x96, 0xB7, 0x72, 0xE1, 0x86, 0x19, 0xCA, 0x97, 0x03, 0x5F, 0x84, 0xF4, 0xE8,
    0x7B, 0x30, 0x41, 0x2C, 0x9A, 0x91, 0x3F, 0x01, 0xCF, 0xB9, 0x6D, 0x07, 0x24, 0xA1, 0xEC, 0x52,
    0x22, 0xBE, 0x68, 0x8E, 0x69, 0x1F, 0x99, 0x4B, 0x6D, 0xB6, 0xDE, 0x27, 0xE9, 0x2F, 0x0E, 0x4D,
    0x4E, 0x54, 0x23, 0xA7, 0x18, 0xFF, 0x4D, 0xDB, 0x53, 0xD9, 0x4A, 0x53, 0x7A, 0x27, 0xDB, 0x11};
static const unsigned char RSA_PSS_sig_broken[] = {
    0x8C, 0xB3, 0x4D, 0x28, 0x0F, 0xB8, 0x10, 0xFE, 0x56, 0x08, 0xEF, 0x22, 0x3E, 0xC6, 0x39, 0xDD,
    0x93, 0x9B, 0x13, 0x26, 0xC2, 0x18, 0x3D, 0x36, 0xB4, 0x09, 0x5D, 0x4D, 0x23, 0x97, 0xA7, 0x3F,
    0x9C, 0x6D, 0xEB, 0x48, 0x44, 0xF9, 0x40, 0xA8, 0x84, 0xA1, 0xC0, 0x87, 0x4E, 0xEB, 0x2C, 0xAF,
    0x83, 0x50, 0xEB, 0x19, 0x8A, 0x14, 0xEB, 0xF1, 0x7F, 0xF2, 0x48, 0x15, 0x54, 0xEB, 0xFE, 0xCE,
    0x9B, 0x39, 0xDF, 0x29, 0x04, 0xEA, 0xD8, 0x74, 0xFF, 0x9B, 0x4A, 0x8B, 0x29, 0x93, 0x79, 0x7B,
    0xC0, 0x73, 0xD6, 0xF1, 0x37, 0x23, 0x84, 0xF1, 0x53, 0xE9, 0xC0, 0xCF, 0xA2, 0x5F, 0xF6, 0x23,
    0xD9, 0xFC, 0x23, 0x9E, 0xEA, 0xCE, 0x4B, 0x62, 0xD6, 0xA5, 0x57, 0x61, 0xFA, 0xE6, 0x27, 0xBF,
    0xE0, 0x27, 0xC4, 0x33, 0x88, 0x35, 0x05, 0x05, 0x06, 0x9A, 0x00, 0x6E, 0xB6, 0xC0, 0xDD, 0x01,
    0x14, 0x86, 0x78, 0xAF, 0x27, 0x24, 0x19, 0x7A, 0xB3, 0x86, 0xB7, 0x93, 0x27, 0xC3, 0xC0, 0x89,
    0x1E, 0xAF, 0xFF, 0xA3, 0xF8, 0xF0, 0x21, 0x97, 0xF7, 0xCD, 0x51, 0xD0, 0xF0, 0xE4, 0xBC, 0x9B,
    0x3C, 0x20, 0x15, 0xBB, 0x7A, 0x67, 0x66, 0x63, 0x3F, 0x98, 0x82, 0x27, 0x56, 0x1E, 0x83, 0x99,
    0x03, 0xA7, 0x20, 0x47, 0xF6, 0x3B, 0x6A, 0x04, 0x6A, 0x84, 0x36, 0x89, 0x88, 0xA6, 0x51, 0xC0,
    0xC0, 0x2D, 0xF0, 0xE0, 0xD4, 0xE5, 0xD1, 0xD8, 0x17, 0xFD, 0xEA, 0x2F, 0x25, 0xBA, 0xE8, 0xEF,
    0x3D, 0xA8, 0xAE, 0x38, 0xB8, 0x25, 0x59, 0x0D, 0xF8, 0x37, 0xD0, 0x7A, 0x88, 0x4B, 0x6B, 0x0E,
    0x51, 0x3F, 0xF5, 0x9F, 0x01, 0x8F, 0x70, 0xAE, 0x43, 0x36, 0xCF, 0x7A, 0x5B, 0xE0, 0x71, 0x9E,
    0x5B, 0xD0, 0xE9, 0xC4, 0x25, 0x5E, 0x63, 0xA1, 0xA0, 0xB8, 0x74, 0x17, 0xFD, 0x2D, 0x8E, 0xEC};

/** \known Data: (rsa_privK_ka) RSA encrypt output */
static const unsigned char rsa_privK_ka[] =
    {
        0x21, 0x72, 0xA3, 0x1F, 0x36, 0x5F, 0xC5, 0xE3, 0x51, 0xC5, 0x56, 0xEB, 0xD7, 0xBF, 0x9B, 0x3D,
        0x68, 0xC3, 0x25, 0xAB, 0x5D, 0xE8, 0x51, 0xD7, 0xFA, 0xF9, 0xFE, 0x2C, 0xFA, 0x37, 0xB0, 0xF2,
        0xF2, 0x6F, 0x9C, 0xA8, 0xFC, 0x07, 0xB6, 0xB5, 0x73, 0x66, 0x9B, 0xE6, 0x88, 0x4C, 0x7D, 0x27,
        0x39, 0x1A, 0x96, 0x47, 0x7E, 0x43, 0x49, 0x4D, 0x10, 0x0A, 0xEB, 0x8D, 0x98, 0x61, 0x08, 0x9A,
        0xCC, 0xB9, 0xD8, 0x46, 0x96, 0x4B, 0xBF, 0x5D, 0xFF, 0x58, 0x60, 0xF0, 0x8F, 0x25, 0xAA, 0xED,
        0x6B, 0x93, 0x3A, 0xE3, 0xD4, 0xA0, 0x0D, 0x60, 0x30, 0x6D, 0x0F, 0x8C, 0xA8, 0x77, 0xAC, 0x31,
        0x70, 0x06, 0x24, 0x48, 0x7F, 0x03, 0x40, 0xAB, 0x9E, 0x69, 0xAD, 0x40, 0x6D, 0x6A, 0x69, 0xA6,
        0x5B, 0x3F, 0xAB, 0x3B, 0xA7, 0xA9, 0xEE, 0x9F, 0x69, 0x09, 0xCB, 0x67, 0x8F, 0x74, 0xE7, 0xE5,
        0xDE, 0xA2, 0xBC, 0x64, 0xF3, 0xE1, 0x1C, 0x4A, 0x22, 0x27, 0xAC, 0x6A, 0xD5, 0xFE, 0x71, 0x0D,
        0x15, 0xE3, 0x43, 0x54, 0x2E, 0x78, 0x55, 0xF5, 0x8C, 0x7A, 0xB8, 0x56, 0x74, 0xF4, 0x1F, 0x81,
        0x4B, 0x80, 0x30, 0x88, 0x0B, 0xFF, 0xE5, 0x19, 0xD3, 0xA0, 0xEA, 0x4B, 0x2C, 0x3A, 0xF8, 0xEF,
        0x87, 0xFB, 0x3E, 0xD0, 0x8C, 0x7F, 0xAE, 0x9B, 0x05, 0x2D, 0x1E, 0x2F, 0xFE, 0x59, 0xC8, 0xD1,
        0xC6, 0xD0, 0x15, 0xFA, 0x4F, 0x44, 0x0B, 0x41, 0x4D, 0xBD, 0xBE, 0x4B, 0x43, 0x13, 0xAF, 0xE6,
        0x71, 0x8D, 0x13, 0x84, 0xF9, 0xCB, 0x85, 0xF4, 0xBE, 0x97, 0x04, 0x37, 0xD0, 0x9F, 0x02, 0x38,
        0x40, 0xDC, 0x87, 0x11, 0x42, 0x3B, 0xB5, 0x06, 0x52, 0xF2, 0xFD, 0xCD, 0x20, 0x59, 0x71, 0x8A,
        0x4B, 0x1F, 0x3D, 0x75, 0xB5, 0x98, 0xE3, 0xD5, 0x32, 0x40, 0x8A, 0x88, 0xCC, 0x65, 0xFB, 0xAB};
/** \known Data: (rsa_pubK_ka) RSA decrypt output */
static const unsigned char rsa_pubK_ka[]=
  {   0xAE,0xC9,0x81,0x7F,0xCF,0x9B,0x5B,0x52,
      0x37,0x54,0xD4,0x6A,0xA4,0x9F,0x83,0x16,
      0x46,0xDF,0x03,0xEB,0xB8,0xA3,0x8A,0xC4,
      0xB2,0x99,0x45,0x51,0x5A,0x50,0x55,0xCA,
      0x56,0xFD,0xFB,0x94,0x65,0x1E,0x87,0xC0,
      0xB1,0x51,0xD7,0x26,0xB8,0xE3,0x06,0x4D,
      0x6C,0x14,0x13,0x67,0x12,0x0B,0x66,0x36,
      0x35,0x00,0x74,0xEC,0x74,0x38,0x67,0xC0,
      0x76,0x65,0xEB,0xE6,0xC2,0x61,0x1E,0x67,
      0x49,0x56,0x16,0x25,0x0E,0xFB,0x2B,0xC2,
      0xDB,0xB3,0x0C,0x8E,0xBD,0x45,0x5F,0xE5,
      0x7E,0x42,0xA1,0x9A,0x55,0xC4,0xFD,0x6F,
      0x43,0xA0,0x29,0xBD,0x84,0xE1,0xB0,0xF6,
      0x49,0x1C,0x33,0x8D,0x61,0xD9,0x42,0x72,
      0xB2,0xC8,0x4C,0x12,0x8B,0xAD,0xDB,0x38,
      0x0B,0xB1,0x81,0x5C,0x58,0xC9,0xD4,0xD1
  };
  
/** \known Data: (aes_ka) AES-256 output */
static const unsigned char aes_ka[]=
  {   0x03,0x81,0x39,0x0C,0x8A,0xA4,0x68,0x79,
      0xFD,0x7D,0x3A,0x45,0xFD,0xF6,0x0C,0xA1,
      0xD5,0xDF,0x8D,0x56,0xAB,0x33,0x16,0xAF,
      0x2D,0xD1,0x0C,0x9D,0xC1,0x86,0x01,0x48,
      0x83,0x32,0x98,0x3E,0xD9,0x16,0xE4,0xAC,
      0x78,0xF8,0xC2,0x38,0x9F,0xCF,0x06,0x4E,
      0x3B,0x38,0x4F,0x45,0xDD,0xDF,0x7D,0x87,
      0xA6,0x6E,0xF0,0x7E,0xD6,0x91,0x11,0x1B
  };
/** \known Data: (sha1_ka) SHA1 output */
static const unsigned char sha1_ka[]=
  {    
    0x8C,0x0E,0x3A,0xDD,0x4C,0xEF,0x97,0x27,
    0xD4,0xD8,0xA4,0x84,0x8A,0xB5,0x8A,0xB7,
    0x7F,0x29,0xF9,0x33
  };

/** \known Data: (sha256_ka) SHA256 output */
static const unsigned char sha256_ka[]=
  {
    0x9D,0xBF,0xF5,0x99,0x91,0x9F,0xC4,0xCD,
    0x91,0x7A,0x37,0x65,0x28,0x57,0x53,0xAE,
    0x3B,0xFC,0x3E,0xC2,0x4E,0x3C,0xA4,0xBA,
    0x0B,0xFD,0xF8,0xE5,0xCC,0x7B,0xC7,0x80
  };


/** \known Data: (sha512_ka) SHA512 output */
static const unsigned char sha512_ka[]=
  {
    0xD5,0x8E,0xB8,0xCC,0xDC,0xA3,0x2C,0xA7,
    0x30,0xA1,0xA4,0xFE,0x2D,0xBD,0x68,0xB1,
    0x2D,0x8A,0x48,0xEE,0xF0,0x5C,0x09,0x92,
    0x7C,0x47,0xC3,0x83,0x7A,0x9B,0x8B,0x7D,
    0x9E,0x49,0xEC,0x0A,0x0C,0x29,0x28,0xE4,
    0x9F,0x33,0x14,0x8F,0x09,0xE4,0xE5,0xAA,
    0xD0,0x21,0x8A,0x69,0x7A,0xA8,0x8F,0x6C,
    0xCE,0x1F,0xE3,0xD3,0xA5,0x12,0xA4,0x30
  };

/** \known Data: (sha3_512_ka) SHA3_512 output */
static const unsigned char sha3_512_ka[]=
  {
    0x34,0xEB,0x54,0x8F,0xF4,0xC7,0x0E,0x29,
    0xD6,0xF2,0x8B,0xD0,0xDC,0x72,0x75,0xB3,
    0x3A,0xAF,0x5F,0xDC,0xC2,0x84,0x2D,0xA2,
    0xF7,0xEB,0x05,0xFD,0x49,0x1E,0x33,0x33,
    0x4C,0x1E,0x4D,0x89,0x16,0xBE,0xFE,0x2A,
    0x0D,0x72,0x0A,0x52,0x1A,0x3E,0xA3,0x51,
    0x8C,0xDD,0xE6,0x7E,0x9D,0xF6,0x14,0xC4,
    0x42,0xB6,0x80,0xB8,0x7D,0x4A,0xD3,0x22
  };




/** \known Data: (dsa_privK_ka) 2048 bit DSA private key */
static const unsigned char dsa_privK_ka[] =
  {
    0x30,0x82,0x03,0x56,0x02,0x01,0x00,0x02,0x82,0x01,0x01,0x00,0xE9,0xE5,0x1F,0xE9,
    0xBE,0x05,0x20,0x23,0xE1,0x70,0xA3,0xF6,0x40,0x88,0x8D,0xD4,0xBB,0x6F,0xDA,0xC0,
    0x01,0xBB,0xC5,0xAF,0xBB,0x92,0x21,0xF9,0x47,0xDC,0xA8,0x33,0xA9,0xB3,0x2A,0x4F,
    0xC6,0x3E,0x98,0x21,0xFB,0xBF,0x07,0x60,0x9F,0xD6,0xD2,0x87,0x8C,0x61,0x5D,0xC6,
    0x47,0x79,0xA8,0xBB,0x8C,0x1E,0x6A,0xC3,0xF8,0xD6,0xDA,0xDA,0x26,0x08,0x56,0x5D,
    0x66,0x6A,0x49,0x9E,0x68,0x91,0xCB,0xF5,0x25,0x20,0xC9,0x8A,0x1C,0xD8,0xF4,0xD8,
    0x6F,0xDC,0x06,0xB5,0xFC,0xB1,0x6F,0x48,0xFA,0xA0,0x58,0xD7,0xD1,0x43,0x42,0x7A,
    0xBB,0xB8,0x06,0x5B,0x92,0x1A,0x1A,0x39,0x9F,0xB7,0xDF,0x4F,0xD6,0xE0,0x57,0xF0,
    0x75,0x66,0xE7,0x59,0x30,0xFB,0xD9,0x0C,0x13,0xE7,0xE9,0x81,0x38,0x0D,0x46,0x58,
    0xFB,0x9A,0x5A,0xDA,0x4B,0x7F,0x2C,0x3D,0x1A,0x56,0x57,0x3E,0x21,0x79,0x55,0xD2,
    0xC9,0xB6,0xE5,0x3F,0x32,0xC4,0xE4,0xB1,0x1D,0x58,0x76,0xD4,0x04,0x46,0x93,0x7A,
    0x55,0x23,0x35,0x66,0x43,0x4D,0xE2,0x84,0x1A,0x0F,0x3A,0x97,0x22,0x42,0x6A,0xFF,
    0xED,0xE3,0xA8,0xD6,0xB4,0x40,0x86,0x10,0x0D,0x71,0xBF,0x3A,0xBA,0x51,0xB1,0xD7,
    0x36,0x31,0x41,0x08,0x09,0x2C,0x61,0xBC,0xC7,0x8C,0xB6,0x22,0x71,0x51,0xE7,0x5E,
    0x63,0x92,0x67,0x9A,0x16,0x93,0x18,0xE5,0x20,0xA5,0xB4,0x02,0xF3,0xEB,0x82,0xA3,
    0x7D,0xFD,0x20,0x02,0x9A,0x07,0x36,0xA0,0x79,0x04,0xF9,0xD1,0x95,0x2E,0x84,0xAE,
    0x4A,0x7E,0x8F,0x5C,0xBF,0x19,0x2E,0xCC,0xCC,0x57,0x0C,0xDF,0x02,0x21,0x00,0xEB,
    0x67,0x2C,0x11,0x0E,0x2C,0xD3,0xB1,0x1A,0x2B,0x14,0x50,0xEB,0xC1,0xDD,0x5A,0x0E,
    0x3A,0x47,0x82,0x8B,0x89,0x59,0x2A,0x35,0x0C,0xBA,0x0B,0x09,0x11,0x1C,0x87,0x02,
    0x82,0x01,0x00,0x60,0x75,0x81,0xDB,0x53,0xF3,0xF1,0x5D,0x2E,0x87,0x2A,0xA8,0xE7,
    0x54,0x73,0xCB,0xF7,0x2D,0x95,0x97,0xC6,0xA3,0x55,0xDD,0xB4,0x51,0x42,0x30,0x04,
    0xD3,0x51,0x3A,0x39,0xD5,0xAA,0xEB,0xD5,0x1A,0x92,0x25,0x90,0x31,0x38,0x14,0x7F,
    0x6D,0x5A,0xF5,0xCA,0xFE,0x79,0xEC,0x07,0x9C,0xAB,0x06,0x44,0x32,0x43,0xD5,0x31,
    0x4F,0xE6,0x11,0x16,0x6C,0x33,0xC4,0x21,0x39,0x39,0x92,0xA0,0x3D,0x94,0x47,0x8D,
    0x59,0x50,0x12,0xDE,0xA4,0x56,0xAF,0xF8,0xE3,0xAC,0xAA,0x85,0x2A,0x93,0x08,0x4A,
    0xE3,0x7A,0x5E,0x84,0x0C,0x87,0x4D,0x28,0x98,0x56,0xBC,0x03,0x50,0xB8,0x80,0x7D,
    0x3B,0xF2,0x0E,0xF6,0x83,0xCB,0xB8,0x67,0x84,0x37,0x6A,0xDA,0x51,0x03,0x89,0x67,
    0xAA,0xB3,0xF9,0x58,0xCC,0xDF,0x68,0x1C,0x4F,0x4F,0x5B,0x3D,0xA0,0xBC,0x4F,0x51,
    0x1C,0x76,0xFC,0x02,0x3D,0xE3,0xA4,0x1B,0x38,0x5F,0xA9,0x3A,0x61,0x3C,0x45,0x72,
    0x6E,0x47,0xF5,0x43,0x4C,0xFB,0xB5,0x00,0x0E,0x6B,0xFD,0xEB,0x1C,0x6D,0xED,0xCA,
    0xED,0x75,0xE4,0xE0,0x3B,0x4F,0x40,0x3F,0xFA,0xE0,0xF2,0xF2,0xAE,0x45,0xB7,0xF8,
    0xE9,0x81,0xF9,0x8E,0x09,0x89,0x82,0x40,0x7D,0x09,0x3F,0x7C,0x00,0x76,0x38,0xBA,
    0xC7,0x9F,0x74,0x20,0xFE,0xD9,0x9B,0x13,0xC4,0xA9,0xF2,0xB8,0x65,0x6A,0x74,0xEE,
    0x7C,0xCC,0xC6,0x80,0x00,0xCA,0x1A,0x91,0x27,0xF4,0x54,0x3E,0x11,0x96,0x76,0x30,
    0x95,0xD9,0x98,0xEC,0x4A,0x3B,0x4E,0x3C,0x33,0x94,0x45,0x76,0xBD,0x87,0xAC,0xF9,
    0xB7,0xE4,0x76,0x02,0x82,0x01,0x00,0x5D,0x60,0x51,0x3D,0x2F,0x40,0xE2,0xA9,0xA5,
    0xA9,0x60,0x6C,0x81,0xD9,0x91,0x19,0x2D,0xCF,0xE5,0x7D,0x2A,0x3F,0xD6,0xAD,0xA3,
    0xB1,0xB0,0x7A,0x35,0xF3,0xF1,0x24,0x15,0x7D,0x5E,0xA3,0xC8,0x66,0xE5,0x50,0x47,
    0xBC,0x09,0x4D,0x24,0xC0,0xA9,0x1B,0xD2,0x1E,0xA4,0x88,0xC6,0x4F,0x54,0x77,0x79,
    0xD6,0xD1,0xEF,0x91,0x36,0xE6,0xA2,0x21,0xF5,0x5C,0x81,0x0D,0x50,0x99,0x9E,0xD8,
    0x85,0x5D,0xF4,0x1F,0x7E,0x18,0xA2,0x46,0xE0,0x36,0x83,0x0A,0x0B,0xB8,0x99,0xD8,
    0x6D,0x7A,0x2A,0x38,0xDA,0x5E,0xF5,0x4B,0x6B,0xBA,0x04,0x20,0x92,0xDF,0x70,0x8A,
    0x4A,0x86,0xC9,0x59,0x54,0xFE,0x94,0xE4,0x15,0x41,0xEB,0xD3,0x3A,0xAD,0xAE,0x6A,
    0x73,0x8D,0x10,0x3A,0x67,0x1E,0xB0,0xAB,0x85,0xC4,0xF6,0x1D,0x13,0xD2,0xCF,0xC6,
    0x68,0xAA,0x23,0x15,0x55,0x41,0x06,0x9D,0x76,0x0E,0xCC,0x44,0x20,0x33,0x50,0xA6,
    0x72,0xB0,0xE9,0x35,0x71,0x8C,0xD1,0xFE,0xDF,0x9D,0x6E,0xF4,0xC2,0xBC,0xF6,0x5B,
    0xF0,0x98,0xFA,0xDF,0xC4,0x5C,0x15,0xFF,0x00,0x1E,0xC7,0x24,0xB9,0xA7,0x9A,0xFA,
    0x0B,0x9C,0xA6,0x62,0x71,0x60,0x9B,0xF3,0x25,0xBA,0xF3,0x3D,0xA0,0x41,0xC6,0xDC,
    0xEC,0x83,0xE5,0xCB,0x52,0x91,0x6B,0x07,0x35,0xF2,0x53,0x4A,0x10,0x8D,0x68,0xDE,
    0x0C,0xCC,0xD2,0xE3,0xA4,0x1D,0x65,0xD9,0x9E,0x34,0xB1,0xA4,0x40,0x17,0x55,0xA7,
    0x3E,0x4A,0x81,0xE2,0xBF,0x87,0xD6,0x3E,0x78,0xF5,0x4B,0x2D,0x73,0xDC,0xAF,0xFA,
    0x44,0x42,0x7B,0x85,0x53,0x25,0x1D,0x02,0x21,0x00,0x87,0x62,0x69,0xDF,0x73,0x67,
    0x04,0xA5,0xD5,0x21,0xF6,0x34,0x54,0x20,0x66,0x13,0xDB,0xFD,0x3B,0x8A,0xE4,0x79,
    0xA5,0xCD,0x17,0xB5,0x3D,0xAE,0x4F,0x16,0xFD,0x26
  };
  
/** \known Data: (dsa_sig_ka) DSA known signature of "in" with dsa_privK_ka
   
*/
static const unsigned char dsa_sig_ka[] =
  {
    0x30,0x45,0x02,0x21,0x00,0xC7,0x36,0x5D,0x1A,0x3E,0xED,0xB9,0xB7,0x60,0x2F,0x37,
    0x52,0x34,0x4D,0xD6,0x90,0xEC,0xB4,0xA2,0x83,0x0D,0x83,0xEA,0xBE,0xAB,0xA9,0x13,
    0xDF,0x5A,0x6D,0x6B,0x43,0x02,0x20,0x09,0xD3,0x5E,0x35,0x93,0x96,0xDD,0x0E,0xBE,
    0x78,0xB2,0x0C,0x2F,0x89,0x32,0x75,0xE8,0x5B,0x8F,0xAC,0x43,0x9A,0x7E,0x64,0xC0,
    0xD7,0x3C,0xC5,0x56,0x98,0x75,0xD1
  };  


/** \known Data: (hmac_ka_key) SHA1-HMAC key */
static const unsigned char  hmac_ka_key[] = {
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
  0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f, 
  0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17, 
  0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
  0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27, 
  0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
  0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f
};
/** \known Data: (hmac_ka_data) SHA1-HMAC input */
static const unsigned char hmac_ka_data[] = {
  0x53,0x61,0x6d,0x70,0x6c,0x65,0x20,0x23,
  0x31
};
/** \known Data: (hmac_ka) SHA1-HMAC output */
static const unsigned char hmac_ka[] = {
  0x4f,0x4c,0xa3,0xd5,0xd6,0x8b,0xa7,0xcc,
  0x0a,0x12,0x08,0xc9,0xc6,0x1e,0x9c,0x5d,
  0xa0,0x40,0x3c,0x0a
};
/** \known Data: (hmac_ka_key) SHA2/3-HMAC keys */
static const unsigned char  hmacsha2_ka_key[] = {
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
  0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f, 
  0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17, 
  0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
  0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27, 
  0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
  0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f
};
/** \known Data: (hmac_ka_data) SHA2/3-HMAC input */
static const unsigned char hmacsha2_ka_data[] = {
  0x53,0x61,0x6d,0x70,0x6c,0x65,0x20,0x23,
  0x31,0x23,0x24,0x25,0x26,0x27,0x28,0x39,
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
  0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f, 
  0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17, 
  0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
  0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27, 
  0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
  0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f

};

/** \known Data: (hmac_ka) HMAC-SHA256 output */
static const unsigned char hmac256_ka[] = {
  0x4F,0xD8,0xEC,0xEE,0xB8,0x92,0xBA,0x45,
  0x89,0x6B,0x1B,0x72,0x29,0x5E,0xC2,0x75,
  0x24,0x8E,0x51,0x93,0x5E,0x2A,0xA8,0xF9,
  0x18,0x60,0xE0,0x25,0x2D,0x1C,0x67,0x54
};


/** \known Data: (hmac_ka) HMAC-SHA3-512 output */
static const unsigned char hmac3_512_ka[] = {
  0x6C,0x5B,0x92,0x50,0x13,0xE3,0xA6,0x30,
  0x98,0x64,0xF2,0x31,0x8E,0x4F,0xFA,0xAF,
  0x9D,0x52,0xD4,0x03,0x27,0xD7,0x04,0x2A,
  0x0A,0xF9,0x0D,0x99,0x33,0x6D,0xFD,0x8C,
  0x5D,0x1B,0x14,0xC8,0x37,0x23,0xDD,0x29,
  0x31,0x87,0x52,0xCC,0xB4,0x83,0xA0,0xD9,
  0x5B,0x08,0x0B,0xAE,0x71,0xAD,0x6B,0x82,
  0x30,0xB7,0x15,0xB4,0x89,0xC1,0x86,0x97
};


/** \known Data: (cmac_ka_key) CMAC AES-256 key */
static const unsigned char cmac_ka_key[] = {
  0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,
  0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
  0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,
  0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4
};
/** \known Data: (cmac_ka_data) CMAC input , used for all CMAC's*/
static const unsigned char cmac_ka_data[]= {
  0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
  0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a
};

/** \known Data: (cmac_ka) CMAC output, only for AES-256 */
static const unsigned char cmac_ka[] = {
  0x28,0xa7,0x02,0x3f,0x45,0x2e,0x8f,0x82,
  0xbd,0x4b,0xf2,0x8d,0x8c,0x37,0xc3,0x5c
};

#if defined(KNOWN) 
/** \known Data. OpenSSL curve name corresponding to NIST B-233  
    for binary field KAT
*/
static const char *EC_curve_B233 = "sect233r1";
/** \known Data. OpenSSL curve name corresponding to NIST K-233  
    for binary field KAT
*/
static const char *EC_curve_K233 = "sect233k1";

/** \known Data. OpenSSL curve name corresponding to NIST P-384  
    for prime field KAT
*/
static const char *EC_curve_P384 = "secp384r1";

#endif

/** \known Data: EC DER encoded Private key for NIST P-384 
*/
static const unsigned char EC_key_P384[] = {

 0x30,0x81,0xA4,0x02,0x01,0x01,0x04,0x30,0xBF,0x58,0x82,0x58,0xF6,0xDF,0x8D,0x17,
0x13,0x72,0x86,0xFB,0x1D,0x7B,0xDF,0x15,0x6B,0x5F,0xD0,0x68,0xF9,0x8E,0x87,0x54,
0x2B,0xCE,0x7A,0xEC,0x45,0x19,0x53,0xD0,0xB5,0x8B,0x8B,0x50,0xC1,0x74,0xF7,0x13,
0xD4,0xDA,0xEF,0xC3,0xA1,0x2F,0x8B,0x70,0xA0,0x07,0x06,0x05,0x2B,0x81,0x04,0x00,
0x22,0xA1,0x64,0x03,0x62,0x00,0x04,0xF4,0xE9,0xFC,0x96,0xBE,0x8C,0xBC,0xF6,0xC3,
0xD4,0xC5,0x9A,0x36,0xA3,0x56,0xC8,0x17,0xD8,0x6B,0x7D,0xB8,0xB5,0xDB,0xFF,0xD0,
0x63,0xDB,0xC4,0xC4,0x98,0x2D,0xEF,0xBD,0x19,0x22,0x9C,0xCF,0x13,0x59,0x9C,0x92,
0xB6,0x9B,0xB8,0xBA,0xD2,0x3A,0xBD,0xA8,0x5D,0x31,0xA9,0x84,0xCB,0x68,0xA0,0x5C,
0x5C,0x68,0x22,0x69,0x5E,0x32,0x5D,0x76,0x84,0x0A,0x53,0x50,0xCD,0x11,0xEB,0xB7,
0x12,0x95,0xEE,0xD3,0x47,0xFC,0x59,0xE6,0xC9,0x75,0xC3,0x04,0xBA,0xBA,0xC2,0x26,
0x4A,0x53,0xEA,0xEB,0xC4,0x11,0xD8
};

/** \known Data: EC DER encoded signature for P-384
*/

static const unsigned char EC_sig_P384[] = {
0x30,0x65,0x02,0x31,0x00,0xE5,0x51,0x41,0x84,0x32,0x3B,0xBD,0x4F,0x84,0x6A,0xAC,
0x73,0x18,0xF4,0x01,0x54,0x15,0x9A,0xE5,0xE1,0xA8,0xCE,0xD2,0x7C,0xA4,0x79,0x3D,
0x04,0x1D,0x02,0xAA,0x6C,0xF4,0x49,0x52,0xF1,0x43,0x52,0x1E,0xF4,0x14,0x15,0xC2,
0x55,0xB3,0xAD,0x8D,0x05,0x02,0x30,0x4C,0x83,0x35,0xBB,0x48,0xE7,0xA9,0x0B,0x26,
0x7D,0x70,0x74,0x74,0xEB,0xAF,0xC0,0x8E,0xC9,0xF3,0xA2,0xA8,0xFB,0x23,0x0F,0x4C,
0x75,0xEA,0xAE,0x12,0x70,0x28,0x84,0x5D,0xCD,0xC4,0x28,0xA1,0x0D,0xF8,0xBD,0x1E,
0xAD,0x34,0x5F,0x56,0x69,0x71,0x60

};


/** \known Data: EC DER encoded signature for P-384 with a broken RNG
*/

static const unsigned char EC_sig_P384_broken[] = {
0x30,0x65,0x02,0x30,0x50,0x89,0xA9,0x06,0x4C,0xA4,0x92,0x12,0xBF,0xE5,0x43,0x4C,
0x82,0xF2,0x8D,0xE5,0x5A,0x93,0x57,0x9C,0xC2,0x60,0xAA,0x08,0xDE,0x59,0x72,0x99,
0x82,0x30,0x20,0xE0,0xB4,0x60,0x27,0x18,0x42,0xB9,0x73,0x24,0xD5,0xFF,0x46,0x10,
0xF1,0x8C,0x74,0x70,0x02,0x31,0x00,0xC3,0x9E,0x54,0x15,0x81,0xFD,0xFE,0x14,0xEC,
0x63,0x95,0x3B,0xAB,0x63,0x78,0x9B,0x31,0x5E,0xDF,0xB9,0x44,0xB0,0x80,0xD0,0x6A,
0xDC,0x90,0xF1,0xAC,0x41,0x36,0x0F,0x6D,0x1B,0x09,0x34,0xAD,0x5E,0xDA,0xD6,0xA0,
0xA3,0x4D,0xA9,0xA3,0x4E,0x0C,0x09

};

/** \known Data: EC DER encoded Private key from NIST B-233 
    for binary field KAT
    - The private key contains the curve parameters and
    the public key as well as the private key. 
    We use the public key and parameters from here to verify
    the EC curve.
*/
static const unsigned char EC_key_B233[] = {
0x30,0x6E,0x02,0x01,0x01,0x04,0x1E,0x00,0x82,0x46,0xB3,0xFD,0xAB,0xE7,0xCC,0x95,
0x06,0x21,0x53,0x9C,0x0C,0xB3,0x63,0x68,0x9B,0x67,0x8D,0x55,0x61,0x2A,0xA5,0xB9,
0xC8,0x7A,0x33,0x64,0xC8,0xA0,0x07,0x06,0x05,0x2B,0x81,0x04,0x00,0x1B,0xA1,0x40,
0x03,0x3E,0x00,0x04,0x00,0xD6,0x33,0x59,0xAB,0x7F,0x57,0xFF,0x8C,0x70,0x41,0xF8,
0xD4,0xB1,0x21,0x9D,0x7A,0xC2,0x42,0xD4,0xB8,0x4B,0x6F,0x75,0x59,0x39,0x6F,0xB2,
0x25,0xF9,0x01,0x23,0x72,0x77,0xEB,0xFA,0x69,0xD7,0xCB,0x48,0x59,0x3E,0x8A,0xCE,
0x5E,0x5C,0x10,0x0E,0xDF,0xE1,0x0B,0xB6,0x1A,0x7B,0x24,0xE4,0x50,0x96,0xF6,0x5B,
};


/** \known Data: EC DER encoded signature for B-233

 */
static const unsigned char EC_sig_B233[] = {
0x30,0x3F,0x02,0x1D,0x43,0x76,0x9C,0x71,0x13,0x0B,0x44,0x21,0xB9,0x91,0x82,0xD3,
0xE6,0x6B,0x6E,0x80,0xCB,0xBF,0x06,0xF6,0x23,0xDA,0x3E,0xE2,0x8A,0x9E,0x0F,0x1D,
0xB0,0x02,0x1E,0x00,0x83,0xE7,0xFB,0x0C,0x2E,0xE3,0x51,0xC4,0xFB,0xE3,0xC5,0xCB,
0x19,0x89,0xB4,0xED,0x82,0xD9,0x4C,0x97,0x05,0x78,0x69,0x14,0xEE,0xC4,0x78,0xF7,
0x9F

};

/** \known Data: EC DER encoded signature for B-233 with a broken RNG

 */
static const unsigned char EC_sig_B233_broken[] = {
0x30,0x3F,0x02,0x1E,0x00,0xCB,0x10,0x70,0x11,0x07,0x94,0xD9,0x8C,0xC5,0xB3,0xF3,
0x35,0xA2,0xF8,0x6D,0x21,0x35,0xE4,0x4C,0x86,0x4B,0x56,0x25,0x99,0xCF,0x84,0xA8,
0x75,0x6F,0x02,0x1D,0x2A,0x74,0x1B,0xFE,0xD9,0x9F,0x3C,0x30,0xCA,0xD3,0x5A,0x59,
0xB6,0xCD,0x00,0x7A,0x0E,0x8E,0x0A,0x35,0x36,0xD9,0x1A,0xA8,0xE6,0xA0,0x9A,0x06,
0x4F

};
/** \known Data: EC DER encoded Private key from NIST K-233 
    for binary field KAT
    - The private key contains the curve parameters and
    the public key as well as the private key. 
    We use the public key and parameters from here to verify
    the EC curve.
*/
static const unsigned char EC_key_K233[] = {
0x30,0x6D,0x02,0x01,0x01,0x04,0x1D,0x60,0x46,0x92,0xBC,0xA6,0x1B,0x06,0xD6,0x40,
0x16,0x73,0x76,0xAD,0xB2,0x50,0xFA,0x8F,0x7E,0x63,0xA9,0x88,0xA8,0xE7,0xB9,0x34,
0x51,0x63,0x3E,0xFB,0xA0,0x07,0x06,0x05,0x2B,0x81,0x04,0x00,0x1A,0xA1,0x40,0x03,
0x3E,0x00,0x04,0x00,0xA3,0xDA,0x08,0x4D,0xE6,0x31,0xF8,0xBB,0x32,0x62,0x94,0x76,
0x38,0xDB,0xAD,0xCA,0x45,0xF8,0xAC,0xEB,0xCA,0x16,0x29,0xE2,0x4D,0xCD,0xAF,0xA1,
0x66,0x00,0x70,0x6D,0xC5,0xE2,0x91,0xED,0xFA,0x08,0xE7,0x11,0x21,0xDE,0xA6,0x88,
0x72,0x6B,0xA9,0x56,0x3F,0x27,0x47,0x69,0xC2,0xA1,0xC1,0x59,0xC2,0xF3,0xA6

};


/** \known Data: EC DER encoded signature for B-233

 */
static const unsigned char EC_sig_K233[] = {

0x30,0x3E,0x02,0x1D,0x29,0x74,0x61,0xFC,0x25,0x4A,0xB2,0x83,0xBB,0x76,0x82,0xD6,
0xFE,0x26,0x92,0x2A,0x38,0x07,0xAB,0x2D,0x5F,0x83,0xB0,0x23,0xF3,0xC1,0x71,0xFD,
0x58,0x02,0x1D,0x37,0x49,0xFA,0x1A,0xD8,0x77,0x9E,0x1B,0x3C,0x4F,0xE5,0x68,0x23,
0x1C,0xDE,0xCE,0x62,0x82,0x53,0xCD,0x96,0x0B,0x77,0xB9,0x26,0x8F,0x6B,0x8F,0x80,
};


/** \known Data: EC DER encoded signature for B-233 with a broken RNG

 */
static const unsigned char EC_sig_K233_broken[] = {

0x30,0x3E,0x02,0x1D,0x05,0x34,0x4B,0x34,0x82,0xDC,0x4B,0xE6,0xA0,0x7F,0xA3,0xD0,
0x4E,0x81,0x12,0xCA,0x94,0xE1,0x3F,0x38,0xD3,0xB3,0x6E,0x61,0x9C,0x9B,0x07,0xCD,
0x34,0x02,0x1D,0x2A,0x21,0x57,0x01,0xD1,0x77,0x5F,0x89,0xED,0x30,0x28,0x6D,0x8E,
0xB9,0x8D,0xFD,0x22,0x19,0x73,0x47,0xFA,0x56,0x6F,0x9B,0x20,0x1F,0x4E,0xE2,0x87,

};
/** \known Data: EC DER encoded Private key from X448 

*/
static const unsigned char EC_key_X448[] = {
0x30,0x46,0x02,0x01,0x00,0x30,0x05,0x06,0x03,0x2b,0x65,0x6f,0x04,0x3a,0x04,0x38,
0x9a,0x8f,0x49,0x25,0xd1,0x51,0x9f,0x57,0x75,0xcf,0x46,0xb0,0x4b,0x58,0x00,0xd4,
0xee,0x9e,0xe8,0xba,0xe8,0xbc,0x55,0x65,0xd4,0x98,0xc2,0x8d,0xd9,0xc9,0xba,0xf5,
0x74,0xa9,0x41,0x97,0x44,0x89,0x73,0x91,0x00,0x63,0x82,0xa6,0xf1,0x27,0xab,0x1d,
0x9a,0xc2,0xd8,0xc0,0xa5,0x98,0x72,0x6b
};

/** \known Data: EC DER encoded signature for ED448

 */
static const unsigned char EC_sig_ED448[] = {

0x30,0x3E,0x02,0x1D,0x29,0x74,0x61,0xFC,0x25,0x4A,0xB2,0x83,0xBB,0x76,0x82,0xD6,
0xFE,0x26,0x92,0x2A,0x38,0x07,0xAB,0x2D,0x5F,0x83,0xB0,0x23,0xF3,0xC1,0x71,0xFD,
0x58,0x02,0x1D,0x37,0x49,0xFA,0x1A,0xD8,0x77,0x9E,0x1B,0x3C,0x4F,0xE5,0x68,0x23,
0x1C,0xDE,0xCE,0x62,0x82,0x53,0xCD,0x96,0x0B,0x77,0xB9,0x26,0x8F,0x6B,0x8F,0x80,
};


/** \known Data: EC DER encoded signature for B-233 with a broken RNG

 */
static const unsigned char EC_sig_ED448_broken[] = {

0x30,0x3E,0x02,0x1D,0x05,0x34,0x4B,0x34,0x82,0xDC,0x4B,0xE6,0xA0,0x7F,0xA3,0xD0,
0x4E,0x81,0x12,0xCA,0x94,0xE1,0x3F,0x38,0xD3,0xB3,0x6E,0x61,0x9C,0x9B,0x07,0xCD,
0x34,0x02,0x1D,0x2A,0x21,0x57,0x01,0xD1,0x77,0x5F,0x89,0xED,0x30,0x28,0x6D,0x8E,
0xB9,0x8D,0xFD,0x22,0x19,0x73,0x47,0xFA,0x56,0x6F,0x9B,0x20,0x1F,0x4E,0xE2,0x87,

};
/** \known Data: EC DER encoded Private key from X25519 

*/
static const unsigned char EC_key_X25519[] = {
0x30,0x2e,0x02,0x01,0x00,0x30,0x05,0x06,0x03,0x2b,0x65,0x6e,0x04,0x22,0x04,0x20,
0x77,0x07,0x6d,0x0a,0x73,0x18,0xa5,0x7d,0x3c,0x16,0xc1,0x72,0x51,0xb2,0x66,0x45,
0xdf,0x4c,0x2f,0x87,0xeb,0xc0,0x99,0x2a,0xb1,0x77,0xfb,0xa5,0x1d,0xb9,0x2c,0x2a
};

/** \known Data: EC DER encoded signature for ED25519

 */
static const unsigned char EC_sig_ED25519[] = {

0x30,0x3E,0x02,0x1D,0x29,0x74,0x61,0xFC,0x25,0x4A,0xB2,0x83,0xBB,0x76,0x82,0xD6,
0xFE,0x26,0x92,0x2A,0x38,0x07,0xAB,0x2D,0x5F,0x83,0xB0,0x23,0xF3,0xC1,0x71,0xFD,
0x58,0x02,0x1D,0x37,0x49,0xFA,0x1A,0xD8,0x77,0x9E,0x1B,0x3C,0x4F,0xE5,0x68,0x23,
0x1C,0xDE,0xCE,0x62,0x82,0x53,0xCD,0x96,0x0B,0x77,0xB9,0x26,0x8F,0x6B,0x8F,0x80,
};


/** \known Data: EC DER encoded signature for B-233 with a broken RNG

 */
static const unsigned char EC_sig_ED25519_broken[] = {

0x30,0x3E,0x02,0x1D,0x05,0x34,0x4B,0x34,0x82,0xDC,0x4B,0xE6,0xA0,0x7F,0xA3,0xD0,
0x4E,0x81,0x12,0xCA,0x94,0xE1,0x3F,0x38,0xD3,0xB3,0x6E,0x61,0x9C,0x9B,0x07,0xCD,
0x34,0x02,0x1D,0x2A,0x21,0x57,0x01,0xD1,0x77,0x5F,0x89,0xED,0x30,0x28,0x6D,0x8E,
0xB9,0x8D,0xFD,0x22,0x19,0x73,0x47,0xFA,0x56,0x6F,0x9B,0x20,0x1F,0x4E,0xE2,0x87,

};

/** \known Data: AES-CCM AES key 
    @note source: SP800-38C.pdf example 1
*/
static const unsigned char AES_CCM_key[] = {
  0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
  0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f
};
/** \known Data: AES-CCM Nonce
    @note source: SP800-38C.pdf example 1
*/
static const unsigned char AES_CCM_nonce[] = {
  0x10,0x11,0x12,0x13,0x14,0x15,0x16
};

/** \known Data: AES-CCM AAD 
    @note source: SP800-38C.pdf example 1
*/
static const unsigned char AES_CCM_AAD[] = {
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07
};

/** \known Data: AES-CCM Plaintext
    @note source: SP800-38C.pdf example 1
*/
static const unsigned char AES_CCM_PT[] = {
  0x20,0x21,0x22,0x23
};

/** \known Data: AES-CCM Ciphertext
    @note source: SP800-38C.pdf example 1
*/
static const unsigned char AES_CCM_CT[] = {
  0x71,0x62,0x01,0x5b,0x4d,0xac,0x25,0x5d,
};
/** \known Data: AES-GCM key
    @note source: NIST NIST_SP_800-38D_June_2007_for_public_comment.pdf, test vector 4
*/
static const unsigned char  gcm_ka_key[] = {
  0xfe,0xff,0xe9,0x92,0x86,0x65,0x73,0x1c,
  0x6d,0x6a,0x8f,0x94,0x67,0x30,0x83,0x08
};
/** \known Data: AES-GCM plaintext
    @note source: NIST NIST_SP_800-38D_June_2007_for_public_comment.pdf, test vector 4
*/

static const unsigned char gcm_ka_plaintext[] = {
  0xd9,0x31,0x32,0x25,0xf8,0x84,0x06,0xe5,
  0xa5,0x59,0x09,0xc5,0xaf,0xf5,0x26,0x9a,
  0x86,0xa7,0xa9,0x53,0x15,0x34,0xf7,0xda,
  0x2e,0x4c,0x30,0x3d,0x8a,0x31,0x8a,0x72,
  0x1c,0x3c,0x0c,0x95,0x95,0x68,0x09,0x53,
  0x2f,0xcf,0x0e,0x24,0x49,0xa6,0xb5,0x25,
  0xb1,0x6a,0xed,0xf5,0xaa,0x0d,0xe6,0x57,
  0xba,0x63,0x7b,0x39
};
/** \known Data: AES-GCM nonce
    @note source: NIST NIST_SP_800-38D_June_2007_for_public_comment.pdf, test vector 4
*/

static const unsigned char gcm_ka_iv[] = {
  0xca,0xfe,0xba,0xbe,0xfa,0xce,0xdb,0xad,
  0xde,0xca,0xf8,0x88
};
/** \known Data: AES-GCM additional authentication data
    @note source: NIST NIST_SP_800-38D_June_2007_for_public_comment.pdf, test vector 4
*/

static const unsigned char gcm_ka_aad[] = {
  0xfe,0xed,0xfa,0xce,0xde,0xad,0xbe,0xef,
  0xfe,0xed,0xfa,0xce,0xde,0xad,0xbe,0xef,
  0xab,0xad,0xda,0xd2
};
/** \known Data: AES-GCM ciphertext
    @note source: NIST NIST_SP_800-38D_June_2007_for_public_comment.pdf, test vector 4
*/

static const unsigned char gcm_ka_ciphertext[] = {
  0x42,0x83,0x1e,0xc2,0x21,0x77,0x74,0x24,
  0x4b,0x72,0x21,0xb7,0x84,0xd0,0xd4,0x9c,
  0xe3,0xaa,0x21,0x2f,0x2c,0x02,0xa4,0xe0,
  0x35,0xc1,0x7e,0x23,0x29,0xac,0xa1,0x2e,
  0x21,0xd5,0x14,0xb2,0x54,0x66,0x93,0x1c,
  0x7d,0x8f,0x6a,0x5a,0xac,0x84,0xaa,0x05,
  0x1b,0xa3,0x0b,0x39,0x6a,0x0a,0xac,0x97,
  0x3d,0x58,0xe0,0x91
};
/** \known Data: AES-GCM authentication tag
    @note source: NIST NIST_SP_800-38D_June_2007_for_public_comment.pdf, test vector 4
*/

static const unsigned char gcm_ka_authtag[] = {
  0x5b,0xc9,0x4f,0xbc,0x32,0x21,0xa5,0xdb,
  0x94,0xfa,0xe9,0x5a,0xe7,0x12,0x1a,0x47
};

/** \known Data: AES-128-XTS key 
    @note source: NIST XTS sample test vector [Encrypt 1]
*/
static const unsigned char XTS_128_Key[32] = {
  0xa1,0xb9,0x0c,0xba,0x3f,0x06,0xac,0x35,
  0x3b,0x2c,0x34,0x38,0x76,0x08,0x17,0x62,
  0x09,0x09,0x23,0x02,0x6e,0x91,0x77,0x18,
  0x15,0xf2,0x9d,0xab,0x01,0x93,0x2f,0x2f
};

/** \known Data: AES-128-XTS IV
    @note source: NIST XTS sample test vector [Encrypt 1]
*/
static const unsigned char XTS_128_IV[16] = {
  0x4f,0xae,0xf7,0x11,0x7c,0xda,0x59,0xc6,
  0x6e,0x4b,0x92,0x01,0x3e,0x76,0x8a,0xd5,
};
/** \known Data: AES-128-XTS Plaintext
    @note source: NIST XTS sample test vector [Encrypt 1]
*/
static const unsigned char XTS_128_PT[16] = {
  0xeb,0xab,0xce,0x95,0xb1,0x4d,0x3c,0x8d,
  0x6f,0xb3,0x50,0x39,0x07,0x90,0x31,0x1c,
};
/** \known Data: AES-128-XTS Ciphertext
    @note source: NIST XTS sample test vector [Encrypt 1]
*/
static const unsigned char XTS_128_CT[16] = {
  0x77,0x8a,0xe8,0xb4,0x3c,0xb9,0x8d,0x5a,
  0x82,0x50,0x81,0xd5,0xbe,0x47,0x1c,0x63,
};

/** \known Data: SHAKE128 XOF
*/
static const unsigned char SHAKE128_CT[128] = {
    0x92, 0x0B, 0x40, 0x87, 0x7D, 0xEC, 0xC8, 0xDF, 0x62, 0xE3, 0x3F, 0x2D,
    0xD9, 0xDC, 0x87, 0x93, 0x55, 0xFB, 0x17, 0xFB, 0x0E, 0x57, 0x05, 0xCD,
    0x3C, 0x14, 0x8C, 0x7A, 0xB3, 0x84, 0xB0, 0x33, 0x9A, 0xF7, 0x87, 0xD2,
    0x32, 0xDB, 0xED, 0x33, 0xA9, 0x3F, 0x92, 0x8F, 0x44, 0xC9, 0xB5, 0x41,
    0x84, 0xE9, 0x58, 0xEB, 0x35, 0xB1, 0x1F, 0x82, 0x41, 0x30, 0xDA, 0x68,
    0xDC, 0x54, 0xBA, 0x27, 0xB6, 0x90, 0x86, 0x6A, 0x78, 0x34, 0x49, 0xD6,
    0x8A, 0x36, 0x7D, 0xBF, 0xF8, 0x86, 0xBD, 0x5B, 0xA6, 0x2B, 0x67, 0x6C,
    0xEA, 0x8F, 0x47, 0x92, 0x69, 0x89, 0x46, 0x0A, 0x3C, 0x18, 0xDB, 0x86,
    0x20, 0x23, 0x37, 0x0C, 0x5B, 0xF4, 0x5A, 0x35, 0x76, 0x82, 0x83, 0xB7,
    0x06, 0xBA, 0x31, 0x14, 0x53, 0xC2, 0x12, 0x35, 0x1C, 0xFF, 0xB0, 0x4A,
    0x94, 0xA2, 0x9A, 0xF0, 0x62, 0x2C, 0x3F, 0x49
};
/** \known Data: TLS1 KDF
 * inputs are 'secret' and 'salt' non-0 terminated
 */
static const unsigned char TLS1_ka[16] = {
  0x8e, 0x4d, 0x93, 0x25, 0x30, 0xd7, 0x65, 0xa0,
  0xaa, 0xe9, 0x74, 0xc3, 0x04, 0x73, 0x5e, 0xcc
};


/** \known Data: DH
 */

static const unsigned char dh_test_2048_p[] = {
      0xAE,0xEC,0xEE,0x22,0xFA,0x3A,0xA5,0x22,0xC0,0xDE,0x0F,0x09,
      0x7E,0x17,0xC0,0x05,0xF9,0xF1,0xE7,0xC6,0x87,0x14,0x6D,0x11,
      0xE7,0xAE,0xED,0x2F,0x72,0x59,0xC5,0xA9,0x9B,0xB8,0x02,0xA5,
      0xF3,0x69,0x70,0xD6,0xDD,0x90,0xF9,0x19,0x79,0xBE,0x60,0x8F,
      0x25,0x92,0x30,0x1C,0x51,0x51,0x38,0x26,0x82,0x25,0xE6,0xFC,
      0xED,0x65,0x96,0x8F,0x57,0xE5,0x53,0x8B,0x38,0x63,0xC7,0xCE,
      0xBC,0x1B,0x4D,0x18,0x2A,0x5B,0x04,0x3F,0x6A,0x3C,0x94,0x39,
      0xAE,0x36,0xD6,0x5E,0x0F,0xA2,0xCC,0xD0,0xD4,0xD5,0xC6,0x1E,
      0xF6,0xA0,0xF5,0x89,0x4E,0xB4,0x0B,0xA4,0xB3,0x2B,0x3D,0xE2,
      0x4E,0xE1,0x49,0x25,0x99,0x5F,0x32,0x16,0x33,0x32,0x1B,0x7A,
      0xA5,0x5C,0x6B,0x34,0x0D,0x39,0x99,0xDC,0xF0,0x76,0xE5,0x5A,
      0xD4,0x71,0x00,0xED,0x5A,0x73,0xFB,0xC8,0x01,0xAD,0x99,0xCF,
      0x99,0x52,0x7C,0x9C,0x64,0xC6,0x76,0x40,0x57,0xAF,0x59,0xD7,
      0x38,0x0B,0x40,0xDE,0x33,0x0D,0xB8,0x76,0xEC,0xA9,0xD8,0x73,
      0xF8,0xEF,0x26,0x66,0x06,0x27,0xDD,0x7C,0xA4,0x10,0x9C,0xA6,
      0xAA,0xF9,0x53,0x62,0x73,0x1D,0xBA,0x1C,0xF1,0x67,0xF4,0x35,
      0xED,0x6F,0x37,0x92,0xE8,0x4F,0x6C,0xBA,0x52,0x6E,0xA1,0xED,
      0xDA,0x9F,0x85,0x11,0x82,0x52,0x62,0x08,0x44,0xF1,0x30,0x03,
      0xC3,0x38,0x2C,0x79,0xBD,0xD4,0x43,0x45,0xEE,0x8E,0x50,0xFC,
      0x29,0x46,0x9A,0xFE,0x54,0x1A,0x19,0x8F,0x4B,0x84,0x08,0xDE,
      0x20,0x62,0x73,0xCC,0xDD,0x7E,0xF0,0xEF,0xA2,0xFD,0x86,0x58,
      0x4B,0xD8,0x37,0xEB
};

static const unsigned char dh_test_2048_g[] = {
      0x02
};

static const unsigned char dh_test_2048_pub_key[] = {
      0xA0,0x39,0x11,0x77,0x9A,0xC1,0x30,0x1F,0xBE,0x48,0xA7,0xAA,
      0xA0,0x84,0x54,0x64,0xAD,0x1B,0x70,0xFA,0x13,0x55,0x63,0xD2,
      0x1F,0x62,0x32,0x93,0x8E,0xC9,0x3E,0x09,0xA7,0x64,0xE4,0x12,
      0x6E,0x1B,0xF2,0x92,0x3B,0xB9,0xCB,0x56,0xEA,0x07,0x88,0xB5,
      0xA6,0xBC,0x16,0x1F,0x27,0xFE,0xD8,0xAA,0x40,0xB2,0xB0,0x2D,
      0x37,0x76,0xA6,0xA4,0x82,0x2C,0x0E,0x22,0x64,0x9D,0xCB,0xD1,
      0x00,0xB7,0x89,0x14,0x72,0x4E,0xBE,0x48,0x41,0xF8,0xB2,0x51,
      0x11,0x09,0x4B,0x22,0x01,0x23,0x39,0x96,0xE0,0x15,0xD7,0x9F,
      0x60,0xD1,0xB7,0xAE,0xFE,0x5F,0xDB,0xE7,0x03,0x17,0x97,0xA6,
      0x16,0x74,0xBD,0x53,0x81,0x19,0xC5,0x47,0x5E,0xCE,0x8D,0xED,
      0x45,0x5D,0x3C,0x00,0xA0,0x0A,0x68,0x6A,0xE0,0x8E,0x06,0x46,
      0x6F,0xD7,0xF9,0xDF,0x31,0x7E,0x77,0x44,0x0D,0x98,0xE0,0xCA,
      0x98,0x09,0x52,0x04,0x90,0xEA,0x6D,0xF4,0x30,0x69,0x8F,0xB1,
      0x9B,0xC1,0x43,0xDB,0xD5,0x8D,0xC8,0x8E,0xB6,0x0B,0x05,0xBE,
      0x0E,0xC5,0x99,0xC8,0x6E,0x4E,0xF3,0xCB,0xC3,0x5E,0x9B,0x53,
      0xF7,0x06,0x1C,0x4F,0xC7,0xB8,0x6E,0x30,0x18,0xCA,0x9B,0xB9,
      0xBC,0x5F,0x17,0x72,0x29,0x5A,0xE5,0xD9,0x96,0xB7,0x0B,0xF3,
      0x2D,0x8C,0xF1,0xE1,0x0E,0x0D,0x74,0xD5,0x9D,0xF0,0x06,0xA9,
      0xB4,0x95,0x63,0x76,0x46,0x55,0x48,0x82,0x39,0x90,0xEF,0x56,
      0x75,0x34,0xB8,0x34,0xC3,0x18,0x6E,0x1E,0xAD,0xE3,0x48,0x7E,
      0x93,0x2C,0x23,0xE7,0xF8,0x90,0x73,0xB1,0x77,0x80,0x67,0xA9,
      0x36,0x9E,0xDA,0xD2
};
 
static const unsigned char dh_test_2048_priv_key[] = {
      0x0C,0x4B,0x30,0x89,0xD1,0xB8,0x62,0xCB,0x3C,0x43,0x64,0x91,
      0xF0,0x91,0x54,0x70,0xC5,0x27,0x96,0xE3,0xAC,0xBE,0xE8,0x00,
      0xEC,0x55,0xF6,0xCC
};

static const unsigned char dh_test_2048_shared_secret[] = {
    0x62, 0x68, 0x15, 0xbd, 0xc4, 0x9a, 0x3c, 0xfc,
    0xda, 0x5d, 0xc5, 0x81, 0xc9, 0xe7, 0x1b, 0xbb,
    0x94, 0x19, 0xb0, 0x5d, 0x95, 0xc3, 0x98, 0xd0,
    0xc6, 0x8b, 0x05, 0x34, 0xa5, 0xe2, 0xe4, 0xa8,
    0x7c, 0x4b, 0x7c, 0x41, 0xf9, 0x6d, 0xc1, 0xcc,
    0x6e, 0xb6, 0x34, 0xe1, 0x71, 0xc3, 0x00, 0x03,
    0x06, 0x08, 0x1d, 0x90, 0x88, 0x3c, 0x5d, 0x14,
    0x2d, 0x56, 0xac, 0x78, 0x83, 0xd6, 0xe9, 0x7c,
    0x6c, 0x34, 0xdf, 0xe0, 0x98, 0x14, 0xaa, 0xbe,
    0x3b, 0x83, 0xc5, 0xd1, 0xac, 0xec, 0xa6, 0x0b,
    0xc1, 0x94, 0x8d, 0x42, 0x3f, 0xb8, 0x63, 0xef,
    0xb1, 0x1b, 0x60, 0x4f, 0xfa, 0xfa, 0xbb, 0x57,
    0x28, 0x27, 0x4d, 0x78, 0xa4, 0x3d, 0x7a, 0xd8,
    0xab, 0x2e, 0x7d, 0x8b, 0xd3, 0xa9, 0x78, 0x74,
    0xfe, 0x3a, 0x08, 0x5f, 0xe3, 0xf5, 0x5a, 0xfa,
    0xa6, 0x93, 0x67, 0xea, 0xae, 0x5e, 0xd6, 0xc5,
    0xa1, 0xab, 0x0a, 0x1e, 0x78, 0xe7, 0xdd, 0xbc,
    0xae, 0xb7, 0x3e, 0x7d, 0x8b, 0xd8, 0x66, 0x92,
    0x38, 0x1b, 0x96, 0xeb, 0xcb, 0xcb, 0x6a, 0xcc,
    0xd8, 0x42, 0x80, 0x66, 0xa9, 0xa2, 0x75, 0xeb,
    0xe4, 0x79, 0x11, 0x7a, 0xca, 0x84, 0x77, 0x7a,
    0xe6, 0xe2, 0x13, 0xb1, 0x90, 0xd3, 0x0f, 0x87,
    0x2a, 0x0f, 0xf5, 0x17, 0x61, 0x15, 0x05, 0x31,
    0x5f, 0xdf, 0xb4, 0x8e, 0xf3, 0x21, 0x27, 0x6a,
    0x69, 0xdc, 0x52, 0x79, 0x64, 0x51, 0x1f, 0xc0,
    0xed, 0x55, 0x57, 0xd9, 0x5c, 0x6f, 0xdb, 0xaa,
    0x08, 0x44, 0xb9, 0x71, 0x71, 0x15, 0x27, 0xe8,
    0xe9, 0x42, 0x78, 0xc1, 0xc4, 0xc0, 0xbd, 0x28,
    0x23, 0xa1, 0x30, 0x57, 0xf0, 0x2e, 0x24, 0xf0,
    0x34, 0x17, 0x97, 0x1c, 0x4c, 0x2a, 0x98, 0x76,
    0x3d, 0x50, 0x7f, 0x32, 0xa2, 0x25, 0x94, 0x9e,
    0x1e, 0xbc, 0x97, 0x96, 0xd6, 0x14, 0x61, 0x5b
};

/** \known Data: PBKDF2
 * 
 */
/*
Password = "passwordPASSWORDpassword"
Salt = "saltSALTsaltSALTsaltSALTsaltSALTsalt"
Needs to wok on zOS as well though.
*/
static const unsigned char PBKDF2_PWD[] = {0x70,0x61,0x73,0x73,0x77,0x6F,0x72,0x64,
              0x50,0x41,0x53,0x53,0x57,0x4F,0x52,0x44,
              0x70,0x61,0x73,0x73,0x77,0x6F,0x72,0x64
              };

static const unsigned char PBKDF2_Salt[] = {0x73,0x61,0x6C,0x74,0x53,0x41,0x4C,0x54,
              0x73,0x61,0x6C,0x74,0x53,0x41,0x4C,0x54,
              0x73,0x61,0x6C,0x74,0x53,0x41,0x4C,0x54,
              0x73,0x61,0x6C,0x74,0x53,0x41,0x4C,0x54,
              0x73,0x61,0x6C,0x74
              };

static const unsigned char PBKDF2_key[] = {0x34,0x8c,0x89,0xdb,0xcb,0xd3,0x2b,0x2f,
              0x32,0xd8,0x14,0xb8,0x11,0x6e,0x84,0xcf,
              0x2b,0x17,0x34,0x7e,0xbc,0x18,0x00,0x18,
              0x1c,0x4e,0x2a,0x1f,0xb8,0xdd,0x53,0xe1,
              0xc6,0x35,0x51,0x8c,0x7d,0xac,0x47,0xe9
            };

static const int PBKDF2_Iters = 4096;

static const char *PBKDF2_digest = "SHA256";
/* Welcome to the big fat security hole NIST insist we install to get FIPS now
  We have to do DSA/ECDSA/RSA-PSS sign verify tests with known answers. This means
  we need to install a broken RNG to achieve reproducable signatures.
  This is done once at startup because it's unsafe to do it at any other time.
  Later calls to ICC_SelfTest() do a verify on a known signature, then a sign/verify with a real
  random number generator
  */
static void insecure_rand_cleanup(void)
{

}
static int insecure_rand_seed(const void *in,int num)
{
  return 1;
}
static int insecure_rand_add(const void *buf, int num, double add_entropy)
{
  return 1;
}

static int insecure_rand_bytes(unsigned char *buf, int num)
{
  if(num > 0) {
    memset(buf,0x5a,num);
  }
  return 1;
}


static int insecure_rand_pseudo_bytes(unsigned char *buf, int num)
{
  if(num > 0) {
    memset(buf,0xa5,num);
  }

  return 1;
}

static int insecure_rand_status(void)
{
  return (1);
}

static struct rand_meth_st insecure_rand_meth= {
    insecure_rand_seed,
    insecure_rand_bytes,
    insecure_rand_cleanup,
    insecure_rand_add,
    insecure_rand_pseudo_bytes,
    insecure_rand_status
};

/** @brief Common code for signature generation operations
 * @param stat pointer to an ICC_STATUS struct
 * @param pkey An EVP_PKEY containing the key data
 * @param sig a buffer to contain the signature
 * @param sigL a pointer to the provided buffer length, contains the signature length on exit
 * @param flags Only used for RSA, Padding mode. RSA_PKCS1_PADDING, RSA_PKCS1_PSS_PADDING. PSS SHA256 is used. 
 *        - number is a NID for ED448,ED25519
 * @param msg the message to be signed
 * @note we always use SHA256 hash here and the same input (in)
 */

static int GenerateSig(ICC_STATUS *stat,EVP_PKEY *pkey,unsigned char *sig,size_t *sigL,int flags,const char *msg)
{
  int rc = -1;
  EVP_MD_CTX *md_ctx = NULL;
  EVP_PKEY_CTX *pctx = NULL;
  const EVP_MD *md = NULL;
  IN();
  md_ctx = EVP_MD_CTX_new();
  if(flags >= 0) { /* ED448, 25519 don't specifiy the MD as it's defined by the alg*/
    md = EVP_get_digestbyname("SHA256");
  }
  rc = EVP_DigestSignInit(md_ctx,&pctx,md,NULL,pkey);
  switch(flags) {

    case RSA_PKCS1_PADDING:
      EVP_PKEY_CTX_set_rsa_padding(pctx,RSA_PKCS1_PADDING);
    break;
    case RSA_PKCS1_PSS_PADDING:
      EVP_PKEY_CTX_set_rsa_padding(pctx,RSA_PKCS1_PSS_PADDING);
      EVP_PKEY_CTX_set_rsa_pss_saltlen(pctx, RSA_PSS_SALTLEN_AUTO);
      EVP_PKEY_CTX_set_rsa_pss_keygen_mgf1_md(pctx,md);
    break;  
    default:
    break;
  }
  if(1 == rc) {
    EVP_SignUpdate(md_ctx,in,sizeof(in));
    rc = EVP_DigestSignFinal(md_ctx,sig,sigL);
  }
  EVP_MD_CTX_free(md_ctx);
  if(1 != rc) {
    SetStatusLn2(NULL,stat,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,msg,"Signature generation failed",__FILE__,__LINE__);
  }
  OUTRC(stat->majRC);
  return stat->majRC;
}

/** @brief Common code for signature verify operations
 * @param stat An ICC_STATUS pointer
 * @param pkey An EVP_PKEY containing the key data
 * @param sig signature buffer
 * @param sigL a pointer to the provided signature length
 * @param flags control flags
 * @param msg the message to sign
 * @param error force a failure on this path (ICC_INDUCED_FAILURE)
 * @note we always use SHA256 hash here and the same input (in)
 * @return 1 verify suceeded, 0 verify failed, -1 something bad happened
 */

static int VerifySig(ICC_STATUS *stat,EVP_PKEY *pkey,const unsigned char *sig,size_t sigL,int flags,const char *msg,int error)
{
  int rc = -1;
  EVP_MD_CTX *md_ctx = NULL;
  EVP_PKEY_CTX *pctx = NULL;
  const EVP_MD *md = NULL;
  unsigned char *tmp = (unsigned char *)in;
  unsigned char *ptr = NULL;
  IN();
  /** \known Code: Trip failures in verify paths */
  if(error && (icc_failure == error) ) {
    ptr = ICC_Malloc(sizeof(in),__FILE__,__LINE__);
    if(NULL != ptr) {
      memcpy(ptr,in,sizeof(in));
      tmp = ptr;
      tmp[5] = ~tmp[5];
    }
  }
  md_ctx = EVP_MD_CTX_new();
  md = EVP_get_digestbyname("SHA256");
  rc = EVP_DigestVerifyInit(md_ctx,&pctx,md,NULL,pkey);
    switch(flags) {
    case RSA_PKCS1_PADDING:
      EVP_PKEY_CTX_set_rsa_padding(pctx,RSA_PKCS1_PADDING);
    break;
    case RSA_PKCS1_PSS_PADDING:
      EVP_PKEY_CTX_set_rsa_padding(pctx,RSA_PKCS1_PSS_PADDING);
    break;  
    default:
    break;
  }
  if(1 == rc) {
    EVP_SignUpdate(md_ctx,tmp,sizeof(in));
    rc = EVP_DigestVerifyFinal(md_ctx,sig,sigL);
  }
  EVP_MD_CTX_free(md_ctx);
  if(1 != rc) {
    SetStatusLn2(NULL,stat,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,msg,"Signature verify failed",__FILE__,__LINE__);
  }
  if(NULL != ptr) {
    ICC_Free(ptr); 
  }
  OUTRC(stat->majRC);
  return stat->majRC;
}

#if defined(KNOWN)
/**  
     @brief
     This function is used to generate RSA signatures and known answers for the build 
     It is not publicly available and is used as a tool
     to aid in creating the known answer data used in power on self tests.
     It has no impact to the security of the module       
     Output is dumped to stdout.
     @param stat A pointer to an ICC_STATUS struct
     @param key DER encoded key
     @param len length of key
     @param flags RSA Padding mode
     \debug 
     Code: iccGenerateRSASig: controlled by KNOWN , used to generate RSA known answer data
*/

static void iccGenerateRSASig (ICC_STATUS *stat,const unsigned char *key, int len, int flags)
{

  RSA *rsa = NULL;
  unsigned char *sig = NULL;
  const unsigned char *tmp = NULL;
  size_t outL = 0;
  EVP_PKEY *pkey = NULL;

  IN();
  tmp = key;
  d2i_RSAPrivateKey(&rsa,&tmp,len);

  sig = (unsigned char *) ICC_Malloc(2048,__FILE__,__LINE__);
  if( NULL != sig) {    
    pkey = EVP_PKEY_new();
    EVP_PKEY_set1_RSA(pkey,rsa);
    outL = 2048; /* Size of the buffer */
    GenerateSig(stat,pkey,sig,&outL,flags,"RSA siggen");

    printf("RSA signature of known input %d bytes\n",(int)outL);
    iccPrintBytes(sig,outL);

    EVP_PKEY_free(pkey);
    memset(sig,0,sizeof(2048));
    ICC_Free(sig);
  }
  RSA_free(rsa);
  OUT();
}
/**  
     @brief
     This function is used to generate keys for the build 
     It is not publicly available and is used as a tool
     to aid in programming.
     It has no impact to the security of the module       
     Output is dumped to stdout.
     @param stat A pointer to an ICC_STATUS struct
     @param len length of key 
     \debug 
     Code: iccGenerateRSA: controlled by KNOWN , used to generate RSA known answer data
*/
static void iccGenerateRSA (ICC_STATUS *stat,int keyL)
{
  RSA           * rsa = NULL;
  unsigned char * derB = NULL;
  int             allocL = 1024;
  unsigned char * temp = NULL;
  EVP_PKEY      * pkey = NULL;
  size_t          sigL = 0;
  int             len = 0;

  IN();
  allocL = keyL*2;
  derB = (unsigned char *)ICC_Malloc(allocL,__FILE__,__LINE__);

#if !defined(KNOWN_KEYS)  
  /* Generate a new key pair */
  rsa = RSA_generate_key( keyL, 65537, NULL, NULL);
  temp = derB;
  len = i2d_RSAPrivateKey( rsa, &temp);
  printf("RSA Private key %d bits\n",keyL);
  iccPrintBytes(derB,len);
#else
  /* Use the one hardwired in the sources */
  temp = RSA_key;
  len = sizeof(RSA_key);
  d2i_RSA_PrivateKey(&rsa,&temp,len);
#endif


  pkey = EVP_PKEY_new();
  EVP_PKEY_set1_RSA(pkey,rsa);
  sigL = allocL;
  GenerateSig(stat,pkey,derB,&sigL,RSA_PKCS1_PADDING,"RSA PKCS1.5");
  printf("RSA PKCS1.5 signature\n");
  iccPrintBytes(derB, (int)sigL);

  EVP_PKEY_free(pkey);
  pkey = EVP_PKEY_new();
  EVP_PKEY_set1_RSA(pkey,rsa);
  sigL = allocL;
  GenerateSig(stat,pkey,derB,&sigL,RSA_PKCS1_PSS_PADDING,"RSA-PSS");
  printf("RSA PSS signature. (SHA256)\n");
  iccPrintBytes(derB, (int)sigL);

  EVP_PKEY_free(pkey);
  RSA_free(rsa);
  ICC_Free(derB);
  OUT();
}


/**  
     @brief
     This function is used to generate DSA signatures and known answers for the build 
     It is not publicly available and is used as a tool
     to aid in creating the known answer data used in power on self tests.
     It has no impact to the security of the module       
     Output is dumped to stdout.
     @param stat A pointer to an ICC_STATUS struct
     @param key DER encoded key
     @param len length of key
     \debug 
     Code: iccGenerateDSA: controlled by KNOWN , used to generate DSA known answer data
*/

static void iccGenerateDSASig(ICC_STATUS *stat,const unsigned char *key, int   len)
{

  DSA *dsa = NULL;
  unsigned char *sig = NULL;
  const unsigned char *tmp = NULL;
  size_t outL = 0;
  EVP_PKEY *pkey = NULL;

  IN();
  tmp = key;
  d2i_DSAPrivateKey(&dsa,&tmp,len);
  sig = (unsigned char *) ICC_Malloc(2048,__FILE__,__LINE__);
  if( NULL != sig) {    
    pkey = EVP_PKEY_new();
    EVP_PKEY_set1_DSA(pkey,dsa);
    outL = 2048; /* Size of the buffer */
    GenerateSig(stat,pkey,sig,&outL,0,"DSA");

    printf("DSA signature of known input %d bytes\n",(int)outL);
    iccPrintBytes(sig,outL);

    EVP_PKEY_free(pkey);
    memset(sig,0,sizeof(2048));
    ICC_Free(sig);
  }
  DSA_free(dsa);
  OUT();
}
/**  
     @brief
     This function is used to generate DSA keys and known answers for the build 
     It is not publicly available and is used as a tool
     to aid in creating the known answer data used in power on self tests.
     It has no impact to the security of the module       
     Output is dumped to stdout.
     @param keyL length of key
     \debug 
     Code: iccGenerateDSA: controlled by KNOWN , used to generate DSA known answer data
*/

static void iccGenerateDSA(ICC_STATUS *stat,int keyL)
{

  DSA *dsa = NULL;
  DH *dh = NULL;
  unsigned char buf1[64]; 
  unsigned char *derB = NULL;
  unsigned char *tmp = NULL;
  int derL = 0;
  int counter = 0;
  unsigned long h = 0;

  IN();
  /* It's keyL in bits, so this is 8x longer than the number of bytes in the key */
  derB = (unsigned char *)ICC_Malloc(keyL,__FILE__,__LINE__);
#if !defined(KNOWN_KEYS)  
  RAND_bytes(buf1,sizeof(buf1));
  dsa = DSA_generate_parameters(keyL,buf1,sizeof(buf1),&counter,&h,NULL,NULL);
  DSA_generate_key(dsa);
  tmp = derB;
  derL = i2d_DSAPrivateKey( dsa, &tmp);
  printf("DSA private key %d bits\n",keyL);
  iccPrintBytes(derB,derL);
#else
  tmp = DSA_key;
  derL = sizeof(DSA_key);
  d2i_DSAPrivateKey(&dsa,&tmp,derL);
#endif

  iccGenerateDSASig(stat,derB,derL);

  DSA_free(dsa);
  ICC_Free(derB);
  OUT();
}

/**  
     @brief
     This function is used to generate hash known answers for the build 
     It is not publicly available and is used as a tool
     to aid in creating the known answer data used in power on self tests.
     It has no impact to the security of the module       
     Output is dumped to stdout.
     @param hash the OpenSSL has name to use
     \debug 
     Code: iccGenerateHash: controlled by KNOWN , used to generate known answers for hash tests
*/

static void iccGenerateHash (char *hash)
{
  const EVP_MD *md;
  EVP_MD_CTX *md_ctx;
  unsigned char hashbuf[64]; /* Large enough for SHA-512 */
  unsigned int hlen = 0;
  md = EVP_get_digestbyname(hash);
  md_ctx = EVP_MD_CTX_new();
  EVP_DigestInit(md_ctx,md);
  EVP_DigestUpdate(md_ctx,in,sizeof(in));
  EVP_DigestFinal(md_ctx,hashbuf,&hlen);
  EVP_MD_CTX_free(md_ctx);
  printf("Known answer for hash %s\n",hash);
  iccPrintBytes(hashbuf,hlen);
}

/**  
     @brief
     This function is used to generate cmac known answers for the build 
     It is not publicly available and is used as a tool
     to aid in creating the known answer data used in power on self tests.
     It has no impact to the security of the module       
     Output is dumped to stdout.
     @param cmac the OpenSSL cipher to use
     @param key The key data
     @param klen The key length
     \debug 
     Code: iccGenerateCMAC: controlled by KNOWN , used to generate known answers for hash tests
*/
static void iccGenerateCMAC(const char *cmac,unsigned char *key, int klen)
{
  const EVP_CIPHER *cip = NULL;
  CMAC_CTX *cctx = NULL;
  unsigned char cmacbuf[32]; 
  size_t clen = 32;

  cip = EVP_get_cipherbyname(cmac);
  cctx = CMAC_CTX_new();
  CMAC_Init(cctx,key,klen,cip,NULL);
  CMAC_Update(cctx,cmac_ka_data,sizeof(cmac_ka_data));
  CMAC_Final(cctx,cmacbuf,&clen);
  CMAC_CTX_free(cctx);
  printf("Known answer for CMAC %s\n",cmac);
  iccPrintBytes(cmacbuf,(int)clen);
  fflush(stdout);
}


static void iccGenerateHashXOF (char *hash)
{
  const EVP_MD *md;
  EVP_MD_CTX *md_ctx;
  unsigned char hashbuf[128]; /* Larger than SHA-512 */
  unsigned int hlen = sizeof(hashbuf);
  md = EVP_get_digestbyname(hash);
  md_ctx = EVP_MD_CTX_new();
  EVP_DigestInit(md_ctx,md);
  EVP_DigestUpdate(md_ctx,in,sizeof(in));
  EVP_DigestFinalXOF(md_ctx,hashbuf,hlen);
  EVP_MD_CTX_free(md_ctx);
  printf("Known answer for XOF %s\n",hash);
  iccPrintBytes(hashbuf,hlen);
}

/**  
     @brief
     This function is used to generate ECDSA signatures and known answers for the build 
     It is not publicly available and is used as a tool
     to aid in creating the known answer data used in power on self tests.
     It has no impact to the security of the module       
     Output is dumped to stdout.
     @param Der DER encoded key
     @param len length of the key
     @param nid curve nid as ED448.25519 need special case handling
     \debug
     Code: iccGenerateECDSA: controlled by KNOWN , used to generate known answers for ECDSA tests
*/

static void iccGenerateECDSASig(ICC_STATUS *stat,const unsigned char *Der,int len,int nid,const char *msg)
{
  EC_KEY *ec_key = NULL;
  const unsigned char *tmp = NULL;
  unsigned char *sigb = NULL;
  size_t outL = 0;
  EVP_PKEY *pkey = NULL;

  /* Allocate signature buffer */
  sigb = (unsigned char *)ICC_Malloc(1024,__FILE__,__LINE__);

  tmp = Der;
  ec_key = d2i_ECPrivateKey(NULL,&tmp,len);
  pkey = EVP_PKEY_new();
  EVP_PKEY_set1_EC_KEY(pkey,ec_key);
  outL = 1024; /* Size of the buffer */
  GenerateSig(stat,pkey,sigb,&outL,-nid,msg);

  printf("ECDSA signature of hash of known input %d bytes\n",(int)outL);
  iccPrintBytes(sigb,outL);

  EC_KEY_free(ec_key);
  EVP_PKEY_free(pkey);
  ICC_Free(sigb);
}
/**  
     @brief
     This function is used to generate ECDSA keys and known answers for the build 
     It is not publicly available and is used as a tool
     to aid in creating the known answer data used in power on self tests.
     It has no impact to the security of the module       
     Output is dumped to stdout.
     @param curve the OpenSSL curve name to use
     \debug
     Code: iccGenerateECDSA: controlled by KNOWN , used to generate known answers for ECDSA tests
*/

static void iccGenerateECDSA (ICC_STATUS *stat,const char *curve)
{

  EC_KEY *ec_key = NULL;
  int nid = -1;
  unsigned char *derB = NULL;
  unsigned char *tmp = NULL;
  int derL = 0;

  derB = (unsigned char *)ICC_Malloc(2000,__FILE__,__LINE__); /* Long enough to hold the largest DER encoded signatures or EC keys */  


  nid = OBJ_txt2nid((char *)curve);

  ec_key = EC_KEY_new_by_curve_name(nid);
  EC_KEY_generate_key(ec_key); 

  tmp = derB;
  derL = i2d_ECPrivateKey(ec_key, &tmp);
  printf("EC private key curve %s \n",curve);
  iccPrintBytes(derB, derL);


  /* Now dump a signature generated using the created key */
  iccGenerateECDSASig(stat,derB,derL,nid,curve);

  EC_KEY_free(ec_key);
  ICC_Free(derB);
}
#endif

/**
   @brief
   Check a return value from other code against a reference value.
   Set error status as appropriate.
   @param in input buffer
   @param inL length of input
   @param knownAnswer reference data
   @param knownAnswerL length of reference data
   @param icc_stat ICC_STATUS - error condition returned (if any).
   @param file the file name where the error occurred
   @param line the line number where the error occured
   @param mode - algorithm mode being checked
   @param alg  - algorithm being checked
   @return icc_stat->majRC
*/
static int iccCheckKnownAnswer(
			       unsigned char  *in,
			       int            inL,
			       const unsigned char  *knownAnswer,
			       int            knownAnswerL,
			       ICC_STATUS     *icc_stat,
			       const char *file,
			       int line,
			       const char *mode,
			       const char *alg
			       )
{
  int rv = ICC_OK;
  char buf[32];
  IN();
  MARK(mode,alg);
  memset(buf,0,sizeof(buf));
  strncpy(buf,mode,15);
  strncat(buf," ",2);
  strncat(buf,alg,15);
  /* make sure known answer is correct  */
  if (ICC_OK == icc_stat->majRC) {
    if ((NULL != knownAnswer) && (NULL != in)) {
      if (knownAnswerL != inL) {
        rv = SetStatusLn2(NULL, icc_stat, FATAL_ERROR, ICC_LIBRARY_VERIFICATION_FAILED,
                            (char *)ICC_KA_DIFF_LENGTH, buf, file, line);
      }
      else if (memcmp(knownAnswer, in, knownAnswerL) != 0) {
        rv = SetStatusLn2(NULL, icc_stat, FATAL_ERROR, ICC_LIBRARY_VERIFICATION_FAILED,
                            (char *)ICC_KA_DIFF_VALUE, buf, file, line);
      }
    } else {
        rv = SetStatusMem(NULL, icc_stat, __FILE__, __LINE__);
    }
  }
  memset(buf,0,sizeof(buf));
  OUTRC(rv);
  return rv;
}

/* Lifted from OpenSSL, modified some to use available API
  It's a bit different from the other tests as a result
 */
int iccDHTest(ICC_STATUS *icc_stat)
{
  DH *dh = NULL;
  unsigned char *pub_key_bin = NULL;
  int len;
  BIGNUM *p = NULL,*g = NULL,*priv_key = NULL;
  const BIGNUM *pub_key = NULL;
  unsigned char *shared_secret = NULL;
  unsigned char *ptr_p = (unsigned char *)dh_test_2048_p;
  unsigned char ibuf[sizeof(dh_test_2048_p)];

  IN();
  dh = DH_new();

  if (dh == NULL) {
    SetStatusMem(NULL,icc_stat,__FILE__,__LINE__);
  }
  if(ICC_OK == icc_stat->majRC) {
    p = BN_new();
    g = BN_new();
    /*! \induced 162 DH Test: corrupt p */
    if(162 == icc_failure) {
      memcpy(ibuf,dh_test_2048_p,sizeof(dh_test_2048_p));
      ibuf[13] = ~ibuf[13]; 
      ptr_p = ibuf;
    } 
    BN_bin2bn(ptr_p,sizeof(dh_test_2048_p),p);
    BN_bin2bn(dh_test_2048_g,sizeof(dh_test_2048_g),g);
    DH_set0_pqg(dh,p,NULL,g);

    /* note that the private key is much shorter than normally used
       * but still g ** priv_key > p
    */
    priv_key = BN_new();    
    BN_bin2bn(dh_test_2048_priv_key,sizeof(dh_test_2048_priv_key),priv_key);
    pub_key = BN_new();
    BN_bin2bn(dh_test_2048_pub_key, sizeof(dh_test_2048_pub_key), pub_key);

    DH_set0_key(dh, pub_key, priv_key);
  }
  if(ICC_OK == icc_stat->majRC) {
    pub_key = DH_get0_pub_key(dh); 
    len = BN_num_bytes(pub_key);  
    if ((pub_key_bin = OPENSSL_malloc(len)) == NULL) {
      SetStatusMem(NULL,icc_stat,__FILE__,__LINE__);
    }
  }
  if(ICC_OK == icc_stat->majRC) {
    BN_bn2bin(pub_key, pub_key_bin);
    iccCheckKnownAnswer(pub_key_bin,len,dh_test_2048_pub_key,sizeof(dh_test_2048_pub_key),icc_stat,__FILE__,__LINE__,"FFDHE","Public key");
  }

  if(ICC_OK == icc_stat->majRC) {  
    /* Shared secret KAT test */
    len = DH_size(dh);
    if ((shared_secret = OPENSSL_malloc(len)) == NULL) {
      SetStatusMem(NULL,icc_stat,__FILE__,__LINE__);
    }
  }
  if(ICC_OK == icc_stat->majRC) {  
     MARK("iccDHTest", "DH_compute_key");
     if ((len = DH_compute_key(shared_secret,pub_key, dh)) == -1) {
      SetStatusLn(NULL,icc_stat,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,"FFDHE DH_compute_key() failed",__FILE__,__LINE__);
    }
  }
  /*! \induced 163 DH shared secret test: Corrupt shared secret */
  if(163 == icc_failure) {
    shared_secret[13] = ~shared_secret[13];
  }

  iccCheckKnownAnswer(shared_secret,len,dh_test_2048_shared_secret,sizeof(dh_test_2048_shared_secret),icc_stat,__FILE__,__LINE__,"FFDHE","Shared secret");


  if (dh) {
    DH_free(dh);
  }
  if(NULL != shared_secret) {
    OPENSSL_free(shared_secret);
  }
  OPENSSL_free(pub_key_bin);
  OUTRC(icc_stat->majRC);
  return icc_stat->majRC;
}

/** 
 * @brief Known answer tests for the pre-TLS 1.3 KDF
 * @param stat pointer to an ICC_STATUS structure
 * @param digest digest
 * @param secret Secret component
 * @param seclen length of secret
 * @param seed  seed component
 * @param seedlen length of seed
 * @param expected expected output
 * @param explen expected length
 * @return ICC_OK on success, ICC_ERROR otherwise and stat is set
 */
static int iccTestTLS_KDF(ICC_STATUS *stat,const EVP_MD *digest, unsigned char *secret,int seclen,unsigned char *seed,int seedlen,unsigned char *expected,int explen)
{

  unsigned char out[16] = {0,0,0,0,0,0,0,0,0,0,0,00,0,0,0,0};
  EVP_PKEY_CTX *kctx = NULL;
  size_t outlen = 16;
  IN();
  kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_TLS1_PRF,NULL);
  if(NULL != kctx) {
    (void)EVP_PKEY_derive_init(kctx);
    (void)EVP_PKEY_CTX_ctrl(kctx,-1,EVP_PKEY_OP_DERIVE, EVP_PKEY_CTRL_TLS_MD,0,(void *)digest);
    (void)EVP_PKEY_CTX_ctrl(kctx,-1,EVP_PKEY_OP_DERIVE, EVP_PKEY_CTRL_TLS_SECRET,seclen,(void *)secret);
    (void)EVP_PKEY_CTX_ctrl(kctx,-1,EVP_PKEY_OP_DERIVE, EVP_PKEY_CTRL_TLS_SEED,seedlen,(void *)seed);
    (void)EVP_PKEY_derive(kctx, out, &outlen);
    EVP_PKEY_CTX_free(kctx);
    iccCheckKnownAnswer(out,explen,expected,explen,stat,__FILE__,__LINE__,"TLS KDF",""); 
  }
  OUTRC(stat->majRC);       
  return stat->majRC;
}

/**
   @brief
   Known answer tests for SP800-38F Key wrap
   @param stat a pointer to an ICC_STATUS structure
   @param Key the AES key to use
   @param kl length of the AES key (bits) 128,192,256
   @param PT plaintext
   @param ptl length of plaintext
   @param CT known ciphertext
   @param ctl length of known ciphertext
   @param pad Use the padded variant
   @return ICC_OK on success, ICC_ERROR otherwise and stat is set
   @note This tests both wrap and unwrap paths as it wraps the
   key, checks against the known answer, then reverses the transform
   to check that the plaintext is recovered.
*/

static int iccCheckKW(ICC_STATUS *stat,
		      unsigned char *Key,
		      int kl,
		      unsigned char *PT,
		      int ptl,
		      unsigned char *CT,
		      int ctl,
		      int pad
		      )
{
  int rv = ICC_OK;
  unsigned char *tmp = NULL;
  unsigned char *tmp1 = NULL;
  int len = 0;
  SP800_38F_ERR e = SP800_38F_OK;
  IN();
  tmp = (unsigned char *)ICC_Malloc(ctl+16,__FILE__,__LINE__);
  tmp1 = (unsigned char *)ICC_Malloc(ctl+16,__FILE__,__LINE__);
  if(NULL == tmp || NULL == tmp1) {
    rv = SetStatusMem(NULL,stat,__FILE__,__LINE__);
  }
  if(ICC_OK == rv) {
    e = SP800_38F_KW(PT,ptl,tmp,&len,Key,kl,ICC_KW_WRAP | pad);
    if((SP800_38F_OK != e) || (memcmp(CT,tmp,ctl) != 0)) {
      rv = SetStatusLn(NULL,stat,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,"Key wrap test failed",__FILE__,__LINE__);
    }
  }
  if(ICC_OK == rv) {
    e = SP800_38F_KW(tmp,ctl,tmp1,&len,Key,kl,pad);
    if((SP800_38F_OK != e) || (memcmp(PT,tmp1,ptl) != 0)) {
      rv = SetStatusLn(NULL,stat,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,"Key wrap test failed",__FILE__,__LINE__);
    }   
  }
  if(NULL != tmp) {
    memset(tmp,0,ctl+16);
    ICC_Free(tmp);
  }
  if(NULL != tmp1) {
    memset(tmp1,0,ctl+16);
    ICC_Free(tmp1);
  }
  OUTRC(rv);
  return rv;
}

static int iccDSA2KA(ICC_STATUS *status)
{
  int rv = ICC_OK;
  int orv = 0;
  DSA *dsa = NULL;
  DSA_SIG *dsa_sig = NULL;
  unsigned char hash_buf[32];
  unsigned char *sig_buf = NULL;
  unsigned char *ptr = NULL;
  EVP_MD_CTX *md_ctx = NULL;
  char *tmp = NULL;
  BIGNUM *p = NULL;
  BIGNUM *q = NULL;
  BIGNUM *g = NULL;
  BIGNUM *priv_key = NULL;
  BIGNUM *pub_key = NULL;

  int sig_len = 0;

  IN();
  tmp = malloc(2*strlen(DSA2_P));
  if(NULL == tmp) {
    rv = SetStatusMem(NULL,status,(char *)__FILE__,__LINE__);
  }
  if(ICC_OK == rv) {
    dsa = DSA_new();
    if(NULL == dsa) {
      rv = SetStatusMem(NULL,status,(char *)__FILE__,__LINE__);
    }
  }
  if(ICC_OK == rv) {
    /* Key params */
    strncpy(tmp,DSA2_P,strlen(DSA2_P)+1);

    /** \induced 145. ECDSA2, corrupt the DSA parameters */
    if(145 == icc_failure) {
      tmp[5] = (tmp[5] & 0xfe) | (~tmp[5] & 1) ;
    }

    BN_hex2bn(&p,tmp);
    BN_hex2bn(&q,DSA2_Q);
    BN_hex2bn(&g,DSA2_G);
    /* Key */

    BN_hex2bn(&priv_key,DSA2_X);
    strncpy(tmp,DSA2_Y,strlen(DSA2_P)+1);

    /** \induced 146. DSA2, corrupt the DSA public key */
    if(146 == icc_failure) {
      tmp[5] = (tmp[5] & 0xfe) | (~tmp[5] & 1) ;
    } 
    BN_hex2bn(&pub_key,tmp);
    DSA_set0_pqg(dsa,p,q,g);
    DSA_set0_key(dsa,pub_key,priv_key);
   
    md_ctx = EVP_MD_CTX_new();
    if(NULL == md_ctx) {
      rv = SetStatusMem(NULL,status,(char *)__FILE__,__LINE__);
    }
  }
  if(ICC_OK == rv) {
    EVP_DigestInit(md_ctx,EVP_get_digestbyname("SHA256"));
    EVP_DigestUpdate(md_ctx,DSA2_MSG,sizeof(DSA2_MSG));
    EVP_DigestFinal(md_ctx,hash_buf,NULL);
  }
  if(ICC_OK == rv) {
    dsa_sig = DSA_SIG_new();
    if(NULL == dsa_sig) {
      rv = SetStatusMem(NULL,status,(char *)__FILE__,__LINE__);
    }
  }
  /* Signature */
  if(ICC_OK == rv) {
    strncpy(tmp,DSA2_R,strlen(DSA2_R)+1);
    /** \induced 147. DSA, corrupt the signature */
    if(147 == icc_failure) {
      tmp[5] = (tmp[5] & 0xfe) | (~tmp[5] & 1) ;
    }

    BN_hex2bn(&(dsa_sig->r),tmp);
    BN_hex2bn(&(dsa_sig->s),DSA2_S);
   
    sig_len = i2d_DSA_SIG(dsa_sig,NULL);
    sig_buf = ICC_Malloc(sig_len,__FILE__,__LINE__);
    if(NULL != sig_buf) {
      ptr = sig_buf;
      i2d_DSA_SIG(dsa_sig,&ptr);
    } else {
      rv = SetStatusMem(NULL,status,(char *)__FILE__,__LINE__);
    }
  }
  if(ICC_OK == rv) {
    /** \induced 148. DSA2, corrupt the message */
    if(148 == icc_failure) {
      hash_buf[5] = ~hash_buf[5];
    }

    orv = DSA_verify(OBJ_txt2nid("SHA256"),hash_buf,32,sig_buf,sig_len,dsa);
    if(1 != orv) {
      rv = SetStatusLn(NULL,status,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
		       "DSA2 Known answer - message verification failed",__FILE__,__LINE__);
    }
  }
  memset(hash_buf,0,sizeof(hash_buf));
  if(NULL != dsa) {
    DSA_free(dsa);
  }
  if(NULL != dsa_sig) {
    DSA_SIG_free(dsa_sig);
  }
  if(NULL != md_ctx) {
    EVP_MD_CTX_free(md_ctx);
  }
  if(NULL != sig_buf) {
    memset(sig_buf,0,sig_len);
    ICC_Free(sig_buf);
  }
  if(NULL != tmp) {
    memset(tmp,0,2*strlen(DSA2_P));
    ICC_Free(tmp);
  }
  OUTRC(rv);
  return rv;
}
static int iccECDHVerifyKAS(ICC_STATUS *status,
			    EC_POINT *otherp,
			    EC_KEY *mine,
			    unsigned char *shared, 
			    int len
			    )
{
  int rv = ICC_OK;
  unsigned char lclshared[128]; /* > 64 bytes, EC P-521, Allow for -571 */
  IN();
  memset(lclshared,0,sizeof(lclshared));
  ECDH_compute_key(lclshared,len,otherp,mine,NULL);
  /*! \known Test: EC key agreement */
  if(185 == icc_failure) {
    lclshared[0] = ~lclshared[0];
  }
  iccCheckKnownAnswer(lclshared,len,shared,len,status,__FILE__,__LINE__,"ECDH","Key agreement");
  memset(lclshared,0,sizeof(lclshared));
  OUTRC(rv);
  return rv;
}
/**
   @brief
   Creates a digest of the given input data (in)          
   @param iccLib Internal ICC context
   @param in pointer to data to hash
   @param inL length of data to hash
   @param out pointer to the output data buffer
   @param outL pointer to location in which to sture the hash. Must be 'large enough'
   @param alg name of the algorithm "MD5", "SHA1"
   @param icc_stat Error return
*/

static void iccDigest(ICClib *iccLib, unsigned char *in, unsigned inL,
                      unsigned char *out, unsigned *outL, char *alg,
                      ICC_STATUS *icc_stat) {
  EVP_MD *evp_md = NULL;
  EVP_MD_CTX *evp_md_ctx = NULL;
  int evpRC = ICC_OSSL_SUCCESS;

  SetStatusOK(iccLib, icc_stat);

  evp_md = (EVP_MD *)EVP_get_digestbyname(alg);

  /** \induced 10: Digest test, cannot get requested digest
   */
  if (10 == icc_failure) {
    evp_md = NULL;
  }

  if (evp_md == NULL) {
    SetStatusLn2(iccLib, icc_stat, FATAL_ERROR, ICC_INCOMPATIBLE_LIBRARY,
                 ICC_NO_ALG_FOUND, alg, __FILE__, __LINE__);
  }

  if (ICC_OK == icc_stat->majRC) {
    evp_md_ctx = EVP_MD_CTX_new();
  }
  if (ICC_OK == icc_stat->majRC) {
    /** \induced 11. Digest test, memory allocation failure
     */
    if (11 == icc_failure) {
      EVP_MD_CTX_cleanup(evp_md_ctx);
      EVP_MD_CTX_free(evp_md_ctx);
      evp_md_ctx = NULL;
    }
    if (evp_md_ctx == NULL) {
      SetStatusMem(iccLib, icc_stat, __FILE__, __LINE__);
    }
  }

  if (ICC_OK == icc_stat->majRC) {
    /* do the digest */
    evpRC = EVP_DigestInit(evp_md_ctx, evp_md);
    if (1 != evpRC) {
      OpenSSLError(iccLib, icc_stat, __FILE__, __LINE__);
    }
  }
  if (ICC_OK == icc_stat->majRC) {
    /* do the digest                                   */
    evpRC = EVP_DigestUpdate(evp_md_ctx, in, inL);
    if (1 != evpRC) {
      OpenSSLError(iccLib, icc_stat, __FILE__, __LINE__);
    }
  }

  /* finish digest */
  if (ICC_OK == icc_stat->majRC) {
    evpRC = EVP_DigestFinal(evp_md_ctx, (unsigned char *)out, outL);
    if (1 != evpRC) {
      OpenSSLError(iccLib, icc_stat, __FILE__, __LINE__);
    }
  }
  if (evp_md_ctx != NULL) {
    EVP_MD_CTX_free(evp_md_ctx);
  }
}
/**
   @brief
   Creates a digest of the given input data (in)          
   @param iccLib Internal ICC context
   @param in pointer to data to hash
   @param inL length of data to hash
   @param out pointer to the output data buffer
   @param outL pointer to location in which to sture the hash. Must be 'large enough'
   @param alg name of the algorithm "MD5", "SHA1"
   @param icc_stat Error return
*/

static void iccXOF(ICClib *iccLib, unsigned char *in, unsigned inL,
                      unsigned char *out, size_t outL, char *alg,
                      ICC_STATUS *icc_stat) {
  EVP_MD *evp_md = NULL;
  EVP_MD_CTX *evp_md_ctx = NULL;

  int evpRC = ICC_OSSL_SUCCESS;

  SetStatusOK(iccLib, icc_stat);

  evp_md = (EVP_MD *)EVP_get_digestbyname(alg);

  /** \induced 10: Digest test, cannot get requested digest
   */
  if (10 == icc_failure) {
    evp_md = NULL;
  }

  if (evp_md == NULL) {
    SetStatusLn2(iccLib, icc_stat, FATAL_ERROR, ICC_INCOMPATIBLE_LIBRARY,
                 ICC_NO_ALG_FOUND, alg, __FILE__, __LINE__);
  }

  if (ICC_OK == icc_stat->majRC) {
    evp_md_ctx = EVP_MD_CTX_new();
  }

  if (ICC_OK == icc_stat->majRC) {
    /** \induced 11. Shake 128 Digest test, memory allocation failure
     */
    if (11 == icc_failure) {
      EVP_MD_CTX_cleanup(evp_md_ctx);
      EVP_MD_CTX_free(evp_md_ctx);
      evp_md_ctx = NULL;
    }
    if (evp_md_ctx == NULL) {
      SetStatusMem(iccLib, icc_stat, __FILE__, __LINE__);
    }
  }

  if (ICC_OK == icc_stat->majRC) {
    /* do the digest */
    evpRC = EVP_DigestInit(evp_md_ctx, evp_md);
    if (1 != evpRC) {
      OpenSSLError(iccLib, icc_stat, __FILE__, __LINE__);
    }
  }
  if (ICC_OK == icc_stat->majRC) {
    /* do the digest                                   */
    evpRC = EVP_DigestUpdate(evp_md_ctx, in, inL);
    if (1 != evpRC) {
      OpenSSLError(iccLib, icc_stat, __FILE__, __LINE__);
    }
  }

  /* finish digest */
  if (ICC_OK == icc_stat->majRC) {
    evpRC = EVP_DigestFinalXOF(evp_md_ctx, out, outL);
    if (1 != evpRC) {
      OpenSSLError(iccLib, icc_stat, __FILE__, __LINE__);
    }
  }
 
  if (evp_md_ctx != NULL) {
    EVP_MD_CTX_free(evp_md_ctx);
  }
}

/**
   @brief
   Allocate a buffer for encrypted data, just a convenience function.
   @param iccLib Internal ICC context
   @param evp_cipher the cipher we'll be using
   @param clearTextL length of clear text we'll be encrypting
   @param buf a place to store the ponter returned from malloc()
   @return the allocated buffer size
*/
static int iccAllocMemForEncrypt(ICClib *iccLib,
				 const          EVP_CIPHER *evp_cipher,
				 int            clearTextL,
				 void          **buf)
{
  int   outSize;

  /*  allocate memory for encryption */

  outSize = EVP_CIPHER_block_size(evp_cipher);
  outSize = ((clearTextL/outSize)+1)*(outSize+2);

  *buf = (char *)ICC_Malloc(outSize,__FILE__,__LINE__);
  if(NULL == *buf) {
    outSize = 0;
  }
  return outSize;
}
/** 
    @brief
    Performs symmetric encryption of the input data (in) and places
    the encrypted data into encData     
    @param iccLib Internal ICC context
    @param cipher_ctx a cipher context
    @param evp_cipher a cipher type
    @param in input buffer
    @param inL length of input data
    @param key the cipher key
    @param iv the initial value
    @param encData pointer to buffer in which to place encrypted data
    @param encL pointer to place to store length of encrypted data
    @param icc_stat error status
    @param ibuf scratch buffer
    @return icc_stat->majRC
*/
static int iccSymEnc(ICClib *iccLib,
		     EVP_CIPHER_CTX *cipher_ctx,
		     const          EVP_CIPHER *evp_cipher,
		     unsigned char  *in,
		     int            inL,
		     unsigned char  *key,
		     unsigned char  *iv,
		     unsigned char  *encData,
		     int            *encL,
		     ICC_STATUS     *icc_stat,
		     unsigned char *ibuf
		     )
{
  int rv = ICC_OK;
  int evpRC = 1;
  unsigned char *pOut=NULL;
  int outEncL=0;
  int outl=0;

  IN();
  SetStatusOK(iccLib,icc_stat);

  /* initialize for encryption                               */

  evpRC = EVP_EncryptInit( cipher_ctx, evp_cipher, key, iv);
  if( 1 != evpRC ) {
    rv = OpenSSLError(iccLib,icc_stat,__FILE__,__LINE__);
  }

  /** \induced 24. Encryption, memory corruption, known answer input was changed. 
   */
  if( 24 == icc_failure ) {
    /* Avoid the segv from modifying static const data */
    memcpy(ibuf,in,inL);
    in = ibuf;
    in[0] = ~in[0];
  }
    

  if (ICC_OK == rv) {
    pOut = encData;
    evpRC = EVP_EncryptUpdate( cipher_ctx, pOut, &outl, in, inL);
    if( 1 != evpRC ) {
      rv = OpenSSLError(iccLib,icc_stat,__FILE__,__LINE__);
    }   

    /** \induced 25. Encryption, memory corruption, . 
        We alter the output here to induce the failure.
    */
    if( 25 == icc_failure ) {
      pOut[0] = ~pOut[0];
    }


  }

  /* do the encryption                                       */

  if (ICC_OK == rv) { 
    pOut += outl;
    outEncL = outl;
    evpRC = EVP_EncryptFinal( cipher_ctx, pOut, &outl);
    EVP_CIPHER_CTX_cleanup(cipher_ctx);
    *encL = outl + outEncL;
    if( 1 != evpRC ) {
      rv = OpenSSLError(iccLib,icc_stat,__FILE__,__LINE__);
    }   
  }
  OUTRC(rv);
  return rv;
}
/**
   @brief
   Performs symmetric decryption of the input data (encData) and places
   the decrypted data into decData
   @param iccLib Internal ICC context 
   @param cipher_ctx a cipher context
   @param evp_cipher a cipher type
   @param encData input buffer
   @param encL length of input data
   @param key the cipher key
   @param iv the initial value
   @param decData pointer to buffer in which to place decrypted data
   @param decL pointer to place to store length of decrypted data
   @param icc_stat error status
   @return icc_stat->majRC
*/
static int iccSymDec( ICClib         *iccLib,
		      EVP_CIPHER_CTX *cipher_ctx,
		      const EVP_CIPHER *evp_cipher,
		      unsigned char  *encData,
		      int            encL,
		      unsigned char  *key,
		      unsigned char  *iv,
		      unsigned char  *decData,
		      int            *decL,
		      ICC_STATUS     *icc_stat
		      )
{
  int rv = ICC_OK;
  int evpRC = 1;
  unsigned char *pOut=NULL;
  int outDecL=0;
  int outl=0;

  IN();
  SetStatusOK(iccLib,icc_stat);

  /* initialize for decryption                           */

  *decL = 0;

  evpRC = EVP_DecryptInit( cipher_ctx, evp_cipher, key, iv);
    
  if( 1 !=  evpRC ) {
    rv = OpenSSLError(iccLib,icc_stat,__FILE__,__LINE__);
  } 
  /* do decryption                                           */

  if (ICC_OK == rv) { 
    pOut = decData;
    evpRC = EVP_DecryptUpdate( cipher_ctx, pOut, &outl, encData, encL);
    if( 1 != evpRC) {
      rv = OpenSSLError(iccLib,icc_stat,__FILE__,__LINE__);
    }
    /** \induced 31. Decryption, memory corruption, . 
        We altered the output here to induce the failure. Unlike 24 this won't segv
        if const areas aren't writable.
    */
    if( 31 == icc_failure ) {
      pOut[0] = ~pOut[0];
    }
  }

  /* finish decryption                                       */

  if (ICC_OK == rv) { 
    pOut += outl;
    outDecL = outl;
    evpRC = EVP_DecryptFinal( cipher_ctx, pOut, &outl);
    EVP_CIPHER_CTX_cleanup(cipher_ctx);
    *decL = outl + outDecL;
    if( 1 != evpRC) {
      rv = OpenSSLError(iccLib,icc_stat,__FILE__,__LINE__);
    }
  }
  OUTRC(rv);
  return rv;
}
/**
   @brief
   Check for the proper functioning of symmetric cryptographic algorithms 
   @param iccLib internal ICC context
   @param ciphername the name of the algorithm to be tested
   @param in input data buffer
   @param inL length of input data
   @param knownAnswer reference data buffer
   @param knownAnswerL length of reference data
   @param key key to use
   @param iv  initial value
   @param icc_stat error return
   @param ibuf scratch buffer
   @return icc_stat->majRC
*/
static int iccCipherTest(ICClib *iccLib,
			 const char *ciphername,
			 const unsigned char  *in,
			 int   inL,
			 const unsigned char  *knownAnswer,
			 int  knownAnswerL,
			 const unsigned char  *key,
			 const unsigned char  *iv,
			 ICC_STATUS     *icc_stat,
			 unsigned char *ibuf
			 )
{
  EVP_CIPHER_CTX *cipher_ctx = NULL;
  const EVP_CIPHER *evp_cipher = NULL;
  int rv = ICC_OK;
  unsigned char *outEncrypted=NULL;
  unsigned char *outDecrypted=NULL;
  int outEncBL=0;
  int outDecBL=0;
  int outEncL=0;
  int outDecL=0;
  int outl=0;

  IN();

  evp_cipher = EVP_get_cipherbyname(ciphername);
  if (evp_cipher == NULL) {
    rv = SetStatusLn2(iccLib,icc_stat,FATAL_ERROR,ICC_INCOMPATIBLE_LIBRARY,
		      ICC_NO_ALG_FOUND,ciphername,__FILE__,__LINE__); 	    
  }
  if (ICC_OK == icc_stat->majRC) {
    cipher_ctx = EVP_CIPHER_CTX_new();
    if (NULL == cipher_ctx ) {
      rv = SetStatusMem(iccLib,icc_stat,__FILE__,__LINE__);	
    }
  }
  if(ICC_OK == icc_stat->majRC) {
    /*  allocate memory for encryption and decryption          */
    outEncBL = iccAllocMemForEncrypt(iccLib,evp_cipher, inL, (void **)(&outEncrypted));    
    outDecBL = iccAllocMemForEncrypt(iccLib,evp_cipher, inL, (void **)(&outDecrypted));
    if ( (NULL == outEncrypted) || (NULL == outDecrypted)) {
      rv = SetStatusMem(iccLib,icc_stat,__FILE__,__LINE__);
    }
  }

  if (ICC_OK == icc_stat->majRC) {
    rv = iccSymEnc(iccLib, cipher_ctx, evp_cipher, (unsigned char *)in, inL,
		   (unsigned char *)key, (unsigned char *)iv, 
		   outEncrypted, &outEncL, icc_stat,ibuf);
  }
  
  if (ICC_OK == icc_stat->majRC) {
    /*      make sure encrypted data is different from original data */      
    if (outEncL > outEncBL) {
      rv = SetStatusLn2(iccLib,icc_stat,
			FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
			ICC_MEMORY_OVERRUN,ciphername,__FILE__,__LINE__);
    } else {
      outl = iccMin(outEncL, inL);
      if ((outEncrypted == NULL) || (memcmp(in,outEncrypted,outl) == 0)) {
	      rv = SetStatusLn2(iccLib,icc_stat,
			  FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
			  ICC_ENC_DATA_SAME,ciphername,__FILE__,__LINE__);
      }
    }
  }

  if (rv == icc_stat->majRC) {
    /* make sure known answer is correct               */
    rv = iccCheckKnownAnswer(outEncrypted, outEncL, knownAnswer, knownAnswerL, 
			     icc_stat,__FILE__,__LINE__,
			     "cipher",ciphername);
  }
    

  if (ICC_OK == icc_stat->majRC) {
    /* initialize for decryption                       */      
    rv = iccSymDec(iccLib, cipher_ctx, evp_cipher, outEncrypted, outEncL,
		   (unsigned char *)key,(unsigned char *) iv, outDecrypted, 
		   &outDecL, icc_stat);
  }

  /* make sure decrypted data is the same as the original    */

  if (ICC_OK == icc_stat->majRC) {
    if (outDecL > outDecBL) {
      rv = SetStatusLn2(iccLib,icc_stat,
			ICC_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
			ICC_MEMORY_OVERRUN,ciphername,__FILE__,__LINE__);
    } else {
      /* make sure known answer is correct           */
      rv = iccCheckKnownAnswer(outDecrypted, outDecL, in, inL, 
			       icc_stat,__FILE__,__LINE__,
			       "cipher",ciphername
			       );
    }
  }

  /*  free allocated buffers                                 */
    
  if (outEncrypted != NULL) {
    memset(outEncrypted,0,outEncBL);
    ICC_Free(outEncrypted);
  }
  if (outDecrypted != NULL) {
    memset(outDecrypted,0,outDecBL);
    ICC_Free(outDecrypted);
  }
  if(NULL != cipher_ctx) {
    EVP_CIPHER_CTX_cleanup(cipher_ctx);
    EVP_CIPHER_CTX_free(cipher_ctx);
  }
  OUTRC(rv);
  return rv;
}
/*! @brief Validate the algorithms that verify the RNG health
  @param iccLib an ICC library handle
  @param icc_stat an ICC_STATUS pointer
  @return ICC_OK or failure
*/
static void RNGAlgTests(ICClib *iccLib,ICC_STATUS *icc_stat) 
{
  IN();
 
  /* NRBG algorithm tests. pmax, AP, RC */
  if(ICC_OK == icc_stat->majRC) {
    if(0 != pmax4Tests() ) {
      SetStatusLn2(iccLib,icc_stat,
			    FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
			    "Self test failed","NRBG algorithm: minimum entropy",__FILE__,__LINE__);
    }
  }
  if(ICC_OK == icc_stat->majRC) {  
    if(0 != APTests() ) {
      SetStatusLn2(iccLib,icc_stat,
			    FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
			    "Self test failed","NRBG algorithm: Adaptive Proportion",__FILE__,__LINE__);
    }
  }
  if(ICC_OK == icc_stat->majRC) {
    if(0 != RCTests() ) {
        SetStatusLn2(iccLib,icc_stat,
			    FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
			    "Self test failed","NRBG algorithm: Repeat Count",__FILE__,__LINE__);      
    }
  }
  
  OUT();
}

#if defined(KNOWN)
/**
   @brief 
   Utility function to assist in generating data used in self tests
   Generates reference data for known-answer tests, 
   Data output is to stdout.
   @param iccLib internal ICC context
   @param cipher_ctx cipher context
   @param evp_cipher cipher type
   @param in input data buffer
   @param inL length of input data
   @param key key to use
   @param iv  initial value
   @param icc_stat error return
*/
static void iccGenCipherKA(ICClib         *iccLib,
			   EVP_CIPHER_CTX *cipher_ctx,
			   const          EVP_CIPHER *evp_cipher,
			   unsigned char  *in,
			   int            inL,
			   unsigned char  *key,
			   unsigned char  *iv,
			   ICC_STATUS     *icc_stat,
			   unsigned char  *ibuf
			   )
{
  unsigned char *outEncrypted=NULL;
  int outEncL=0;

  SetStatusOK(iccLib,icc_stat);
  /*  allocate memory for encryption and decryption          */

  outEncL = iccAllocMemForEncrypt(iccLib,evp_cipher, inL, (void **)(&outEncrypted));

  iccSymEnc(iccLib, cipher_ctx, evp_cipher, in, inL,
	    key, iv, outEncrypted, &outEncL, icc_stat,ibuf);

#if defined(DEBUG_VERBOSE)
  if (ICC_OK == icc_stat->majRC) {
    if (outEncL != 0) {
      printf("Generated Known Answer:\n");
      iccPrintBytes(outEncrypted, outEncL);
    }
  }
#endif
  ICC_Free(outEncrypted);
  return;
}
#endif


/** @brief Consistancy test on a generated DSA key pair 
    We do a sign/verify operation on the same input
    to test consistancy between the keys.
    This is the NIST key pair consistancy check
    @param iccLib internal ICC context
    @param dsa key PAIR. Both public and private keys are generated 
    in the same DSA structure.
    @return ICC_OK if the test passed. ICC_ERROR if it didn't.
    @note this is called when a new DSA key is created
    \FIPS DSA key consistancy continuous test
*/
int iccDSAPairTest(ICClib *iccLib, DSA *dsa)
{
  unsigned char *sig = NULL;
  int rv = ICC_ERROR;
   size_t outL = 0;
  EVP_PKEY *pkey = NULL;
  EVP_MD_CTX *md_ctx = NULL;
  EVP_PKEY_CTX *pctx = NULL;
  const EVP_MD *md = NULL;

  sig = (unsigned char *) ICC_Malloc(2048,__FILE__,__LINE__);
  if( NULL != sig) {    
    pkey = EVP_PKEY_new();
    md_ctx = EVP_MD_CTX_new();
    md = EVP_get_digestbyname("SHA256");
    EVP_PKEY_set1_DSA(pkey,dsa);
    EVP_DigestSignInit(md_ctx,&pctx,md,NULL,pkey);
    EVP_SignUpdate(md_ctx,in,sizeof(in));
    outL = 2048; /* Size of the buffer */
    rv = EVP_DigestSignFinal(md_ctx,sig,&outL);
    EVP_MD_CTX_cleanup(md_ctx);
    pctx = NULL;
    if( 71 == icc_failure ) {
	    sig[0] = ~sig[0];
    }
    rv = EVP_DigestVerifyInit(md_ctx,&pctx,md,NULL,pkey);
    rv = EVP_SignUpdate(md_ctx,in,sizeof(in));
    rv = EVP_DigestVerifyFinal(md_ctx,sig,outL);
    if(1 != rv) {
      /*  disable ICC when an error doing the known answer        */
      SetFatalError("DSA key consistency test failed",__FILE__,__LINE__);
      rv = ICC_ERROR;
    } else {
      rv = ICC_OK;
    }
    
    memset(sig,0,sizeof(2048));
    ICC_Free(sig);
    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);
  }

  return rv;
}

/**
   @brief Continuous EC key pair test 
   We do a sign/verify operation on the same input to test consistancy between the keys.
   This is the NIST key pair consistancy check
   @param iccLib internal ICC context
   @param eckey key PAIR. Both public and private keys are generated in the same EC_KEY structure.
   @return ICC_OK if the test passed. ICC_ERROR if it didn't.
   @note This is called when a new EC key is created
   \known Continuous Test: ECC key consistancy
*/
int iccECKEYPairTest(ICClib *iccLib, EC_KEY *eckey)
{
  	
  unsigned char *sig = NULL;
  int rv = ICC_ERROR;
  size_t outL = 0;
  EVP_PKEY *pkey = NULL;
  EVP_MD_CTX *md_ctx = NULL;
  EVP_PKEY_CTX *pctx = NULL;
  const EVP_MD *md = NULL;

  sig = (unsigned char *) ICC_Malloc(1024,__FILE__,__LINE__);
  if( NULL != sig) {    
    pkey = EVP_PKEY_new();
    md_ctx = EVP_MD_CTX_new();
    md = EVP_get_digestbyname("SHA256");
    EVP_PKEY_set1_EC_KEY(pkey,eckey);
    EVP_DigestSignInit(md_ctx,&pctx,md,NULL,pkey);
    EVP_SignUpdate(md_ctx,in,sizeof(in));
    outL = 1024; /* Size of the buffer */
    rv = EVP_DigestSignFinal(md_ctx,sig,&outL);
    EVP_MD_CTX_cleanup(md_ctx);
    pctx = NULL;
    if( 81 == icc_failure ) {
	    sig[0] = ~sig[0];
    }
    EVP_DigestVerifyInit(md_ctx,&pctx,md,NULL,pkey);
    EVP_SignUpdate(md_ctx,in,sizeof(in));
    rv = EVP_DigestVerifyFinal(md_ctx,sig,outL);
    if(1 != rv) {
      /*  disable ICC when an error doing the known answer        */
      SetFatalError("EC key consistency test failed",__FILE__,__LINE__);
      rv = ICC_ERROR;
    } else {
      rv = ICC_OK;
    }
    memset(sig,0,1024);
    ICC_Free(sig);
    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);
  }  


  return rv;
}
/** @brief NIST internal key consistancy check for RSA keys
    @param iccLib ICC internal context
    @param rsa to verify. Newly generated key pair - contains both public and private keys
    @return ICC_OK or ICC_ERROR
    @note This is called when a new RSA key is created.
    - We don't know whether the key will be used for RSA sign/verify
    or Encrypt/Decrypt so both consistancy tests are needed.
    \FIPS RSA key consistancy continuous test
    @note We only lock the API if the consistancy test can be run
     - we don't hard fail and lock the API on apparent out of memory errors or NULL rsa keys, just return an error
*/  
int iccRSAKeyPair(ICClib *iccLib, RSA* rsa)
{
  static const unsigned char tbs[] = "ICC RSA Pairwise Check Data";
  unsigned char *sig = NULL;
  unsigned char *ptbuf = NULL;
  int rv = ICC_OK; /* ICC state */
  int rc = 0; /* OpenSSL errors */
  size_t outL = 0;
  EVP_PKEY *pkey = NULL;
  EVP_MD_CTX *md_ctx = NULL;
  EVP_PKEY_CTX *pctx = NULL;
  const EVP_MD *md = NULL;
  int len = 0;
  int Keylen = 0;

  IN();
  /* We can get passed garbage, don't lock the API on stuff that's just broken, 
    condition can be triggered by FIPS tests 
    */
  if (NULL != rsa && (0 != (Keylen = RSA_size(rsa)))) {
    sig = (unsigned char *)ICC_Malloc(Keylen*2, __FILE__, __LINE__); /* Malloc is expensive, allocate space for both buffers, 16k key max */
    pkey = EVP_PKEY_new();
    md_ctx = EVP_MD_CTX_new();
    if ((NULL != sig) && (NULL != pkey) && (NULL != md_ctx))
    {
      outL = 2048;
      md = EVP_get_digestbyname("SHA256");
      EVP_PKEY_set1_RSA(pkey, rsa);
      rc = EVP_DigestSignInit(md_ctx, &pctx, md, NULL, pkey);
      rc = EVP_SignUpdate(md_ctx, in, sizeof(in));
      rc = EVP_DigestSignFinal(md_ctx, sig, &outL);
      EVP_MD_CTX_cleanup(md_ctx);
      pctx = NULL;
      /** \known Code: Trip a failure in the RSA key consistency test (Sign/Verify) */
      if (91 == icc_failure)
      {
        sig[0] = ~sig[0];
      }
      rc = EVP_DigestVerifyInit(md_ctx, &pctx, md, NULL, pkey);
      rc = EVP_SignUpdate(md_ctx, in, sizeof(in));
      rc = EVP_DigestVerifyFinal(md_ctx, sig, outL);
      if (1 != rc)
      {
        /*  disable ICC when we get an error doing the consistency test */
        SetFatalError("RSA key consistency test failed (Sign/verify)", __FILE__, __LINE__);
        rv = ICC_ERROR;
      }
      /* Now the encrypt/decrypt test. Can reuse the same buffer */
      if (ICC_OK == rv)
      {
        len = RSA_public_encrypt(sizeof(tbs) - 1, tbs, sig, rsa,
                                 RSA_PKCS1_PADDING);
        if (len <= 0)
        {
          SetFatalError("RSA key consistency test failed (public encrypt)", __FILE__, __LINE__);
          rv = ICC_ERROR;
        }
      }
      if (ICC_OK == rv)
      {
        /** \known Code: Trip a failure in the RSA key consistency test (Encrypt/Decrypt) */
        if (92 == icc_failure)
        {
          sig[0] = ~sig[0];
        }
        ptbuf = sig + Keylen; /* Avoiding the extra malloc, the decrypt buffer is at the end */
        len = RSA_private_decrypt(len, sig, ptbuf, rsa, RSA_PKCS1_PADDING);
        if ( (len <= 0) || 0 != memcmp(ptbuf, tbs, len))
        {
          SetFatalError("RSA key consistency test failed (private decrypt)", __FILE__, __LINE__);
          rv = ICC_ERROR;
        }
      }
    }
    else
    {
      rv = ICC_ERROR;
    }
  }
 
  if(NULL != sig) {
    memset(sig,0,outL);
    ICC_Free(sig);
  }
  if(NULL != md_ctx) {
    EVP_MD_CTX_free(md_ctx);
  }
  if(NULL != pkey) {
    EVP_PKEY_free(pkey);
  }

  OUTRC(rv);
  return rv;
}

/** @brief Known answer check for asymetric keys
    @param stat A pointer to an ICC_STATUS struct
    @param pkey key to verify. (which could be RSA,DSA,EC)
    @param sig provided signature
    @param outL length of signature
    @param flags padding mode for RSA (PSS uses random data)
    @param msg message to sign
    @param error force an error if set (induced failures)
    @return ICC_OK or ICC_ERROR
    @note  We verify sig, create a new signature, then verify that.
    Logic being, we've confirmed the verify path works, so sign then verify also 
    confirms that sign works. This is what NIST SHOULD have specified. 
    Breaking the RNG in security code is a no-no. 
*/ 
static int KATest(ICC_STATUS *stat,EVP_PKEY *pkey, const unsigned char *sig,size_t outL,unsigned int flags,const char *msg,int error)
{ 

  unsigned char *isig = NULL;
  size_t ioutL = outL *2;
  IN();
  isig = (unsigned char *)ICC_Malloc(ioutL,__FILE__,__LINE__);
  MARK("Sign/Verify KA",msg);
  if(NULL != isig) { 
    /* Sane path, verify KA, sign, verufy new sig. Both paths work */
    VerifySig(stat,pkey,sig,outL,flags,msg,error);
    if(ICC_OK == stat->majRC) {
      GenerateSig(stat,pkey,isig,&ioutL,flags,msg);
      if( ICC_OK == stat->majRC) {
        VerifySig(stat,pkey,isig,ioutL,flags,msg,error);
      } 
    } 
    ICC_Free(isig);
  } else {
    SetStatusMem(NULL,stat,__FILE__,__LINE__);
  }
  if(ICC_OK != stat->majRC) {
    SetFatalError("Known answer test on signature failed",__FILE__,__LINE__);
  }
  OUTRC(stat->majRC);
  return stat->majRC;

}
/** @brief Known answer check for asymetric keys
    @param stat A pointer to an ICC_STATUS struct
    @param pkey key to verify. (which could be RSA,DSA,EC)
    @param sig provided signature
    @param outL length of signature
    @param flags padding mode for RSA (PSS uses random data)
    @param msg message to sign
    @param error expect error if !0
    @return ICC_OK or ICC_ERROR
    @note  This is only usable with a broken RNG.
    We verify sig, then sign and check we get the same answer
    
*/ 
static int KATest_broken(ICC_STATUS *stat,EVP_PKEY *pkey, const unsigned char *sig,size_t outL,unsigned int flags,const char *msg,int error)
{ 
  unsigned char *isig = NULL;
  size_t ioutL = outL *2;
  IN();
  isig = (unsigned char *)ICC_Malloc(ioutL,__FILE__,__LINE__);
  MARK("Sign/Verify KA with broken RNG",msg);
  if(NULL != isig) { 
    /* Relies on a kneecapped RNG */
    VerifySig(stat,pkey,sig,outL,flags,msg,error);
    if(0 == stat->majRC) {
      GenerateSig(stat,pkey,isig,&ioutL,flags,msg);
      if(0 != memcmp(sig,isig,outL) ) {
        SetStatusLn2(NULL,stat,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,msg,"Known answer tests on signature failed, signatures don't match",__FILE__,__LINE__);
      }
    }
    ICC_Free(isig);
  }
  if(ICC_OK != stat->majRC) {
    SetFatalError("Known answer test on signature failed",__FILE__,__LINE__);
  }
  OUTRC(stat->majRC);
  return stat->majRC;
}
/** @brief check the RSA implementation for proper functioning    
    encrypt/decrypt known data and check for known results
    at intermedate steps.
    @param iccLib internal ICC context
    @param rsa rsa key
    @param padding padding type
    @param in input data
    @param inL input data length
    @param privKAB buffer for private key encrypt output
    @param privKAL length of buffer for private key encrypt output
    @param pubKAB  buffer for public key decrypt output
    @param pubKAL  length of buffer for public key decrypt output
    @param icc_stat error status  
    \known RSA encrypt/decrypt
*/

static void iccRSACipherTest(ICClib *iccLib, RSA *rsa, int padding,
                             const unsigned char *in, int inL,
                             const unsigned char *privKAB, int privKAL,
                             const unsigned char *pubKAB, int pubKAL,
                             ICC_STATUS *icc_stat) {
  unsigned char *outEncrypted = NULL;
  unsigned char *outDecrypted = NULL;
  int outEncL = 0;
  int outDecL = 0;
  int evpRC = 1;
  RSA *rsaDup = NULL;
  int rv = ICC_OK;
  IN();
  outEncrypted = (unsigned char *)ICC_Malloc(SCRATCH_SIZE, __FILE__, __LINE__);
  outDecrypted = (unsigned char *)ICC_Malloc(SCRATCH_SIZE, __FILE__, __LINE__);

  SetStatusOK(iccLib, icc_stat);

  if ((NULL == outEncrypted) || (NULL == outEncrypted)) {
    SetStatusMem(iccLib, icc_stat, (char *)__FILE__, __LINE__);
    rv = ICC_ERROR;
  }

  rsaDup = RSAPrivateKey_dup(rsa);
  if (rsaDup == NULL) {
    OpenSSLError(iccLib, icc_stat, __FILE__, __LINE__);
    rv = ICC_ERROR;
  }

  if (ICC_ERROR != rv) {
    evpRC = RSA_private_encrypt(inL, in, outEncrypted, rsa, padding);
    if (evpRC < 0) {
      OpenSSLError(iccLib, icc_stat, __FILE__, __LINE__);
      rv = ICC_ERROR;
    }
  }
#if defined(KNOWN)
  printf("RSA Encrypt known answer\n");
  iccPrintBytes(outEncrypted, evpRC);
#endif
  if (ICC_ERROR != rv) {
    /** \induced 53: RSA public Encrypt. Corrupt output of encrypt
     */
    if (53 == icc_failure) {
      outEncrypted[3] = ~outEncrypted[3];
    }
    outEncL = evpRC;
    if (outEncL > SCRATCH_SIZE) {
      SetStatusLn(iccLib, icc_stat, FATAL_ERROR,
                  ICC_LIBRARY_VERIFICATION_FAILED, ICC_MEMORY_OVERRUN, __FILE__,
                  __LINE__);
      rv = ICC_ERROR;
    } else {
      /* make sure known answer from private encrypt is correct  */
      rv =
	        iccCheckKnownAnswer(outEncrypted, outEncL, privKAB, privKAL, icc_stat,
			    __FILE__, __LINE__, "RSA", "Private encrypt");
    }
  }
  if (ICC_ERROR != rv) {
    /* initialize for decryption               */
    evpRC =
      RSA_public_decrypt(outEncL, outEncrypted, outDecrypted, rsa, padding);
    /* make sure decrypted data is the same as the original*/
    if (evpRC < 0) {
      OpenSSLError(iccLib, icc_stat, __FILE__, __LINE__);
      rv = ICC_ERROR;
    }
    /** \induced 54: RSA public decrypt. Corrupt output of decrypt
     */
    if (54 == icc_failure) {
      outDecrypted[3] = ~outDecrypted[3];
    }
    if (ICC_OK == icc_stat->majRC) {
      outDecL = evpRC;
      if (outDecL > SCRATCH_SIZE) {
        SetStatusLn(iccLib, icc_stat, FATAL_ERROR,
                    ICC_LIBRARY_VERIFICATION_FAILED, ICC_MEMORY_OVERRUN,
                    __FILE__, __LINE__);
        rv = ICC_ERROR;
      } else {
        /* make sure decrypted output == input */
        rv = iccCheckKnownAnswer(outDecrypted, outDecL, in, inL, icc_stat,
                                 __FILE__, __LINE__, "RSA", "Public decrypt");
      }
    }
  }

  if (ICC_ERROR != rv) {
    evpRC = RSA_public_encrypt(inL, (unsigned char *)in, outEncrypted, rsaDup,
                               padding);
    if (evpRC < 0) {
      OpenSSLError(iccLib, icc_stat, __FILE__, __LINE__);
      rv = ICC_ERROR;
    }
    /** \induced 55: RSA public encrypt. Corrupt output of encrypt
     */
    if (55 == icc_failure) {
      outEncrypted[3] = ~outEncrypted[3];
    }
    /** \induced 57: RSA public encrypt. make output == input
     */
    if (57 == icc_failure) {
      memcpy(outEncrypted, in, inL);
    }
    if (evpRC >= 0) {
      outEncL = evpRC;
      if (outEncL > SCRATCH_SIZE) {
        SetStatusLn(iccLib, icc_stat, FATAL_ERROR,
                    ICC_LIBRARY_VERIFICATION_FAILED, ICC_MEMORY_OVERRUN,
                    __FILE__, __LINE__);
        rv = ICC_ERROR;
      } else {
        /* make sure known answer from public encrypt is not the
	   same as the input*/
        if (0 == memcmp(in, outEncrypted, inL)) {
          SetStatusLn(iccLib, icc_stat, FATAL_ERROR,
                      ICC_LIBRARY_VERIFICATION_FAILED, ICC_ENC_DATA_SAME,
                      __FILE__, __LINE__);
          rv = ICC_ERROR;
        }
      }
    }

    if (ICC_ERROR != rv) {
      /* initialize for decryption                 */
      if (ICC_OK == icc_stat->majRC) {
        evpRC = RSA_private_decrypt(outEncL, outEncrypted, outDecrypted, rsaDup,
                                    padding);
        if (evpRC < 0) {
          OpenSSLError(iccLib, icc_stat, __FILE__, __LINE__);
          rv = ICC_ERROR;
        }
      }
    }

    if (ICC_ERROR != rv) {
      /** \induced 56: RSA private decrypt. Corrupt output of decrypt
       */
      if (56 == icc_failure) {
        outDecrypted[3] = ~outDecrypted[3];
      }
      if (evpRC >= 0) {
        outDecL = evpRC;
        if (outDecL > SCRATCH_SIZE) {
          SetStatusLn(iccLib, icc_stat, FATAL_ERROR,
                      ICC_LIBRARY_VERIFICATION_FAILED, ICC_MEMORY_OVERRUN,
                      __FILE__, __LINE__);
        } else {
          /* make sure decrypted output == input*/
          iccCheckKnownAnswer(outDecrypted, outDecL, in, inL, icc_stat,
                              __FILE__, __LINE__, "RSA", "Private decrypt");
        }
      }
    }
  }

  if (rsaDup != NULL)
    RSA_free(rsaDup);
  if (outDecrypted != NULL) {
    memset(outDecrypted,0,SCRATCH_SIZE);
    ICC_Free(outDecrypted);
  }
  if (outEncrypted != NULL) {
    memset(outEncrypted,0,SCRATCH_SIZE);
    ICC_Free(outEncrypted);
  }
  OUT();
  return;
}



/** @brief NIST internal key consistancy check for HMAC
    @param iccLib ICC internal context
    @param icc_stat error status  
    @param Key the HMAC key data
    @param keylen The length of the hmac key
    @param hashname The name of the hash to use i.e. "SHA1"
    @param Data the input data
    @param datalen The length of the input data
    @param Expected A pointer to the expected HMAC
    @param explen   The length of the expected HMAC
    @param ibuf scratch buffer
    \known Test: HMAC core test routine
*/  
static void iccHMACTest(ICClib *iccLib,
			ICC_STATUS *icc_stat,
			unsigned char * Key,
			int keylen,
			const char *hashname,
			unsigned char * Data,
			int datalen,
			const unsigned char *Expected,
			int explen,
			unsigned char *ibuf
			)
{
  HMAC_CTX *hmac_ctx = NULL;
  const EVP_MD *digest = NULL;
  unsigned char * Result = NULL;
  unsigned int outlen = 0;

  IN();
  Result = (unsigned char *)ICC_Malloc(256,__FILE__,__LINE__);

  if( (NULL == Result) ) {
    SetStatusMem(iccLib,icc_stat,(char *)__FILE__,__LINE__);
  }
  
  digest = EVP_get_digestbyname(hashname);
  if( digest == NULL) {
    SetStatusLn2(iccLib,icc_stat,FATAL_ERROR,ICC_INCOMPATIBLE_LIBRARY,
		 ICC_NO_ALG_FOUND,hashname,__FILE__,__LINE__);
  }
  if(ICC_OK == icc_stat->majRC) {
    hmac_ctx = HMAC_CTX_new();
    if( hmac_ctx == NULL) {
      SetStatusMem(iccLib,icc_stat,__FILE__,__LINE__);
    }
  }
  if(ICC_OK == icc_stat->majRC) {
    my_HMAC_Init(hmac_ctx,Key,keylen,digest);
    /** \induced 101.  HMAC-SHA1 
	Corrupt the known data
    */
    if( (101 == icc_failure) && (0 == strcmp(hashname,"SHA1")) ) {
      memcpy(ibuf,Data,datalen);
      Data = ibuf;
      Data[0] = ~Data[0];
    }
    /** \induced 103.  HMAC-SHA256 
	Corrupt the known data
    */
    if( (103 == icc_failure) && (0 == strcmp(hashname,"SHA256")) ) {
      memcpy(ibuf,Data,datalen);
      Data = ibuf;
      Data[0] = ~Data[0];
    }
   
    /** \induced 109.  HMAC-SHA3-512 
	Corrupt the known data
    */
    if( (109 == icc_failure) && (0 == strcmp(hashname,"SHA3-512")) ) {
      memcpy(ibuf,Data,datalen);
      Data = ibuf;
      Data[0] = ~Data[0];
    }
    HMAC_Update(hmac_ctx,Data,datalen);
    HMAC_Final(hmac_ctx,Result,&outlen);
    HMAC_CTX_free(hmac_ctx);
    /* make sure known answer is correct           */
#if defined(KNOWN)
    printf("HMAC %s known answer expected %d got %d\n",hashname,explen,outlen);
    iccPrintBytes(Result,outlen);
#endif
    iccCheckKnownAnswer(Result,outlen,Expected,explen, icc_stat,
			__FILE__,
			__LINE__,
			"HMAC",
			hashname);

  }
  if( NULL != Result) {
    memset(Result,0,256);
    ICC_Free(Result);
  }
  OUT();
}

/** @brief NIST internal key consistancy check for CMAC (AES-256)
    @param iccLib ICC internal context
    @param icc_stat error status  
    @param Key the CMAC key data
    @param klen the length of the CMAC key
    @param ciphername The name of the hash to use i.e. "AES-256-CBC"
    @param Data the input data
    @param datalen The length of the input data
    @param Expected A pointer to the expected HMAC
    @param explen The length of the expected HMAC
    @param ibuf scratch space
    \known Test: CMAC core test routine
*/  
static void iccCMACTest(ICClib *iccLib,
			ICC_STATUS *icc_stat,
			unsigned char * Key,
      int klen,
			char *ciphername,
      unsigned char *Data,
      int datalen,
			const unsigned char *Expected,
			int explen,
			unsigned char *ibuf
			)
{
  CMAC_CTX *cmac_ctx = NULL;
  const EVP_CIPHER *cipher = NULL;
  unsigned char Result[64];
  size_t outlen = 0;
  IN();
  cipher = EVP_get_cipherbyname(ciphername);
  if( cipher == NULL) {
    SetStatusLn2(iccLib,icc_stat,ICC_ERROR,ICC_INCOMPATIBLE_LIBRARY,
		 ICC_NO_ALG_FOUND,ciphername,__FILE__,__LINE__);
  }

  /** \induced 111.  CMAC 
	  Corrupt the known data
  */
  if( 111 == icc_failure ) { 
    memcpy(ibuf,Data,datalen);
    Data = ibuf;
    ibuf[2] = ~ibuf[2];
  }
  if(ICC_OK == icc_stat->majRC) {
    cmac_ctx = CMAC_CTX_new();
    if( cmac_ctx == NULL) {
      SetStatusMem(iccLib,icc_stat,__FILE__,__LINE__);
    }
  }

  if(ICC_OK == icc_stat->majRC) {
    CMAC_Init(cmac_ctx,Key,klen,cipher,NULL);
    CMAC_Update(cmac_ctx,Data,datalen);
    outlen = explen;
    CMAC_Final(cmac_ctx,Result,&outlen);
    CMAC_CTX_free(cmac_ctx);
    /* make sure known answer is correct           */
#if defined(KNOWN)
    printf("CMAC %s known answer\n",ciphername);
    iccPrintBytes(Result,outlen);
#endif
    iccCheckKnownAnswer(Result,explen, Expected,explen,
			icc_stat,__FILE__,__LINE__,"CMAC",ciphername);
  }
  memset(Result,0,sizeof(Result));
  OUT();
}
/** @brief NIST internal key consistancy check for AES-CCM
    @param iccLib ICC internal context
    @param icc_stat error status  
    @param Key the key data
    @param keylen the length (bytes) of the key
    @param nonce The nonce
    @param nlen the nonce size
    @param aad the additional authentication data
    @param aadlen the length of the aad
    @param Data the input data
    @param datalen The length of the input data
    @param Expected A pointer to the expected ciphertext + tag
    @param explen The length of the expected ciphertext + tag
    @param taglen the length of the hash tag
    @param ibuf scratch buffer
    \known Test: AES-CCM core test routine
*/
static void iccAES_CCMTest(ICClib *iccLib, ICC_STATUS *icc_stat,
                           unsigned char *Key, unsigned int keylen,
                           unsigned char *nonce, unsigned int nlen,
                           unsigned char *aad, unsigned long aadlen,
                           unsigned char *Data, unsigned long datalen,
                           unsigned char *Expected, unsigned long explen,
                           unsigned int taglen, unsigned char *ibuf) {
  int rv = 0;
  unsigned char *out = NULL;
  unsigned char *outd = NULL;
  unsigned long outlen = 0;
  IN();
  out = (unsigned char *)ICC_Malloc(datalen + 64, __FILE__, __LINE__);
  outd = (unsigned char *)ICC_Malloc(datalen + 64, __FILE__, __LINE__);
  if ((out == NULL) || (outd == NULL)) {
    rv = SetStatusMem(iccLib, icc_stat, __FILE__, __LINE__);
  } else {
    /** \induced 121.  AES_CCM
        Corrupt the known data before encrypt
    */
    if (121 == icc_failure) {
      memcpy(ibuf, Data, datalen);
      Data = ibuf;
      Data[0] = ~Data[0];
    }
    AES_CCM_Encrypt(NULL,nonce, nlen, Key, keylen, aad, aadlen, Data, datalen, out,
                    &outlen, taglen);
    /* make sure known answer is correct for encrypt  */
    rv = iccCheckKnownAnswer(Expected, explen, out, outlen, icc_stat, __FILE__,
                             __LINE__, "AES", "CCM-Enc");
    if (ICC_OK == rv) {
      /** \induced 122.  AES_CCM
          Corrupt the data before decrypt
      */
      if (122 == icc_failure) {
        out[0] = ~out[0];
      }
      /** \induced 123.  AES_CCM
          Corrupt the nonce before decrypt
      */
      if (123 == icc_failure) {
        memcpy(ibuf, nonce, nlen);
        nonce = ibuf;
        nonce[0] = ~nonce[0];
      }
      /** \induced 124.  AES_CCM
          Corrupt the aad before decrypt
      */
      if (124 == icc_failure) {
        memcpy(ibuf, aad, aadlen);
        aad = ibuf;
        aad[0] = ~aad[0];
      }
      /** \induced 125.  AES_CCM
          Corrupt the tag before decrypt
      */
      if (125 == icc_failure) {
        out[explen - 1] = ~out[explen - 1];
      }
      rv = AES_CCM_Decrypt(NULL,nonce, nlen, Key, keylen, aad, aadlen, out, explen,
                           outd, &outlen, taglen);
      if (rv != 1) { /* Verification failed */
        SetStatusLn(iccLib, icc_stat, FATAL_ERROR,
                    ICC_LIBRARY_VERIFICATION_FAILED, ICC_MEMORY_OVERRUN,
                    __FILE__, __LINE__);
      } else { /* check for known answer mismatch */
        iccCheckKnownAnswer(Data, datalen, outd, outlen, icc_stat, __FILE__,
                            __LINE__, "AES", "CCM-Dec");
      }
    }
  }
  if (NULL != out) {
    memset(out,0,datalen+64);
    ICC_Free(out);
  }
  if (NULL != outd) {
    memset(outd,0,datalen+64);
    ICC_Free(outd);
  }
  OUT();
}
/** @brief NIST internal key consistancy check for AES-GCM
    @param iccLib ICC internal context
    @param icc_stat error status  
    @param Key the AES_GCM key data
    @param keylen the length (bytes) of the key
    @param nonce The nonce
    @param nlen the nonce size
    @param aad the additional authentication data
    @param aadlen the length of the aad
    @param Data the input data
    @param datalen The length of the input data
    @param Expected A pointer to the expected ciphertext
    @param explen The length of the expected ciphertext
    @param exptag The expected authentication tag
    @param taglen the length of the expected authentication tag
    @param ibuf scratch buffer
    @note This tests over all supported acceleration levels
    \known Test: AES-GCM core test routine
*/  
static void iccAES_GCMTest(ICClib *iccLib,          ICC_STATUS *icc_stat,
			   unsigned char * Key,     unsigned int keylen,
			   unsigned char *nonce,    unsigned long nlen,
			   unsigned char *aad,      unsigned long aadlen,
			   unsigned char *Data,     unsigned long datalen,
			   unsigned char *Expected, unsigned long explen,
			   unsigned char *exptag,   unsigned int taglen,
			   unsigned char *ibuf
			   )
{
  int rv = 0;
  unsigned char *out = NULL;
  unsigned char *outd = NULL;
  unsigned long outlen = 0, outl = 0;
  AES_GCM_CTX *gcm_ctx = NULL;
  unsigned char *tag = NULL;

  IN();
  gcm_ctx = AES_GCM_CTX_new();

  out = (unsigned char *)ICC_Malloc(explen,__FILE__,__LINE__);
  outd = (unsigned char *)ICC_Malloc(explen,__FILE__,__LINE__);
  tag = (unsigned char *)ICC_Malloc(taglen,__FILE__,__LINE__);
  if( (NULL == out) || (outd == NULL) || (NULL == tag)) {
    SetStatusMem(iccLib,icc_stat,__FILE__,__LINE__);
  } else {
    /** \induced 131.  AES_GCM
	Corrupt the known data before encrypt
    */
    if( 131 == icc_failure ) {
      memcpy(ibuf,Data,datalen);
      Data = ibuf;
      Data[0] = ~Data[0];
    } 
    AES_GCM_Init(NULL,gcm_ctx,nonce,nlen,Key,keylen);
    AES_GCM_EncryptUpdate(gcm_ctx,aad,aadlen,Data,datalen,out,&outlen);
    outl = outlen;
    AES_GCM_EncryptFinal(gcm_ctx,out+outlen,&outlen,tag);
    outl += outlen;
    /* make sure known answer is correct for encrypt */
    /* Data*/
    iccCheckKnownAnswer(Expected,explen,out,outl, icc_stat,
			__FILE__,__LINE__,"AES_GCM","Enc DATA");
    /* Authtag */
    iccCheckKnownAnswer(exptag,taglen,tag,taglen, icc_stat,
			__FILE__,__LINE__,"AES-GCM","Enc TAG");

    if (ICC_OK == icc_stat->majRC) {
      /** \induced 132.  AES_GCM
          Corrupt the data before decrypt
      */
      if (132 == icc_failure) {
        out[0] = ~out[0];
      }
      /** \induced 133.  AES_GCM
          Corrupt the nonce before decrypt
      */
      if (133 == icc_failure) {
        memcpy(ibuf, nonce, nlen);
        nonce = ibuf;
        nonce[0] = ~nonce[0];
      }
      /** \induced 134.  AES_GCM
          Corrupt the aad before decrypt
      */
      if (134 == icc_failure) {
        memcpy(ibuf, aad, aadlen);
        aad = ibuf;
        aad[0] = ~aad[0];
      }
      /** \induced 135.  AES_GCM
          Corrupt the tag before decrypt
      */
      if (135 == icc_failure) {
        tag[0] = ~tag[0];
      }

      AES_GCM_Init(NULL,gcm_ctx, nonce, nlen, Key, keylen);
      AES_GCM_DecryptUpdate(gcm_ctx, aad, aadlen, out, explen, outd, &outlen);
      outl = outlen;
      rv = AES_GCM_DecryptFinal(gcm_ctx, outd + outlen, &outlen, tag, 16);
      outl += outlen;
      if (1 != rv) { /* Verification failed */
        SetStatusLn2(iccLib, icc_stat, FATAL_ERROR,
                     ICC_LIBRARY_VERIFICATION_FAILED, ICC_MEMORY_OVERRUN,
                     "AES-GCM", __FILE__, __LINE__);
      } else { /* check for known answer mismatch */
        iccCheckKnownAnswer(Data, datalen, outd, outl, icc_stat, __FILE__,
                            __LINE__, "AES-GCM", "Decrypt");
      }
    }
    if(NULL != gcm_ctx) {
      AES_GCM_CTX_free(gcm_ctx);
      gcm_ctx = NULL;
    }  
    
    if(NULL != out) {
      memset(out,0,explen);
      ICC_Free(out);
      out = NULL;
    }
    if(NULL != outd) {
      memset(outd,0,explen);
      ICC_Free(outd);
      outd = NULL;
    }
    if(NULL != tag) {
      memset(tag,0,taglen);
      ICC_Free(tag);
      tag = NULL;
    }
  }
  OUT();
}
/*! @brief NIST internal key consistancy check for AES-XTS
  @param iccLib ICC internal context
  @param icc_stat error status  
  @param alg the algorithm text (AES-128-XTS/AES-256-XTS)
  @param Key the AES_XTS key data
  @param IV  the IV data
  @param PT the plaintext data
  @param Plen The length of the input data
  @param CT A pointer to the expected ciphertext
  @param Clen The length of the expected ciphertext
  @param ibuf scracth buffer
  @note Acceleration level tests dropped. The underlying 
  OpenSSL now supports asm, which is compact and
  faster than all.
  \known Test: AES-XTS core test routine
*/  
static void iccAES_XTSTest(ICClib *iccLib,          
			   ICC_STATUS *icc_stat,
			   const char *alg,
			   unsigned char *Key,     
			   unsigned char *IV,
			   unsigned char *PT,     
			   int Plen,
			   unsigned char *CT, 
			   int Clen,
			   unsigned char *ibuf
			   )
{

  const EVP_CIPHER *cip = NULL;
  unsigned char *out = NULL;
  unsigned char *outd = NULL;
  unsigned char *Data = NULL;
  int outlen = 0, outl = 0;
  EVP_CIPHER_CTX *xts_ctx = NULL;

  IN();
  out = (unsigned char *)ICC_Malloc(Clen,__FILE__,__LINE__);
  outd = (unsigned char *)ICC_Malloc(Plen,__FILE__,__LINE__);
				       
  if( (NULL == out) || (outd == NULL)) {
    SetStatusMem(iccLib,icc_stat,__FILE__,__LINE__);
  }
  if(ICC_OK == icc_stat->majRC) {
    /** \induced 136.  AES_XTS
	Corrupt the known data before encrypt
    */
    Data = PT;
    if( 136 == icc_failure ) {
      memcpy(ibuf,PT,Plen);
      Data = ibuf;
      Data[0] = ~Data[0];
    } 
    cip = EVP_get_cipherbyname(alg);
    xts_ctx = EVP_CIPHER_CTX_new();
    if((NULL == cip) || (NULL == xts_ctx)) {
      SetStatusLn2(iccLib,icc_stat,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
		   ICC_NO_ALG_FOUND,alg,__FILE__,__LINE__);      
    } 
  }
  outlen = outl =  0;
  if(ICC_OK == icc_stat->majRC) {
    EVP_EncryptInit(xts_ctx,cip,Key,IV);
    EVP_CIPHER_CTX_set_padding(xts_ctx,0);
    EVP_EncryptUpdate(xts_ctx,out,&outl,Data,Plen);
    EVP_EncryptFinal(xts_ctx,(out+outl),&outlen);
    EVP_CIPHER_CTX_cleanup(xts_ctx);
    outl += outlen;
      
    /* make sure known answer is correct for encrypt */
    iccCheckKnownAnswer(CT,Clen,out,outl, icc_stat,
			__FILE__,__LINE__,alg,"Encrypt");
     
  }
  if(ICC_OK == icc_stat->majRC) {
    /** \induced 137.  AES_XTS
	Corrupt the data before decrypt
    */
    if( 137 == icc_failure ) {     
      out[0] = ~out[0];
    }
    outlen = 0;
    EVP_DecryptInit(xts_ctx,cip,Key,IV);
    EVP_CIPHER_CTX_set_padding(xts_ctx,0);
    EVP_DecryptUpdate(xts_ctx,outd,&outl,out,outl);
    EVP_DecryptFinal(xts_ctx,(unsigned char *)(outd+outl),&outlen);
    EVP_CIPHER_CTX_cleanup(xts_ctx);
    outl += outlen;

    iccCheckKnownAnswer(PT,Plen,outd,outl, icc_stat,
			__FILE__,__LINE__,alg,"Decrypt");
      
  }
  if(NULL != xts_ctx) {
    EVP_CIPHER_CTX_free(xts_ctx);
    xts_ctx = NULL;
  }  
  if(NULL != out) {
    memset(out,0,Clen);
    ICC_Free(out);
    out = NULL;
  }
  if(NULL != outd) {
    memset(outd,0,Plen);
    ICC_Free(outd);
    outd = NULL;
  }
  OUT();
}
unsigned char *my_HKDF_Expand(const EVP_MD *evp_md,
                           const unsigned char *prk, size_t prk_len,
                           const unsigned char *info, size_t info_len,
                           unsigned char *okm, size_t okm_len);

 
unsigned char *my_HKDF_Extract(const EVP_MD *evp_md,
                            const unsigned char *salt, size_t salt_len,
                            const unsigned char *key, size_t key_len,
                            unsigned char *prk, size_t *prk_len);

static void iccHKDFTest(ICC_STATUS *status,
  const char *digest,
  const unsigned char *ikm, int keyLen, const unsigned char *salt,int saltLen, const unsigned char *data, int dataLen, 
  const unsigned char *ref_prk, int prkLen, const unsigned char *ref_okm, int okLen)
{
  const EVP_MD *md = NULL;
  unsigned char my_prk[EVP_MAX_MD_SIZE];
  unsigned char *my_okm = NULL;
  size_t my_prkLen = 0;
  IN();
  memset(my_prk,0,EVP_MAX_MD_SIZE);
  my_okm = (unsigned char *)ICC_Calloc(1,okLen,__FILE__,__LINE__);
  if(NULL == my_okm) {
    SetStatusMem(NULL,status,__FILE__,__LINE__);
  }
  md = EVP_get_digestbyname(digest);
  if(NULL == md) {
      SetStatusLn2(NULL,status,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
		   ICC_NO_ALG_FOUND,digest,__FILE__,__LINE__); 
  }
  if(ICC_OK == status->majRC) { 
    my_HKDF_Extract(md,salt,saltLen,ikm,keyLen,my_prk,&my_prkLen);
    iccCheckKnownAnswer((unsigned char *)ref_prk,prkLen,my_prk,(int)my_prkLen,status,
		  	__FILE__,__LINE__,digest,"HKDF_Extract");
  }
  if(status->majRC == ICC_OK) {
    my_HKDF_Expand(md,ref_prk,prkLen,data,dataLen,my_okm,okLen);
    iccCheckKnownAnswer(my_okm,okLen,ref_okm,okLen,status,
			__FILE__,__LINE__,digest,"HKDF_Expand");
  }
  ICC_Free(my_okm);
  OUT();
}

static void iccChaChaPolyTest(ICC_STATUS *status,
  const unsigned char *key,const unsigned char *iv,int ivlen, const unsigned char *aad, int aadlen,
  const unsigned char *pt, int ptlen, const unsigned char *ref_tag, int taglen, const unsigned char *ref_ct,int reflen)
{
  int outl = 0;
  int totl = 0;
  unsigned char *obuf = NULL;
  unsigned char tag[64];
  EVP_CIPHER_CTX *cctx = EVP_CIPHER_CTX_new();
  const EVP_CIPHER *cip = EVP_get_cipherbyname("ChaCha20-Poly1305");

  IN();
  /* Note that the output buffer will contain the tag during decrypt
     so it needs to be large enough to deal with that 
  */ 
  obuf = ICC_Malloc(ptlen+32,__FILE__,__LINE__);
  EVP_CIPHER_CTX_set_flags(cctx, EVP_CIPHER_CTX_FLAG_WRAP_ALLOW);

  EVP_EncryptInit(cctx, cip, key,iv);
  if(0 != ivlen) {
    EVP_CIPHER_CTX_ctrl(cctx, EVP_CTRL_AEAD_SET_IVLEN,ivlen, NULL);
  }
  EVP_CIPHER_CTX_ctrl(cctx, EVP_CTRL_AEAD_SET_TAG, taglen,NULL);
  EVP_CIPHER_CTX_set_padding(cctx,0);
  if(0 != aadlen) {
    EVP_EncryptUpdate(cctx, NULL, &outl,aad, aadlen);
  }
  if( 0 != ptlen) {
    EVP_EncryptUpdate(cctx, obuf, &outl,pt, ptlen);
    totl += outl;
  }
  EVP_EncryptFinal(cctx, (obuf + totl), &outl);
  totl += outl;
  if(taglen > 0) {
    EVP_CIPHER_CTX_ctrl(cctx, EVP_CTRL_AEAD_GET_TAG,taglen, tag);
  }

  iccCheckKnownAnswer(obuf,totl,ref_ct,reflen,status,__FILE__,__LINE__,"chacha-poly1305","ciphertext");
  iccCheckKnownAnswer(tag,taglen,ref_tag,taglen,status,__FILE__,__LINE__,"chacha-poly1305","tag");
  ICC_Free(obuf);
  EVP_CIPHER_CTX_free(cctx);
  OUT();
} 
/*
  @brief run all the SP800-90 RNG self tests on FIPS compliant
  modes during POST
  - In accordance with SP800-90 the self tests are run
  when the SP800-90 PRNG's are instantiated.
  - All we need to do is instantiate a PRNG of each type
  sucessfully.
  - The ICC implementation runs the self test at all strengths 
  for each PRNG type when that type is instantiated so 
  we don't need to loop through at each strength again here.
  @param ctx an ICC library context
  @param status an ICC_STATUS structure
*/

static void iccSP800_90Test(ICClib *ctx,ICC_STATUS *status)
{
  /* This is what we should do - but it may be too slow */
  const char **Fips_list;
  int i = 0;
  PRNG_CTX *pctx = NULL;
  PRNG *prng = NULL;
  SP800_90STATE pstate = SP800_90UNINIT;
 
  char *ptr = NULL;
  IN();
  Fips_list = get_SP800_90FIPS();

  if(NULL == Fips_list[0]) {
    SetStatusLn(ctx,status,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
		"No FIPS RNG instances found",__FILE__,__LINE__);
  }
  
  for(i = 0; (NULL != Fips_list[i]) && (ICC_OK == status->majRC) ; i++) {
    /* The NRBG's test taps don't have deterministic tests */
    if(NULL != strstr(Fips_list[i],"TRNG")) continue;

    prng = get_RNGbyname(Fips_list[i],1);
    if(NULL == prng) {
      SetStatusLn(ctx,status,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
		  "FIPS RNG requested, but was unavailable",__FILE__,__LINE__);
      break;
    }
    if(ICC_OK == status->majRC) {
      pctx = RNG_CTX_new();
      if( NULL == pctx) {
	      SetStatusMem(ctx,status,__FILE__,__LINE__);
      }
    }
    if (ICC_OK == status->majRC) {
      if (SP800_90INIT != RNG_CTX_Init(pctx, prng, NULL, 0, 256, 0)) {
        RNG_CTX_ctrl(pctx, SP800_90_GETLASTERROR, 0, &ptr);
        SetStatusLn2(ctx, status, FATAL_ERROR,
                     ICC_LIBRARY_VERIFICATION_FAILED, "RNG_CTX_Init failed", Fips_list[i] != NULL ? Fips_list[i]:"unknown",
                     __FILE__, __LINE__);
      }
    }
    if (ICC_OK == status->majRC) {
      /* Force the self test to run, we may have done this twice
         but who cares
      */
      RNG_CTX_ctrl(pctx, SP800_90_SELFTEST, 0, &pstate);
      if (SP800_90CRIT == pstate) {
        RNG_CTX_ctrl(pctx, SP800_90_GETLASTERROR, 0, &ptr);
        SetStatusLn2(ctx, status, FATAL_ERROR,
                     ICC_LIBRARY_VERIFICATION_FAILED,"RNG_CTX_ctrl failed",Fips_list[i] != NULL ? Fips_list[i]:"unknown",
                     __FILE__, __LINE__);
      }
    }
    if (ICC_OK == status->majRC) {
      if (SP800_90INIT != RNG_CTX_Init(pctx, prng, NULL, 0, 256, 0)) {
        RNG_CTX_ctrl(pctx, SP800_90_GETLASTERROR, 0, &ptr);
        SetStatusLn2(ctx, status, FATAL_ERROR,
                     ICC_LIBRARY_VERIFICATION_FAILED, "RNG_CTX_ctrl failed",Fips_list[i] != NULL ? Fips_list[i]:"unknown",
                     __FILE__, __LINE__);
      }
    }
    if (NULL != pctx) {
      RNG_CTX_free(pctx);
      pctx = NULL;
    }
  }
  OUT();
}

/*!
  @brief run all the SP800-108 KDF self tests on FIPS compliant
  modes during POST
  - The KDF objects are very simple and contain no state, 
  - Self test is done on the first call to SP800_108_get_KDFbyname() 
  if that suceeds we passed
  @param ctx A ICC library context
  @param status an ICC status structure
*/
static void iccSP800_108Test(ICClib *ctx,ICC_STATUS *status)
{
  const char **Fips_list;
  int i = 0;
  const KDF *kdf = NULL;
  
  IN();
  Fips_list = get_SP800_108FIPS();
  /* Clear the tested state if it wasn't a previous fail so
     the tests will run again
  */  
  SP800_108_clear_tested();
  for(i = 0; NULL != Fips_list[i] ; i++) {
    kdf = SP800_108_get_KDFbyname(ctx,(char *)Fips_list[i]);
    if(NULL == kdf) {      
      SetStatusLn2(ctx,status,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
		   "SP800-108 KDF self test failed for algorithm",Fips_list[i],__FILE__,__LINE__);
      break;
    }
  }
  OUT();
}
static void iccPBKDF2Test(ICC_STATUS *status,
  const char *digest,int iters,
  const char *pwd, int pwdlen,
  const unsigned char *salt,int saltlen, 
  const unsigned char *ref_key,int keylen)
{
  const EVP_MD *md = NULL;
  unsigned char *my_key = NULL;

  IN();
  md = EVP_get_digestbyname(digest);
  if(NULL == md) {
      SetStatusLn2(NULL,status,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
		   ICC_NO_ALG_FOUND,digest,__FILE__,__LINE__); 
  }

  my_key = (unsigned char *)ICC_Calloc(1,keylen,__FILE__,__LINE__);
  if(NULL == my_key) {
    SetStatusMem(NULL,status,__FILE__,__LINE__);
  }

  if(ICC_OK == status->majRC) { 
    PKCS5_PBKDF2_HMAC(pwd,pwdlen,salt,saltlen,iters,md,keylen,my_key);
    iccCheckKnownAnswer((unsigned char *)ref_key,keylen,my_key,keylen,status,
		  	__FILE__,__LINE__,digest,"PKCS5_PBKDF2_HMAC");
  }
  ICC_Free(my_key);
  OUT();
}



/** @brief call OpenSSL's RNG cleanup from ICC
    remove the callback we set
*/
void iccCleanupRNG()
{
  IN();
  /* Call the cleanup function to deallocate any data structures 
    Note that the underlying code cleans up any residual RNG state.
  */
  RAND_cleanup();
  /* Remove the callback 
     Note, removed with 1.1.0g as we get deadlocked.
     This isn't really necessary as are exiting 
     RAND_set_rand_method(NULL);
  */
  OUT();
}

/**  @brief This function inserts the ICC FIPS compliant NRBG/DRBG
     as the default source for all of libcrypt to use                
     This is done by default by ICC.
     @param iccLib internal ICC context
     @param icc_stat  error return 
     @param seedB pointer to a seed value
     @param seedL length of seed buffer
     @return ICC_OSSL_SUCESS or ICC_FAILURE
*/


int iccSetRNG(ICClib *iccLib, ICC_STATUS *icc_stat, void * seedB, int seedL)
{
  int evpRC=1;
  int ret = ICC_OSSL_SUCCESS;
  int rc = 0;
  unsigned char buffer[80];
  const RAND_METHOD *rngICCRand = NULL;

  IN();
  /* Make sure the locks for the PRNG types get set,
     and load the list of FIPS acceptable PRNG types 
  */  
  get_SP800_90FIPS();     

  /* initialize FIPS random functions ... */
  rc = RAND_FIPS_init(seedB, seedL);
  MARK("TRNG ",GetTRNGName());
  MARK("PRNG ",GetPRNGName());
  if (rc != RAND_R_PRNG_OK) {
    ret = ICC_FAILURE;
    SetStatusLn(iccLib,icc_stat,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
		"An error occured when initializing the FIPS PRNG",__FILE__,__LINE__);

  } else {
    rngICCRand = RAND_FIPS();
    if (rngICCRand == NULL) {
      ret = ICC_FAILURE;
      SetStatusLn(iccLib,icc_stat,FATAL_ERROR,ICC_LIBRARY_VERIFICATION_FAILED,
		  "Failed to retrieve the FIPS PRNG implmentation",__FILE__,__LINE__);
	
    } else {
      /*  ... and make it the default */
      /* overwrite the RNG function with our function which adds FIPS logic
        and a thread pool of RBG's for both NRBG (RAND_bytes()) and DRBG (RAND_pseudo_bytes())
        Note that we also patch OpenSSL so that it uses RAND_pseudo_bytes() in appropriate places
        rather than using RAND_bytes() for everything
      */
      evpRC = RAND_set_rand_method( rngICCRand);
      if (evpRC != 1) {
	      OpenSSLError(iccLib,icc_stat,__FILE__,__LINE__); 
      }
      if (ICC_OK == icc_stat->majRC) {
        /** Cycle the system RNG twice */
        MARK("Init RNG", "Read 80 bytes from RNG 1");
        if (ICC_OSSL_SUCCESS != fips_rand_bytes(buffer, 80)) {
          SetStatusLn(NULL, &(Global.status), ICC_ERROR | ICC_FATAL,
                      ICC_LIBRARY_VERIFICATION_FAILED, "RNG failure", __FILE__,
                      __LINE__);
        }
        MARK("Init RNG", "Read 80 bytes from RNG 2");
        if (ICC_OSSL_SUCCESS != fips_rand_bytes(buffer, 80)) {
          SetStatusLn(NULL, &(Global.status), ICC_ERROR | ICC_FATAL,
                      ICC_LIBRARY_VERIFICATION_FAILED, "RNG failure", __FILE__,
                      __LINE__);
        }
      }
    }
  }
  memset(buffer,0,sizeof(buffer));
  OUTRC(ret);
  return(ret);
}

/** @brief These tests are run ONLY at startup (Library Load/POST) , they require known answers for signatures
          unfortunately some algs have a random compronent in the signature generation
          so we have to subvert the RNG to run these

  Later programmatic calls to ICC_SelfTest() will call a different set of tests which Verify(known) then Sign(random), Verify(random)
  We've already checked the Verify path the first time, so if the second sign/verify
  also works we can also assert that Sign was functional

  @param pcb internal ICC context
  @param stat ICC status structure
  @return ICC_OK or ICC_ERROR
*/
static int DoVeryBrokenTests(ICClib *pcb, ICC_STATUS *stat)
{
  RSA *rsa = NULL;
  DSA *dsa = NULL;
  EC_KEY *ec_key = NULL;
  EVP_PKEY *pkey = NULL;
  unsigned char *sig = NULL;
  const unsigned char *tmp = NULL;
  RAND_METHOD *rngICCRand = NULL;
  int runECtest = 1;
  /*return (stat->majRC);*/
  IN();
  rngICCRand = RAND_FIPS();
  MARK("Install broken RNG - required for FIPS compliance","Only used during POST");
  /* Only need to cripple PRNG paths */
  insecure_rand_meth.bytes = rngICCRand->bytes;
  RAND_set_rand_method(&insecure_rand_meth);

#if defined(KNOWN)
  printf("\nKnown answers with a broken RNG\n\n");
  printf("EC_key_P384\n");
  iccGenerateECDSASig(stat,EC_key_P384,sizeof(EC_key_P384),0,"P-384");
  printf("EC_key_B233\n");
  iccGenerateECDSASig(stat,EC_key_B233,sizeof(EC_key_B233),0,"B-233");
  printf("EC_key_K233\n");
  iccGenerateECDSASig(stat,EC_key_K233,sizeof(EC_key_K233),0,"K-233");
  printf("EC_key_X448\n");
  iccGenerateECDSASig(stat,EC_key_X448,sizeof(EC_key_X448),0,"X448");
   printf("EC_key_X25519\n");
  iccGenerateECDSASig(stat,EC_key_X25519,sizeof(EC_key_X448),0,"X25519");
  
  printf("DSA_key\n");
  iccGenerateDSASig(stat,DSA_key,sizeof(DSA_key));
  /*
  printf("RSA_key, PSS, SHA256\n");
  iccGenerateRSASig(stat,RSA_key,sizeof(RSA_key),RSA_PKCS1_PSS_PADDING);
  */
  printf("\nEnd known answers with a broken RNG\n\n");
#endif

  sig = ICC_Malloc(2048,__FILE__,__LINE__);
  if(NULL == sig) {
    SetStatusMem(pcb,stat,__FILE__,__LINE__);
  } else {
    if (ICC_OK == stat->majRC)
    {
      MARK("Broken RNG", "RSA PKCS 1.5");
      /* PKCS 1.5 padding. No random component so we don't need a special sig */
      pkey = EVP_PKEY_new();
      tmp = RSA_key;
      d2i_RSAPrivateKey(&rsa, &tmp, sizeof(RSA_key));
      EVP_PKEY_set1_RSA(pkey, rsa);
      /** \induced 83: Make RSA PKCS1.5 sign/verify known answer test break */
      KATest_broken(stat, pkey, RSA_PKCS_sig, sizeof(RSA_PKCS_sig), RSA_PKCS1_PADDING, "RSA PKCS1.5", 83);
      EVP_PKEY_free(pkey);
      RSA_free(rsa);
      rsa = NULL;
    }
    if (ICC_OK == stat->majRC)
    {
      MARK("Broken RNG", "RSA-PSS (SHA256)");
      /* PSS padding */
      pkey = EVP_PKEY_new();
      tmp = RSA_key;
      d2i_RSAPrivateKey(&rsa, &tmp, sizeof(RSA_key));
      EVP_PKEY_set1_RSA(pkey, rsa);
      /** \induced 84: Make RSA PSS sign/verify known answer test break */
      KATest_broken(stat, pkey, RSA_PSS_sig_broken, sizeof(RSA_PSS_sig_broken), RSA_PKCS1_PSS_PADDING, "RSA-PSS", 84);
      EVP_PKEY_free(pkey);
      RSA_free(rsa);
      rsa = NULL;
    }
    /* Tests that won't pass on Z because the Z hardware paths add their own nonce to EC signatures
      It is fixable, but there's a performance hit, hence this mess
      */

#if defined(__MVS__) || defined(__s390__)
    /* zSeries by default uses an internal nonce for ECDSA sign, that can be disabled
     *  but it's a performance hit. So we only disable that in FIPS mode, problem
     *  then is that the FULL EC KA test can't run 
     *                */
    if(!FIPS_mode()) {
        runECtest = 0;
    }
#endif
    if (runECtest)
    {
      if (ICC_OK == stat->majRC)
      {
        MARK("Broken RNG", "EC P-384");
        /* P-384 */
        pkey = EVP_PKEY_new();
        tmp = EC_key_P384;
        d2i_ECPrivateKey(&ec_key, &tmp, sizeof(EC_key_P384));
        EVP_PKEY_set1_EC_KEY(pkey, ec_key);
        /** \induced 85: Make P-384 sign/verify known answer test break */
        KATest_broken(stat, pkey, EC_sig_P384_broken, sizeof(EC_sig_P384_broken), 0, "EC P384", 85);
        EVP_PKEY_free(pkey);
        EC_KEY_free(ec_key);
        ec_key = NULL;
      }
      if (ICC_OK == stat->majRC)
      {
        MARK("Broken RNG", "EC B-233");
        /* B-233 */
        pkey = EVP_PKEY_new();
        tmp = EC_key_B233;
        d2i_ECPrivateKey(&ec_key, &tmp, sizeof(EC_key_B233));
        EVP_PKEY_set1_EC_KEY(pkey, ec_key);
        /** \induced 86: Make B-233 sign/verify known answer test break */
        KATest_broken(stat, pkey, EC_sig_B233_broken, sizeof(EC_sig_B233_broken), 0, "EC-B-233", 86);
        EVP_PKEY_free(pkey);
        EC_KEY_free(ec_key);
        ec_key = NULL;
      }
      if (ICC_OK == stat->majRC)
      {
        MARK("Broken RNG", "EC K-233");
        /* K-233 */
        pkey = EVP_PKEY_new();
        tmp = EC_key_K233;
        d2i_ECPrivateKey(&ec_key, &tmp, sizeof(EC_key_K233));
        EVP_PKEY_set1_EC_KEY(pkey, ec_key);
        /** \induced 87: Make K-233 sign/verify known answer test break */
        KATest_broken(stat, pkey, EC_sig_K233_broken, sizeof(EC_sig_K233_broken), 0, "EC K-233", 87);
        EVP_PKEY_free(pkey);
        EC_KEY_free(ec_key);
        ec_key = NULL;
      }
    } else if (!runECtest)
    {
      MARK("Full EC KA test skipped, using internal hardware nonce generator when not in FIPS mode", "zSeries only");
    }

    if (ICC_OK == stat->majRC)
    {
      MARK("Broken RNG", "DSA");
      pkey = EVP_PKEY_new();
      tmp = DSA_key;
      d2i_DSAPrivateKey(&dsa, &tmp, sizeof(DSA_key));
      EVP_PKEY_set1_DSA(pkey, dsa);
      /** \induced 88: Make DSA sign/verify known answer test break */
      KATest_broken(stat, pkey, DSA_sig_broken, sizeof(DSA_sig_broken), 0, "DSA", 88);
      EVP_PKEY_free(pkey);
      DSA_free(dsa);
      dsa = NULL;
    }
    ICC_Free(sig);
  }
  MARK("Restore real rng","");
  RAND_set_rand_method( rngICCRand);
  MARK("Poison broken RNG so it can't be used again","");
  memset(&insecure_rand_meth,0,sizeof(insecure_rand_meth));
  OUTRC(stat->majRC);
  return stat->majRC;
}

/* @brief Safe KA tests of key pairs via signatures
  Programmatic calls to ICC_SelfTest() will call this set of tests which Verify(known) then Sign(random), Verify(random)
  We've already checked the Verify path the first time, so if the second sign/verify
  also works we can also assert that Sign was functional
  @param pcb internal ICC context
  @param stat ICC status structure
  @return ICC_OK or ICC_ERROR
*/
static int DoSigTests(ICClib *pcb, ICC_STATUS *stat)
{
  RSA *rsa = NULL;
  DSA *dsa = NULL;
  EC_KEY *ec_key = NULL;
  EVP_PKEY *pkey = NULL;
  unsigned char *sig = NULL;
  const unsigned char *tmp = NULL;
  IN();

  sig = ICC_Malloc(2048,__FILE__,__LINE__);
  if(NULL == sig) {
    SetStatusMem(pcb,stat,__FILE__,__LINE__);
  } else {

    if (ICC_OK == stat->majRC)
    {
      /* PKCS 1.5 padding. No random component so we don't need a special sig */
      pkey = EVP_PKEY_new();
      tmp = RSA_key;
      d2i_RSAPrivateKey(&rsa, &tmp, sizeof(RSA_key));
      EVP_PKEY_set1_RSA(pkey, rsa);
      /** \induced 93: Make RSA PKCS1.5 sign/verify known answer test break */
      KATest(stat, pkey, RSA_PKCS_sig, sizeof(RSA_PKCS_sig), RSA_PKCS1_PADDING, "RSA PKCS1.5", 93);
      EVP_PKEY_free(pkey);
      RSA_free(rsa);
      rsa = NULL;
    }
    if (ICC_OK == stat->majRC)
    {
      /* PSS padding */
      pkey = EVP_PKEY_new();
      tmp = RSA_key;
      d2i_RSAPrivateKey(&rsa, &tmp, sizeof(RSA_key));
      EVP_PKEY_set1_RSA(pkey, rsa);
      /** \induced 94: Make RSA PSS sign/verify known answer test break */
      KATest(stat, pkey, RSA_PSS_sig, sizeof(RSA_PSS_sig), RSA_PKCS1_PSS_PADDING, "RSA-PSS", 94);
      EVP_PKEY_free(pkey);
      RSA_free(rsa);
      rsa = NULL;
    }
    if (ICC_OK == stat->majRC)
    {
      /* P-384 */
      pkey = EVP_PKEY_new();
      tmp = EC_key_P384;
      d2i_ECPrivateKey(&ec_key, &tmp, sizeof(EC_key_P384));
      EVP_PKEY_set1_EC_KEY(pkey, ec_key);
      /** \induced 95: Make P-384 sign/verify known answer test break */
      KATest(stat, pkey, EC_sig_P384, sizeof(EC_sig_P384), 0, "EC P-384", 95);
      EVP_PKEY_free(pkey);
      EC_KEY_free(ec_key);
      ec_key = NULL;
    }
    if (ICC_OK == stat->majRC)
    {

      /* B-233 */
      pkey = EVP_PKEY_new();
      tmp = EC_key_B233;
      d2i_ECPrivateKey(&ec_key, &tmp, sizeof(EC_key_B233));
      EVP_PKEY_set1_EC_KEY(pkey, ec_key);
      /** \induced 96: Make B-233 sign/verify known answer test break */
      KATest(stat, pkey, EC_sig_B233, sizeof(EC_sig_B233), 0, "EC B233", 96);
      EVP_PKEY_free(pkey);
      EC_KEY_free(ec_key);
      ec_key = NULL;
    }
    if (ICC_OK == stat->majRC)
    {

      /* K-233 */
      pkey = EVP_PKEY_new();
      tmp = EC_key_K233;
      d2i_ECPrivateKey(&ec_key, &tmp, sizeof(EC_key_K233));
      EVP_PKEY_set1_EC_KEY(pkey, ec_key);
      /** \induced 97: Make K-233 sign/verify known answer test break */
      KATest(stat, pkey, EC_sig_K233, sizeof(EC_sig_K233), 0, "EC K 233", 97);
      EVP_PKEY_free(pkey);
      EC_KEY_free(ec_key);
      ec_key = NULL;
    }
    if (ICC_OK == stat->majRC)
    {

      pkey = EVP_PKEY_new();
      tmp = DSA_key;
      d2i_DSAPrivateKey(&dsa, &tmp, sizeof(DSA_key));
      EVP_PKEY_set1_DSA(pkey, dsa);
      /** \induced 98: Make DSA sign/verify known answer test break */
      KATest(stat, pkey, DSA_sig, sizeof(DSA_sig), 0, "DSA", 98);
      EVP_PKEY_free(pkey);
      DSA_free(dsa);
      dsa = NULL;
    }
    ICC_Free(sig);
  }
  OUTRC(stat->majRC);
  return stat->majRC;
}
/** @brief Run NIST known answer tests
      - Cycle the system RNG twice
      - SHA224
      - SHA256
      - SHA384
      - SHA512
      - RSA
      -  sign/verify with known keys and outputs
      -  encrypt/decrypt with known keys and outputs
      - DSA
      - verify with known keys and input
      - Sign/Verify with known key
      - ECDSA
      - verify with representative key classes
      - Sign/verify with P-384
      - SHA1 HMAC with known key and output
      - SHA224 HMAC with known key and output
      - SHA256 HMAC with known key and output
      - SHA384 HMAC with known key and output
      - SHA512 HMAC with known key and output
      - AES-256 CMAC with known key and output
      - AES
      - AES-CCM
      - AES-GCM
      - AES-KW
      - NIST algs used in the RNG
      - HKDF
      - TLS 1.x KDF
      - PBKDF2
      @param iccLib ICC internal context
      @param icc_stat error return
      \FIPS Known answer tests are carried out here.
  */
void iccDoKnownAnswer(ICClib * iccLib, ICC_STATUS * icc_stat) {
  unsigned char digest[256];
  unsigned digestL = 0;
  RSA *rsaKey = NULL;
  const unsigned char *p1 = NULL;
  EVP_PKEY *rsaPkey = NULL;
  unsigned char *signature = NULL;
  DSA *dsa = NULL;
  unsigned char *ibuf = NULL;
  ICC_STATUS *mystat = NULL;
  const unsigned char *tmp = NULL;

  IN();
  if (NULL == icc_stat)
  {
    mystat = ICC_Calloc(1, sizeof(ICC_STATUS), __FILE__, __LINE__);
    icc_stat = mystat;
  }
  if (NULL == icc_stat)
  {
    SetStatusMem(iccLib, icc_stat, (char *)__FILE__, __LINE__);
  }
  else
  {

    SetStatusOK(iccLib, icc_stat);
    if (ICC_OK == icc_stat->majRC)
    {
      if (insecure_rand_meth.bytes != NULL)
      {
        DoVeryBrokenTests(iccLib, icc_stat);
      }
      else
      {
        DoSigTests(iccLib, icc_stat);
      }
    }
#if defined(KNOWN)
    GenerateKAData(iccLib, icc_stat);
#endif

    ibuf = (unsigned char *)ICC_Malloc(SCRATCH_SIZE, __FILE__, __LINE__);
    signature = (unsigned char *)ICC_Malloc(SCRATCH_SIZE, __FILE__, __LINE__);

    rsaPkey = EVP_PKEY_new();

    /** \induced 60. Memory allocation failure in self test code. (Out of
   * memory)
   */
    if (60 == icc_failure)
    {
      EVP_PKEY_free(rsaPkey);
      rsaPkey = NULL;
    }

    if ((NULL == signature) || (NULL == rsaPkey) || (NULL == ibuf))
    {
      SetStatusMem(iccLib, icc_stat, (char *)__FILE__, __LINE__);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** Cycle the system RNG twice */
      MARK("SelfTest", "fips_rand_bytes 1");
      if (ICC_OSSL_SUCCESS != fips_rand_bytes(ibuf, 80))
      {
        SetStatusLn(NULL, &(Global.status), ICC_ERROR | ICC_FATAL,
                    ICC_LIBRARY_VERIFICATION_FAILED, "RNG failure", __FILE__,
                    __LINE__);
      }
      MARK("SelfTest", "fips_rand_bytes 2");
      if (ICC_OSSL_SUCCESS != fips_rand_bytes(ibuf, 80))
      {
        SetStatusLn(NULL, &(Global.status), ICC_ERROR | ICC_FATAL,
                    ICC_LIBRARY_VERIFICATION_FAILED, "RNG failure", __FILE__,
                    __LINE__);
      }
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: SHA1 digest test with known input and output
        @note This isn't strictly needed as the library verification test
       also validates this but the time taken is so short that it may as
       well be done anyway.
    */
      iccDigest(iccLib, (unsigned char *)in, sizeof(in), digest, &digestL, "SHA1",
                icc_stat);
      p1 = sha1_ka;
      /* make sure known answer is correct                     */
      /** \induced 12. SHA-1 digest test, wrong known answer
     */
      if (12 == icc_failure)
      {
        memcpy(ibuf, sha1_ka, sizeof(sha1_ka));
        ibuf[sizeof(sha1_ka) - 1] ^= 0x01;
        p1 = (const unsigned char *)ibuf;
      }
      if (ICC_OK == icc_stat->majRC)
      {
        iccCheckKnownAnswer(digest, digestL, p1, sizeof(sha1_ka), icc_stat,
                            __FILE__, __LINE__, "HASH", "SHA1");
      }
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: SHA256 digest test with known input and output */
      iccDigest(iccLib, (unsigned char *)in, sizeof(in), digest, &digestL,
                "SHA256", icc_stat);
      /* make sure known answer is correct                     */
      p1 = sha256_ka;
      /** \induced 14. SHA-256 digest test, wrong known answer
     */
      if (14 == icc_failure)
      {
        memcpy(ibuf, sha256_ka, sizeof(sha256_ka));
        ibuf[sizeof(sha256_ka) - 1] ^= 0x01;
        p1 = (const unsigned char *)ibuf;
      }
      if (ICC_OK == icc_stat->majRC)
      {
        iccCheckKnownAnswer(digest, digestL, p1, sizeof(sha256_ka), icc_stat,
                            __FILE__, __LINE__, "HASH", "SHA256");
      }
    }

    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: SHA512 digest test with known input and output  */
      iccDigest(iccLib, (unsigned char *)in, sizeof(in), digest, &digestL,
                "SHA512", icc_stat);
      /* make sure known answer is correct                     */
      p1 = sha512_ka;
      /** \induced 16. SHA-512 digest test, wrong known answer
     */
      if (16 == icc_failure)
      {
        memcpy(ibuf, sha512_ka, sizeof(sha512_ka));
        ibuf[sizeof(sha512_ka) - 1] ^= 0x01;
        p1 = (const unsigned char *)ibuf;
      }
      if (ICC_OK == icc_stat->majRC)
      {
        iccCheckKnownAnswer(digest, digestL, p1, sizeof(sha512_ka), icc_stat,
                            __FILE__, __LINE__, "HASH", "SHA512");
      }
    }

    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: SHA512 digest test with known input and output  */
      iccDigest(iccLib, (unsigned char *)in, sizeof(in), digest, &digestL,
                "SHA3-512", icc_stat);
      /* make sure known answer is correct                     */
      p1 = sha3_512_ka;
      /** \induced 64. SHA3-512 digest test, wrong known answer
     */
      if (64 == icc_failure)
      {
        memcpy(ibuf, sha3_512_ka, sizeof(sha3_512_ka));
        ibuf[sizeof(sha3_512_ka) - 1] ^= 0x01;
        p1 = (const unsigned char *)ibuf;
      }
      if (ICC_OK == icc_stat->majRC)
      {
        iccCheckKnownAnswer(digest, digestL, p1, sizeof(sha3_512_ka), icc_stat,
                            __FILE__, __LINE__, "HASH", "SHA3-512");
      }
    }

    if (ICC_OK == icc_stat->majRC)
    {
      digestL = 128;
      /** \known Test: SHAKE128 XOF digest test with known input and output  */
      iccXOF(iccLib, (unsigned char *)in, sizeof(in), digest, sizeof(SHAKE128_CT),
             "SHAKE128", icc_stat);
      /* make sure known answer is correct                     */
      p1 = SHAKE128_CT;
      /** \induced 65. SHAKE128 XOF test, wrong known answer
     */
      if (65 == icc_failure)
      {
        memcpy(ibuf, SHAKE128_CT, sizeof(SHAKE128_CT));
        ibuf[sizeof(SHAKE128_CT) - 1] ^= 0x01;
        p1 = (const unsigned char *)ibuf;
      }
      if (ICC_OK == icc_stat->majRC)
      {
        iccCheckKnownAnswer(digest, digestL, p1, sizeof(SHAKE128_CT), icc_stat,
                            __FILE__, __LINE__, "XOF", "SHAKE128");
      }
    }

    /* End SHA3 */

    /** \known Test: RSA encrypt/decrypt test with known keys, inputs and
   * outputs */
    if (ICC_OK == icc_stat->majRC)
    {
      tmp = RSA_key;
      d2i_RSAPrivateKey(&rsaKey, &tmp, sizeof(RSA_key));
      iccRSACipherTest(iccLib, rsaKey, 1, in, sizeof(in), rsa_privK_ka,
                       sizeof(rsa_privK_ka), rsa_pubK_ka, sizeof(rsa_pubK_ka),
                       icc_stat);
    }

    if (ICC_OK == icc_stat->majRC)
    {

      /** \known Test: RSA key pair verification with known (good) keys */
      if (ICC_OK != iccRSAKeyPair(iccLib, rsaKey))
      {
        SetStatusLn(iccLib, icc_stat, FATAL_ERROR,
                    ICC_LIBRARY_VERIFICATION_FAILED,
                    "Verification of RSA key pair failed.", __FILE__, __LINE__);
      }
    }

    if (ICC_OK == icc_stat->majRC)
    {
      dsa = DSA_new();

      p1 = dsa_privK_ka;
      /** \induced 73. DSA key pair consistency test, corrupt key
     */
      if (73 == icc_failure)
      {
        memcpy(ibuf, dsa_privK_ka, sizeof(dsa_privK_ka));
        ibuf[sizeof(dsa_privK_ka) - 1] ^= 0x01;
        p1 = (const unsigned char *)ibuf;
      }
      d2i_DSAPrivateKey(&dsa, &p1, sizeof(dsa_privK_ka));
      /** \known Test: DSA key pair test with known (good) keys */
      if (iccDSAPairTest(iccLib, dsa) != ICC_OK)
      {
        SetStatusLn(iccLib, icc_stat, FATAL_ERROR,
                    ICC_LIBRARY_VERIFICATION_FAILED,
                    "Verification of DSA key pair failed.", __FILE__, __LINE__);
      }
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: DSA verify with known public key and signature
        @note you can't check the signatures themselves, they vary each time
    */
      if (1 != DSA_verify(0, in, 20, dsa_sig_ka, sizeof(dsa_sig_ka), dsa))
      {
        OpenSSLError(iccLib, icc_stat, __FILE__, __LINE__);
      }
    }

    DSA_free(dsa);
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: SHA1-HMAC test with known key, input and output */
      p1 = hmac_ka;
      /** \induced 17. SHA-1 HMAC test, wrong known answer
     */
      if (17 == icc_failure)
      {
        memcpy(ibuf, hmac_ka, sizeof(hmac_ka));
        ibuf[sizeof(hmac_ka) - 1] ^= 0x01;
        p1 = (const unsigned char *)ibuf;
      }
      iccHMACTest(iccLib, icc_stat, (unsigned char *)hmac_ka_key,
                  sizeof(hmac_ka_key), "SHA1", (unsigned char *)hmac_ka_data,
                  sizeof(hmac_ka_data), p1, sizeof(hmac_ka), ibuf);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: SHA256 HMAC test with known key, input and output */
      p1 = hmac256_ka;
      /** \induced 19. SHA-256 HMAC test, wrong known answer
     */
      if (21 == icc_failure)
      {
        memcpy(ibuf, hmac256_ka, sizeof(hmac256_ka));
        ibuf[sizeof(hmac256_ka) - 1] ^= 0x01;
        p1 = (const unsigned char *)ibuf;
      }
      iccHMACTest(iccLib, icc_stat, (unsigned char *)hmacsha2_ka_key,
                  sizeof(hmacsha2_ka_key), "SHA256",
                  (unsigned char *)hmacsha2_ka_data, sizeof(hmacsha2_ka_data), p1,
                  sizeof(hmac256_ka), ibuf);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: SHA3-512-HMAC test with known key, input and output */
      p1 = hmac3_512_ka;
      /** \induced 68. SHA3-512 HMAC test, wrong known answer
     */
      if (68 == icc_failure)
      {
        memcpy(ibuf, hmac3_512_ka, sizeof(hmac3_512_ka));
        ibuf[sizeof(hmac3_512_ka) - 1] ^= 0x01;
        p1 = (const unsigned char *)ibuf;
      }
      iccHMACTest(iccLib, icc_stat, (unsigned char *)hmacsha2_ka_key,
                  sizeof(hmacsha2_ka_key), "SHA3-512",
                  (unsigned char *)hmacsha2_ka_data, sizeof(hmacsha2_ka_data), p1,
                  sizeof(hmac3_512_ka), ibuf);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: AES-256 CMAC test with known key, input and output */
      p1 = cmac_ka;
      /** \induced 27. AES-CMAC test, wrong known answer
     */
      if (27 == icc_failure)
      {
        memcpy(ibuf, cmac_ka, sizeof(cmac_ka));
        ibuf[sizeof(cmac_ka) - 1] ^= 0x01;
        p1 = (const unsigned char *)ibuf;
      }
      iccCMACTest(iccLib, icc_stat, (unsigned char *)cmac_ka_key, 32, "AES-256-CBC",
                  (unsigned char *)cmac_ka_data, sizeof(cmac_ka_data), p1,
                  sizeof(cmac_ka), ibuf);
    }

    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: AES-256-CBC known answer with known key, iv, input and
     * output */
      p1 = aes_ka;
      /** \induced 80. AES-256-CBC encryption/decryption test, wrong known
     * answer
     */
      if (80 == icc_failure)
      {
        memcpy(ibuf, aes_ka, sizeof(aes_ka));
        ibuf[sizeof(aes_ka) - 1] ^= 0x01;
        p1 = (const unsigned char *)ibuf;
      }
      iccCipherTest(iccLib, "AES-256-CBC", in, sizeof(in), p1, sizeof(aes_ka),
                    aes_key, cbc_iv, icc_stat, ibuf);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      RNGAlgTests(iccLib, icc_stat);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      iccDSA2KA(icc_stat);
    }
    /** \known Test: EC key pair verification prime field with known (good)
   * keys P384 */
    if (ICC_OK == icc_stat->majRC)
    {
      EC_KEY *eckey = NULL;
      const unsigned char *ptr = EC_key_P384;
      eckey = d2i_ECPrivateKey(NULL, &ptr, sizeof(EC_key_P384));
      if (ICC_OK != iccECKEYPairTest(iccLib, eckey))
      {
        SetStatusLn(
            iccLib, icc_stat, FATAL_ERROR, ICC_LIBRARY_VERIFICATION_FAILED,
            "Verification of ECDSA key pair failed (P-284).", __FILE__, __LINE__);
      }
      EC_KEY_free(eckey);
    }
    /** \known Test: EC key pair verification (binary field) with known (good)
   * keys B233 */
    if (ICC_OK == icc_stat->majRC)
    {
      EC_KEY *eckey = NULL;
      const unsigned char *ptr = EC_key_B233;
      eckey = d2i_ECPrivateKey(NULL, &ptr, sizeof(EC_key_B233));
      if (ICC_OK != iccECKEYPairTest(iccLib, eckey))
      {
        SetStatusLn(
            iccLib, icc_stat, FATAL_ERROR, ICC_LIBRARY_VERIFICATION_FAILED,
            "Verification of ECDSA key pair failed (B233).", __FILE__, __LINE__);
      }
      EC_KEY_free(eckey);
    }

    if (ICC_OK == icc_stat->majRC)
    {

      EC_KEY *mine = NULL;
      EC_POINT *otherp = NULL;
      EC_POINT *minep = NULL;
      BN_CTX *bn_ctx = NULL;
      BIGNUM *x = NULL;
      BIGNUM *y = NULL;
      BIGNUM *priv = NULL;
      const EC_GROUP *group = NULL;
      int nid = 0;
      nid = OBJ_txt2nid("secp521r1");

      mine = EC_KEY_new_by_curve_name(nid);
      bn_ctx = BN_CTX_new();
      group = EC_KEY_get0_group(mine);
      strncpy((char *)ibuf, ECDH_pub_otherX, SCRATCH_SIZE - 1);
      /* \induced 140. EDCH, change other public key */
      if (icc_failure == 140)
      {
        ibuf[10] = ~ibuf[10];
      }
      BN_hex2bn(&x, (char *)ibuf);
      BN_hex2bn(&y, ECDH_pub_otherY);
      otherp = EC_POINT_new(group);
      EC_POINT_set_affine_coordinates_GFp(group, otherp, x, y, bn_ctx);
      BN_clear_free(x);
      BN_clear_free(y);
      x = y = NULL;
      BN_hex2bn(&x, ECDH_pub_mineX);
      BN_hex2bn(&y, ECDH_pub_mineY);
      minep = EC_POINT_new(group);
      EC_POINT_set_affine_coordinates_GFp(group, minep, x, y, bn_ctx);
      BN_clear_free(x);
      BN_clear_free(y);
      EC_KEY_set_public_key(mine, minep);
      strncpy((char *)ibuf, ECDH_priv_mine, SCRATCH_SIZE - 1);
      /* \induced 141. EDCH, change my private key */
      if (icc_failure == 141)
      {
        ibuf[10] = ~ibuf[10];
      }
      BN_hex2bn(&priv, (char *)ibuf);
      EC_KEY_set_private_key(mine, priv);
      memcpy(ibuf, ECDH_shared, sizeof(ECDH_shared));
      /* \induced 142. EDCH, change shared secret */
      if (icc_failure == 142)
      {
        ibuf[10] = ~ibuf[10];
      }

      /* \known Test: ECDH
     */
      iccECDHVerifyKAS(icc_stat, otherp, mine, ibuf, sizeof(ECDH_shared));
      EC_POINT_free(otherp);
      EC_KEY_free(mine);
      EC_POINT_free(minep);
      BN_clear_free(priv);
      BN_CTX_free(bn_ctx);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: AES_CCM
        - Encrypt: check against known ciphertext + tag
        - Decrypt: check flags (tags matched),
        - Decrypt: check decrypted text against plaintext
    */
      iccAES_CCMTest(iccLib, icc_stat, (unsigned char *)AES_CCM_key,
                     sizeof(AES_CCM_key), (unsigned char *)AES_CCM_nonce,
                     sizeof(AES_CCM_nonce), (unsigned char *)AES_CCM_AAD,
                     sizeof(AES_CCM_AAD), (unsigned char *)AES_CCM_PT,
                     sizeof(AES_CCM_PT), (unsigned char *)AES_CCM_CT,
                     sizeof(AES_CCM_CT), 4, ibuf);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: AES_GCM
        - Encrypt: check against known ciphertext + tag
        - Decrypt: check flags (tags matched),
        - Decrypt: check decrypted text against plaintext
    */
      iccAES_GCMTest(iccLib, icc_stat, (unsigned char *)gcm_ka_key,
                     sizeof(gcm_ka_key), (unsigned char *)gcm_ka_iv,
                     sizeof(gcm_ka_iv), (unsigned char *)gcm_ka_aad,
                     sizeof(gcm_ka_aad), (unsigned char *)gcm_ka_plaintext,
                     sizeof(gcm_ka_plaintext), (unsigned char *)gcm_ka_ciphertext,
                     sizeof(gcm_ka_ciphertext), (unsigned char *)gcm_ka_authtag,
                     sizeof(gcm_ka_authtag), ibuf);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: AES-128-XTS
        - Encrypt: check against known data
        - Decrypt: check decrypted text against plaintext
    */
      iccAES_XTSTest(iccLib, icc_stat, "AES-128-XTS",
                     (unsigned char *)XTS_128_Key, (unsigned char *)XTS_128_IV,
                     (unsigned char *)XTS_128_PT, sizeof(XTS_128_PT),
                     (unsigned char *)XTS_128_CT, sizeof(XTS_128_CT), ibuf);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: SP800-90 PRNG's
     */
      iccSP800_90Test(iccLib, icc_stat);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: SP800-108 PRNG's
     */
      iccSP800_108Test(iccLib, icc_stat);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: SP800-38F Key wrap, no pad */
      memcpy(ibuf, KW_P, sizeof(KW_P));
      /*! \induced 180. SP800-38F Key wrap, no pad*/
      if (180 == icc_failure)
      {
        ibuf[3] = ~ibuf[3];
      }
      iccCheckKW(icc_stat, (unsigned char *)KW_K, sizeof(KW_K),
                 (unsigned char *)ibuf, sizeof(KW_P), (unsigned char *)KW_C,
                 sizeof(KW_C), 0);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: SP800-38F Key wrap, padded */
      memcpy(ibuf, KWP_P, sizeof(KWP_P));
      /*! \induced 181. SP800-38F Key wrap, padded */
      if (181 == icc_failure)
      {
        ibuf[3] = ~ibuf[3];
      }
      iccCheckKW(icc_stat, (unsigned char *)KWP_K, sizeof(KWP_K),
                 (unsigned char *)ibuf, sizeof(KWP_P), (unsigned char *)KWP_C,
                 sizeof(KWP_C), ICC_KW_PAD);
    }

    if (ICC_OK == icc_stat->majRC)
    {
      int i;
      /** \known Test: HKDF */

      /** \induced 158 HKDF mess up the salt */
      memcpy(ibuf, HKDF_salt, sizeof(HKDF_salt));
      if (158 == icc_failure)
      {
        ibuf[0] = ~ibuf[0];
      }

      /** \induced 159 HKDF mess up the reference output */
      i = SCRATCH_SIZE / 2;
      memcpy(&ibuf[i], HKDF_OKM, sizeof(HKDF_OKM));

      if (159 == icc_failure)
      {
        ibuf[i] = ~ibuf[i];
      }

      iccHKDFTest(icc_stat, "SHA256", HKDF_IKM, sizeof(HKDF_IKM), ibuf, sizeof(HKDF_salt),
                  HKDF_data, sizeof(HKDF_data), HKDF_PRK, sizeof(HKDF_PRK), &ibuf[i], sizeof(HKDF_OKM));
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: TLS1_prf. In hex because of EBCDIC systems */
      static const char TLS_seed[4] = {0x73, 0x65, 0x65, 0x64}; /* "seed" */
      static const char TLS_secret[6] = {0x73, 0x65, 0x63, 0x72, 0x65, 0x74}; /* "secret" */
      const EVP_MD *md = NULL;
      md = EVP_get_digestbyname("SHA256");
      /** \induced 160 TLS1 kdf mess up the seed */
      memcpy(&ibuf[0], TLS_seed, 4);
      if (160 == icc_failure)
      {
        ibuf[0] = ~ibuf[0];
      }
      /** \induced 161 TLS1 kdf mess up the secret */
      memcpy(&ibuf[16], TLS_secret, 6);
      if (161 == icc_failure)
      {
        ibuf[18] = ~ibuf[18];
      }
      iccTestTLS_KDF(icc_stat, md, &ibuf[16], 6, &ibuf[0], 4, (unsigned char *)TLS1_ka, 16);
    }
    if (ICC_OK == icc_stat->majRC)
    {
      iccDHTest(icc_stat);
    }

    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: ChaCha-Poly1305 */
      int i = 256; /* Offset into temp buffer */

      /** \induced 182 ChaCha20-Poly1305 mess up the ciphertext */
      memcpy(ibuf, CHAPOLY_PT, sizeof(CHAPOLY_PT));
      if (182 == icc_failure)
      {
        ibuf[0] = ~ibuf[0];
      }
      /* i = sizeof(CHAPOLY_PT); */
      /** \induced 183 ChaCha-Poly1305 mess up the AAD to cause a tag mismatch */
      memcpy(ibuf + i, CHAPOLY_AAD, sizeof(CHAPOLY_AAD));
      if (183 == icc_failure)
      {
        ibuf[i] = ~ibuf[i];
      }

      iccChaChaPolyTest(icc_stat, CHAPOLY_Key, CHAPOLY_IV, sizeof(CHAPOLY_IV), ibuf + i, sizeof(CHAPOLY_AAD),
                        ibuf, sizeof(CHAPOLY_PT), CHAPOLY_TAG, sizeof(CHAPOLY_TAG), CHAPOLY_CT, sizeof(CHAPOLY_CT));
    }
    if (ICC_OK == icc_stat->majRC)
    {
      /** \known Test: PBKDF2 */
      /** \induced 184 Change the Password */
      memcpy(ibuf, PBKDF2_PWD, sizeof(PBKDF2_PWD));
      if (184 == icc_failure)
      {
        ibuf[0] = ~ibuf[0];
      }
      iccPBKDF2Test(icc_stat, PBKDF2_digest, PBKDF2_Iters,
                    (const char *)ibuf, sizeof(PBKDF2_PWD), PBKDF2_Salt, sizeof(PBKDF2_Salt), PBKDF2_key, sizeof(PBKDF2_key));
    }
    if (rsaPkey != NULL)
    {
      EVP_PKEY_free(rsaPkey);
    }
    if (rsaKey != NULL)
    {
      RSA_free(rsaKey);
    }

    /*  disable ICC when an error is found doing the known answer tests  */
    if (ICC_OK != icc_stat->majRC)
    {
      SetFatalError("Unhandled error during SelfTest", __FILE__, __LINE__);
    }

    if (NULL != signature)
    {
      memset(signature, 0, SCRATCH_SIZE);
      ICC_Free(signature);
    }
    if (NULL != ibuf)
    {
      memset(ibuf, 0, SCRATCH_SIZE);
      ICC_Free(ibuf);
    }
    if (NULL != mystat)
    {
      memset(mystat, 0, sizeof(ICC_STATUS));
      ICC_Free(mystat);
    }
  }
  OUT();
}
/**
   @brief helper function for the new version of the signature checks
   @param stat a pointer to an ICC_STATUS structure
   @return an EVP_PKEY containing the public key used to validate
   file signatures. The caller must free this key
*/
EVP_PKEY *get_pubkey(ICC_STATUS *stat) {
  EVP_PKEY *rsaPKey = NULL;
  IN();
  rsaPKey = EVP_PKEY_new();
  if (NULL == rsaPKey) {
    SetStatusMem(NULL, stat, __FILE__, __LINE__);
  } else {
    const unsigned char *p1 = NULL;
    p1 = (unsigned char *)rsa_pub_key;
    /** \induced 157. Signature test, corrupt DER encoding
     */
    if (157 == icc_failure) {
      p1++;
    }
    rsaPKey = d2i_PublicKey(EVP_PKEY_RSA, &rsaPKey, (const unsigned char **)&p1,
                            sizeof(rsa_pub_key));

    /** \induced 153. iccSignature test. Couldn't convert embedded key to
       binary representation. This should never happen as if OpenSSL isn't
       working we should fail earlier.
    */
    if (153 == icc_failure) {
      EVP_PKEY_free(rsaPKey);
      rsaPKey = NULL;
    }
    if (NULL == rsaPKey) {
      SetStatusLn(NULL, stat, FATAL_ERROR, ICC_LIBRARY_VERIFICATION_FAILED,
                  "Could not parse RSA key, memory corruption ?", __FILE__,
                  __LINE__);
    }
  }
  OUTRC((NULL != rsaPKey));
  return rsaPKey;
}

#if defined(KNOWN) 
/**
   @brief Utility to generate known answer vectors
   @note Does nothing unless KNOWN is defined
   Move to a separate routine to clean up the code
   it was called from
*/
static void GenerateKAData(ICClib *iccLib,ICC_STATUS *stat) {
  printf("\nGenerate known answers for FIPS tests\n");
  iccGenerateDSA(stat,2048);
  iccGenerateRSA(stat,2048);
  iccGenerateECDSA(stat, EC_curve_B233);
  iccGenerateECDSA(stat, EC_curve_K233);
  iccGenerateECDSA(stat, EC_curve_P384);
  iccGenerateHash("SHA224");
  iccGenerateHash("SHA256");
  iccGenerateHash("SHA384");
  iccGenerateHash("SHA512");
  iccGenerateHash("SHA3-224");
  iccGenerateHash("SHA3-256");
  iccGenerateHash("SHA3-384");
  iccGenerateHash("SHA3-512");
  iccGenerateHashXOF("SHAKE128");
  iccGenerateCMAC("AES-256-CBC",(unsigned char *)aes_key,32);
  printf("\nEnd known answers for FIPS tests\n");
}
#endif
