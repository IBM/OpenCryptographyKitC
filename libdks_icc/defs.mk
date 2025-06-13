# set defaults to be LINUX where OS=, undefined  or some unknown value, use WIN defaults if "WIN" is found in OS

# From Makefile:
# LINUX_CFLAGS += -Wall -Wextra -Wpedantic -Wmissing-prototypes -Wredundant-decls \
  -Wshadow -Wpointer-arith -O3 -fomit-frame-pointer -z noexecstack
# We remove non-essential flags that cause problems on some official build platforms.
# Inherit cflags and nistflags and append/remove flags depending on what we need.
# If there is no $(OS)_CFLAGS i.e AMD64_LINUX_CFLAGS set it to the $(OSLIKE)_CFLAGS) base. Then CFLAGS is set to our OS specfici (AMD64 etc) version.
# Note LINUX_CFLAGS is used for OSLIKE and then OS for ia32.

# main ICC makefiles use OPSYS, PQC uses OS so make sure both are set
ifeq ($(OPSYS),)
OPSYS=$(OS)
endif

OSLIKE=LINUX
ifeq ($(findstring WIN, $(OS)),WIN)
OSLIKE=WIN
endif

include $(ICC_ROOT)/icc/opensslver.mk
include $(ICC_ROOT)/icc/icc_defs.mk
include $(ICC_ROOT)/icc/platforms.mk

# installed SDKs are outside of icc dir
SDK_PATH=../../../..
#OPENSSL_SDK_PATH=$(SDK_PATH)/openssl
# use the one we build for this platform
OPENSSL_SDK_PATH=../../../openssl-1.1.1

LINUX_CFLAGS += -fPIC
LINUX_NISTFLAGS += -fPIC

ifeq ($(CONFIG), debug)
LINUX_CFLAGS += -g
LINUX_NISTFLAGS += -g
endif

# Expliticly set for if platform specific flag modifications are needed
# linux64 needs c99 for nistkat/*.c on phelix
AMD64_LINUX_CFLAGS := -std=gnu99 $(filter-out -Wpedantic -Wvla -m32,$(LINUX_CFLAGS))
LINUX_CFLAGS := -std=gnu99 -m32 $(filter-out -Wpedantic -Wvla -Wno-unused-result -m32,$(LINUX_CFLAGS))
PPC64_LINUX_CFLAGS := -m64 $(filter-out -Wpedantic -Wvla -z noexecstack -m32,$(LINUX_CFLAGS))
S390X_LINUX_CFLAGS := -std=c99 $(filter-out -Wpedantic -Wvla -m32,$(LINUX_CFLAGS))
AIX64_CFLAGS := -q64 -qmaxmem=16384 -qtbtable=full -qthreaded $(filter-out -fPIC -Wall -Wextra -Wpedantic -Wmissing-prototypes -Wredundant-decls -Wshadow -Wpointer-arith -Wvla -Wconversion -fomit-frame-pointer -z noexecstack -m32,$(LINUX_CFLAGS))

AMD64_LINUX_NISTFLAGS := $(AMD64_LINUX_CFLAGS)
LINUX_NISTFLAGS := $(LINUX_CFLAGS)
PPC64_LINUX_NISTFLAGS := $(PPC64_LINUX_CFLAGS)
S390X_LINUX_NISTFLAGS := $(S390X_LINUX_CFLAGS)
AIX64_NISTFLAGS := $(AIX64_CFLAGS)

# win-like defaults
WIN_CFLAGS=-GS -Zi
WIN_debug_CFLAGS=-MDd
WIN_release_CFLAGS=-MD
WIN_CFLAGS+=$(WIN_$(CONFIG)_CFLAGS)

WIN_NISTFLAGS=$(WIN_CFLAGS)

ifeq ($(strip $($(OS)_CFLAGS)),)
$(OS)_CFLAGS:=$($(OSLIKE)_CFLAGS)
endif
CFLAGS=$($(OS)_CFLAGS) -I $(OPENSSL_SDK_PATH)/include

ifeq ($(strip $($(OS)_NISTFLAGS)),)
$(OS)_NISTFLAGS=$($(OSLIKE)_NISTFLAGS)
endif
NISTFLAGS=$($(OS)_NISTFLAGS) -I $(OPENSSL_SDK_PATH)/include

WIN_OUT=-Fe:
LINUX_OUT=-o 
AMD64_LINUX_OUT=$(LINUX_OUT)
ifeq ($(strip $($(OS)_OUT)),)
$(OS)_OUT=$($(OSLIKE)_OUT)
endif
OUT=$($(OS)_OUT)

LINUX_OBJ_EXT=.o
WIN_OBJ_EXT=.obj
ifeq ($(strip $($(OS)_OBJ_EXT)),)
$(OS)_OBJ_EXT=$($(OSLIKE)_OBJ_EXT)
endif
OBJ_EXT=$($(OS)_OBJ_EXT)

LINUX_SO_EXT=.so
WIN_SO_EXT=.dll
ifeq ($(strip $($(OS)_SO_EXT)),)
$(OS)_SO_EXT=$($(OSLIKE)_SO_EXT)
endif
SO_EXT=$($(OS)_SO_EXT)

WIN_STLPRFX=
LINUX_STLPRFX=lib
ifeq ($(strip $($(OS)_STLPRFX)),)
$(OS)_STLPRFX=$($(OSLIKE)_STLPRFX)
endif
STLPRFX=$($(OS)_STLPRFX)

WIN_STLSUFX=.lib
LINUX_STLSUFX=.a
ifeq ($(strip $($(OS)_STLSUFX)),)
$(OS)_STLSUFX=$($(OSLIKE)_STLSUFX)
endif
STLSUFX=$($(OS)_STLSUFX)

LINUX_SO_FLAGS=-shared -fPIC
WIN_SO_FLAGS=-dll
ifeq ($(strip $($(OS)_SO_FLAGS)),)
$(OS)_SO_FLAGS=$($(OSLIKE)_SO_FLAGS)
endif
SO_FLAGS=$($(OS)_SO_FLAGS)

WIN_LIBS=Advapi32.lib $(OPENSSL_SDK_PATH)/libcrypto.lib
LINUX_LIBS=-L $(OPENSSL_SDK_PATH) -lcrypto
ifeq ($(strip $($(OS)_LIBS)),)
$(OS)_LIBS=$($(OSLIKE)_LIBS)
endif
LIBS=$($(OS)_LIBS)

WIN_LDFLAGS=-link
ifeq ($(OS),LINUX)
else
ifeq ($(strip $($(OS)_LDFLAGS)),)
$(OS)_LDFLAGS=$($(OSLIKE)_LDFLAGS)
endif
endif
LDFLAGS=$($(OS)_LDFLAGS)

LINUX_AR=ar
WIN_AR=link
ifeq ($(strip $($(OS)_AR)),)
$(OS)_AR=$($(OSLIKE)_AR)
endif
AR=$($(OS)_AR)

LINUX_ARFLAGS=rcs $@
WIN_ARFLAGS=-lib -nologo -out:$@
ifeq ($(strip $($(OS)_ARFLAGS)),)
$(OS)_ARFLAGS=$($(OSLIKE)_ARFLAGS)
endif
ARFLAGS=$($(OS)_ARFLAGS)

show_config:
	echo "Set in defs.mk or ref/Makefile"
	echo "=========== Config ================"
	echo "CFLAGS			= " "$(CFLAGS)"
	echo "LINUX_CFLAGS		= " "$(LINUX_CFLAGS)"
	echo "OS				= " "$(OS)"
	echo "OSLIKE			= " "$(OSLIKE)"
