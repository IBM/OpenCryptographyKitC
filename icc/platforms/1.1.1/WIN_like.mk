include platforms/${OPENSSL_LIBVER}/BASE_OSSL_FILES.mk

#TWEAKS="ICC_TRNG=TRNG_ALT4"

$(ICC_RAND_OBJ): $(OSSLINC_DIR) icc_rand.c 
	$(CC) $(CFLAGS) -I./ -I$(ZLIB_DIR) -I$(TRNG_DIR) -I$(OSSLINC_DIR) -I$(OSSL_DIR) -I$(SDK_DIR) icc_rand.c $(ASM_TWEAKS)


opensslrc.RES: opensslrc.rc
	rc -DVTAG=$(VTAG) opensslrc.rc

icc.res: icc.rc
	rc -DICC_OFFICIAL_BUILD icc.rc

$(MYOPENSSL): openssl.exe
	$(CP) openssl.exe $@

../$(OPENSSL_VER)/apps/openssl$(OBJSUFX): ../$(OPENSSL_VER)/apps/openssl.c
	$(BUILD_OSSL)

openssl$(OBJSUFX): $(OSSL_DIR)/apps/openssl$(OBJSUFX)
	cp $(OSSL_DIR)/apps/openssl$(OBJSUFX) $@

openssl.exe: openssl$(OBJSUFX) Build_OSSL_Complete $(E_OBJ) $(SLIBCRYPTO_OBJS) $(SLIBSSL_OBJS) platform$(OBJSUFX)
	$(LD) $(LDFLAGS) openssl$(OBJSUFX) platform$(OBJSUFX) $(E_OBJ) $(SLIBCRYPTO_OBJS) $(SLIBSSL_OBJS) $(OPENSSL_LIBS) $(LDLIBS) ws2_32.lib

#
# default make stuff used on most platforms
# Those that aren't cross compiled typically.
#

icc.sig: $(MYOPENSSL) $(ICCDLL) $(FILESIZE)
	./$(FILESIZE) $(ICCDLL) >icc.sig
	$(MYOPENSSL) dgst -sha256 -hex   $(ICCDLL) >> icc.sig
	$(MYOPENSSL) dgst -sha256 -hex -sign privkey.rsa $(ICCDLL) >> icc.sig

ossl.sig: $(MYOPENSSL) $(OSSLDLL) $(FILESIZE)
	./$(FILESIZE) $(OSSLDLL) >ossl.sig
	$(MYOPENSSL) dgst -sha256 -hex   $(OSSLDLL) >> ossl.sig
	$(MYOPENSSL) dgst -sha256 -hex -sign privkey.rsa $(OSSLDLL) >> ossl.sig

icchash.h: icc.sig ossl.sig icchash$(EXESUFX)
	./icchash icc.sig ossl.sig $@



$(OSSLDLL): Build_OSSL_Complete $(MY_OSSLDLL_NAME) icc.res

#
# Moved to WIN_like.mk because this is where we perform MS authnticode
# signing if the signing script is available
#
#- Copy OpenSSL crypto library to package directory
#
# RELOC_ASM_OBJS are objects which windows builds but don't get copied to
# tmp32dll
#
tmp/dummyfile: icc.res
	-$(MKDIR) tmp
	-$(CP) $(OSSL_DIR)/tmp32dll/*.obj tmp/
	-$(CP) icc.res tmp/
	-$(CP) icc.res ../iccpkg/
	touch tmp/dummyfile

