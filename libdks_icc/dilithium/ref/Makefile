CC ?= /usr/bin/cc
LINUX_CFLAGS += -Wall -Wextra -Wpedantic -Wmissing-prototypes -Wredundant-decls \
  -Wshadow -Wvla -Wpointer-arith -O3 -fomit-frame-pointer
LINUX_NISTFLAGS += -Wno-unused-result -O3 -fomit-frame-pointer
RM = rm

SOURCES = sign.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c
HEADERS = config.h params.h api.h sign.h packing.h polyvec.h poly.h ntt.h \
  reduce.h rounding.h symmetric.h randombytes.h
KECCAK_SOURCES = $(SOURCES) fips202.c symmetric-shake.c
KECCAK_SOURCES_2 = $(addprefix 2-,$(KECCAK_SOURCES))
KECCAK_SOURCES_3 = $(addprefix 3-,$(KECCAK_SOURCES))
KECCAK_SOURCES_5 = $(addprefix 5-,$(KECCAK_SOURCES))
KECCAK_HEADERS = $(HEADERS) fips202.h

TESTS = nistkat/PQCgenKAT_sign2$(EXESUFX)

.PHONY: default all nistkat static speed shared clean

default: all

ICC_ROOT=../../..
include ../../defs.mk

all: \
  test/test_dilithium2$(EXESUFX) \
  test/test_dilithium3$(EXESUFX) \
  test/test_dilithium5$(EXESUFX) \
  test/test_vectors2$(EXESUFX) \
  test/test_vectors3$(EXESUFX) \
  test/test_vectors5$(EXESUFX)

nistkat: \
  nistkat/PQCgenKAT_sign2$(EXESUFX) \
  nistkat/PQCgenKAT_sign3$(EXESUFX) \
  nistkat/PQCgenKAT_sign5$(EXESUFX)

speed: \
  test/test_mul$(EXESUFX) \
  test/test_speed2$(EXESUFX) \
  test/test_speed3$(EXESUFX) \
  test/test_speed5$(EXESUFX)

static: \
  libpqcrystals_dilithium2_ref$(STLSUFX) \
  libpqcrystals_dilithium3_ref$(STLSUFX) \
  libpqcrystals_dilithium5_ref$(STLSUFX) \
  libpqcrystals_fips202_ref$(STLSUFX) \

shared: \
  libpqcrystals_dilithium2_ref$(SO_EXT) \
  libpqcrystals_dilithium3_ref$(SO_EXT) \
  libpqcrystals_dilithium5_ref$(SO_EXT) \
  libpqcrystals_fips202_ref$(SO_EXT) \

tests: $(TESTS)
	$(OPENSSL_PATH_SETUP) nistkat/PQCgenKAT_sign2$(EXESUFX)

Makefile: ../../defs.mk
	touch $@

libpqcrystals_fips202_ref$(STLSUFX): fips202.c fips202.h Makefile
	$(CC) $(CFLAGS) -c $<
	$(AR) $(ARFLAGS) fips202$(OBJ_EXT)

libpqcrystals_fips202_ref$(SO_EXT): fips202.c fips202.h Makefile
	$(CC) $(SO_FLAGS) $(CFLAGS) -o $@ $<

libpqcrystals_dilithium2_ref$(STLSUFX): $(KECCAK_SOURCES_2) $(KECCAK_HEADERS) Makefile
	$(CC) $(CFLAGS) -DDILITHIUM_MODE=2 -c $(KECCAK_SOURCES_2)
	$(AR) $(ARFLAGS) $(subst .c,$(OBJ_EXT), $(KECCAK_SOURCES_2))

libpqcrystals_dilithium2_ref$(SO_EXT): libpqcrystals_dilithium2_ref$(STLSUFX)
	$(CC) $(SO_FLAGS) $(CFLAGS) -DDILITHIUM_MODE=2 -o $@ libpqcrystals_dilithium2_ref$(STLSUFX)

libpqcrystals_dilithium3_ref$(STLSUFX): $(KECCAK_SOURCES_3) $(KECCAK_HEADERS) Makefile
	$(CC) $(CFLAGS) -DDILITHIUM_MODE=3 -c $(KECCAK_SOURCES_3)
	$(AR) $(ARFLAGS) $(subst .c,$(OBJ_EXT), $(KECCAK_SOURCES_3))

libpqcrystals_dilithium3_ref$(SO_EXT): libpqcrystals_dilithium3_ref$(STLSUFX)
	$(CC) $(SO_FLAGS) $(CFLAGS) -DDILITHIUM_MODE=3 -o $@ libpqcrystals_dilithium3_ref$(STLSUFX)

libpqcrystals_dilithium5_ref$(STLSUFX): $(KECCAK_SOURCES_5) $(KECCAK_HEADERS) Makefile
	$(CC) $(CFLAGS) -DDILITHIUM_MODE=5 -c $(KECCAK_SOURCES_5)
	$(AR) $(ARFLAGS) $(subst .c,$(OBJ_EXT), $(KECCAK_SOURCES_5))

libpqcrystals_dilithium5_ref$(SO_EXT): libpqcrystals_dilithium5_ref$(STLSUFX)
	$(CC) $(SO_FLAGS) $(CFLAGS) -DDILITHIUM_MODE=5 -o $@ libpqcrystals_dilithium5_ref$(STLSUFX)

test/test_dilithium2$(EXESUFX): test/test_dilithium.c randombytes.c $(KECCAK_SOURCES) \
  $(KECCAK_HEADERS)
	$(CC) $(CFLAGS) -DDILITHIUM_MODE=2 \
	  -o $@ $< randombytes.c $(KECCAK_SOURCES)

test/test_dilithium3$(EXESUFX): test/test_dilithium.c randombytes.c $(KECCAK_SOURCES) \
  $(KECCAK_HEADERS)
	$(CC) $(CFLAGS) -DDILITHIUM_MODE=3 \
	  -o $@ $< randombytes.c $(KECCAK_SOURCES)

test/test_dilithium5$(EXESUFX): test/test_dilithium.c randombytes.c $(KECCAK_SOURCES) \
  $(KECCAK_HEADERS)
	$(CC) $(CFLAGS) -DDILITHIUM_MODE=5 \
	  -o $@ $< randombytes.c $(KECCAK_SOURCES)

test/test_vectors2$(EXESUFX): test/test_vectors.c $(KECCAK_SOURCES) \
  $(KECCAK_HEADERS)
	$(CC) $(CFLAGS) -DDILITHIUM_MODE=2 \
	  -o $@ $< $(KECCAK_SOURCES)

test/test_vectors3$(EXESUFX): test/test_vectors.c $(KECCAK_SOURCES) $(KECCAK_HEADERS)
	$(CC) $(CFLAGS) -DDILITHIUM_MODE=3 \
	  -o $@ $< $(KECCAK_SOURCES)

test/test_vectors5: test/test_vectors.c $(KECCAK_SOURCES) \
  $(KECCAK_HEADERS)
	$(CC) $(CFLAGS) -DDILITHIUM_MODE=5 \
	  -o $@ $< $(KECCAK_SOURCES)

test/test_speed2$(EXESUFX): test/test_speed.c test/speed_print.c test/speed_print.h \
  test/cpucycles.c test/cpucycles.h randombytes.c $(KECCAK_SOURCES) \
  $(KECCAK_HEADERS)
	$(CC) $(CFLAGS) -DDILITHIUM_MODE=2 \
	  -o $@ $< test/speed_print.c test/cpucycles.c randombytes.c \
	  $(KECCAK_SOURCES)

test/test_speed3$(EXESUFX): test/test_speed.c test/speed_print.c test/speed_print.h \
  test/cpucycles.c test/cpucycles.h randombytes.c $(KECCAK_SOURCES) \
  $(KECCAK_HEADERS)
	$(CC) $(CFLAGS) -DDILITHIUM_MODE=3 \
	  -o $@ $< test/speed_print.c test/cpucycles.c randombytes.c \
	  $(KECCAK_SOURCES)

test/test_speed5$(EXESUFX): test/test_speed.c test/speed_print.c test/speed_print.h \
  test/cpucycles.c test/cpucycles.h randombytes.c $(KECCAK_SOURCES) \
  $(KECCAK_HEADERS)
	$(CC) $(CFLAGS) -DDILITHIUM_MODE=5 \
	  -o $@ $< test/speed_print.c test/cpucycles.c randombytes.c \
	  $(KECCAK_SOURCES)

test/test_mul$(EXESUFX): test/test_mul.c randombytes.c $(KECCAK_SOURCES) $(KECCAK_HEADERS)
	$(CC) $(CFLAGS) -UDBENCH -o $@ $< randombytes.c $(KECCAK_SOURCES)

nistkat/rng$(OBJ_EXT) : nistkat/rng.c nistkat/rng.h
	$(CC) $(CFLAGS) $(OUT)$@ -c $<
nistkat/PQCgenKAT_sign2$(OBJ_EXT) : nistkat/PQCgenKAT_sign.c
	$(CC) $(CFLAGS) $(OUT)$@ -DDILITHIUM_MODE=2 -c $<
nistkat/PQCgenKAT_sign3$(OBJ_EXT) : nistkat/PQCgenKAT_sign.c
	$(CC) $(CFLAGS) $(OUT)$@ -DDILITHIUM_MODE=3 -c $<
nistkat/PQCgenKAT_sign5$(OBJ_EXT) : nistkat/PQCgenKAT_sign.c
	$(CC) $(CFLAGS) $(OUT)$@ -DDILITHIUM_MODE=5 -c $<

nistkat/PQCgenKAT_sign2$(EXESUFX): nistkat/PQCgenKAT_sign2$(OBJ_EXT) nistkat/rng$(OBJ_EXT) \
				libpqcrystals_dilithium2_ref$(STLSUFX)
	$(LD) $(LDFLAGS) $< nistkat/rng$(OBJ_EXT) $(LDFLAGS) libpqcrystals_dilithium2_ref$(STLSUFX) $(LIBS)

nistkat/PQCgenKAT_sign3$(EXESUFX): nistkat/PQCgenKAT_sign3$(OBJ_EXT) nistkat/rng$(OBJ_EXT) \
				libpqcrystals_dilithium3_ref$(STLSUFX)
	$(LD) $(LDFLAGS) $< nistkat/rng$(OBJ_EXT) $(LDFLAGS) libpqcrystals_dilithium3_ref$(STLSUFX) $(LIBS)

nistkat/PQCgenKAT_sign5$(EXESUFX): nistkat/PQCgenKAT_sign5$(OBJ_EXT) nistkat/rng$(OBJ_EXT) \
				libpqcrystals_dilithium5_ref$(STLSUFX)
	$(LD) $(LDFLAGS) $< nistkat/rng$(OBJ_EXT) $(LDFLAGS) libpqcrystals_dilithium5_ref$(STLSUFX) $(LIBS)

clean:
	rm -f *~ test/*~ *.gcno *.gcda *.lcov
	rm -f libpqcrystals_dilithium2_ref.so
	rm -f libpqcrystals_dilithium3_ref.so
	rm -f libpqcrystals_dilithium5_ref.so
	rm -f libpqcrystals_fips202_ref.so
	rm -f test/test_dilithium2
	rm -f test/test_dilithium3
	rm -f test/test_dilithium5
	rm -f test/test_vectors2
	rm -f test/test_vectors3
	rm -f test/test_vectors5
	rm -f test/test_speed2
	rm -f test/test_speed3
	rm -f test/test_speed5
	rm -f test/test_mul
	rm -f nistkat/PQCgenKAT_sign2
	rm -f nistkat/PQCgenKAT_sign3
	rm -f nistkat/PQCgenKAT_sign5
