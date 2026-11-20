FONT ?= Standard
FONT_Standard = glyphs.png
FONT_Small = glyphs-5x7.png
FONT_Tiny = glyphs-4x6.png
SUFFIX = $(if $(filter Standard,$(FONT)),,-$(FONT))
INT = glyphs$(SUFFIX).h
OBJ = NeoFontTerminal$(SUFFIX).o
TARGET = NeoFontTerminal$(SUFFIX).OS3KApp

include ../../Makefile.common

$(OBJ): NeoFontTerminal.c $(INT) $(OS3K_HEADERS) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -DFONT_GLYPHS='"$(INT)"' -DFONT_$(FONT) -c $< -o $@

$(INT): fontgen.py $(FONT_$(FONT))
	python3 $< "$(FONT_$(FONT))" "$@"

ifeq ($(FONT),Standard)
.PHONY: Small Tiny clean-Small clean-Tiny
all: Small Tiny
Small Tiny:
	+$(MAKE) FONT=$@ all
clean: clean-Small clean-Tiny
clean-Small clean-Tiny:
	+$(MAKE) FONT=$(@:clean-%=%) clean
endif
