TARGET = DebugTool.OS3KApp

SRC = betawise.c debug.c
OBJ = $(SRC:.c=.o)

CROSS_COMPILE = m68k-elf-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
CFLAGS = -m68000 -mpcrel -Os -fomit-frame-pointer -ffixed-a5 -U_FORTIFY_SOURCE
LDFLAGS = -T betawise.lds

.SUFFIXES = .c

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(TARGET)

$(TARGET): debug.o betawise.o
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(OBJ) $(TARGET)

debug.o: debug.c betawise.h
betawise.o: betawise.c betawise.h
