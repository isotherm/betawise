INT = glyphs.h
OBJ = NeoFontTerminal.o
TARGET = NeoFontTerminal.OS3KApp

include ../Makefile.common

NeoFontTerminal.o: glyphs.h

glyphs.h: glyphs.png
	./fontgen.py "glyphs.png" "glyphs.h"
