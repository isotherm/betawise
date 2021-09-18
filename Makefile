INT = version.h
OBJ = os3k.o
TARGET = libos3k.a
SUBDIRS = $(shell find -mindepth 1 -maxdepth 1 -type d -not -path "./.*")

include Makefile.common

os3k.o: version.h

version.h: versiongen.py
	./versiongen.py > version.h
