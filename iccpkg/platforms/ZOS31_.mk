$(AUXLIB_B).so: icc_aux$(OBJSUFX)
	$(SLD)  $(SLDFLAGS) icc_aux$(OBJSUFX) ../package/gsk_sdk/libgsk8iccs.x $(LDLIBS)
	$(STRIP) $@
	-$(CP) $@ $(GSK_SDK)/$@

$(GSKLIB_B).so: gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) ../icc/csvquery.o
	$(SLD) $(SLDFLAGS) \
		gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) ../icc/csvquery.o \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB)  \
		$(LDLIBS)
	-$(CP) $(GSKLIB_B).x $(GSK_SDK)/
	$(STRIP) $@

# Java

$(JGSKLIB_B).so: jgsk_wrap2$(OBJSUFX) jexp$(OBJSUFX) \
		$(JTIMER_OBJS) \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) ../icc/csvquery.o
	$(SLD)  $(SLDFLAGS)  \
		jgsk_wrap2$(OBJSUFX) jexp$(OBJSUFX) \
		$(JTIMER_OBJS) ../icc/csvquery.o \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) \
		$(LDLIBS)
	-$(MKDIR) $(JGSK_SDK)
	-$(CP) $(JGSKLIB_B).x $(JGSK_SDK)/
	$(STRIP) $@

cache_test$(EXESUFX): cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) 
	$(LD) cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET)   \
		$(STKPK11) $(ZLIB_LIB) \
		$(LDLIBS) $(OUT) $@
		