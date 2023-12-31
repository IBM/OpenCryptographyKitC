
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
