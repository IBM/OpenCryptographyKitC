##
## * Copyright IBM Corp. 2023
## *
## * Licensed under the Apache License 2.0 (the "License").  You may not use
## * this file except in compliance with the License.  You can obtain a copy
## * in the file LICENSE in the source distribution.
## 

ZLIB_VER   = 1.2.13

ZLIB	   = zlib-$(ZLIB_VER)

# EX_SUFFIX (=_ex) is defined to build from source already extracted from tar file and
# checked into source control. This option is used by iSeries/OS400 for Clearcase builds.
ZLIB_DIR   = $(ICC_ROOT)/$(ZLIB)$(EX_SUFFIX)

ZLIB_SRC = $(ZLIB_DIR)/adler32.c  $(ZLIB_DIR)/compress.c $(ZLIB_DIR)/crc32.c \
	$(ZLIB_DIR)/deflate.c $(ZLIB_DIR)/inffast.c $(ZLIB_DIR)/inflate.c \
	$(ZLIB_DIR)/infback.c $(ZLIB_DIR)/inftrees.c $(ZLIB_DIR)/trees.c \
	$(ZLIB_DIR)/uncompr.c $(ZLIB_DIR)/zutil.c

ZLIB_OBJ = adler32$(OBJSUFX)  compress$(OBJSUFX) crc32$(OBJSUFX) \
	deflate$(OBJSUFX) inffast$(OBJSUFX) inflate$(OBJSUFX) \
	infback$(OBJSUFX) inftrees$(OBJSUFX) trees$(OBJSUFX) \
	uncompr$(OBJSUFX) zutil$(OBJSUFX)

make_zlib: $(STLPRFX)zlib$(STLSUFX) 

$(STLPRFX)zlib$(STLSUFX): $(ZLIB_OBJ)
	$(AR) $(ARFLAGS) $(ZLIB_OBJ)

#- Build zlib objects. Slow and ugly since it's more portable
adler32$(OBJSUFX): $(ZLIB_DIR)/adler32.c $(ZLIB_DIR)/zlib.h
	$(CC) $(CFLAGS) -I$(ZLIB_DIR)  $(OUT)$@ $(ZLIB_DIR)/adler32.c

compress$(OBJSUFX): $(ZLIB_DIR)/compress.c $(ZLIB_DIR)/zlib.h
	$(CC) $(CFLAGS) -I$(ZLIB_DIR)  $(OUT)$@ $(ZLIB_DIR)/compress.c

crc32$(OBJSUFX): $(ZLIB_DIR)/crc32.c $(ZLIB_DIR)/zlib.h
	$(CC) $(CFLAGS) -I$(ZLIB_DIR)  $(OUT)$@ $(ZLIB_DIR)/crc32.c

deflate$(OBJSUFX): $(ZLIB_DIR)/deflate.c $(ZLIB_DIR)/zlib.h
	$(CC) $(CFLAGS) -I$(ZLIB_DIR)  $(OUT)$@ $(ZLIB_DIR)/deflate.c

inffast$(OBJSUFX): $(ZLIB_DIR)/inffast.c $(ZLIB_DIR)/zlib.h
	$(CC) $(CFLAGS) -I$(ZLIB_DIR)  $(OUT)$@ $(ZLIB_DIR)/inffast.c

inflate$(OBJSUFX): $(ZLIB_DIR)/inflate.c $(ZLIB_DIR)/zlib.h
	$(CC) $(CFLAGS) -I$(ZLIB_DIR)  $(OUT)$@ $(ZLIB_DIR)/inflate.c

infback$(OBJSUFX): $(ZLIB_DIR)/infback.c $(ZLIB_DIR)/zlib.h
	$(CC) $(CFLAGS) -I$(ZLIB_DIR)  $(OUT)$@ $(ZLIB_DIR)/infback.c

inftrees$(OBJSUFX): $(ZLIB_DIR)/inftrees.c $(ZLIB_DIR)/zlib.h
	$(CC) $(CFLAGS) -I$(ZLIB_DIR)  $(OUT)$@ $(ZLIB_DIR)/inftrees.c

trees$(OBJSUFX): $(ZLIB_DIR)/trees.c $(ZLIB_DIR)/zlib.h
	$(CC) $(CFLAGS) -I$(ZLIB_DIR)  $(OUT)$@ $(ZLIB_DIR)/trees.c

uncompr$(OBJSUFX): $(ZLIB_DIR)/uncompr.c $(ZLIB_DIR)/zlib.h
	$(CC) $(CFLAGS) -I$(ZLIB_DIR)  $(OUT)$@ $(ZLIB_DIR)/uncompr.c

zutil$(OBJSUFX): $(ZLIB_DIR)/zutil.c $(ZLIB_DIR)/zlib.h
	$(CC) $(CFLAGS) -I$(ZLIB_DIR)  $(OUT)$@ $(ZLIB_DIR)/zutil.c

#- Create the zlib sources from tarfile and patches
#- This has NO automated dependencies as that messes the automated builds
#- this MUST be tripped manually.
#-
#- Nothing to do if using extracted source in Clearcase (for iSeries builds)
#-

create_zlib: ../openssl_source/$(ZLIB).tar.gz
	[ -n "$(EX_SUFFIX)" ] || \
	( cd .. ;\
		tar xzf openssl_source/$(ZLIB).tar.gz ;\
	)
	[ -n "$(EX_SUFFIX)" ] || \
	( cd $(ZLIB_DIR); \
		sh ../openssl_source/tools/patchem2 "../openssl_source/zlib/$(ZLIB_VER)" ;\
	)

