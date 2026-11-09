#
# This Makefile SHOULD allow you to build GSkit-Crypto
# which encapsulates the last FIPS ICC and the current build
# on any platform
# make OPSYS=<something>
#

default: all

# Makefile needs to define this as relative path from Makefile's dir to the base of ICC source dir
ICC_ROOT=..

CONFIG = debug
BUILD = developer

include $(ICC_ROOT)/icc/VTAG.mk
include $(ICC_ROOT)/icc/platforms.mk


# Version # of the old ICC
OLDVER = $(FVTAG)

# Version # of the new ICC

include $(ICC_ROOT)/icc/icc_defs.mk
include $(ICC_ROOT)/icc/opensslver.mk

# GSKit-Crypto defs
# defines JGSK_DIR and GSK_DIR ($(ICC_ROOT)/package/gskit_crypto)
include gsk_crypto.mk

## Build specific definitions - file target etc.
include iccpkg.mk


# Hack to allow us to build a gskit_crypto without the OLD icc present
include muppet.mk

developer_CFLAGS = -DVTAG=$(VTAG) -DINSTDIR=\""$(GSK_GLOBAL)"\"
OFFICIAL_CFLAGS = -DICC_OFFICIAL_BUILD $(developer_CFLAGS)

ICCLIB		= $(STLPRFX)icc$(STLSUFX)

NEW_ICC		= ../package

ZLIB_LIB 	= ../icc/$(STLPRFX)zlib$(STLSUFX)

NEW_LIBICC	= $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)

ifeq ($(strip $(IS_FIPS)),)
   MYICC = newicc
else
   MYICC = icc
endif		


# Sign tool for Microsoft Authenticode
SIGN_COMMAND    = /build/build/scripts/signwindowsfile.ksh

#BVT
GSK_BVT		= $(GSK_SDK)/icctest$(EXESUFX)

# OpenSSL command line
GSK_OPENSSL     = $(GSK_SDK)/openssl$(EXESUFX)




SDK_TARGETS = 	$(GSK_RNG)

#		$(ICC_RNG) \

JSDK_TARGETS = 	\
		$(JGSK_SDK)/inc/icc.h \
		$(JGSK_SDK)/inc/iccglobals.h \
		$(JGSK_SDK)/inc/icc_a.h \
		$(JGSK_SDK)/inc/jcc_a.h \
		$(JGSK_SDK)/bin/jcctest$(EXESUFX)

# Note the lib/ dir in the JSDK only exists on windows and 
# is built & populated when we build the shared lib

JGSK_TARGETS = \
		$(JGSK_LIB)

#
# Source .[c|h] files created as part of the build
#
TMP_SRC = iccpkg.h icc.h icc_a.h iccglobals.h \
	iccversion.h \
	jcctest.c jgsk_wrap2.c

#
# objects copied from elsewhere, don't exist on all platforms !
#
TMP_OBJS = delta$(OBJSUFX) iccstub$(OBJSUFX) high_res_timer$(OBJSUFX) \
	rng-ppc.o RdCTR_raw.o api$(STLSUFX) 

#
# Extra function only in the current step library
#
EX_OBJS =
#EX_OBJS = exp$(OBJSUFX) totp$(OBJSUFX)

TARGETS = runtime_setup Jruntime_setup \
	$(TIMER_OBJS) \
	$(GSK_LIB) \
	icctest$(EXESUFX) \
	$(SDK_TARGETS) \
	$(GSK_OPENSSL) \
	$(GSK_BVT) \
	$(JGSK_TARGETS) $(JSDK_TARGETS)


#	cache_test$(EXESUFX) Specify manually as a target, may not build/run on all platforms due
#  to duplaicted objects in the executable


all:  $(TARGETS)




include ../icc/zlib.mk

# Disabled. The reference code doesn't implement the spec.
#include Argon2/Argon2.mk

# Dilithium/Kyber Post-Quantum algorithms
#include DilKyb/DilKyb.mk



ICCPKG_OBJS =
# $(argon2_obj) $(DilKyb_obj) totp$(OBJSUFX)


ICCPKG_GFX = \
	../doc/graphics/ICCPKG.png \
	../doc/graphics/ICC_func.png

ICCPKG_DOC_PARTS = \
   ../doc/iccpkg_main.h \
   ../doc/iccpkg.h \
   ../doc/functional.h \
   ../doc/implementation.h \
   ../doc/random.h \
   ../doc/KDF.h \
   ../doc/KW.h \
   ../doc/config_file.h \
   ../doc/sdk.h \
   ../doc/regression.h \
   ../doc/hardware.h \
    ../doc/microbench.h \
   ../doc/troubleshooting.h \
   ../doc/problem_determination.h \
   ../doc/security.h \
   ../doc/performance.h \
   ../doc/glossary.h \
   ../doc/changes.h \
   ../doc/end.h

#
# DOC source - NOT a shipped file
#
iccpkg.h: $(ICCPKG_DOC_PARTS)
	cat $(ICCPKG_DOC_PARTS) >$@

icc_a.h: iccpkg_a.h
	$(CP) iccpkg_a.h $@

iccversion.h: ../icc/iccversion.h
	$(CP) ../icc/iccversion.h $@

icc.h: ../icc/icc.h
	$(CP) ../icc/icc.h $@

iccglobals.h: ../icc/iccglobals.h
	$(CP) ../icc/iccglobals.h $@

delta$(OBJSUFX): ../icc/delta$(OBJSUFX)
	$(CP) ../icc/delta$(OBJSUFX) $@

looper$(OBJSUFX): ../icc/looper$(OBJSUFX)
	$(CP) ../icc/looper$(OBJSUFX) $@

iccstub$(OBJSUFX): ../icc/iccstub$(OBJSUFX)
	$(CP) ../icc/iccstub$(OBJSUFX) $@

high_res_timer$(OBJSUFX): ../icc/high_res_timer$(OBJSUFX)
	$(CP) ../icc/high_res_timer$(OBJSUFX) $@

# Needed on some platforms $(ASMOBJS)
rng-ppc.o: ../icc/rng-ppc.o
	$(CP) ../icc/rng-ppc.o $@

RdCTR_raw.o: ../icc/RdCTR_raw.o
	$(CP) ../icc/RdCTR_raw.o $@


TIMER_OBJS = 	delta$(OBJSUFX) iccstub$(OBJSUFX) \
		high_res_timer$(OBJSUFX) $(ASMOBJS) \
		
#		$(argon2_obj) $(DilKyb_obj) 

JTIMER_OBJS = 	delta$(OBJSUFX) iccstub$(OBJSUFX) \
		high_res_timer$(OBJSUFX) $(ASMOBJS) \

#		$(argon2_obj) $(DilKyb_java_obj) 



$(GSK_SDK):
	$(MKDIR) $(GSK_SDK)

$(GSK_DIR):
	$(MKDIR) $(GSK_DIR)

$(GSK_SDK)/icc.h: $(GSK_SDK) icc.h
	$(CP) icc.h $@

$(GSK_SDK)/icc_a.h: $(GSK_SDK) icc_a.h
	$(CP) icc_a.h $@

$(GSK_SDK)/icc_aux.h: $(GSK_SDK) icc_aux.h
	$(CP) icc_aux.h $@

$(GSK_SDK)/icc_aux_a.h: $(GSK_SDK) icc_aux_a.h
	$(CP) icc_aux_a.h $@

$(GSK_SDK)/iccglobals.h: $(GSK_SDK) iccglobals.h
	$(CP) iccglobals.h $@


$(GSK_SDK)/delta_t.h: $(GSK_SDK) ../icc/DELTA/delta_t.h
	$(CP) ../icc/DELTA/delta_t.h $@

$(GSK_SDK)/GSKit_Crypto.pdf: $(GSK_SDK) ../doc/GSKit_Crypto.pdf
	$(CP) ../doc/GSKit_Crypto.pdf $@



$(INST_DIR):
	$(MKDIR) $(INST_DIR)


runtime_setup_N: $(GSK_SDK) $(INST_DIR)/N/icc/icclib/$(NEW_LIBICC) \
		$(INST_DIR)/N/icc/ReadMe.txt \
		$(GSK_SDK)/delta_t.h

$(INST_DIR)/N/icc/ReadMe.txt:
	-$(CP) $(NEW_ICC)/icc/ReadMe.txt $@


$(INST_DIR)/N/icc/icclib/$(NEW_LIBICC): $(NEW_ICC)/icc/icclib/$(NEW_LIBICC)
	-$(MKDIR) $(INST_DIR)/N/icc/icclib
	$(CP) $(NEW_ICC)/icc/icclib/* $(INST_DIR)/N/icc/icclib/


runtime_setup: $(INST_DIR) runtime_setup_N



clean: clean_jgsk
	-$(RM)	icctest$(EXESUFX)
	-$(RM)  smalltest$(EXESUFX) memleak$(EXESUFX)  \
		smalltest1$(EXESUFX) smalltest2$(EXESUFX) smalltest4$(EXESUFX) \
		smalltest5$(EXESUFX) \
		cache_test$(EXESUFX) \
		smalltestW$(EXESUFX)
	-$(RM)  *.so *.dylib *.dll *.sl *.x *.lib
	-$(RM) -r $(GSK_LIB)
	-$(RM) 	$(GSK_LIB).unstripped
	-$(RM)	gsk_wrap2$(OBJSUFX) \
		icctest$(OBJSUFX) \
		$(TIMER_OBJS) $(JTIMER_OBJS) smalltest1$(OBJSUFX) \
		cache_test$(OBJSUFX) \
		jgsk_wrap2$(OBJSUFX)
	-$(RM)  loaded.c loaded.h tracer.h jrng_real.c jhkdf.c cache_test.c
	-$(RM) 	iccpkg.h
	-$(RM) 	API_html/*
	-$(RM) 	$(INST_DIR)/N/icc/icclib/$(NEW_LIBICC)
	-$(RM) -r $(INST_DIR)/* 
	-$(RM) -r $(GSK_SDK)/*
	-$(RM) -r $(TMP_SRC) $(TMP_OBJS)


# Need to rm after copy as Windows looks in the same directory as exe's
# for DLL's, but in this case the rest of the directory structure is 
# missing
# Note the target for $(GSK_LIBNAME) is set in platforms/$(OPSYS)_.mk using GSK_LIB_B and must match this one
$(GSK_LIB): $(GSK_SDK) $(GSK_LIBNAME)
	-$(CP) $(GSK_LIBNAME) $@
	-$(RM) $(GSK_LIBNAME)
	touch keep_tar_quiet.pdb
	-$(CP) *.pdb $(GSK_SDK)
	touch  ../package/iccsdk/keep_tar_quiet.pdb
	-$(CP) ../package/iccsdk/*.pdb $(GSK_SDK)


# Separate rules for Windows and "other" platforms
# so the libs can be signed 
# and we have the 32 and 64 bit variants ... ugly, really ugly
# plus z/OS which doesn't link normally
#
# These have been moved into platforms/${OPSYS}_.mk
#

# Platform specifc recipies for building libs
include platforms/$(OPSYS)_.mk

gsk_wrap2$(OBJSUFX): gsk_wrap2.c gsk_wrap2_a.c ../icc/loaded.c name_cache.c name_cache_tables.c \
	../icc/loaded.h icc.h icc_a.h iccglobals.h iccversion.h ../icc/tracer.h
	$(CC) $(CFLAGS) -DICCPKG -DMYNAME=gskiccs8 -DGSK_LIBNAME=\""$(GSK_LIBNAME)"\" -DOPSYS="\"$(OPSYS)\"" -DGSK_GLOBAL=\""$(GSK_GLOBAL)"\" -I ./ -I $(ICC_ROOT)/icc gsk_wrap2.c

#loaded.c: ../icc/loaded.c
#	$(CP) ../icc/loaded.c $@

#loaded.h: ../icc/loaded.h
#	$(CP) ../icc/loaded.h $@

#tracer.h: ../icc/tracer.h
#	$(CP) ../icc/tracer.h $@

$(NEW_ICC)/iccsdk/$(ICCLIB): runtime_setup_N

OS_helpers$(OBJSUFX): ../icc/OS_helpers.c
	$(CC) $(CFLAGS) ../icc/OS_helpers.c $(OUT)$@

#hkdf$(OBJSUFX): HKDF/hkdf.c HKDF/hkdf.h
#	$(CC) $(CFLAGS) -I$(OSSL_DIR)/include -I $(GSK_SDK) HKDF/hkdf.c

#jhkdf.c: HKDF/hkdf.c
#	$(CP) HKDF/hkdf.c $@


#jhkdf$(OBJSUFX): jhkdf.c 
#	$(CC) $(CFLAGS) -DJGSK_WRAP -I$(OSSL_DIR)/include -I $(GSK_SDK) jhkdf.c

icctest.c: ../icc/icctest.c
	$(CP) ../icc/icctest.c .

icc.res: ../icc/icc.res
	$(CP) ../icc/icc.res $@

cache_test$(OBJSUFX): gsk_wrap2.c gsk_wrap2_a.c ../icc/loaded.c name_cache.c name_cache_tables.c \
	../icc/loaded.h icc.h icc_a.h iccglobals.h iccversion.h ../icc/tracer.h 
	$(CC) $(CFLAGS) -I../icc/DELTA -DSTANDALONE -DICCPKG -DMYNAME=gskiccs8 -DGSK_LIBNAME=\""$(GSK_LIBNAME)"\"  -DOPSYS="\"$(OPSYS)\"" -DGSK_GLOBAL=\""$(GSK_GLOBAL)"\" -I ./ -I $(ICC_ROOT)/icc gsk_wrap2.c $(OUT)$@


icctest$(EXESUFX): icctest.c $(GSK_SDK)
	$(CC) $(CFLAGS) -I./ -I ../icc -DICCPKG  icctest.c 
	$(LD) $(LDFLAGS) icctest$(OBJSUFX) $(ICCPKG_LIBS) $(LDLIBS) 
	$(CP) icctest$(EXESUFX) $(GSK_SDK)/

smalltest$(EXESUFX):  ../icc/tools/smalltest.c
	$(CC) $(CFLAGS) -I./ -I ../icc -DICCPKG ../icc/tools/smalltest.c
	$(LD) $(LDFLAGS) smalltest$(OBJSUFX) $(ICCPKG_LIBS) $(LDLIBS) 

# GSK_LIB and ICCPKG_LIBS is coming from gsk_crypto.mk - references the step import library
# IS_FIPS and MUPPET comes from muppet.mk


$(GSK_OPENSSL): ../package/iccsdk/openssl$(EXESUFX)
	$(CP) ../package/iccsdk/openssl$(EXESUFX) $@

$(GSK_BVT): icctest$(EXESUFX)
	$(CP) icctest$(EXESUFX) $@

#
# -------------- JCEPlus variants -------------
#
# Namespaced and with a different shared library name to avoid direct conflicts 
# between Java installed libraries and the C/C++ stack.
#
# To date problems were only seen on AIX, but lets use the big hammer
# and keep it like that.
#


$(JINST_DIR):
	$(MKDIR) $(JINST_DIR)

Jruntime_setup_N: $(JINST_DIR)/N/icc/icclib/$(NEW_LIBICC) \
		$(JINST_DIR)/N/icc/ReadMe.txt

$(JINST_DIR)/N/icc/ReadMe.txt: $(JINST_DIR)
	$(CP) $(NEW_ICC)/icc/ReadMe.txt $@


$(JINST_DIR)/N/icc/icclib/$(NEW_LIBICC):
	-$(MKDIR) $(JINST_DIR)/N/icc/icclib
	-$(CP) $(NEW_ICC)/icc/icclib/*  $(JINST_DIR)/N/icc/icclib/


Jruntime_setup: $(JINST_DIR) Jruntime_setup_N

$(JGSK_DIR):
	$(MKDIR) $(JGSK_DIR)

$(JGSK_LIB): $(JGSK_SDK) $(JGSK_DIR) $(JGSK_LIBNAME)
	-$(MKDIR) $(JGSK_SDK)/debug
	-$(CP) $(JGSK_LIBNAME) $@
	-$(RM) $(JGSK_LIBNAME)


jgsk_wrap2$(OBJSUFX): gsk_wrap2.c gsk_wrap2_a.c ../icc/loaded.c name_cache.c name_cache_tables.c \
	../icc/loaded.h icc.h icc_a.h iccglobals.h iccversion.h ../icc/tracer.h jcc_a.h
	$(CC) $(CFLAGS) -DICCPKG -DJGSK_WRAP -DMYNAME=jgskiccs8 -DGSK_LIBNAME=\""$(JGSK_LIBNAME)"\" -DOPSYS="\"$(OPSYS)\"" -DGSK_GLOBAL=\""$(GSK_GLOBAL)"\" -I ./ -I $(ICC_ROOT)/icc gsk_wrap2.c $(OUT)$@

#jgsk_wrap2.c: gsk_wrap2.c
#	$(CP) gsk_wrap2.c jgsk_wrap2.c

clean_jgsk:
	-$(RM) $(JGSK_DIR)/N/icc/icclib/*
	-$(RM) $(JGSK_DIR)/C/icc/icclib/*
	-$(RM) $(JGSK_SDK)/inc/*.h
	-$(RM) $(JGSK_SDK)/debug/*
	-$(RM) $(JGSK_SDK)/docs/*
	-$(RM) $(JGSK_SDK)/bin/jcctest$(EXESUFX)
	-$(RM) $(JGSK_LIBNAME) jcctest$(EXESUFX)
	-$(RM) -r $(JGSK_LIB)

#
# JGSK BVT executable
#

jcctest.c: icctest.c
	$(CP) icctest.c $@
#
# Common tables for name/nid caches
#
name_cache_tables.c: ../icc/name_cache_tables.c
	$(CP) ../icc/name_cache_tables.c $@

jcctest$(EXESUFX):  jcctest.c $(JGSK_LIB)
	$(CC) $(CFLAGS) -I./ -DICCPKG -DJGSK_WRAP jcctest.c $(OUT)jcctest$(OBJSUFX)
	$(LD) $(LDFLAGS) jcctest$(OBJSUFX) $(JCCPKG_LIBS) $(LDLIBS) 


$(JGSK_SDK):
	$(MKDIR) $(JGSK_SDK)

$(JGSK_SDK)/bin/jcctest$(EXESUFX): $(JGSK_SDK) jcctest$(EXESUFX)
	-$(MKDIR) $(JGSK_SDK)/bin
	-$(CP) jcctest$(EXESUFX) $@


$(JGSK_SDK)/docs/GSKit_Crypto.pdf: $(JGSK_SDK) ../doc/GSKit_Crypto.pdf
	-$(MKDIR) $(JGSK_SDK)/docs
	-$(MKDIR) $(JGSK_SDK)/bin
	$(CP) ../doc/GSKit_Crypto.pdf $@

$(JGSK_SDK)/inc: $(JGSK_SDK)
	$(MKDIR) $(JGSK_SDK)/inc

$(JGSK_SDK)/inc/icc.h: $(JGSK_SDK)/inc icc.h
	$(CP) icc.h $@

$(JGSK_SDK)/inc/iccglobals.h: iccglobals.h
	$(CP) iccglobals.h $@

$(JGSK_SDK)/inc/icc_a.h: icc_a.h
	$(CP) icc_a.h $@

$(JGSK_SDK)/inc/jcc_a.h: jcc_a.h
	$(CP) jcc_a.h $@

# Tests. BVT
# GSK_SETUP comes from gsk_crypto.mk and sets PATH for these tests
# ignored errors will be reported in the log
PATH:=..\package\gskit_crypto;$(PATH)
tests:
	$(RM) GSKIT_CRYPTO.log; touch GSKIT_CRYPTO.log;
	$(GSK_SETUP);
	cat GSKIT_CRYPTO.log; $(RM) GSKIT_CRYPTO.log; touch GSKIT_CRYPTO.log;
	$(GSK_SETUP); ./icctest
	cat GSKIT_CRYPTO.log; $(RM) GSKIT_CRYPTO.log;

#
# ---------------- End of JCEPlus stanzas --------------------------
#

#- Assist in debugging Makefiles, show config. Also useful for build.
show_config:
	echo "Set in Makefile"
	echo "=========== Config ================"
	echo "OPSYS  				= $(OPSYS)" 
	echo "CONFIG 				= $(CONFIG)"
	echo "BUILD 				= $(BUILD)"
	echo "ZLIB					= $(ZLIB)"
	echo "XPLINK				= $(XPLINK)"
	echo "========== Directories ============"
	echo "ZLIB_DIR    		= $(ZLIB_DIR)"
	echo "GSK_DIR				= $(GSK_DIR)"
	echo "GSK_SDK				= $(GSK_SDK)"
	echo "============= Files ==============="
	echo "TARGETS				= $(TARGETS)"
	echo "ICCLIB				= $(ICCLIB)"
	echo "From platforms.mk"
	echo "========= System utils ============"
	echo "RM 					= $(RM)"
	echo "MKDIR 				= $(MKDIR)"
	echo "CP						= $(CP)"
	echo "DEBUGGER				= $(DEBUGGER)"
	echo "CC						= $(CC)"
	echo "CXX					= $(CXX)"
	echo "CFLAGS				= $(CFLAGS)"
	echo "CFLAGS2				= $(CFLAGS2)"
	echo "LD						= $(LD)"
	echo "LD_CXX				= $(LD_CXX)"
	echo "SLD					= $(SLD)"
	echo "AR						= $(AR)"
	echo "STRIP					= $(STRIP)"
	echo "ICC_RUN_SETUP     = $(ICC_RUN_SETUP)"
	echo "=========== Flags ================="
	echo "LDFLAGS				= $(LDFLAGS)"
	echo "SLDFLAGS				= $(SLDFLAGS)"
	echo "LDFLAGS2				= $(LDFLAGS2)"
	echo "SLDFLAGS2			= $(SLDFLAGS2)"
	echo "ARFLAGS				= $(ARFLAGS)"
	echo "LDLIBS				= $(LDLIBS)"
	echo "OUT               = $(OUT)"
	echo "======= Filename extensions ======="
	echo "OBJSUFX				= $(OBJSUFX)"
	echo "EXESUFX				= $(EXESUFX)"
	echo "SHLPRFX				= $(SHLPRFX)"
	echo "SHLSUFX				= $(SHLSUFX)"
	echo "STLPRFX				= $(STLPRFX)"
	echo "STLSUFX				= $(STLSUFX)"
	echo 
	echo "Set in iccpkg.mk"
	echo "NEW_LIBICC			= $(NEW_LIBICC)"
	echo "ICCPKG_LIBS       = $(ICCPKG_LIBS)"
	echo
	echo "Set in icc_defs.mk"
	echo "TARGETS           = $(TARGETS)"
	echo "OPSYS     			= $(OPSYS)"
	echo "Linker    			= $(SLD)"
	echo "Link flag 			= $(SLDFLAGS)"
	echo "Compiler  			= $(CC)"
	echo "CFLAGS   	 		= $(CFLAGS)"
	echo "Export flag			= $(EXPORT_FLAG)"
	echo "Export defs.      = $(ICCPKG_EXPFILE)"
	echo "tar	      		= $(TARCMD)"
	echo "OpenSSL defs set in icc_defs.mk"
	echo "OSSLLIB_NAME	   = $(OSSLLIB_NAME)"
	echo "OSSLDLL_NAME	   = $(OSSLDLL_NAME)"
	echo "MY_OSSLDLL_NAME	= $(MY_OSSLDLL_NAME)"
	echo "OSSLINC_DIR	      = $(OSSLINC_DIR)"
	echo
	echo "Set in gsk_crypto.mk - libs for current GSkit"
	echo "GSKLIB		      = $(GSKLIB)"
	echo "GSK_LIBNAME       = $(GSK_LIBNAME)"
	echo "GSK_LIB           = $(GSK_LIB)"
	echo "ICCPKG_LIBS       = $(ICCPKG_LIBS)"
	echo "ICCPKG_OBJS       = $(ICCPKG_OBJS)"
	echo "GSK_GLOBAL        = $(GSK_GLOBAL)"
	echo
	echo " Namespaced variant for JCEPlus"
	echo "JGSK_DIR           = $(JGSK_DIR)"
	echo "JGSK_SDK           = $(JGSK_SDK)"
	echo "JGSK_LIB           = $(JGSK_LIB)"
	echo "JGSKLIB_B          = $(JGSKLIB_B)"
	echo "JCCPKG_LIBS        = $(JCCPKG_LIBS)"
	echo "JGSK_TARGETS       = $(JGSK_TARGETS)"
	echo "JSDK_TARGETS       = $(JSDK_TARGETS)"
	echo
	echo "Library search path for BVT/FVT"
	echo "GSK_SETUP			 = $(GSK_SETUP)"

