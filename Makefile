OBJ = betawise.o
TARGET = libbetawise.a
SUBDIRS = $(wildcard */.)

include Makefile.common
