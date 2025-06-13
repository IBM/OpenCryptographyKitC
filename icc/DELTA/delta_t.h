/*
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*/

#ifndef INCLUDED_DELTA_T
#define INCLUDED_DELTA_T

#ifdef __cplusplus
extern "C" {
#endif
#if defined(__MVS__)
#pragma export(Delta_T)
#pragma export(Delta_res)
#pragma export(Delta2Time)
#pragma export(Delta_spanT)
#pragma export(Delta_spanC)
#endif

/*! @brief access high speed event counters
  @param mode 1 = initialize, 0 = read
  @param d a pointer to an unsigned long to hold raw count
  @note This code will account for at most one overflow.
  - This is important !. This code is intended ONLY to measure SHORT
  time spans at high resolution.
  - You can try to compenstate for this
  by using gettimeofday() or clock() as well to work out how many times
  you must have cycled. That's not done internally as the latency needed
  to do that may cause it's own problems in many use cases.
  \sa Delta_spanT() for the routine to use find the usable timing window
*/
unsigned long Delta_T(int mode, unsigned long *d);

/*! @brief get the estimate for the base event counter resolution
  @return the estimated counter resolution (in counts)
  @note quite often the lower bits are stuck-at and are unusable
*/
unsigned long Delta_res();

/*! @brief get the estimated conversion factor for counts to nS
  @param recalc The recalculation of the conversion factor
  can take quite a while depending on the span of the counter (seconds), 
  so by default we do it once and simply return the previously 
  generated result on subsequent calls. !0 forces this this to be redone.
  @return The conversion factor for counts to nS
  @note This isn't particularly accurate and it shouldn't be used
  in intermediate calculations. i.e. use count values in any statistics and
  use the conversion here to normalize results to real time at the end of the
  process.
  - Also note, this is extremely expensive in CPU time, 
  it may appear to hang for quite a while depending on arch
  - on the other hand, you don't HAVE to call this routine
  - Accuracy SHOULD be better than 1%
  - Called by Delta_spanT() if it hasn't been run before
*/
double Delta2Time(int recalc);

/*! @brief return the APPOXIMATE time span of the counter in nS
  @return The approximate time span of the counter
  @note This will run Delta2Time() if it hasn't already been run
*/
double Delta_spanT();

/*! @brief return the counter span in counts,
  i.e. The limit before the counter overflows
  @return  The counter span in counts
*/
unsigned long Delta_spanC();

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_DELTA_T */
