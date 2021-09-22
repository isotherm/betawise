INT = version.h
OBJ = os3k.o syscall.o
TARGET = libos3k.a
SUBDIRS = $(shell find -mindepth 2 -maxdepth 2 -type f -name Makefile -printf '%h\n')

include Makefile.common

os3k.o: version.h

version.h: versiongen.py
	./versiongen.py > version.h
