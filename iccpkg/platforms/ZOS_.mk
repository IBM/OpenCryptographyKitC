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

$(AUXLIB_B).so: icc_aux$(OBJSUFX)
	$(SLD)  $(SLDFLAGS) icc_aux$(OBJSUFX) ../package/gsk_sdk/libgsk8iccs_64.x $(LDLIBS)
	$(STRIP) $@
	-$(CP) $@ $(GSK_SDK)/$@

$(GSKLIB_B)_64.so: gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) \
		$(ZICCOBJ) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) ../icc/csvquery_64.o
	$(SLD)  $(SLDFLAGS)     \
		gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) ../icc/csvquery_64.o \
		$(ZICCOBJ) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB)  \
		$(LDLIBS)
	-$(CP) $(GSKLIB_B)_64.x $(GSK_SDK)/
	$(STRIP) $@


# Java

$(JGSKLIB_B)_64.so: jgsk_wrap2$(OBJSUFX) $(JEX_OBJS) \
		$(JTIMER_OBJS) \
		$(ZICCOBJ) $(MUPPET) \
		$(ZLIB_LIB) ../icc/csvquery_64.o
	$(SLD)  $(SLDFLAGS)  \
		jgsk_wrap2$(OBJSUFX) $(JEX_OBJS) \
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
		