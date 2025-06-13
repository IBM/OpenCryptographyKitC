/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: sha256sum with ICC specific bonus (parse ICCSIG.txt, check lib)
//
*************************************************************************/


/* File format ICCSIG.txt
# Comments
# Environment variables, same syntax and attributes as ICC environment
# variables
# And some diagnostics
# PLATFORM=LINUX_PPC32
# BUILD_DATE=
# VERSION=
#
# SHA256 HASH of the crypto library, for support
# Commented out as it's never needed internally
#HASH=XXXXXXXXXXXXXXXXXXXXXXXXXXXX
# Signature of the crypto shared library
FILE=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
# Signature of self
SELF=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
#
# The following configuration items may be edited
# BUT NOTE The file itself is signature checked up to and including the
# SELF= line. Line wrap is Unix format, editing on non-Unix platforms
# may corrupt the file and prevent the crypto. module from loading
#
#ICC_SEED_GENERATOR=OS

# Increase the number of NRBG's and DRBG's in the thread pool.
# The default (7) has been tested up to 56 cores, more may be needed
# when running on machines with a higher core count.
# This ONLY improves performance with core count, NOT thread count.
#
# Change the pseudo random number generator ICC uses
# Valid values:
HMAC-SHA256,HMAC-SHA384,HMAC-SHA512,AES-256-ECB,SHA256,SHA384,SHA512 #
Equivalent environment variable (none)
#
#ICC_RANDOM_GENERATOR=
#
#
#ICC_RNG_INSTANCES=256
#
# Use a different method for initial setup of the RNG, valid values 1,2
#ICC_RNG_TUNER=1
# Disable hardware acceleration. Not recommended, but sometimes useful for
# benchmarking
#ICC_CPU_CAPABILITY_MASK=0000000000000000
#


*/
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#define off_t long
#define strdup(x) _strdup(x)
#endif
#include "openssl/err.h"
#include "openssl/evp.h"
#include "openssl/rsa.h"


#define NAMELEN 2048

EVP_MD_CTX *EVP_MD_CTX_new();
void EVP_MD_CTX_free(EVP_MD_CTX *ctx);
#define EVP_MD_CTX_cleanup EVP_MD_CTX_reset

static char fbuf[16384];

static char *mybasename(char *in)
{
   char *out = NULL;
   if (NULL != in)
   {
      if('/' == in[strlen(in)]) {
         in[strlen(in)] = '\0';
      }
      (out = strrchr(in,'/')) ? ++out : (out = in);
   }
   return out;
}

/* Tidy up a buffer returned by fgets
   by removing the line terminator
*/
static void bClean(char *buffer) {
  char *ptr;
  ptr = strchr(buffer, '\n');
  if (NULL != ptr) {
    *ptr = '\0';
  }
  ptr = strchr(buffer, '\r');
  if (NULL != ptr) {
    *ptr = '\0';
  }
}

/*! @brief
  @param fin the file pointer
  @param pos the offset in the file to hash to. (0 it's calculated as the total
  file size)
  @param md_ctx message digest context
  @param md message digest
  @return pos the number of bytes unread
  @note WARNING, this code uses an OpenSSL specific trick
  SignInit/VerifyInit are aliases to DigestInit
  "" Update
*/
static long HashCore(FILE *fin, long pos, EVP_MD_CTX *md_ctx,
                     const EVP_MD *md) {
  size_t len = 0;
  long amt = 0;

  if (NULL != fin) {
    if (0 == pos) {
      fseek(fin, 0, SEEK_END);
      pos = ftell(fin);
    }
    fseek(fin, 0, SEEK_SET);
    EVP_MD_CTX_cleanup(md_ctx);
    EVP_DigestInit(md_ctx, md);
    /* Work out how much to read */
    while (pos > 0) {
      amt = sizeof(fbuf);
      if (pos < amt) {
        amt = pos;
      }
      len = fread(fbuf, 1, amt, fin);
      if (len > 0) {
        EVP_DigestUpdate(md_ctx, fbuf, len);
        pos -= (long)len;
      } else {
        break;
      }
    }
  }
  return pos;
}

/*!
  @brief Generate the hash of a file
  @param fin the file pointer
  @param hashout Where to store the hash
  @param pos the offset in the file to hash to. (0 it's calculated as the total
  file size)
  @return the size of the hash
*/

static int GenHash(FILE *fin, unsigned char *hashout, long pos) {
  EVP_MD_CTX *md_ctx = NULL;
  const EVP_MD *md = NULL;
  unsigned int signL = 0;
  int evpRC = 0;

  if (NULL != fin) {
    if (0 == pos) {
      fseek(fin, 0, SEEK_END);
      pos = ftell(fin);
    }
    fseek(fin, 0, SEEK_SET);
    md_ctx = EVP_MD_CTX_new();
    md = EVP_get_digestbyname("SHA256");
    if (NULL != md_ctx && NULL != md) {
      pos = HashCore(fin, pos, md_ctx, md);
      /* printf("Unread %ld\n",pos); */
      evpRC = EVP_DigestFinal(md_ctx, hashout, &signL);
      if (1 != evpRC) {
        signL = 0;
      }
      EVP_MD_CTX_cleanup(md_ctx);
      EVP_MD_CTX_free(md_ctx);
    }
  }
  return (int)signL;
}

static char fntag[] = "# File name="; /*!< Tag for the library name */
static char htag[] = "# File Hash (SHA256)="; /*!< Tag for the sha256 hash of the library */

int ReadConfigItems(FILE *fin, char filename[NAMELEN],char hash[80]) {
  int rv = 1;
  long pos = 0;
  char *fptr = NULL;

  if((NULL == filename) || (NULL == hash) || (NULL == fin)) {
   rv = 0;
  }
  if (1 == rv)
  {
   filename[0] = '\0';
   hash[0] = '\0';

   pos = ftell(fin);
   fseek(fin, 0, SEEK_SET);

   while (NULL != fgets((char *)fbuf, sizeof(fbuf), fin))
   {
      bClean((char *)fbuf);
      fptr = fbuf;

 
      if (' ' == *fptr)
        continue;
      if ('\n' == *fptr)
        continue;
      if ('\r' == *fptr)
        continue;
      if ('\0' == *fptr)
        continue;

      /* Ignore signatures */
      if (0 == strncmp(fntag, (char *)fptr, strlen(fntag)))
      {
        strncpy(filename,fptr+strlen(fntag),NAMELEN-1 );
        /* printf("filename found [%s]\n",filename); */
        continue;
      }

      if (0 == strncmp(htag, (char *)fptr, strlen(htag))) {
         strncpy(hash,fptr+strlen(htag),80-1);
         /* printf("hash found [%s]\n",hash); */
         continue;
      }

   }
   fseek(fin, pos, SEEK_SET);
  }
  return rv;
}
static void usage(char *pname, char *str) {
  printf("usage:\t %s file ", pname);
  printf("\tReplicates sha256sum unless the target is ICCSIG.txt\n");
  printf("\tin which case it parses ICCSIG.txt for the library file name"
         "\tand hash and checks that as well\n");
}
static char iccsig[] = "ICCSIG.txt";


int sha256sum(char *fname,char lclhash[80]) 
{
   int rv = 0;
   int hlen = 0;
   FILE *fin = NULL;
   unsigned char hashout[64];
   int i = 0;

   fin = fopen(fname,"rb");
   if(NULL != fin) {
      rv = 1;
      hlen = GenHash(fin,hashout,0L);
      for(i = 0; i < hlen;i++) {
         sprintf(lclhash+(2*i),"%02x",(unsigned int)hashout[i]);
      }
      printf("%s %s\n",lclhash,fname);
      fclose(fin);
   }
   return rv;
}
char tmppath[NAMELEN];
char filename[NAMELEN];

char hash[80];
char myhash[80];

int main(int argc, char *argv[])
{
  FILE *libf = NULL;
  const char *bname = NULL;
  char *ptr = NULL;
  int rv = 0;

  if (argc < 2)
  {
    usage(argv[0], "Insufficient arguments, need filename\n");
    exit(1);
  }
  /* Do sha256 sum unconditionally*/
  if( 0 != sha256sum(argv[1],myhash) ) {
   strncpy(tmppath,argv[1],NAMELEN-1);
   bname = mybasename(tmppath);
   /* If it's ICCSIG.txt then check the lib as well */
   if(0 == strcmp(bname,iccsig)) {
      libf = fopen(argv[1],"rb");
      ReadConfigItems(libf,filename,hash);
      fclose(libf);
      if('\0' != filename[0]) {
         strncpy(tmppath,argv[1],NAMELEN-1);
         ptr = &tmppath[0] + strlen(tmppath) - strlen(iccsig);
         *ptr = '\0'; 
         strcat(ptr,filename);
         /* printf("target lib [%s]\n",tmppath); */
         sha256sum(tmppath,myhash);
         if(0 != strcmp(myhash,hash)) {
            printf("Hash in ICCSIG.txt does NOT match the hash of %s\n",tmppath);
            rv = 1;
         } else {
            printf("Hash in ICCSIG.txt matches that of %s\n",tmppath);
         }
      } else {
         printf("Info: Could not find library name in ICCSIG.txt\n");
      }
   }
  } else {
      printf("Could not open %s\n",argv[1]);
      rv = 2;
  }   
  OPENSSL_init_crypto(
      OPENSSL_INIT_NO_LOAD_CONFIG | OPENSSL_INIT_LOAD_CRYPTO_STRINGS |
          OPENSSL_INIT_ADD_ALL_DIGESTS | OPENSSL_INIT_ADD_ALL_CIPHERS,
      NULL);

  OPENSSL_cleanup();
  return rv;
}
