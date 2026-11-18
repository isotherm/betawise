MICROPY_DIR ?= ../upstream
BUILD ?= build
MP_ARCHIVE = $(BUILD)/libmicropython.a
OBJ = MicroPython.o setjmp.o
TARGET = MicroPython.OS3KApp

include ../../../Makefile.common

CFLAGS += -I. -I$(MICROPY_DIR) -I$(BUILD) -DNDEBUG -fno-short-enums -malign-int
LDFLAGS += --start-group $(MP_ARCHIVE) $(OS3K_LIB) --end-group

MicroPython.o: mpconfigport.h $(BUILD)/genhdr/qstrdefs.generated.h
$(TARGET): applet.mk $(MP_ARCHIVE)
