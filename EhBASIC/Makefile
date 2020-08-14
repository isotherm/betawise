TARGET = EhBASIC.OS3KApp

SRC = EhBASIC.asm
OBJ = $(SRC:.asm=.o)

CROSS_COMPILE = m68k-elf-
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
CFLAGS = -m68000 -mpcrel -Os -fomit-frame-pointer -ffixed-a5 -U_FORTIFY_SOURCE -I..
ASFLAGS = --pcrel
LDFLAGS = -T ../betawise.lds

.SUFFIXES: .c .asm
.PHONY: all clean

.c.o:
	$(CC) $(CFLAGS) -c $<

.asm.o:
	$(AS) $(ASFLAGS) $< -o $@

all: $(TARGET)

$(TARGET): EhBASIC.o wrapper.o ../betawise.a
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(OBJ) $(TARGET)

EhBASIC.o: EhBASIC.asm EhBASIC.inc

wrapper.o: wrapper.c ../betawise.h
