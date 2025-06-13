/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

#if !defined(STATS_H)
#define STATS_H

/* Clear/init the stats buffer */
void StatsClear();
/* Accumulate stats of deltas */
void StatD(unsigned long d);
/* Accumulate stats on frequency of LSBytes of samples */
void StatV(unsigned long v);


/* Dump stats to a file prefix##D.dat prefix##V.dat */

void dump_stats(const char *prefix);
#endif
