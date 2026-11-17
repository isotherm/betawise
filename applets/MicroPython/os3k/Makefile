.DEFAULT_GOAL := applet
MICROPY_DIR ?= ../upstream
CROSS_COMPILE ?= m68k-linux-gnu-
include $(MICROPY_DIR)/py/mkenv.mk

QSTR_DEFS = qstrdefsport.h
include $(TOP)/py/py.mk

CFLAGS += -I. -I$(TOP) -I$(BUILD) -m68000 -malign-int -msep-data -fomit-frame-pointer
CFLAGS += -fno-store-merging -Os -DNDEBUG -ffunction-sections -fdata-sections
OBJ = $(PY_CORE_O)

.PHONY: applet clean-applet
applet: $(BUILD)/libmicropython.a
	$(MAKE) -f applet.mk CROSS_COMPILE="$(CROSS_COMPILE)" MICROPY_DIR="$(MICROPY_DIR)" BUILD="$(BUILD)"

clean: clean-applet
clean-applet:
	$(MAKE) -f applet.mk clean

include $(TOP)/py/mkrules.mk
