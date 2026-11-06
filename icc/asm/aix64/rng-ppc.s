#;-*- asm -*------------------------------------------------------------
#; Copyright IBM Corp. 2023
#;
#; Licensed under the Apache License 2.0 (the "License"). You may not use
#; this file except in compliance with the License. You can obtain a copy
#; in the file LICENSE in the source distribution.
#;----------------------------------------------------------------:-)---

		.file "rng-ppc.s"
.toc
.set SP,1;   .set RTOC,2;
.set r0,0;   .set r1,1;   .set r2,2;   .set r3,3;   .set r4,4;   .set r5,5;   .set r6,6;   .set r7,7
.set r8,8;   .set r9,9;   .set r10,10; .set r11,11; .set r12,12; .set r13,13; .set r14,14; .set r15,15
.set r16,16; .set r17,17; .set r18,18; .set r19,19; .set r20,20; .set r21,21; .set r22,22; .set r23,23
.set r24,24; .set r25,25; .set r26,26; .set r27,27; .set r28,28; .set r29,29; .set r30,30; .set r31,31
		#; Read time base register of PowerPC CPU
		#; function decl: int RdTBR();
		#.func RdTBR
	.globl RdTBR
	.globl .RdTBR
	.csect RdTBR[DS],2
RdTBR:	.llong .RdTBR, TOC[tc0], 0
	.csect .text[PR]
	.align 2
.RdTBR:
		#;
		mftb	r3
		blr
		#.funcEnd ?RdTBR
		#.fileEnd "rng-ppc"
