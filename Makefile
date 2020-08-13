TARGET = betawise.a

SRC = betawise.c
OBJ = $(SRC:.c=.o)

CROSS_COMPILE = m68k-elf-
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
CFLAGS = -m68000 -mpcrel -Os -fomit-frame-pointer -ffixed-a5 -U_FORTIFY_SOURCE
ARFLAGS = rcs

.SUFFIXES = .c

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(TARGET)
	+$(MAKE) -C DebugTool

$(TARGET): betawise.o
	$(AR) $(ARFLAGS) $@ $^

clean:
	rm -f $(OBJ) $(TARGET)

betawise.o: betawise.c betawise.h
