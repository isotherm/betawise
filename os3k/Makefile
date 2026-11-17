INT = version.h
OBJ = os3k.o syscall.o crt0.o
TARGET = libos3k.a

include ../Makefile.common

$(TARGET): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

os3k.o syscall.o: os3k.h version.h
crt0.o: crt0.S
	$(CC) -m68000 -c $< -o $@

version.h: versiongen.py
	./versiongen.py > version.h
