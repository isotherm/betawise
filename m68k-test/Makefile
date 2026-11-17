OBJ = test.o externs.o crt0.o
TARGET = test.OS3KApp

include ../Makefile.common

.PHONY: check
check: CROSS_COMPILE = m68k-linux-gnu-
check:
	CROSS_COMPILE="$(CROSS_COMPILE)" ./build.sh
