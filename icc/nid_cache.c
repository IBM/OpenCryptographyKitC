/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*
  Name/NID caches for digests/ciphers
  mainly used for the FIPS algorithm callback
  though there may also be a small performance gain if the callback isn't active

   Note that this creates TWO caches, one to speed name lookups 
  for common objects.
  The other is nid sorted and used for FIPS allowed algorithm lookups

  Note: We need both because the names can have alias's and the FIPS algorithm
  lookup has to work for all the alias's. 
  So lookup name->md (as per normal), if we get an md and the callback
  is enabled then md->nid and check nid is FIPS allowed.

 The name cache is relatively obvious, the nid cache less so. 
        The nid cache ends up populated with nids sorted by order for all the known FIPS allowed ciphers and digests
        This is complete BY NID as the nids (unlike the names) don't have aliases.
        IF we find the name in the name cache, we have it's nid and FIPS status directly from the found object
        ELSE we call down to OpenSSL, then we check the nid of the returned EVP_MD against the nid sorted cache
        because it could have been an alias without an entry in the name cache.
*/


#define HAVE_C_ICC

#include "name_cache_tables.c"

#undef HAVE_C_ICC
/* Enable for testing, doesn't use the cache 
 * callbacks with still run, but no FIPS indicators
#define RAW
*/
/* Create the space for the NID sorted variants
*/
static MD_CACHE md_nids[sizeof(C_diglist)/sizeof(MD_CACHE)];
static MD_CACHE cip_nids[sizeof(C_ciplist)/sizeof(MD_CACHE)];



/* name->md, Use case insensitive string compare 
  works with either MD or Cipher as the cache objects being manipulated 
  are the same size and contain the same size objects
  Note that alias's are allowed and this will miss alias's
*/
static int mycmp(const void *p1, const void *p2)
{
    return strcasecmp(((MD_CACHE *)p1)->name,((MD_CACHE *)p2)->name);
}

/* nid->fips, 
  works with either MD or Cipher as the cache objects being manipulated 
  are the same size and contain the same size objects

*/
static int myNIDcmp(const void *p1, const void *p2)
{
    return ((MD_CACHE *)p1)->nid - ((MD_CACHE *)p2)->nid;
}
/* Callback for ICC_EVP_getXYZbyname() calls */

typedef void * (*NAMEFUNC)(const char *name);
/* Callback for EVP_MD_type, EVP_CIPHER_type calls */
typedef int (*NIDFUNC)(const void *thing);

/* Generic cache setup, basically sort the entries by name 
  grab the nid and stash that
  sort the name cache, copy the data to the nid cache and sort that as well.
  Note we have in effect TWO caches here, one sorted by name, the other by NID
*/

static void init_caches(MD_CACHE *dig,MD_CACHE *nids,int n,NAMEFUNC func,NIDFUNC nidfunc)
{
    int i;
    for(i = 0; i < n;i++) {
        dig[i].md = (*func)(dig[i].name);
        if(NULL != dig[i].md) {
          dig[i].nid = (*nidfunc)(dig[i].md);
        }
    }    
    qsort(dig,n,sizeof(MD_CACHE),mycmp);     /* Sort the name cache */
    memcpy(nids,dig,sizeof(MD_CACHE)*n);     /* Copy the populated name data to the nid cache */
    qsort(nids,n,sizeof(MD_CACHE),myNIDcmp); /* Sort that one by NID */
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
static MD_CACHE *name_cache_lookup(MD_CACHE *base, int n, const char *name) {
  MD_CACHE *tmp = NULL;
  MD_CACHE mymd; /* Dummy for the compare function */
  if(NULL != name) {
    mymd.name = name;
    tmp = bsearch(&mymd, base, n, sizeof(MD_CACHE), mycmp);
  }
  return tmp;
}

/*! 
  @brief Lookup a cache entry by nid
  @param base of the cache to search
  @param n number of entries in this cache
  @param nid the nid of the object to locate
  @return the cache entry or NULL on no match
  @note there's an amount of overloading here.
  @note Returns the cache entry not the cipher/digest object so we can check attributes before returning
  @note the caches have to be populated. That's done when we create a context.
  */
static MD_CACHE *NID_cache_lookup(MD_CACHE *base, int n, int nid) {
  MD_CACHE *tmp = NULL;
  MD_CACHE mymd; /* Dummy for the compare function */
  if(0 != nid) {
    mymd.nid = nid;
    tmp = bsearch(&mymd, base, n, sizeof(MD_CACHE), myNIDcmp);
  }
  return tmp;
}


/*! @brief Init caches for the common digests and ciphers , by name and by nid
    @note this only being called when signle threaded is dealt with elsewhere in the code
        but it should be harmless anyway as repeated calls to the sort leave the data in
        the same place. (FLW)
*/

static void init_name_caches()
{
  init_caches(C_diglist, md_nids,sizeof(C_diglist)/sizeof(MD_CACHE),(NAMEFUNC)EVP_get_digestbyname,(NIDFUNC)EVP_MD_type);
  init_caches((MD_CACHE *)C_ciplist,cip_nids,sizeof(C_ciplist)/sizeof(MD_CACHE),(NAMEFUNC)EVP_get_cipherbyname,(NIDFUNC)EVP_CIPHER_type);
}

/*! @brief Check if a message digest is FIPS approved 
    @param nid the NID to lookup
    @return 1 for FIPS 0 otherwise
*/
static int FIPS_MDbyNID(int nid)
{
  int fips = 0;
  MD_CACHE *tmp = NULL;
  tmp = NID_cache_lookup(md_nids,sizeof(C_diglist) / sizeof(MD_CACHE), nid);
  if(tmp != NULL) {
    fips = tmp->fips;
  }
  return fips; 
}

/*! @brief Check if a cipher is FIPS approved 
    @param nid the NID to lookup
    @return 1 for FIPS 0 otherwise
*/
static int FIPS_CipherbyNID(int nid)
{
  int fips = 0;
  MD_CACHE *tmp = NULL;
  tmp = NID_cache_lookup(cip_nids,sizeof(C_ciplist) / sizeof(MD_CACHE), nid);
  if(tmp != NULL) {
    fips = tmp->fips;
  }
  return fips; 
}

const EVP_MD * my_EVP_get_digestbyname(ICClib *pcb, const char *name)
{
  const EVP_MD *rv = NULL;
  MD_CACHE *tmp = NULL;
  int nid = 0;
  int fips = 0;
#if defined(RAW)
  rv = EVP_get_digestbyname(name);
  if(NULL != rv) {
    nid = EVP_MD_type(rv); 
  }
#else
  tmp = name_cache_lookup(C_diglist, sizeof(C_diglist) / sizeof(MD_CACHE), name);
  if (NULL != tmp) {
    rv = (const EVP_MD *)tmp->md;
    fips = tmp->fips;
    nid = tmp->nid;
  } else { /* This is where we need to check the nid cache, aliases for the digest names */
    rv = EVP_get_digestbyname(name);
    if(NULL != rv) {
      nid = EVP_MD_type(rv);
      tmp = NID_cache_lookup(md_nids,sizeof(C_diglist) / sizeof(MD_CACHE), nid);
      if(tmp != NULL) {
        fips = tmp->fips;
      }
    }
  }
#endif  
  /* If the MD is NULL, don't do the callback */
  if ((NULL != pcb->callback) && (NULL != rv)) {   
      (pcb->callback)("EVP_get_digestbyname",nid,fips);
  }   
  return rv;
}

const EVP_CIPHER * my_EVP_get_cipherbyname(ICClib *pcb, const char *name)
{
  const EVP_CIPHER *rv = NULL;
  MD_CACHE *tmp = NULL;
  int nid = 0;
  int fips = 0;
#if defined(RAW)
  rv = EVP_get_cipherbyname(name);
  if(NULL != rv) {
    nid = EVP_CIPHER_type(rv);
  }
#else  
  tmp = name_cache_lookup((MD_CACHE *)C_ciplist, sizeof(C_ciplist) / sizeof(CIP_CACHE), name);
  if (NULL != tmp) {
    rv = (const EVP_CIPHER *)tmp->md;
    fips = tmp->fips;
    nid = tmp->nid;
  } else { /* This is where we need to check the nid cache, aliases for the cipher names */
    rv = EVP_get_cipherbyname(name);
    if (NULL != rv) {
      nid = EVP_CIPHER_type(rv);
      tmp = NID_cache_lookup(cip_nids, sizeof(C_ciplist) / sizeof(CIP_CACHE), nid);
      if (tmp != NULL) {
        fips = tmp->fips;
      }
    }
  }
#endif   
  /* If the MD is NULL, don't do the callback */
  if ((NULL != pcb->callback) && (NULL != rv)) {
      (pcb->callback)("EVP_get_cipherbyname",nid,fips);
  }
 
  return rv;
}


