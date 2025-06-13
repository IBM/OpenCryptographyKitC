# WIN64 stubs for building the libraries

# GSkit8

# EX_OBJS TIMER_OBJS comes from the calling Makefile (iccpkg/Makefile)
# IS_FIPS and MUPPET comes from muppet.mk (written by ICCencapsulator in prebuild step) - it is to oiptionally link in OLD_ICC
# STKPK11 comes from the calling Makefile (iccpkg/Makefile) - it is the pkcs11 provider API
# ZLIB_LIB is the external zlib for compression

# $(GSK_LIBNAME) is defined in gsk_crypto.mk

$(GSK_LIBNAME): $(GSK_SDK) $(GSK_DIR) gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) icc.res
	$(SLD) $(SLDFLAGS) -DEBUG -PDB:$(GSKLIB_B)_64.pdb gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) $(EXPORT_FLAG)$(ICCPKG_EXPFILE) \
		$(LDLIBS)
	$(CP) *.lib $(GSK_SDK)/
	$(CP) *.pdb $(GSK_SDK)/
	$(STRIP) $@
	( \
		if [ -e $(SIGN_COMMAND) ] ; then \
			echo "Authenticode signing $@" ; \
			$(SIGN_COMMAND) $@ ; \
		else \
			echo " $(SIGN_COMMAND) is missing skip signing $@" ;\
		fi ;\
	)
	
$(GSK_LIB_STATIC): $(GSK_SDK)/static gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) icc.res
	echo static lib
	$(AR) $(ARFLAGS) gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB)
	$(CP) $(GSK_SDK)/$(GSKLIB_B)_64.pdb $(GSK_SDK)/static

# Java
# $(JGSK_LIBNAME) is defined in gsk_crypto.mk

$(JGSK_LIBNAME): $(JGSK_SDK)/lib $(JGSK_SDK)/debug jgsk_wrap2$(OBJSUFX) $(JEX_OBJS) \
		$(JTIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) icc.res
	$(SLD) $(SLDFLAGS) -DEBUG -PDB:$(JGSKLIB_B)_64.pdb jgsk_wrap2$(OBJSUFX) $(JEX_OBJS) \
		$(JTIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) $(EXPORT_FLAG)$(JCCPKG_EXPFILE) \
		$(LDLIBS)
	$(CP) jgsk*.lib $(JGSK_SDK)/lib/
	$(CP) jgsk*.pdb $(JGSK_SDK)/debug/
	$(CP) $(PACKAGE_DIR)/iccsdk/icclib*.pdb $(JGSK_SDK)/debug/
	$(STRIP) $@
	( \
		if [ -e $(SIGN_COMMAND) ] ; then \
			echo "Authenticode signing $@" ; \
			$(SIGN_COMMAND) $@ ; \
		else \
			echo " $(SIGN_COMMAND) is missing skip signing $@" ;\
		fi ;\
	)

# ICKC
$(ICKC_LIBNAME): $(ICKC_SDK) ickc_wrap2$(OBJSUFX) \
		$(ICKCTIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) icc.res
	$(SLD) $(SLDFLAGS) -DEBUG -PDB:$(ICKCLIB_B)_64.pdb ickc_wrap2$(OBJSUFX) \
		$(ICKCTIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) $(EXPORT_FLAG)$(ICKCPKG_EXPFILE) \
		$(LDLIBS)
	$(CP) ickc*.lib $(ICKC_SDK)/
	$(CP) ickc*.pdb $(ICKC_SDK)/
	$(CP) $(PACKAGE_DIR)/iccsdk/icclib*.pdb $(ICKC_SDK)/
	$(STRIP) $@
	( \
		if [ -e $(SIGN_COMMAND) ] ; then \
			echo "Authenticode signing $@" ; \
			$(SIGN_COMMAND) $@ ; \
		else \
			echo " $(SIGN_COMMAND) is missing skip signing $@" ;\
		fi ;\
	)

cache_test$(EXESUFX): cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) 
	$(LD) cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET)   \
		$(STKPK11) $(ZLIB_LIB) \
		$(LDLIBS) $(OUT) $@
		