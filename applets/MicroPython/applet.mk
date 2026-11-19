MICROPY_DIR ?= upstream
include variant.mk
MP_ARCHIVE = $(BUILD)/libmicropython.a
OBJ = $(BUILD)/MicroPython.o $(BUILD)/mphalport.o
TARGET = $(BUILD)/MicroPython.OS3KApp

include ../../Makefile.common

CFLAGS += -DMICROPY_CONFIG_ROM_LEVEL=$(ROM_LEVEL)
CFLAGS += -I. -I$(MICROPY_DIR) -I$(BUILD) -DNDEBUG -fno-short-enums -malign-int
LDFLAGS += --start-group $(MP_ARCHIVE) $(OS3K_LIB) --end-group

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: %.c mpconfigport.h mphalport.h variant.mk applet.mk $(OS3K_HEADERS) $(BUILD)/genhdr/qstrdefs.generated.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TARGET): applet.mk variant.mk $(MP_ARCHIVE)
