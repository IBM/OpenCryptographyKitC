# Windows 32 stubs for building the libraries.

# GSkit8

$(AUXLIB_B).dll: icc_aux$(OBJSUFX) $(ICCPKG_LIBS)
	$(SLD)  $(SLDFLAGS) -DEBUG -PDB:$(AUXLIB_B).pdb $(EXPORT_FLAG)$(ICCAUX_EXPFILE) icc_aux$(OBJSUFX) \
		$(ICCPKG_LIBS) $(LDLIBS)
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
	$(CP) $@ $(GSK_SDK)/

$(GSK_LIBNAME): $(GSK_SDK) $(GSK_DIR) gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) icc.res
	$(SLD) $(SLDFLAGS) -DEBUG -PDB:$(GSKLIB_B).pdb gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
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

# Java variant
# $(JGSK_LIBNAME) is defined in gsk_crypto.mk

$(JGSK_LIBNAME): $(JGSK_SDK)/lib $(JGSK_SDK)/debug jgsk_wrap2$(OBJSUFX) $(JEX_OBJS) \
		$(JTIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) icc.res
	$(SLD) $(SLDFLAGS) -DEBUG -PDB:$(JGSKLIB_B).pdb jgsk_wrap2$(OBJSUFX) $(JEX_OBJS) \
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
		$(TIMER_OBJS) $(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) 
	$(LD) cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET)   \
		$(STKPK11) $(ZLIB_LIB) \
		$(LDLIBS) $(OUT) $@
		