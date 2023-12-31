DEFAULT_EXTRA_FILES    = *.pdb *.ilk *.plg
DEFAULT_EXPORT_FLAG    = -def:
DEFAULT_ICCLIB_EXPFILE = exports/icclib_win32.def
DEFAULT_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
DEFAULT_OSSLDLL_NAME   = libeay32.dll
DEFAULT_OSSLINC_DIR    = $(OSSL_DIR)/inc32
DEFAULT_release_OSSL_SUFFIX  =
DEFAULT_debug_OSSL_SUFFIX    = 
DEFAULT_release_EXPORT  = export OSSL_RELEASE=1
DEFAULT_debug_EXPORT    = export OSSL_DEBUG=debug
DEFAULT_OSSLOBJ_DIR    = $(OSSL_DIR)/out32dll$($(OPSYS)_$(CONFIG)_OSSL_SUFFIX)
DEFAULT_OPENSSL        = openssl
DEFAULT_TARCMD	     = tar -cvf
DEFAULT_TEST_CMD       = ./icctest.exe
# This is actually used to build an rc file on Windows
DEFAULT_ASMOBJS        =
DEFAULT_EXTRAS         =
DEFAULT_debug_FILES    = icclib$(VTAG).pdb openssl.pdb vc90.pdb \
			$(OSSL_DIR)/out32dll/libeay32.pdb 
DEFAULT_MANIFESTS      =

#set up defaults for future stanzas
$(OPSYS)_EXTRA_FILES    = $(DEFAULT)_EXTRA_FILES
$(OPSYS)_EXPORT_FLAG    = $(DEFAULT_EXPORT_FLAG)
$(OPSYS)_ICCLIB_EXPFILE = $(DEFAULT_ICCLIB_EXPFILE)
$(OPSYS)_ICCDLL_NAME    = $(DEFAULT_ICCDLL_NAME)
$(OPSYS)_OSSLDLL_NAME   = $(DEFAULT_OSSLDLL_NAME)
$(OPSYS)_OSSLINC_DIR    = $(DEFAULT_OSSLINC_DIR)
$(OPSYS)_release_OSSL_SUFFIX = $(DEFAULT_release_OSSL_SUFFIX)
$(OPSYS)_debug_OSSL_SUFFIX = $(DEFAULT_debug_OSSL_SUFFIX)
$(OPSYS)_release_EXPORT  = $(DEFAULT_release_EXPORT)
$(OPSYS)_debug_EXPORT    = $(DEFAULT_debug_EXPORT)
$(OPSYS)_OSSLOBJ_DIR    = $(DEFAULT_OSSLOBJ_DIR)
$(OPSYS)_OPENSSL        = $(DEFAULT_OPENSSL)
$(OPSYS)_TARCMD	     = $(DEFAULT_TARCMD)
$(OPSYS)_TEST_CMD       = $(DEFAULT_TEST_CMD)
# This is actually used to build an rc file on Windows
$(OPSYS)_ASMOBJS        = $(DEFAULT_ASMOBJS)
$(OPSYS)_EXTRAS         = $(DEFAULT_EXTRAS)
$(OPSYS)_debug_FILES    = $(DEFAULT_debug_FILES)
$(OPSYS)_MANIFESTS      = $(DEFAULT_MANIFESTS)

WIN32_EXTRA_FILES    = *.pdb *.ilk *.plg
WIN32_EXPORT_FLAG    = -def:
WIN32_ICCLIB_EXPFILE = exports/icclib_win32.def
WIN32_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
WIN32_OSSLDLL_NAME   = libeay32.dll
WIN32_OSSLINC_DIR    = $(OSSL_DIR)/inc32
WIN32_release_OSSL_SUFFIX  =
WIN32_debug_OSSL_SUFFIX    = 
WIN32_release_EXPORT  = export OSSL_RELEASE=1
WIN32_debug_EXPORT    = export OSSL_DEBUG=debug
WIN32_OSSLOBJ_DIR    = $(OSSL_DIR)/out32dll$(WIN32_$(CONFIG)_OSSL_SUFFIX)
WIN32_OPENSSL        = openssl
WIN32_TARCMD	     = tar -cvf
WIN32_TEST_CMD       = ./icctest.exe
# This is actually used to build an rc file on Windows
WIN32_ASMOBJS        = icc.res
WIN32_EXTRAS         = 
WIN32_debug_FILES    = icclib$(VTAG).pdb openssl.pdb vc90.pdb \
			$(OSSL_DIR)/out32dll/libeay32.pdb 
WIN32_MANIFESTS      = openssl.exe.manifest ../package/iccsdk/GenRndData.exe.manifest ../package/iccsdk/GenRndData2.exe.manifest ../iccspeed/bin/WIN32/icc_thread.exe.manifest $(WIN32_$(CONFIG)_FILES)

WIN64_AMD_EXTRA_FILES    = *.pdb *.ilk *.plg
WIN64_AMD_EXPORT_FLAG    = -def:
WIN64_AMD_ICCLIB_EXPFILE = $(WIN32_ICCLIB_EXPFILE)
WIN64_AMD_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
WIN64_AMD_OSSLDLL_NAME   = $(WIN32_OSSLDLL_NAME)
WIN64_AMD_OSSLINC_DIR    = $(OSSL_DIR)/inc32
WIN64_AMD_release_OSSL_SUFFIX  =
WIN64_AMD_debug_OSSL_SUFFIX    =
WIN64_AMD_release_EXPORT  = export OSSL_RELEASE=1
WIN64_AMD_debug_EXPORT    = export OSSL_DEBUG=debug
WIN64_AMD_OSSLOBJ_DIR    = $(OSSL_DIR)/out32dll$(WIN64_AMD_$(CONFIG)_OSSL_SUFFIX)
WIN64_AMD_OPENSSL        = $(OSSLOBJ_DIR)/openssl
# Moved to platforms/
WIN64_AMD_TARCMD	       = tar -cvf
WIN64_AMD_TEST_CMD       = ./icctest.exe
WIN64_AMD_ASMOBJS        = icc.res
WIN64_AMD_EXTRAS  	     = $(WIN32_EXTRAS)
WIN64_AMD_MANIFESTS      =

#- Visual studio 2013
WIN32_VS2013_EXTRA_FILES    = *.pdb *.ilk *.plg
WIN32_VS2013_EXPORT_FLAG    = -def:
WIN32_VS2013_ICCLIB_EXPFILE = exports/icclib_win32.def
WIN32_VS2013_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
WIN32_VS2013_OSSLDLL_NAME   = libeay32.dll
WIN32_VS2013_OSSLINC_DIR    = $(OSSL_DIR)/inc32
WIN32_VS2013_release_OSSL_SUFFIX  =
WIN32_VS2013_debug_OSSL_SUFFIX    = 
WIN32_VS2013_release_EXPORT  = export OSSL_RELEASE=1
WIN32_VS2013_debug_EXPORT    = export OSSL_DEBUG=debug
WIN32_VS2013_OSSLOBJ_DIR    = $(OSSL_DIR)/out32dll$(WIN32_VS2013_$(CONFIG)_OSSL_SUFFIX)
WIN32_VS2013_OPENSSL        = openssl
WIN32_VS2013_TARCMD	     = tar -cvf
WIN32_VS2013_TEST_CMD       = ./icctest.exe
# This is actually used to build an rc file on Windows
WIN32_VS2013_ASMOBJS        = icc.res
WIN32_VS2013_EXTRAS         = 
WIN32_VS2013_debug_FILES    = icclib$(VTAG).pdb openssl.pdb vc90.pdb \
			$(OSSL_DIR)/out32dll/libeay32.pdb 
WIN32_VS2013_MANIFESTS      = openssl.exe.manifest ../package/iccsdk/GenRndData.exe.manifest ../package/iccsdk/GenRndData2.exe.manifest ../iccspeed/bin/WIN32/icc_thread.exe.manifest $($(OPSYS)_$(CONFIG)_FILES)

WIN32_VS2022_ASMOBJS        = icc.res

WIN64_AMD_VS2013_EXTRA_FILES    = *.pdb *.ilk *.plg
WIN64_AMD_VS2013_EXPORT_FLAG    = -def:
WIN64_AMD_VS2013_ICCLIB_EXPFILE = $(WIN32_ICCLIB_EXPFILE)
WIN64_AMD_VS2013_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
WIN64_AMD_VS2013_OSSLDLL_NAME   = $(WIN32_OSSLDLL_NAME)
WIN64_AMD_VS2013_OSSLINC_DIR    = $(OSSL_DIR)/inc32
WIN64_AMD_VS2013_release_OSSL_SUFFIX  =
WIN64_AMD_VS2013_debug_OSSL_SUFFIX    =
WIN64_AMD_VS2013_release_EXPORT  = export OSSL_RELEASE=1
WIN64_AMD_VS2013_debug_EXPORT    = export OSSL_DEBUG=debug
WIN64_AMD_VS2013_OSSLOBJ_DIR    = $(OSSL_DIR)/out32dll$(WIN64_AMD_$(CONFIG)_OSSL_SUFFIX)
WIN64_AMD_VS2013_OPENSSL        = $(OSSLOBJ_DIR)/openssl
# Moved to platforms/
WIN64_AMD_VS2013_TARCMD	       = tar -cvf
WIN64_AMD_VS2013_TEST_CMD       = ./icctest.exe
WIN64_AMD_VS2013_ASMOBJS        = icc.res
WIN64_AMD_VS2013_EXTRAS  	     = $(WIN32_EXTRAS)
WIN64_AMD_VS2013_MANIFESTS      =

WIN64_VS2022_EXTRA_FILES    = *.pdb *.ilk *.plg
WIN64_VS2022_EXPORT_FLAG    = -def:
WIN64_VS2022_ICCLIB_EXPFILE = $(WIN32_ICCLIB_EXPFILE)
WIN64_VS2022_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
WIN64_VS2022_OSSLDLL_NAME   = $(WIN32_OSSLDLL_NAME)
WIN64_VS2022_OSSLINC_DIR    = $(OSSL_DIR)/inc32
WIN64_VS2022_release_OSSL_SUFFIX  =
WIN64_VS2022_debug_OSSL_SUFFIX    =
WIN64_VS2022_release_EXPORT  = export OSSL_RELEASE=1
WIN64_VS2022_debug_EXPORT    = export OSSL_DEBUG=debug
WIN64_VS2022_OSSLOBJ_DIR    = $(OSSL_DIR)/out32dll$(WIN64_VS2022_$(CONFIG)_OSSL_SUFFIX)
WIN64_VS2022_OPENSSL        = $(OSSLOBJ_DIR)/openssl
# Moved to platforms/
WIN64_VS2022_TARCMD	       = tar -cvf
WIN64_VS2022_TEST_CMD       = ./icctest.exe
WIN64_VS2022_ASMOBJS        = icc.res
WIN64_VS2022_EXTRAS  	     = $(WIN32_EXTRAS)
WIN64_VS2022_MANIFESTS      =

# Linux, generic, but targetted at ia32. Build with this first on a new platform
LINUX_EXPORT_FLAG    = -Wl,--version-script,
LINUX_ICCLIB_EXPFILE = exports/icclib_linux.exp
LINUX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
LINUX_OSSLDLL_NAME   = libcrypto.so.$(OPENSSL_LIBVER)
LINUX_OSSLINC_DIR    = $(OSSL_DIR)/include
LINUX_OSSLOBJ_DIR    = $(OSSL_DIR)
LINUX_OPENSSL        = $(OSSL_DIR)/apps/openssl
# semicolon deliberate
LINUX_OPENSSL_PATH_SETUP = export LD_LIBRARY_PATH=$(OSSL_DIR);
LINUX_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(LINUX_$(CONFIG)_CFLAGS) -m32 linux-elf; make depend; make
LINUX_CLEAN_OSSL     = cd $(OSSL_DIR); make clean
LINUX_TARCMD	     = tar -cvhf
LINUX_TEST_CMD       = ./icctest;cd $(OSSL_DIR);make tests;apps/openssl speed;cd  ../icc
LINUX_EXTRAS  	     = 

# Linux using normal shared library conventions
# used to test the build for platforms with no dlopen/dlsym.
LINUXDL_EXPORT_FLAG    = $(LINUX_EXPORT_FLAG)
LINUXDL_ICCLIB_EXPFILE = $(LINUX_ICCLIB_EXPFILE)
LINUXDL_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
LINUXDL_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
LINUXDL_OSSLINC_DIR    = $(OSSL_DIR)/include
LINUXDL_OSSLOBJ_DIR    = $(OSSL_DIR)
LINUXDL_OPENSSL        = $(OSSL_DIR)/apps/openssl
# semicolon deliberate
LINUXDL_OPENSSL_PATH_SETUP = export LD_LIBRARY_PATH=$(OSSL_DIR);
LINUXDL_BUILD_OSSL     = $(LINUX_BUILD_OSSL)
LINUXDL_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
LINUXDL_TARCMD	       = tar -cvhf
LINUXDL_TEST_CMD       = $(LINUX_TEST_CMD)
LINUXDL_ICCLIB_FLAGS   = -L ../package/icc/osslib -lcryptoIBM$(VTAG)  -L ../package/icc/icclib -licclib
LINUXDL_EXTRAS         = $(LINUX_EXTRAS)

# Linux icc (Intel compiler)
# Most, but not all, definitions are snarfed from the GCC config above
LINUX_CICC_EXPORT_FLAG    = $(LINUX_EXPORT_FLAG)
LINUX_CICC_ICCLIB_EXPFILE = $(LINUX_ICCLIB_EXPFILE)
LINUX_CICC_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
LINUX_CICC_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
LINUX_CICC_OSSLINC_DIR    = $(LINUX_OSSLINC_DIR)
LINUX_CICC_OSSLOBJ_DIR    = $(LINUX_OSSLOBJ_DIR)
LINUX_CICC_OPENSSL        = $(LINUX_OPENSSL)
LINUX_CICC_OPENSSL_PATH_SETUP = $(LINUX_OPENSSL_PATH_SETUP)
# This one DOES differ
LINUX_CICC_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(LINUX_CICC_$(CONFIG)_CFLAGS)  linux-pentium-icc; make depend; make
LINUX_CICC_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
LINUX_CICC_TARCMD         = $(LINUX_TARCMD)
LINUX_CICC_TEST_CMD       = $(LINUX_TEST_CMD)
LINUX_CICC_EXTRAS         = $(LINUX_EXTRAS)

IA64_LINUX_EXPORT_FLAG    = $(LINUX_EXPORT_FLAG)
IA64_LINUX_ICCLIB_EXPFILE = $(LINUX_ICCLIB_EXPFILE)
IA64_LINUX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
IA64_LINUX_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
IA64_LINUX_OSSLINC_DIR    = $(OSSL_DIR)/include
IA64_LINUX_OSSLOBJ_DIR    = $(OSSL_DIR)
IA64_LINUX_OPENSSL        = $(OSSL_DIR)/apps/openssl
# semicolon deliberate
IA64_LINUX_OPENSSL_PATH_SETUP = export LD_LIBRARY_PATH=$(OSSL_DIR);
IA64_LINUX_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(IA64_LINUX_$(CONFIG)_CFLAGS) linux-ia64; make depend; make
IA64_LINUX_CLEAN_OSSL     = cd $(OSSL_DIR); make clean clean-shared; $(RM) $(OSSL_DIR)/Makefile
IA64_LINUX_TARCMD         = tar -cvhf
IA64_LINUX_TEST_CMD       = $(LINUX_TEST_CMD)
IA64_LINUX_EXTRAS         = $(LINUX_EXTRAS)

ARM_LINUX_EXPORT_FLAG    = $(LINUX_EXPORT_FLAG)
ARM_LINUX_ICCLIB_EXPFILE = $(LINUX_ICCLIB_EXPFILE)
ARM_LINUX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
ARM_LINUX_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
ARM_LINUX_OSSLINC_DIR    = $(OSSL_DIR)/include
ARM_LINUX_OSSLOBJ_DIR    = $(OSSL_DIR)
ARM_LINUX_OPENSSL        = $(OSSL_DIR)/apps/openssl
# semicolon deliberate
ARM_LINUX_OPENSSL_PATH_SETUP = export LD_LIBRARY_PATH=$(OSSL_DIR);
ARM_LINUX_BUILD_OSSL     = cd $(OSSL_DIR);./Configure threads shared $(OSSL_FLAGS) $(ARM_LINUX_$(CONFIG)_CFLAGS) linux-armv4; make depend; make
ARM_LINUX_CLEAN_OSSL     = cd $(OSSL_DIR); make clean clean-shared; $(RM) $(OSSL_DIR)/Makefile
ARM_LINUX_TARCMD         = tar -cvhf
ARM_LINUX_TEST_CMD       = $(LINUX_TEST_CMD)
ARM_LINUX_EXTRAS         = $(LINUX_EXTRAS)

ARM64_LINUX_EXPORT_FLAG    = $(LINUX_EXPORT_FLAG)
ARM64_LINUX_ICCLIB_EXPFILE = $(LINUX_ICCLIB_EXPFILE)
ARM64_LINUX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
ARM64_LINUX_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
ARM64_LINUX_OSSLINC_DIR    = $(OSSL_DIR)/include
ARM64_LINUX_OSSLOBJ_DIR    = $(OSSL_DIR)
ARM64_LINUX_OPENSSL        = $(OSSL_DIR)/apps/openssl
# semicolon deliberate
ARM64_LINUX_OPENSSL_PATH_SETUP = export LD_LIBRARY_PATH=$(OSSL_DIR);
ARM64_LINUX_BUILD_OSSL     = cd $(OSSL_DIR);./Configure threads shared $(OSSL_FLAGS) $(ARM_LINUX_$(CONFIG)_CFLAGS) linux-aarch64; make depend; make
ARM64_LINUX_CLEAN_OSSL     = cd $(OSSL_DIR); make clean clean-shared; $(RM) $(OSSL_DIR)/Makefile
ARM64_LINUX_TARCMD         = tar -cvhf
ARM64_LINUX_TEST_CMD       = $(LINUX_TEST_CMD)
ARM64_LINUX_EXTRAS         = $(LINUX_EXTRAS)

# ia64 Linux cross compiled, a bit smarter than s390 in reusing native defs
CROSS_IA64_LINUX_EXPORT_FLAG    = $(IA64_LINUX_EXPORT_FLAG)
CROSS_IA64_LINUX_ICCLIB_EXPFILE = $(IA64_LINUX_ICCLIB_EXPFILE)
CROSS_IA64_LINUX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
CROSS_IA64_LINUX_OSSLDLL_NAME   = $(IA64_LINUX_OSSLDLL_NAME)
CROSS_IA64_LINUX_OSSLINC_DIR    = $(IA64_LINUX_OSSLINC_DIR)
CROSS_IA64_LINUX_OSSLOBJ_DIR    = $(IA64_LINUX_OSSLOBJ_DIR)
CROSS_IA64_LINUX_OPENSSL        = openssl
CROSS_IA64_LINUX_OPENSSL_PATH_SETUP = $(IA64_LINU;./Configure threads shared $(OSSL_FLAGS) $(CROSS_IA64_LINUX_$(CONFIG)_CFLAGS) linux-ia64-cross ; make depend; make
CROSS_IA64_LINUX_CLEAN_OSSL     = $(IA64_LINUX_CLEAN_OSSL)
CROSS_IA64_LINUX_TARCMD	        = $(IA64_LINUX_TARCMD)
CROSS_IA64_LINUX_TEST_CMD       = $(IA64_LINUX_TEST_CMD)
CROSS_IA64_LINUX_EXTRAS         = $(LINUX_EXTRAS)

AMD64_LINUX_EXPORT_FLAG    = $(LINUX_EXPORT_FLAG)
AMD64_LINUX_ICCLIB_EXPFILE = $(LINUX_ICCLIB_EXPFILE)
AMD64_LINUX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
AMD64_LINUX_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
AMD64_LINUX_OSSLINC_DIR    = $(OSSL_DIR)/include
AMD64_LINUX_OSSLOBJ_DIR    = $(OSSL_DIR)
AMD64_LINUX_OPENSSL        = $(OSSL_DIR)/apps/openssl
# semicolon deliberate
AMD64_LINUX_OPENSSL_PATH_SETUP = export LD_LIBRARY_PATH=$(OSSL_DIR);
AMD64_LINUX_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(AMD64_LINUX_$(CONFIG)_CFLAGS)  linux-x86_64; make depend; make
AMD64_LINUX_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
AMD64_LINUX_TARCMD	   = tar -cvhf
AMD64_LINUX_TEST_CMD       = $(LINUX_TEST_CMD)
AMD64_LINUX_EXTRAS         = $(LINUX_EXTRAS)

PPC_LINUX_EXPORT_FLAG    = $(LINUX_EXPORT_FLAG)
PPC_LINUX_ICCLIB_EXPFILE = $(LINUX_ICCLIB_EXPFILE)
PPC_LINUX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
PPC_LINUX_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
PPC_LINUX_OSSLINC_DIR    = $(OSSL_DIR)/include
PPC_LINUX_OSSLOBJ_DIR    = $(OSSL_DIR)
PPC_LINUX_OPENSSL        = $(OSSL_DIR)/apps/openssl
PPC_LINUX_OPENSSL_PATH_SETUP = export LD_LIBRARY_PATH=$(OSSL_DIR);
PPC_LINUX_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(PPC_LINUX_$(CONFIG)_CFLAGS) linux-ppc; make depend; make
PPC_LINUX_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
PPC_LINUX_TARCMD	 = tar -cvhf
PPC_LINUX_TEST_CMD       = $(LINUX_TEST_CMD)
PPC_LINUX_EXTRAS         = $(LINUX_EXTRAS)

#PPC64 Big endian
PPC64_LINUX_EXPORT_FLAG    = $(LINUX_EXPORT_FLAG)
PPC64_LINUX_ICCLIB_EXPFILE = $(LINUX_ICCLIB_EXPFILE)
PPC64_LINUX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
PPC64_LINUX_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
PPC64_LINUX_OSSLINC_DIR    = $(OSSL_DIR)/include
PPC64_LINUX_OSSLOBJ_DIR    = $(OSSL_DIR)
PPC64_LINUX_OPENSSL        = $(OSSL_DIR)/apps/openssl
PPC64_LINUX_OPENSSL_PATH_SETUP = export LD_LIBRARY_PATH=$(OSSL_DIR);
PPC64_LINUX_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(PPC64_LINUX_$(CONFIG)_CFLAGS) linux-ppc64; make depend; make
PPC64_LINUX_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
PPC64_LINUX_TARCMD         = tar -cvhf
PPC64_LINUX_TEST_CMD       = $(LINUX_TEST_CMD)
PPC64_LINUX_EXTRAS         = $(LINUX_EXTRAS)

#PPC64 Little Endian
PPC64LE_LINUX_EXPORT_FLAG    = $(LINUX_EXPORT_FLAG)
PPC64LE_LINUX_ICCLIB_EXPFILE = $(LINUX_ICCLIB_EXPFILE)
PPC64LE_LINUX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
PPC64LE_LINUX_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
PPC64LE_LINUX_OSSLINC_DIR    = $(OSSL_DIR)/include
PPC64LE_LINUX_OSSLOBJ_DIR    = $(OSSL_DIR)
PPC64LE_LINUX_OPENSSL        = $(OSSL_DIR)/apps/openssl
PPC64LE_LINUX_OPENSSL_PATH_SETUP = export LD_LIBRARY_PATH=$(OSSL_DIR);
PPC64LE_LINUX_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(PPC64LE_LINUX_$(CONFIG)_CFLAGS) linux-ppc64le; make depend; make
PPC64LE_LINUX_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
PPC64LE_LINUX_TARCMD         = tar -cvhf
PPC64LE_LINUX_TEST_CMD       = $(LINUX_TEST_CMD)
PPC64LE_LINUX_EXTRAS         = $(LINUX_EXTRAS)



# PCC Mac OS X - PCC only. See below for variants designed to build to fat binaries
OSX_ICCLIB_EXPFILE	=  exports/icclib_osx.def
# NOTE, the next line has a SPACE after exported_symbols_list
# It MUST stay there
OSX_EXPORT_FLAG         =  -Wl,-exported_symbols_list 
# -Wl,"-exported_symbols_list exports/icclib_osx.exp"
#-Wl,--version-script,
OSX_ICCLIB_FLAGS        = -init _ICCLoad -all_load
#  exports/icclib_linux.exp
OSX_ICCDLL_NAME         = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
OSX_OSSLDLL_NAME	= libcrypto.$(OPENSSL_LIBVER).dylib
# What all the fuss is about, all this just to rename the library...
OSX_OSSLINC_DIR		= $(OSSL_DIR)/include
OSX_OSSLOBJ_DIR		= $(OSSL_DIR)
OSX_OPENSSL		= $(OSSL_DIR)/apps/openssl
OSX_OPENSSL_PATH_SETUP 	= export DYLD_LIBRARY_PATH=$(OSSL_DIR);
OSX_BUILD_OSSL		=
OSX_CLEAN_OSSL       	= $(LINUX_CLEAN_OSSL)
OSX_TARCMD		= tar -cvhf
OSX_TEST_CMD         	= $(LINUX_TEST_CMD)
OSX_ASMMAK 		= ./dummy.mk
OSX_EXTRAS           	= $(LINUX_EXTRAS)


#
# Mac OS/X i386 cross compiled binary variant
#
OSX_X86_EXPORT_FLAG	= $(OSX_EXPORT_FLAG)
OSX_X86_ICCLIB_EXPFILE	= $(OSX_ICCLIB_EXPFILE)
OSX_X86_ICCLIB_FLAGS    = $(OSX_ICCLIB_FLAGS)
#  exports/icclib_linux.exp
OSX_X86_ICCDLL_NAME     = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
OSX_X86_OSSLDLL_NAME	= $(OSX_OSSLDLL_NAME)
OSX_X86_OSSLINC_DIR	= $(OSX_OSSLINC_DIR)
OSX_X86_OSSLOBJ_DIR	= $(OSX_OSSLOBJ_DIR)
# Can't always run the x86 version we just built, so use the one on the system
OSX_X86_OPENSSL		= openssl
OSX_X86_OPENSSL_PATH_SETUP = $(OSX_OPENSSL_PATH_SETUP)
OSX_X86_BUILD_OSSL	= cd $(OSSL_DIR) ; ./Configure threads shared $(OSSL_FLAGS) darwin-i386-cc; make depend ; make
OSX_X86_CLEAN_OSSL	= $(OSX_CLEAN_OSSL)
OSX_X86_TARCMD		= $(OSX_TARCMD) 
OSX_X86_TEST_CMD 	= $(OSX_TEST_CMD)
OSX_X86_ASMMAK		= $(OSX_ASMMAK)
OSX_X86_EXTRAS		= $(OSX_EXTRAS)

#
# Mac OS/X 10.5+ x86_64 cross compiled binary variant
#
OSX_X86_64_EXPORT_FLAG          = $(OSX_EXPORT_FLAG)
OSX_X86_64_ICCLIB_EXPFILE	= $(OSX_ICCLIB_EXPFILE)
OSX_X86_64_ICCLIB_FLAGS    = $(OSX_ICCLIB_FLAGS)
OSX_X86_64_ICCDLL_NAME          = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
OSX_X86_64_OSSLDLL_NAME         = $(OSX_OSSLDLL_NAME)
OSX_X86_64_OSSLINC_DIR          = $(OSX_OSSLINC_DIR)
OSX_X86_64_OSSLOBJ_DIR          = $(OSX_OSSLOBJ_DIR)
OSX_X86_64_OPENSSL              = openssl
OSX_X86_64_OPENSSL_PATH_SETUP   = $(OSX_OPENSSL_PATH_SETUP)
OSX_X86_64_BUILD_OSSL           = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) darwin64-x86_64-cc; make depend ; make
OSX_X86_64_CLEAN_OSSL           = $(OSX_CLEAN_OSSL)
OSX_X86_64_TARCMD               = $(OSX_TARCMD)
OSX_X86_64_TEST_CMD             = $(OSX_TEST_CMD)
OSX_X86_64_ASMMAK               = $(OSX_ASMMAK)
OSX_X86_64_EXTRAS						= $(OSX_EXTRAS)


#
# We call the main Makefile from MakefileObeseMac to build some comonents
# as "FAT4"
#

OSX_FAT4_EXPORT_FLAG		= $(OSX_EXPORT_FLAG)
OSX_FAT4_ICCLIB_EXPFILE		= $(OSX_ICCLIB_EXPFILE)
OSX_FAT4_ICCLIB_FLAGS        	= $(OSX_ICCLIB_FLAGS)
OSX_FAT4_ICCDLL_NAME            = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
OSX_FAT4_OSSLDLL_NAME		= $(OSX_OSSLDLL_NAME)
OSX_FAT4_OSSLINC_DIR		= $(OSX_OSSLINC_DIR)
OSX_FAT4_OSSLOBJ_DIR		=
OSX_FAT4_OPENSSL		= ./openssl
OSX_FAT4_OPENSSL_PATH_SETUP 	= 
OSX_FAT4_BUILD_OSSL		=
OSX_FAT4_CLEAN_OSSL       	= 
OSX_FAT4_TARCMD			= $(OSX_TARCMD)
OSX_FAT4_TEST_CMD         	= $(OSX_TEST_CMD)
OSX_FAT4_ASMMAK 		= $(OSX_ASMMAK)
OSX_FAT4_EXTRAS           	= $(OSX_EXTRAS)

#
# Mac OS/X 10.5+ x86_64 cross compiled binary variant
#
OSXV9_EXPORT_FLAG          = $(OSX_EXPORT_FLAG)
OSXV9_ICCLIB_EXPFILE	   = $(OSX_ICCLIB_EXPFILE)
OSXV9_ICCLIB_FLAGS    		= $(OSX_ICCLIB_FLAGS)
OSXV9_ICCDLL_NAME          = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
OSXV9_OSSLDLL_NAME         = $(OSX_OSSLDLL_NAME)
OSXV9_OSSLINC_DIR          = $(OSX_OSSLINC_DIR)
OSXV9_OSSLOBJ_DIR          = $(OSX_OSSLOBJ_DIR)
OSXV9_OPENSSL              = openssl
OSXV9_OPENSSL_PATH_SETUP   = $(OSX_OPENSSL_PATH_SETUP)
OSXV9_BUILD_OSSL           = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) darwin64-x86_64-cc; make depend ; make
OSXV9_CLEAN_OSSL           = $(OSX_CLEAN_OSSL)
OSXV9_TARCMD               = $(OSX_TARCMD)
OSXV9_TEST_CMD             = $(OSX_TEST_CMD)
OSXV9_ASMMAK               = $(OSX_ASMMAK)
OSXV9_EXTRAS					= $(OSX_EXTRAS)

#
# Mac M1 OS/X ARM64
#
OSX_ARM64_EXPORT_FLAG          = $(OSX_EXPORT_FLAG)
OSX_ARM64_ICCLIB_EXPFILE	    = $(OSX_ICCLIB_EXPFILE)
OSX_ARM64_ICCLIB_FLAGS         = $(OSX_ICCLIB_FLAGS)
OSX_ARM64_ICCDLL_NAME          = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
OSX_ARM64_OSSLDLL_NAME         = $(OSX_OSSLDLL_NAME)
OSX_ARM64_OSSLINC_DIR          = $(OSX_OSSLINC_DIR)
OSX_ARM64_OSSLOBJ_DIR          = $(OSX_OSSLOBJ_DIR)
OSX_ARM64_OPENSSL              = openssl
OSX_ARM64_OPENSSL_PATH_SETUP   = $(OSX_OPENSSL_PATH_SETUP)
OSX_ARM64_BUILD_OSSL           = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) darwin64-arm64-cc; make depend ; make
OSX_ARM64_CLEAN_OSSL           = $(OSX_CLEAN_OSSL)
OSX_ARM64_TARCMD               = $(OSX_TARCMD)
OSX_ARM64_TEST_CMD             = $(OSX_TEST_CMD)
OSX_ARM64_ASMMAK               = $(OSX_ASMMAK)
OSX_ARM64_EXTRAS				= $(OSX_EXTRAS)


S390_LINUX_EXPORT_FLAG    = $(LINUX_EXPORT_FLAG)
S390_LINUX_ICCLIB_EXPFILE = $(LINUX_ICCLIB_EXPFILE)
S390_LINUX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
S390_LINUX_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
S390_LINUX_OSSLINC_DIR    = $(OSSL_DIR)/include
S390_LINUX_OSSLOBJ_DIR    = $(OSSL_DIR)
S390_LINUX_OPENSSL        = $(OSSL_DIR)/apps/openssl
S390_LINUX_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(S390_LINUX_$(CONFIG)_CFLAGS) linux32-s390x; make depend; make
S390_LINUX_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
S390_LINUX_TARCMD	  = tar -cvhf
S390_LINUX_TEST_CMD       = $(LINUX_TEST_CMD)
S390_LINUX_EXTRAS         = $(LINUX_EXTRAS)


S390X_LINUX_EXPORT_FLAG    = $(LINUX_EXPORT_FLAG)
S390X_LINUX_ICCLIB_EXPFILE = $(LINUX_ICCLIB_EXPFILE)
S390X_LINUX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
S390X_LINUX_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
S390X_LINUX_OSSLINC_DIR    = $(OSSL_DIR)/include
S390X_LINUX_OSSLOBJ_DIR    = $(OSSL_DIR)
# HACK alert, for a cross compiler, you need a native openssl
S390X_LINUX_OPENSSL        = $(OSSL_DIR)/apps/openssl
S390X_LINUX_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(S390X_LINUX_$(CONFIG)_CFLAGS)  linux64-s390x; make depend; make
S390X_LINUX_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
S390X_LINUX_TARCMD         = tar -cvhf
S390X_LINUX_TEST_CMD       = $(LINUX_TEST_CMD)
S390X_LINUX_EXTRAS         = $(LINUX_EXTRAS)


# z/OS 64 bit config

#
ZOS_EXPORT_FLAG    = -Wc,dll,exportall$(CXPLINK) -Wl,dll$(CXPLINK)
ZOS_ICCLIB_EXPFILE = 
ZOS_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
ZOS_OSSLDLL_NAME   = libcrypto.$(OPENSSL_LIBVER).dll
ZOS_OSSLINC_DIR    = $(OSSL_DIR)/include
ZOS_OSSLOBJ_DIR    = $(OSSL_DIR)
ZOS_OPENSSL        = $(OSSL_DIR)/apps/openssl
ZOS_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared  $(OSSL_FLAGS) -Wc,exportall OS390-Unix; make depend; make
ZOS_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
ZOS_TARCMD	  = pax -wLvf
ZOS_TEST_CMD       = $(LINUX_TEST_CMD)
ZOS_EXTRAS         = $(LINUX_EXTRAS)

# z/OS 64 bit ASCII
ZOSA_EXPORT_FLAG    = -Wc,dll,exportall$(CXPLINK) -Wl,dll$(CXPLINK)
ZOSA_ICCLIB_EXPFILE = 
ZOSA_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
ZOSA_OSSLDLL_NAME   = libcrypto.$(OPENSSL_LIBVER).dll
ZOSA_OSSLINC_DIR    = $(OSSL_DIR)/include
ZOSA_OSSLOBJ_DIR    = $(OSSL_DIR)
ZOSA_OPENSSL        = $(OSSL_DIR)/apps/openssl
ZOSA_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared  $(OSSL_FLAGS) -Wc,exportall OS390-ASCII; make depend; make
ZOSA_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
ZOSA_TARCMD	  = pax -wLvf
ZOSA_TEST_CMD       = $(LINUX_TEST_CMD)
ZOSA_EXTRAS         = $(LINUX_EXTRAS)
# we need the semicolon in here because other platforms have no CHTAG_CMD definition
# which mucks up the sh command syntax
ZOSA_CHTAG_CMD      = chtag -tc ISO8859-1 ../icc/ICC_ver.txt ;

#
# z/OS 31 bit config
#
ZOS31_EXPORT_FLAG    = -Wc,dll,exportall$(CXPLINK) -Wl,dll$(CXPLINK)
ZOS31_ICCLIB_EXPFILE = 
ZOS31_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
ZOS31_OSSLDLL_NAME   = libcrypto.$(OPENSSL_LIBVER).dll
ZOS31_OSSLINC_DIR    = $(OSSL_DIR)/include
ZOS31_OSSLOBJ_DIR    = $(OSSL_DIR)
ZOS31_OPENSSL        = $(OSSL_DIR)/apps/openssl
ZOS31_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared  $(OSSL_FLAGS) -Wc,exportall OS390-Unix31; make depend; make
ZOS31_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
ZOS31_TARCMD	  = pax -wLvf
ZOS31_TEST_CMD       = $(LINUX_TEST_CMD)
ZOS31_EXTRAS         = $(LINUX_EXTRAS)

#
# z/OS 31 bit ASCII config
#
ZOSA31_EXPORT_FLAG    = -Wc,dll,exportall$(CXPLINK) -Wl,dll$(CXPLINK)
ZOSA31_ICCLIB_EXPFILE = 
ZOSA31_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
ZOSA31_OSSLDLL_NAME   = libcrypto.$(OPENSSL_LIBVER).dll
ZOSA31_OSSLINC_DIR    = $(OSSL_DIR)/include
ZOSA31_OSSLOBJ_DIR    = $(OSSL_DIR)
ZOSA31_OPENSSL        = $(OSSL_DIR)/apps/openssl
ZOSA31_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared  $(OSSL_FLAGS) -Wc,exportall OS390-ASCII31; make depend; make
ZOSA31_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
ZOSA31_TARCMD	  = pax -wLvf
ZOSA31_TEST_CMD       = $(LINUX_TEST_CMD)
ZOSA31_EXTRAS         = $(LINUX_EXTRAS)
ZOSA31_CHTAG_CMD      = chtag -tc ISO8859-1 ../icc/ICC_ver.txt ;

AIX_EXPORT_FLAG    = -bexport:
AIX_ICCLIB_EXPFILE = exports/icclib_aix.exp
AIX_OSSLLIB_NAME   = libcrypto
AIX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
AIX_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
AIX_OSSLINC_DIR    = $(OSSL_DIR)/include
AIX_OSSLOBJ_DIR    = $(OSSL_DIR)
AIX_OPENSSL        = $(OSSL_DIR)/apps/openssl
AIX_OPENSSL_PATH_SETUP = export LIBPATH=$(OSSL_DIR);
AIX_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(AIX_$(CONFIG)_CFLAGS) aix-cc; make depend; make
AIX_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
AIX_TARCMD 	   = tar -cvhf
AIX_TEST_CMD       = echo "OpenSSL Tests disabled because of hangs"
AIX_ASM_TWEAKS     = -DAIX
AIX_ASMOBJS        = rng-ppc.o
AIX_EXTRAS         = $(LINUX_EXTRAS)

AIX64_EXPORT_FLAG    = -bexport:
AIX64_ICCLIB_EXPFILE = $(AIX_ICCLIB_EXPFILE) 
AIX64_OSSLLIB_NAME   = libcrypto
AIX64_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
AIX64_OSSLDLL_NAME   = libcrypto64.so.$(OPENSSL_LIBVER)
AIX64_OSSLINC_DIR    = $(OSSL_DIR)/include
AIX64_OSSLOBJ_DIR    = $(OSSL_DIR)
AIX64_OPENSSL        = $(OSSL_DIR)/apps/openssl
AIX64_OPENSSL_PATH_SETUP = export LIBPATH=$(OSSL_DIR);
AIX64_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(AIX64_$(CONFIG)_CFLAGS) aix64-cc; make depend; make
AIX64_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
AIX64_TARCMD 	     = tar -cvhf
AIX64_TEST_CMD       = $(AIX_TEST_CMD)
AIX64_ASM_TWEAKS     = -DAIX
AIX64_ASMOBJS        = rng-ppc.o
AIX64_EXTRAS         = $(LINUX_EXTRAS)


POWERH_EXPORT_FLAG    = -bexport:
POWERH_ICCLIB_EXPFILE = $(AIX_ICCLIB_EXPFILE) 
POWERH_OSSLLIB_NAME   = libcrypto
POWERH_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
POWERH_OSSLDLL_NAME   = $(AIX_OSSLDLL_NAME)
POWERH_OSSLINC_DIR    = $(OSSL_DIR)/include
POWERH_OSSLOBJ_DIR    = $(OSSL_DIR)
POWERH_OPENSSL        = $(OSSL_DIR)/apps/openssl
POWERH_OPENSSL_PATH_SETUP = export LIBPATH=$(OSSL_DIR);
POWERH_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(POWERH_$(CONFIG)_CFLAGS) aix64-cc; make depend; make
POWERH_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
POWERH_TARCMD 	     = tar -cvhf
POWERH_TEST_CMD       = $(LINUX_TEST_CMD)
POWERH_ASM_TWEAKS     = -DAIX
POWERH_ASMOBJS        = rng-ppc.o
POWERH_EXTRAS         = $(LINUX_EXTRAS)

SUN_EXPORT_FLAG    = -Wl,-M
SUN_ICCLIB_EXPFILE = exports/icclib_sun.exp
SUN_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
SUN_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
SUN_OSSLINC_DIR    = $(OSSL_DIR)/include
SUN_OSSLOBJ_DIR    = $(OSSL_DIR)
SUN_OPENSSL        = $(OSSL_DIR)/apps/openssl
SUN_OPENSSL_PATH_SETUP = set LD_LIBRARY_PATH=$(OSSL_DIR);export LD_LIBRARY_PATH;
SUN_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(SUN_$(CONFIG)_CFLAGS) -R$(OSSLOBJ_DIR) solaris-sparcv9-cc; make depend;  make
SUN_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
SUN_TARCMD 	   = tar -cvhf
SUN_TEST_CMD       = $(LINUX_TEST_CMD)
SUN_ASM_TWEAKS     = -DSOL8_FAST_TICK 
SUN_ASMOBJS        = RdCTR_raw.o
SUN_EXTRAS         = $(LINUX_EXTRAS)

# Solaris x86 (32 bit)

SUN_X86_EXPORT_FLAG    = $(SUN_EXPORT_FLAG)
SUN_X86_ICCLIB_EXPFILE = $(SUN_ICCLIB_EXPFILE)
SUN_X86_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
SUN_X86_OSSLDLL_NAME   = $(SUN_OSSLDLL_NAME)
SUN_X86_OSSLINC_DIR    = $(OSSL_DIR)/include
SUN_X86_OSSLOBJ_DIR    = $(OSSL_DIR)
SUN_X86_OPENSSL        = $(OSSL_DIR)/apps/openssl
SUN_X86_OPENSSL_PATH_SETUP = $(SUN_OPENSSL_PATH_SETUP)
SUN_X86_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) solaris-x86-gcc; make depend; make
SUN_X86_CLEAN_OSSL     = $(SUN_CLEAN_OSSL)
SUN_X86_TARCMD 	       = tar -cvhf
SUN_X86_TEST_CMD       = $(SUN_TEST_CMD)
SUN_X86_ASM_TWEAKS     =
SUN_X86_EXTRAS         = $(LINUX_EXTRAS)

SUN_SOL8_EXPORT_FLAG    = $(SUN_EXPORT_FLAG)
SUN_SOL8_ICCLIB_EXPFILE = $(SUN_ICCLIB_EXPFILE)
SUN_SOL8_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
SUN_SOL8_OSSLDLL_NAME   = $(SUN_OSSLDLL_NAME)
SUN_SOL8_OSSLINC_DIR    = $(SUN_OSSLINC_DIR)
SUN_SOL8_OSSLOBJ_DIR    = $(SUN_OSSLOBJ_DIR)
SUN_SOL8_OPENSSL        = $(SUN_OPENSSL)
SUN_SOL8_OPENSSL_PATH_SETUP = $(SUN_OPENSSL_PATH_SETUP)
SUN_SOL8_BUILD_OSSL     = $(SUN_BUILD_OSSL)
SUN_SOL8_CLEAN_OSSL     = $(SUN_CLEAN_OSSL)
SUN_SOL8_TARCMD 	= $(SUN_TARCMD)
SUN_SOL8_TEST_CMD       = $(SUN_TEST_CMD)
# inline assembler to read the internal CPU cycle counter
SUN_SOL8_ASM_TWEAKS     = -DSOL8_FAST_TICK 
SUN_SOL8_ASMOBJS        = RdCTR_raw.o
SUN_SOL8_EXTRAS         = $(LINUX_EXTRAS)


SUN64_EXPORT_FLAG    = $(SUN_EXPORT_FLAG)
SUN64_ICCLIB_EXPFILE = $(SUN_ICCLIB_EXPFILE)
SUN64_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
SUN64_OSSLDLL_NAME   = $(SUN_OSSLDLL_NAME)
SUN64_OSSLINC_DIR    = $(OSSL_DIR)/include
SUN64_OSSLOBJ_DIR    = $(OSSL_DIR)
SUN64_OPENSSL        = $(OSSL_DIR)/apps/openssl
SUN64_OPENSSL_PATH_SETUP = $(SUN_OPENSSL_PATH_SETUP)
SUN64_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(SUN_$(CONFIG)_CFLAGS) -R$(OSSLOBJ_DIR) solaris64-sparcv9-cc; make depend;  make
SUN64_CLEAN_OSSL     = $(SUN_CLEAN_OSSL)
SUN64_TARCMD 	     = tar -cvhf
SUN64_TEST_CMD       =  $(SUN_TEST_CMD)
SUN64_ASM_TWEAKS     = -DSOL8_FAST_TICK 
SUN64_ASMOBJS        = RdCTR_raw.o
SUN64_EXTRAS         = $(LINUX_EXTRAS)

SUN_AMD64_EXPORT_FLAG    =  $(SUN_EXPORT_FLAG)
SUN_AMD64_ICCLIB_EXPFILE = $(SUN_ICCLIB_EXPFILE)
SUN_AMD64_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
SUN_AMD64_OSSLDLL_NAME   = $(SUN_OSSLDLL_NAME)
SUN_AMD64_OSSLINC_DIR    = $(OSSL_DIR)/include
SUN_AMD64_OSSLOBJ_DIR    = $(OSSL_DIR)
SUN_AMD64_OPENSSL        = $(OSSL_DIR)/apps/openssl
SUN_AMD64_OPENSSL_PATH_SETUP = $(SUN_OPENSSL_PATH_SETUP)
# Updated to use gcc to build OpenSSL (only) so x25519 compiles properly
SUN_AMD64_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS)  -R$(OSSLOBJ_DIR) solaris64-x86_64-gcc; make depend;  make
#SUN_AMD64_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(SUN_$(CONFIG)_CFLAGS) -R$(OSSLOBJ_DIR) solaris64-x86_64-cc; make depend;  make
SUN_AMD64_CLEAN_OSSL     = $(SUN_CLEAN_OSSL)
SUN_AMD64_TARCMD 	 = tar -cvhf
SUN_AMD64_TEST_CMD       = $(SUN_TEST_CMD)
SUN_AMD64_ASM_TWEAKS     = 
SUN_AMD64_EXTRAS         = $(LINUX_EXTRAS)

# HPUX - defaults to PA-RISC 2 (for performance)
HPUX_EXPORT_FLAG    = -c
HPUX_ICCLIB_EXPFILE = exports/icclib_hpux.exp
HPUX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
HPUX_OSSLDLL_NAME   = libcrypto.sl.$(OPENSSL_LIBVER)
HPUX_OSSLINC_DIR    = $(OSSL_DIR)/include
HPUX_OSSLOBJ_DIR    = $(OSSL_DIR)
HPUX_OPENSSL        = $(SDK_DIR)/openssl
HPUX_OPENSSL_PATH_SETUP = export LD_LIBRARY_PATH=$(OSSL_DIR);
HPUX_BUILD_OSSL        = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(HPUX_$(CONFIG)_CFLAGS) hpux-parisc1_1-cc; make depend;  make
HPUX_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
HPUX_TARCMD 	    = tar -cvhf
HPUX_TEST_CMD       = $(LINUX_TEST_CMD)
HPUX_EXTRAS         = $(LINUX_EXTRAS)



HPUX64_EXPORT_FLAG    = -c
HPUX64_ICCLIB_EXPFILE = $(HPUX_ICCLIB_EXPFILE)
HPUX64_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
HPUX64_OSSLDLL_NAME   = $(HPUX_OSSLDLL_NAME)
HPUX64_OSSLINC_DIR    = $(HPUX_OSSLINC_DIR)
HPUX64_OSSLOBJ_DIR    = $(HPUX_OSSLOBJ_DIR)
HPUX64_OPENSSL        = $(HPUX_OPENSSL)
HPUX64_OPENSSL_PATH_SETUP = $(HPUX_OPENSSL_PATH_SETUP)
HPUX64_BUILD_OSSL     = cd $(OSSL_DIR);  ./Configure threads shared $(OSSL_FLAGS) $(HPUX64_$(CONFIG)_CFLAGS) hpux64-parisc2-cc; make depend;  make
HPUX64_CLEAN_OSSL     = $(HPUX_CLEAN_OSSL)
HPUX64_TARCMD         = $(HPUX_TARCMD)
HPUX64_TEST_CMD       = $(HPUX_TEST_CMD)
HPUX64_EXTRAS         = $(LINUX_EXTRAS)


#--- 32-bit GNU CC (aCC) objects on HP-UX IA64
#
IA64_HPUX_EXPORT_FLAG    = -c
IA64_HPUX_ICCLIB_EXPFILE = $(HPUX_ICCLIB_EXPFILE)
# Yes, it *IS* different from the pa-risc shared library name
IA64_HPUX_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
IA64_HPUX_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
IA64_HPUX_OSSLINC_DIR    = $(OSSL_DIR)/include
IA64_HPUX_OSSLOBJ_DIR    = $(OSSL_DIR)
IA64_HPUX_OPENSSL        = $(OSSL_DIR)/apps/openssl
IA64_HPUX_OPENSSL_PATH_SETUP = export LD_LIBRARY_PATH=$(OSSL_DIR);
IA64_HPUX_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(IA64_HPUX_$(CONFIG)_CFLAGS) hpux-ia64-cc; make depend;  make
IA64_HPUX_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
IA64_HPUX_TARCMD         = tar -cvhf
IA64_HPUX_TEST_CMD       = $(LINUX_TEST_CMD)
IA64_HPUX_EXTRAS         = $(LINUX_EXTRAS)


#--- 64-bit native aCC objects on HP-UX IA64
#
IA64_HPUX64_EXPORT_FLAG    = -c
IA64_HPUX64_ICCLIB_EXPFILE = $(HPUX_ICCLIB_EXPFILE)
IA64_HPUX64_ICCDLL_NAME    = $(SHLPRFX)icclib$(VTAG)$(SHLSUFX)
IA64_HPUX64_OSSLDLL_NAME   = $(LINUX_OSSLDLL_NAME)
IA64_HPUX64_OSSLINC_DIR    = $(OSSL_DIR)/include
IA64_HPUX64_OSSLOBJ_DIR    = $(OSSL_DIR)
IA64_HPUX64_OPENSSL        = $(OSSL_DIR)/apps/openssl
IA64_HPUX64_OPENSSL_PATH_SETUP = export SHLIB_PATH=$(OSSL_DIR):/usr/lib;
IA64_HPUX64_BUILD_OSSL     = cd $(OSSL_DIR); ./Configure threads shared $(OSSL_FLAGS) $(IA64_HPUX64_$(CONFIG)_CFLAGS) hpux64-ia64-cc; make depend;  make
IA64_HPUX64_CLEAN_OSSL     = $(LINUX_CLEAN_OSSL)
IA64_HPUX64_TARCMD         = tar -cvhf
IA64_HPUX64_TEST_CMD       = $(LINUX_TEST_CMD)
IA64_HPUX64_EXTRAS         = $(LINUX_EXTRAS)

# i5/OS (native compiler - not maintained)
# This platform has no dlopen/dlsym
OS400_OSSL_OUTPUTDIR_PATH = /qsys.lib/$(OS400_OSSL_OUTPUTDIR).lib
OS400_ICC_OUTPUTDIR_PATH = /qsys.lib/$(OS400_ICC_OUTPUTDIR).lib
OS400_ICC_LIB_PATH = /qsys.lib/$(OS400_ICC_LIB).lib
OS400_ICCSDK_OUTPUTDIR_PATH = /qsys.lib/$(OS400_ICCSDK_OUTPUTDIR).lib
OS400_ICCTEST_OUTPUTDIR_PATH = /qsys.lib/$(OS400_ICCTEST_OUTPUTDIR).lib
OS400_ICCSHL_OUTPUTDIR_PATH = /qsys.lib/$(OS400_ICCSHL_OUTPUTDIR).lib
OS400_ICCPKG_OUTPUTDIR_PATH = /qsys.lib/$(OS400_ICCPKG_OUTPUTDIR).lib
OS400_README_PATH	= /qsys.lib/$(OS400_ICCPKG_OUTPUTDIR).lib/readme.file/readme.mbr
OS400_SETUP_OSSL	= export OUTPUTDIR=$(OS400_OSSL_OUTPUTDIR)
OS400_SETUP_ICC		= export OUTPUTDIR=$(OS400_ICC_OUTPUTDIR)
OS400_EXPORT_LIBRARYPATH = /qsys.lib/$(OS400_ICC_OUTPUTDIR).lib
OS400_EXPORT_FILE	= QSRVSRC
OS400_EXPORT_FILEPATH	= $(OS400_EXPORT_LIBRARYPATH)/$(OS400_EXPORT_FILE).FILE
OS400_EXPORT_MEMBERPATH = $(OS400_EXPORT_FILEPATH)/LIBICCLIB.MBR
OS400_EXPORT_MEMBER  	= LIBICCLIB
OS400_EXPORT_FLAG	= -x -qSRCFILE=$(OS400_ICC_OUTPUTDIR)/$(OS400_EXPORT_FILE) -qSRCMBR=$(OS400_EXPORT_MEMBER)
OS400_OSSLDLL_NAME   	= libcrypto.so.$(OPENSSL_LIBVER)
OS400_OSSLINC_DIR    	= $(OSSL_DIR)/include 
OS400_OSSLOBJ_DIR    	= $(OSSL_DIR)
OS400_OPENSSL        	= $(OSSL_DIR)/apps/openssl
# semicolon deliberate
OS400_OPENSSL_PATH_SETUP = export LD_LIBRARY_PATH=$(OSSL_DIR); 
OS400_BUILD_OSSL   	  = $(OS400_SETUP_OSSL); cd $(OSSL_DIR); ./Configure $(CONFIG)-OS400 threads shared $(OSSL_FLAGS); make depend; make
OS400_CLEAN_OSSL     	= $(OS400_SETUP_OSSL); cd $(OSSL_DIR); ./Configure $(CONFIG)-OS400 threads shared $(OSSL_FLAGS);  make clean-os400 clean-shared; $(RM) $(OSSL_DIR)/Makefile
OS400_OSSLLIB_NAME   	= libcrypto.so.$(OPENSSL_LIBVER)
OS400_ICCDLL_NAME 	= $(SHLPRFX)icclib$(SHLSUFX)
OS400_TARCMD		= tar -cvf
OS400_TEST_CMD      	= ./icctest; $(OS400_SETUP_ICC); cd $(OSSL_DIR);make tests;apps/openssl speed;cd  ../icc
OS400_ICCLIB_FLAGS 	= 
OS400_ICCSPEED_OUTPUTDIR_PATH = /qsys.lib/$(OS400_ICCSPEED_OUTPUTDIR).lib
OS400_SETUP_ICCSPEED    = if [ ! -d $(OS400_ICCSPEED_OUTPUTDIR_PATH) ]; then mkdir $(OS400_ICCSPEED_OUTPUTDIR_PATH); fi; export -s OUTPUTDIR=$(OS400_ICCSPEED_OUTPUTDIR);
OS400_SETUP_SHL		= export -s OUTPUTDIR=$(OS400_ICCSHL_OUTPUTDIR)
OS400_CLEAN_PERF        = system clrlib $(OS400_ICCSPEED_OUTPUTDIR)
OS400_CLEAN_ICC         = system clrlib $(OS400_ICCSPEED_OUTPUTDIR); system clrlib $(OS400_ICCSDK_OUTPUTDIR); system clrlib $(OS400_ICC_OUTPUTDIR); system clrlib $(OS400_ICCPKG_OUTPUTDIR); system clrlib $(OS400_ICC_LIB)
OS400_OBJS	        = getnmi.o
OS400_EXTRAS            =


# i5/OS (built with an AIX hosted cross compiler)
OS400X_EXPORT_FLAG    =
OS400X_ICCLIB_EXPFILE = exports/icclib_os400.exp
OS400X_ICCDLL_NAME    = $(SHLPRFX)icclib$(SHLSUFX)
# note: OSSL_HOSTBIN and ICC_HOSTBIN are the same, but they could be defined differently
OS400X_release_OSSL_HOSTBIN   = $(OS400X_release_ICC_HOSTBIN)
OS400X_debug_OSSL_HOSTBIN     = $(OS400X_debug_ICC_HOSTBIN)
OS400X_CLEAN400_OSSL  = rmlib400x -s $(HOSTSYS) $(OS400X_$(CONFIG)_OSSL_HOSTBIN)
OS400X_OSSLLIB_NAME   = libcrypto
OS400X_OSSLDLL_NAME   = libcrypto.so
OS400X_OSSLINC_DIR    = $(OSSL_DIR)/include
OS400X_OSSLOBJ_DIR    = $(OSSL_DIR)
OS400X_OPENSSL        = $(OSSL_DIR)/apps/openssl
##OS400X_OPENSSL_PATH_SETUP = export LD_LIBRARY_PATH=$(OSSL_DIR);
#--------------------------------------------------------
# need to create dummy version of this header if we are not running 'make depend' in openssl, because
# Makefiles come with dependencies that require this file, which is not generated when using the 'no_idea" option
# OS400X_SSL_HACK       = touch include/openssl/idea.h
OS400X_SSL_HACK = echo no_hack
#--------------------------------------------------------
# version when NOT using extracted source in Clearcase
OS400X_BUILD_OSSL_     = cd $(OSSL_DIR); export HOSTBINOPT="-H $(OS400X_$(CONFIG)_OSSL_HOSTBIN)"; ./Configure threads shared $(OSSL_FLAGS) $(OS400X_$(CONFIG)_CFLAGS) os400x; "$(MAKE)" depend; $(OS400X_SSL_HACK); "$(MAKE)" build_all
#--------------------------------------------------------
# version when using extracted source in Clearcase
# need to copy template files
OS400X_TMPL_FILES = crypto/objects/obj_mac.num
OS400X_BUILD_OSSL__ex   = cd $(OSSL_DIR); export HOSTBINOPT="-H $(OS400X_$(CONFIG)_OSSL_HOSTBIN)"; if [ ! -r Makefile ]; then for file in $(OS400X_TMPL_FILES); do cp -p $${file}.tmpl $${file}; chmod u+w $${file}; done;  ./Configure threads shared $(OSSL_FLAGS) $(OS400X_$(CONFIG)_CFLAGS) os400x; $(OS400X_SSL_HACK); fi; "$(MAKE)" build_all
#--------------------------------------------------------
OS400X_BUILD_OSSL = $(OS400X_BUILD_OSSL_$(EX_SUFFIX))
#--------------------------------------------------------
OS400X_CLEAN_OSSL     	= cd $(OSSL_DIR); "$(MAKE)" clean clean-shared; $(RM) $(OSSL_DIR)/Makefile
OS400X_TARCMD 	      	= tar -cvhf
OS400X_TEST_CMD     	=  ./icctest; cd $(OSSL_DIR); export HOSTBINOPT="-H $(OS400X_$(CONFIG)_OSSL_HOSTBIN)"; "$(MAKE)" tests; apps/openssl speed; cd  ../icc
OS400X_ASM_TWEAKS     	=
OS400X_ASMOBJS        	=
OS400X_EXTRAS         	=


#
# Bring it together...
#

EXPORT_FLAG     = $($(OPSYS)_EXPORT_FLAG)
ICCLIB_EXPFILE  = $($(OPSYS)_ICCLIB_EXPFILE)
ICCDLL_NAME     = $($(OPSYS)_ICCDLL_NAME)
OSSLLIB_NAME    = $($(OPSYS)_OSSLLIB_NAME)
# Name of the DLL built by OpenSSL
OSSLDLL_NAME    = $($(OPSYS)_OSSLDLL_NAME)
# Name of the DLL used by ICC. (on OS/X at least we have to rename it)
# Most OS's it stays the same as the native OpenSSL name
OSSLINC_DIR     = $($(OPSYS)_OSSLINC_DIR)
OSSLOBJ_DIR     = $($(OPSYS)_OSSLOBJ_DIR)
OPENSSL         = $($(OPSYS)_OPENSSL)
OPENSSL_PATH_SETUP = $($(OPSYS)_OPENSSL_PATH_SETUP)
BUILD_OSSL      = $($(OPSYS)_BUILD_OSSL)
CLEAN_OSSL      = $($(OPSYS)_CLEAN_OSSL)
TARCMD 		= $($(OPSYS)_TARCMD)
TEST_CMD        = $($(OPSYS)_TEST_CMD)
# The next lines cater for ugly bits of inline assembler needed
# to get the CPU cycle counter on some OS's
# It's handled different on different OS's as well (UGLY !!)
ASM_TWEAKS      = $($(OPSYS)_ASM_TWEAKS)
ASMOBJS         = $($(OPSYS)_ASMOBJS)
ICCLIB_FLAGS 	= $($(OPSYS)_ICCLIB_FLAGS)
SETUP_ICC	= $($(OPSYS)_SETUP_ICC)
SETUP_ICCSPEED	= $($(OPSYS)_SETUP_ICCSPEED)
SETUP_SHL	= $($(OPSYS)_SETUP_SHL)
EXTRAS	 	= $($(OPSYS)_EXTRAS)
MANIFESTS       = $($(OPSYS)_MANIFESTS)
CHTAG_CMD       = $($(OPSYS)_CHTAG_CMD)
