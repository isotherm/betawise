APPLET_DIRS = $(patsubst %/Makefile,%,$(wildcard */Makefile))

# EhBASIC's assembly workspace relocations need a separate fix.
DISABLED_APPLETS ?= EhBASIC
SUBDIRS = $(filter-out $(DISABLED_APPLETS),$(APPLET_DIRS))

CLEAN_SUBDIRS = $(APPLET_DIRS)

include ../Makefile.common

ifneq ($(filter NeoFontTerminal,$(SUBDIRS)),)
UCBLogo: NeoFontTerminal
endif
