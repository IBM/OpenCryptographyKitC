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

ZICCOBJ = ../icc/$(MYICC)$(OBJSUFX)

$(AUXLIB_B).so: icc_aux$(OBJSUFX)
	$(SLD)  $(SLDFLAGS) icc_aux$(OBJSUFX) ../package/gsk_sdk/libgsk8iccs_64.x $(LDLIBS)
	$(STRIP) $@
	-$(CP) $@ $(GSK_SDK)/$@

$(GSKLIB_B)_64.so: gsk_wrap2$(OBJSUFX) exp$(OBJSUFX) totp$(OBJSUFX) \
		$(TIMER_OBJS) \
		$(ZICCOBJ) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) ../icc/csvquery_64.o
	if [ -e OLD_ICC/ZOS*/iccsdk/libicc.a ] ; then chtag -r OLD_ICC/ZOS*/iccsdk/libicc.a; fi
	if [ -e OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt ] ; then chtag -c ISO8859-1 OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt; fi
	$(SLD)  $(SLDFLAGS)     \
		gsk_wrap2$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) ../icc/csvquery_64.o \
		$(ZICCOBJ) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB)  \
		$(LDLIBS)
	-$(CP) $(GSKLIB_B)_64.x $(GSK_SDK)/
	$(STRIP) $@

# GSkit7 compat

$(GSKLIB_B_OLD)_64.so: $(GSK_DIR_OLD) $(GSK_SDK_OLD) \
		 gsk_wrap2_old$(OBJSUFX) exp$(OBJSUFX)  \
		$(ZICCOBJ) $(MUPPET) \
		$(TIMER_OBJS) $(STKPK11) $(ZLIB_LIB) ../icc/csvquery_64.o
	$(SLD)  $(SLDFLAGS) \
		gsk_wrap2_old$(OBJSUFX) exp$(OBJSUFX)  \
		$(TIMER_OBJS) ../icc/csvquery_64.o \
		$(ZICCOBJ) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) \
		$(LDLIBS)
	-$(CP) $(GSKLIB_B_OLD)_64.x $(GSK_SDK_OLD)/
	$(STRIP) $@


# Java

$(JGSKLIB_B)_64.so: jgsk_wrap2$(OBJSUFX) jexp$(OBJSUFX)  \
		$(JTIMER_OBJS) \
		$(ZICCOBJ) $(MUPPET) \
		$(ZLIB_LIB) ../icc/csvquery_64.o
	if [ -e OLD_ICC/ZOS*/iccsdk/libicc.a ] ; then chtag -r OLD_ICC/ZOS*/iccsdk/libicc.a; fi
	if [ -e OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt ] ; then chtag -c ISO8859-1 OLD_ICC/ZOS*A*/icc/icclib/ICCSIG.txt; fi
	$(SLD)  $(SLDFLAGS)  \
		jgsk_wrap2$(OBJSUFX) jexp$(OBJSUFX) \
		$(JTIMER_OBJS) ../icc/csvquery_64.o \
		$(ZICCOBJ) $(MUPPET) \
		$(ZLIB_LIB) \
		$(LDLIBS)
	-$(MKDIR) $(JGSK_SDK)
	-$(CP) $(JGSKLIB_B)_64.x $(JGSK_SDK)/
	$(STRIP) $@

cache_test$(EXESUFX): cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(ZICCOBJ) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) 
	$(LD) cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(ZICCOBJ) $(MUPPET)   \
		$(STKPK11) $(ZLIB_LIB) \
		$(LDLIBS) $(OUT) $@
		