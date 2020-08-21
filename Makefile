INT = version.h
OBJ = betawise.o
TARGET = libbetawise.a
SUBDIRS = $(wildcard */.)

include Makefile.common

betawise.o: version.h

version.h: versiongen.py
	./versiongen.py > version.h
