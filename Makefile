SUBDIRS = os3k applets

include Makefile.common

toolchain:
	make -C m68k-elf
