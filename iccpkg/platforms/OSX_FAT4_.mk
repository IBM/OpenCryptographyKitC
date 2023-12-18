# GSkit8

$(AUXLIB_B).dylib: icc_aux$(OBJSUFX) $(GSK_LIB)
	$(SLD)  $(SLDFLAGS) icc_aux$(OBJSUFX) $(ICCPKG_LIBS) $(LDLIBS)
	-$(CP) $@ $(GSK_SDK)/$@.unstripped
	$(STRIP) $@
	-$(CP) $@ $(GSK_SDK)/$@

$(GSKLIB_B).dylib: gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB)
	$(SLD) $(SLDFLAGS) gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) $(EXPORT_FLAG)$(ICCPKG_EXPFILE) \
		$(LDLIBS)
	-$(CP) $@ $(GSK_DIR)/$@.unstripped
	$(STRIP) $@

#Java

$(JGSKLIB_B).dylib: jgsk_wrap2$(OBJSUFX) jexp$(OBJSUFX) \
		$(JTIMER_OBJS) \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB)
	$(SLD) $(SLDFLAGS) jgsk_wrap2$(OBJSUFX) jexp$(OBJSUFX) \
		$(JTIMER_OBJS)  $(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) $(EXPORT_FLAG)$(JCCPKG_EXPFILE) \
		$(LDLIBS)
	-$(MKDIR) $(JGSK_SDK)/debug
	-$(CP) $@ $(JGSK_SDK)/debug/$@.unstripped
	$(STRIP) $@

cache_test$(EXESUFX): cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) 
	$(LD) cache_test$(OBJSUFX) exp$(OBJSUFX)  \
		$(TIMER_OBJS) $(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET)   \
		$(STKPK11) $(ZLIB_LIB) \
		$(LDLIBS) $(OUT) $@