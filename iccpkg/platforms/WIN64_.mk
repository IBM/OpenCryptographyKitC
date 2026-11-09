# WIN64 stubs for building the libraries

# GSkit8

$(AUXLIB_B).dll: icc_aux$(OBJSUFX) $(ICCPKG_LIBS)
	$(SLD)  $(SLDFLAGS) -DEBUG -PDB:$(AUXLIB_B).pdb $(EXPORT_FLAG)$(ICCAUX_EXPFILE) icc_aux$(OBJSUFX) \
		$(ICCPKG_LIBS) $(LDLIBS)
	-$(CP) *.lib $(GSK_SDK)/
	-$(CP) *.pdb $(GSK_SDK)/
	$(STRIP) $@
	( \
		if [ -e $(SIGN_COMMAND) ] ; then \
			echo "Authenticode signing $@" ; \
			$(SIGN_COMMAND) $@ ; \
		else \
			echo " $(SIGN_COMMAND) is missing skip signing $@" ;\
		fi ;\
	)
	-$(CP) $@ $(GSK_SDK)/
	
$(GSKLIB_B)_64.dll: gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) icc.res
	$(SLD) $(SLDFLAGS) -DEBUG -PDB:$(GSKLIB_B)_64.pdb  \
		gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) $(EXPORT_FLAG)$(ICCPKG_EXPFILE) \
		$(LDLIBS)
	-$(CP) *.lib $(GSK_SDK)/
	-$(CP) *.pdb $(GSK_SDK)/
	$(STRIP) $@
	( \
		if [ -e $(SIGN_COMMAND) ] ; then \
			echo "Authenticode signing $@" ; \
			$(SIGN_COMMAND) $@ ; \
		else \
			echo " $(SIGN_COMMAND) is missing skip signing $@" ;\
		fi ;\
	)

# Java

$(JGSKLIB_B)_64.dll: jgsk_wrap2$(OBJSUFX) $(JEX_OBJS) \
		$(JTIMER_OBJS) \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) icc.res
	$(SLD) $(SLDFLAGS) -DEBUG -PDB:$(JGSKLIB_B)_64.pdb  \
		jgsk_wrap2$(OBJSUFX) $(JEX_OBJS) \
		$(JTIMER_OBJS) \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) $(EXPORT_FLAG)$(JCCPKG_EXPFILE) \
		$(LDLIBS)
	-$(MKDIR) $(JGSK_SDK)/lib
	-$(CP) jgsk*.lib $(JGSK_SDK)/lib/
	-$(MKDIR) $(JGSK_SDK)/debug
	-$(CP) jgsk*.pdb $(JGSK_SDK)/debug/
	-$(CP) ../package/iccsdk/icclib*.pdb $(JGSK_SDK)/debug/
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
		$(LDLIBS) $(OUT)$@
		