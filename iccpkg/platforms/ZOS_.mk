# Fix a problem only on z/OS, the two stub loaders created from icc.c need to have 
# different object names on this platform
# Since the FIPS ICC was already built, change ONLY the name of the object used in non-FIPS mode
# The define is ONLY present in FIPS mode and is in iccpkg/muppet.mk in the FIPS builds
# zoS also seems to need one of the icc stub loaders to be an object. 
# Or - at least linking the object directly was less messy than renaming the library as well
# $(MUPPET) references the OLD (normally FIPS) library that may or may not be available 
# during this build. This may be a new port or the FIPS build for example.
#
# Also note the zOS specific use of chtag to ensure there are no file tags on libicc.a which will 
# prevent it being parsed by the linker.
#
# and -chtag for the case where we may or may not have OLD_ICC (FIPS)
#
#

ifeq ($(strip $(IS_FIPS)),)
   MYICC = newicc
else
   MYICC = icc
endif	

ZICCOBJ = ../icc/$(MYICC)$(OBJSUFX)

$(AUXLIB_B)$(SHLSUFX): icc_aux$(OBJSUFX)
	$(SLD)  $(SLDFLAGS) icc_aux$(OBJSUFX) ../package/gsk_sdk/libgsk8iccs_64.x $(LDLIBS)
	-$(CP) $@ $(GSK_SDK)/$@.unstripped
	$(STRIP) $@
	-$(CP) $@ $(GSK_SDK)/$@

$(GSK_LIBNAME): $(GSK_SDK) gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) ../icc/csvquery_64.o \
		$(ZICCOBJ) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB)
	if [ -e OLD_ICC/ZOS*/iccsdk/libicc.a ] ; then chtag -r OLD_ICC/ZOS*/iccsdk/libicc.a; fi
	if [ -e OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt ] ; then chtag -c ISO8859-1 OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt; fi
	$(SLD) $(SLDFLAGS) gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) ../icc/csvquery_64.o \
		$(ZICCOBJ) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB)  \
		$(LDLIBS)
	$(CP) $(GSKLIB_B)_64.x $(GSK_SDK)/
	$(STRIP) $@


ifneq ($(strip $(MUPPET)),)
OLD_ICC_OBJ=icc$(OBJSUFX)
OLD_ICC_OBJ_AR=$(AR) x $(MUPPET) $(OLD_ICC_OBJ)
OLD_ICC_OBJ_CLEAN=$(RM) $(OLD_ICC_OBJ)
endif

# Static lib
$(GSK_LIB_STATIC): $(GSK_SDK)/static gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) ../icc/csvquery_64.o
	echo static lib
	$(OLD_ICC_OBJ_AR)
	$(AR) $(ARFLAGS) \
		gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(ICC_ROOT)/icc/$(MYICC)$(OBJSUFX) $(OLD_ICC_OBJ) \
		$(STKPK11) $(addprefix $(ICC_ROOT)/icc/,$(ZLIB_OBJ)) ../icc/csvquery_64.o
	$(OLD_ICC_OBJ_CLEAN)

# Java
$(JGSK_LIBNAME): $(JGSK_SDK) $(JGSK_DIR) jgsk_wrap2$(OBJSUFX) jexp$(OBJSUFX) \
		$(JTIMER_OBJS) ../icc/csvquery_64.o \
		$(ZICCOBJ) $(MUPPET) \
		$(ZLIB_LIB)
	if [ -e OLD_ICC/ZOS*/iccsdk/libicc.a ] ; then chtag -r OLD_ICC/ZOS*/iccsdk/libicc.a; fi
	if [ -e OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt ] ; then chtag -c ISO8859-1 OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt; fi
	$(SLD) $(SLDFLAGS) jgsk_wrap2$(OBJSUFX) jexp$(OBJSUFX) \
		$(JTIMER_OBJS) ../icc/csvquery_64.o \
		$(ZICCOBJ) $(MUPPET) \
		$(ZLIB_LIB) \
		$(LDLIBS)
	$(CP) $(JGSKLIB_B)_64.x $(JGSK_SDK)/
	$(STRIP) $@

# ICKC
$(ICKC_LIBNAME): $(ICKC_SDK) $(ICKC_DIR) ickc_wrap2$(OBJSUFX) \
		$(ICKCTIMER_OBJS) ../icc/csvquery_64.o \
		$(ZICCOBJ) $(MUPPET) \
		$(ZLIB_LIB)
	if [ -e OLD_ICC/ZOS*/iccsdk/libicc.a ] ; then chtag -r OLD_ICC/ZOS*/iccsdk/libicc.a; fi
	if [ -e OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt ] ; then chtag -c ISO8859-1 OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt; fi
	$(SLD) $(SLDFLAGS) ickc_wrap2$(OBJSUFX) \
		$(ICKCTIMER_OBJS) ../icc/csvquery_64.o \
		$(ZICCOBJ) $(MUPPET) \
		$(ZLIB_LIB) \
		$(LDLIBS)
	$(CP) $(ICKCLIB_B)_64.x $(ICKC_SDK)/
	$(STRIP) $@

cache_test$(EXESUFX): cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(ZICCOBJ) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) 
	$(LD) cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(ZICCOBJ) $(MUPPET)   \
		$(STKPK11) $(ZLIB_LIB) \
		$(LDLIBS) $(OUT) $@
		