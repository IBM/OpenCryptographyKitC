#
# Make file for ICCPKG components exported to GSkit
#

iccpkg: ICC_ver.txt ../package/ICCPKG.tar  \
	../package/gsk_crypto.tar ../package/gsk_crypto_sdk.tar \
	../package/jgsk_crypto.tar ../package/jgsk_crypto_sdk.tar
	@echo $@ made


../package/gsk_crypto.tar: ../package/gskit_crypto
	-$(RM) ../package/gskit_crypto/dummyfile
	( \
		cd ../package/gskit_crypto/; \
		$(TARCMD) ../gsk_crypto.tar * \
	)

../package/gsk_crypto_sdk.tar: ../package/gsk_sdk
	( \
		cd ../package; \
	        ( \
			cd gsk_sdk ; \
			touch keep_tar_quiet.pdb ; \
		 	$(TARCMD) pdb.tar *.pdb; \
			cd .. ; \
		); \
		$(TARCMD) gsk_crypto_sdk.tar gsk_sdk \
	)

../package/jgsk_crypto.tar: ../package/jgskit_crypto
	( \
		cd ../package/jgskit_crypto/; \
		$(TARCMD) ../jgsk_crypto.tar *; \
	)

../package/jgsk_crypto_sdk.tar: ../package/jgsk_sdk
	( \
		cd ../package; \
		$(TARCMD) jgsk_crypto_sdk.tar jgsk_sdk ; \
	)

../package/ICCPKG.tar: ../iccpkg/gsk_wrap2.c  
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
	$(CP) ../iccpkg/iccpkg_a.h $(PACKAGE_DIR)/iccpkg_sdk/icc_a.h
	$(CP) icc.h $(PACKAGE_DIR)/iccpkg_sdk/
	$(CP) iccglobals.h $(PACKAGE_DIR)/iccpkg_sdk/
	$(CP) $(SDK_DIR)/openssl $(PACKAGE_DIR)/iccpkg_sdk/
# Copy the sources for ICCPKG component
	$(CP) ../iccpkg/gsk_wrap2.c $(PACKAGE_DIR)/sources/
	$(CP) ../iccpkg/gsk_wrap2_a.c $(PACKAGE_DIR)/sources/
# Copy the exports files
	$(CP) ../iccpkg/exports/* $(PACKAGE_DIR)/sources/exports/
# Copy the static libraries, ICC's and ICCPKG PKCS#11
	$(CP) $(SDK_DIR)/$(STLPRFX)icc$(STLSUFX) $(PACKAGE_DIR)/sources/
# Copy the zlib library and headers
	$(CP) $(STLPRFX)zlib$(STLSUFX) $(PACKAGE_DIR)/zlib/
	$(CP) $(ZLIB_DIR)/zlib.h $(PACKAGE_DIR)/zlib/include/
	$(CP) $(ZLIB_DIR)/zconf.h $(PACKAGE_DIR)/zlib/include/
# Copy the test case sources
	$(CP) icctest.c  $(PACKAGE_DIR)/bvt/icc/
	( \
		cd ../package; \
		$(TARCMD) ICCPKG.tar icc/* sources/* bvt/* iccpkg_sdk/* \
		  	zlib/*;  \
	)

