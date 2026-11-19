OBJ = test.o externs.o
TARGET = test.OS3KApp

include ../../Makefile.common

.PHONY: check
check:
	+CROSS_COMPILE="$(CROSS_COMPILE)" ./build.sh

	+CROSS_COMPILE="$(CROSS_COMPILE)" python3 abi-bridges.py

	+CROSS_COMPILE="$(CROSS_COMPILE)" python3 setjmp.py
