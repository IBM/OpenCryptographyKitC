#
# Make file for ICCPKG components exported to GSkit
#

iccpkg: ICC_ver.txt $(ICC_ROOT)/package/ICCPKG.tar  \
	$(ICC_ROOT)/package/gsk_crypto.tar $(ICC_ROOT)/package/gsk_crypto_sdk.tar \
	$(ICC_ROOT)/package/jgsk_crypto.tar $(ICC_ROOT)/package/jgsk_crypto_sdk.tar


$(ICC_ROOT)/package/gsk_crypto.tar: $(ICC_ROOT)/package/gskit_crypto
	-$(RM) $(ICC_ROOT)/package/gskit_crypto/dummyfile
	( \
		cd $(ICC_ROOT)/package/gskit_crypto/; \
		$(TARCMD) $(ICC_ROOT)/gsk_crypto.tar * \
	)

$(ICC_ROOT)/package/gsk_crypto_sdk.tar: $(ICC_ROOT)/package/gsk_sdk
	( \
		cd $(ICC_ROOT)/package; \
	        ( \
			cd gsk_sdk ; \
			touch keep_tar_quiet.pdb ; \
		 	$(TARCMD) pdb.tar *.pdb; \
			cd .. ; \
		); \
		$(TARCMD) gsk_crypto_sdk.tar gsk_sdk \
	)

$(ICC_ROOT)/package/jgsk_crypto.tar: $(ICC_ROOT)/package/jgskit_crypto
	( \
		cd $(ICC_ROOT)/package/jgskit_crypto/; \
		$(TARCMD) $(ICC_ROOT)/jgsk_crypto.tar *; \
	)

$(ICC_ROOT)/package/jgsk_crypto_sdk.tar: $(ICC_ROOT)/package/jgsk_sdk
	( \
		cd $(ICC_ROOT)/package; \
		$(TARCMD) jgsk_crypto_sdk.tar jgsk_sdk ; \
	)

$(ICC_ROOT)/package/ICCPKG.tar: $(ICC_ROOT)/iccpkg/gsk_wrap2.c  
	$(MKDIR) $(PACKAGE_DIR)/sources
	$(MKDIR) $(PACKAGE_DIR)/sources/exports
	$(MKDIR) $(PACKAGE_DIR)/bvt
	$(MKDIR) $(PACKAGE_DIR)/bvt/icc
	$(MKDIR) $(PACKAGE_DIR)/iccpkg_sdk
#	$(MKDIR) $(PACKAGE_DIR)/manifests
	$(MKDIR) $(PACKAGE_DIR)/zlib
	$(MKDIR) $(PACKAGE_DIR)/zlib/include
	$(MKDIR) $(PACKAGE_DIR)/gskit_crypto
	$(MKDIR) $(PACKAGE_DIR)/doc
	echo "Dummy file to stop tar complaining" > $(PACKAGE_DIR)/gskit_crypto/dummyfile
# Copy the bits that end up in the iccpkg SDK
	$(CP) $(ICC_ROOT)/iccpkg/iccpkg_a.h $(PACKAGE_DIR)/iccpkg_sdk/icc_a.h
	$(CP) $(ICC_ROOT)/icc/icc.h $(PACKAGE_DIR)/iccpkg_sdk/
	$(CP) $(ICC_ROOT)/icc/iccglobals.h $(PACKAGE_DIR)/iccpkg_sdk/
	$(CP) $(SDK_DIR)/openssl$(EXESUFX) $(PACKAGE_DIR)/iccpkg_sdk/
# Copy the sources for ICCPKG component
	$(CP) $(ICC_ROOT)/iccpkg/gsk_wrap2.c $(PACKAGE_DIR)/sources/
	$(CP) $(ICC_ROOT)/iccpkg/gsk_wrap2_a.c $(PACKAGE_DIR)/sources/
# Copy the exports files
	$(CP) $(ICC_ROOT)/iccpkg/exports/* $(PACKAGE_DIR)/sources/exports/
# Copy the static libraries, ICC's and ICCPKG PKCS#11
	$(CP) $(SDK_DIR)/$(STLPRFX)icc$(STLSUFX) $(PACKAGE_DIR)/sources/
# Copy the zlib library and headers
	$(CP) $(STLPRFX)zlib$(STLSUFX) $(PACKAGE_DIR)/zlib/
	$(CP) $(ZLIB_DIR)/zlib.h $(PACKAGE_DIR)/zlib/include/
	$(CP) $(ZLIB_DIR)/zconf.h $(PACKAGE_DIR)/zlib/include/
# Copy the test case sources
	$(CP) icctest.c  $(PACKAGE_DIR)/bvt/icc/
	( \
		cd $(ICC_ROOT)/package; \
		$(TARCMD) ICCPKG.tar icc/* sources/* bvt/* iccpkg_sdk/* \
		  	zlib/*;  \
	)

