#Linux/unix systems need LD_LIBRARY_PATH setup to run the SSL tests
#Linux/unix systems need icc symlink to run the ICC tests
#(TODO use ICC_ROOT define for Linux so can remove icc symlink)
#runtime uses symlink for icc
#Windows systems need libeay32.dll in PATH or current directory

# NOTE: This file should contain NO references to directories in the
#       build system. It's ALL generic system stuff

# CLFAGS2/LDLFAGS2/SLDFLAGS2 are to cater with cross complied systems
# where we need to build some native executables, OS400, OSX both
# exploit this

# these are the same on all platforms
$(OPSYS)_RM = rm -f
$(OPSYS)_CP = cp -f
$(OPSYS)_MKDIR = [ -d $@ ] || mkdir -p

#--- VisualC++ definitions on Win32
# cant use symlinks so define absolute location of ICC ie parent of package directory
DEFAULT_DEBUGGER       = msdev
DEFAULT_CC             = cl
DEFAULT_CXX             = cl -TP
DEFAULT_debug_CFLAGS   = -MDd -Zi -D DEBUG -RTCu
# Optimization off generates faster starting code ...
DEFAULT_release_CFLAGS = -MD -Zi
DEFAULT_CFLAGS         = -nologo $($(OPSYS)_$(CONFIG)_CFLAGS) -W3 -GF  -GS -D WIN32 -D $(OPSYS) -D _MBCS -D_CRT_SECURE_NO_WARNINGS -c 
DEFAULT_CXXFLAGS       = $($(OPSYS)_CFLAGS)
DEFAULT_LD             = link
DEFAULT_LD_CXX         = link
DEFAULT_SLD            = $($(OPSYS)_LD)
DEFAULT_AR             = $($(OPSYS)_LD)
DEFAULT_debug_LDFLAGS  = 
DEFAULT_LDFLAGS        = -DYNAMICBASE -NXCOMPAT -nologo -DEBUG -out:$@
DEFAULT_LDXXFLAGS      = $($(OPSYS)_LDFLAGS)
DEFAULT_SLDFLAGS       = -dll $($(OPSYS)_LDFLAGS) 
DEFAULT_ARFLAGS        = -lib -nologo -out:$@ 
DEFAULT_release_LDLIBS = ws2_32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib
DEFAULT_debug_LDLIBS = ws2_32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib
DEFAULT_LDLIBS         = -DYNAMICBASE -NXCOMPAT $($(OPSYS)_$(CONFIG)_LDLIBS)
DEFAULT_OBJSUFX        = .obj
DEFAULT_EXESUFX        = .exe
DEFAULT_SHLPRFX        = 
# SO_EXT in other words
DEFAULT_SHLSUFX        = .dll
DEFAULT_STLPRFX        = 
DEFAULT_STLSUFX        = .lib
DEFAULT_debug_STRIP   = touch
DEFAULT_release_STRIP = touch
DEFAULT_OUT           = -Fo
DEFAULT_CFLAGS2        = $($(OPSYS)_CFLAGS)
DEFAULT_LDFLAGS2       = $($(OPSYS)_LDFLAGS)
DEFAULT_SLDFLAGS2      = $($(OPSYS)_SLDFLAGS)
# mt is deprecated
DEFAULT_MT	     = echo mt is deprecated ,

# pre-build will set MUPPET but we can override it
# use this when no FIPS module is available
NO_MUPPET =
# use this definition to link in old FIPS module
UNIX_MUPPET         = $(OLD_ICC)/iccsdk/$(ICCLIB)

DEFAULT_MUPPET         = $(OLD_ICC)/iccsdk/$(ICCLIB)

# any flag can be defaulted this way to reduce copying overhead note = not :=
# example default LDXXFLAGS to be LDFLAGS
# override in OPSYS stanza

#set up defaults for all windows - specific stanzas will redefined as required
$(OPSYS)_CC = $(DEFAULT_CC)
$(OPSYS)_CXX = $(DEFAULT_CXX)
$(OPSYS)_debug_CFLAGS   = $(DEFAULT_debug_CFLAGS)
$(OPSYS)_release_CFLAGS = $(DEFAULT_release_CFLAGS)
$(OPSYS)_CFLAGS         = $(DEFAULT_CFLAGS)
$(OPSYS)_CXXFLAGS = $($(OPSYS)_CFLAGS)
$(OPSYS)_LD             = $(DEFAULT_LD)
$(OPSYS)_LD_CXX         = $(DEFAULT_LD_CXX)
$(OPSYS)_SLD            = $(DEFAULT_LD)
$(OPSYS)_AR             = $(DEFAULT_AR)
$(OPSYS)_debug_LDFLAGS  = $(DEFAULT_debug_LDFLAGS)
$(OPSYS)_LDFLAGS        = $(DEFAULT_LDFLAGS)
$(OPSYS)_LDXXFLAGS = $($(OPSYS)_LDFLAGS)
$(OPSYS)_SLDFLAGS       = $(DEFAULT_SLDFLAGS)
$(OPSYS)_ARFLAGS        = $(DEFAULT_ARFLAGS)
$(OPSYS)_release_LDLIBS = $(DEFAULT_release_LDLIBS)
$(OPSYS)_debug_LDLIBS   = $(DEFAULT_debug_LDLIBS)
$(OPSYS)_LDLIBS         = $(DEFAULT_LDLIBS)
$(OPSYS)_OBJSUFX        = $(DEFAULT_OBJSUFX)
$(OPSYS)_EXESUFX        = $(DEFAULT_EXESUFX)
$(OPSYS)_SHLPRFX        = $(DEFAULT_SHLPRFX)
$(OPSYS)_SHLSUFX        = $(DEFAULT_SHLSUFX)
$(OPSYS)_STLPRFX        = $(DEFAULT_STLPRFX)
$(OPSYS)_STLSUFX        = $(DEFAULT_STLSUFX)
$(OPSYS)_debug_STRIP   = $(DEFAULT_debug_STRIP)
$(OPSYS)_release_STRIP = $(DEFAULT_release_STRIP)
$(OPSYS)_OUT           = $(DEFAULT_OUT)
$(OPSYS)_CFLAGS2        = $(DEFAULT_CFLAGS2)
$(OPSYS)_LDFLAGS2       = $(DEFAULT_LDFLAGS2)
$(OPSYS)_SLDFLAGS2      = $(DEFAULT_SLDFLAGS2)
$(OPSYS)_MT	     = $(DEFAULT_MT)
$(OPSYS)_MUPPET         = $(DEFAULT_MUPPET)

#--- VisualC++ definitions on Win32
# cant use symlinks so define absolute location of ICC ie parent of package directory
WIN32_CP             = cp -f
WIN32_DEBUGGER       = msdev
WIN32_CC             = cl
WIN32_CXX             = cl -TP
WIN32_debug_CFLAGS   = -MDd -Zi -D DEBUG -RTCu
# Optimization off generates faster starting code ...
WIN32_release_CFLAGS = -MD -Zi
WIN32_CFLAGS         = -c -nologo $(WIN32_$(CONFIG)_CFLAGS) -W3 -GF -GS -D WIN32 -D_MBCS -D_CRT_SECURE_NO_WARNINGS
WIN32_LD             = link
WIN32_LD_CXX         = link
WIN32_SLD            = $(WIN32_LD)
WIN32_AR             = $(WIN32_LD)
WIN32_debug_LDFLAGS  = 
WIN32_LDFLAGS = -DYNAMICBASE -NXCOMPAT -nologo -DEBUG -out:$@
WIN32_SLDFLAGS       = -dll  $(WIN32_LDFLAGS) 
WIN32_ARFLAGS        = -lib -nologo -out:$@ 
WIN32_release_LDLIBS = ws2_32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib
WIN32_debug_LDLIBS = ws2_32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib
WIN32_LDLIBS         = -DYNAMICBASE -NXCOMPAT $(WIN32_$(CONFIG)_LDLIBS)
WIN32_OBJSUFX        = .obj
WIN32_EXESUFX        = .exe
WIN32_SHLPRFX        = 
WIN32_SHLSUFX        = .dll
WIN32_STLPRFX        = 
WIN32_STLSUFX        = .lib
WIN32_debug_STRIP   = touch
WIN32_release_STRIP = touch
WIN32_OUT           = -Fo
WIN32_CFLAGS2        = $(WIN32_CFLAGS)
WIN32_LDFLAGS2       = $(WIN32_LDFLAGS)
WIN32_SLDFLAGS2      = $(WIN32_SLDFLAGS)
#--- VisualC++ definitions on Win32

#--- VisualC++ definitions on Win64-AMD
#
# cant use symlinks so define absolute location of ICC ie parent of package directory
#
WIN64_AMD_RM             = rm -f
WIN64_AMD_MKDIR          = [ -d $@ ] || mkdir -p
WIN64_AMD_CP             = cp -f
WIN64_AMD_DEBUGGER       = msdev
WIN64_AMD_CC             = cl
WIN64_AMD_CXX            = cl -TP
WIN64_AMD_debug_CFLAGS   = -MDd -Zi -Od
WIN64_AMD_release_CFLAGS = -MD -Zi -Ox
WIN64_AMD_CFLAGS         = -c $(WIN64_AMD_$(CONFIG)_CFLAGS) -GS -nologo -D_AMD64_=1 -DWIN64 -D_WIN64 -DWIN32 -W4 -D_CRT_SECURE_NO_WARNINGS
WIN64_AMD_CXXFLAGS       = $(WIN64_AMD_CFLAGS)
WIN64_AMD_LD             = link
WIN64_AMD_LD_CXX         = link
WIN64_AMD_SLD            = $(WIN64_AMD_LD)
WIN64_AMD_AR             = $(WIN64_AMD_LD)
WIN64_AMD_debug_LDFLAGS  = -debug
WIN64_AMD_LDFLAGS        = -DYNAMICBASE -NXCOMPAT -DEBUG -nologo $(WIN64_AMD_$(CONFIG)_LDFLAGS) -out:$@
WIN64_AMD_SLDFLAGS       = -dll $(WIN64_AMD_LDFLAGS)
WIN64_AMD_ARFLAGS        = -lib -nologo -out:$@
WIN64_AMD_release_LDLIBS = ws2_32.lib gdi32.lib advapi32.lib user32.lib
WIN64_AMD_debug_LDLIBS   = $(WIN64_AMD_release_LDLIBS)
WIN64_AMD_LDLIBS         = -DYNAMICBASE -NXCOMPAT $(WIN64_AMD_$(CONFIG)_LDLIBS)
WIN64_AMD_OBJSUFX        = .obj
WIN64_AMD_EXESUFX        = .exe
WIN64_AMD_SHLPRFX        =
WIN64_AMD_SHLSUFX        = .dll
WIN64_AMD_STLPRFX        =
WIN64_AMD_STLSUFX        = .lib
WIN64_AMD_debug_STRIP    = touch
WIN64_AMD_release_STRIP  = touch
WIN64_AMD_OUT            = $(WIN32_OUT)
WIN64_AMD_CFLAGS2        = $(WIN64_AMD_CFLAGS)
WIN64_AMD_LDFLAGS2       = $(WIN64_AMD_LDFLAGS)
WIN64_AMD_SLDFLAGS2      = $(WIN64_AMD_SLDFLAGS)

# --- VisualC++ definitions on Win32 Visual Studio 2013 
WIN32_VS2013_RM             = rm -f
WIN32_VS2013_MKDIR          = [ -d $@ ] || mkdir -p
WIN32_VS2013_CP             = cp -f
WIN32_VS2013_DEBUGGER       = msdev
WIN32_VS2013_CC             = cl
WIN32_VS2013_CXX             = cl -TP
WIN32_VS2013_debug_CFLAGS   = -MDd -Zi -D DEBUG -RTCu
WIN32_VS2013_release_CFLAGS = -MD -Zi
WIN32_VS2013_CFLAGS         = -c -nologo $(WIN32_$(CONFIG)_CFLAGS) -W3 -GF -GS -D WIN32 -D _MBCS -D_CRT_SECURE_NO_WARNINGS
WIN32_VS2013_CXXFLAGS       = $(WIN32_VS2013_CFLAGS)
WIN32_VS2013_LD             = link
WIN32_VS2013_LD_CXX         = link
WIN32_VS2013_SLD            = $(WIN32_LD)
WIN32_VS2013_AR             = $(WIN32_LD)
WIN32_VS2013_debug_LDFLAGS  = 
WIN32_VS2013_LDFLAGS        = -DYNAMICBASE -NXCOMPAT -nologo -DEBUG -out:$@
WIN32_VS2013_SLDFLAGS       = -dll $(WIN32_VS2013_LDFLAGS)
WIN32_VS2013_ARFLAGS        = -lib -nologo -out:$@ 
WIN32_VS2013_release_LDLIBS = ws2_32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib
WIN32_VS2013_debug_LDLIBS = ws2_32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib
WIN32_VS2013_LDLIBS         = -DYNAMICBASE -NXCOMPAT $(WIN32_$(CONFIG)_LDLIBS)
WIN32_VS2013_OBJSUFX        = .obj
WIN32_VS2013_EXESUFX        = .exe
WIN32_VS2013_SHLPRFX        = 
WIN32_VS2013_SHLSUFX        = .dll
WIN32_VS2013_STLPRFX        = 
WIN32_VS2013_STLSUFX        = .lib
WIN32_VS2013_debug_STRIP   = touch
WIN32_VS2013_release_STRIP = touch
WIN32_VS2013_OUT           = -Fo
WIN32_VS2013_CFLAGS2        = $(WIN32_VS2013_CFLAGS)
WIN32_VS2013_LDFLAGS2       = $(WIN32_VS2013_LDFLAGS)
WIN32_VS2013_SLDFLAGS2      = $(WIN32_SLDFLAGS)
#--- VisualC++ definitions on Win32 Visual Studio 2013

# --- VisualC++ definitions on Win32 Visual Studio 2019/22
# build platform is Windows 10 build machine set up with VS2022 (upgraded from VS2019)
# use defaults
# NOTE: OPSYS=WIN32_VS2022
# --- end

#--- VisualC++ definitions on Win64-AMD Visual Studio 2013 
#
# cant use symlinks so define absolute location of ICC ie parent of package directory
#
WIN64_AMD_VS2013_RM             = rm -f
WIN64_AMD_VS2013_MKDIR          = [ -d $@ ] || mkdir -p
WIN64_AMD_VS2013_CP             = cp -f
WIN64_AMD_VS2013_DEBUGGER       = msdev
WIN64_AMD_VS2013_CC             = cl
WIN64_AMD_VS2013_CXX            = cl -TP
WIN64_AMD_VS2013_debug_CFLAGS   = -MDd -Zi
WIN64_AMD_VS2013_release_CFLAGS = -MD -Zi -Ox
# warnings to errors - can cause crash on some platforms - i.e 64 bit pointers cannot be int (32 bits)
#  warning C4013: ... undefined; assuming extern returning int
#  warning C4047: ... differs in levels of indirection from 'int'
WIN64_AMD_VS2013_W_FLAGS         = -W4 -D_CRT_SECURE_NO_WARNINGS -we4047 -we4013
WIN64_AMD_VS2013_CFLAGS         = -c $(WIN64_AMD_$(CONFIG)_CFLAGS) -GS -nologo -D_AMD64_=1 -DWIN64 -D_WIN64 -DWIN32 -D_WIN32 $(WIN64_AMD_VS2013_W_FLAGS)
WIN64_AMD_VS2013_CXXFLAGS       = $(WIN64_AMD_VS2013_CFLAGS)
WIN64_AMD_VS2013_LD             = link
WIN64_AMD_VS2013_LD_CXX         = link
WIN64_AMD_VS2013_SLD            = $(WIN64_AMD_LD)
WIN64_AMD_VS2013_AR             = $(WIN64_AMD_LD)
WIN64_AMD_VS2013_debug_LDFLAGS  = -debug
WIN64_AMD_VS2013_LDFLAGS        = -DYNAMICBASE -NXCOMPAT -DEBUG -nologo $(WIN64_AMD_$(CONFIG)_LDFLAGS) -out:$@
WIN64_AMD_VS2013_SLDFLAGS       = -dll $(WIN64_AMD_LDFLAGS)
WIN64_AMD_VS2013_ARFLAGS        = -lib -nologo -out:$@
WIN64_AMD_VS2013_release_LDLIBS = ws2_32.lib gdi32.lib advapi32.lib user32.lib
WIN64_AMD_VS2013_debug_LDLIBS   = $(WIN64_AMD_release_LDLIBS)
WIN64_AMD_VS2013_LDLIBS         = -DYNAMICBASE -NXCOMPAT $(WIN64_AMD_$(CONFIG)_LDLIBS)
WIN64_AMD_VS2013_OBJSUFX        = .obj
WIN64_AMD_VS2013_EXESUFX        = .exe
WIN64_AMD_VS2013_SHLPRFX        =
WIN64_AMD_VS2013_SHLSUFX        = .dll
WIN64_AMD_VS2013_STLPRFX        =
WIN64_AMD_VS2013_STLSUFX        = .lib
WIN64_AMD_VS2013_debug_STRIP    = touch
WIN64_AMD_VS2013_release_STRIP  = touch
WIN64_AMD_VS2013_OUT            = $(WIN32_OUT)
WIN64_AMD_VS2013_CFLAGS2        = $(WIN64_AMD_CFLAGS)
WIN64_AMD_VS2013_LDFLAGS2       = $(WIN64_AMD_LDFLAGS)
WIN64_AMD_VS2013_SLDFLAGS2      = $(WIN64_AMD_SLDFLAGS)

WIN64_VS2022_RM             = rm -f
WIN64_VS2022_MKDIR          = [ -d $@ ] || mkdir -p
WIN64_VS2022_CP             = cp -f
WIN64_VS2022_DEBUGGER       = msdev
WIN64_VS2022_CC             = cl
WIN64_VS2022_CXX            = cl -TP
WIN64_VS2022_debug_CFLAGS   = -MDd -Zi
WIN64_VS2022_release_CFLAGS = -MD -Zi -Ox
# warnings to errors - can cause crash on some platforms - i.e 64 bit pointers cannot be int (32 bits)
#  warning C4013: ... undefined; assuming extern returning int
#  warning C4047: ... differs in levels of indirection from 'int'
WIN64_VS2022_W_FLAGS         = -W4 -D_CRT_SECURE_NO_WARNINGS -we4047 -we4013
WIN64_VS2022_CFLAGS         = -c $(WIN64_VS2022_$(CONFIG)_CFLAGS) -GS -nologo -D_AMD64_=1 -DWIN64 -D_WIN64 -DWIN32 -D_WIN32 $(WIN64_VS2022_W_FLAGS)
WIN64_VS2022_CXXFLAGS       = $(WIN64_VS2022_CFLAGS) /EHsc
WIN64_VS2022_LD             = link
WIN64_VS2022_LD_CXX         = link
WIN64_VS2022_SLD            = $(WIN64_VS2022_LD)
WIN64_VS2022_AR             = $(WIN64_VS2022_LD)
WIN64_VS2022_debug_LDFLAGS  = -debug
WIN64_VS2022_LDFLAGS        = -DYNAMICBASE -NXCOMPAT -DEBUG -nologo $(WIN64_VS2022_$(CONFIG)_LDFLAGS) -out:$@
WIN64_VS2022_SLDFLAGS       = -dll $(WIN64_VS2022_LDFLAGS)
WIN64_VS2022_ARFLAGS        = -lib -nologo -out:$@
WIN64_VS2022_release_LDLIBS = gdi32.lib advapi32.lib user32.lib ws2_32.lib
WIN64_VS2022_debug_LDLIBS   = $(WIN64_VS2022_release_LDLIBS)
WIN64_VS2022_LDLIBS         = -DYNAMICBASE -NXCOMPAT $(WIN64_VS2022_$(CONFIG)_LDLIBS)
WIN64_VS2022_OBJSUFX        = .obj
WIN64_VS2022_EXESUFX        = .exe
WIN64_VS2022_SHLPRFX        =
WIN64_VS2022_SHLSUFX        = .dll
WIN64_VS2022_STLPRFX        =
WIN64_VS2022_STLSUFX        = .lib
WIN64_VS2022_debug_STRIP    = touch
WIN64_VS2022_release_STRIP  = touch
WIN64_VS2022_OUT            = $(WIN32_OUT)
WIN64_VS2022_CFLAGS2        = $(WIN64_VS2022_CFLAGS)
WIN64_VS2022_LDFLAGS2       = $(WIN64_VS2022_LDFLAGS)
WIN64_VS2022_SLDFLAGS2      = $(WIN64_VS2022_SLDFLAGS)
WIN64_VS2022_MUPPET         = $(OLD_ICC)/iccsdk/$(ICCLIB)

# Convenience definition for Linuxi
#
LINUX_OUT           = -o 

LINUX_debug_CFLAGS   = -g3 -O0
LINUX_asan_CFLAGS	 = $(LINUX_debug_CFLAGS) -fsanitize=address
LINUX_beam_CFLAGS    = -g -O3
LINUX_release_CFLAGS = -g1 -O3

LINUX_asan_LDFLAGS	 = -fsanitize=address

# std=gnu99 will allow declaring variables in the for loop header on phelix (gcc 4.1.2/2006)
LINUX32_CFLAGS = -std=gnu99 -m32 $(LINUX_$(CONFIG)_CFLAGS) -D_REENTRANT -fno-strict-aliasing -fno-exceptions -fPIC -Wall -c
LINUX32_CXXFLAGS = -std=gnu99 -m32 $(LINUX_$(CONFIG)_CFLAGS) -D_REENTRANT -fno-strict-aliasing -fPIC -Wall -c
LINUX32_LDFLAGS = -std=gnu99 -m32 $(LINUX_$(CONFIG)_LDFLAGS) $(LINUX_OUT) $@
LINUX32_SLDFLAGS = -std=gnu99 -m32 $(LINUX_$(CONFIG)_LDFLAGS) -shared -Wl,-soname,$@  -Wl,--rpath,\$$ORIGIN $(LINUX_OUT) $@

# This one makes too many "error" matches in log file searches
#LINUX64_CFLAGS += -Werror=implicit-function-declaration
ifeq (OFFICIAL, $(BUILD))
LINUX64_CFLAGS = -std=gnu99 -m64 $(LINUX_$(CONFIG)_CFLAGS) -D_REENTRANT -fno-strict-aliasing -fno-exceptions -fPIC -Wall -c
LINUX64_CXXFLAGS = -std=gnu99 -m64 $(LINUX_$(CONFIG)_CFLAGS) -D_REENTRANT -fno-strict-aliasing -fPIC -Wall -c
else
LINUX64_CFLAGS = -std=gnu99 -m64 $(LINUX_$(CONFIG)_CFLAGS) -D_REENTRANT -fno-strict-aliasing -fno-exceptions -fPIC -Wall -c \
                 -Werror=incompatible-pointer-types \
                 -Werror=implicit-int \
                 -Werror=implicit-function-declaration \
                 -Werror=return-type \
                 -Werror=int-conversion   
LINUX64_CXXFLAGS = -std=gnu99 -m64 $(LINUX_$(CONFIG)_CFLAGS) -D_REENTRANT -fno-strict-aliasing -fPIC -Wall -c \
                   -Werror=return-type
endif

LINUX64_LDFLAGS = -std=gnu99 -m64 $(LINUX_$(CONFIG)_LDFLAGS) $(LINUX_OUT) $@
LINUX64_SLDFLAGS = -std=gnu99 -m64 $(LINUX_$(CONFIG)_LDFLAGS) -shared -Wl,-soname,$@ -Wl,--rpath,\$$ORIGIN $(LINUX_OUT) $@

#--- GCC definitions on Linux IA32
LINUX_RM             = rm -f
LINUX_MKDIR          = [ -d $@ ] || mkdir -p
LINUX_CP             = cp -f
LINUX_DEBUGGER       = ddd
LINUX_CC             = gcc
LINUX_CXX            = g++
LINUX_CFLAGS         = $(LINUX32_CFLAGS)
LINUX_CXXFLAGS       = $(LINUX32_CXXFLAGS)
LINUX_release_LD     = $(LINUX_CC)
LINUX_debug_LD       = $(LINUX_CC)
LINUX_beam_LD        = true
LINUX_LD             = $(LINUX_CC)
LINUX_LD_CXX         = $(LINUX_CXX)
LINUX_SLD            = $(LINUX_LD)
LINUX_AR             = ar
LINUX_LDFLAGS        = $(LINUX32_LDFLAGS)
LINUX_SLDFLAGS       = $(LINUX32_SLDFLAGS)  -z noexecstack
LINUX_ARFLAGS        = -ruv $@
LINUX_LDLIBS         = -lpthread -ldl
LINUX_release_OBJSUFX   = .o
LINUX_debug_OBJSUFX  = .o
LINUX_beam_OBJSUFX   = .beam
LINUX_OBJSUFX        = $(LINUX_$(CONFIG)_OBJSUFX)
LINUX_EXESUFX        =
LINUX_SHLPRFX        = lib
LINUX_SHLSUFX        = .so
LINUX_STLPRFX        = lib
LINUX_STLSUFX        = .a
LINUX_debug_STRIP   = touch
LINUX_release_STRIP = strip
LINUX_beam_STRIP    = touch
LINUX_CFLAGS2        = $(LINUX32_CFLAGS)
LINUX_LDFLAGS2       = $(LINUX32_LDFLAGS)
LINUX_SLDFLAGS2      = $(LINUX32_SLDFLAGS)  -z noexecstack
LINUX_MT             = true
# HP/UX Specific
LINUX_ICCLIB_LNK     =


#--- GCC definitions on Linux IA32/IA64, Directly linked to OpenSSL
#--
#-- Uses "normal" shared library linkage not dlopen/dlsym
#-- This isn't a production version, but it's used to debug platforms 
#-- like OS400 which don't have dlopen/dlsym
#--
#-- Note that that it's also useful for debugging, the table based calls aren't
#-- traced well by debuggers like valgrind unless the libs are linked normally.
#--
LINUXDL_RM             = rm -f
LINUXDL_MKDIR          = [ -d $@ ] || mkdir -p
LINUXDL_CP             = cp -f
LINUXDL_DEBUGGER       = ddd
LINUXDL_CC             = $(LINUX_CC)
LINUXDL_CXX            = $(LINUX_CXX)
LINUXDL_debug_CFLAGS   = -g3
LINUXDL_beam_CFLAGS    = -g -O3
LINUXDL_release_CFLAGS = -g1 -O3
LINUXDL_CFLAGS         = $(LINUXDL_$(CONFIG)_CFLAGS) -D_REENTRANT -DDIRECT_LINK -fPIC -Wall  -c
LINUXDL_release_LD     = $(LINUXDL_CC)
LINUXDL_debug_LD       = $(LINUXDL_CC)
LINUXDL_beam_LD        = true
LINUXDL_LD             = $(LINUX_LD)
LINUXDL_LD_CXX         = $(LINUXDL_CXX)
LINUXDL_SLD            = $(LINUXDL_LD)
LINUXDL_AR             = ar
LINUXDL_LDFLAGS        = $(LINUX_OUT)
LINUXDL_SLDFLAGS       = $(LINUX_SLDFLAGS)  -z noexecstack
LINUXDL_ARFLAGS        = -ruv $@
LINUXDL_LDLIBS         = -lpthread -ldl
LINUXDL_debug_OBJSUFX  = .o
LINUXDL_beam_OBJSUFX   = .beam
LINUXDL_OBJSUFX        = $(LINUXDL_$(CONFIG)_OBJSUFX)
LINUXDL_EXESUFX        =
LINUXDL_SHLPRFX        = lib
LINUXDL_SHLSUFX        = .so
LINUXDL_STLPRFX        = lib
LINUXDL_STLSUFX        = .a
LINUXDL_debug_STRIP   = touch
LINUXDL_release_STRIP = strip
LINUXDL_beam_STRIP    = touch
LINUXDL_OUT           = $(LINUX_OUT)
LINUXDL_CFLAGS2        = $(LINUXDL_CFLAGS)
LINUXDL_LDFLAGS2       = $(LINUXDL_LDFLAGS)
LINUXDL_SLDFLAGS2      = $(LINUXDL_SLDFLAGS)  -z noexecstack
LINUX_DL_MT            = true

#--- GCC definitions on Linux IA64
IA64_LINUX_RM             = rm -f
IA64_LINUX_MKDIR          = [ -d $@ ] || mkdir -p
IA64_LINUX_CP             = cp -f
IA64_LINUX_DEBUGGER       = ddd
IA64_LINUX_CC             = $(LINUX_CC)
IA64_LINUX_CXX            = $(LINUX_CXX)
IA64_LINUX_CFLAGS         = $(filter-out -m64, $(LINUX64_CFLAGS))
IA64_LINUX_debug_CFLAGS   =
IA64_LINUX_release_CFLAGS =
IA64_LINUX_CXXFLAGS       = $(filter-out -m64, $(LINUX64_CXXFLAGS))
IA64_LINUX_LD             = $(LINUX_CC)
IA64_LINUX_LD_CXX         = $(LINUX_CXX)
IA64_LINUX_SLD            = $(LINUX_LD)
IA64_LINUX_AR             = ar
IA64_LINUX_LDFLAGS        = $(LINUX_OUT) $@
IA64_LINUX_SLDFLAGS       = $(LINUX64_SLDFLAGS)  -z noexecstack
IA64_LINUX_ARFLAGS        = -ruv $@
IA64_LINUX_LDLIBS         = -lpthread -ldl
IA64_LINUX_OBJSUFX        = .o
IA64_LINUX_EXESUFX        =
IA64_LINUX_SHLPRFX        = lib
IA64_LINUX_SHLSUFX        = .so
IA64_LINUX_STLPRFX        = lib
IA64_LINUX_STLSUFX        = .a
IA64_LINUX_debug_STRIP	  = touch
IA64_LINUX_release_STRIP  = strip
IA64_LINUX_OUT            = $(LINUX_OUT)
IA64_LINUX_CFLAGS2        = $(IA64_LINUX_CFLAGS)
IA64_LINUX_LDFLAGS2       = $(IA64_LINUX_LDFLAGS)
IA64_LINUX_SLDFLAGS2      = $(IA64_LINUX_SLDFLAGS)  -z noexecstack
IA64_LINUX_MT             = true
IA64_LINUX_MUPPET         =

#--- GCC definitions on Linux ARM
# Note X compile arm x-compiler installed and
#  export ARCH=arm
#  export CROSS_COMPILE=/usr/lib64/ccache/arm-gp2x-linux-

ARM_LINUX_RM             = rm -f
ARM_LINUX_MKDIR          = [ -d $@ ] || mkdir -p
ARM_LINUX_CP             = cp -f
ARM_LINUX_DEBUGGER       = ddd
#ARM_LINUX_CC             = /usr/lib64/ccache/arm-gp2x-linux-gcc
#ARM_LINUX_CXX            = /usr/lib64/ccache/arm-gp2x-linux-g++
ARM_LINUX_CC             = $(LINUX_CC)
ARM_LINUX_CXX            = $(LINUX_CXX)
ARM_LINUX_CXX            = $(LINUX_CXX)
ARM_LINUX_debug_CFLAGS   = $(LINUX_debug_CFLAGS)
ARM_LINUX_release_CFLAGS = $(LINUX_release_CFLAGS)
ARM_LINUX_CFLAGS         = $(filter-out -m64, $(LINUX64_CFLAGS))
ARM_LINUX_CXXFLAGS       = $(filter-out -m64, $(LINUX64_CXXFLAGS))
ARM_LINUX_LD             = $(ARM_LINUX_CC)
ARM_LINUX_LD_CXX         = $(ARM_LINUX_CXX)
ARM_LINUX_SLD            = $(ARM_LINUX_LD)
ARM_LINUX_AR             = ar
ARM_LINUX_LDFLAGS        = $(LINUX_OUT) $@
ARM_LINUX_SLDFLAGS       = -shared -Wl,-soname,$@  -Wl,--rpath,\$$ORIGIN $(LINUX_OUT) $@  -Wl,--rpath,\$$ORIGIN $(LINUX_OUT)  $@
ARM_LINUX_ARFLAGS        = -ruv $@
ARM_LINUX_LDLIBS         = -lpthread -ldl
ARM_LINUX_OBJSUFX        = .o
ARM_LINUX_EXESUFX        =
ARM_LINUX_SHLPRFX        = lib
ARM_LINUX_SHLSUFX        = .so
ARM_LINUX_STLPRFX        = lib
ARM_LINUX_STLSUFX        = .a
ARM_LINUX_debug_STRIP	  = touch
ARM_LINUX_release_STRIP  = strip
ARM_LINUX_OUT            = $(LINUX_OUT)
ARM_LINUX_CFLAGS2        = $(ARM_LINUX_CFLAGS)
ARM_LINUX_LDFLAGS2       = $(ARM_LINUX_LDFLAGS)
ARM_LINUX_SLDFLAGS2      = $(ARM_LINUX_SLDFLAGS)
ARM_LINUX_MT             = true

#--- GCC definitions on Linux ARM


ARM64_LINUX_RM             = rm -f
ARM64_LINUX_MKDIR          = [ -d $@ ] || mkdir -p
ARM64_LINUX_CP             = cp -f
ARM64_LINUX_DEBUGGER       = ddd
ARM64_LINUX_CC             = $(LINUX_CC)
ARM64_LINUX_CXX            = $(LINUX_CXX)
ARM64_LINUX_CFLAGS         = $(filter-out -m64, $(LINUX64_CFLAGS))
ARM64_LINUX_CXXFLAGS       = $(filter-out -m64, $(LINUX64_CXXFLAGS))
ARM64_LINUX_LD             = $(ARM64_LINUX_CC)
ARM64_LINUX_LD_CXX         = $(ARM64_LINUX_CXX)
ARM64_LINUX_SLD            = $(ARM64_LINUX_LD)
ARM64_LINUX_AR             = ar
ARM64_LINUX_LDFLAGS        = $(LINUX_OUT) $@
ARM64_LINUX_SLDFLAGS       = -shared -Wl,-soname,$@  -Wl,--rpath,\$$ORIGIN $(LINUX_OUT) $@  -Wl,--rpath,\$$ORIGIN $(LINUX_OUT) $@
ARM64_LINUX_ARFLAGS        = -ruv $@
ARM64_LINUX_LDLIBS         = -lpthread -ldl
ARM64_LINUX_OBJSUFX        = .o
ARM64_LINUX_EXESUFX        =
ARM64_LINUX_SHLPRFX        = lib
ARM64_LINUX_SHLSUFX        = .so
ARM64_LINUX_STLPRFX        = lib
ARM64_LINUX_STLSUFX        = .a
ARM64_LINUX_debug_STRIP	  = touch
ARM64_LINUX_release_STRIP  = strip
ARM64_LINUX_OUT            = $(LINUX_OUT)
ARM64_LINUX_CFLAGS2        = $(ARM64_LINUX_CFLAGS)
ARM64_LINUX_LDFLAGS2       = $(ARM64_LINUX_LDFLAGS)
ARM64_LINUX_SLDFLAGS2      = $(ARM64_LINUX_SLDFLAGS)
ARM64_LINUX_MT             = true

#--- GCC definitions on Linux AMD64
AMD64_LINUX_CC             = $(LINUX_CC)
AMD64_LINUX_CXX             = $(LINUX_CXX)
AMD64_LINUX_CFLAGS         = $(LINUX64_CFLAGS)
AMD64_LINUX_debug_CFLAGS   =
AMD64_LINUX_release_CFLAGS =
AMD64_LINUX_CXXFLAGS       = $(LINUX64_CXXFLAGS)
AMD64_LINUX_LD             = $(LINUX_LD)
AMD64_LINUX_LD_CXX         = $(LINUX_CXX)
AMD64_LINUX_SLD            = $(LINUX_LD)
AMD64_LINUX_AR             = ar
AMD64_LINUX_LDFLAGS        = $(LINUX64_LDFLAGS)
AMD64_LINUX_SLDFLAGS       = $(LINUX64_SLDFLAGS) -z noexecstack
AMD64_LINUX_ARFLAGS        = -ruv $@
AMD64_LINUX_LDLIBS         = -lpthread -ldl
AMD64_LINUX_OBJSUFX        = .o
AMD64_LINUX_EXESUFX        =
AMD64_LINUX_SHLPRFX        = lib
AMD64_LINUX_SHLSUFX        = .so
AMD64_LINUX_STLPRFX        = lib
AMD64_LINUX_STLSUFX        = .a
AMD64_LINUX_debug_STRIP    = touch
AMD64_LINUX_asan_STRIP	   = touch
AMD64_LINUX_release_STRIP  = strip
AMD64_LINUX_OUT            = $(LINUX_OUT)
AMD64_LINUX_CFLAGS2        = $(AMD64_LINUX_CFLAGS)
AMD64_LINUX_LDFLAGS2       = $(AMD64_LINUX_LDFLAGS)
AMD64_LINUX_SLDFLAGS2      = $(AMD64_LINUX_SLDFLAGS)
AMD64_LINUX_MT             = true
AMD64_LINUX_MUPPET         = $(UNIX_MUPPET)

#--- GCC definitions on Linux PPC
PPC_LINUX_RM             = rm -f
PPC_LINUX_MKDIR          = [ -d $@ ] || mkdir -p
PPC_LINUX_CP             = cp -f
PPC_LINUX_DEBUGGER       = ddd
PPC_LINUX_CC             = $(LINUX_CC)
PPC_LINUX_CXX            = $(LINUX_CXX)
PPC_LINUX_CFLAGS         = $(LINUX32_CFLAGS)
PPC_LINUX_debug_CFLAGS   =
PPC_LINUX_release_CFLAGS =
PPC_LINUX_CXXFLAGS       = $(LINUX32_CXXFLAGS)
PPC_LINUX_LD             = $(PPC_LINUX_CC)
PPC_LINUX_LD_CXX         = $(PPC_LINUX_CXX)
PPC_LINUX_SLD            = $(PPC_LINUX_LD)
PPC_LINUX_AR             = ar
PPC_LINUX_LDFLAGS        = $(LINUX32_LDFLAGS)
PPC_LINUX_SLDFLAGS       = $(LINUX32_SLDFLAGS)
PPC_LINUX_ARFLAGS        = -ruv $@
PPC_LINUX_LDLIBS         = -lpthread -ldl
PPC_LINUX_OBJSUFX        = .o
PPC_LINUX_EXESUFX        =
PPC_LINUX_SHLPRFX        = lib
PPC_LINUX_SHLSUFX        = .so
PPC_LINUX_STLPRFX        = lib
PPC_LINUX_STLSUFX        = .a
PPC_LINUX_debug_STRIP    = touch
PPC_LINUX_release_STRIP  = strip
PPC_LINUX_OUT            = $(LINUX_OUT)
PPC_LINUX_CFLAGS2        = $(PPC_LINUX_CFLAGS)
PPC_LINUX_LDFLAGS2       = $(PPC_LINUX_LDFLAGS)
PPC_LINUX_SLDFLAGS2      = $(PPC_LINUX_SLDFLAGS)
PPC_LINUX_MT             = true

#--- GCC definitions on Linux PPC64)
PPC64_LINUX_RM             = rm -f
PPC64_LINUX_MKDIR          = [ -d $@ ] || mkdir -p
PPC64_LINUX_CP             = cp -f
PPC64_LINUX_DEBUGGER       = ddd
PPC64_LINUX_CC             = $(LINUX_CC)
PPC64_LINUX_CXX             = $(LINUX_CXX)
# OpenSSL command segfaults with -O3,-O2, -O1 and -O0, but not as often with O2
PPC64_LINUX_CFLAGS         = $(LINUX64_CFLAGS)
PPC64_LINUX_debug_CFLAGS   =
PPC64_LINUX_release_CFLAGS =
PPC64_LINUX_CXXFLAGS       = $(LINUX64_CXXFLAGS)
PPC64_LINUX_LD             = $(LINUX_CC)
PPC64_LINUX_LD_CXX         = $(LINUX_CXX)
PPC64_LINUX_SLD            = $(LINUX_LD)
PPC64_LINUX_AR             = ar
PPC64_LINUX_LDFLAGS        = $(LINUX64_LDFLAGS)
PPC64_LINUX_SLDFLAGS       = $(LINUX64_SLDFLAGS)
PPC64_LINUX_ARFLAGS        = -ruv $@
PPC64_LINUX_LDLIBS         = -lpthread -ldl
PPC64_LINUX_OBJSUFX        = .o
PPC64_LINUX_EXESUFX        =
PPC64_LINUX_SHLPRFX        = lib
PPC64_LINUX_SHLSUFX        = .so
PPC64_LINUX_STLPRFX        = lib
PPC64_LINUX_STLSUFX        = .a
PPC64_LINUX_debug_STRIP    = touch
PPC64_LINUX_release_STRIP  = strip
PPC64_LINUX_OUT            = $(LINUX_OUT)
PPC64_LINUX_CFLAGS2        = $(PPC64_LINUX_CFLAGS)
PPC64_LINUX_LDFLAGS2       = $(PPC64_LINUX_LDFLAGS)
PPC64_LINUX_SLDFLAGS2      = $(PPC64_LINUX_SLDFLAGS)
PPC64_LINUX_MT             = true
PPC64_LINUX_MUPPET         = $(UNIX_MUPPET)

##--- GCC definitions on Linux PPC64 Little-Endian
PPC64LE_LINUX_RM             = rm -f
PPC64LE_LINUX_MKDIR          = [ -d $@ ] || mkdir -p
PPC64LE_LINUX_CP             = cp -f
PPC64LE_LINUX_DEBUGGER       = ddd
PPC64LE_LINUX_CC             = $(LINUX_CC)
PPC64LE_LINUX_CXX             = $(LINUX_CXX)
# OpenSSL command segfaults with -O3,-O2, -O1 and -O0, but not as often with O2
PPC64LE_LINUX_CFLAGS         = $(LINUX64_CFLAGS)
PPC64LE_LINUX_debug_CFLAGS   =
PPC64LE_LINUX_release_CFLAGS =
PPC64LE_LINUX_CXXFLAGS       = $(LINUX64_CXXFLAGS)
PPC64LE_LINUX_LD             = $(LINUX_CC)
PPC64LE_LINUX_LD_CXX         = $(LINUX_CXX)
PPC64LE_LINUX_SLD            = $(LINUX_LD)
PPC64LE_LINUX_AR             = ar
PPC64LE_LINUX_LDFLAGS        = $(LINUX64_LDFLAGS)
PPC64LE_LINUX_SLDFLAGS       = $(LINUX64_SLDFLAGS) -z noexecstack
PPC64LE_LINUX_ARFLAGS        = -ruv $@
PPC64LE_LINUX_LDLIBS         = -lpthread -ldl
PPC64LE_LINUX_OBJSUFX        = .o
PPC64LE_LINUX_EXESUFX        =
PPC64LE_LINUX_SHLPRFX        = lib
PPC64LE_LINUX_SHLSUFX        = .so
PPC64LE_LINUX_STLPRFX        = lib
PPC64LE_LINUX_STLSUFX        = .a
PPC64LE_LINUX_debug_STRIP    = touch
PPC64LE_LINUX_release_STRIP  = strip
PPC64LE_LINUX_OUT            = $(LINUX_OUT)
PPC64LE_LINUX_CFLAGS2        = $(PPC64LE_LINUX_CFLAGS)
PPC64LE_LINUX_LDFLAGS2       = $(PPC64LE_LINUX_LDFLAGS)
PPC64LE_LINUX_SLDFLAGS2      = $(PPC64LE_LINUX_SLDFLAGS)
PPC64LE_LINUX_MT             = true
PPC64LE_LINUX_MUPPET         = $(UNIX_MUPPET)

# Special definitions for OS/X 
# This changes every release and is a real pain to deal with
#

DARWIN_LDFLAGS          = -isysroot /Developer/SDKs/MacOSX10.6.sdk -o $@
DARWIN_SLDFLAGS         = -dynamiclib -dylib -Wl,-syslibroot /Developer/SDKs/MacOSX10.6.sdk -install_name ./$@ -headerpad_max_install_names -o $@
# OSVER is never defined - this must be a typo
#OSX_LDFLAGS             = $($(OSVER)_LDFLAGS)
#OSX_SLDFLAGS            = $($(OSVER)_SLDFLAGS)



#--- GCC definitions for OSX PPC part build for fat binaries
#--- REPLACED BY PPC_OSX !!!!
#--- NOTE NOTE NOTE. This is used as the template for the other OSX 
#--- build configs!!!!
OSX_RM         		= rm -f
OSX_MKDIR     		= [ -d $@ ] || mkdir -p
OSX_CP             	= cp -f
OSX_DEBUGGER       	= gdb
OSX_CC             	= $(LINUX_CC) 
OSX_CXX             = $(LINUX_CXX) 
OSX_debug_CFLAGS   	= -g3 -O3
OSX_release_CFLAGS 	= -g1 -O3
OSX_CFLAGS         	= $(OSX_$(CONFIG)_CFLAGS) -D_REENTRANT -fPIC -Wall -fno-strict-aliasing -fno-common -fomit-frame-pointer -c
OSX_CXXFLAGS        = $(OSX_CFLAGS)
OSX_LD             	= $(OSX_CC) 
OSX_LD_CXX         	= $(OSX_CXX)
# Need to use ld here, it's the only way we can selectively make sybols private.
OSX_SLD            	= $(OSX_CC)
OSX_AR             	= ar
OSX_LDFLAGS        	= $(DARWIN_LDFLAGS)
OSX_SLDFLAGS       	= $(DARWIN_SLDFLAGS)
OSX_ARFLAGS        	= -ruvs $@
OSX_LDLIBS         	= -lpthread -ldl -lSystemStubs
OSX_OBJSUFX        	= .o
OSX_EXESUFX        	=
OSX_SHLPRFX        	= lib
OSX_SHLSUFX        	= .dylib
OSX_STLPRFX        	= lib
OSX_STLSUFX        	= .a
OSX_debug_STRIP   	= chmod +w $@; touch
OSX_release_STRIP 	= chmod +w $@; touch
OSX_OUT                 = -o 
OSX_CFLAGS2             = -arch i386 -arch x86_64 $(OSX_CFLAGS)
OSX_LDFLAGS2            = -arch i386 -arch x86_64 $(OSX_LDFLAGS)
OSX_SLDFLAGS2           = -arch i386 -arch x86_64 $(OSX_SLDFLAGS)
OSX_MT                 	= true


#--- GCC definitions for OSX x86 part build for fat binaries
#--- NOT The same as the builds above, this is half the platform
OSX_X86_ARCH            = -arch i386
OSX_X86_RM         	= $(OSX_RM)
OSX_X86_MKDIR     	= $(OSX_MKDIR)
OSX_X86_CP             	= $(OSX_CP)
OSX_X86_DEBUGGER       	= $(OSX_DEBUGGER)
OSX_X86_CC             	= $(OSX_CC)
OSX_X86_CXX             = $(OSX_CXX)
OSX_X86_CFLAGS         	= $(OSX_X86_ARCH) $(OSX_CFLAGS)
OSX_X86_debug_CFLAGS    =
OSX_X86_release_CFLAGS  =
OSX_X86_LD             	= $(OSX_LD)
OSX_X86_LD_CXX         	= $(OSX_LD_CXX)
# Need to use ld here, it's the only way we can selectively make sybols private.
OSX_X86_SLD            	= $(OSX_SLD)
OSX_X86_AR             	= $(OSX_AR)
# Problem here, with the flag combos. Fix it :).
OSX_X86_LDFLAGS        	= $(OSX_X86_ARCH) $(OSX_LDFLAGS)
OSX_X86_SLDFLAGS       	= $(OSX_X86_ARCH) $(OSX_SLDFLAGS)
OSX_X86_ARFLAGS        	= $(OSX_ARFLAGS)
OSX_X86_LDLIBS         	= $(OSX_LDLIBS)
OSX_X86_OBJSUFX        	= $(OSX_OBJSUFX)
OSX_X86_EXESUFX        	= $(OSX_EXESUFX)
OSX_X86_SHLPRFX        	= $(OSX_SHLPRFX)
OSX_X86_SHLSUFX        	= $(OSX_SHLSUFX)
OSX_X86_STLPRFX        	= $(OSX_STLPRFX)
OSX_X86_STLSUFX        	= $(OSX_STLSUFX)
OSX_X86_debug_STRIP   	= $(OSX_debug_STRIP)
OSX_X86_release_STRIP 	= $(OSX_release_STRIP)
OSX_X86_OUT             = $(LINUX_OUT)
OSX_X86_CFLAGS2		= $(OSX_CFLAGS2)
OSX_X86_LDFLAGS2	= $(OSX_LDFLAGS2)
OSX_X86_SLDFLAGS2	= $(OSX_SLDFLAGS2)
OSX_X86_MT 		= true


#--- GCC definitions for OSX x86_64 part build for fat binaries
#--- NOT The same as the builds above, this is 1/4 of the platform
OSX_X86_64_ARCH         = -arch x86_64
OSX_X86_64_RM           = $(OSX_RM)
OSX_X86_64_MKDIR        = $(OSX_MKDIR)
OSX_X86_64_CP           = $(OSX_CP)
OSX_X86_64_DEBUGGER     = $(OSX_DEBUGGER)
OSX_X86_64_CC           = $(OSX_CC)
OSX_X86_64_CXX          = $(OSX_CXX)
OSX_X86_64_CFLAGS       = $(OSX_X86_64_ARCH) $(OSX_CFLAGS)
OSX_X86_64_debug_CFLAGS   =
OSX_X86_64_release_CFLAGS =
OSX_X86_64_CXXFLAGS     = $(OSX_X86_64_CFLAGS)
OSX_X86_64_LD           = $(OSX_LD)
OSX_X86_64_LD_CXX       = $(OSX_LD_CXX)
OSX_X86_64_SLD          = $(OSX_SLD)
OSX_X86_64_AR           = $(OSX_AR)
OSX_X86_64_LDFLAGS      = $(OSX_X86_64_ARCH) $(OSX_LDFLAGS)
OSX_X86_64_SLDFLAGS     = $(OSX_X86_64_ARCH) $(OSX_SLDFLAGS)
OSX_X86_64_ARFLAGS      = $(OSX_ARFLAGS)
OSX_X86_64_LDLIBS       = $(OSX_LDLIBS)
OSX_X86_64_OBJSUFX      = $(OSX_OBJSUFX)
OSX_X86_64_EXESUFX      = $(OSX_EXESUFX)
OSX_X86_64_SHLPRFX      = $(OSX_SHLPRFX)
OSX_X86_64_SHLSUFX      = $(OSX_SHLSUFX)
OSX_X86_64_STLPRFX      = $(OSX_STLPRFX)
OSX_X86_64_STLSUFX      = $(OSX_STLSUFX)
OSX_X86_64_debug_STRIP  = $(OSX_debug_STRIP)
OSX_X86_64_release_STRIP        = $(OSX_release_STRIP)
OSX_X86_64_OUT          = $(OSX_OUT)
OSX_X86_64_CFLAGS2	= $(OSX_CFLAGS2)
OSX_X86_64_LDFLAGS2	= $(OSX_LDFLAGS2)
OSX_X86_64_SLDFLAGS2	= $(OSX_SLDFLAGS2)
OSX_X86_64_MT		= true



#--- GCC definitions for four architecture fat binaries
#--- Actually dual arch , we dropped PPC/PPC64
#--- Easier to 'fix' here than in another swarm of Makefiles
#--- Where there's no asm and the compiler can cope.
#--- Test cases/regression tests use this
#
OSX_FAT4_ARCH 		= -arch i386 -arch x86_64 
OSX_FAT4_RM         	= $(OSX_RM)
OSX_FAT4_MKDIR     	= $(OSX_MKDIR)
OSX_FAT4_CP             = $(OSX_CP)
OSX_FAT4_DEBUGGER       = $(OSX_DEBUGGER)
OSX_FAT4_CC             = $(OSX_CC)
OSX_FAT4_CXX            = $(OSX_CXX)
OSX_FAT4_CFLAGS         = $(OSX_FAT4_ARCH) $(OSX_CFLAGS)
OSX_FAT4_debug_CFLAGS    =
OSX_FAT4_release_CFLAGS  =
OSX_FAT4_LD             = $(OSX_LD)
OSX_FAT4_LD_CXX         = $(OSX_LD_CXX)
OSX_FAT4_SLD            = $(OSX_SLD)
OSX_FAT4_AR             = $(OSX_AR)
OSX_FAT4_LDFLAGS        = $(OSX_FAT4_ARCH) $(OSX_LDFLAGS)
OSX_FAT4_SLDFLAGS       = $(OSX_FAT4_ARCH) $(OSX_SLDFLAGS)
OSX_FAT4_ARFLAGS        = $(OSX_ARFLAGS)
OSX_FAT4_LDLIBS         = $(OSX_LDLIBS)
OSX_FAT4_OBJSUFX        = $(OSX_OBJSUFX)
OSX_FAT4_EXESUFX        = $(OSX_EXESUFX)
OSX_FAT4_SHLPRFX        = $(OSX_SHLPRFX)
OSX_FAT4_SHLSUFX        = $(OSX_SHLSUFX)
OSX_FAT4_STLPRFX        = $(OSX_STLPRFX)
OSX_FAT4_STLSUFX        = $(OSX_STLSUFX)
OSX_FAT4_debug_STRIP   	= $(OSX_debug_STRIP)
OSX_FAT4_release_STRIP 	= $(OSX_release_STRIP)
OSX_FAT4_OUT            = $(LINUX_OUT)
OSX_FAT4_CFLAGS2	= $(OSX_FAT4_CFLAGS)
OSX_FAT4_LDFLAGS2	= $(OSX_FAT4_LDFLAGS)
OSX_FAT4_SLDFLAGS2	= $(OSX_FAT4_SLDFLAGS)
OSX_FAT4_MT		= true


#DARWIN_LDFLAGS          = -isysroot /Developer/SDKs/MacOSX10.6.sdk -o $@
#DARWIN_SLDFLAGS         = -dynamiclib -dylib -Wl,-syslibroot /Developer/SDKs/MacOSX10.6.sdk -install_name ./$@ -headerpad_max_install_names -o $@
#--- GCC definitions for OSX x86_64  part build for GSKitV9
#--- NOT The same as the builds above, this is 1/4 of the platform
OSXV9_ARCH         = -arch x86_64
OSXV9_RM           = $(OSX_RM)
OSXV9_MKDIR        = $(OSX_MKDIR)
OSXV9_CP           = $(OSX_CP)
OSXV9_DEBUGGER     = $(OSX_DEBUGGER)
OSXV9_CC           = $(OSX_CC)
OSXV9_CXX          = $(OSX_CXX)
OSXV9_CFLAGS       = $(OSXV9_ARCH) $(OSX_CFLAGS)
OSXV9_debug_CFLAGS    =
OSXV9_release_CFLAGS  =
OSXV9_LD           = $(OSX_LD)
OSXV9_LD_CXX       = $(OSX_LD_CXX)
OSXV9_SLD          = $(OSX_SLD)
OSXV9_AR           = $(OSX_AR)
OSXV9_LDFLAGS      = $(OSXV9_ARCH) -o $@
OSXV9_SLDFLAGS     = $(OSXV9_ARCH) -dynamiclib -dylib  -install_name @rpath/$@ -headerpad_max_install_names -o $@
OSXV9_ARFLAGS      = $(OSX_ARFLAGS)
OSXV9_LDLIBS       = -lpthread -ldl 
OSXV9_OBJSUFX      = $(OSX_OBJSUFX)
OSXV9_EXESUFX      = $(OSX_EXESUFX)
OSXV9_SHLPRFX      = $(OSX_SHLPRFX)
OSXV9_SHLSUFX      = $(OSX_SHLSUFX)
OSXV9_STLPRFX      = $(OSX_STLPRFX)
OSXV9_STLSUFX      = $(OSX_STLSUFX)
OSXV9_debug_STRIP  = $(OSX_debug_STRIP)
OSXV9_release_STRIP        = $(OSX_release_STRIP)
OSXV9_OUT          = $(OSX_OUT)
OSXV9_CFLAGS2	= $(OSX_CFLAGS2)
OSXV9_LDFLAGS2	= $(OSX_LDFLAGS2)
OSXV9_SLDFLAGS2	= $(OSX_SLDFLAGS2)
OSXV9_MT		= true

#
# Mac M1 OS/X ARM64
#
OSX_ARM64_EXPORT_FLAG          = $(OSX_EXPORT_FLAG)
OSX_ARM64_ARCH         =
OSX_ARM64_RM           = $(OSX_RM)
OSX_ARM64_MKDIR        = $(OSX_MKDIR)
OSX_ARM64_CP           = $(OSX_CP)
OSX_ARM64_DEBUGGER     = $(OSX_DEBUGGER)
OSX_ARM64_CC           = $(OSX_CC)
OSX_ARM64_CXX          = $(OSX_CXX)
OSX_ARM64_CFLAGS       = $(OSX_ARM64_ARCH) $(OSX_CFLAGS)
OSX_ARM64_debug_CFLAGS    =
OSX_ARM64_release_CFLAGS  =
OSX_ARM64_LD           = $(OSX_LD)
OSX_ARM64_LD_CXX       = $(OSX_LD_CXX)
OSX_ARM64_SLD          = $(OSX_SLD)
OSX_ARM64_AR           = $(OSX_AR)
OSX_ARM64_LDFLAGS      = $(OSX_ARM64_ARCH) -o $@
OSX_ARM64_SLDFLAGS     = $(OSX_ARM64_ARCH) -dynamiclib -dylib  -install_name @rpath/$@ -headerpad_max_install_names -o $@
OSX_ARM64_ARFLAGS      = $(OSX_ARFLAGS)
OSX_ARM64_LDLIBS       = -lpthread -ldl 
OSX_ARM64_OBJSUFX      = $(OSX_OBJSUFX)
OSX_ARM64_EXESUFX      = $(OSX_EXESUFX)
OSX_ARM64_SHLPRFX      = $(OSX_SHLPRFX)
OSX_ARM64_SHLSUFX      = $(OSX_SHLSUFX)
OSX_ARM64_STLPRFX      = $(OSX_STLPRFX)
OSX_ARM64_STLSUFX      = $(OSX_STLSUFX)
OSX_ARM64_debug_STRIP  = $(OSX_debug_STRIP)
OSX_ARM64_release_STRIP        = $(OSX_release_STRIP)
OSX_ARM64_OUT          = $(OSX_OUT)
OSX_ARM64_CFLAGS2	= $(OSX_CFLAGS2)
OSX_ARM64_LDFLAGS2	= $(OSX_LDFLAGS2)
OSX_ARM64_SLDFLAGS2	= $(OSX_SLDFLAGS2)
OSX_ARM64_MT		= true


#--- GCC definitions on Linux s390 32 bit
S390_LINUX_RM             = rm -f
S390_LINUX_MKDIR          = [ -d $@ ] || mkdir -p
S390_LINUX_CP             = cp -f
S390_LINUX_DEBUGGER       = ddd
S390_LINUX_CC             = $(LINUX_CC)
S390_LINUX_CXX            = $(LINUX_CXX)
S390_LINUX_CFLAGS         = -m31 $(LINUX_$(CONFIG)_CFLAGS) -D_REENTRANT -fno-strict-aliasing -fno-exceptions -fPIC -Wall -c
S390_LINUX_debug_CFLAGS    =
S390_LINUX_release_CFLAGS  =
S390_LINUX_CXXFLAGS       = -m31 $(LINUX_$(CONFIG)_CFLAGS) -D_REENTRANT -fno-strict-aliasing -fPIC -Wall -c
S390_LINUX_LD             = $(S390_LINUX_CC)
S390_LINUX_LD_CXX         = $(S390_LINUX_CXX)
S390_LINUX_SLD            = $(S390_LINUX_LD)
S390_LINUX_AR             = ar
S390_LINUX_LDFLAGS        = -m31 $(LINUX_OUT) $@

S390_LINUX_SLDFLAGS       = -m31 -shared -Wl,-soname,$@  -Wl,--rpath,\$$ORIGIN $(LINUX_OUT) $@
S390_LINUX_ARFLAGS        = $(LINUX_ARFLAGS)
S390_LINUX_LDLIBS         = $(LINUX_LDLIBS)
S390_LINUX_OBJSUFX        = .o
S390_LINUX_EXESUFX        =
S390_LINUX_SHLPRFX        = lib
S390_LINUX_SHLSUFX        = .so
S390_LINUX_STLPRFX        = lib
S390_LINUX_STLSUFX        = .a
S390_LINUX_debug_STRIP    = touch
S390_LINUX_release_STRIP  = strip
S390_LINUX_OUT            = $(LINUX_OUT) 
S390_LINUX_CFLAGS2        = $(S390_LINUX_CFLAGS)
S390_LINUX_LDFLAGS2       = $(S390_LINUX_LDFLAGS)
S390_LINUX_SLDFLAGS2      = $(S390_LINUX_SLDFLAGS)
S390_LINUX_MT		  = true

#--- GCC definitions on Linux S390Xx, 64 bit)
S390X_LINUX_RM             = rm -f
S390X_LINUX_MKDIR          = [ -d $@ ] || mkdir -p
S390X_LINUX_CP             = cp -f
S390X_LINUX_DEBUGGER       = ddd
S390X_LINUX_CC             = $(LINUX_CC)
S390X_LINUX_CXX            = $(LINUX_CXX)
S390X_LINUX_CFLAGS         = $(LINUX64_CFLAGS)
S390X_LINUX_debug_CFLAGS    =
S390X_LINUX_release_CFLAGS  =
S390X_LINUX_CXXFLAGS       = $(LINUX64_CXXFLAGS)
S390X_LINUX_LD             = $(S390X_LINUX_CC)
S390X_LINUX_LD_CXX         = $(S390X_LINUX_CXX)
S390X_LINUX_SLD            = $(S390X_LINUX_LD)
S390X_LINUX_AR             = ar
S390X_LINUX_LDFLAGS        = $(LINUX64_LDFLAGS)
S390X_LINUX_SLDFLAGS       = $(LINUX64_SLDFLAGS) -z noexecstack
S390X_LINUX_ARFLAGS        = $(LINUX_ARFLAGS)
S390X_LINUX_LDLIBS         = $(LINUX_LDLIBS)
S390X_LINUX_OBJSUFX        = .o
S390X_LINUX_EXESUFX        =
S390X_LINUX_SHLPRFX        = lib
S390X_LINUX_SHLSUFX        = .so
S390X_LINUX_STLPRFX        = lib
S390X_LINUX_STLSUFX        = .a
S390X_LINUX_debug_STRIP    = touch
S390X_LINUX_release_STRIP  = strip
S390X_LINUX_OUT            = $(LINUX_OUT)
S390X_LINUX_CFLAGS2        = $(S390X_LINUX_CFLAGS)
S390X_LINUX_LDFLAGS2       = $(S390X_LINUX_LDFLAGS)
S390X_LINUX_SLDFLAGS2      = $(S390X_LINUX_SLDFLAGS)
S390X_LINUX_MT		   = true
S390X_LINUX_MUPPET         = $(UNIX_MUPPET)


#--- c89 definitions on z/OS 64 bit
ZOS_DEBUGGER       = 
# cc (not c89 anymore from z/OS 1.6 on) # could be cc
ZOS_CC             = c99

# xlc++ has trouble with envelope.c. We use a cpp compiler for testing. It doesn't like the openssl headers.
#ZOS_CXX            = xlc++
ZOS_CXX            = c99 -+

ZOS_debug_CFLAGS   = -g
ZOS_release_CFLAGS = -O3 -Wc,strict,hgpr
ZOS_CFLAGS         = $(ZOS_$(CONFIG)_CFLAGS) -DCHARSET_EBCDIC -DOPEN_THREADS=2 -D_XOPEN_SOURCE=600 -D__SUSV3 -Wc,langlvl\(extended\),dll,xplink,lp64 -Wl,dll,xplink,lp64 -c 
# Unfortunately, I had to add the dll flags HERE, or else icclib.c 
# and similar files exported no symbols... -D_REENTRANT -c 
ZOS_LD             = $(ZOS_CC)
# Correct, use the C compiler to link.
ZOS_LD_CXX         = $(ZOS_CC)
ZOS_SLD            = $(ZOS_LD)
ZOS_AR             = ar
ZOS_LDFLAGS        = -Wl,dll,xplink,lp64 $(ZOS_OUT) $@ 
ZOS_SLDFLAGS       = -Wc,dll,xplink,lp64 -Wl,dll,xplink,lp64 $(ZOS_LDFLAGS) 
ZOS_ARFLAGS        = -ruv $@
ZOS_LDLIBS         = 
ZOS_OBJSUFX        = .o
ZOS_EXESUFX        =
ZOS_SHLPRFX        = lib
ZOS_SHLSUFX        = .so
ZOS_STLPRFX        = lib
ZOS_STLSUFX        = .a
ZOS_debug_STRIP    = touch
ZOS_release_STRIP  = strip
ZOS_OUT            = $(LINUX_OUT)
ZOS_CFLAGS2        = $(ZOS_CFLAGS)
ZOS_LDFLAGS2       = $(ZOS_LDFLAGS)
ZOS_SLDFLAGS2      = $(ZOS_SLDFLAGS)
ZOS_MT  	   = true
ZOS_MUPPET         = $(UNIX_MUPPET)


#--- c89 definitions on z/OS 64 bit
ZOSA_DEBUGGER       = 
# cc (not c89 anymore from z/OS 1.6 on) # could be cc
ZOSA_CC             = $(ZOS_CC)
ZOSA_CXX            = $(ZOS_CXX)
ZOSA_debug_CFLAGS   = -g
ZOSA_release_CFLAGS = -O3 -Wc,strict,hgpr
ZOSA_CFLAGS         = $(ZOSA_$(CONFIG)_CFLAGS) -DOPEN_THREADS=2 -D_XOPEN_SOURCE=600 -D__SUSV3 -Wc,langlvl\(extended\),dll,xplink,lp64,ASCII -Wl,dll,xplink,lp64 -c 
ZOSA_CXXFLAGS       = $(ZOSA_CFLAGS)
# Unfortunately, I had to add the dll flags HERE, or else icclib.c 
# and similar files exported no symbols... -D_REENTRANT -c 
ZOSA_LD             = $(ZOSA_CC)
# Correct, use the C compiler to link.
ZOSA_LD_CXX         = $(ZOSA_CC)
ZOSA_SLD            = $(ZOSA_LD)
ZOSA_AR             = ar
ZOSA_LDFLAGS        = -Wl,dll,xplink,lp64 $(ZOS_OUT) $@ 
ZOSA_SLDFLAGS       = -Wc,dll,xplink,lp64 -Wl,dll,xplink,lp64 $(ZOS_LDFLAGS) 
ZOSA_ARFLAGS        = -ruv $@
ZOSA_LDLIBS         = 
ZOSA_OBJSUFX        = .o
ZOSA_EXESUFX        =
ZOSA_SHLPRFX        = lib
ZOSA_SHLSUFX        = .so
ZOSA_STLPRFX        = lib
ZOSA_STLSUFX        = .a
ZOSA_debug_STRIP    = touch
ZOSA_release_STRIP  = strip
ZOSA_OUT            = $(LINUX_OUT)
ZOSA_CFLAGS2        = $(ZOSA_CFLAGS)
ZOSA_LDFLAGS2       = $(ZOSA_LDFLAGS)
ZOSA_SLDFLAGS2      = $(ZOSA_SLDFLAGS)
ZOSA_MT  	   = true
ZOSA_MUPPET         = $(UNIX_MUPPET)
# Somehow invoking ksh primes the terminal to expect ascii output from icctest and automatically converts it.
ZOSA_ICC_RUN_SETUP = ksh --version;

#--- c89 definitions on z/OS 31 bit
ZOS31_DEBUGGER       = 
# cc (not c89 anymore from z/OS 1.6 on) # could be cc
ZOS31_CC             = $(ZOS_CC)
ZOS31_CXX            = $(ZOS_CXX)
ZOS31_debug_CFLAGS   = -g
ZOS31_release_CFLAGS = -O3 -Wc,strict,hgpr
ZOS31_CFLAGS         = $(ZOS31_$(CONFIG)_CFLAGS) -DCHARSET_EBCDIC -DOPEN_THREADS=2 -D_XOPEN_SOURCE=600 -D__SUSV3 -Wc,ARCH\(9\),langlvl\(extended\),dll,xplink -Wl,dll,xplink -c 
# Unfortunately, I had to add the dll flags HERE, or else icclib.c 
# and similar files exported no symbols... -D_REENTRANT -c 
ZOS31_LD             = $(ZOS31_CC)
# Correct, use the C compiler to link.
ZOS31_LD_CXX         = $(ZOS31_CC)
ZOS31_SLD            = $(ZOS31_LD)
ZOS31_AR             = ar
ZOS31_LDFLAGS        = -Wl,dll,xplink $(ZOS31_OUT) $@ 
ZOS31_SLDFLAGS       = -Wc,dll,xplink -Wl,dll,xplink $(ZOS31_LDFLAGS) 
ZOS31_ARFLAGS        = -ruv $@
ZOS31_LDLIBS         = 
ZOS31_OBJSUFX        = .o
ZOS31_EXESUFX        =
ZOS31_SHLPRFX        = lib
ZOS31_SHLSUFX        = .so
ZOS31_STLPRFX        = lib
ZOS31_STLSUFX        = .a
ZOS31_debug_STRIP    = touch
ZOS31_release_STRIP  = strip
ZOS31_OUT            = $(LINUX_OUT)
ZOS31_CFLAGS2        = $(ZOS31_CFLAGS)
ZOS31_LDFLAGS2       = $(ZOS31_LDFLAGS)
ZOS31_SLDFLAGS2      = $(ZOS31_SLDFLAGS)
ZOS31_MT  	   = true

#--- c89 definitions on z/OS 31 bit ASCII mode
ZOSA31_DEBUGGER       = 
# cc (not c89 anymore from z/OS 1.6 on) # could be cc
ZOSA31_CC             = $(ZOS_CC)
ZOSA31_CXX            = $(ZOS_CXX)
ZOSA31_debug_CFLAGS   = -g
ZOSA31_release_CFLAGS = -O3 -Wc,strict,hgpr
ZOSA31_CFLAGS         = $(ZOSA31_$(CONFIG)_CFLAGS) -DOPEN_THREADS=2 -D_XOPEN_SOURCE=600 -D__SUSV3 -Wc,ARCH\(9\),langlvl\(extended\),dll,xplink,ASCII -Wl,dll,xplink -c 
# Unfortunately, I had to add the dll flags HERE, or else icclib.c 
# and similar files exported no symbols... -D_REENTRANT -c 
ZOSA31_LD             = $(ZOSA31_CC)
# Correct, use the C compiler to link.
ZOSA31_LD_CXX         = $(ZOSA31_CC)
ZOSA31_SLD            = $(ZOSA31_LD)
ZOSA31_AR             = ar
ZOSA31_LDFLAGS        = -Wl,dll,xplink $(ZOSA31_OUT) $@ 
ZOSA31_SLDFLAGS       = -Wc,dll,xplink -Wl,dll,xplink $(ZOSA31_LDFLAGS) 
ZOSA31_ARFLAGS        = -ruv $@
ZOSA31_LDLIBS         = 
ZOSA31_OBJSUFX        = .o
ZOSA31_EXESUFX        =
ZOSA31_SHLPRFX        = lib
ZOSA31_SHLSUFX        = .so
ZOSA31_STLPRFX        = lib
ZOSA31_STLSUFX        = .a
ZOSA31_debug_STRIP    = touch
ZOSA31_release_STRIP  = strip
ZOSA31_OUT            = $(LINUX_OUT)
ZOSA31_CFLAGS2        = $(ZOS31_CFLAGS)
ZOSA31_LDFLAGS2       = $(ZOS31_LDFLAGS)
ZOSA31_SLDFLAGS2      = $(ZOS31_SLDFLAGS)
ZOSA31_MT  	   = true
ZOSA31_ICC_RUN_SETUP = ksh --version;

#--- end of c89 definitions on z/OS


#--- VisualAge definitions on AIX
AIX_RM             = rm -f
AIX_MKDIR          = [ -d $@ ] || mkdir -p
AIX_CP             = cp -f
AIX_DEBUGGER       = idebug
AIX_CC             = xlc_r
AIX_CXX            = xlc++_r
AIX_debug_CFLAGS   = -g -qdbxextra -qthreaded
AIX_release_CFLAGS = -O -qmaxmem=16384 -qtbtable=full -qthreaded
AIX_CFLAGS         = $(AIX_$(CONFIG)_CFLAGS) -c
AIX_CXXFLAGS       = $(AIX_CFLAGS)
AIX_LD             = xlc_r
AIX_SLD            = xlc_r
AIX_LD_CXX         = $(AIX_CXX)
AIX_AR             = ar
AIX_LDFLAGS        = $(AIX_OUT) $@
AIX_SLDFLAGS       = -G $(AIX_LDFLAGS)
AIX_ARFLAGS        = -ruv $@
AIX_LDLIBS         = -lpthread -ldl
AIX_OBJSUFX        = .o
AIX_EXESUFX        =
AIX_SHLPRFX        = lib
AIX_SHLSUFX        = .so
AIX_STLPRFX        = lib
AIX_STLSUFX        = .a
AIX_debug_STRIP    = touch
AIX_release_STRIP  = strip
AIX_OUT            = $(LINUX_OUT)
AIX_CFLAGS2        = $(AIX_CFLAGS)
AIX_LDFLAGS2       = $(AIX_LDFLAGS)
AIX_SLDFLAGS2      = $(AIX_SLDFLAGS)
AIX_MT		   = true
AIX_ICCLIB_LNK     =  -binitfini:iccSLInit:iccSLFini:0

#--- VisualAge definitions on AIX
AIX64_RM             = rm -f
AIX64_MKDIR          = [ -d $@ ] || mkdir -p
AIX64_CP             = cp -f
AIX64_DEBUGGER       = idebug
AIX64_CC             = $(AIX_CC)
AIX64_CXX            = $(AIX_CXX)
AIX64_debug_CFLAGS   = -g -q64 -qdbxextra -qthreaded
AIX64_release_CFLAGS = -O -q64 -qmaxmem=16384 -qtbtable=full -qthreaded
AIX64_CFLAGS         = $(AIX64_$(CONFIG)_CFLAGS)  -c 
AIX64_CXXFLAGS       = -q64 -c
AIX64_LD             = $(AIX_LD)
AIX64_SLD            = $(AIX_SLD)
AIX64_LD_CXX         = $(AIX64_CXX)
AIX64_AR             = ar
AIX64_LDFLAGS        = -q64 $(AIX64_OUT) $@
AIX64_SLDFLAGS       = -G $(AIX64_LDFLAGS)
AIX64_ARFLAGS        = -X64 -ruv $@
AIX64_LDLIBS         = -b64 -lpthread -ldl
AIX64_OBJSUFX        = .o
AIX64_EXESUFX        =
AIX64_SHLPRFX        = lib
AIX64_SHLSUFX        = .so
AIX64_STLPRFX        = lib
AIX64_STLSUFX        = .a
AIX64_debug_STRIP    = touch
AIX64_release_STRIP  = strip -X64
AIX64_OUT            = $(LINUX_OUT)
AIX64_CFLAGS2        = $(AIX64_CFLAGS)
AIX64_LDFLAGS2       = $(AIX64_LDFLAGS)
AIX64_SLDFLAGS2      = $(AIX64_SLDFLAGS)
AIX64_MT	     = true
AIX64_ICCLIB_LNK     = -binitfini:iccSLInit:iccSLFini:0
AIX64_MUPPET         = $(UNIX_MUPPET)

#--- Power hypervisor
POWERH_RM             = rm -f
POWERH_MKDIR          = [ -d $@ ] || mkdir -p
POWERH_CP             = cp -f
POWERH_DEBUGGER       = idebug
POWERH_CC             = xlc_r
POWERH_CXX            = xlc_r++
POWERH_release_CFLAGS = -bstatic -O -qlanglvl=offsetnonpod:nonewexcp:noansifor:nognu_warning:noimplicitint:zeroextarray -qnortti -qnostdinc -qnolib -qlist -qasm -qsource -qlistopt -qarch=pwr6 -qenum=8 -q64 -qnoeh -qdebug=npage0 -qxflag=kernel_node -qdebug=NFPCONC -qidirfirst -qhalt=e 
POWERH_debug_CFLAGS   = $(POWERH_release_CFLAGS)
POWERH_CFLAGS         = $(POWERH_$(CONFIG)_CFLAGS)  -c
POWERH_CXXFLAGS       = -c
POWERH_LD             = xlc_r
POWERH_SLD            = xlc_r
POWERH_LD_CXX         = $(POWERH_CXX)
POWERH_AR             = ar
POWERH_LDFLAGS        = -q64 $(POWERH_OUT) $@
POWERH_SLDFLAGS       = -G $(POWERH_LDFLAGS)
POWERH_ARFLAGS        = -X64 -ruv $@
POWERH_LDLIBS         = -b64 
POWERH_OBJSUFX        = .o
POWERH_EXESUFX        =
POWERH_SHLPRFX        = lib
POWERH_SHLSUFX        = .so
POWERH_STLPRFX        = lib
POWERH_STLSUFX        = .a
POWERH_debug_STRIP    = touch
POWERH_release_STRIP  = strip -X64
POWERH_OUT            = $(LINUX_OUT)
POWERH_CFLAGS2        = $(POWERH_CFLAGS)
POWERH_LDFLAGS2       = $(POWERH_LDFLAGS)
POWERH_SLDFLAGS2      = $(POWERH_SLDFLAGS)
POWERH_MT	     = true
POWERH_ICCLIB_LNK     = -binitfini:iccSLInit:iccSLFini:0

# default all platforms to current setting /usr/bin/sh
# := prevents assignment loop
$(OPSYS)_SHELL:=$(SHELL)
# SUN's sh is not handling 'if' properly so use ksh
SUN_SHELL=/usr/bin/ksh
SUN_AMD64_SHELL=$(SUN_SHELL)
SUN64_SHELL=$(SUN_SHELL)
SUN_X86_SHELL=$(SUN_SHELL)

#--- Forte definitions on SUN
SUN_RM             = rm -f
SUN_MKDIR          = [ -d $@ ] || mkdir -p
SUN_CP             = cp -f
SUN_CC             = cc
SUN_CXX            = cc
SUN_DEBUGGER       = workshop -D
SUN_debug_CFLAGS   = -g -xs
SUN_release_CFLAGS = -xO5
SUN_CFLAGS         = $(SUN_$(CONFIG)_CFLAGS) -mt -KPIC -c
SUN_LD             = cc
SUN_LD_CXX         = cc
SUN_SLD            = cc
SUN_AR             = ar
SUN_LDFLAGS        = -mt $(SUN_OUT) $@
SUN_SLDFLAGS       = -G $(SUN_LDFLAGS) -R \$$ORIGIN
SUN_ARFLAGS        = -ruv $@
SUN_LDLIBS         = -lrt -lpthread -ldl -lsocket
SUN_OBJSUFX        = .o
SUN_EXESUFX        =
SUN_SHLPRFX        = lib
SUN_SHLSUFX        = .so
SUN_STLPRFX        = lib
SUN_STLSUFX        = .a
SUN_debug_STRIP    = touch
SUN_release_STRIP  = strip
SUN_OUT            = $(LINUX_OUT)
SUN_CFLAGS2        = $(SUN_CFLAGS)
SUN_LDFLAGS2       = $(SUN_LDFLAGS)
SUN_SLDFLAGS2      = $(SUN_SLDFLAGS)
SUN_MT		   = true

#--- Forte definitions on SUN Solaris x86
SUN_X86_RM             = rm -f
SUN_X86_MKDIR          = [ -d $@ ] || mkdir -p
SUN_X86_CP             = cp -f
SUN_X86_CC             = $(LINUX_CC)
SUN_X86_CXX            = $(LINUX_CXX)
SUN_X86_DEBUGGER       = workshop -D
SUN_X86_debug_CFLAGS   = -g -O3 -m32 -fPIC
SUN_X86_release_CFLAGS = -O3 -m32 -fPIC
SUN_X86_CFLAGS         = $(SUN_X86_$(CONFIG)_CFLAGS) -c
SUN_X86_CXXFLAGS       = $(SUN_X86_CFLAGS)
SUN_X86_LD             = cc
SUN_X86_LD_CXX         = $(LINUX_LD_CXX)
SUN_X86_SLD            = cc
SUN_X86_AR             = ar
# Needed to strip h/ware capability bits from libcrypto and openssl
SUN_X86_LDFLAGS        =  -mt -m32 $(SUN_X86_OUT) $@
SUN_X86_SLDFLAGS       = -G $(SUN_X86_LDFLAGS) -R \$$ORIGIN
SUN_X86_ARFLAGS        = -ruv $@
SUN_X86_LDLIBS         = -lrt -lpthread -ldl -lsocket
SUN_X86_OBJSUFX        = .o
SUN_X86_EXESUFX        =
SUN_X86_SHLPRFX        = lib
SUN_X86_SHLSUFX        = .so
SUN_X86_STLPRFX        = lib
SUN_X86_STLSUFX        = .a
SUN_X86_debug_STRIP    = touch
SUN_X86_release_STRIP  = strip
SUN_X86_OUT            = $(LINUX_OUT)
SUN_X86_OUT            = $(LINUX_OUT)
SUN_X86_CFLAGS2        = $(SUN_X86_CFLAGS)
SUN_X86_LDFLAGS2       = $(SUN_X86_LDFLAGS)
SUN_X86_SLDFLAGS2       = $(SUN_X86_SLDFLAGS)
SUN_X86_MT		= true


#--- Forte definitions on SUN64
SUN64_RM             = rm -f
SUN64_MKDIR          = [ -d $@ ] || mkdir -p
SUN64_CP             = cp -f
SUN64_CC             = cc
SUN64_CXX             = cc
SUN64_DEBUGGER       = workshop -D
SUN64_debug_CFLAGS   = -g  -xs 
SUN64_release_CFLAGS = -O
SUN64_CFLAGS         = $(SUN64_$(CONFIG)_CFLAGS) -m64 -xstrconst -mt -KPIC  -DB_ENDIAN -D_REENTRANT -c
SUN64_CXXFLAGS       = $(SUN64_CFLAGS)
SUN64_LD             = $(SUN64_CC) 
SUN64_LD_CXX	     = $(SUN64_CXX) 
SUN64_SLD            = $(SUN64_LD) 
SUN64_AR             = ar
SUN64_LDFLAGS        = -m64 -mt $(SUN64_OUT) $@  
SUN64_SLDFLAGS       = -G $(SUN64_LDFLAGS)  -R \$$ORIGIN
SUN64_ARFLAGS        = -ruv $@
SUN64_LDLIBS         = -lrt -lpthread -ldl -lsocket
SUN64_OBJSUFX        = .o
SUN64_EXESUFX        =
SUN64_SHLPRFX        = lib
SUN64_SHLSUFX        = .so
SUN64_STLPRFX        = lib
SUN64_STLSUFX        = .a
SUN64_debug_STRIP    = touch
SUN64_release_STRIP  = strip
SUN64_OUT            = $(SUN_OUT)
SUN_OUT              = $(LINUX_OUT)
SUN64_CFLAGS2        = $(SUN64_CFLAGS)
SUN64_LDFLAGS2       = $(SUN64_LDFLAGS)
SUN64_SLDFLAGS2      = $(SUN64_SLDFLAGS)
SUN64_MT		= true

#--- Forte definitions on AMD_64
SUN_AMD64_RM             = rm -f
SUN_AMD64_MKDIR          = [ -d $@ ] || mkdir -p
SUN_AMD64_CP             = cp -f
SUN_AMD64_CC             = cc
SUN_AMD64_CXX            = g++
SUN_AMD64_DEBUGGER       = workshop -D
SUN_AMD64_debug_CFLAGS   = -g  -m64 -xarch=generic -xO5 -xstrconst -KPIC -Xa  -DL_ENDIAN -D_REENTRANT
SUN_AMD64_release_CFLAGS = -xO5 -m64 -xarch=generic -xstrconst -KPIC -Xa  -DL_ENDIAN -D_REENTRANT
SUN_AMD64_CFLAGS         = $(SUN_AMD64_$(CONFIG)_CFLAGS) -mt -KPIC  -c
SUN_AMD64_CXXFLAGS       = -m64 -c
SUN_AMD64_LD             = $(SUN_AMD64_CC)
SUN_AMD64_LD_CXX	     = $(SUN_AMD64_CXX)
SUN_AMD64_SLD            = $(SUN_AMD64_LD)
SUN_AMD64_AR             = ar
SUN_AMD64_LDFLAGS        = -mt $(SUN_AMD64_OUT) $@  -m64 -xarch=generic
SUN_AMD64_LDXXFLAGS      = -mt -m64
SUN_AMD64_SLDFLAGS       = -G $(SUN_AMD64_LDFLAGS) -R \$$ORIGIN
SUN_AMD64_ARFLAGS        = -ruv $@
SUN_AMD64_LDLIBS         = -lrt -lpthread -ldl -lsocket
SUN_AMD64_OBJSUFX        = .o
SUN_AMD64_EXESUFX        =
SUN_AMD64_SHLPRFX        = lib
SUN_AMD64_SHLSUFX        = .so
SUN_AMD64_STLPRFX        = lib
SUN_AMD64_STLSUFX        = .a
SUN_AMD64_debug_STRIP    = touch
SUN_AMD64_release_STRIP  = strip
SUN_AMD64_OUT            = $(SUN_OUT)
SUN_AMD64_CFLAGS2        = $(SUN_AMD64_CFLAGS)
SUN_AMD64_LDFLAGS2       = $(SUN_AMD64_LDFLAGS)
SUN_AMD64_SLDFLAGS2      = $(SUN_AMD64_SLDFLAGS)
SUN_AMD64_MT		= true


#--- HP ANSI C definitions on HPUX
#-     Note: -D_REENTRANT -D_POSIX_C_SOURCE should be replaced with -mt, but we
#            have the older compiler here in Raleigh that doesn't support that
#            compiler option.
HPUX_RM             = rm -f
HPUX_MKDIR          = [ -d $@ ] || mkdir -p
HPUX_CP             = cp -f
HPUX_CC             = cc
HPUX_CXX            = cc
HPUX_DEBUGGER       = dde
HPUX_debug_CFLAGS   = -g +O2 +Osize +Oprocelim
# Keep the opt level low, the OpenSSL build has test failures otherwise
HPUX_release_CFLAGS = +O3 +Osize +Oprocelim
HPUX_CFLAGS         =  $(HPUX_$(CONFIG)_CFLAGS) -AC99 -Ae +ESlit -D_REENTRANT -D_POSIX_C_SOURCE=199506L -mt +Z -c
HPUX_CXXFLAGS       =  $(HPUX_CFLAGS)
HPUX_LD             = $(HPUX_CC)
HPUX_LD_CXX         = $(HPUX_CXX)
HPUX_SLD            = ld
HPUX_AR             = ar
HPUX_LDFLAGS        = $(HPUX_OUT) $@ 
HPUX_SLDFLAGS       = -z -b $(HPUX_LDFLAGS) +b \$$ORIGIN:/usr/lib -B direct -B symbolic +s
HPUX_ICCLIB_LNK     = +I iccSLInitializer	
HPUX_ARFLAGS        = -ruv $@
HPUX_LDLIBS         = -lpthread  -ldld -lrt
HPUX_OBJSUFX        = .o
HPUX_EXESUFX        =
HPUX_SHLPRFX        = lib
HPUX_SHLSUFX        = .sl
HPUX_STLPRFX        = lib
HPUX_STLSUFX        = .a
HPUX_debug_STRIP    = touch
HPUX_release_STRIP  = chmod +w $@; strip
HPUX_OUT            = $(LINUX_OUT)
HPUX_CFLAGS2        = $(HPUX_CFLAGS)
HPUX_LDFLAGS2       = $(HPUX_LDFLAGS)
HPUX_SLDFLAGS2       = $(HPUX_SLDFLAGS)
HPUX_MT			= true

#--- HP ANSI C definitions on HPUX64
HPUX64_RM             = rm -f
HPUX64_MKDIR          = [ -d $@ ] || mkdir -p
HPUX64_CP             = cp -f
HPUX64_CC             = cc  +DD64
HPUX64_CXX	      = cc  +DD64
HPUX64_DEBUGGER       = dde
HPUX64_debug_CFLAGS   = -g
# DO NOT up the opt level, it causes subtle problems, like not exiting 
HPUX64_release_CFLAGS = +O3 +Osize +Oprocelim 
HPUX64_CFLAGS         = $(HPUX64_$(CONFIG)_CFLAGS) -AC99 +Optrs_strongly_typed +Olibcalls -Ae +ESlit -DB_ENDIAN -DMD32_XARRAY -D_POSIX_C_SOURCE=199506L -D_REENTRANT -mt +Z -c
HPUX64_CXXFLAGS       = $(HPUX64_CFLAGS)
HPUX64_LD             = $(HPUX64_CC)
HPUX64_LD_CXX	      = $(HPUX64_CXX)
HPUX64_SLD            = ld
HPUX64_AR             = ar
HPUX64_LDFLAGS        = $(HPUX_LDFLAGS)
HPUX64_SLDFLAGS       = $(HPUX_SLDFLAGS)
HPUX64_ICCLIB_LNK     = $(HPUX_ICCLIB_LNK)
HPUX64_ARFLAGS        = -ruv $@
HPUX64_LDLIBS         = -lpthread -ldld -lrt
HPUX64_OBJSUFX        = .o
HPUX64_EXESUFX        =
HPUX64_SHLPRFX        = lib
HPUX64_SHLSUFX        = .sl
HPUX64_STLPRFX        = lib
HPUX64_STLSUFX        = .a
HPUX64_debug_STRIP   = touch
HPUX64_release_STRIP = chmod +w $@; strip
HPUX64_OUT           = $(HPUX_OUT)
HPUX64_CFLAGS2       = $(HPUX64_CFLAGS)
HPUX64_LDFLAGS2      = $(HPUX64_LDFLAGS)
HPUX64_SLDFLAGS2     = $(HPUX64_SLDFLAGS)
HPUX64_MT	     = true

#--- HP-UX & aCC 64-bit on IA64 
#
#  Note: Will fail if release_CFLAG is changed to -03
#
IA64_HPUX_RM             = rm -f
IA64_HPUX_MKDIR          = [ -d $@ ] || mkdir -p
IA64_HPUX_CP             = cp -f
IA64_HPUX_CC             = aCC
IA64_HPUX_CXX            = aCC
IA64_HPUX_DEBUGGER       = dde
IA64_HPUX_debug_CFLAGS   = -g -DDEBUG
IA64_HPUX_release_CFLAGS = -O2
IA64_HPUX_CFLAGS         = $(IA64_HPUX_$(CONFIG)_CFLAGS) -AC99 -D_POSIX_C_SOURCE=199506L -mt -ext -Ae +O2 +z -c
IA64_HPUX_CXXFLAGS       = $(IA64_HPUX_CFLAGS)
IA64_HPUX_LD             = $(IA64_HPUX_CC)
IA64_HPUX_LD_CXX         = $(IA64_HPUX_CXX)
IA64_HPUX_SLD            = ld
IA64_HPUX_AR             = ar
IA64_HPUX_LDFLAGS        = $(HPUX_LDFLAGS)
IA64_HPUX_SLDFLAGS       = $(HPUX_SLDFLAGS)
IA64_HPUX_ICCLIB_LNK     = $(HPUX_ICCLIB_LNK)
IA64_HPUX_ARFLAGS        = -ruv $@
IA64_HPUX_LDLIBS         =  -lpthread -ldl -lrt
IA64_HPUX_OBJSUFX        = .o
IA64_HPUX_EXESUFX        =
IA64_HPUX_SHLPRFX        = lib
IA64_HPUX_SHLSUFX        = .so
IA64_HPUX_STLPRFX        = lib
IA64_HPUX_STLSUFX        = .a
IA64_HPUX_debug_STRIP    = touch
IA64_HPUX_release_STRIP  = chmod +w $@; strip
IA64_HPUX_OUT            = $(HPUX_OUT)
IA64_HPUX_CFLAGS2        = $(IA64_HPUX_CFLAGS)
IA64_HPUX_LDFLAGS2       = $(IA64_HPUX_LDFLAGS)
IA64_HPUX_SLDFLAGS2       = $(IA64_HPUX_SLDFLAGS)
IA64_HPUX_MT		= true


#--- HP-UX & aCC V5.5x or latter for 64-bit on IA64
#
# @ May/04 this compiles, but icctest fails to load ICC lib
#
IA64_HPUX64_RM              = rm -f
IA64_HPUX64_MKDIR           = [ -d $@ ] || mkdir -p
IA64_HPUX64_CP              = cp -f
IA64_HPUX64_CC              = aCC +DD64
IA64_HPUX64_CXX             = aCC +DD64
IA64_HPUX64_DEBUGGER        = dde
IA64_HPUX64_debug_CFLAGS    = -g -DDEBUG
IA64_HPUX64_release_CFLAGS  = -O3 
IA64_HPUX64_CFLAGS          = $(IA64_HPUX64_$(CONFIG)_CFLAGS) -AC99 -D_REENTRANT -D_POSIX_C_SOURCE=199506L -mt -Ae +z -c
IA64_HPUX64_CXXFLAGS        = $(IA64_HPUX64_CFLAGS)
IA64_HPUX64_LD              = $(IA64_HPUX64_CC)
IA64_HPUX64_LD_CXX          = $(IA64_HPUX64_CXX)
IA64_HPUX64_SLD             = ld
IA64_HPUX64_AR              = ar
IA64_HPUX64_LDFLAGS         = $(HPUX_LDFLAGS)  
IA64_HPUX64_SLDFLAGS        = $(HPUX_SLDFLAGS)  
IA64_HPUX64_ICCLIB_LNK      = $(HPUX_ICCLIB_LNK)
IA64_HPUX64_ARFLAGS         = -ruv $@
IA64_HPUX64_LDLIBS          = -lpthread -ldl -lrt
IA64_HPUX64_OBJSUFX         = .o
IA64_HPUX64_EXESUFX         =
IA64_HPUX64_SHLPRFX         = lib
IA64_HPUX64_SHLSUFX         = .so
IA64_HPUX64_STLPRFX         = lib
IA64_HPUX64_STLSUFX         = .a
IA64_HPUX64_debug_STRIP     = touch
IA64_HPUX64_release_STRIP   = chmod +w $@; strip
IA64_HPUX64_OUT             = $(HPUX_OUT)
IA64_HPUX64_CFLAGS2        = $(IA64_HPUX64_CFLAGS)
IA64_HPUX64_LDFLAGS2       = $(IA64_HPUX64_LDFLAGS)
IA64_HPUX64_SLDFLAGS2       = $(IA64_HPUX64_SLDFLAGS)
IA64_HPUX64_MT			= true

#-- OS400
#-- Note: touch does not work with symlnks
#-- NOTE: add -DNON_FIPS_ICC to CFLAGS and CFLAGS2 to bypass sign/verify of the libraries
#--
OS400_ICC_VER		= 17
OS400_LIB_400		= LIBICC400
OS400_ICCSDK_OUTPUTDIR	= QICCSDK
OS400_OSSL_OUTPUTDIR	 = QOSSL098E
OS400_ICC_OUTPUTDIR	 = ICC
OS400_ICCSPEED_OUTPUTDIR = QICCSPEED
OS400_ICCTEST_OUTPUTDIR  = QICC_TEST
OS400_ICCSHL_OUTPUTDIR 	 = QICC_SHL
OS400_ICCPKG_OUTPUTDIR 	 = QICC$(OS400_ICC_VER)PK
OS400_ICC_LIB		 = QICC$(OS400_ICC_VER)
OS400_RM		= rm -f
OS400_MKDIR		= [ -d $@ ] || mkdir -p
OS400_CP		= cp -H -f
OS400_DEBUGGER   	= ddd
OS400_release_CC	= icc
OS400_debug_CC		= icc
OS400_beam_CC 		= beam_compile
OS400_CC             	= $(OS400_$(CONFIG)_CC)
OS400_release_CXX   	= icc
OS400_debug_CXX   	= icc
OS400_beam_CXX   	= beam_compile
OS400_CXX            	= $(OS400_$(CONFIG)_CXX)
OS400_debug_CFLAGS  	= -c -g -DDEBUG  
OS400_release_CFLAGS 	= -c -O4
OS400_CFLAGS         	=  -v $(OS400_$(CONFIG)_CFLAGS) -DOS400 -D__cplusplus__strings__ -DB_ENDIAN -DNO_SYS_PARAM_H -D_ALL_SOURCE -DNO_SYSLOG -D_REENTRANT -qTGTRLS=V5R3M0 -qLOCALETYPE=*LOCALE -qCSOPT=\"-qconvliteral=850\" -qSTGMDL=*INHERIT -qTERASPACE=*YES -qENUM=*SMALL -I/qibm/proddata/qadrt/include
OS400_CFLAGS2         	=  -v $(OS400_$(CONFIG)_CFLAGS) -DOS400 -D__cplusplus__strings__ -DB_ENDIAN -DNO_SYS_PARAM_H -D_ALL_SOURCE -DNO_SYSLOG -D_REENTRANT -qTGTRLS=V5R3M0 -qLOCALETYPE=*LOCALE  -qENUM=*SMALL -qSTGMDL=*INHERIT -qTERASPACE=*YES
OS400_release_LD     	= ld -v
OS400_debug_LD       	= ld -v
OS400_LD             	= icc 
OS400_LD_CXX         	= icc 
OS400_SLD            	= $(OS400_SETUP_ICC); $(OS400_$(CONFIG)_LD)
OS400_AR             	= ar -v -c -r
OS400_LDFLAGS      	= -v $(OS400_OUT) $@ -qTGTRLS=V5R3M0 -qENTMOD=$(OS400_ICCSDK_OUTPUTDIR)/QADRTMAIN2 -qBNDSRVPGM=$(OS400_ICCSDK_OUTPUTDIR)/QADRTTS
OS400_LDFLAGS2      	= -v $(OS400_OUT) $@ -qTGTRLS=V5R3M0 -qENTMOD=$(OS400_ICCSDK_OUTPUTDIR)/QADRTMAIN2 -qBNDSRVPGM="$(OS400_ICCSDK_OUTPUTDIR)/QADRTTS $(OS400_ICC_LIB)/$(OS400_LIB_400)"
OS400_SLDFLAGS       	= -v $(OS400_OUT) $@ -qTGTRLS=V5R3M0 -qBNDSRVPGM="$(OS400_ICCSDK_OUTPUTDIR)/QADRTTS $(OS400_ICC_LIB)/$(OS400_LIB_400)" -qSTGMDL=*INHERIT -qALWLIBUPD=*NO -qALWUPD=*NO
OS400_SLDFLAGS2       	= -v $(OS400_OUT) $@ -qTGTRLS=V5R3M0 -qBNDSRVPGM=$(OS400_ICCSDK_OUTPUTDIR)/QADRTTS -qSTGMDL=*INHERIT -qALWLIBUPD=*NO -qALWUPD=*NO
OS400_ARFLAGS        	= -ruv $@
OS400_LDLIBS         	= 
OS400_release_OBJSUFX  	= .o
OS400_debug_OBJSUFX  	= .o
OS400_OBJSUFX        	= $(OS400_$(CONFIG)_OBJSUFX)
OS400_EXESUFX        	=
OS400_SHLPRFX        	= lib
OS400_SHLSUFX        	= .so
OS400_STLPRFX        	= lib
OS400_STLSUFX        	= .a
OS400_debug_STRIP   	= chmod +w
OS400_release_STRIP 	= chmod +w
OS400_OUT               = $(LINUX_OUT)
OS400_MT		= true

#-- OS400X (using cross-compiler on AIX)
#-- Note: touch does not work with symlinks
#-- NOTE: add -DNON_FIPS_ICC to CFLAGS and CFLAGS2 to bypass sign/verify of the OS400X_release_IC
OS400X_release_ICC_HOSTBIN  = QICC$(VTAG)
OS400X_debug_ICC_HOSTBIN    = DICC$(VTAG)
OS400X_release_ICC_SDKLIB   = QICCSDK$(VTAG)
OS400X_debug_ICC_SDKLIB     = DICCSDK$(VTAG)
OS400X_CLEAN400_ICC   = rmlib400x -s $(HOSTSYS) $(OS400X_$(CONFIG)_ICC_HOSTBIN) $(OS400X_$(CONFIG)_ICC_SDKLIB)
OS400X_CLEAN400_MODS  = rmlib400x -s $(HOSTSYS) -p $(HOSTPFX)
OS400X_EX_SUFFIX      = $(CLEARCASE_DIREXT)
OS400X_RM             = rm -f
OS400X_MKDIR          = [ -d $@ ] || mkdir -p
OS400X_CP             = cp -f
##OS400X_DEBUGGER     = idebug
OS400X_CC             = xlc400x
OS400X_CXX            = xlC400x
OS400X_debug_CFLAGS   = -g -DDEBUG
OS400X_release_CFLAGS = -O -B40 -Q
OS400X_CFLAGS_MISC    = -DICC_HOSTBIN=\"$(OS400X_$(CONFIG)_ICC_HOSTBIN)\"  -DOSSL_HOSTBIN=\"$(OS400X_$(CONFIG)_OSSL_HOSTBIN)\"
# CFLAGS(2) is for ascii modules
OS400X_CFLAGS         = $(OS400X_$(CONFIG)_CFLAGS) -qtgtccsid=850 -qadrt -DOS400 -DCUEO $(OS400X_CFLAGS_MISC)
OS400X_CFLAGS2        = $(OS400X_CFLAGS)
# CFLAGSE is for ebcdic modules
OS400X_CFLAGSE        = $(OS400X_$(CONFIG)_CFLAGS) -DOS400 -DCUEO $(OS400X_CFLAGS_MISC)
# Specify library name for SDK modules
OS400X_SDKFLAGS       = -lib=$(OS400X_$(CONFIG)_ICC_SDKLIB)
# ?? Do we need -B -Q -g for ld400x ??
# "LD" defines are for linking programs
# "SLD" defines are for linking service programs
OS400X_LD             = ld400x -H $(OS400X_$(CONFIG)_ICC_HOSTBIN)
OS400X_SLD            = ld400x -H $(OS400X_$(CONFIG)_ICC_HOSTBIN)
OS400X_LD_CXX         = ld400x -H $(OS400X_$(CONFIG)_ICC_HOSTBIN)
OS400X_AR             = ar
OS400X_LDFLAGS        = -qadrt -o $@
OS400X_LDFLAGS2       = $(OS400X_LDFLAGS)
# for ebcdic programs
OS400X_LDFLAGSE       = -o $@
OS400X_SLDFLAGS       = -qadrt -b 'ALWLIBUPD(*NO)' -b 'ALWUPD(*NO)' -o $@
OS400X_SLDFLAGS2      = $(OS400X_SLDFLAGS)
OS400X_ARFLAGS        = -ruv $@
OS400X_LDLIBS         =
OS400X_OBJSUFX        = .o
OS400X_EXESUFX        =
OS400X_SHLPRFX        = lib
OS400X_SHLSUFX        = .so
OS400X_STLPRFX        = lib
OS400X_STLSUFX        = .a
OS400X_LIBOBJS        = getnmi$(OS400X_OBJSUFX)
# following are noop operations on proxy files
OS400X_debug_STRIP   = touch
OS400X_release_STRIP = touch
OS400X_OUT           = $(LINUX_OUT)
OS400X_MT		= true

#--- Map platform specific definitions to global definitions

SHELL          = $($(OPSYS)_SHELL)
RM             = $($(OPSYS)_RM)
MKDIR          = $($(OPSYS)_MKDIR)
CP             = $($(OPSYS)_CP)
DEBUGGER       = $($(OPSYS)_DEBUGGER)
CC             = $($(OPSYS)_CC)
CXX            = $($(OPSYS)_CXX)
CFLAGS         = $($(OPSYS)_CFLAGS) $($(BUILD)_CFLAGS)
CXXFLAGS       = $($(OPSYS)_CXXFLAGS)
LD             = $($(OPSYS)_LD)
LD_CXX         = $($(OPSYS)_LD_CXX)
SLD            = $($(OPSYS)_SLD)
AR             = $($(OPSYS)_AR)
LDFLAGS        = $($(OPSYS)_LDFLAGS)
LDXXFLAGS      = $($(OPSYS)_LDXXFLAGS)
SLDFLAGS       = $($(OPSYS)_SLDFLAGS)
ARFLAGS        = $($(OPSYS)_ARFLAGS)
LDLIBS         = $($(OPSYS)_LDLIBS)
OBJSUFX        = $($(OPSYS)_OBJSUFX)
EXESUFX        = $($(OPSYS)_EXESUFX)
SHLPRFX        = $($(OPSYS)_SHLPRFX)
SHLSUFX        = $($(OPSYS)_SHLSUFX)
STLPRFX        = $($(OPSYS)_STLPRFX)
STLSUFX        = $($(OPSYS)_STLSUFX)
STRIP          = $($(OPSYS)_$(CONFIG)_STRIP)
ICC_RUN_SETUP  = $($(OPSYS)_ICC_RUN_SETUP)

CFLAGS2         = $($(OPSYS)_CFLAGS2)
SLDFLAGS2       = $($(OPSYS)_SLDFLAGS2)
ICCLIB_LNK      = $($(OPSYS)_ICCLIB_LNK)
LDFLAGS2        = $($(OPSYS)_LDFLAGS2)
CFLAGSE         = $($(OPSYS)_CFLAGSE)
LDFLAGSE        = $($(OPSYS)_LDFLAGSE)

OUT             = $($(OPSYS)_OUT)
MT		= $($(OPSYS)_MT)

SDKFLAGS        = $($(OPSYS)_SDKFLAGS)
EX_SUFFIX       = $($(OPSYS)_EX_SUFFIX)

CLEAN400_ICC    = $($(OPSYS)_CLEAN400_ICC)
CLEAN400_MODS   = $($(OPSYS)_CLEAN400_MODS)

# muppet.mk needs this definition, maily used in iccpkg
$(OPSYS)_OLD_ICC	= OLD_ICC/$(OPSYS)
# avoid directory rename here - WIN_X86_64 can stay
WIN64_VS2022_OLD_ICC	= OLD_ICC/WIN_X86_64
WIN32_VS2022_OLD_ICC	= OLD_ICC/WIN32_VS2013
OLD_ICC		= $($(OPSYS)_OLD_ICC)

# MUPPET is set by the build system based on FIPS module presence and it also writes gsk_wrap2_a.c to match
# if we reset it here then we will be inconsistent with gsk_wrap2_a.c which links in FIPS ICCC_ prefix APIs
#MUPPET = $($(OPSYS)_MUPPET)
