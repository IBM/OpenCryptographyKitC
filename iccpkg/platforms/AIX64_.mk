# Filenames should come from iccpkg/gsk_crypto.mk
# GSKit8/9

$(AUXLIB_B)$(SHLSUFX): $(GSK_SDK) $(GSK_DIR) icc_aux$(OBJSUFX)
	$(SLD)  $(SLDFLAGS) icc_aux$(OBJSUFX) $(LDLIBS)
	$(CP) $@ $(GSK_SDK)/$@.unstripped
	$(STRIP) $@
	$(CP) $@ $(GSK_SDK)/

$(GSK_LIBNAME): $(GSK_SDK) $(GSK_DIR) gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) 
	$(SLD) $(SLDFLAGS) gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET)   \
		$(STKPK11) $(ZLIB_LIB) $(EXPORT_FLAG)$(ICCPKG_EXPFILE) \
		$(LDLIBS)
	$(CP) $@ $(GSK_SDK)/$@.unstripped
	$(STRIP) $@
	$(CP) $@ $(GSK_DIR)/

ifneq ($(strip $(MUPPET)),)
# ar x not working on AIX64 for some reason
# will need to link $(MUPPET) in icctest_s
#OLD_ICC_OBJ=icc$(OBJSUFX)
#OLD_ICC_OBJ_AR=$(AR) t $(MUPPET) ; $(AR) x $(MUPPET) $(OLD_ICC_OBJ)
#OLD_ICC_OBJ_CLEAN=$(RM) $(OLD_ICC_OBJ)
OLD_ICC_OBJ=
OLD_ICC_OBJ_AR=
OLD_ICC_OBJ_CLEAN=
endif

# Static lib
$(GSK_LIB_STATIC): $(GSK_SDK)/static gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB)
	echo static lib
	$(OLD_ICC_OBJ_AR)
	$(AR) $(ARFLAGS) \
		gsk_wrap2$(OBJSUFX) $(EX_OBJS) \
		$(TIMER_OBJS) $(ICC_ROOT)/icc/$(MYICC)$(OBJSUFX) $(OLD_ICC_OBJ)\
		$(STKPK11) $(addprefix $(ICC_ROOT)/icc/,$(ZLIB_OBJ))
	$(OLD_ICC_OBJ_CLEAN)
	$(CP) $(MUPPET) $(GSK_SDK)/static

# Java
$(JGSK_LIBNAME): $(JGSK_SDK)/debug $(JGSK_DIR) jgsk_wrap2$(OBJSUFX) jexp$(OBJSUFX) \
		$(JTIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB)
	$(SLD) $(SLDFLAGS) jgsk_wrap2$(OBJSUFX) jexp$(OBJSUFX) \
		$(JTIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) $(EXPORT_FLAG)$(JCCPKG_EXPFILE) \
		$(LDLIBS)
	$(CP) $@ $(JGSK_SDK)/debug/$@.unstripped
	$(STRIP) $@
	$(CP) $@ $(JGSK_DIR)/

# ICKC
$(ICKC_LIBNAME): $(ICKC_SDK)/debug $(ICKC_DIR) ickc_wrap2$(OBJSUFX) \
		$(ICKCTIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB)
	$(SLD) $(SLDFLAGS) ickc_wrap2$(OBJSUFX) \
		$(ICKCTIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(ZLIB_LIB) $(EXPORT_FLAG)$(ICKCPKG_EXPFILE) \
		$(LDLIBS)
	$(CP) $@ $(ICKC_SDK)/debug/$@.unstripped
	$(STRIP) $@
	$(CP) $@ $(ICKC_DIR)/

cache_test$(EXESUFX): cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET) \
		$(STKPK11) $(ZLIB_LIB) 
	$(LD) cache_test$(OBJSUFX) exp$(OBJSUFX) \
		$(TIMER_OBJS) $(PACKAGE_DIR)/iccsdk/$(ICCLIB) $(MUPPET)   \
		$(STKPK11) $(ZLIB_LIB) \
		$(LDLIBS) $(OUT) $@
