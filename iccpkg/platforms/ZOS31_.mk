# Ref ZOS_.mk if there is a FIPS module (MUPPET)
# ZICCOBJ = ../icc/$(MYICC)$(OBJSUFX)

$(AUXLIB_B)$(SHLSUFX): icc_aux$(OBJSUFX)
	$(SLD)  $(SLDFLAGS) icc_aux$(OBJSUFX) ../package/gsk_sdk/libgsk8iccs.x $(LDLIBS)
	-$(CP) $@ $(GSK_SDK)/$@.unstripped
	$(STRIP) $@
	-$(CP) $@ $(GSK_SDK)/$@

$(GSK_LIBNAME): $(GSK_SDK) gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) ../icc/csvquery.o
	if [ -e OLD_ICC/ZOS*/iccsdk/libicc.a ] ; then chtag -r OLD_ICC/ZOS*/iccsdk/libicc.a; fi
	if [ -e OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt ] ; then chtag -c ISO8859-1 OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt; fi
	$(SLD) $(SLDFLAGS) \
		gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) ../icc/csvquery.o \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB)  \
		$(LDLIBS)
	$(CP) $(GSKLIB_B).x $(GSK_SDK)/
	$(STRIP) $@

ifneq ($(strip $(MUPPET)),)
OLD_ICC_OBJ=icc$(OBJSUFX)
OLD_ICC_OBJ_AR=$(AR) x $(MUPPET) $(OLD_ICC_OBJ)
OLD_ICC_OBJ_CLEAN=$(RM) $(OLD_ICC_OBJ)
endif

# Static lib
$(GSK_LIB_STATIC): $(GSK_SDK)/static gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) ../icc/csvquery.o
	echo static lib
	if [ -e OLD_ICC/ZOS*/iccsdk/libicc.a ] ; then chtag -r OLD_ICC/ZOS*/iccsdk/libicc.a; fi
	if [ -e OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt ] ; then chtag -c ISO8859-1 OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt; fi
	$(OLD_ICC_OBJ_AR)
	$(AR) $(ARFLAGS)  \
		gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(ICC_ROOT)/icc/$(MYICC)$(OBJSUFX) $(OLD_ICC_OBJ)\
		$(STKPK11) $(addprefix $(ICC_ROOT)/icc/,$(ZLIB_OBJ)) ../icc/csvquery.o
	$(OLD_ICC_OBJ_CLEAN)

# Java
$(JGSK_LIBNAME): $(JGSK_SDK) jgsk_wrap2$(OBJSUFX) jexp$(OBJSUFX) \
		$(JTIMER_OBJS) \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) ../icc/csvquery.o
	if [ -e OLD_ICC/ZOS*/iccsdk/libicc.a ] ; then chtag -r OLD_ICC/ZOS*/iccsdk/libicc.a; fi
	if [ -e OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt ] ; then chtag -c ISO8859-1 OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt; fi
	$(SLD)  $(SLDFLAGS)  \
		jgsk_wrap2$(OBJSUFX) jexp$(OBJSUFX) \
		$(JTIMER_OBJS) ../icc/csvquery.o \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) \
		$(LDLIBS)
	$(CP) $(JGSKLIB_B).x $(JGSK_SDK)/
	$(STRIP) $@

# ICKC
$(ICKC_LIBNAME): $(ICKC_SDK) $(ICKC_DIR) ickc_wrap2$(OBJSUFX) \
		$(ICKCTIMER_OBJS) ../icc/csvquery.o \
		$(ZICCOBJ) $(MUPPET) \
		$(ZLIB_LIB)
	if [ -e OLD_ICC/ZOS*/iccsdk/libicc.a ] ; then chtag -r OLD_ICC/ZOS*/iccsdk/libicc.a; fi
	if [ -e OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt ] ; then chtag -c ISO8859-1 OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt; fi
	$(SLD) $(SLDFLAGS) ickc_wrap2$(OBJSUFX) \
		$(ICKCTIMER_OBJS) ../icc/csvquery.o \
		$(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) \
		$(LDLIBS)
	$(CP) $(ICKCLIB_B).x $(ICKC_SDK)/
	$(STRIP) $@

cache_test$(EXESUFX): cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) 
	$(LD) cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(NEW_ICC)/iccsdk/$(ICCLIB) $(MUPPET)   \
		$(STKPK11) $(ZLIB_LIB) \
		$(LDLIBS) $(OUT) $@