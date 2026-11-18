INT = version.h
OBJ = os3k.o syscall.o crt0.o arithmetic.o graphics.o
TARGET = libos3k.a
OS3K_HEADERS = os3k.h version.h

include ../Makefile.common

$(TARGET): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

crt0.o: crt0.S
	$(CC) -m68000 -c $< -o $@

# The version also depends on Git refs. Keep the timestamp if it is unchanged.
.PHONY: FORCE
version.h: versiongen.py FORCE
	@set -e; trap '$(RM) "$@.tmp"' EXIT; \
	./versiongen.py > "$@.tmp"; \
	if ! cmp -s "$@.tmp" "$@"; then mv "$@.tmp" "$@"; fi
