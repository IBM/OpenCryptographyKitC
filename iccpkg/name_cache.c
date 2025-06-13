/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

#include "name_cache_tables.c"

/* 
  Note that the nid field in the data structures is not used by THIS code
  however having the data structure common saves a lot of maintenance.

  Note that this will have to go once the FIPS code has it's own
  variant
*/

/* name->md, Use case insensitive string compare 
  works with either MD or Cipher as the cache objects being manipulated 
  are the same size and contain the same size objects
  Note that alias's are allowed and this will miss alias's
  it simply provides a fast path for the commonly used names
*/
static int mycmp(const void *p1, const void *p2)
{
    return strcasecmp(((MD_CACHE *)p1)->name,((MD_CACHE *)p2)->name);
}
/* Callback for ICC_EVP_getXYZbyname() calls */

typedef void * (*NAMEFUNC)(ICC_CTX *ctx,const char *name);


/* Generic cache setup, basically sort the entries by name */

static void init_cache(ICC_CTX *ctx,MD_CACHE *dig,int n,NAMEFUNC func)
{
    int i;
    for(i = 0; i < n;i++) {
        dig[i].md = (*func)(ctx,dig[i].name);
    }    
    qsort(dig,n,sizeof(MD_CACHE),mycmp);
}

/*! 
  @brief Lookup a cache entry by name
  @param base of the cache to search
  @param n number of entries in this cache
  @param name the name of the object to locate
  @return the cache entry or NULL on no match
  @note there's an amount of overloading here.
  @note Returns the cache entry not the cipher/digest objecty so we can check whether it's blacklisted
  before calling the underlying functions
  @note the caches have to be populated. That's done when we create a context.
  */
static MD_CACHE *cache_lookup(MD_CACHE *base, int n, const char *name) {
  MD_CACHE *tmp = NULL;
  MD_CACHE mymd; /* Dummy for the compare function */
  if(NULL != name) {
    mymd.name = name;
    tmp = bsearch(&mymd, base, n, sizeof(MD_CACHE), mycmp);
  }
  return tmp;
}


/*! @brief Init caches for the current contexts 
    @param An ICC context pointer
    @note this only being called when necessary is dealt with elsewhere in the code
        but it should be harmless anyway as repeated calls to the sort leave the data in
        the same place. (FLW)
*/


static void init_caches(WICC_CTX *wctx) 
{
#if defined(HAVE_C_ICC)  
  if(wctx->Cctx) {
    init_cache(wctx->Cctx,C_diglist,sizeof(C_diglist)/sizeof(MD_CACHE),(NAMEFUNC)ICCC_EVP_get_digestbyname);
    init_cache(wctx->Cctx,(MD_CACHE *)C_ciplist,sizeof(C_ciplist)/sizeof(MD_CACHE),(NAMEFUNC)ICCC_EVP_get_cipherbyname);
  } 
#endif
#if defined(HAVE_N_ICC)  
  if(wctx->Nctx) {
    init_cache(wctx->Nctx,N_diglist,sizeof(N_diglist)/sizeof(MD_CACHE),(NAMEFUNC)ICCN_EVP_get_digestbyname);
    init_cache(wctx->Nctx,(MD_CACHE *)N_ciplist,sizeof(N_ciplist)/sizeof(MD_CACHE),(NAMEFUNC)ICCN_EVP_get_cipherbyname);
  } 
#endif  
}



#undef ICC_EVP_get_digestbyname
#if defined(JGSK_WRAP)
const ICC_EVP_MD * JCC_EVP_get_digestbyname(ICC_CTX *ctx, const char *name) 
#else
#if defined(ICKC_WRAP)
const ICC_EVP_MD* ICKC_EVP_get_digestbyname(ICC_CTX* ctx, const char* name)
#else
const ICC_EVP_MD * ICC_EVP_get_digestbyname(ICC_CTX *ctx, const char *name) 
#endif
#endif
{
  WICC_CTX *wctx = (WICC_CTX *)ctx;
  const ICC_EVP_MD *rv = NULL;
  MD_CACHE *tmp = NULL;
#if defined(HAVE_C_ICC)
  if (wctx->Cctx) {
    tmp = cache_lookup( C_diglist, sizeof(C_diglist) / sizeof(MD_CACHE), name);
    if (NULL != tmp) {
      if (0 == tmp->block) {
        rv = (ICC_EVP_MD *)tmp->md;
      }
    } else {
      rv = ICCC_EVP_get_digestbyname(wctx->Cctx,name);
    }
  }
#endif
#if defined(HAVE_N_ICC)
  if (wctx->Nctx) {
    tmp = cache_lookup(N_diglist, sizeof(N_diglist) / sizeof(MD_CACHE), name);
    if (NULL != tmp) {
      if (0 == tmp->block) {
        rv = tmp->md;
      }
    } else {
      rv = ICCN_EVP_get_digestbyname(wctx->Nctx,name);
    }
  }
#endif  
  return rv;
}


#undef ICC_EVP_get_cipherbyname
#if defined(JGSK_WRAP)
const ICC_EVP_CIPHER *JCC_EVP_get_cipherbyname(ICC_CTX *ctx, const char *name) 
#else
#if defined(ICKC_WRAP)
const ICC_EVP_CIPHER* ICKC_EVP_get_cipherbyname(ICC_CTX* ctx, const char* name)
#else
const ICC_EVP_CIPHER *ICC_EVP_get_cipherbyname(ICC_CTX *ctx, const char *name) 
#endif
#endif
{
  WICC_CTX *wctx = (WICC_CTX *)ctx;
  const ICC_EVP_CIPHER *rv = NULL;
  MD_CACHE *tmp = NULL;
#if defined(HAVE_C_ICC)  
  if (wctx->Cctx) {
    tmp = cache_lookup((MD_CACHE *)C_ciplist, sizeof(C_ciplist) / sizeof(CIP_CACHE), name);
    if (NULL != tmp) {
      if (0 == tmp->block) {
        rv = (const ICC_EVP_CIPHER *)tmp->md;
      }
    } else {
      rv = ICCC_EVP_get_cipherbyname(wctx->Cctx,name);
    }
  }
#endif
#if defined(HAVE_N_ICC)  
  if (wctx->Nctx) {
    tmp = cache_lookup((MD_CACHE *)N_ciplist, sizeof(N_ciplist) / sizeof(CIP_CACHE), name);
    if (NULL != tmp) {
      if (0 == tmp->block) {
        rv = (const ICC_EVP_CIPHER *)tmp->md;
      }
    } else {
      rv = ICCN_EVP_get_cipherbyname(wctx->Nctx,name);
    }
  }
#endif  
  return rv;
}

#if defined(STANDALONE)
#include "delta_t.h"

static void coverage(ICC_CTX *ctx, MD_CACHE *base, int n, NAMEFUNC func) {
  int i;
  unsigned long t1;
  unsigned long t2;
  unsigned long tc = 0, tnc = 0;
  int count = 0;
  MD_CACHE *tmp = NULL;
  const ICC_EVP_MD *md = NULL;

  for (i = 0; i < n ; i++) {
    Delta_T(1, &t1);
    tmp = cache_lookup(base, n, base[i].name);
    t1 = Delta_T(0, &t1);
    if ((NULL == tmp) || (NULL == tmp->md)) {
      printf("Cache miss %s\n", base[i].name);
    }
    tc += t1;
    Delta_T(1, &t2);
    md = (func)(ctx, base[i].name);
    t2 = Delta_T(0, &t2);
    printf("c %ld nc %ld\n", t1, t2);
    tnc += t2;
    if (NULL != tmp) {
      if (tmp->md != md) {
        printf("Cache object mismatch (%s) !\n", base[i].name);
      }
    }
    count++;
  }
  tc /= count;
  tnc /= count;
  printf("Averages c %ld nc %ld ratio tc/tnc %f\n", tc, tnc, (double)tc / tnc);
}

static void cache_test(WICC_CTX *wctx) {
#if defined(HAVE_C_ICC)  
    if(wctx->Cctx) {
      printf("FIPS digests\n");
      coverage(wctx->Cctx,C_diglist,sizeof(C_diglist)/sizeof(MD_CACHE),(NAMEFUNC)ICCC_EVP_get_digestbyname);
      printf("FIPS ciphers\n");
      coverage(wctx->Cctx,(MD_CACHE *)C_ciplist,sizeof(C_ciplist)/sizeof(MD_CACHE),(NAMEFUNC)ICCC_EVP_get_cipherbyname);
    }
#endif
#if defined(HAVE_N_ICC)    
    if(wctx->Nctx) {
      printf("Non FIPS digests\n");
      coverage(wctx->Nctx,N_diglist,sizeof(N_diglist)/sizeof(MD_CACHE),(NAMEFUNC)ICCN_EVP_get_digestbyname);
      printf("Non FIPS ciphers\n");
      coverage(wctx->Nctx,(MD_CACHE *)N_ciplist,sizeof(N_ciplist)/sizeof(MD_CACHE),(NAMEFUNC)ICCN_EVP_get_cipherbyname);
    }
#endif    
}

#endif
