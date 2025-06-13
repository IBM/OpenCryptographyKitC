VARIANT = shake-128s
PARAMS = sphincs-$(VARIANT)
THASH = robust

CC=/usr/bin/gcc
LINUX_CFLAGS=-Wall -Wextra -Wpedantic -O3 -std=c99 -Wconversion -Wmissing-prototypes

MY_CFLAGS=-DPARAMS=$(PARAMS) $(EXTRA_CFLAGS)

LIB_SOURCES =                    address.c merkle.c wots.c wotsx1.c utils.c utilsx1.c fors.c sign.c
HEADERS = params.h randombytes.h address.h merkle.h wots.h wotsx1.h utils.h utilsx1.h fors.h api.h hash.h thash.h

ifneq (,$(findstring shake,$(PARAMS)))
	HASH_SOURCES = fips202.c
	THASH_SOURCES = hash_shake.c thash_shake_$(THASH).c
	HASH_HEADERS = fips202.h
endif
ifneq (,$(findstring haraka,$(PARAMS)))
	HASH_SOURCES = haraka.c
	THASH_SOURCES = hash_haraka.c thash_haraka_$(THASH).c
	HASH_HEADERS = haraka.h
endif
ifneq (,$(findstring sha2,$(PARAMS)))
	HASH_SOURCES = sha2.c
	THASH_SOURCES = hash_sha2.c thash_sha2_$(THASH).c
	HASH_HEADERS = sha2.h
endif
LIB_SOURCES += $(THASH_SOURCES)
LIB_SOURCES := $(addprefix $(VARIANT)-,$(LIB_SOURCES))
HEADERS += $(HASH_HEADERS)

SOURCES = $(LIB_SOURCES) $(addprefix $(VARIANT)-,randombytes.c)

DET_SOURCES = $(SOURCES:randombytes.%=rng.%)
DET_HEADERS = $(HEADERS:randombytes.%=rng.%)

TESTS =         test/fors \
		test/spx \

BENCHMARK = test/benchmark

#OS=LINUX
OS=WIN

.PHONY: default clean test benchmark static

default: PQCgenKAT_sign

ICC_ROOT=../../..
include ../../defs.mk

CFLAGS+=$(MY_CFLAGS)

Makefile: ../../defs.mk
	touch $@

all: static PQCgenKAT_sign tests benchmarks

tests: $(TESTS)

test: $(TESTS:=.exec)

benchmarks: $(BENCHMARK)

benchmark: $(BENCHMARK:=.exec)

static: libsphincs_ref-$(PARAMS)$(STLSUFX) libsphincs_ref-hash-$(PARAMS)$(STLSUFX)

libsphincs_ref-$(PARAMS)$(STLSUFX): $(LIB_SOURCES) $(HEADERS) Makefile
	$(CC) $(CFLAGS) -c $(LIB_SOURCES)
	$(AR) $(ARFLAGS) $(subst .c,$(OBJ_EXT), $(LIB_SOURCES))
	$(RM) $(subst .c,$(OBJ_EXT), $(LIB_SOURCES))

libsphincs_ref-hash-$(PARAMS)$(STLSUFX): $(HASH_SOURCES) $(HASH_HEADERS) Makefile
	$(CC) $(CFLAGS) -c $(HASH_SOURCES)
	$(AR) $(ARFLAGS) $(subst .c,$(OBJ_EXT), $(HASH_SOURCES))
	$(RM) $(subst .c,$(OBJ_EXT), $(HASH_SOURCES))

PQCgenKAT_sign: PQCgenKAT_sign.c $(DET_SOURCES) $(DET_HEADERS)
	$(CC) $(CFLAGS) -o $@ $(DET_SOURCES) $< -lcrypto

test/benchmark: test/benchmark.c test/cycles.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ test/cycles.c $(SOURCES) $< $(LDLIBS)

test/%: test/%.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $< $(LDLIBS)

test/haraka: test/haraka.c $(filter-out haraka.c,$(SOURCES)) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(filter-out haraka.c,$(SOURCES)) $< $(LDLIBS)

test/%.exec: test/%
	@$<

clean:
	-$(RM) $(TESTS)
	-$(RM) $(BENCHMARK)
	-$(RM) PQCgenKAT_sign
	-$(RM) PQCsignKAT_*.rsp
	-$(RM) PQCsignKAT_*.req
