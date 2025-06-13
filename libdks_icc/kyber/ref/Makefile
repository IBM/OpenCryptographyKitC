CC ?= /usr/bin/cc
LINUX_CFLAGS += -Wall -Wextra -Wpedantic -Wmissing-prototypes -Wredundant-decls \
  -Wshadow -Wpointer-arith -O3 -fomit-frame-pointer -z noexecstack
LINUX_NISTFLAGS += -Wno-unused-result -O3 -fomit-frame-pointer
RM = rm

SOURCES = kem.c indcpa.c polyvec.c poly.c ntt.c cbd.c reduce.c verify.c
SOURCES_512 = $(addprefix 512-,$(SOURCES))
SOURCES_768 = $(addprefix 768-,$(SOURCES))
SOURCES_1024 = $(addprefix 1024-,$(SOURCES))
SOURCESKECCAK = fips202.c symmetric-shake.c
SOURCESKECCAK_512 = $(addprefix 512-,$(SOURCESKECCAK))
SOURCESKECCAK_768 = $(addprefix 768-,$(SOURCESKECCAK))
SOURCESKECCAK_1024 = $(addprefix 1024-,$(SOURCESKECCAK))
HEADERS = params.h kem.h indcpa.h polyvec.h poly.h ntt.h cbd.h reduce.c verify.h symmetric.h
HEADERSKECCAK = $(HEADERS) fips202.h

#OS=LINUX
OS=WIN

TESTS = nistkat/PQCgenKAT_kem512$(EXESUFX)

.PHONY: default all speed static shared clean nistkat

default: all

ICC_ROOT=../../..
include ../../defs.mk

all: static shared test nistkat

test: \
  test/test_kyber512$(EXESUFX) \
  test/test_kyber768$(EXESUFX) \
  test/test_kyber1024$(EXESUFX) \
  test/test_vectors512$(EXESUFX) \
  test/test_vectors768$(EXESUFX) \
  test/test_vectors1024$(EXESUFX) \

speed: \
  test/test_speed512$(EXESUFX) \
  test/test_speed768$(EXESUFX) \
  test/test_speed1024$(EXESUFX) \

static: lib \
  lib/libpqcrystals_kyber512_ref$(STLSUFX) \
  lib/libpqcrystals_kyber768_ref$(STLSUFX) \
  lib/libpqcrystals_kyber1024_ref$(STLSUFX) \
  lib/libpqcrystals_fips202_ref$(STLSUFX) \

shared: \
  lib/libpqcrystals_kyber512_ref$(SO_EXT) \
  lib/libpqcrystals_kyber768_ref$(SO_EXT) \
  lib/libpqcrystals_kyber1024_ref$(SO_EXT) \
  lib/libpqcrystals_fips202_ref$(SO_EXT) \

nistkat: \
	nistkat/PQCgenKAT_kem512$(EXESUFX) \
	nistkat/PQCgenKAT_kem768$(EXESUFX) \
	nistkat/PQCgenKAT_kem1024$(EXESUFX) \

tests: $(TESTS)
	$(OPENSSL_PATH_SETUP) nistkat/PQCgenKAT_kem512$(EXESUFX)

Makefile: ../../defs.mk
	touch $@

lib:
	mkdir lib

lib/libpqcrystals_fips202_ref$(STLSUFX): lib fips202.c fips202.h Makefile
	$(CC) $(CFLAGS) -c fips202.c
	$(AR) $(ARFLAGS) fips202$(OBJ_EXT)

lib/libpqcrystals_fips202_ref$(SO_EXT): fips202.c fips202.h
	$(CC) $(CFLAGS) fips202.c $(OUT)$@ $(LDFLAGS) $(SO_FLAGS)

lib/libpqcrystals_kyber512_ref$(STLSUFX): $(SOURCES_512) $(HEADERS) $(SOURCESKECCAK_512) $(HEADERSKECCAK) Makefile
	$(CC) $(CFLAGS) -DKYBER_K=2 -c $(SOURCES_512) $(SOURCESKECCAK_512)
	$(AR) $(ARFLAGS) $(subst .c,$(OBJ_EXT), $(SOURCES_512) $(SOURCESKECCAK_512))

lib/libpqcrystals_kyber512_ref$(SO_EXT): lib/libpqcrystals_kyber512_ref$(STLSUFX) $(HEADERS) symmetric-shake.c fips202.c fips202.h randombytes.c
	$(CC) $(CFLAGS) -DKYBER_K=2 symmetric-shake.c fips202.c randombytes.c lib/libpqcrystals_kyber512_ref$(STLSUFX) $(LIBS) $(OUT)$@ $(LDFLAGS) $(SO_FLAGS)

lib/libpqcrystals_kyber768_ref$(STLSUFX): $(SOURCES_768) $(HEADERS) $(SOURCESKECCAK_768) $(HEADERSKECCAK) Makefile
	$(CC) $(CFLAGS) -DKYBER_K=3 -c $(SOURCES_768) $(SOURCESKECCAK_768)
	$(AR) $(ARFLAGS) $(subst .c,$(OBJ_EXT), $(SOURCES_768) $(SOURCESKECCAK_768))

lib/libpqcrystals_kyber768_ref$(SO_EXT): lib/libpqcrystals_kyber768_ref$(STLSUFX) $(HEADERS) symmetric-shake.c fips202.c fips202.h randombytes.c
	$(CC) $(CFLAGS) -DKYBER_K=3 symmetric-shake.c fips202.c randombytes.c lib/libpqcrystals_kyber768_ref$(STLSUFX) $(LIBS) $(OUT)$@ $(LDFLAGS) $(SO_FLAGS)

lib/libpqcrystals_kyber1024_ref$(STLSUFX): $(SOURCES_1024) $(HEADERS) $(SOURCESKECCAK_1024) $(HEADERSKECCAK) Makefile
	$(CC) $(CFLAGS) -DKYBER_K=4 -c $(SOURCES_1024) $(SOURCESKECCAK_1024)
	$(AR) $(ARFLAGS) $(subst .c,$(OBJ_EXT), $(SOURCES_1024) $(SOURCESKECCAK_1024))

lib/libpqcrystals_kyber1024_ref$(SO_EXT): lib/libpqcrystals_kyber1024_ref$(STLSUFX) $(HEADERS) symmetric-shake.c fips202.c fips202.h randombytes.c
	$(CC) $(CFLAGS) -DKYBER_K=4 symmetric-shake.c fips202.c randombytes.c lib/libpqcrystals_kyber1024_ref$(STLSUFX) $(LIBS) $(OUT)$@ $(LDFLAGS) $(SO_FLAGS)

test/test_kyber512$(EXESUFX): $(SOURCESKECCAK) $(HEADERSKECCAK) test/test_kyber.c randombytes.c lib/libpqcrystals_kyber512_ref$(STLSUFX)
	$(CC) $(CFLAGS) -DKYBER_K=2 $(SOURCESKECCAK) randombytes.c test/test_kyber.c $(OUT)$@ $(LDFLAGS) lib/libpqcrystals_kyber512_ref$(STLSUFX) $(LIBS)

test/test_kyber768$(EXESUFX): $(SOURCESKECCAK) $(HEADERSKECCAK) test/test_kyber.c randombytes.c lib/libpqcrystals_kyber768_ref$(STLSUFX)
	$(CC) $(CFLAGS) -DKYBER_K=3 $(SOURCESKECCAK) randombytes.c test/test_kyber.c $(OUT)$@ $(LDFLAGS) lib/libpqcrystals_kyber768_ref$(STLSUFX) $(LIBS)

test/test_kyber1024$(EXESUFX): $(SOURCESKECCAK) $(HEADERSKECCAK) test/test_kyber.c randombytes.c lib/libpqcrystals_kyber1024_ref$(STLSUFX)
	$(CC) $(CFLAGS) -DKYBER_K=4 $(SOURCESKECCAK) randombytes.c test/test_kyber.c $(OUT)$@ $(LDFLAGS) lib/libpqcrystals_kyber1024_ref$(STLSUFX) $(LIBS)

test/test_vectors512$(EXESUFX): $(SOURCESKECCAK) $(HEADERSKECCAK) test/test_vectors.c
	$(CC) $(CFLAGS) -DKYBER_K=2 $(SOURCESKECCAK) test/test_vectors.c $(OUT)$@ $(LDFLAGS) lib/libpqcrystals_kyber512_ref$(STLSUFX) $(LIBS)

test/test_vectors768$(EXESUFX): $(SOURCESKECCAK) $(HEADERSKECCAK) test/test_vectors.c
	$(CC) $(CFLAGS) -DKYBER_K=3 $(SOURCESKECCAK) test/test_vectors.c $(OUT)$@ $(LDFLAGS) lib/libpqcrystals_kyber768_ref$(STLSUFX) $(LIBS)

test/test_vectors1024$(EXESUFX): $(SOURCESKECCAK) $(HEADERSKECCAK) test/test_vectors.c
	$(CC) $(CFLAGS) -DKYBER_K=4 $(SOURCESKECCAK) test/test_vectors.c $(OUT)$@ $(LDFLAGS) lib/libpqcrystals_kyber1024_ref$(STLSUFX) $(LIBS)

test/test_speed512$(EXESUFX): $(SOURCESKECCAK) $(HEADERSKECCAK) test/cpucycles.h test/cpucycles.c test/speed_print.h test/speed_print.c test/test_speed.c randombytes.c
	$(CC) $(CFLAGS) -DKYBER_K=2 $(SOURCESKECCAK) randombytes.c test/cpucycles.c test/speed_print.c test/test_speed.c $(OUT)$@ $(LDFLAGS) $(LIBS)

test/test_speed768$(EXESUFX): $(SOURCESKECCAK) $(HEADERSKECCAK) test/cpucycles.h test/cpucycles.c test/speed_print.h test/speed_print.c test/test_speed.c randombytes.c
	$(CC) $(CFLAGS) -DKYBER_K=3 $(SOURCESKECCAK) randombytes.c test/cpucycles.c test/speed_print.c test/test_speed.c $(OUT)$@ $(LDFLAGS) $(LIBS)

test/test_speed1024$(EXESUFX): $(SOURCESKECCAK) $(HEADERSKECCAK) test/cpucycles.h test/cpucycles.c test/speed_print.h test/speed_print.c test/test_speed.c randombytes.c
	$(CC) $(CFLAGS) -DKYBER_K=4 $(SOURCESKECCAK) randombytes.c test/cpucycles.c test/speed_print.c test/test_speed.c $(OUT)$@ $(LDFLAGS) $(LIBS)

nistkat/rng$(OBJ_EXT) : nistkat/rng.c nistkat/rng.h
	$(CC) $(CFLAGS) $(OUT)$@ -c $<
nistkat/PQCgenKAT_kem512$(OBJ_EXT) : nistkat/PQCgenKAT_kem.c
	$(CC) $(CFLAGS) $(OUT)$@ -DKYBER_K=2 -c $<
nistkat/PQCgenKAT_kem768$(OBJ_EXT) : nistkat/PQCgenKAT_kem.c
	$(CC) $(CFLAGS) $(OUT)$@ -DKYBER_K=3 -c $<
nistkat/PQCgenKAT_kem1024$(OBJ_EXT) : nistkat/PQCgenKAT_kem.c
	$(CC) $(CFLAGS) $(OUT)$@ -DKYBER_K=4 -c $<

nistkat/PQCgenKAT_kem512$(EXESUFX): nistkat/PQCgenKAT_kem512$(OBJ_EXT) nistkat/rng$(OBJ_EXT) \
						lib/libpqcrystals_kyber512_ref$(STLSUFX)
	$(LD) $(LDFLAGS) nistkat/PQCgenKAT_kem512$(OBJ_EXT) nistkat/rng$(OBJ_EXT) lib/libpqcrystals_kyber512_ref$(STLSUFX) $(LIBS)

nistkat/PQCgenKAT_kem768$(EXESUFX): nistkat/PQCgenKAT_kem768$(OBJ_EXT) nistkat/rng$(OBJ_EXT) \
						lib/libpqcrystals_kyber768_ref$(STLSUFX)
	$(LD) $(LDFLAGS) nistkat/PQCgenKAT_kem768$(OBJ_EXT) nistkat/rng$(OBJ_EXT) lib/libpqcrystals_kyber768_ref$(STLSUFX) $(LIBS)

nistkat/PQCgenKAT_kem1024$(EXESUFX): nistkat/PQCgenKAT_kem1024$(OBJ_EXT) nistkat/rng$(OBJ_EXT) \
						lib/libpqcrystals_kyber1024_ref$(STLSUFX)
	$(LD) $(LDFLAGS) nistkat/PQCgenKAT_kem1024$(OBJ_EXT) nistkat/rng$(OBJ_EXT) lib/libpqcrystals_kyber1024_ref$(STLSUFX) $(LIBS)

clean:
	-$(RM) -f *.gcno *.gcda *.lcov *.o *.obj *.so *.dll
	-$(RM) -f test/test_kyber512$(EXESUFX)
	-$(RM) -f test/test_kyber768$(EXESUFX)
	-$(RM) -f test/test_kyber1024$(EXESUFX)
	-$(RM) -f test/test_vectors512$(EXESUFX)
	-$(RM) -f test/test_vectors768$(EXESUFX)
	-$(RM) -f test/test_vectors1024$(EXESUFX)
	-$(RM) -f test/test_speed512$(EXESUFX)
	-$(RM) -f test/test_speed768$(EXESUFX)
	-$(RM) -f test/test_speed1024$(EXESUFX)
	-$(RM) -f nistkat/PQCgenKAT_kem512$(EXESUFX)
	-$(RM) -f nistkat/PQCgenKAT_kem768$(EXESUFX)
	-$(RM) -f nistkat/PQCgenKAT_kem1024$(EXESUFX)
	-$(RM) -f nistkat/*.req
	-$(RM) -f nistkat/*.rsp
	-$(RM) -rf lib/

