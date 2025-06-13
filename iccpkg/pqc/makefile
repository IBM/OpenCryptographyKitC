#OQS_FLAGS = -DOQS_MINIMAL_BUILD="KEM_kyber_768;SIG_dilithium_3;SIG_sphincs_shake_256s_simple" -DOQS_BUILD_ONLY_LIB=ON
#DEFAULT_BUILD_OQS      = cd $(ICC_ROOT)/liboqs; cmake -G "Unix Makefiles" $(OQS_FLAGS) . ; make

ICC_ROOT=../..

include $(ICC_ROOT)/iccpkg/muppet.mk
include $(ICC_ROOT)/icc/platforms.mk
include $(ICC_ROOT)/icc/icc_defs.mk
include $(ICC_ROOT)/iccpkg/gsk_crypto.mk

#-I common/aes -I common/rand -I common/sha2 -I common/sha3
DilKyb_CFLAGS_$(OPSYS) = -I $(GSK_SDK) -I . $(PQCINC) $(CFLAGS)

DilKyb_CFLAGS_SUN_X86 := -std=c99 $(DilKyb_CFLAGS_$(OPSYS))
DilKyb_CFLAGS_SUN_AMD64 := -xc99=all $(DilKyb_CFLAGS_$(OPSYS))

DilKyb_CFLAGS=$(DilKyb_CFLAGS_$(OPSYS))

OBJD=o

# LDLIBS is from platforms.mk
SYSLIBS=$(LDLIBS)

all : $(OBJD) kemtest$(EXESUFX) sigtest$(EXESUFX)

t_kemtest:
	$(GSK_SETUP) ; ./kemtest
t_kemtest_v:
	$(GSK_SETUP) ; ./kemtest -v
t_k1024:
ifeq ($(strip $(IS_FIPS)),)
	$(GSK_SETUP) ; ./kemtest -alg "Kyber_1024"
else
	echo Kyber_1024 not tested on FIPS
endif
t_k768:
	$(GSK_SETUP) ; ./kemtest -alg "MLKEM768"
t_ktcb:
	$(GSK_SETUP) ; ./kemtest -tcb

t_sigtest_v:
	$(GSK_SETUP) ; ./sigtest -v
t_srsa:
	$(GSK_SETUP) ; ./sigtest -alg "rsaEncryption"
t_srsa_tcb:
	$(GSK_SETUP) ; ./sigtest -tcb -alg "rsaEncryption"
# FIPS 8.6 may show an error here
t_srsa_fips:
	$(GSK_SETUP) ; ./sigtest -fips -fcb -alg "rsaEncryption"
t_sdsa44:
	$(GSK_SETUP) ; ./sigtest -alg "ML_DSA_44" -l 8000
t_sdsa65:
ifeq ($(strip $(IS_FIPS)),)
	$(GSK_SETUP) ; ./sigtest -alg "ML_DSA_65" -h "SHA256"
	$(GSK_SETUP) ; ./sigtest -alg "MLDSA65" -l 80000
else
	echo ML_DSA_65 not tested on FIPS
endif
t_sshake128s:
	$(GSK_SETUP) ; ./sigtest -alg "SLH_DSA_SHAKE_128s" -l 10000

testkem: kemtest$(EXESUFX) t_kemtest t_kemtest_v t_k1024 t_k768 t_ktcb
	$(GSK_SETUP) ; ./kemtest -?
testsig: sigtest$(EXESUFX) t_sigtest_v t_srsa t_srsa_tcb t_srsa_fips t_sdsa44 t_sdsa65 t_sshake128s
	$(GSK_SETUP) ; ./sigtest -?

tests: testkem testsig
	@echo PQC tests complete   

tests_all_kyber: kemtest$(EXESUFX)
	$(GSK_SETUP) ; ./kemtest -alg "MLKEM512"
	$(GSK_SETUP) ; ./kemtest -alg "MLKEM768"
	$(GSK_SETUP) ; ./kemtest -alg "MLKEM1024"

tests_all_dilithium: sigtest$(EXESUFX)
	$(GSK_SETUP) ; ./sigtest -alg "ML_DSA_44"
	$(GSK_SETUP) ; ./sigtest -alg "ML_DSA_65"
	$(GSK_SETUP) ; ./sigtest -alg "ML_DSA_87"

tests_all_sphincs: sigtest$(EXESUFX)
	$(GSK_SETUP) ; ./sigtest -alg "SLH_DSA_SHA2_128s"
	$(GSK_SETUP) ; ./sigtest -alg "SLH_DSA_SHA2_128f"
	$(GSK_SETUP) ; ./sigtest -alg "SLH_DSA_SHA2_192s"
	$(GSK_SETUP) ; ./sigtest -alg "SLH_DSA_SHA2_192f"
	$(GSK_SETUP) ; ./sigtest -alg "SLH_DSA_SHA2_256s"
	$(GSK_SETUP) ; ./sigtest -alg "SLH_DSA_SHA2_256f"
	$(GSK_SETUP) ; ./sigtest -alg "SLH_DSA_SHAKE_128s"
	$(GSK_SETUP) ; ./sigtest -alg "SLH_DSA_SHAKE_128f"
	$(GSK_SETUP) ; ./sigtest -alg "SLH_DSA_SHAKE_192s"
	$(GSK_SETUP) ; ./sigtest -alg "SLH_DSA_SHAKE_192f"
	$(GSK_SETUP) ; ./sigtest -alg "SLH_DSA_SHAKE_256s"
	$(GSK_SETUP) ; ./sigtest -alg "SLH_DSA_SHAKE_256f"

$(OBJD):
	mkdir $@

kemtest$(EXESUFX): $(OBJD)/kemtest$(OBJSUFX)
	$(LD) $(LDFLAGS) $^ $(ICCPKG_LIBS) $(SYSLIBS)

sigtest$(EXESUFX): $(OBJD)/sigtest$(OBJSUFX)
	$(LD) $(LDFLAGS) $^ $(ICCPKG_LIBS) $(SYSLIBS)

$(OBJD)/kemtest$(OBJSUFX): kemtest.c makefile $(OBJD)
	$(CC) $(DilKyb_CFLAGS) $< $(OUT)$@

$(OBJD)/sigtest$(OBJSUFX): sigtest.c makefile $(OBJD)
	$(CC) $(DilKyb_CFLAGS) $< $(OUT)$@

clean:
	-$(RM) -rf $(OBJD)
	-$(RM) kemtest$(EXESUFX) sigtest$(EXESUFX) *.ilk *.pdb

