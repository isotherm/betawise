TARGET = DebugTool.OS3KApp

SRC = DebugTool.c
OBJ = $(SRC:.c=.o)

CROSS_COMPILE = m68k-elf-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
CFLAGS = -m68000 -mpcrel -Os -fomit-frame-pointer -ffixed-a5 -U_FORTIFY_SOURCE -I..
LDFLAGS = -T ../betawise.lds

.SUFFIXES = .c

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(TARGET)

$(TARGET): DebugTool.o ../betawise.a
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(OBJ) $(TARGET)

DebugTool.o: DebugTool.c ../betawise.h
