/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Utility functions used in SP800-90 (and other) code
//              It's isolated here so it's easilly testable
//
*************************************************************************/


#include "utils.h"

#if defined(STANDALONE)
#include <stdio.h>
#include <string.h>



static unsigned char zero[4] = {0x00,0x00,0x00,0x00};
static unsigned char minus1[4] = {0xff,0xff,0xff,0xff};

static unsigned char s1[4] = {0x00,0x01,0x02,0x03};
static unsigned char s3[4] = {0x01,0xff,0xff,0xff};
static unsigned char s4[4] = {0xff,0xff,0xff,0x01};
static unsigned char s5[4] = {0xa5,0xa5,0xa5,0xa5};
static unsigned char s6[4] = {0x5a,0x5a,0x5a,0x5a};


static unsigned char x3[4] = {0xff,0xfe,0xfd,0xfc};


static unsigned char al1[4] = {0xff,0x00,0x02,0x03};
static unsigned char le_one[4] = {0x01,0x00,0x00,0x00};
static unsigned char al2[4] = {0x02,0xff,0xff,0xff};
static unsigned char al3[4] = {0x01,0x01,0x02,0x03};
static unsigned char al4[4] = {0x00,0x00,0x00,0x02};
static unsigned char al5[4] = {0xff,0x00,0x03,0x03}; /* LE s1 + 1st 2bytes of -1) */
static unsigned char al6[4] = {0xff,0x00,0x00,0x00}; /* LE -1 + 1st 2 bytes of s1 */


static unsigned char ab1[4] = {0x01,0x00,0x02,0x02};
static unsigned char be_one[4] = {0x00,0x00,0x00,0x01};
static unsigned char ab2[4] = {0x00,0x01,0x02,0x02};
static unsigned char ab3[4] = {0x00,0x01,0x02,0x04};
static unsigned char ab4[4] = {0x02,0x00,0x00,0x00};
static unsigned char ab5[4] = {0xff,0xff,0xff,0x02};
static unsigned char ab6[4] = {0x00,0x02,0x02,0x02}; /* BE s1 + 1st 2 bytes of -1 */
static unsigned char ab7[4] = {0x00,0x00,0x02,0x02}; /* BE -1 + 1st 2 bytes of s1 */


int cmp(unsigned char *s1,unsigned const char *s2,int n, char *test)
{
  int i;
  if(memcmp(s1,s2,n) != 0) {
    printf("Test %s failed\nExp =",test);
    for(i = 0; i < n; i++) {
      printf("0x%02x,",s2[i]);
    }
    printf("\nGot =");
    for(i = 0; i < n; i++) {
      printf("0x%02x,",s1[i]);
    }
    printf("\n");
  }
}
int main(int argc,char *argv[])
{
  unsigned char r[4];
  xor(r,s1,s1,4);
  cmp(r,zero,4,"xor 1");
  xor(r,minus1,minus1,4);
  cmp(r,zero,4,"xor 2");
  xor(r,s1,minus1,4);
  cmp(r,x3,4,"xor 3");
  
  Add_LE(r,zero,4,le_one,4);
  cmp(r,le_one,4,"Add_LE 1");

  Add_LE(r,minus1,4,le_one,4);
  cmp(r,zero,4,"Add_LE 2");

  Add_LE(r,s1,4,minus1,4);
  cmp(r,al1,4,"Add_LE 3");

  Add_LE(r,minus1,4,s1,4);
  cmp(r,al1,4,"Add_LE 4");

  /* Now the one byte add, used for increments */
  Add_LE(r,minus1,4,le_one,1);
  cmp(r,zero,4,"Add_LE 5");

  Add_LE(r,zero,4,le_one,1);
  cmp(r,le_one,4,"Add_LE 6");

  Add_LE(r,le_one,4,s3,4);
  cmp(r,al2,4,"Add_LE 7");
 
  Add_LE(r,s3,4,le_one,1);
  cmp(r,al2,4,"Add_LE 8");
 
  Add_LE(r,le_one,4,s1,4);
  cmp(r,al3,4,"Add_LE 9");

  Add_LE(r,le_one,4,s4,4);
  cmp(r,al4,4,"Add_LE 10");
 
  Add_LE(r,s4,4,le_one,1);
  cmp(r,al4,4,"Add_LE 11");

  /* Test our convenience usage, len i2 == len i1 */
  Add_LE(r,s1,4,minus1,0);
  cmp(r,al1,4,"Add_LE 3");

  Add_LE(r,minus1,4,s1,0);
  cmp(r,al1,4,"Add_LE 4");

  /* Test partial adds (> 1 byte) */
  Add_LE(r,s1,4,minus1,2);
  cmp(r,al5,4,"Add_LE s1 + -1(2)");
  Add_LE(r,minus1,4,s1,2);
  cmp(r,al6,4,"Add LE -1 + s1(2)");

  /* Big endian adds */

  Add_BE(r,zero,4,be_one,4);
  cmp(r,be_one,4,"Add_BE 1");

  Add_BE(r,minus1,4,be_one,4);
  cmp(r,zero,4,"Add_BE 2");

  Add_BE(r,s1,4,minus1,4);
  cmp(r,ab2,4,"Add_BE 3");

  Add_BE(r,minus1,4,s1,4);
  cmp(r,ab2,4,"Add_BE 4");

  /* Now the one byte add, used for increments */
  Add_BE(r,minus1,4,le_one,1);
  cmp(r,zero,4,"Add_BE 5");

  Add_BE(r,zero,4,le_one,1);
  cmp(r,be_one,4,"Add_BE 6");

  Add_BE(r,be_one,4,s1,4);
  cmp(r,ab3,4,"Add_BE 7");

  Add_BE(r,s3,4,le_one,1);
  cmp(r,ab4,4,"Add_BE 8");
 
  Add_BE(r,be_one,4,s1,4);
  cmp(r,ab3,4,"Add_BE 9");

  Add_BE(r,be_one,4,s4,4);
  cmp(r,ab5,4,"Add_BE 10");
 
  Add_BE(r,s4,4,le_one,1);
  cmp(r,ab5,4,"Add_BE 11");

  /* Test our convenience usage, len i2 == len i1 */
  Add_BE(r,be_one,4,s4,0);
  cmp(r,ab5,4,"Add_BE 12");

  Add_BE(r,be_one,4,s1,0);
  cmp(r,ab3,4,"Add_BE 13");

  /* Partial adds, Big endian */
  /* Be careful here,it's CONFUSING!
     we point to the start of an N byte BE buffer 
     NOT to the end of the 4 byte buffers and count back
     i.e. remember your buffers are only the specified count long
  */
  Add_BE(r,s1,4,minus1,2); /* BE add s1 + -1(2) */
  cmp(r,ab6,4,"Add_BE s1 + -1(2)");

  Add_BE(r,minus1,4,s1+2,2); /* BE add -1 + (s1+2)(2) */
  cmp(r,ab7,4,"Add_BE -1 + s1(2)");


  Add_BE(r,minus1,4,s1,2); /* BE add -1 + s1(2) */
  cmp(r,zero,4,"Add_BE -1 + s1(2)");

  

}

#endif

/** @brief xor two buffers into a destination. 
    (Which may be one of the source buffers )
    @param dest the destination buffer
    @param s1 first source buffer
    @param s2 second source buffer
    @param blen buffer length
*/

void xor(unsigned char *dest, unsigned char *s1, unsigned char *s2, unsigned blen)
{
  unsigned int i;
  for(i = 0; i < blen; i++) {
    dest[i] = s1[i] ^ s2[i];
  }
}

/*!
  @brief  Binary add src1 + src2 result in dest
  Numbers are assumed little endian
  @param dest destination,may be the same as src1 or src2
  @param src1 source data 1
  @param s1 length of s1 and dest
  @param src2 source data 2
  @param s2 length of src2
  @note s2 may be set to 0, in which case s1 is used for s2.
  s2 = 1 and src->0, A single "1" byte is typical for an increment.
*/
void Add_LE(unsigned char *dest,
	 unsigned char *src1,unsigned int s1, 
	 unsigned char *src2,unsigned int s2)
{
  unsigned int t = 0,t1 = 0,t2 = 0;
  int i = 0;
  unsigned int cy = 0;
  if(0 == s2) s2 = s1;

  for(i = 0; i < (int)s1 ; i++) {
    t1 = src1[i];
    t2 = 0;
    if( i < (int)s2 ) {
      t2 = src2[i];
    }
    t = t1 + t2 +cy;     
    cy = 0;
    if(t > 255 ) {
      cy = 1;
    }
    dest[i] = t & 0xff;
  }
}


/*!
  @brief  Binary add src1 + src2 result in dest
  Numbers are assumed big endian.
  READ the details of the implementation. There are some quirks.
  @param dest destination,may be the same as src1 or src2
  @param src1 source data 1
  @param s1 length of s1 and dest
  @param src2 source data 2
  @param s2 length of src2
  @note s2 may be set to 0, in which case s1 is used for s2.
  s2 = 1 and src->0, A single "1" byte is typical for an increment.
*/
void Add_BE(unsigned char *dest,
	 unsigned char *src1,unsigned int s1, 
	 unsigned char *src2,unsigned int s2)
{
  unsigned int t = 0,t1 = 0,t2 = 0;
  int i = 0;
  unsigned int cy = 0;
  if(0 == s2) s2 = s1;
  
  for(i = 1; i <= (int)s1 ; i++) {
    t1 = src1[s1-i];
    t2 = 0;
    if( ((int)s2 - i) >= 0) {
      t2 = src2[s2 - i];
    }
    t = t1 + t2 + cy;     
    cy = 0;
    if(t > 255 ) {
      cy = 1;
    }
    dest[s1-i] = t & 0xff;
  }
}

/*!
  @brief hunt for uninitialized data
  valgrind will tell us when we use uninitialized memory
  This allows for us to scan for the uninitialized blocks
  by inserting memchk() calls into the source until we find the
  UMR source
  @param data The data to scan
  @param n the number of bytes
  \debug This is a debug routine. Left in place because it's
  very short and doesn't expose data.
*/
int memchk(unsigned char *data,int n)
{
  int x =0;
  int i = 0;
  for(i = 0; i< n ; i++) {
    if(data[i]) x++; /* Conditional test on memory */
  }
  return x;
}
