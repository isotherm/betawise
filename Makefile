INT = version.h
OBJ = betawise.o
TARGET = libbetawise.a
SUBDIRS = $(shell find -mindepth 1 -maxdepth 1 -type d -not -path "./.*")

include Makefile.common

betawise.o: version.h

version.h: versiongen.py
	./versiongen.py > version.h
