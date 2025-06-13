include platforms/${OPENSSL_LIBVER}/UNIX_like.mk

#
# default make stuff used on most platforms
# Those that aren't cross compiled typically.
#

icc.sig: $(MYOPENSSL) $(ICCDLL) $(FILESIZE)
	- install_name_tool -delete_rpath ./   $(ICCDLL)
	- install_name_tool -delete_rpath @loader_path/.   $(ICCDLL)
	./$(FILESIZE) $(ICCDLL) >icc.sig
	$(MYOPENSSL) dgst -sha256 -hex   $(ICCDLL) >> icc.sig
	$(MYOPENSSL) dgst -sha256 -hex -sign privkey.rsa $(ICCDLL) >> icc.sig

ossl.sig: $(MYOPENSSL) $(OSSLDLL) $(FILESIZE)
	- install_name_tool -delete_rpath ./   $(OSSLDLL)
	- install_name_tool -delete_rpath @loader_path/.   $(OSSLDLL)
	./$(FILESIZE) $(OSSLDLL) >ossl.sig
	$(MYOPENSSL) dgst -sha256 -hex   $(OSSLDLL) >> ossl.sig
	$(MYOPENSSL) dgst -sha256 -hex -sign privkey.rsa $(OSSLDLL) >> ossl.sig


icchash.h: icc.sig ossl.sig icchash$(EXESUFX)
	./icchash icc.sig ossl.sig $@

#
# Moved to platform specific because at least on HP/UX 
# we need to unarchive libcrypto.a and relink it as a
# new shared library to get the internal library name correct
# This is the default (old) 
#
#- Copy OpenSSL crypto library to package directory
$(OSSLDLL): $(RTE_DIR)/osslib $(OSSLOBJ_DIR)/$(OSSLDLL_NAME)
	$(CP) $(OSSLOBJ_DIR)/$(OSSLDLL_NAME) $@
	$(STRIP) $@

ASM_OBJS = \
armcap.o \
arm64cpuid.o \
bf_enc.o \
c_enc.o \
des_enc.o \
camellia.o \
cmll_misc.o \
cmll_cbc.o \
chacha-armv8.o \
ecp_nistz256.o \
ecp_nistz256-armv8.o \
rc4_enc.o \
rc4_skey.o \
ghashv8-armx.o \
sha1-armv8.o \
keccak1600-armv8.o \
sha256-armv8.o \
sha512-armv8.o \
aesv8-armx.o \
vpaes-armv8.o \
poly1305-armv8.o \
async_posix.o \
bn_asm.o \
armv8-mont.o \
threads_pthread.o


BUILD_OBJS = $(BASE_OBJS) $(ASM_OBJS)

$(SLIBCRYPTO): Build_OSSL_Complete

tmp/dummyfile: $(SLIBCRYPTO)
	$(MKDIR) tmp
	( cd tmp; \
	  ar -x ../$(SLIBCRYPTO); \
	  cd .. ; \
	)
	touch $@

