/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*
// Description: Checking own signature against external signature file
*/

/* File format
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
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#include "openssl/err.h"
#include "openssl/evp.h"
#include "openssl/rsa.h"


#include "extsig.h"
#include "iccversion.h"
#   if !defined(STANDALONE)
#include "tracer.h"
#else
#   define IN()
#   define OUTRC(x)
#endif

EVP_MD_CTX *EVP_MD_CTX_new();
void EVP_MD_CTX_free(EVP_MD_CTX *ctx);
static int x2bin(char c);
#define EVP_MD_CTX_cleanup EVP_MD_CTX_reset

#if defined(STANDALONE)
#define ICC_Free(x) CRYPTO_free(x, __FILE__, __LINE__)
#endif

static unsigned char fbuf[16384];
static unsigned char signB[4096]; /* Binary signature */

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
int x2bin(char cin) {
  int i = 0;
  int c = (int)cin;
  c = tolower(c);
  if (c >= 'a' && c <= 'f') {
    i = c - 'a' + 10;
  } else if (c >= '0' && c <= '9') {
    i = c - '0';
  }
  return i;
}
static int Block2Bin(char *in, unsigned char *outb) {
  int h, l;
  int i;
  int signL;
  signL = (int)strlen(in) / 2;
  /* Convert the rest of the buffer to binary */
  for (i = 0; i < signL; i++) {
    h = x2bin(in[i * 2]);
    l = x2bin(in[(i * 2) + 1]);
    outb[i] = (unsigned char)((h << 4) | l);
  }
  return signL;
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
  int rc = 0;

  if (NULL != fin) {
    if (0 == pos) {
      fseek(fin, 0, SEEK_END);
      pos = ftell(fin);
    }
    fseek(fin, 0, SEEK_SET);
    EVP_MD_CTX_cleanup(md_ctx);
    rc = EVP_DigestInit(md_ctx, md);
    if (1 != rc) {
       printf("HashCore:EVP_DigestInit failed %d\n", rc);
    }
    /* Work out how much to read */
    while (pos > 0) {
      amt = sizeof(fbuf);
      if (pos < amt) {
        amt = pos;
      }
      len = fread(fbuf, 1, amt, fin);
      if (len > 0) {
        rc = EVP_DigestUpdate(md_ctx, fbuf, len);
        if (1 != rc) {
           printf("HashCore:EVP_DigestUpdate failed %d\n", rc);
        }
        pos -= (long)len;
      } else {
         printf("HashCore:fread failed\n");
         break;
      }
    }
  }
  return pos;
}

/*!
  @brief Generate the has of a file
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
static void GenPHash(FILE *fin, long pos) {
  unsigned char digest[64];

  memset(digest, 0, sizeof(digest));
  /* printf("pos = %ld :",pos); */
  GenHash(fin, digest, pos);
}

int ReadConfigItems(FILE *fin, char *tweaks[], int n) {
  int i = 0;
  long pos = 0;
  unsigned char *fptr = NULL;

  IN();
  if (NULL != fin && NULL != tweaks) {
    pos = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    i = 0;
    while (NULL != fgets((char *)fbuf, sizeof(fbuf), fin)) {
      bClean((char *)fbuf);
      fptr = fbuf;

      if ('#' == *fptr)
        continue;
      if (' ' == *fptr)
        continue;
      if ('\n' == *fptr)
        continue;
      if ('\r' == *fptr)
        continue;
      if ('\0' == *fptr)
        continue;

      /* Ignore signatures */
      if (0 == strncmp("FILE=", (char *)fptr, 5))
        continue;
      if (0 == strncmp("SELF=", (char *)fptr, 5))
        continue;
      if (NULL != strchr((char *)fptr, '=')) {
        if (i < n) {
          tweaks[i] = strdup((char *)fptr); /* Tweaks are free'd by caller */
          i++;
        } else {
          break; /* No space left for configs */
        }
      }
    }
    fseek(fin, pos, SEEK_SET);
  }
  OUTRC(i);
  return i;
}
/*! @brief
  Signature check using the passed in signature file and the
  file handle of the binary file we are to check
  @brief sigfile The open file handle of the signature file
  @brief targ The open file handle of the binary
  @brief rsaPKey A pointer to the PKEY containing the public key
  @brief OnlySigFile if set only the signature file is checked
  @return 0 O.K. 1 self sig fails, 2 file sig fails,
          3 "something bad", 4 0 length file
*/
int CheckSig(FILE *sigfile, FILE *targ, EVP_PKEY *rsaPKey, int OnlySigFile) {
  int rv = 0; /* 0 O.K. 1 self sig fails, 2 file sig fails, 3 SBH */
  long fpos = 0;
  long pos = 0;
  long tpos = 0;
  int signL = 0;
  int evpRC = 0;

  unsigned char *ptr = NULL;
  EVP_MD_CTX *md_ctx = NULL;
  const EVP_MD *md = NULL;

  IN();
  if ((NULL == sigfile) | (NULL == targ)) {
    rv = 4;
  } else {
    fseek(sigfile, 0, SEEK_SET);
    md_ctx = EVP_MD_CTX_new();

    md = EVP_get_digestbyname("SHA256");
    if (NULL == md_ctx || NULL == md) {
      rv = 3; /* Crypto code MIA */
    } else {
      while (1) {
        tpos = ftell(sigfile);
        if (NULL == fgets((char *)fbuf, sizeof(fbuf) - 2, sigfile)) {
          break;
        }
        ptr = fbuf;
        if ('\r' == *ptr) {
          ptr++;
          tpos++;
        }
        if ('\n' == *ptr) {
          ptr++;
          tpos++;
        }
        if (0 == strncmp("SELF=", (const char *)ptr, 5)) {
          /* Found self signed signature */
          pos = tpos;
          break;
        }

        if (0 == strncmp("FILE=", (const char *)ptr, 5)) {
          /* Position of File signature */
          fpos = tpos;
        } else if ('#' == fbuf[0]) {
          /* skip comments */
        }
      }
      if(NULL != ptr) {
        if (0 == strncmp("SELF=", (const char *)ptr, 5)) {
          GenPHash(sigfile, pos);
          fseek(sigfile, pos, SEEK_SET);
          fgets((char *)fbuf, sizeof(fbuf) - 2, sigfile);
          ptr = fbuf;
          ptr += 5; /* Skip past "SELF=" */
          bClean((char *)ptr);
          memset(signB, 0, sizeof(signB));
          signL = Block2Bin((char *)ptr, signB);
          /* printf("signL = %d\n",signL); */
          HashCore(sigfile, pos, md_ctx, md);

          evpRC = EVP_VerifyFinal(md_ctx, signB, signL, rsaPKey);
          if (evpRC != 1) {
            rv = 1;
          }
        } else { /* SELF sig missing */
          rv = 1;
        }
      } else {
        rv = 3;
      }
    }
    if ((0 == rv) && (pos == 0 || fpos == 0)) {
      rv = 4;
    }
    if (!OnlySigFile) {
      /* IF Signature of self was O.K.
         extract the signature of the File
         the "FILE=" block has to occur before that.
         Signature check passed, fpos is set
      */
      if (0 == rv && (pos > fpos)) {
        fseek(sigfile, fpos, SEEK_SET);
        fgets((char *)fbuf, sizeof(fbuf) - 2, sigfile);
        ptr = fbuf;
        /* Now check the signature of the binary */
        if (0 == strncmp("FILE=", (const char *)fbuf, 5)) {
          ptr += 5;
          bClean((char *)ptr);
          memset(signB, 0, sizeof(signB));
          signL = Block2Bin((char *)ptr, signB);
          HashCore(targ, 0, md_ctx, md);
          evpRC = EVP_VerifyFinal(md_ctx, signB, signL, rsaPKey);
          if (1 != evpRC) {
            rv = 2;
          }
        }
      } else { /* Signature file misformatted */
        rv = 1;
      }
    }
  }
  if (NULL != md_ctx) {
    EVP_MD_CTX_cleanup(md_ctx);
    EVP_MD_CTX_free(md_ctx);
  }
  OUTRC(rv);
  return rv;
}
#if defined(STANDALONE)
/*!
  @brief return the Day of the week as a string given a
  week number 0-6 Sun->Sat
  @param wday the weekday number
  @return the weekday text
*/
static const char *DoW(int wday) {
  const char *wk = "---";
  static const char *DoWA[7] = {"Sun", "Mon", "Tue", "Wed",
                                "Thu", "Fri", "Sat"};

  if ((wday >= 0) && (wday < 7)) {
    wk = DoWA[wday];
  }
  return wk;
}
/*!
  @brief return the Month of the year as a string given
  the month number (0-11)
  @param month the month number
  @return the month text
*/
static const char *MoY(int month) {
  const char *mo = "---";
  static const char *MoYA[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  if ((month >= 0) && (month < 12)) {
    mo = MoYA[month];
  }
  return mo;
}

#if defined(_WIN32)
/*!
  @brief reproduce the Unix date/time function
  @param buffer a place to return the Timestamp text
  @return the timestamp text
*/
static char *TimeStamp(char *buffer) {
  SYSTEMTIME lt;
  GetSystemTime(&lt);

  sprintf(buffer, "%s %s %02d %02d:%02d:%2d %04d", DoW(lt.wDayOfWeek),
          MoY(lt.wMonth - 1), lt.wDay, lt.wHour, lt.wMinute, lt.wSecond,
          lt.wYear);
  buffer[24] = ' ';
  buffer[25] = '\0';
  return buffer;
}
#else
/*!
  @brief reproduce the Unix date/time function
  @param buffer a place to return the Timestamp text
  @return the timestamp text (== buffer)
  Note that it's simpler to do this by hand than it is to include the
  correct headers across every OS variant
*/
static char *TimeStamp(char *buffer) {
  time_t timep;
  struct tm tm;
  time(&timep);
  gmtime_r(&timep, &tm);
  sprintf(buffer, "%s %s %02d %02d:%02d:%02d %04d", DoW(tm.tm_wday),
          MoY(tm.tm_mon), tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
          tm.tm_year + 1900);
  buffer[24] = ' ';
  buffer[25] = '\0';
  return buffer;
}
#endif

/* Below , code to generate and manually verify the signatures
   Checked with a script "quite difficult"
*/

static char PRIVKEY_HDR[] = "-----BEGIN RSA PRIVATE KEY-----\n";
static char PRIVKEY_FTR[] = "-----END RSA PRIVATE KEY-----\n";
static char PUBKEY_HDR[] = "-----BEGIN RSA PUBLIC KEY-----\n";
static char PUBKEY_FTR[] = "-----END RSA PUBLIC KEY-----\n";

static int do_b64decode_string(unsigned char *in, int inl,
                               unsigned char **out) {
  int n = 0, l = 0;

  EVP_ENCODE_CTX *ctx;
  unsigned char *optr;

  ctx = EVP_ENCODE_CTX_new();
  /* Allocate return buffer, caller must free */
  if (out && *out == NULL) {
    *out = (unsigned char *)malloc(inl);
  }
  if (out && *out) {
    EVP_DecodeInit(ctx);

    optr = *out;
    EVP_DecodeUpdate(ctx, (unsigned char *)optr, &n, (unsigned char *)in, inl);

    l = n;
    optr = (*out) + n;
    EVP_DecodeFinal(ctx, (unsigned char *)optr, &n);
    l += n;
  }

  EVP_ENCODE_CTX_free(ctx);

  return l;
}

/*!
  @brief parse a formatted key from a string buffer
  @param icc_ctx an initialized ICC context
  @param buf the string buffer containing the key
  @param type "private" or "public"
*/
static EVP_PKEY *parse_key(char *buf, char *type) {
  char *ptr = NULL, *ptr1 = NULL;
  unsigned char *b64 = NULL;
  unsigned char *p = NULL;
  unsigned char *pu = NULL;
  int l = 0;
  EVP_PKEY *key = NULL;

  key = EVP_PKEY_new();

  /* if we didn't ask for a public key, read a private key */
  if (strcmp(type, "public") != 0) {
    /* Search for start of private key.... */
    ptr = strstr((const char *)buf, PRIVKEY_HDR);
    ptr1 = strstr((const char *)buf, PRIVKEY_FTR);
    if (ptr) {
      ptr += strlen(PRIVKEY_HDR);
      /* Convert from BASE64 -> binary */
      l = (int)(ptr1 - ptr);
      b64 = (unsigned char *)malloc(l);
      p = b64;
      l = do_b64decode_string((unsigned char *)ptr, l, &p);
      p = b64;
      pu = (unsigned char *)p;
      if (NULL == d2i_PrivateKey(EVP_PKEY_RSA, &key,
                                 (const unsigned char **)&pu, (long)l)) {
        EVP_PKEY_free(key);
        key = NULL;
      }
      free(b64);
    }
    /* The private key contains a copy of the public key as well
       so if we read a private key we don't need to re-read it
    */
  } else if (strcmp(type, "private") != 0) {
    ptr = strstr((const char *)buf, PUBKEY_HDR);
    ptr1 = strstr((const char *)buf, PUBKEY_FTR);
    if (ptr) {
      ptr += strlen(PUBKEY_HDR);
      l = (int)(ptr1 - ptr);
      b64 = (unsigned char *)malloc(l);
      p = b64;
      l = do_b64decode_string((unsigned char *)ptr, l, &p);
      p = b64;
      pu = p;
      if (NULL == d2i_PublicKey(EVP_PKEY_RSA, &key, (const unsigned char **)&pu,
                                (long)l)) {
        EVP_PKEY_free(key);
        key = NULL;
      }
      free(b64);
    }
  }
  return key;
}
/* read a key from a file generated as above,
   convert back to internal key format

   EXPLANATION.
   Normally, when you read a key, you'd only be reading either
   the public or private component.
   There's a "feature" of OpenSSL, it overwrites the previously
   loaded common components of the key when you der decode the
   "other" half of a key. This causes a memory leak.

   That leads to the somewhat funky logic below.
   If we didn't ask for a "private" key, read a public key.
   if we didn't ask for a "public" key, read a private key.
   Hence, if we didn't know what was being asked for we
   read both.
   type can be "public", "private", or any other string typ. ""
*/
static EVP_PKEY *read_key(FILE *in, char *type) {
  off_t pos;
  char *buf = NULL;
  EVP_PKEY *key = NULL;

  /* Work out how big the file is */
  fseek(in, 0, SEEK_END);
  pos = ftell(in);
  fseek(in, 0, SEEK_SET);
  /* We SHOULD check that the file is a reasonable size here ....
     but in this case we don't need to, this code is only called during the
     build process, the public key in the executable is just DER encoded data
     no read required.
  */
  if (pos > 0) {
    buf = (char *)malloc(pos + 1);
    if (NULL != buf) {
      fread(buf, pos, 1, in);
      buf[pos] = '\0'; /*terminate the buffer so string searches will stop */
      key = parse_key(buf, type);
    }
  }
  if (buf)
    free(buf);
  return key;
}

/*!
  @brief Generate the signature of a file
  @param the path to the file to sign
  @return the length of the signature in bytes
  @note The signature is returned as binary
*/

static int GenSig(FILE *fin, unsigned char *sigout, EVP_PKEY *key, long pos) {
  EVP_MD_CTX *md_ctx = NULL;
  const EVP_MD *md = NULL;
  unsigned int signL = 0;
  int evpRC = 0;

  if (NULL != fin) {
    md_ctx = EVP_MD_CTX_new();
    md = EVP_get_digestbyname("SHA256");
    if (NULL != md_ctx && NULL != md) {
      HashCore(fin, pos, md_ctx, md);
      evpRC = EVP_SignFinal(md_ctx, sigout, &signL, key);
      if (1 != evpRC) {
         printf("GenSig: EVP_SignFinal error %d\n", evpRC);
         signL = 0;
      }
      EVP_MD_CTX_free(md_ctx);
    }
    else {
       printf("GenSig: EVP error\n");
    }
    fseek(fin, pos, SEEK_SET);
  }
  else {
     printf("GenSig: fin error\n");
  }
  return (int)signL;
}
static void usage(char *pname, char *str) {
  printf("usage:\t %s sigfile keyfile [-v(erify)] [-SELF] [-FILE file] "
         "[\"X=Y\"] ...[\"Z=K\"]\n",
         pname);
  printf("OR:\t%s sigfile keyfile -v(erify) -FILE file\n", pname);
  if (NULL != str) {
    printf("\t\tError:%s\n", str);
  }
  printf("\tsigfile is the target file which will be updated\n");
  printf("\tkeyfile contains the RSA key pair\n");
  printf("\t-v will verify rather than sign the library and signature file\n");
  printf("\t   NOTE: The library will be verified immediately after sign "
         "anyway, this is a separate 'verify it'\n");
  printf("\t-FILE file appends the signature of the ICC binary to sigfile, "
         "must be done before [-SELF]\n");
  printf("\t-SELF signs sigfile - may be specified with -FILE, in which case "
         "it happens after FILE=\n");
  printf("\t\"X=Y ICC settings to be applied, must be provided before or with "
         "-SELF\n");
}
#define MAXTWEAKS 20
int main(int argc, char *argv[]) {
  FILE *sigf = NULL;
  FILE *rsaf = NULL;
  FILE *bfile = NULL;
  const char *bname = NULL;
  const char *ptr = NULL;
  char *tptr = NULL;
  char *pptr = NULL;
  EVP_PKEY *rsakey = NULL;
  char *tweaks[MAXTWEAKS];
  int i = 0, j = 0;
  int signself = 0;
  int verify = 0;
  int len;
  char tbuf[80];
  long pos = 0;

  memset(tweaks, 0, sizeof(tweaks));
  if (argc < 4) {
    usage(argv[0], "Insufficient arguments");
    exit(1);
  }
  if (argc > ((sizeof(tweaks) / sizeof(char *)) + 3)) {
    usage(argv[0], "Too many arguments");
    exit(1);
  }

  rsaf = fopen(argv[2], "r");
  if (NULL == rsaf) {
    usage(argv[0], "Could not open keyfile");
    exit(1);
  }

  {
     int rc = 0;
     rc = OPENSSL_init_crypto(
      OPENSSL_INIT_NO_LOAD_CONFIG | OPENSSL_INIT_LOAD_CRYPTO_STRINGS |
          OPENSSL_INIT_ADD_ALL_DIGESTS | OPENSSL_INIT_ADD_ALL_CIPHERS,
      NULL);
     if (rc != 1) {
        usage("OpenSSL", "OPENSSL_init_crypto");
        exit(1);
     }
     ERR_load_crypto_strings();
  }

  /* Step through and pick up anything else */
  for (i = 3; i < argc; i++) {
    if (NULL != strstr(argv[i], "-v")) {
      verify = 1;
    } else if (NULL != strstr(argv[i], "-SELF")) {
      signself = 1;
    } else if (NULL != strstr(argv[i], "-FILE")) {
      bname = argv[i + 1];
      bfile = fopen(argv[i + 1], "rb");
      if (NULL == bfile) {
        usage(argv[0], "-FILE specified, but file could not be opened");
        exit(1);
      }
      i++;
    } else if (NULL != strstr(argv[i], "=")) {
      /* 42415 - allow multiple tweaks in one quoted string
         for make friendliness
      */

      ptr = strdup(argv[i]);
      if (NULL != ptr)
      {
        tptr = (char *)ptr;
        while (j < MAXTWEAKS)
        {
          pptr = strstr(tptr, " ");
          if (NULL != pptr)
          {
            *pptr = '\0';
            tweaks[j++] = strdup(tptr);
            tptr = pptr + 1;
          }
          else
          {
            if (tptr && *tptr)
            {
              tweaks[j++] = strdup(tptr);
            }
            break;
          }
        }
      }
      free((void *)ptr);
    }
  }

  if (verify) {
    sigf = fopen(argv[1], "rb");
    if (NULL == sigf) {
      usage(argv[0], "Could not open sigfile");
      exit(1);
    }
    rsakey = read_key(rsaf, "private");
    if (NULL == rsakey) {
      usage(argv[0], "Could not read public key from file");
      exit(1);
    }
    switch (CheckSig(sigf, bfile, rsakey, 0)) {
    case 0:
      printf("Binary file %s verified O.K.\n", bname);
      printf("Signature file verified O.K.\n");
      for (i = 0; i < MAXTWEAKS; i++) {
        if (tweaks[i] != NULL) {
          if (i == 0) {
            printf("Global Settings\n");
          }
          printf("\t%s\n", tweaks[i]);
        }
      }
      break;
    case 1:
      printf("Signature file failed verification.\n");
      break;
    case 2:
      printf("Binary file %s failed verification.\n", bname);
      break;
    default:
    case 3:
      printf("Something bad (tm) happened, giving up.\n");
      break;
    }
  } else {
    sigf = fopen(argv[1], "w+b");
    if (NULL == sigf) {
      usage(argv[0], "Could not open sigfile");
      exit(1);
    }
    TimeStamp(tbuf);
    /* At this point, we should have everything, start pushing it out */
    fprintf(sigf, "# IBM Crypto for C.%s", EOL);
    fprintf(sigf, "# ICC Version %d.%d.%d.%d%s", ICC_VERSION_VER,
            ICC_VERSION_REL, ICC_VERSION_MOD, ICC_VERSION_FIX, EOL);
    fprintf(sigf,
            "#%s# Note the signed library contains a copy of cryptographic "
            "code from OpenSSL (www.openssl.org),%s",
            EOL, EOL);
    fprintf(sigf, "# zlib (www.zlib.org) %s", EOL);
    fprintf(sigf, "# and IBM code (www.ibm.com)%s#%s", EOL, EOL);
    fprintf(sigf, "# Platform %s%s#%s", OPSYS, EOL, EOL);
    TimeStamp(tbuf);
    fprintf(sigf, "# Generated %s%s#%s", tbuf, EOL, EOL);
    if (NULL == bfile) {
      usage(argv[0], "No target binary file specified, aborting");
      exit(0);
    }

    /* Generate the hash of the binary */
    len = GenHash(bfile, signB, 0);
    if (len <= 0) {
      usage(argv[0], "Failed to generate hash of binary file");
      exit(1);
    }
    ptr = strrchr(bname, '/');
    if (NULL != ptr) {
      ptr++;
    }
    if (NULL == ptr) {
      ptr = bname;
    }
    fprintf(sigf, "# File name=%s%s", ptr, EOL);
    fprintf(sigf, "# File Hash (SHA256)=");
    for (i = 0; i < len; i++) {
      fprintf(sigf, "%02x", signB[i]);
    }
    fprintf(sigf, "%s#%s", EOL, EOL);
    rsakey = read_key(rsaf, "private");
    if (NULL == rsakey) {
      usage(argv[0], "Could not read private key from file");
      exit(1);
    }

    /* Generate the signature of the binary */
    fseek(bfile, 0L, SEEK_SET);
    len = GenSig(bfile, signB, rsakey, 0);
    if (len <= 0) {
      printf("Error: GenSig: %d\n", len);
      printf("OpenSSL Error: %s\n", ERR_error_string(ERR_get_error(), NULL));
      usage(argv[0], "Failed to generate signature");
      exit(1);
    }

    fprintf(sigf, "FILE=");
    for (i = 0; i < len; i++) {
      fprintf(sigf, "%02x", signB[i]);
    }
    fprintf(sigf, "%s#%s", EOL, EOL);
    fflush(sigf);
    pos = ftell(sigf);
    /* Generate the hash of the signature file , debug aid */

    GenPHash(sigf, pos);

    fseek(sigf, pos, SEEK_SET);

    if (signself) {
      /* Now do the self sign */
      len = GenSig(sigf, signB, rsakey, pos);
      fprintf(sigf, "SELF=");
      for (i = 0; i < len; i++) {
        fprintf(sigf, "%02x", signB[i]);
      }
      fprintf(sigf, "%s#", EOL);
    }
    fflush(sigf);
    fprintf(sigf, "%s#Do not edit before this line%s#", EOL, EOL);
    if (NULL != tweaks[0]) {
      fprintf(sigf, "%s# Global Settings%s", EOL, EOL);
      for (i = 0; NULL != tweaks[i]; i++) {
        fprintf(sigf, "%s%s", tweaks[i], EOL);
      }
      fprintf(sigf, "#%s", EOL);
    }
  }
  fseek(sigf, 0, SEEK_SET);
  fseek(bfile, 0, SEEK_SET);
  /* And check it ourselves */
  i = CheckSig(sigf, bfile, rsakey, 0);
  /* printf("Verifying signature rc = %d\n",i); */
  switch (i) {
  case 0:
    printf("Verified O.K.\n");
    break;
  case 1:
    printf("Signature file failed verification.\n");
    break;
  case 2:
    printf("Binary file %s failed verification.\n", bname);
    break;
  default:
  case 3:
    printf("Something bad(tm) happened, giving up.\n");
    break;
  }

  for (i = 0; i < MAXTWEAKS; i++) {
    if (NULL != tweaks[i]) {
      free(tweaks[i]);
    } else {
      break;
    }
  }
  printf("%d config items found\n", ReadConfigItems(sigf, tweaks, 20));

  fclose(sigf);
  fclose(bfile);
  fclose(rsaf);

  if (NULL != rsakey) {
    EVP_PKEY_free(rsakey);
  }

  for (i = 0; i < MAXTWEAKS; i++) {
    if (NULL != tweaks[i]) {
      free(tweaks[i]);
    } else {
      break;
    }
  }
  OPENSSL_cleanup();
  return 0;
}

#endif
