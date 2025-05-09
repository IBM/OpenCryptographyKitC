include platforms/${OPENSSL_LIBVER}/UNIX_like.mk


ASM_OBJS = s390xcpuid.o bn_asm.o s390xcap.o s390x-mont.o s390x-gf2m.o \
	aes-s390x.o \
	camellia.o cmll_misc.o cmll_cbc.o \
	sha1-s390x.o sha256-s390x.o sha512-s390x.o \
	rc4-s390x.o \
	c_enc.o bf_enc.o des_enc.o ghash-s390x.o \
	threads_pthread.o async_posix.o \
	chacha-s390x.o  poly1305-s390x.o \
	keccak1600-s390x.o ecp_s390x_nistp.o \
	hmac_s390x.o 

# poly1305-s390x.o

BUILD_OBJS = $(BASE_OBJS) $(ASM_OBJS)

tmp/dummyfile: $(SLIBCRYPTO)
	$(MKDIR) tmp
	( cd tmp; \
	  ar x ../$(SLIBCRYPTO); \
	  cd .. ; \
	)
	touch $@
