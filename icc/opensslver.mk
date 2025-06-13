# What OpenSSL version library we'll use
# Laid out like this for developer convienience.
# Note the openssl config stanza at the end of this file as well
# 1.1.0.x (still in beta)

OPENSSL_LIBVER = 1.1.1
OPENSSL_BIN = $(OPENSSL_LIBVER)

# Windows (of course) uses it's own library version #
WIN_VTAG = 1_1

# What exact version, used to work out what patch sets to apply


# What version of OpenSSL that's built from
OPENSSL_VER    = openssl-$(OPENSSL_BIN)


# EX_SUFFIX (=_ex) is defined to build from source already extracted from tar file and
# checked into source control. This option is used by iSeries/OS400 for Clearcase builds.

# Where the source tree for OpenSSL will be
OSSL_DIR       = $(ICC_ROOT)/$(OPENSSL_VER)$(EX_SUFFIX)

# What file we use in the OpenSSL tree to trip a regen of the sources
OSSL_TAG       = $(OSSL_DIR)/Makefile

#
# only rc5 is still a problem (idea is a no-op)
#
OPENSSL_debug_FLAG = --debug
OPENSSL_release_FLAG =
OPENSSL_asan_FLAG = $(OPENSSL_debug_FLAG) -fsanitize=address
OPENSSL__FLAG = ${OPENSSL_release_FLAG}

# OpenSSL 1.1.x
OSSL_FLAGS	= 	no-engine no-sctp no-idea no-rc5 no-whirlpool no-zlib enable-mdc2 enable-camellia enable-md2 no-seed \
				$(OPENSSL_$(CONFIG)_FLAG)
