export CXPLINK=,xplink 

include platforms/${OPENSSL_LIBVER}/UNIX_like.mk

# Equivalent of the assembler modules for code built without asm.
# Used during debug and it takes a long time to get to a complete list
#
#ASM_OBJS = s390xcpuid.o s390x.o s390x-mont.o bn_nist.o ecp_nist.o  s390xcap.o s390x-gf2m.o 
#	aes-s390x.o \
#	camellia.o cmll_misc.o cmll_cbc.o \
#	sha1-s390x.o sha256-s390x.o sha512-s390x.o \
#	rc4-s390x.o \
#	c_enc.o bf_enc.o des_enc.o ghash-s390x.o \
#	threads_pthread.o async_posix.o \
#	keccak1600-s390x.o ecp_s390x_nistp.o \
#	ebcdic.o async_null.o  \
#	chacha-s390x.o poly1305-s390x.o \
#	hmac_s390x.o \

ASM_OBJS = s390xcpuid.o s390x.o s390x-mont.o bn_nist.o ecp_nist.o  s390xcap.o s390x-gf2m.o \
	aes-s390x.o \
	camellia.o cmll_misc.o cmll_cbc.o \
	sha1-s390x.o sha256-s390x.o sha512-s390x.o \
	rc4-s390x.o \
	c_enc.o bf_enc.o des_enc.o ghash-s390x.o \
	threads_pthread.o async_posix.o \
	keccak1600-s390x.o ecp_s390x_nistp.o \
	ebcdic.o async_null.o  \
	chacha-s390x.o poly1305-s390x.o \
	hmac_s390x.o \

csvquery_64.o: asm/zos/csvquery_64.s
	-as -aegimrsx=$^.list -m"GOFF,SYSPARM(USE_XPLINK),ESD" $^

dll_init.o: dll_init.cpp
	xlc++ -c -g -W"c,XPLINK,LP64,WARN64" -o $@ $^ 

BUILD_OBJS = $(BASE_OBJS) $(ASM_OBJS) csvquery_64.o dll_init.o

tmp/dummyfile: $(SLIBCRYPTO) dll_init.o csvquery_64.o
	$(MKDIR) tmp
	( cd tmp; \
	  ar x ../$(SLIBCRYPTO); \
	  cd .. ; \
	)
	$(CP) csvquery_64.o tmp/
	$(CP) dll_init.o tmp/
	touch $@
