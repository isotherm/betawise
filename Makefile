SUBDIRS = os3k m68k-test applets

include Makefile.common

ifneq ($(MAKECMDGOALS),clean)
m68k-test applets: os3k
endif
