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

# Allow us to build a gskit_crypto without the OLD icc present
include muppet.mk

include $(ICC_ROOT)/icc/VTAG.mk
include $(ICC_ROOT)/icc/platforms.mk

include $(ICC_ROOT)/icc/icc_defs.mk
include $(ICC_ROOT)/icc/opensslver.mk

# GSKit-Crypto defs
# defines JGSK_DIR and GSK_DIR ($(ICC_ROOT)/package/gskit_crypto)
include gsk_crypto.mk

## Build specific definitions - file target etc.
include iccpkg.mk

developer_CFLAGS = -DVTAG=$(VTAG) -DINSTDIR=\""$(GSK_GLOBAL)"\"
OFFICIAL_CFLAGS = -DICC_OFFICIAL_BUILD $(developer_CFLAGS)


ICCLIB		= $(STLPRFX)icc$(STLSUFX)
#STKPK11		= pkcs11_iccpkg_container$(OBJSUFX)

# Note: match same definitions in icc/Makefile
PACKAGE_DIR    = $(ICC_ROOT)/package
RTE_DIR        = $(PACKAGE_DIR)/icc
SDK_DIR        = $(PACKAGE_DIR)/iccsdk

# platforms/*.mk are still using this alias
NEW_ICC = $(PACKAGE_DIR)

ZLIB_LIB 	= $(ICC_ROOT)/icc/$(STLPRFX)zlib$(STLSUFX)

# Version # of the old ICC from VTAG.mk
OLDVER = $(FVTAG)
OLD_LIBICC 	= $(SHLPRFX)icclib$(OLDVER)$(SHLSUFX)
# ref ICCDLL_NAME
NEW_LIBICC	= $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)

# This gets linked into step library for ICCN_ symbols
ifeq ($(strip $(IS_FIPS)),)
   MYICC = newicc
else
   MYICC = icc
endif		

# local alias directory names - deprecated and only used in this Makefile
# These are the runtime directories (package/*gskit_crypto/)
INST_DIR	= $(GSK_DIR)
JINST_DIR   = $(JGSK_DIR)


# Sign tool for Microsoft Authenticode
ifeq ($(findstring VS2022, $(OPSYS)), VS2022)
SIGN_COMMAND = ../../signwindowsfile.ksh
else
SIGN_COMMAND    = /build/build/scripts/signwindowsfile.ksh
endif

# IBM Crypto for C perf tool
ICCPKG_PERF	= $(GSK_SDK)/icc_perf$(EXESUFX)

# IBM Crypto for C perf tool (Java variant)
JICC_PERF	= $(JGSK_SDK)/bin/jicc_perf$(EXESUFX)

# PKCS#11 perf tool
PK11_PERF	= $(GSK_SDK)/pkcs11_thread$(EXESUFX)

#Random number generator
GSK_RNG 	= $(GSK_SDK)/GenRndData2$(EXESUFX)

#ICC RNG core
ICC_RNG 	= $(GSK_SDK)/GenRndData$(EXESUFX)

# OpenSSL command line
GSK_OPENSSL     = $(GSK_SDK)/openssl$(EXESUFX)

$(GSK_DIR)/Standard_International_Program_License_Agreement.pdf: $(GSK_DIR) $(ICC_ROOT)/icc/Standard_International_Program_License_Agreement.pdf
	$(CP) $(ICC_ROOT)/icc/Standard_International_Program_License_Agreement.pdf $@
	chmod +w $@

$(GSK_SDK)/Standard_International_Program_License_Agreement.pdf: $(GSK_SDK) $(ICC_ROOT)/icc/Standard_International_Program_License_Agreement.pdf
	$(CP) $(ICC_ROOT)/icc/Standard_International_Program_License_Agreement.pdf $@
	chmod +w $@

SDK_TARGETS = \
		$(GSK_RNG) \
		$(GSK_OPENSSL) \
		$(GSK_SDK)/icctest$(EXESUFX) \
		$(GSK_SDK)/chkiccload$(EXESUFX) \
		$(GSK_SDK)/sigtest$(EXESUFX) \
		$(GSK_SDK)/kemtest$(EXESUFX) \
		$(GSK_SDK)/sigtest.c \
		$(GSK_SDK)/kemtest.c


#		$(ICC_RNG) \

JSDK_TARGETS = 	\
		$(JGSK_SDK)/inc/icc.h \
		$(JGSK_SDK)/inc/iccglobals.h \
		$(JGSK_SDK)/inc/icc_a.h \
		$(JGSK_SDK)/inc/jcc_a.h \
		$(JGSK_SDK)/bin/jcctest$(EXESUFX)

#		$(JGSK_SDK)/inc/hkdf.h \

# Note the lib/ dir in the JSDK only exists on windows and 
# is built & populated when we build the shared lib

JGSK_TARGETS = \
		$(JGSK_LIB)


ICKC_TARGETS = $(ICKC_LIB)
ICKCSDK_TARGETS : runtime_setup sdk_setup Jruntime_setup $(ICKC_SDK) \
		$(ICKC_SDK)/inc/icc.h \
		$(ICKC_SDK)/inc/iccglobals.h \
		$(ICKC_SDK)/inc/icc_a.h \
		$(ICKC_SDK)/inc/ickc_a.h \
		$(ICKC_SDK)/bin/ickctest$(EXESUFX)
	echo $@ made

#
# Source .[c|h] files created as part of the build
#
TMP_SRC = iccpkg.h icc_a.h jcctest.c ickctest.c

#
# objects copied from elsewhere, don't exist on all platforms !
#
TMP_OBJS = delta$(OBJSUFX) iccstub$(OBJSUFX) high_res_timer$(OBJSUFX) \
	rng-ppc.o RdCTR_raw.o $(STLPRFX)pkcs11_iccpkg_api$(STLSUFX) \
	looper$(OBJSUFX)

#
# Extra function only in the current step library
#
EX_OBJS =
#EX_OBJS = exp$(OBJSUFX) totp$(OBJSUFX)
JEX_OBJS =
#JEX_OBJS = jexp$(OBJSUFX)

TARGETS = \
	$(TIMER_OBJS) \
	icctest$(EXESUFX) \
	icctest_s$(EXESUFX) \
	GenRndData2$(EXESUFX) \
	smalltest$(EXESUFX) \
	$(JGSK_TARGETS) $(JSDK_TARGETS) \
	$(ICKC_TARGETS) ICKCSDK_TARGETS

SDK_HEADERS = $(GSK_SDK)/icc.h $(GSK_SDK)/icc_a.h $(GSK_SDK)/iccglobals.h

# our local dks integration and test code
# sigtest and kemtest comes from here
pqc_test_build:
	$(MAKE) -C pqc all

# only run the pqc test if we have pqc support present
ifeq ($(strip $(PQCLIBS)),)
pqc_tests:
	echo "No dks pqc support"
else
pqc_tests:
	$(MAKE) -C pqc tests
endif

#	cache_test$(EXESUFX) Specify manually as a target, may not build/run on all platforms due
#  to duplaicted objects in the executable



# note: from gsk_crypto.mk GSK_LIB = $(GSK_DIR)/$(GSK_LIBNAME)

all: $(SDK_HEADERS) $(GSK_LIB) $(GSK_LIB_STATIC) runtime_setup sdk_setup Jruntime_setup $(TARGETS) pqc_test_build $(SDK_TARGETS)

include $(ICC_ROOT)/icc/zlib.mk

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

# iccpkg_a.h is written by the ICCencapsulator but is always copied to icc_a.h to be used so is not the same as icc/icc_a.h
# it has the ICC_ API names for the step library and NOT the NICC or CICC names that the module gets.
# This header substitution is what allows the icctest.c source to be built against the module (in icc) or the step library (in iccpkg)
#icc_a.h: iccpkg_a.h
#	$(CP) iccpkg_a.h $@

delta$(OBJSUFX): $(ICC_ROOT)/icc/delta$(OBJSUFX)
	$(CP) $(ICC_ROOT)/icc/delta$(OBJSUFX) $@

looper$(OBJSUFX): $(ICC_ROOT)/icc/looper$(OBJSUFX)
	$(CP) $(ICC_ROOT)/icc/looper$(OBJSUFX) $@

iccstub$(OBJSUFX): $(ICC_ROOT)/icc/iccstub$(OBJSUFX)
	$(CP) $(ICC_ROOT)/icc/iccstub$(OBJSUFX) $@

high_res_timer$(OBJSUFX): $(ICC_ROOT)/icc/high_res_timer$(OBJSUFX)
	$(CP) $(ICC_ROOT)/icc/high_res_timer$(OBJSUFX) $@

# Needed on some platforms $(ASMOBJS)
rng-ppc.o: $(ICC_ROOT)/icc/rng-ppc.o
	$(CP) $(ICC_ROOT)/icc/rng-ppc.o $@

RdCTR_raw.o: $(ICC_ROOT)/icc/RdCTR_raw.o
	$(CP) $(ICC_ROOT)/icc/RdCTR_raw.o $@

# note: not $(ARGON) here - step library has no blake2b clash

TIMER_OBJS = 	delta$(OBJSUFX) iccstub$(OBJSUFX) \
		high_res_timer$(OBJSUFX) $(ASMOBJS) \
		$(argon2_obj) $(DilKyb_obj) looper$(OBJSUFX)

JTIMER_OBJS = 	delta$(OBJSUFX) iccstub$(OBJSUFX) \
		high_res_timer$(OBJSUFX) $(ASMOBJS) \
		$(argon2_obj) $(DilKyb_java_obj) looper$(OBJSUFX)

# no experimental APIs - these are integrated into the module now.
ICKCTIMER_OBJS = delta$(OBJSUFX) iccstub$(OBJSUFX) \
		high_res_timer$(OBJSUFX) $(ASMOBJS) \
		looper$(OBJSUFX)

# edit the base Doxyfile to convert project heading "(confidential)" to the current release version info

Doxyfile: ICCPKG_Doxyfile $(ICC_ROOT)/icc/ICC_ver.txt docbits/h.txt docbits/t.txt
	$(CP) $(ICC_ROOT)/icc/ICC_ver.txt docbits/ver.txt
	sed -i 's/_/./g' docbits/ver.txt
	cat docbits/h.txt > docbits/sedcmd
	echo -n `cat docbits/ver.txt` >> docbits/sedcmd
	cat docbits/t.txt >> docbits/sedcmd
	sed -f docbits/sedcmd ICCPKG_Doxyfile >Doxyfile

doc:IBM_CKC.pdf

IBM_CKC.pdf: iccpkg.h iccpkg_a.h $(ICC_ROOT)/icc/icc.h $(ICC_ROOT)/icc/iccglobals.h Doxyfile $(ICC_ROOT)/icc/ICC_ver.txt
	doxygen Doxyfile
	$(CP) $(ICCPKG_GFX) API_html/
	( \
		$(CP) $(ICC_ROOT)/doc/doxygen.sty latex/ ; \
		perl ./makePDFver.pl ; \
		cd latex ; \
		rm -f *.ps *.dvi *.aux *.toc *.idx *.ind *.ilg *.log *.out *.brf *.blg *.bbl refman.pdf; \
		pdflatex refman; \
		makeindex refman.idx ; \
		pdflatex refman; \
		latex_count=5 ; \
		while egrep -s 'Rerun (LaTeX|to get cross-references right)' refman.log && [ $$latex_count -gt 0 ] ;\
		do \
			echo "Rerunning latex...." ;\
			pdflatex refman ;\
			latex_count=`expr $$latex_count - 1` ;\
		done; \
		$(CP) refman.pdf IBM_CKC.pdf ;\
		cd ..;\
	)


# Headers for experimental algorithms
$(GSK_SDK)/gsk_argon2.h: $(GSK_SDK) Argon2/gsk_argon2.h
	$(CP) Argon2/gsk_argon2.h $@

$(GSK_SDK)/pqapi.h: $(GSK_SDK) DilKyb/headers/pqapi.h
	$(CP) DilKyb/headers/pqapi.h $@

$(GSK_SDK)/pqparams.h: $(GSK_SDK) DilKyb/headers/pqparams.h
	$(CP) DilKyb/headers/pqparams.h $@

$(GSK_SDK)/gsk_exp.h: $(GSK_SDK) gsk_exp.h
	$(CP) gsk_exp.h $@

# deprecated - use newer API - ref kemtest.c
# Example code for 'experimental' algorithms
$(GSK_SDK)/PQCgenKAT_sign.c: $(GSK_SDK) DilKyb/dilithium/PQCgenKAT_sign.c
	$(CP) DilKyb/dilithium/PQCgenKAT_sign.c $@

$(GSK_SDK)/PQCgenKAT_kem.c: $(GSK_SDK) DilKyb/kyber/PQCgenKAT_kem.c
	$(CP) DilKyb/kyber/PQCgenKAT_kem.c $@
# end deprecated

$(GSK_SDK)/argon2_example.c: $(GSK_SDK) Argon2/argon2_example.c
	$(CP) Argon2/argon2_example.c $@

pqc/kemtest$(EXESUFX):
	$(MAKE) -C pqc all

$(GSK_SDK)/kemtest$(EXESUFX): $(GSK_SDK) pqc/kemtest$(EXESUFX)
	$(CP) pqc/kemtest$(EXESUFX) $@

$(GSK_SDK)/kemtest.c: $(GSK_SDK) pqc/kemtest.c
	$(CP) pqc/kemtest.c $@

pqc/sigtest$(EXESUFX):
	$(MAKE) -C pqc all

$(GSK_SDK)/sigtest$(EXESUFX): $(GSK_SDK) pqc/sigtest$(EXESUFX)
	$(CP) pqc/sigtest$(EXESUFX) $@

$(GSK_SDK)/sigtest.c: $(GSK_SDK) pqc/sigtest.c
	$(CP) pqc/sigtest.c $@

OS_helpers.c: $(ICC_ROOT)/icc/OS_helpers.c
	$(CP) $(ICC_ROOT)/icc/OS_helpers.c $@

$(GSK_SDK)/icc_aux.h: $(GSK_SDK) icc_aux.h
	$(CP) icc_aux.h $@

$(GSK_SDK)/icc_aux_a.h: $(GSK_SDK) icc_aux_a.h
	$(CP) icc_aux_a.h $@

$(GSK_SDK)/totp.h: $(GSK_SDK) TOTP/totp.h
	$(CP) TOTP/totp.h $@
	
$(GSK_SDK)/delta_t.h: $(GSK_SDK) $(ICC_ROOT)/icc/DELTA/delta_t.h
	$(CP) $(ICC_ROOT)/icc/DELTA/delta_t.h $@

$(ICC_ROOT)/doc/IBM_CKC.pdf:
	echo "this needs to be checked in after building docs"

$(GSK_SDK)/IBM_CKC.pdf: $(GSK_SDK) $(ICC_ROOT)/doc/IBM_CKC.pdf
	$(CP) $(ICC_ROOT)/doc/IBM_CKC.pdf $@

# if it is a no_legacy build then delete the C dir completely

runtime_setup_C: $(INST_DIR)/C/icc/icclib/$(OLD_LIBICC) $(INST_DIR)/C/icc/ReadMe.txt
	if [ -f $(ICC_ROOT)/icc/create_icc_no_legacy.0 ]; then echo "No FIPS ICC build"; fi ;
	if [ -f $(ICC_ROOT)/icc/create_icc_no_legacy.0 ]; then $(RM) -r $(INST_DIR)/C; fi ;

sdk_setup_C: $(GSK_SDK)/icc.h $(GSK_SDK)/icc_a.h $(GSK_SDK)/iccglobals.h

#		$(GSK_SDK)/pqapi.h $(GSK_SDK)/pqparams.h \
#		$(GSK_SDK)/gsk_argon2.h $(GSK_SDK)/gsk_exp.h

#		$(GSK_SDK)/hkdf.h

# create archive to upload to github release - ref icc/Makefile create_icc and OLD_ICC/$(OPSYS) targets
# this needs to be loaded manually into a github release
oldicc_$(OPSYS).tar.gz: OLD_ICC/$(OPSYS)
	tar czf $@ $<

$(INST_DIR)/C : $(INST_DIR)
	$(MKDIR) $@

$(INST_DIR)/C/icc : $(INST_DIR)/C
	$(MKDIR) $@

$(INST_DIR)/C/icc/icclib: $(INST_DIR)/C/icc
	$(MKDIR) $@

$(INST_DIR)/C/icc/ReadMe.txt: $(INST_DIR)/C/icc/icclib
	-$(CP) OLD_ICC/AMD64_LINUX/icc/ReadMe.txt $@

# OLD_ICC has checked in binaries so enforce correct permissions
# ignore errors because it may not be present
# for a no legacy build we will remove the FIPS lib if present
$(INST_DIR)/C/icc/icclib/$(OLD_LIBICC): $(INST_DIR)/C/icc/icclib
	-$(CP) $(OLD_ICC)/icc/icclib/* $(INST_DIR)/C/icc/icclib/
	-chmod +x $@

# if we are building FIPS then there is no legacy and we only have C which we just built in N - so rename it
runtime_setup_N: $(INST_DIR)/N/icc/icclib/$(NEW_LIBICC) $(INST_DIR)/N/icc/ReadMe.txt
	if [ -f $(ICC_ROOT)/icc/create_fips.0 ]; then mv $(INST_DIR)/N $(INST_DIR)/C; fi ;

# unstripped modules will need matching ICCSIG.txt and be OS signed on some platforms.
#$(INST_DIR)/N/icc/icclib/$(NEW_LIBICC).unstripped: $(GSK_SDK)/$(NEW_LIBICC).unstripped
#	$(CP) $(GSK_SDK)/$(NEW_LIBICC).unstripped $@

sdk_setup_N: $(GSK_SDK)/delta_t.h

$(INST_DIR)/N : $(INST_DIR)
	$(MKDIR) $@

$(INST_DIR)/N/icc : $(INST_DIR)/N
	$(MKDIR) $@

$(INST_DIR)/N/icc/icclib: $(INST_DIR)
	$(MKDIR) $@

$(INST_DIR)/N/icc/ReadMe.txt: $(INST_DIR)/N/icc/icclib $(RTE_DIR)/ReadMe.txt
	$(CP) $(RTE_DIR)/ReadMe.txt $@

$(INST_DIR)/N/icc/icclib/$(NEW_LIBICC): $(INST_DIR)/N/icc/icclib $(RTE_DIR)/icclib/$(NEW_LIBICC)
	$(CP) $(RTE_DIR)/icclib/* $(INST_DIR)/N/icc/icclib/


jexp$(OBJSUFX): exp.c
	$(CC) $(CFLAGS) -DJGSK_WRAP exp.c $(OUT)$@

exp$(OBJSUFX): exp.c
	$(CC) $(CFLAGS) exp.c $(OUT)$@

totp$(OBJSUFX): TOTP/totp.c TOTP/totp.h
	$(CC) $(CFLAGS) TOTP/totp.c -I$(ICC_ROOT)/icc -ITOTP $(OUT)$@

runtime_setup: runtime_setup_C runtime_setup_N

sdk_setup: sdk_setup_C sdk_setup_N

clean: clean_jgsk clean_ickc
	-$(RM) icctest$(EXESUFX) pktest_direct$(EXESUFX) loadtest$(EXESUFX) chkiccload$(EXESUFX)
	-$(RM) icctest.c jcctest.c ickctest.c
	-$(RM) $(TARGETS) $(SDK_TARGETS)
	-$(RM) *.o *.obj *.so *.dylib *.dll *.sl *.x *.lib
	-$(RM) *.ilk *.manifest *.pdb
	-$(RM) $(GSK_LIB)
	-$(RM) 	$(GSK_LIB).unstripped
	-$(RM) delta.exp
	-$(RM) 	API_html/*
	-$(RM) $(INST_DIR)/N/icc/icclib/$(NEW_LIBICC)
	-$(RM) -r $(INST_DIR)/* 
	-$(RM) -r $(GSK_SDK)/*
	-$(RM) -r $(TMP_SRC) $(TMP_OBJS)
	-$(RM) exports_old/*.exp exports_old/iccstepZOS.h
	-$(MAKE) -C TOTP clean
	-$(RM) Doxyfile
	-$(MAKE) -C pqc clean


# Note: Need to rm after copy as Windows looks in the same directory as exe's
# for DLL's, but in this case the rest of the directory structure is missing

# GSK_LIBNAME (shared step library file name (e.g gsk8iccs_64.dll)), GSKLIB_B (no extension)
# and GSK_LIB ($(GSK_DIR)/$(GSK_LIBNAME)) are coming from gsk_crypto.mk.
# $(GSK_LIBNAME) is defined in gsk_crypto.mk but the build target is defined in platforms/$(OPSYS)_.mk using GSKLIB_B (e.g WIN64_.mk)

$(GSK_LIB): $(GSK_DIR) $(GSK_LIBNAME)
	$(CP) $(GSK_LIBNAME) $@
	$(RM) $(GSK_LIBNAME)
	touch keep_tar_quiet.pdb
	-$(CP) *.pdb $(GSK_SDK)
	touch  $(ICC_ROOT)/package/iccsdk/keep_tar_quiet.pdb
	-$(CP) $(ICC_ROOT)/package/iccsdk/*.pdb $(GSK_SDK)
	-touch  OLD_ICC/$(OPSYS)/iccsdk/keep_tar_quiet.pdb
	-$(CP) OLD_ICC/$(OPSYS)/iccsdk/*.pdb $(GSK_SDK)


# Separate rules for Windows and "other" platforms
# so the libs can be signed 
# and we have the 32 and 64 bit variants ... ugly, really ugly
# plus z/OS which doesn't link normally
#
# These have been moved into platforms/$(OPSYS)_.mk
#

# Platform specifc recipies for building libs
include platforms/$(OPSYS)_.mk

gsk_wrap2$(OBJSUFX): gsk_wrap2.c gsk_wrap2_a.c $(ICC_ROOT)/icc/loaded.c name_cache.c $(ICC_ROOT)/icc/name_cache_tables.c \
	$(GSK_SDK)/icc.h $(GSK_SDK)/icc_a.h $(GSK_SDK)/iccglobals.h $(ICC_ROOT)/icc/loaded.h $(ICC_ROOT)/icc/iccversion.h $(ICC_ROOT)/icc/tracer.h
	$(CC) $(CFLAGS) -DICCPKG -DMYNAME=gskiccs8 -DGSK_LIBNAME=\""$(GSK_LIBNAME)"\" -DOPSYS="\"$(OPSYS)\"" -DGSK_GLOBAL=\""$(GSK_GLOBAL)"\" -I./ -I$(GSK_SDK) -I$(ICC_ROOT)/icc gsk_wrap2.c

$(SDK_DIR)/$(ICCLIB): runtime_setup_N

OS_helpers$(OBJSUFX): OS_helpers.c
	$(CC) $(CFLAGS) OS_helpers.c

loadtest$(OBJSUFX): loadtest.c
	$(CC) $(CFLAGS) -DGSK_LIBNAME=\""$(GSK_LIBNAME)"\" -I$(GSK_SDK) loadtest.c

icc.res: $(ICC_ROOT)/icc/icc.res
	$(CP) $(ICC_ROOT)/icc/icc.res $@

cache_test$(OBJSUFX): gsk_wrap2.c gsk_wrap2_a.c $(ICC_ROOT)/icc/loaded.c name_cache.c $(ICC_ROOT)/icc/name_cache_tables.c \
	$(GSK_SDK)/icc.h $(GSK_SDK)/icc_a.h $(GSK_SDK)/iccglobals.h $(ICC_ROOT)/icc/loaded.h $(ICC_ROOT)/icc/iccversion.h $(ICC_ROOT)/icc/tracer.h 
	$(CC) $(CFLAGS) -DICCPKG -DMYNAME=gskiccs8 -DGSK_LIBNAME=\""$(GSK_LIBNAME)"\" -DOPSYS="\"$(OPSYS)\"" -DGSK_GLOBAL=\""$(GSK_GLOBAL)"\" -DSTANDALONE -I$(ICC_ROOT)/icc/DELTA -I./ -I$(GSK_SDK) -I$(ICC_ROOT)/icc gsk_wrap2.c $(OUT)$@


# GSK_LIB and ICCPKG_LIBS is coming from gsk_crypto.mk - references the step import library
# Note different from icc/icctest which links direct to the module

icctest$(OBJSUFX): $(ICC_ROOT)/icc/icctest.c $(SDK_HEADERS)
	$(CP) $(ICC_ROOT)/icc/icctest.c .
	$(CC) $(CFLAGS) -I$(GSK_SDK) -I$(ICC_ROOT)/icc -DICCPKG icctest.c 
	-$(RM) icctest.c

icctest$(EXESUFX): icctest$(OBJSUFX) $(GSK_LIB)
	$(LD) $(LDFLAGS) icctest$(OBJSUFX) $(ICCPKG_LIBS) $(LDLIBS)

icctest_s$(EXESUFX): icctest$(OBJSUFX) $(GSK_LIB_STATIC)
	$(LD) $(LDFLAGS) icctest$(OBJSUFX) $(ICCPKG_S_LIBS) $(LDLIBS)

$(GSK_SDK)/icctest$(EXESUFX): icctest$(EXESUFX)
	$(CP) icctest$(EXESUFX) $@

argon2_example$(EXESUFX): Argon2/argon2_example.c $(GSK_LIB)
	$(CC) $(CFLAGS) -I./ -I$(GSK_SDK) Argon2/argon2_example.c 
	$(LD) $(LDFLAGS) argon2_example$(OBJSUFX) $(ICCPKG_LIBS)  $(LDLIBS) 

smalltest$(EXESUFX): $(ICC_ROOT)/icc/tools/smalltest.c $(GSK_LIB)
	$(CC) $(CFLAGS) -I./ -I$(GSK_SDK) -DICCPKG $(ICC_ROOT)/icc/tools/smalltest.c
	$(LD) $(LDFLAGS) smalltest$(OBJSUFX) $(ICCPKG_LIBS) $(LDLIBS) 

memleak$(EXESUFX): memleak.c $(GSK_LIB)
	$(CC) $(CFLAGS) -I./ -I$(GSK_SDK) -DGSK_LIBNAME=\""$(GSK_LIBNAME)"\" memleak.c 
	$(LD) $(LDFLAGS) memleak$(OBJSUFX) $(ICCPKG_LIBS)  $(LDLIBS) 

smalltest1$(EXESUFX): smalltest1.c $(GSK_LIB)
	$(CC) $(CFLAGS) -I$(GSK_SDK) -DICCPKG smalltest1.c 
	$(LD) $(LDFLAGS) smalltest1$(OBJSUFX) $(ICCPKG_LIBS) $(LDLIBS) 

smalltest2$(EXESUFX): smalltest2.c $(GSK_LIB)
	$(CC) $(CFLAGS) -I./ -I$(GSK_SDK) -DICCPKG smalltest2.c 
	$(LD) $(LDFLAGS) smalltest2$(OBJSUFX) $(ICCPKG_LIBS) $(LDLIBS) 

smalltest3$(EXESUFX): smalltest3.c $(GSK_LIB)
	$(CC) $(CFLAGS) -I./ -I$(GSK_SDK) -DICCPKG smalltest3.c 
	$(LD) $(LDFLAGS) smalltest3$(OBJSUFX) $(ICCPKG_LIBS) $(LDLIBS) 

smalltest4$(EXESUFX): smalltest4.c $(GSK_LIB)
	$(CC) $(CFLAGS) -I./ -I$(GSK_SDK) -DICCPKG smalltest4.c 
	$(LD) $(LDFLAGS) smalltest4$(OBJSUFX) $(ICCPKG_LIBS)  $(LDLIBS) 

smalltest5$(EXESUFX): smalltest5.c $(GSK_LIB)
	$(CC) $(CFLAGS) -I./ -I$(GSK_SDK) -DICCPKG smalltest5.c 
	$(LD) $(LDFLAGS) smalltest5$(OBJSUFX) $(ICCPKG_LIBS) $(LDLIBS) 

smalltestW$(EXESUFX): smalltestW.c $(GSK_LIB)
	$(CC) $(CFLAGS) -I./ -I$(GSK_SDK) -DICCPKG  smalltestW.c 
	$(LD) $(LDFLAGS) smalltestW$(OBJSUFX) $(ICCPKG_LIBS) $(LDLIBS) 

dltest$(EXESUFX): dltest.c $(GSK_LIB)
	$(CC) $(CFLAGS) -I./ -I$(GSK_SDK) -DICCPKG -DGSK_LIBNAME=\""$(GSK_LIBNAME)"\" dltest.c 
	$(LD) $(LDFLAGS) dltest$(OBJSUFX) $(LDLIBS) 

icc_aux_sa$(EXESUFX): icc_aux$(OBJSUFX) icc_aux_test$(OBJSUFX) $(GSK_LIB)  
	$(LD) $(LDFLAGS) icc_aux$(OBJSUFX) icc_aux_test$(OBJSUFX) $(ICCPKG_LIBS) $(LDLIBS) 

icc_aux$(OBJSUFX): icc_aux.c 
	$(CC) $(CFLAGS) -I./ -I$(OSSL_DIR)/include/ icc_aux.c

icc_aux_test$(OBJSUFX): icc_aux_test.c
	$(CC) $(CFLAGS) -I./ -I$(OSSL_DIR)/include/ icc_aux_test.c

icc_aux_test$(EXESUFX): icc_aux_test$(OBJSUFX) $(AUXLIB)  $(GSK_LIB)
	$(LD) $(LDFLAGS) icc_aux_test$(OBJSUFX) $(AUX_LIBS) $(ICCPKG_LIBS)  $(LDLIBS) 



GenRndData2$(EXESUFX): $(ICC_ROOT)/icc/tools/GenRndData2.c $(GSK_LIB)
	$(CC) $(CFLAGS) -I./ -I$(GSK_SDK) -DICCPKG $(ICC_ROOT)/icc/tools/GenRndData2.c $(OUT)GenRndData2$(OBJSUFX)
	$(LD) $(LDFLAGS) GenRndData2$(OBJSUFX) $(ICCPKG_LIBS) $(LDLIBS) 
	-$(CP) GenRndData2$(EXESUFX) $(GSK_SDK)/

# GSK_LIB and ICCPKG_LIBS is coming from gsk_crypto.mk - references the step import library
# IS_FIPS and MUPPET comes from muppet.mk (written by ICCencapsulator in prebuild step)

chkiccload$(OBJSUFX): chkiccload.c muppet.mk
	$(CC) $(CFLAGS) -I./ -I$(GSK_SDK) -DICCPKG -DIS_FIPS=$(IS_FIPS) -DLEGACY=\""$(MUPPET)"\" chkiccload.c

chkiccload$(EXESUFX): chkiccload$(OBJSUFX) $(GSK_LIB)
	$(LD) $(LDFLAGS) chkiccload$(OBJSUFX) $(ICCPKG_LIBS) $(LDLIBS)

$(GSK_SDK)/chkiccload$(EXESUFX) : chkiccload$(EXESUFX)
	$(CP) chkiccload$(EXESUFX) $@

loadtest$(EXESUFX): loadtest$(OBJSUFX)
	$(LD) $(LDFLAGS) loadtest$(OBJSUFX) $(LDLIBS)


pktest_common$(OBJSUFX): $(ICC_ROOT)/pk11/pktest_common.c
	$(CC) $(CFLAGS) -DPKTEST_STATIC -I$(ICC_ROOT)/pkcs11/v2.20/rsa/inc \
		 -I$(ICC_ROOT)/pk11 $(ICC_ROOT)/pk11/pktest_common.c 

pktest$(OBJSUFX): $(ICC_ROOT)/pk11/pktest.c
	$(CC) $(CFLAGS) -I$(ICC_ROOT)/pkcs11/v2.20/rsa/inc -I$(ICC_ROOT)/pk11 -I$(ICC_ROOT)/pk11/tests -I$(GSK_SDK) \
		$(ICC_ROOT)/pk11/pktest.c

$(GSK_SDK)/pktest_direct$(EXESUFX): $(GSK_SDK) pktest_direct$(EXESUFX)
	$(CP) pktest_direct$(EXESUFX) $@

pktest_direct$(EXESUFX): pktest$(OBJSUFX) pktest_common$(OBJSUFX) \
		$(ICC_ROOT)/pk11/pk11_errs$(OBJSUFX) $(ICC_ROOT)/pk11/utils$(OBJSUFX) $(GSK_LIB)
	$(LD) $(LDFLAGS) pktest$(OBJSUFX) pktest_common$(OBJSUFX) \
		$(ICC_ROOT)/pk11/pk11_errs$(OBJSUFX) $(ICC_ROOT)/pk11/utils$(OBJSUFX) \
		$(LDLIBS) $(ICCPKG_LIBS)

# Build the performance test code for ICCPKG
$(ICCPKG_PERF): $(GSK_LIB)
	$(SETUP_ICCSPEED) $(MAKE) -C $(ICC_ROOT)/iccspeed OPSYS=$(OPSYS) CONFIG=$(CONFIG) BUILD=$(BUILD) XPLINK=$(XPLINK) gsk
	$(CP) $(ICC_ROOT)/iccspeed/bin/$(OPSYS)/icc_perf$(EXESUFX) $@

$(JICC_PERF): $(JGSK_LIB)
	$(SETUP_ICCSPEED) $(MAKE) -C $(ICC_ROOT)/iccspeed OPSYS=$(OPSYS) CONFIG=$(CONFIG) BUILD=$(BUILD) XPLINK=$(XPLINK) jgsk;
	$(CP) $(ICC_ROOT)/iccspeed/bin/$(OPSYS)/jicc_perf$(EXESUFX) $@

$(ICC_ROOT)/pk11/keystoretool$(EXESUFX):
	$(MAKE) -C $(ICC_ROOT)/pk11 keystoretool$(EXESUFX)

$(GSK_SDK)/keystoretool$(EXESUFX): $(ICC_ROOT)/pk11/keystoretool$(EXESUFX)
	$(CP) $(ICC_ROOT)/pk11/keystoretool$(EXESUFX) $@

$(PK11_PERF): $(GSK_LIB)
	$(SETUP_ICCSPEED) $(MAKE) -C $(ICC_ROOT)/iccspeed OPSYS=$(OPSYS) CONFIG=$(CONFIG) BUILD=$(BUILD) XPLINK=$(XPLINK) pkcs11
	$(CP) $(ICC_ROOT)/iccspeed/bin/$(OPSYS)/pkcs11_thread$(EXESUFX) $@

$(GSK_RNG): $(GSK_LIB) GenRndData2$(EXESUFX)
	$(CP) GenRndData2$(EXESUFX) $@

$(ICC_RNG): $(ICC_ROOT)/package/iccsdk/GenRndData$(EXESUFX) 
	$(CP) $(ICC_ROOT)/package/iccsdk/GenRndData$(EXESUFX) $@


$(GSK_OPENSSL): $(ICC_ROOT)/package/iccsdk/openssl$(EXESUFX)
	$(CP) $(ICC_ROOT)/package/iccsdk/openssl$(EXESUFX) $@

#
# -------------- JCEPlus variants -------------
#
# Namespaced and with a different shared library name to avoid direct conflicts 
# between Java installed libraries and the C/C++ stack.
#
# To date problems were only seen on AIX, but lets use the big hammer
# and keep it like that.
#

#$(JGSK_DIR)/Standard_International_Program_License_Agreement.pdf: $(JGSK_DIR) $(ICC_ROOT)/icc/Standard_International_Program_License_Agreement.pdf
#	$(CP) $(ICC_ROOT)/icc/Standard_International_Program_License_Agreement.pdf $@
#	chmod +w $@
#
#$(JGSK_SDK)/Standard_International_Program_License_Agreement.pdf: $(JGSK_DIR) $(ICC_ROOT)/icc/Standard_International_Program_License_Agreement.pdf
#	$(CP) $(ICC_ROOT)/icc/Standard_International_Program_License_Agreement.pdf $@
#	chmod +w $@

Jruntime_setup_C: $(JINST_DIR)/C/icc/icclib/$(OLD_LIBICC) $(JINST_DIR)/C/icc/ReadMe.txt
	if [ -f $(ICC_ROOT)/icc/create_icc_no_legacy.0 ]; then echo "No FIPS ICC build" >> $(JINST_DIR)/C/icc/ReadMe.txt; fi ;
	if [ -f $(ICC_ROOT)/icc/create_icc_no_legacy.0 ]; then $(RM) $(JINST_DIR)/C/icc/icclib/*; fi ;

$(JINST_DIR)/C/icc/icclib: $(JINST_DIR)
	$(MKDIR) $@

$(JINST_DIR)/N/icc/icclib: $(JINST_DIR)
	$(MKDIR) $@

$(JINST_DIR)/C/icc/ReadMe.txt: $(JINST_DIR)/C/icc
	-$(CP) OLD_ICC/AMD64_LINUX/icc/ReadMe.txt $@

# OLD_ICC has checked in binaries so enforce correct permissions
$(JINST_DIR)/C/icc/icclib/$(OLD_LIBICC): $(JINST_DIR)/C/icc/icclib
	-$(CP) $(OLD_ICC)/icc/icclib/* $(JINST_DIR)/C/icc/icclib/
	-chmod +x $@


Jruntime_setup_N: $(JINST_DIR)/N/icc/icclib/$(NEW_LIBICC) $(JINST_DIR)/N/icc/ReadMe.txt

$(JINST_DIR)/N/icc/ReadMe.txt: $(JINST_DIR) $(RTE_DIR)/ReadMe.txt
	-$(CP) $(RTE_DIR)/ReadMe.txt $@

$(JINST_DIR)/N/icc/icclib/$(NEW_LIBICC): $(JINST_DIR)/N/icc/icclib
	-$(CP) $(RTE_DIR)/icclib/*  $(JINST_DIR)/N/icc/icclib/


Jruntime_setup: Jruntime_setup_C Jruntime_setup_N

# $(JGSK_LIBNAME) is defined in gsk_crypto.mk but the build target is defined in platforms/$(OPSYS)_.mk using GSKLIB_B (e.g WIN64_.mk)

$(JGSK_LIB): $(JGSK_DIR) $(JGSK_LIBNAME)
	$(CP) $(JGSK_LIBNAME) $@
	$(RM) $(JGSK_LIBNAME)


jgsk_wrap2$(OBJSUFX): gsk_wrap2.c gsk_wrap2_a.c $(ICC_ROOT)/icc/loaded.c name_cache.c $(ICC_ROOT)/icc/name_cache_tables.c \
	$(JGSK_SDK)/inc/icc.h $(JGSK_SDK)/inc/icc_a.h $(JGSK_SDK)/inc/iccglobals.h $(ICC_ROOT)/icc/loaded.h $(ICC_ROOT)/icc/iccversion.h $(ICC_ROOT)/icc/tracer.h $(JGSK_SDK)/inc/jcc_a.h
	$(CC) $(CFLAGS) -DICCPKG -DJGSK_WRAP -DMYNAME=jgskiccs8 -DGSK_LIBNAME=\""$(JGSK_LIBNAME)"\" -DOPSYS="\"$(OPSYS)\"" -DGSK_GLOBAL=\""$(GSK_GLOBAL)"\" -I$(JGSK_SDK)/inc/ -I$(ICC_ROOT)/icc/ gsk_wrap2.c $(OUT)$@

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

jcctest$(OBJSUFX): $(ICC_ROOT)/icc/icctest.c $(SDK_HEADERS)
	$(CP) $(ICC_ROOT)/icc/icctest.c jcctest.c
	$(CC) $(CFLAGS) -I./ -I$(GSK_SDK) -I$(ICC_ROOT)/icc -DICCPKG -DJGSK_WRAP jcctest.c $(OUT)$@
	-$(RM) jcctest.c

jcctest$(EXESUFX): jcctest$(OBJSUFX) $(JGSK_LIB)
	$(LD) $(LDFLAGS) jcctest$(OBJSUFX) $(JCCPKG_LIBS) $(LDLIBS) 

$(JGSK_SDK)/bin/jcctest$(EXESUFX): $(JGSK_SDK)/bin jcctest$(EXESUFX)
	-$(CP) jcctest$(EXESUFX) $@

$(JGSK_SDK)/docs/IBM_CKC.pdf: $(JGSK_SDK)/docs $(ICC_ROOT)/doc/IBM_CKC.pdf
	$(CP) $(ICC_ROOT)/doc/IBM_CKC.pdf $@

$(JGSK_SDK)/inc/icc.h: $(JGSK_SDK)/inc $(ICC_ROOT)/icc/icc.h
	$(CP) $(ICC_ROOT)/icc/icc.h $@

$(JGSK_SDK)/inc/iccglobals.h: $(JGSK_SDK)/inc $(ICC_ROOT)/icc/iccglobals.h
	$(CP) $(ICC_ROOT)/icc/iccglobals.h $@

$(JGSK_SDK)/inc/icc_a.h: $(JGSK_SDK)/inc $(ICC_ROOT)/iccpkg/iccpkg_a.h
	$(CP) $(ICC_ROOT)/iccpkg/iccpkg_a.h $@

$(JGSK_SDK)/inc/jcc_a.h: $(JGSK_SDK)/inc jcc_a.h
	$(CP) jcc_a.h $@

$(JGSK_SDK)/inc/gsk_exp.h: $(JGSK_SDK)/inc gsk_exp.h
	$(CP) gsk_exp.h $@

$(JGSK_SDK)/inc/pqapi.h: $(JGSK_SDK)/inc DilKyb/headers/pqapi.h
	$(CP) DilKyb/headers/pqapi.h $@

$(JGSK_SDK)/inc/pqparams.h: $(JGSK_SDK)/inc DilKyb/headers/pqparams.h
	$(CP) DilKyb/headers/pqparams.h $@

$(JGSK_SDK)/docs/PQCgenKAT_kem.c: $(JGSK_SDK)/docs DilKyb/kyber/PQCgenKAT_kem.c
	$(CP) DilKyb/kyber/PQCgenKAT_kem.c $@

$(JGSK_SDK)/docs/PQCgenKAT_sign.c: $(JGSK_SDK)/docs DilKyb/dilithium/PQCgenKAT_sign.c
	$(CP) DilKyb/dilithium/PQCgenKAT_sign.c $@

#
# ICKC_ Namespaced
#

$(ICKC_DIR)/Standard_International_Program_License_Agreement.pdf: $(ICKC_DIR) $(ICC_ROOT)/icc/Standard_International_Program_License_Agreement.pdf
	$(CP) $(ICC_ROOT)/icc/Standard_International_Program_License_Agreement.pdf $@
	chmod +w $@

$(ICKC_SDK)/Standard_International_Program_License_Agreement.pdf: $(ICKC_DIR) $(ICC_ROOT)/icc/Standard_International_Program_License_Agreement.pdf
	$(CP) $(ICC_ROOT)/icc/Standard_International_Program_License_Agreement.pdf $@
	chmod +w $@

# $(ICKC_LIBNAME) is defined in gsk_crypto.mk but the build target is defined in platforms/$(OPSYS)_.mk using GSKLIB_B (e.g WIN64_.mk)

$(ICKC_LIB): $(ICKC_DIR) $(ICKC_LIBNAME)
	$(CP) $(ICKC_LIBNAME) $@

ickc_wrap2$(OBJSUFX): gsk_wrap2.c gsk_wrap2_a.c $(ICC_ROOT)/icc/loaded.c name_cache.c $(ICC_ROOT)/icc/name_cache_tables.c \
	$(ICKC_SDK)/inc/icc.h $(ICKC_SDK)/inc/icc_a.h $(ICKC_SDK)/inc/iccglobals.h $(ICC_ROOT)/icc/loaded.h $(ICC_ROOT)/icc/iccversion.h $(ICC_ROOT)/icc/tracer.h $(ICKC_SDK)/inc/ickc_a.h
	$(CC) $(CFLAGS) -DICCPKG -DICKC_WRAP -DMYNAME=ickc -DGSK_LIBNAME=\""$(ICKC_LIBNAME)"\" -DOPSYS="\"$(OPSYS)\"" -DGSK_GLOBAL=\""$(GSK_GLOBAL)"\" -I$(ICKC_SDK)/inc/ -I$(ICC_ROOT)/icc/ gsk_wrap2.c $(OUT)$@

ickctest$(OBJSUFX): $(ICC_ROOT)/icc/icctest.c $(SDK_HEADERS)
	$(CP) $(ICC_ROOT)/icc/icctest.c ickctest.c
	$(CC) $(CFLAGS) -I./ -I$(ICKC_SDK)/inc -I$(ICC_ROOT)/icc -DICCPKG -DICKC_WRAP ickctest.c $(OUT)$@
	-$(RM) ickctest.c

ickctest$(EXESUFX): ickctest$(OBJSUFX) $(ICKC_LIB)
	$(LD) $(LDFLAGS) ickctest$(OBJSUFX) $(ICKCPKG_LIBS) $(LDLIBS)

$(ICKC_SDK)/bin/ickctest$(EXESUFX): $(ICKC_SDK)/bin ickctest$(EXESUFX)
	-$(CP) ickctest$(EXESUFX) $@

$(ICKC_SDK)/inc/icc.h: $(ICKC_SDK)/inc $(ICC_ROOT)/icc/icc.h
	$(CP) $(ICC_ROOT)/icc/icc.h $@

$(ICKC_SDK)/inc/iccglobals.h: $(ICKC_SDK)/inc $(ICC_ROOT)/icc/iccglobals.h
	$(CP) $(ICC_ROOT)/icc/iccglobals.h $@

$(ICKC_SDK)/inc/icc_a.h: $(ICKC_SDK)/inc $(ICC_ROOT)/iccpkg/iccpkg_a.h
	$(CP) $(ICC_ROOT)/iccpkg/iccpkg_a.h $@

# just convert the JCC namespacer to ICKC
$(ICKC_SDK)/inc/ickc_a.h: $(ICKC_SDK)/inc jcc_a.h
	sed 's/JCC_/ICKC_/g' jcc_a.h > $@

clean_ickc:
	-$(RM) $(ICKC_DIR)/N/icc/icclib/*
	-$(RM) $(ICKC_DIR)/C/icc/icclib/*
	-$(RM) $(ICKC_SDK)/inc/*.h
	-$(RM) $(ICKC_SDK)/debug/*
	-$(RM) $(ICKC_SDK)/docs/*
	-$(RM) $(ICKC_SDK)/bin/jcctest$(EXESUFX)
	-$(RM) $(ICKC_LIBNAME) jcctest$(EXESUFX)
	-$(RM) -r $(ICKC_LIB)


# Tests. BVT
# GSK_SETUP comes from gsk_crypto.mk and sets PATH for these tests
# ignored errors will be reported in the log
# This is called from icc/Makefile:tests
PATH:=..\package\gskit_crypto;$(PATH)

# make sure we show the log even if the test fails
log_init:
	touch GSKIT_CRYPTO.log; $(RM) GSKIT_CRYPTO.log; touch GSKIT_CRYPTO.log
log_cat:
	cat GSKIT_CRYPTO.log
	$(RM) GSKIT_CRYPTO.log
log_rm:
	-$(RM) GSKIT_CRYPTO.log

tests: log_init chkiccload_test log_cat log_init icctest_test log_cat pqc_tests
	echo tests complete

chkiccload_test:
	$(GSK_SETUP); ./chkiccload

icctest_test:
	$(GSK_SETUP); ./icctest

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
	echo "SDK_TARGETS			= $(SDK_TARGETS)"
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
	echo "OLD_LIBICC			= $(OLD_LIBICC)"
	echo "NEW_LIBICC			= $(NEW_LIBICC)"
	echo "ICCPKG_LIBS       = $(ICCPKG_LIBS)"
	echo
	echo "Set in icc_defs.mk"
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
	echo "GSK_LIB_STATIC    = $(GSK_LIB_STATIC)"
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

