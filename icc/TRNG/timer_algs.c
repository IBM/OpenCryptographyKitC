
/*-----------------------------------------------------------------------------
// Copyright IBM Corp. 2023
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
//
// Description: Code for the time specific entropy source
//              Refactored from the original egather.c
//              - which just grew and grew and grew 
*******************************************************************************/

#if !defined(STANDALONE)
#include "platform.h"
#endif
#include "TRNG/nist_algs.h"
#include "TRNG/timer_entropy.h"

#include "induced.h"


unsigned long RdCTR_raw();
unsigned char getbyte_i(int xloops,MINIBUF *b );
unsigned long RdCTR();
/*
  Switch to the new tuner only on the platforms where we know it helps.
  AIX, PPC Linux, HP/UX Itanium
*/
#if (defined(_AIX) || (defined(__linux) && defined( __PPC__)) || (defined(__hpux) && defined(__ia64__)) ) 
int default_tuner = 2;
#else
int default_tuner = 1;
#endif


/* Controls forcing full tuning for TRNG_ALT/TRNG_ALT2 */
static int full_rng_setup = 0; 

#if !defined(HIGH_RES_TIMER)
static int bitter(void);
static int bitter1(void);
static int bitter2(void);
int pmax(unsigned char in,E_EST *e);
unsigned char getbyte(MINIBUF *b,MINIBUF *tr);
unsigned char getbyte_alt(MINIBUF *b);


/*! @brief This table defines the loop counts we'll try to get entropy 
     Why do we stop ?, because if we get to the end of this table, 
     it's going to take too long anyway. 
     If it's really bad, we hang in the later distribution tests in
     the TRNG. This tuning is aimed at avoiding the pathological cases
     later, rather than obtaining perfect tuning here.
     - And yes they are primes so that if we settle into a repeating pattern
     it's at least a long one.
*/
static const int ptable[] = {1,2,3,7,17,31,61,127,251,509,1021,2017};


#endif
/* The following two come up as defined but unused of some platforms
   but are needed on ARM
*/
static int count = 0;  /*!< Counter for induced failure test 207 */


/*! @brief mask for the bits "removed" from the low end of the counter 
    because they aren't very random - however they do usually change fast 
    and very unpredictably when observed from another process so it's 
    worth mixing them back in to gain resistance to timing attacks.
*/
static const unsigned long marray[16] = {
  0x0000,0x0001,0x0003,0x0007,0x000f,0x001f,0x003f,0x007f,
  0x00ff,0x01ff,0x03ff,0x07ff,0x0fff,0x1fff,0x3fff,0x7fff
};

/* Delay selection bit mask, 0 - no delay, 1 = close(-1), 2 = open_bad_f(), 4 = Jent */
static int G_useDelay = 4;

void setDelay(int d)
{
    G_useDelay = d;
}


/*!
  @brief 
  In some pathological cases (i.e. virtualized) the dynamic shift calculation
  can be useless because the timers are so unstable. if ex_loops != -1 
  bitter*() will simply set loops = ex_loops and return
  ex_loops is set via the environment or the config file. i.e. ICC_LOOPS=
*/
int ex_loops = -1;


extern int loops;
static int timer_ok = 0;



/*!
  @brief one off entry point for full noise source tuning
  - remove stuck bits
  - optimize sampling interval to maximum jitter
*/
static int tuned = 0;
static int tuned_alt = 0;

/*! 
  @brief PREPENTROPY is a macro defining how the CPU event counter data
  is conditioned to allow for variations on different archtectures,
  this is used to create the entropy source. See \ref trng_raw()  
  - Generally this removes bits which are "stuck at" or very badly biased
  leaving us a low bit which behaves like a coin toss.
  - There are two tuning aspects here. 
    - Removing any stuck bits. See \ref CalcShift()
    - Tweaking the number of instructions between samples to try
      to get near a 50% "heads or tails" distribution on our 
      low (coin toss) bit. See \ref bitter1()
  @note The definition below is a dummy to generate documentation
  see platform specific sections of the code for individual OS's
*/
#define DOC_PREPENTROPY 

/*! 
  @brief PREPENTROPYALT is a macro  defining how CPU event counter
  data to be mixed with OS "/dev/random" sources is to be conditioned
  to create the entropy source. See \ref getbyte_alt()
  - Generally this only removes bits which are "stuck at" 
    or very badly biased.See \ref CalcShift()
  @note The definition below is a dummy to generate documentation
  see platform specific sections of the code for individual OS's
*/
#define DOC_PREPENTROPYALT 

/* 
   Messy platform specific entropy gathering code
   The default usesdeltas on the number of increments of a variable per clock() tick
   The platform specific variants uses delta's of the CPU instruction counter
   if there's one available.


   RdCTR_raw() is a probe routine for test code we use to make sure these things
   actually return valid bits. See the s390/zSeries section for a reason why
   this is a good idea.

   PREPENTROPY does any setup required for the entropy source

   MIN_SHIFT defines the minimum data shift required to remove dead bits

   This has to be included as the last "header" file, it may include other headers,
   or define functions
   
   Note: This uses the 'native' compiler definitions to find platform -
         NOT 'OPSYS' That's deliberate so we can simply compile the testCTR
         program on the native platform and run it.

   Note: Virtualization. 
   The move to virtualization left us with configurations with
   little usable entropy. In this case it's possible to configure a fallback - 
   which is to use /dev/urandom as a source. Since we don't totally trust this we
   do mix in high res counter data to make us less weak WRT to attacks on the OS source.
   Note we use /dev/urandom as many /dev/random implementations suck the life out of an
   entropy pool and will stall in low entropy situations. 
   If we can't get any entropy the OS is unlikely to either.
   This is not a great situation to be in - but this appears to be the best we can do in
   these environments.
   Eventually we'd hope that the hypervisors would maintain an entropy pool for the hosted 
   OS's - in which case /dev/urandom becomes quite a good source.
*/

/* Prototype for platforms which have alternate entropy sources 
   We set the pointer to the entropy source up once at startup
   on those platforms.
*/
typedef unsigned long (*RDCTR)(void);


unsigned long CalcShift(int);

/*!
  @brief 
  In some pathological cases (i.e. virtualized) the dynamic shift calculation
  can be useless because the timers are so unstable. if ex_shift != -1 && ex_shift <16
  CalcShift() will simply set shift = ex_shift and return
  ex_shift is set via the environment or the config file. i.e. ICC_SHIFT=
*/
int ex_shift = -1;

/*! 
  @brief
  Some cycle counter implementations have 'dead bits' at the low
  end of the counter. (S390). We need to shift these out to usable
  bits. This is the shift count required.
*/
static int shift; 
/*!
  @brief
  This is the minimum data shift we have to use in the TRNG routine,
  anything less than this and the low bit is "stuck at"
*/
static int shift_min;
/*!
  @brief
  This is a fix for systems with a timebase clock asynchronous 
  from the CPU clock. In these systems we get beat frequencies
  in the sampled data.
  This is the maximum bit shift we can use, anything more than this 
  and the bit rate is less than one bit change/sample.
*/
static int shift_max;


int Set_rng_setup(int i)
{
  full_rng_setup = i;
  return i;
}

/*
  Now all the platform specific means of reading high speed counters
*/

/* 
   START ia32 Linux, OS/X
 */
#if (defined(__linux__) &&  defined(__i386__) ) || (defined(__APPLE__) && defined(__i386__))
unsigned long RdCTR_raw() {
    unsigned long lo;
    __asm__ __volatile__("rdtsc\n" : "=a" (lo) : : "edx");
    return lo;
}

  

/* END ia32 Linux, OS/X */

/* START Linux ia64 */
#elif defined(__linux__) && defined(__ia64__)

unsigned long RdCTR_raw() {
    unsigned long lo;
    __asm__ __volatile__("mov %0=ar.itc" : "=r"(lo));
    return lo;
}

/* START HP/UX Itanium with gcc */
#elif defined(__hpux) && defined(__GNUC__)  &&  defined(__ia64) 
unsigned long RdCTR_raw() {
    unsigned long lo;
    __asm__ __volatile__("mov %0=ar.itc" : "=r"(lo));
    return lo;
}

/* END HP/UX Itanium with gcc */

/* START HP/UX Itanium with aCC */
#elif defined(__hpux) &&  defined(__ia64)
#include <machine/sys/inline.h>

#define __TICKS _Asm_mov_from_ar(_AREG_ITC)
unsigned long RdCTR_raw() {     
    return __TICKS;
}
/*! @brief Set the minimum shift on this HP/IX to 3
   This is because the instruction count difference is a
   multiple of 8 in the code we execute
   UNLESS an interrupt occurs - which can
   shift us by an odd number of counts 
*/
#define MIN_SHIFT 3
/*! @brief \sa DOC_PREPENTROPY
 */
#define PREPENTROPY  {    \
    CalcShift(MIN_SHIFT); \
    loops = bitter();    \
  }

/* END HP/UX Itanium with aCC */

/* Start X86_64 */
/* START  X86_64 Linux, OS/X */
#elif defined(__linux__) && defined(__x86_64__)   || (defined(__APPLE__) && defined(__x86_64__)) 

unsigned long RdCTR_raw() {
    unsigned long lo;
    __asm__ __volatile__("rdtsc\n" : "=a" (lo) : : "edx");
    return lo;
}

/* END X86_64 Linux, OS/X */

/* START X86_64 Solaris SunPro compiler */
#elif defined(__SUNPRO_C) && defined(__amd64) 


/* UNTESTED, AMD-64 Solaris 10 , should use rdtsc later*/

unsigned long RdCTR_raw() {
  return gethrtime();
}

/* END X86_64 Solaris SunPro compiler */    
    
/* END X86_64 */

/* START x86, Solaris SunPro compiler */
#elif defined(__SUNPRO_C) && defined(__i386)

/* UNTESTED, Solaris x86 , should use rdtsc later*/

unsigned long RdCTR_raw() {
    return gethrtime();
}
  
/* END Solaris SunPro compiler */

/* PowerPC with GCC, Linux or OS/X - Note PPC on OS/X is dead now. */    
#elif (defined(__linux__) &&  defined(__PPC__)) || (defined(__APPLE__) && (defined(__ppc__) || defined(__ppc64__)))
/* 32 & 64 bit PPC Linux or Mac OS X on PPC*/
unsigned long RdCTR_raw() {
    unsigned long lo;

    __asm__ __volatile__("mftb %0\n" : "=r" (lo) : );
    return lo;
}


/*END defined(__linux__) &&  defined(__PPC__) */

/* START zSeries , Linux & Z/OS */
#elif (defined(__linux__) && defined(__s390__)) || defined(__MVS__)   
#include <stdint.h>
/* Cycle counter on s390/zSeries  */

/* Low XXX bits are constant on the machines we've tested this on,
   I assume they leave room for faster processors with more resolution in future
   Which is why the "odd" looking code.
   stcke returns 16 bytes, stck 8, stckf 8
*/
unsigned long CalcShift(int mn);
#if defined(__linux__)
unsigned long RdCTR_raw(void)
{
  uint32_t fred[4];

  __asm__("stcke 0(%0)" : : "a" (&(fred)) : "memory", "cc");
  return fred[1];
}
#elif defined(__MVS__)
#include <builtins.h> 
/* STCK1 provides more accuracy (but that may not be needed) 
   We want resolution and "unscrewed with" here more than 
   accuracy, so raw counts are preferred
*/
unsigned long RdCTR_raw(void)
{
  uint32_t fred[4];

  /* compile egather.c with -W "c,langlvl(extended)" */
  /* 
     We only use at most the 8 lowest moving bits
  */
  __stcke(&fred); 
  return fred[1]; 
}

#endif



/*! @brief This *was* different, s390 stck the low bit is known unusable, 
  hence the CalcShift(1) 
  Once we changed to stcke, this hack to avoid the 1Hz heartbeat in the LSBit
  of stck isn't needed.
*/

#define MIN_SHIFT 0
/*! @brief \sa DOC_PREPENTROPY
 */
#define PREPENTROPY  {    \
    CalcShift(MIN_SHIFT); \
    loops = bitter();    \
  }


/* END zSeries, Linux and Z/OS */
                

/* START AIX on PPC */
#elif defined(_AIX)
/* 32 and 64 bit AIX on PPC */

#include <sys/time.h>

extern int RdTBR();

static int tested = 0; /*!< Have we tested for the mode to use yet ? */

static unsigned long rdctrlo() {
  unsigned long l;
  l = RdTBR();
  return l;
}




static RDCTR rdctr = (RDCTR)rdctrlo;

unsigned long RdCTR_raw(void)
{
  return (*rdctr)();
}




/* END AIX */
#elif defined(__sun__) && defined(__i386__) && defined(__GNUC__)

unsigned long RdCTR_raw() {
    unsigned long lo;
    __asm__ __volatile__("rdtsc\n" : "=a" (lo) : : "edx");
    return lo;
}

/* START Solaris Sparc */ 
#elif defined(__sun) &&  defined(__SUNPRO_C)

/* SOL8_FAST_TICK was 'experimental' many years back, now all sparc has it */


#if 1 /* Quick hack to get it up and functional */
/* Sparc hardware - 
   64 bit kernels and some 32 bit, use %tick register
   Where that doesn't work, drop back to gethrtime (much slower)
   %tick access is done via an inline function, asm/sparc/RdCTR.il

*/
#include <sys/time.h>
#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>
volatile unsigned int RdCTR_asm();



static int tested = 0; /*!< Have we tested for %tick access yet ? */


static unsigned long rdctrlo() {
  unsigned long l;
  l = RdCTR_asm();
  return l;
}

static unsigned long rdctrslo() {
  unsigned long l;
  hrtime_t t;
  t = gethrtime();
  l = t ;
  return l;
}



static RDCTR rdctr = (RDCTR)rdctrlo;

unsigned long RdCTR_raw() {
  return ( (*rdctr)() );
}

static void SetupCTR() 
{
  char *env = NULL;
  if(0 == tested) {  
    env = getenv("ICC_TICK");
    if(NULL != env) {
      if(0 == strcasecmp("NOTICK",env)) {
	      rdctr = rdctrslo;
	      tested = 1;
      } else if(0 == strcasecmp("TICK",env)) {
	/* Yes, this is useful, for testing the failure case */
	      tested = 1;
      }
    }
    /* Still don't know ? */
    if( 0 == tested) { /* Autodetect */
      /* If %tick is unusable, this will return 0, and we'll
	 fall back to gethrtime()
      */   
      if ( 0 == (RdCTR_raw() ^ RdCTR_raw()) ) {
	      rdctr = rdctrslo;
      }
    }
    tested = 1;
  }
#if defined(DEBUG_TRNG)
  printf("Source = %s\n",rdctr == rdctrslo ? "gethrtime": "%TICK");
#endif
}
/* And Sparc can use multiple sources, 
   depending on whats available in the hardware
*/
/*! @brief \sa DOC_PREPENTROPY
 */
#define PREPENTROPY         { \
               if( !tested ) { \
                  SetupCTR(); \
		  CalcShift(0);	\
                  loops = bitter(); \
               } \
            }
/*! @brief \sa DOC_PREPENTROPYALT
*/
#define PREPENTROPYALT      { \
               if( !tested ) { \
                  SetupCTR(); \
		  CalcShift(0);	\
               } \
            }

#else
#include <sys/time.h>
unsigned long RdCTR_raw(void) {
  hrtime_t t;
  unsigned long l;
  t = gethrtime();
  l = t;
  return l;
}
/*! @brief \sa DOC_PREPENTROPY
 */
#define PREPENTROPY         { \
		  CalcShift(0);	\
                  loops = bitter(); \
            }

/*! @brief \sa DOC_PREPENTROPYALT
 */ 
#define PREPENTROPYALT      { \
		  CalcShift(0);	\
            }



#endif
/* END Solaris Sparc */

/* START HP/UX PA-RISC2 */

#elif defined(__hppa__) || defined(__hppa) 
/* HPUX on HP parisc2 hardware
   ansic supported the inline asm, aCC doesn't,
   so use the less performant gethrtime() instead
*/

#if 0
#include <machine/inline.h>
unsigned long RdCTR_raw(void)
{
  register ret;
  _MFCTL(16, ret);
  return ret;
}
#else 
#include <time.h>
unsigned long RdCTR_raw(void)
{
  unsigned long ret;
  ret = (unsigned long)gethrtime();
  return ret;
}
#endif

/* END HP/UX PA-RISC2 */


/* START Windows 64 bit Itanium */
#elif defined(WIN64)

#if defined(_IA64_ ) /* Itanium */
/* 
   Windows on ia64 
*/
extern unsigned long long  RdCTR_raw();
 

/* END Windows 64 bit Itanium */

/* START Windows 64 X86_64 */
#else 
#   if 1
#include <intrin.h>
unsigned long RdCTR_raw()
{
    return __rdtsc();
}
#   else
unsigned long RdCTR_raw() 
{
  LARGE_INTEGER ctr;
  unsigned long rv;
  QueryPerformanceCounter(&ctr);
  rv = (unsigned long)ctr.LowPart;
  return rv;
}
#   endif
#endif

/* END Windows 64 bit X86_64 */

/* START Windows 32 bit (ia32) */

#elif defined(_WIN32) && !defined(WIN64)
/* 
   Windows on ia32 rdtsc , well this isn't QUITE right, but we only support
   x86 and itanium currently 
*/

unsigned long RdCTR_raw() 
{
    __asm _emit 0fh
    __asm _emit 31h
}


/* END  Windows 32 bit (ia32) */

/* START iSeries */

#elif defined(OS400)
#include <mimchint.h>            /* _MATUTC mi instruction          */
#include <sys/time.h>            /* timeval struct                  */


#define MIN_SHIFT 0

unsigned long RdCTR_raw()
{
  union {
    _MI_Time mi_time; /* 64 bit (char[8] array) MI time returned from _MATUTC  */
    unsigned long long	time;
  } mi;

  /* v5r4 and previous:  The high 49 bits of this 64 bit word is a timer that increments each 8 usec.
   * v6r1 and later: The high 52 bits of this 64 bit word is a timer that increments each 1 usec.
   * The lower "undefined" bits are described as unique bits, and should be different with each call.
   * Steve, if you are short of performance, it's possible that this is simply the raw counter register 
   * read scaled, so you may be able to use those low 12 bits.
   */
	_MATUTC(mi.mi_time);

	/* Here we remove most(v5r4)/all(v6r1) of the undefined bits.
	 * If you change this right shift value, then you may want to change MIN_SHIFT above. */

	mi.time = (mi.time >> 12);

	return((unsigned long)mi.time);
}


/* END iSeries */
#elif defined(__ARMEL__) || defined(__ARMEB__) || defined(__aarch64__)
/* ARM */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>



#if 1
static inline long long
armtick(void)
{
 int fddev = -1;
 long long result = 0;
 static struct perf_event_attr attr;
 int count = 0;
 attr.type = PERF_TYPE_HARDWARE;
 attr.config = PERF_COUNT_HW_CPU_CYCLES;
  for(count = 0; count < 5; count++) {
   fddev = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
   if(fddev >= 0) {
     if (read(fddev, &result, sizeof(result)) < sizeof(result)) {
       result = 0;
     }  
     close(fddev);
     break;
   } /* Else try try again */
 }    
 return result;
}
#else 
/* Direct read of register from userspace, gets sigill */
static inline uint32_t 

armtick(void)
{
#if defined(__GNUC__) && defined(__ARM_ARCH_7A__)
        uint32_t r = 0;
        asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(r) );
        return r;
#else
#error Unsupported architecture/compiler!
#endif
}
#endif
static int arm_tested = 0;

/* 
   NOTE!, on some older arm arches, this will be very slow and ICC will 
   appear to hang.
   It's roughly 10^6 times slower than code on other platforms.
*/
unsigned long RdCTR_raw()
{

  unsigned long l;
  int i;
  volatile int j;
  if(0 == arm_tested) {
    l = armtick();
    arm_tested = 2;
    for(i = 0; i < 3; i++) {
      for(j = 0; j < 1000; j++);
      if(armtick() != l) {
	      arm_tested = 1;
	      break;
      }
    }
  }
  switch(arm_tested) {
  default:
  case 0:
    l = 0;
    break;
  case 1:
    l = (unsigned long) armtick();
    break;
  case 2:  /* This is very very slow */
    l = (unsigned long) clock();
    for(j = 0; l == clock(); j++);
    l += j;
    break;
  }
  return l;
}
#define ARM_READ

/*! 
  @brief hopefully faster way into the ARM event counters
  open the event counters, 
  read enough data to generate 64 bytes of entropy (at least 64*8 reads)
  close event counters
 */
unsigned char arm_getbyte_i(int xloops,int fddev,MINIBUF *b)
{
  int i;
  volatile int j;
  volatile int l;
  unsigned long s[8]; /* Byte sample register */
  unsigned char rv = '\0';
  unsigned long t = 0;
  unsigned char t2 = 0; /* Ends up with mixed up "throw away" LSbits */
  unsigned long long result;
  memset(s,0,sizeof(s));
  /* Grab samples - at the same rate as the tuning software */
  l = xloops;
  for(i = 7; i >= 0; i--) {
    read(fddev, &result, sizeof(result));
    s[i] = result >> shift;
    j = 0;
    looper(&j,&l, (s[i] >> shift) & 0xff, G_useDelay);
  }
  /* Stir gently */
  for(i = 0; i <8 ; i++) {
    /* 
       We explicitly only "trust" one bit/byte to have 
       good distribution characteristics BUT 
       We also save the fast moving bits we can't actually trust
       not to be patterned - these we'll compress and mix
       in at the end to gain resistance to timing attacks
    */
    t = (t << (shift+1)) ^ ( s[i] & marray[shift]);
    rv = (rv << 1) ^(s[i] & 0xff);  
  }
  /* Compress the "fast moving" bits down to one byte */
  for(i = 0; i < sizeof(t); i++) {
    t2 ^= (t & 0xff);
    t >>= 8;
  } 
  /* save any contribution from the compressed low bits 
  */
  minib_merge(b,t);

  /*! \induced 204. TRNG. Failure in the low level event counter
       stuck at a contant, non-zero, value
   */
  if(204 == icc_failure) {
    rv = 0xa5;
  }
  /*! \induced 207. TRNG. Failure in the low level event counter
       burst of failures (one off burst of values the same)
       We SHOULD recover
   */
  if(207 == icc_failure) {
    if(count < 128) {
      rv = 0xa5;
      count++;
    }
  }
  return rv;
}

void arm_read(void *ex_data, unsigned char *buffer ,int n,MINIBUF *b)
{
  int fddev = -1;
  int j = 0;
  unsigned long l = 0;
  static struct perf_event_attr attr;
  int done = 0;
  attr.type = PERF_TYPE_HARDWARE;
  attr.config = PERF_COUNT_HW_CPU_CYCLES;

  while (0 == done) {
    done = 1; /* Assume this will work */
    if (!arm_tested) {
      RdCTR_raw();
    }
    switch (arm_tested) {
    default:
    case 0: /* RNG I see no steenking RNG */
      memset(buffer, 0, n);
      break;
    case 1: /* timer is available via syscall */
      for (j = 0; j < 5; j++) {
        fddev = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
        if (fddev >= 0) {
          while (n >= 0) {
            buffer[--n] = arm_getbyte_i(loops, fddev,b);
          }
          close(fddev);
          break;
        }
      }
      /* SUID case, syscall was privileged and went away  */
      if (j >= 5) {
          arm_tested = 0;
          done = 0;
      }
      break;
    case 2: /* Fallback, use clock(). This is very very slow */
      while (n >= 0) {
        l = (unsigned long)clock();
        for (j = 0; l == clock(); j++) ; /* Loop until value changes */
        l += j;
        buffer[--n] = (unsigned char)(l & 0xff);
      }
      break;
    }
  }
}
#else
#error "\nNo viable entropy source, compliation aborted\n"
/* If all else fails ..... use this code instead */

/*! @brief 
  MIN_SHIFT is the minimum right shift we need to remove bits
  from the event counter which are stuck at or stuck mostly
  - we want something that behaves like a coin toss.
 */
#define MIN_SHIFT 0

/*! @brief \sa DOC_PREPENTROPY
 */
#define PREPENTROPY ;



/* Dummy to make sure our test code fails */
unsigned long RdCTR_raw() 
{ 
	return -1;
}	
#endif

/* 
   Setup for machines where we use timing jitter in an internal 
   CPU counter for an entropy source. 
   The problem is estimating how long we have to wait for a sufficiently 
   jittery input. 
   There's a tradeoff here between sampling time and source quality 
   so we attempt to do an estimate of the sample interval up front.

   1) First stage -  read the raw timer registers. This is platform dependent code, 
   OS specific calls or assembler.
   
   2) A mandatory setup stage which determines which is the fastest moving usable 
   bit in a sample form the raw timer source.

   3) An optional setup stage where we try to optimize the number of software loops
   to obtain sufficient jitter in the timer samples.

   4) Intermediate step RdCTR_raw() - where we know the LSBit is usable. 

   5) Final step - output, getbyte_i(loops) - which calls RdCTR() and accumulates
   bits to produce on byte of data - which theoretically is entropic enough to
   pass the later health checks, and is still at least somewhat resistant to 
   timing attacks. 

   ====================================================================================
   

   This data is used to produce three different TRNG constructions. The code for these
   steps is OUTSIDE this file.
   

   TRNG              getbyte_i(loops) used directly by the primary TRNG
   TRNG_ALT          getbyte_i(1) mixed with input from an OS or hardware PRNG
                        with timer samples to provide prediction resistance. 
		        (Avoids the sometimes substantial overhead of 3)
   TRNG_ALT2	     getbyte_i(1) fed into an SP800-90 PRNG in 
		        prediction resistance mode with an  entropy estimate of one bit/byte
			
			

   Note 1:
   It's important to note that this code won't guarantee well distributed
   random numbers - all it does is ensure that our sampling system is
   seeing SOME jitter in it's input. 
   The aim of this code is to give us a decent chance of passing the 
   later distribution tests.

   Note 2:
   This can fail, we could run the calibration run when there's a lot 
   of system noise, and run with lower noise later - 
   that's a real defect in this, but given that we have
   to cope with machine variety as well there's little we can do about this.
   We believe that this isn't a major weakness for the normal use of 
   this software - 
   the machine is likely to be quieter during initialization than it is 
   when the software is running.   
*/




#if !defined(MIN_SHIFT)
#define MIN_SHIFT 0
#endif
#if !defined(PREPENTROPY)
/*! @brief \sa DOC_PREPENTROPY
  Default entropy setup, note the check for counter over-run 
  This uses the older heuristic setup code - which worked just fine
  on most OS's, the potentially problem ones switch to bitter2()
  bitter2() is probably better, but is still relatively untested
 */
#define PREPENTROPY {\
    CalcShift(MIN_SHIFT); \
    loops = bitter();	  \
  }
#endif

#if !defined(PREPENTROPYALT) 
/*! 
  @brief \sa DOC_PREPENTROPYALT
   Default entropy setup for the alternate TRNG, 
   note the check for counter over-run */
#define PREPENTROPYALT {\
    CalcShift(MIN_SHIFT); \
  }
#endif


#if !defined(SAMPLES)
#define SAMPLES 512
#endif

unsigned long Shift()
{
  return shift;
}  
unsigned long Loops()
{
  return loops;
}  

#if !defined(HIGH_RES_TIMER)
/* Do we can test the arm paths on another platform */
#   if defined(BURST_READ) && !defined(ARM_READ)


void arm_read(void *ex_data, unsigned char *buffer ,int n. MINIBUF *b)
{

  while(n > 0) {
    buffer[--n] = getbyte_i(loops,b);
  }
}
#   endif

 

#endif
/* 
   @brief
   Check for 'stuck bits' in the low end of the value returned
   by RdCTR_raw().
   Here because it's used by a test program "testCTR.c"
   which is used to manually check this code on various platforms
   On platforms which need this correction, this is calculated
   dynamically within the PREPENTROPY macro.
   Note that this was updated to cope with beat effects in some TimeBase registers.
   @param mn minimum shift. (used if there are n known stuck low bits), i.e. s390
   @return number of low bits to throw away.
 */
#define S_TRY 5  /*!< Number of times to try and find the optimal shift */
unsigned long CalcShift(int mn)
{
  unsigned int i;
  int j = 0;
  int n = 0;
  int wrap = 0;
  unsigned long X = 0, X1 = 0;
  unsigned long *XA = NULL;
  long Xsum = 0, Xd = 0;
  int sarray[S_TRY];
  int val[16];

  if (ex_shift >= 0 && ex_shift < 16)
  {
    shift = ex_shift;
    tuned = 1;
    tuned_alt = 1;
    timer_ok = 1;
  }
  else
  {
    /* Add a sanity check to mn */
    if (mn > 15 || mn < 0)
    {
      mn = 0;
    }
    XA = (unsigned long *)ICC_Calloc(SAMPLES + 1, sizeof(unsigned long), __FILE__, __LINE__);
    for (i = 0; i < S_TRY; i++)
    {
      sarray[i] = 0;
    }

    for (j = 0; j < S_TRY; j++)
    {
      /* Look for first non-constant bit */
      for (i = 0; i < SAMPLES; i++)
      {
        XA[i] = RdCTR_raw();
      }
      if (!timer_ok)
      {
        /* Sanity check on a stuck event counter */
        for (i = 1; i < SAMPLES; i++)
        {
          if (XA[i] != XA[0])
          {
            timer_ok = 1;
            break;
          }
        }
      }
      X = 0;
      for (i = 0, X = 0; i < (SAMPLES / 2); i++)
      {
        X1 = XA[i] ^ XA[SAMPLES - i - 1];
        /*    printf("%08lx %08lx\n",XA[i],XA[SAMPLES-i-1]); */
        X |= X1;
      }

      /* if we KNOW we don't trust the low bits already we set mn to ignore at 
	      least this many bits.
	      s390 does "something funky" with that bit for example ...
      */
      for (i = mn; i < 16; i++)
      { 
        /* Extra careful here, in some cases we have an unstuck lower bit */
        if ( ((unsigned int)X & (1 << i)) &&  ( (unsigned int)X & (1 << (i+1)))  )
        {
          shift = shift_min = i;
          sarray[j] = shift;
          /* printf("i = %d,X %08lx, (1L<< i) %08lx ,(X & (1 <<i)) %08lx, mn = %d Shift = %d\n",i,X,(1L<<i), X & (1L<<1),mn,shift); */
          break;
        }
      }      
      /* Now the new code. IBM now uses a fixed rate clock for it's TimeBase 
	      register, and that can cause beat frequencies in the rate the 
	      TBR vs instruction execution rate changes.
	      That means the low n bits aren't random anymore, but aren't fixed either.
	      We deal with this by down-shifting the clock until it's 
	      "just ticking" relative to instruction rate.
      */
      /* Calculate the average clock stride per sample interval
	      Allow for the fact that the clock can wrap
      */
      for (i = n = 0, Xsum = 0; i < SAMPLES - 1; i++)
      {
        Xd = XA[i + 1] - XA[i];
        if (Xd < 0)
        { /* counter wrapped on us */
          wrap++;
        }
        else
        {
          n++;
          Xsum += Xd;
        }
      }
      /* The TimeBase counter shouldn't wrap more than once, if it did this 
	      fixup can't be done 
	      Why 4 ?. Testing. We actually rely on time "mostly advancing" 
	      in the existing heuristics 
	      - so this moves us away from the beat problem
      */
#if defined(DEBUG_TRNG)
      printf("Xsum = %ld, n = %d, wrap = %d delay = %d\n", Xsum, n, wrap,Xsum/(n+1));
#endif
      if (wrap < 2)
      {
        /* Start with the average ticks/sample, work out how many times 
	        we can shift before that's no data at all.
	      */
        for (Xd = Xsum / n; Xd > 1 && shift_max < 16; Xd >>= 1)
        {
          shift_max++;
        }
      }
#if defined(DEBUG_TRNG)
      printf("X = %08lx, shift_min = %d, shift_max = %d, shift = %d\n", X, shift_min, shift_max, shift);
#endif
    }

    for (j = 0; j < 16; j++)
    {
      val[j] = 0;
    }

    for (j = 0; j < S_TRY; j++)
    {
      val[(sarray[j] & 15)]++;
    }
    /* Find the median value */
    i = -1;
    for (j = 0; j < 16; j++)
    {
      if (val[j] > i)
      {
        i = val[j];
        shift = j;
      }
    }

    ICC_Free(XA);
    /* With the number of samples we collect, anything higher than
       an 8 bit shift is invalid 
    */
    if (shift > 16)
      shift = 16;
  }
  return X;
}

#if !defined(HIGH_RES_TIMER)
/* 
   Code below here is not needed if only using the timing functions
*/

/*!
  @brief Test the entropy available with a particular delay loop value
  @param loops the number of loops between samples
  @return the lowest entropy estimate over the sameple interval
  @note Self tuning in case the estimator characteristics change
*/
static int TestEntropy(int loops)
{
  int i = 0,x = 0;
  int est = 0, min_e = 101;
  E_EST e;
  memset(&e, 0, sizeof(e));

  /* Run the entropy estimator until the buckets are full 
    Note that we don't provide a minibuf to getbyte_i() here
    as the timing resistance scavenge is irrelevant
  */
  for(i = x = 0; !e.EntropyState; i++,x++) {
    est = pmax(getbyte_i(loops,NULL),&e);
  } 
  x *= 253;
  /* Then run it many times that number of samples and find
     the minimum entropy over that interval 
  */
  for(i = 0; i < x; i++) {
    est = pmax(getbyte_i(loops,NULL),&e);
    if(e.EntropyState) {
      if(est < min_e)	 {
	      min_e = est;
      }
    }
  }

  return min_e;
}
/* There used to be two tuners, bitter1() has gone,
   it was complex and no better than the later bitter2()
   but being paranoid people we didn't change something
   that worked for a few years
*/
static int bitter()
{
  int loops = 0;
  if(ex_loops >= 0) {
    loops = ex_loops;
  } else {
    switch(default_tuner) {
      case 1:
        loops = bitter1();
      break;
      default:
        loops = bitter2();
      break;
    }  
  }
  return loops;
}
/* 
   This appears to be a good default on most platforms, but it can be
   overridden in the platform specific code if necessary.
*/
#if !defined(NSAMP1)
#define NSAMP1 2053
#endif

/*!
  @brief sampling loop for startup entropy estimation
  get the raw data and the first differences
  @param sarray sample array
  @param xarray0 calculated first differences
  @param look count to check
*/
static void sample(volatile unsigned long *sarray, long *xarray0, int loops)
{
  int i;
  volatile int j;
  volatile int k;
  unsigned long t;
  
  memset(xarray0,'\0',sizeof(long)*NSAMP1);  
  memset((void *)sarray,'\0',sizeof(unsigned long)*NSAMP1);
  k = loops;
  for(i = 0; i <NSAMP1; i++) {
    j = 0;
    t = RdCTR_raw() >> shift;
    looper(&j, &k, (t >> shift) & 0xff, G_useDelay);
    sarray[i] =  t & 0xff;
  }
  for (i = 0; i < (NSAMP1-1); i++) {
    xarray0[i] = sarray[i+1] - sarray[i];
#if defined(DEBUG_TRNG1)
    printf(" %d %d\n",sarray[i],xarray0[i]);
#endif
  }
}
/*!
  @brief Quick heuristic test on the sampled data, 
  "does it looks somewhat random" ?
  @param sarray sample array
  @param xarray0 first differences
  @return 0 bad distribution, 1 good distribution
  @note this IS ugly. The problem is that we could have all sorts
  of non-random input at the front end - the heuristics are chosen to
  pick up things that are just counters, or with lots of "stuck" values.
  To do this properly needs a lot more data and probably things like fft's
  run on the input.
*/
static int testdata(volatile unsigned long *sarray,long *xarray0)
{
  int i;
  int nz = 0;
  int runs = 0;
  int bitbalance = 0;
  int deltas = 0;
  int notzero  = 0;
  int rv = 0; /* 0 is bad, 1 is good */

  for(i = 0; i < NSAMP1; i++) {      
    if(sarray[i] & 1) {
      bitbalance ++;
    } else {
      bitbalance--;
    }
    if(xarray0[i] != 0) {
      notzero++; /* Number of state changes in sample */
      if(nz > runs) runs = nz;
      nz = 0;
    } else {
      nz ++;
    }
    if(xarray0[i] > 0) deltas++;   /* number of "unders and overs" */
    else if(xarray0[i] < 0) deltas--; 
  }
#if defined(DEBUG_TRNG)        
  printf("NSAMP1 %d, loops = %d, shift = %d, bit balance %d, notzero %d  runs %d\n",
         NSAMP1,loops,shift,bitbalance,notzero,runs);  
#endif
  if(
     /* The number of state changes is at least 20%  */
     (notzero > (NSAMP1/5)) && 
     /* No runs of unchanged sample times longer than 10% */     
     (runs < (NSAMP1/10)) &&  
     /* The distribution of 1's and 0's is within 25% of even 
        Relaxed for D40350. This hits hardware with async
        clock sources too hard at 20%. 
      */
     (abs(bitbalance) < (NSAMP1/4)) 
     ) { 
    rv = 1;
#if defined(DEBUG_TRNG)
    printf("Good sample\n");
#endif

  } else {
#if defined(DEBUG_TRNG)
    printf("Bad sample\n");
#endif 
    rv = 0;
  }
  return rv;
}

/*!
  @brief bit tester V 1. 
  Quickly try to tune the TRNG to the point where there's
  noise in the clock source
  @return the number of delay loops between samples (for historical reasons).
  @note What we SHOULD be doing here is using an FFT or FHT to find the CPU 
  clock harmonics and beat frequencies caused by an independently 
  clocked timebase, and create a digital filter to remove those 
  components leaving only the noise.
  Frankly - "too hard".
  - This is still in use because it works on a vast variety of hardware variants.
  The simpler and easier to understand bitter2() doesn't.
*/  
  
static int bitter1()
{ 
  int m;
  /* Volatile so the compilers won't optimize some operations away */
  volatile unsigned long *sarray = NULL; 
  long *xarray0 = NULL; /* delta's i.e. needs to be signed */  

  sarray = (volatile unsigned long *) ICC_Calloc(sizeof(unsigned long),NSAMP1,__FILE__,__LINE__);
  xarray0 = (long *) ICC_Calloc(sizeof(long),NSAMP1,__FILE__,__LINE__);

  for( m = 1; m < (sizeof(ptable)/sizeof(int)) ; m++) { 
    loops = ptable[m];
    sample(sarray,xarray0,m);
    /* Can we get good enough data with just the sample interval ? */
    if( 1 == testdata(sarray,xarray0) ) {
      break;
    } 
  }
  if(shift > 15) shift = 15; /* Limit it's good for */
  /* Final phase, we do this as it's a near free 50% performance boost */
  if(m > 0 && m < (sizeof(ptable)/sizeof(int)) && (loops = (ptable[m]+ptable[m-1])/2 ) > 1 ) {
#if defined(DEBUG_TRNG)
    printf("Optimize %d ,loops = %d \n",m,loops);
#endif
    sample(sarray,xarray0,m);
    if(0 == testdata(sarray,xarray0)) { /* if that didn't help, go the conservative choice */
      loops = ptable[m];
    }
  }
  
  ICC_Free(xarray0);
  ICC_Free((unsigned long *)sarray); /* The volatile trips warnings on some compilers */
  return loops;
} 


/*! 
  @brief Entropy estimating tuner to cope with the async timebase
  on some hardware (IBM Power hardware, zSeries).
  Alas, it doesn't work on all hardware variants, the older code (bitter1()) works
  far better on many.
  @return the number of wait loops required to get >= 0.5 bits/bit 
  entropy
*/
static int bitter2() {
  int m = 0;
  int loops = 0,tloops = 0;
  int est = 0;
  int tabsize = sizeof(ptable)/sizeof(int);
  for(m = 0; m < tabsize; m++) {
    loops = ptable[m];
    est = TestEntropy(loops);   	
    if( est >= 50) {
      break;
    }
  }
  /* Final phase, see if there's it's viable with a value half way between the current and previous values
     we do this as it's often a near free 50% performance boost and that can make a huge difference
     at higher loop counts
  */
  if(m > 3 && m < tabsize && (tloops = (ptable[m]+ptable[m-1])/2 ) > 1 ) {
    est = TestEntropy(tloops);
    if(est >= 50) {
      loops = tloops;
    }
  }  
  /*  printf("Config: loops = %d\n",j); */
  return loops;
}




/*=====================================================================================
                                 Exported functions
  ====================================================================================*/

/*!
 @brief get a byte of "random" data.
 This is constructed from 8 samples from the event counter registers
 with the lowest moving bit shuffled one place each time.
 We only trust the "lowest bit known to be changing randomly" to be 
 anything like well distributed and random but ...
 While we don't trust the low bits we discard not to be patterned or 
 stuck, we compress the otherwise discarded bits and save them to mix
 back in at the HMAC compressor to improve resistance to timing attacks
 @param xloops the number of loops to execute before re-reading the event counter
 @param MINIBUF somewhere to stash potentially faster moving bits as a protection
        against timing attacks
 @return one byte of data
*/
unsigned char getbyte_i(int xloops, MINIBUF *b )
{
  unsigned char rv = '\0';
  int i;
  volatile int j,k;

  unsigned long s[8]; /* sample register */
  unsigned long t = 0;
  unsigned char t2 = 0; /* Ends up with mixed up "throw away" LSbits */
  memset(s,0,sizeof(s));
  /* Grab samples - at the same rate as the tuning software */
  j = xloops;
  for(i = 7; i >= 0; i--) {
    s[i] = RdCTR_raw();
    k = 0;
    looper(&k,&j, (s[i] >> shift) & 0xff, G_useDelay);
  }

  /* Stir gently */
  for(i = 0; i <8 ; i++) {
    /* Move the bit we want for entropy to the LSB and merge it in */
    rv = (rv << 1) ^ ((s[i] >> shift) & 0xff);
    /* 
       We explicitly only "trust" one bit/byte to have 
       good distribution characteristics (above) BUT 
       We also save the fast moving bits we can't actually trust
       to change - these we'll compress and mix
       in at HMAC compressor to potentially gain more resistance 
       to timing attacks if they do flicker now and then
       This may be no contribution at all.
    */
    t2 = s[i] & marray[shift];
    t = (t << shift) ^ t2;

  }
  /* Compress the "fast moving" bits we collected down to one byte 
    Note we don't just store the bytes and xor because there are a variable
    number of bits we could use depending on platform and the objective here
    is to make timing attacks harder so we want as many of these as we can get.
  */
  for(i = 0; i < sizeof(t); i++) {
    t2 ^= (t & 0xff);
    t >>= 8;
  } 
  /* Store any contribution from the compressed low bits 
    for later mixing
  */
  minib_merge(b,t2);

  /*! \induced 204. TRNG. Failure in the low level event counter
       stuck at a contant, non-zero, value
   */
  if(204 == icc_failure) {
    rv = 0xa5;
  }
  /*! \induced 207. TRNG. Failure in the low level event counter
       burst of failures (one off burst of values the same)
       We SHOULD recover gracefully from this
   */
  if(207 == icc_failure) {
    if(count < 128) {
      rv = 0xa5;
      count++;
    }
  }

  return rv;
}


/*! @brief Access to CPU event counters for 'salt' 
  @return event counter reading
*/
unsigned long RdCTR()
{
  return RdCTR_raw() >> shift;
}
/*!
 @brief get a byte of "random" data.
 loops is set by one off setup done by the PREPENTROPY macro,
 called by prepentropy()
 @param b a MINIBUF used IF we burst read the source. 
        i.e. on ARM a source only available via an OS call with horrible latency
 @param tr a MINIBUF which accumulates data to assist with resistance to timing attacks 
 @return one byte of data
 @note The buffered read case doesn't occur on the FIPS platforms.
 @note getbyte() only exists within timer_entropy.c as the tuning parameter 'loops' has only local scope.
*/
unsigned char getbyte(MINIBUF *b,MINIBUF *tr)
{
  unsigned char rv = 0;
#if defined(BURST_READ)
  rv = minib_get(b); /* Note in this case, loops is irrelevant */
#else
  rv = getbyte_i(loops,tr);
#endif
  return rv;
}


void Timer_NOISE_preinit(int reinit)
{
  if(!tuned || reinit) {
    tuned = 1;
    tuned_alt = 1;
    PREPENTROPY;
    /* printf("loops = %d, shift = %d\n",loops,shift); */
  }
} 


/*!
  @brief one off entry point for partial noise tuning
  - remove stuck bits
*/


void Timer_NOISE_ALT_preinit(int reinit)
{
  /* Only do the minimum tuning code needed to find stuck bits */
  if(full_rng_setup || reinit) {
    Timer_NOISE_preinit(reinit);
  } else if(!tuned_alt || reinit) {
    tuned_alt = 1;
    PREPENTROPYALT;
    /* printf("loops = %d, shift = %d\n",loops,shift); */
  }
}

/*! @brief Return the noise source optimization algorithm
    @return 1 = heuristic tuner
            2 = entropy estimating
*/
int Get_default_tuner() 
{
  return default_tuner;
}

/*! @brief Select the default noise source optimization algorithm
    @param tuner 1 = heuristic, otherwise entropy estimating
    @return 1 (heuristic) or 2 (estimating)
*/

int Set_default_tuner(int tuner) 
{
  switch(tuner) {
  case 1:
    default_tuner = 1;
    break;
  default:
    default_tuner = 2;
    break;
  }
  return default_tuner;
}

/*! @brief return the event counter status
  @return 1 means 'plausibly functional'
          0 means not changing, totally unusable
*/
int timer_status()
{
  return timer_ok;
}

#endif /* HIGH_RES_TIMER */
