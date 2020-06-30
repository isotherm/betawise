TARGET = DebugTool.OS3KApp

SRC = betawise.c debug.c
OBJ = $(SRC:.c=.o)

CC = m68k-elf-gcc
CFLAGS = -m68000 -mpcrel -Os -fomit-frame-pointer -ffixed-a5
INCLUDE = -I.
LIB =
LD = m68k-elf-ld
LDFLAGS = -T betawise.lds

.SUFFIXES = .c

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

all: $(TARGET)

$(TARGET): debug.o betawise.o
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(OBJ) $(TARGET)

debug.o: debug.c betawise.h
betawise.o: betawise.c betawise.h
