/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Unit test for ICC
// 
*************************************************************************/

#if !defined(NIST_ALGS_H)
#define NIST_ALGS_H



#if defined(STANDALONE)

/* Defines for standalone testing of the entropy core
   independant of ICC itself \sa GenRndData
*/   
#define ICC_Calloc(a,b,c,d) calloc(a,b)
#define ICC_Free(a) free(a)

#endif
/* Enough data to run the NIST health tests */
#define E_ESTB_BUFLEN 512

/* Maximum number of contiguous ht test fails allowed 
   in the first phase of entropy collection.
   Note that we use 'test and discard' to drop batches of low
   entropy data. Failures are expected here but we also want
   some way out of there's a total loss of function so that we get
   an error rather than a hang.
   Yes, it's a large number but its still < 1 minute real time.
*/
#define MAX_HT_FAIL 9



/*! @brief
 Data structure for NIST SP800-90A Adaptive proportion test
 Assumptions 8 bit samples, entropy guarantee > 0.5 bits/bit
*/
/*! @brief 
  Collected data structures for Entropy source health tests 
*/
typedef struct ENTROPY_HT_t {
  int H;       /*!< Entropy guarantee we were created with 25/50/75 */
  int e;       /*!< Entropy estimage from last call */
  const char *id;    /*!< Debug string */
} ENTROPY_HT;

/* 
  Variants of the alg, 8 bit and without the running buffer
*/


unsigned int pmaxLGetEnt(unsigned char *data, int len);

/*! @brief Initialize a an Entropy health test structure, setting it up
  for the appropriate entropy guarantees
  @param E a pointer to the entropy health test data structure
  @param H the desired entropy guarantee as a % in 8 bits
  i.e. 25,50,75
  @return 1 on sucess, 0 on failure
*/
int ht_Init(ENTROPY_HT *HT, int H);


/*! @brief  Run the health tests on the data block
   @param HT a pointer to a TRNG health test data structure
   @param data a block of generated data
   @return 0 if O.K., !0 otherwise
*/
int ht(ENTROPY_HT * HT, unsigned char data[E_ESTB_BUFLEN]);



/*!
  @brief Continuous entropy estimate based on Appendix C.3 SP800-90
  Returns entropic bits out/100 bits in over a 4 bit space
  Very coarse approx.i.e. 0:0-24,25:25-49,50:50-74,75:75-99,100:100 and is conservative.
  And yes there are five values reported. Log functions :{
  @param data data in
  @param len length of data (min 32 bytes)
  @note returns 0 if too little data is provided
*/

int pmax4(unsigned char *data,int len);




/*!
  @brief run the Adaptive proportion test
  @param N the % entropy target
  @param data the buffer to test
  @return 0 O.K., !0 fail
*/
int APtestBK(int N,unsigned char data[512]);
/*!
  @brief run the Repeat count test on a block of data
  @param H the % entropy required
  @param data a buffer full of data
  @return 0 O.K., !0 fail
*/
int RCtestBK(int H,unsigned char data[512]);

int pmax4Tests();
int APTests();
int RCTests();

#endif
