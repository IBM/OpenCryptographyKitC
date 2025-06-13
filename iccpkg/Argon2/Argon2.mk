ARGON2_SRC = 	Argon2/argon2.c Argon2/core.c Argon2/blake2/blake2b.c \
	Argon2/thread.c Argon2/encoding.c Argon2/opt.c

ARGON2_CFLAGS = -I Argon2/ $(CFLAGS)

argon2_obj = Argon2/argon2$(OBJSUFX) Argon2/blake2b$(OBJSUFX) Argon2/thread$(OBJSUFX) Argon2/encoding$(OBJSUFX) Argon2/core$(OBJSUFX) Argon2/ref$(OBJSUFX)



Argon2/argon2$(OBJSUFX): $(ICC_ROOT)/iccpkg/Argon2/Argon2.mk Argon2/argon2.c Argon2/argon2.h Argon2/encoding.h \
			Argon2/core.h
	$(CC) $(ARGON2_CFLAGS) Argon2/argon2.c $(OUT)$@

Argon2/core$(OBJSUFX): $(ICC_ROOT)/iccpkg/Argon2/Argon2.mk Argon2/core.c Argon2/core.h Argon2/thread.h \
		Argon2/blake2/blake2.h Argon2/blake2/blake2-impl.h   
	$(CC) $(ARGON2_CFLAGS) Argon2/core.c $(OUT)$@

Argon2/blake2b$(OBJSUFX): $(ICC_ROOT)/iccpkg/Argon2/Argon2.mk Argon2/blake2/blake2b.c \
		Argon2/blake2/blake2.h Argon2/blake2/blake2-impl.h   
	$(CC) $(ARGON2_CFLAGS) Argon2/blake2/blake2b.c $(OUT)$@

Argon2/thread$(OBJSUFX): $(ICC_ROOT)/iccpkg/Argon2/Argon2.mk Argon2/thread.c Argon2/thread.h
	$(CC) $(ARGON2_CFLAGS) Argon2/thread.c $(OUT)$@

Argon2/encoding$(OBJSUFX): $(ICC_ROOT)/iccpkg/Argon2/Argon2.mk Argon2/encoding.c Argon2/core.h Argon2/encoding.h
	$(CC) $(ARGON2_CFLAGS) Argon2/encoding.c $(OUT)$@

Argon2/ref$(OBJSUFX): $(ICC_ROOT)/iccpkg/Argon2/Argon2.mk Argon2/ref.c Argon2/argon2.h \
		Argon2/blake2/blamka-round-ref.h   
	$(CC) $(ARGON2_CFLAGS) Argon2/ref.c $(OUT)$@


clean_argon2:
		rm -f $(argon2_obj)

show_argon2:
		echo "ARGON2_CFLAGS 	= $(ARGON2_CFLAGS)"
		echo "argon2_obj  	= $(argon2_obj)"

test:           Argon2/test.c
		$(CC) $(CFLAGS)  -Wextra -Wno-type-limits $^ -o testcase
		@sh kats/test.sh
		./testcase