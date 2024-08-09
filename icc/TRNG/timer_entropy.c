/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "TRNG/timer_entropy.h"
#include "TRNG/looper.h"
/*! @brief delay loop
    It's in this module exactly because it's not used by any code in here
    so the optimizer can't see it and make it a NOOP and hopefully the optimizer in
    other modules can't see it either
    @param i pointer to the loop iterator
    @param j pointer to the loop count
    @return total, just so that this apepars to do something
*/


int ex_loops = -1;
int ex_shift = -1;


unsigned int shift = 0;
int shift_done = 0;   /* Flag to indicate we've run CalcShift() */
static int full_rng_setup = 0;

/* Low level timers used across OS variants */

/* 
   START ia32 Linux, OS/X
 */
#if (defined(__linux__) &&  defined(__i386__) ) || (defined(__APPLE__) && defined(__i386__) )
ICC_UINT64 RdCTR_raw() {
    ICC_UINT32 lo;
    __asm__ __volatile__("rdtsc\n" : "=a" (lo) : : "edx");
    return (ICC_UINT64)lo;
}

  

/* END ia32 Linux, OS/X */

/* START Linux ia64 */
#elif defined(__linux__) && defined(__ia64__)

ICC_UINT64 RdCTR_raw() {
    ICC_UINT64 lo;
    __asm__ __volatile__("mov %0=ar.itc" : "=r"(lo));
    return lo;
}

/* START HP/UX Itanium with gcc */
#elif defined(__hpux) && defined(__GNUC__)  &&  defined(__ia64) 
ICC_UINT64 RdCTR_raw() {
    ICC_UINT64 lo;
    __asm__ __volatile__("mov %0=ar.itc" : "=r"(lo));
    return lo;
}

/* END HP/UX Itanium with gcc */

/* START HP/UX Itanium with aCC */
#elif defined(__hpux) &&  defined(__ia64)
#include <machine/sys/inline.h>

#define __TICKS _Asm_mov_from_ar(_AREG_ITC)
ICC_UINT64 RdCTR_raw() {     
    return __TICKS;
}

/* END HP/UX Itanium with aCC */

/* Start X86_64 */
/* START  X86_64 Linux, OS/X */
#elif defined(__linux__) && defined(__x86_64__)   || (defined(__APPLE__) && defined(__x86_64__)) || (defined(__sun) && defined(__x86_64__) && defined(__GNUC__))

ICC_UINT64 RdCTR_raw() {
    ICC_UINT64 lo;
    __asm__ __volatile__("rdtsc\n" : "=a" (lo) : : "edx");
    return lo;
}

/* Start OSX_ARM64 */

#elif defined(__APPLE__) && defined(__aarch64__)
/* OSX_ARM64
Ref: https://stackoverflow.com/questions/74757124/rdtsc-rdtscp-for-arm-mac-m1-m2
*/
#include <time.h>

ICC_UINT64 RdCTR_raw() {
    ICC_UINT64 x;
    struct timespec A;
    clock_gettime(CLOCK_MONOTONIC_RAW,&A);
    x = A.tv_nsec;
    return x;
}

/* End OSX_ARM64 */

/* START X86_64 Solaris SunPro compiler */
#elif defined(__SUNPRO_C) && defined(__amd64) 

/* UNTESTED, AMD-64 Solaris 10 , should use rdtsc later*/

ICC_UINT64 RdCTR_raw() {
  return gethrtime();
}

/* END X86_64 Solaris SunPro compiler */    

/* END X86_64 */

/* START x86, Solaris SunPro compiler */
#elif defined(__SUNPRO_C) && defined(__i386)

/* UNTESTED, Solaris x86 , should use rdtsc later*/

ICC_UINT64 RdCTR_raw() {
    return gethrtime();
}
/* END Solaris SunPro compiler */
/* Solaris x86 with gcc (faster) */
#elif defined(__sun__) && defined(__i386__) && defined(__GNUC__)

ICC_UINT64 RdCTR_raw() {
    ICC_UINT32 lo;
    __asm__ __volatile__("rdtsc\n" : "=a" (lo) : : "edx");
    return (ICC_UINT64)lo;
}

/* End  Solaris x86 */
/* Solaris Sparc, external asm */
#elif defined(__sun) &&  defined(__SUNPRO_C)
extern volatile unsigned int RdCTR_asm();
ICC_UINT64 RdCTR_raw() {
  ICC_UINT64 l = 0;
  l = RdCTR_asm();
  return l;
}

/* End Solaris Sparc */
/* PowerPC with GCC, Linux or OS/X - Note PPC on OS/X is dead now. */    
#elif (defined(__linux__) &&  defined(__PPC__))
/* 32 & 64 bit PPC Linux  on PPC*/
ICC_UINT64 RdCTR_raw() {
    ICC_UINT32 lo;

    __asm__ __volatile__("mftb %0\n" : "=r" (lo) : );
    return (ICC_UINT64)lo;
}
/* End PowerPC Linux */
/* START zSeries , Linux & Z/OS */
/* Start z/OS */
// Note for Z:
// The machine actually has only ever incremented at bit 62 of the TOD clock.
// Bits 0:55 increment based on a precise oscillator, and bits 56:63 are a saturating counter based on cycle counts.
// There can be some bias in the cycle counting ...  As far as I am aware, this behavior has been the same at least z13-z16.
// Looking at [bits] 48:55 seems like probably your best option for the best entropy. - Jonathan Bradbury
#elif defined(__MVS__)
#include <stdint.h>
#include <builtins.h> 
/* STCK1 provides more accuracy (but that may not be needed) .
   We want resolution and "unscrewed with" here more than 
   accuracy, so raw counts are preferred
*/
ICC_UINT64 RdCTR_raw(void)
{
  uint32_t fred[4];

  /* compile egather.c with -W "c,langlvl(extended)" */
  /* "loses high order bits; is that really correct?"
     "Yes, we only use at most the 8 lowest moving bits"
  */
  __stcke(&fred); 
  return (ICC_UINT64)fred[1];
}
/* END z/OS */
#elif (defined(__linux__) && defined(__s390__))   
#include <stdint.h>
/* Cycle counter on s390/zSeries  */

/* Low XXX bits are constant on the machines we've tested this on,
   I assume they leave room for faster processors with more resolution in future
   Which is why the "odd" looking code.
   stcke returns 16 bytes, stck 8, stckf 8
*/

ICC_UINT64 RdCTR_raw(void)
{
  uint32_t fred[4];

  __asm__("stcke 0(%0)" : : "a" (&(fred)) : "memory", "cc");
  return (ICC_UINT64)fred[1];
}

/* HP/UX on pa-risc */
#elif defined(__hpux)
#if 0
/* disabled as shouldn't be calling OPENSSL_ here */
extern ICC_UINT64 OPENSSL_rdtsc();
ICC_UINT64 RdCTR_raw() { 
    return OPENSSL_rdtsc();
}
#else
/* START HP/UX PA-RISC2 */

/*#elif defined(__hppa__) || defined(__hppa) */
/* HPUX on HP parisc2 hardware
   ansic supported the inline asm, aCC doesn't,
   so use the less performant gethrtime() instead
*/

#if 0
#include <machine/inline.h>
ICC_UINT64 RdCTR_raw(void)
{
   register ret;
   _MFCTL(16, ret);
   return ret;
}
#else 
#include <time.h>
ICC_UINT64 RdCTR_raw(void)
{
   ICC_UINT64 ret;
   ret = (ICC_UINT64)gethrtime();
   return ret;
}
#endif

/* END HP/UX PA-RISC2 */

#endif
/* End HP/UX on parisc */
/* Start AIX 32 & 64 bit, external asm used */
#elif defined(_AIX)
extern ICC_UINT32 RdTBR();
ICC_UINT64 RdCTR_raw()
{
    ICC_UINT32 l;
    l = RdTBR();
    return (ICC_UINT64)l;
}
/* End AIX 32 & 64 bit */

/* START Windows 32 bit (ia32) */

#elif defined(_WIN32) && !defined(WIN64)
/* 
   Windows on ia32 rdtsc , well this isn't QUITE right, but we only support
   x86 and itanium currently 
*/

ICC_UINT64 RdCTR_raw() 
{
    __asm _emit 0fh
    __asm _emit 31h
}
/* END Windows on ia32 */
/* Start Windows 64 on x86 */
#elif defined(WIN64) && !defined(_IA64_)
#include <windows.h>
#include <intrin.h>
ICC_UINT64 RdCTR_raw() 
{
#if 1
    unsigned __int64 rv;
    rv = __rdtsc();
#else
    LARGE_INTEGER ctr;
    ICC_UINT64 rv;
    QueryPerformanceCounter(&ctr);
    rv = (ICC_UINT64)ctr.LowPart;
#endif
    return rv;
}
/* END win64 on x86 */
/* Start Windows 64 on ia64 */
/* External */
/* END Windows 64 bit on ia64 */

#elif defined(__ARMEL__) || defined(__ARMEB__) || defined(__aarch64__)
#if 1
/* *_ARM - discovered for OSX but should work for all ARM linux
Ref: https://stackoverflow.com/questions/74757124/rdtsc-rdtscp-for-arm-mac-m1-m2
*/
#include <time.h>

ICC_UINT64 RdCTR_raw() {
   ICC_UINT64 x;
   struct timespec A;
   clock_gettime(CLOCK_MONOTONIC_RAW, &A);
   x = A.tv_nsec;
   return x;
}

#else
/* ARM - old way
    Which is painful as generally there's no direct access to the timer registers
*/
/* Only ARM Linux bypasses RdCTR_raw() at present */
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
static int armrd(unsigned long *buffer, int len)
{

    int fddev = -1;
    int result = len;
    int n = 0;
    static struct perf_event_attr attr;
    int count = 0;
    int i;
    volatile int j;
    unsigned long long ibuffer[256]; /* read() pulls in 64 bit quantitites, we process 32 bit */

    attr.type = PERF_TYPE_HARDWARE;
    attr.config = PERF_COUNT_HW_CPU_CYCLES;
    n = len;
    for (count = 0; len > 0 && count < 5; count++)
    { /* Yes there's a limit, note the "static" above, this is NOT a general purpose API */
        fddev = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
        if(fddev <= 0) continue;
        for(i = 0;i < n ; i++) {
            if(0 >= read(fddev,ibuffer,sizeof(unsigned long long))) {
                close(fddev);
                fddev = -1;
                break;
            }
            buffer[i] = (unsigned long)ibuffer[0];
            for(j = 0; j < ex_loops;j++);
            len --;
        }
        close(fddev); /* Note we close and reopen because it sometimes wedges/instance */
        fddev = -1;
    }

    if (len > 0)
    {
        result = 0;
    }

    return result;
}
unsigned long RdCTR_raw()
{
    unsigned long buffer[2];
    armrd(buffer,1);
    return buffer[0];
}
#endif
#endif

int RdCtrBurst(ICC_UINT64 *buffer,unsigned int len, int localloops)
{
   int rv = 0;
   int i = 0;
   volatile int k,l; /* Volatile so the compilers don't optimise the delay loop away */

    l = localloops;
    for (i = 0; i < len; i++)
    {
        buffer[i] = RdCTR_raw() ; 
        k = 0;
        looper(&k,&l);
    }
    rv = len;

    for (i = 0; i < len; i++) {
        buffer[i] = buffer[i] >> shift; /* Remove stuck LSBits etc */
    }
    return rv;
}



/* 
   @brief
   Check for 'stuck bits' in the low end of the value returned
   by RdCTR_raw().
   Here because it's used by a test program "testCTR.c"
   which is used to manually check this code on various platformsSHIFT
   @param mn minimum shift. (used if there are n known stuck low bits), i.e. s390
   @return number of low bits to throw away.
 */
#define S_TRY 5  /*!< Number of times to try and find the optimal shift */
#if !defined(SAMPLES)
#define SAMPLES 512
#endif
int shift_max = 0;
int timer_ok = 0;

#if !defined(MIN_SHIFT)
#define MIN_SHIFT 0
#endif

int Shift() {
    return shift;
}
/*! @brief tweak for CalcShift, on some platforms the LSBit we detect as moving 
    isn't moving much and we need another shift. Sanity check here checks that
    the bit distribution across a reasonable number of samples is plausible.
    @param lps loop count
    @param shft bits to shift
    @note We don't get more paranoid or try harder because if it really bad
    in the other bits we can't really recover anyway. Well we could but it'd
    need something like a circular shift each time to fix the bit distribution
*/
static int checkShift(int lps,int shft)
{
	int i,i1;
	volatile int j;
	ICC_UINT64 X;
	unsigned char smpl[512];
	ICC_UINT64 dist[8];
	int prob = 0;

	memset(smpl,0,512);
#if defined(DEBUG_DUMP)
	printf("Loops = %d shift = %d\n",lps,shft);
#endif	
    /* Note we read to a buffer because doing the processing inline
       would change the sampling rate
    */
	for (i = 0; i < 65536; i+=512)
	{
		memset(smpl,0,512);
		for(i1 = 0; i1 < 512; i1++) {
			X = (RdCTR_raw() >> shft);
			j = 0;
			looper(&j,&lps);
			smpl[i1] = X & 0xff;
		}
		memset(dist,0,sizeof(dist));
		/* You can do it with a loop as well, this was just as easy */
		for (i1 = 0; i1 < 512; i1++)
		{
			if (smpl[i1] & 1)
				dist[0]++;
			if (smpl[i1] & 2)
				dist[1]++;
			if (smpl[i1] & 4)
				dist[2]++;
			if (smpl[i1] & 8)
				dist[3]++;
			if (smpl[i1] & 16)
				dist[4]++;
			if (smpl[i1] & 32)
				dist[5]++;
			if (smpl[i1] & 64)
				dist[6]++;
			if (smpl[i1] & 128)
				dist[7]++;
		}
	}

#if defined(DEBUG_DUMP)	
	printf("\n");
	for (i = 0; i < 8; i++)
	{
		printf("%-3d %-5lu\t", i, dist[i]);

	}
#endif	
	for (i = 0; i < 8; i++)
	{
		if (dist[i] < 128 )  /* Mostly 0 */
		{ /* Badly biased Bit */
			prob ++;
		}
		else if( dist[i] > (256+128) ) /* Mostly 1 case */
		{  
			/* Badly biased Bit */
			prob ++;
		}
	}
	return prob; /* Probably needs another shift */
}

ICC_UINT64 CalcShift(int mn)
{
    unsigned int i;
    int j = 0;
    int n = 0;
    int wrap = 0;
    ICC_UINT64 X = 0, X1 = 0;
    ICC_UINT64 *XA = NULL;
    ICC_INT64 Xsum = 0, Xd = 0;
    int sarray[S_TRY];
    int val[16];

    if (ex_shift >= 0)
    {
        shift = ex_shift;
    }
    else
    {
        /* Add a sanity check to mn */
        if (mn > 15 || mn < 0)
        {
            mn = 0;
        }
        XA = (ICC_UINT64 *)calloc(SAMPLES + 1, sizeof(ICC_UINT64));
        if (NULL != XA)
        {
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
                    if (((unsigned int)X & (1 << i)) && ((unsigned int)X & (1 << (i + 1))))
                    {
                        shift = i;
                        sarray[j] = shift;
                        /* printf("i = %d,X %08lx, (1L<< i) %08lx ,(X & (1 <<i)) %08lx, mn = %d Shift = %d\n",i,X,(1L<<i), X & (1L<<1),mn,shift); */
                        break;
                    }
                }
                /* Now the new code. If a fixed rate clock is used for the TimeBase 
	      register, it can cause beat frequencies in the rate the 
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
	            in the code
            */
#if defined(DEBUG_TRNG)
                printf("Xsum = %ld, n = %d, wrap = %d delay = %d\n", Xsum, n, wrap, Xsum / (n + 1));
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
                printf("X = %08lx, shift = %d\n", X, shift);
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

            free(XA);
        }
        /* Final sanity check to try and deal with an erratic LSbit */

        if(checkShift(1,shift)) {
            shift++;
        }
        
        if (shift > 16)
        {
            shift = 16;
        }
    }
    return X;
}

ICC_UINT64 RdCTR()
{
    return (RdCTR_raw() >> shift);
}
int Get_default_tuner()
{
    return 1;
}
int Set_default_tuner(int tuner)
{
    return 1;
}
int Set_rng_setup(int i)
{
  full_rng_setup = i;
  return i;
}

