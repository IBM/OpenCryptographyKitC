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


void xor(unsigned char *dest, unsigned char *s1, unsigned char *s2, unsigned blen);
void Add_LE(unsigned char *dest,
	    unsigned char *src1,unsigned int s1, 
	    unsigned char *src2,unsigned int s2);

void Add_BE(unsigned char *dest,
	    unsigned char *src1,unsigned int s1, 
	    unsigned char *src2,unsigned int s2);

/* Assume the NIST spec uses a Big Endian representation of bit streams */
#define Add(a,b,c,d,e) Add_BE(a,b,c,d,e)

/*!
  @brief hunt for uninitialized data
  valgrind will tell us when we use uninitialized memory
  this allows for us to scan for the uninitialized block to find it's creator
  
*/
int memchk(unsigned char *data,int n);
