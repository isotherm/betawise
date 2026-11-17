SUBDIRS = os3k tests/qemu applets

include Makefile.common

tests/qemu applets: os3k
