TARGET = EhBASIC.OS3KApp

SRC = EhBASIC.asm
OBJ = $(SRC:.asm=.o)

CROSS_COMPILE = m68k-elf-
AS = $(CROSS_COMPILE)as
ASFLAGS = --pcrel
LD = $(CROSS_COMPILE)ld
LDFLAGS = -T ../betawise.lds

.SUFFIXES: .asm
.PHONY: all clean

.asm.o:
	$(AS) $(ASFLAGS) $< -o $@

all: $(TARGET)

$(TARGET): EhBASIC.o
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(OBJ) $(TARGET)

EhBASIC.o: EhBASIC.asm EhBASIC.inc
