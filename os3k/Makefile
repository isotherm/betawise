INT = version.h
OBJ = os3k.o os3k_abi_bridge.o os3k_display.o os3k_keyboard.o os3k_stdlib.o syscall.o crt0.o setjmp.o os3k_graphics.o $(CRT_OBJECTS)
TARGET = libos3k.a
OS3K_HEADERS = os3k.h os3k_applet.h os3k_dialog.h os3k_display.h os3k_file.h \
               os3k_irda.h os3k_keyboard.h os3k_localization.h os3k_markup.h os3k_power.h os3k_serial.h os3k_speech.h os3k_wireless.h os3k_stdlib.h os3k_text.h os3k_time.h setjmp.h version.h

include compiler-rt/builtins.mk
include ../Makefile.common
os3k.o os3k_display.o os3k_stdlib.o: os3k_display_internal.h

$(TARGET): $(OBJ) Makefile compiler-rt/builtins.mk
	@set -e; trap '$(RM) "$@.tmp"' EXIT; \
	$(RM) "$@.tmp"; \
	$(AR) $(ARFLAGS) "$@.tmp" $(OBJ); \
	mv "$@.tmp" "$@"

crt0.o: crt0.S
	$(CC) -m68000 -c $< -o $@

# The version also depends on Git refs. Keep the timestamp if it is unchanged.
.PHONY: FORCE
version.h: versiongen.py FORCE
	@set -e; trap '$(RM) "$@.tmp"' EXIT; \
	./versiongen.py > "$@.tmp"; \
	if ! cmp -s "$@.tmp" "$@"; then mv "$@.tmp" "$@"; fi
