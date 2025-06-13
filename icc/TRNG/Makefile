#
# * Copyright IBM Corp. 2023
# *
# * Licensed under the Apache License 2.0 (the "License").  You may not use
# * this file except in compliance with the License.  You can obtain a copy
# * in the file LICENSE in the source distribution.
# 

#******************************************************************************
#
# Description: This is the Makefile for the new TRNG code.
#
# Usage:  make OPSYS=<[WIN32*|LINUX|AIX|SUN|HPUX]> [CONFIG=<debug|release*>]
#              * - denotes default values if not explicitly specified
#
# Note:  OpenSSL and ICC require Perl to build
#
# Note:  Windows requires a UNIX-like shell to be installed, and the compiler
#        environment to be setup.  This can either be done at compiler
#        installation time, or by running vcvars32.bat before attempting to
#        build.  (vcvars32.bat need only be run once per environment/DOS box)
#
#******************************************************************************

ICC_ROOT=../..

#-- Directories
ICC_INC_DIR    = ../
ICC_PRNG_DIR   =
OPENSSL_INC_DIR = $(ICC_ROOT)/openssl-1.0.1/include/ 

HDRS	= -I ./ -I $(ICC_INC_DIR) -I $(OPENSSL_INC_DIR) -I../fips-prng

#- Default targets, before the makefile stubs below (some of which
# have their own targets) so it *IS* the default
#- Do everything EXCEPT create sources (default target)

default: all

# Define what OpenSSL version we are using.
include opensslver.mk
include platforms.mk
include icc_defs.mk

include zlib.mk

# Documentation build.
#

# What we build.

TARGETS  = \
	nist_algs$(OBJSUFX) \
	timer_entropy$(OBJSUFX) \
	personalise$(OBJSUFX) \
	noise_to_entropy$(OBJSUFX) \
	entropy_estimator$(OBJSUFX) \
	MINIMAL$(OBJSUFX) \
	TRNG$(OBJSUFX) \
	TRNG_ALT$(OBJSUFX) \
	entropy_to_NRBG$(OBJSUFX) \
	SP800-90TRNG$(OBJSUFX)

# define OpenSSL related variables
include ../ossl.mk

# Include platform specific make functions. These can use "native"
# make features if necessary.

include ../platforms/$(OPSYS)_.mk

#-------------------------------------------------------------------------------
#- Targets
#-------------------------------------------------------------------------------

all:  build_all 

#- Just build
build_all:  $(TARGETS)

#- Clean
clean:
	-rm *$(OBJSUFX) *~

#- Build ICC TRNG code
#- Note some platforms, SUN_SOL8/SUN64 AIX/AIX64 need added assembler tweaks
#
entropy_to_NRBG$(OBJSUFX): entropy_to_NRBG.h noise_to_entropy.h \
	MINIMAL.h TRNG.h TRNG_ALT.h
	$(CC) $(CFLAGS)  $(HDRS) $(ASM_TWEAKS) entropy_to_NRBG.c

nist_algs$(OBJSUFX): nist_algs.c nist_algs.h
	$(CC) $(CFLAGS) $(HDRS) nist_algs.c

noise_to_entropy$(OBJSUFX): noise_to_entropy.c noise_to_entropy.h 
	$(CC) $(CFLAGS) $(HDRS) noise_to_entropy.c

timer_entropy$(OBJSUFX): timer_entropy.c
	$(CC) $(CFLAGS) $(HDRS) timer_entropy.c

personalise$(OBJSUFX): personalise.c
	$(CC) $(CFLAGS) $(HDRS) personalise.c

entropy_estimator$(OBJSUFX): entropy_estimator.c entropy_estimator.h
	$(CC) $(CFLAGS) $(HDRS) entropy_estimator.c

MINIMAL$(OBJSUFX): MINIMAL.c MINIMAL.h timer_entropy.h
	$(CC) $(CFLAGS)  $(HDRS) MINIMAL.c

TRNG$(OBJSUFX): TRNG.c TRNG.h timer_entropy.h
	$(CC) $(CFLAGS)  $(HDRS) TRNG.c

TRNG_ALT$(OBJSUFX): TRNG.c TRNG.h TRNG_ALT.h timer_entropy.h
	$(CC) $(CFLAGS)  $(HDRS) TRNG_ALT.c

ICC_NRBG$(OBJSUFX): ICC_NRBG.c MINIMAL.h TRNG.h TRNG_ALT.h
	$(CC) $(CFLAGS) $(HDRS)  ICC_NRBG.c

SP800-90TRNG$(OBJSUFX):  ../fips-prng/SP800-90.h ../fips-prng/SP800-90i.h
	$(CC) $(CFLAGS) $(HDRS)  SP800-90TRNG.c




