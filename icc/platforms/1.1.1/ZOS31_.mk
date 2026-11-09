export CXPLINK=,xplink 

include platforms/${OPENSSL_LIBVER}/UNIX_like.mk


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

csvquery.o: asm/zos/csvquery.s
	-as -aegimrsx=$^.list -m"GOFF,SYSPARM(USE_XPLINK),ESD" $^

dll_init.o: dll_init.cpp
	xlc++ -c -g -W"c,XPLINK" -o $@ $^ 

BUILD_OBJS = $(BASE_OBJS) $(ASM_OBJS) csvquery.o dll_init.o

tmp/dummyfile: $(SLIBCRYPTO) dll_init.o csvquery.o
	$(MKDIR) tmp
	( cd tmp; \
	  ar x ../$(SLIBCRYPTO); \
	  cd .. ; \
	)
	$(CP) csvquery.o tmp/
	$(CP) dll_init.o tmp/
	touch $@
