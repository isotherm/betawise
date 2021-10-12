INT = version.h
OBJ = os3k.o syscall.o
TARGET = libos3k.a

include ../Makefile.common

os3k.o: version.h

version.h: versiongen.py
	./versiongen.py > version.h
