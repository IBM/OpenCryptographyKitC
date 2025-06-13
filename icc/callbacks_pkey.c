/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

typedef enum {
  TEST_PASS = 0,
  TEST_FAIL = 1,
  TEST_SKIP = 2,
  TEST_NI = 3
} T_RV;

/*
  Driver code for exercising the EVP_PKEY_keygen() API
  Lifted from icc_test/evp/do_Keygen.c to avoid having to reimplement everything
  from scratch which is why there's somewhat odd argument/parameter handling in
  places.
  Abstracted into a separate file just to get the bulk down in the main test driver.
*/
/* Finite field Diffie-Hellman keygen (FFDH)
*/
static T_RV dox_ffdh(ICC_CTX *ctx,char *ctrl[6]) {
  T_RV rv = TEST_PASS;
  int rc = 1;
  char *Ctrl = NULL;
  char **tmp = NULL;
  int nid = -1;
  int mnid = -1;
  ICC_EVP_PKEY_CTX *pctx = NULL;
  ICC_EVP_PKEY *pkey = NULL;

  Ctrl = ctrl[0]; /* Only one ctrl for FFDHE, keylen */
  if (NULL != Ctrl) {
    tmp = explode(Ctrl, ":");
  } else {
    rv = TEST_FAIL;
  }

  if (TEST_PASS == rv) {
    mnid = ICC_OBJ_txt2nid(ctx,"dhKeyAgreement");
    if (mnid <= 0) {
      printf("Method not found [%s]\n","dhKeyAgreement");
      rv = TEST_FAIL;
    }
  }
  if (TEST_PASS == rv) {
    pctx = ICC_EVP_PKEY_CTX_new_id(ctx, mnid, NULL);
    if (NULL == pctx) {
      rv = TEST_NI;
    }
  }
  if (TEST_PASS == rv) {
    nid = ICC_OBJ_txt2nid(ctx, tmp[1]);
    if (nid <= 0) {
      printf("Algorithm not found %s ", tmp[1]);
      rv = TEST_FAIL;
    }
  }
  if (TEST_PASS == rv) {
    rc = ICC_EVP_PKEY_CTX_ctrl_str(ctx, pctx, (const char *)tmp[0],
                                   (const char *)tmp[1]);
    if (1 != rc) {
      rv = TEST_FAIL;
    }
  }
  if (TEST_PASS == rv) {
    rc = ICC_EVP_PKEY_keygen_init(ctx, pctx);
    if (1 != rc) {
      rv = TEST_FAIL;
    }
  }
  if (TEST_PASS == rv) {
    rc = ICC_EVP_PKEY_keygen(ctx, pctx, &pkey);
    if (1 != rc) {
      rv = TEST_FAIL;
    }
  }
  if (NULL != tmp) {
    explodefree(tmp);
  }
  if (NULL != pkey) { 
      ICC_EVP_PKEY_free(ctx, pkey);
  }
  if (NULL != pctx) {
    ICC_EVP_PKEY_CTX_free(ctx, pctx);
  }
  return rv;
}
/*
  EC key gen 
*/

#define NUM_EC_CTRLS 3
static T_RV dox_EC(ICC_CTX *ctx, char *ctrl[6]) {
  T_RV rv = TEST_PASS;
  int rc = 1;
  char *Ctrl[NUM_EC_CTRLS] = {NULL, NULL, NULL};
  char **tmp = NULL;
  int i = 0;
  ICC_EVP_PKEY_CTX *pctx = NULL;
  ICC_EVP_PKEY *pkey = NULL;

  Ctrl[0] = ctrl[0];
  Ctrl[1] = ctrl[1];
  Ctrl[2] = ctrl[2];

  if (TEST_PASS == rv) {
    pctx = ICC_EVP_PKEY_CTX_new_id(ctx, ICC_EVP_PKEY_EC, NULL);
    if (NULL == pctx) {
      rv = TEST_NI;
    }
  }
  if (TEST_PASS == rv) {
    rc = ICC_EVP_PKEY_keygen_init(ctx, pctx);
    if (1 != rc) {
      rv = TEST_FAIL;
    }
  }
  for (i = 0; i < NUM_EC_CTRLS; i++) {
    if (TEST_PASS == rv) {
      if (NULL != Ctrl[i]) {
        tmp = explode(Ctrl[i], ":");
        rc = ICC_EVP_PKEY_CTX_ctrl_str(ctx, pctx, (const char *)tmp[0],
                                       (const char *)tmp[1]);
        if (1 != rc) {
          rv = TEST_FAIL;
        }
        explodefree(tmp);
        tmp = NULL;
      }
    }
  }
  if (TEST_PASS == rv) {
    rc = ICC_EVP_PKEY_keygen(ctx, pctx, &pkey);
    if (1 != rc) {
      rv = TEST_FAIL;
    }
  }
  if (NULL != pkey) { 
    ICC_EVP_PKEY_free(ctx, pkey);
  }

  if (NULL != pctx) {
    ICC_EVP_PKEY_CTX_free(ctx, pctx);
  }
  return rv;
}

/*
  RSA key gen.
  Note some of these tests come up as NI. 
  OAEP and PSS with those modes create a generic RSA key and set the padding mode 
  in the sign or verify operation so the ops at this point are meaningless
*/
#define NUM_RSA_CTRLS 6
static T_RV dox_RSA(ICC_CTX *ctx, char *ctrl[6]) {
  T_RV rv = TEST_PASS;
  int rc = 1;
  char *Ctrl[NUM_RSA_CTRLS] = {NULL, NULL, NULL, NULL, NULL, NULL};
  char **tmp = NULL;
  int i = 0;
  ICC_EVP_PKEY_CTX *pctx = NULL;
  ICC_EVP_PKEY *pkey = NULL;
  

  Ctrl[0] = ctrl[0];
  Ctrl[1] = ctrl[1];
  Ctrl[2] = ctrl[2];
  Ctrl[3] = ctrl[3];
  Ctrl[4] = ctrl[4];
  Ctrl[5] = ctrl[5];

  if (TEST_PASS == rv) {
    pctx = ICC_EVP_PKEY_CTX_new_id(ctx,ICC_EVP_PKEY_RSA, NULL);
    if (NULL == pctx) {
      rv = TEST_NI;
    }
  }
  if (TEST_PASS == rv) {
    rc = ICC_EVP_PKEY_keygen_init(ctx, pctx);
    if (1 != rc) {
      rv = TEST_FAIL;
    }
  }
  for (i = 0; i < NUM_RSA_CTRLS; i++) {
    if (TEST_PASS == rv) {
      if (NULL != Ctrl[i]) {
        tmp = explode(Ctrl[i], ":");
        if ((0 == strcmp(tmp[0], "rsa_padding_mode")) &&
            ((0 == strcmp(tmp[1], "oaep")) || (0 == strcmp(tmp[1], "pss")))) {
          rv = TEST_NI;
        } else {
          rc = ICC_EVP_PKEY_CTX_ctrl_str(ctx, pctx, (const char *)tmp[0],
                                         (const char *)tmp[1]);
          if (1 != rc) {
            rv = TEST_FAIL;
          }
        }
        explodefree(tmp);
        tmp = NULL;
      }
    }
  }
  if (TEST_PASS == rv) {
    rc = ICC_EVP_PKEY_keygen(ctx, pctx, &pkey);
    if (1 != rc) {
      rv = TEST_FAIL;
    }
  }
  if (NULL != pkey) { 
    ICC_EVP_PKEY_free(ctx, pkey);
  }
  if (NULL != pctx) {
    ICC_EVP_PKEY_CTX_free(ctx, pctx);
  }
  return rv;
  }



  #define NUM_DSA_CTRLS 3

  static T_RV dox_DSA(ICC_CTX *ctx, char *ctrl[6]) {
    T_RV rv = TEST_PASS; /* In theory this should work, in practice it doesn't */ 
    int rc = 1;
    char *Ctrl[NUM_DSA_CTRLS] = {NULL, NULL, NULL};
    char **tmp = NULL;
    int i = 0;
    ICC_EVP_PKEY_CTX *pctx = NULL; /* Context for params */
    ICC_EVP_PKEY *pkey = NULL;      /* PKEY for params */
    ICC_EVP_PKEY_CTX *kctx = NULL; /* Context for keygen */
    ICC_EVP_PKEY *kkey = NULL;     /* PKEY for actual DSA key */

    Ctrl[0] = ctrl[0];
    Ctrl[1] = ctrl[0];
    Ctrl[2] = ctrl[0];
    if (TEST_PASS == rv) {
      pctx = ICC_EVP_PKEY_CTX_new_id(ctx, ICC_EVP_PKEY_DSA, NULL);
      if (NULL == pctx) {
        rv = TEST_NI;
      }
    }

    if (TEST_PASS == rv) {
      rc = ICC_EVP_PKEY_paramgen_init(ctx, pctx);
      if (1 != rc) {
        rv = TEST_FAIL;
      }
    }
    for (i = 0; i < NUM_DSA_CTRLS; i++) {
      if (TEST_PASS == rv) {
        if (NULL != Ctrl[i]) {
          tmp = explode(Ctrl[i], ":");
          rc = ICC_EVP_PKEY_CTX_ctrl_str(ctx, pctx, (const char *)tmp[0],
                                         (const char *)tmp[1]);
          if (1 != rc) {
            rv = TEST_FAIL;
          }
          explodefree(tmp);
          tmp = NULL;
        }
      }
    }
    if (TEST_PASS == rv) {
      rc = ICC_EVP_PKEY_paramgen(ctx, pctx,&pkey);
      if (1 != rc) {
        rv = TEST_FAIL;
      }
    }

    kctx = ICC_EVP_PKEY_CTX_new(ctx,pkey,NULL);

    if (TEST_PASS == rv) {
      rc = ICC_EVP_PKEY_keygen_init(ctx, kctx);
      if (1 != rc) {
        rv = TEST_FAIL;
      }
    }   
    if (TEST_PASS == rv) {
      rc = ICC_EVP_PKEY_keygen(ctx, kctx, &kkey);
      if (1 != rc) {
        rv = TEST_FAIL;
      }
    }
    if (NULL != kkey) {
      ICC_EVP_PKEY_free(ctx, kkey);
    }
    if (NULL != kctx) {
      ICC_EVP_PKEY_CTX_free(ctx, kctx);
    }
    if(NULL != pkey) {
      ICC_EVP_PKEY_free(ctx,pkey);
    }
    if(NULL != pctx) {
      ICC_EVP_PKEY_CTX_free(ctx, pctx);
    }
    return rv;
  }
  