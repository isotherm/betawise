APPLET_DIRS = $(shell find -mindepth 2 -maxdepth 2 -type f -name Makefile -printf '%h\n')

# EhBASIC's assembly workspace relocations need a separate fix.
DISABLED_APPLETS ?= EhBASIC
SUBDIRS = $(filter-out $(addprefix ./,$(DISABLED_APPLETS)),$(APPLET_DIRS))

include ../Makefile.common
