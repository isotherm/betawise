.DEFAULT_GOAL := applet
MICROPY_DIR ?= ../upstream
CROSS_COMPILE ?= m68k-linux-gnu-
include $(MICROPY_DIR)/py/mkenv.mk

QSTR_DEFS = qstrdefsport.h
include $(TOP)/py/py.mk

CFLAGS += -I. -I$(TOP) -I$(BUILD) -m68000 -msoft-float -malign-int -msep-data -fomit-frame-pointer
# OS3K supplies libc calls directly, without glibc fortify wrappers.
CFLAGS += -U_FORTIFY_SOURCE
CFLAGS += -fno-store-merging -Os -DNDEBUG -ffunction-sections -fdata-sections
LIBM_SRC = lib/libm/math.c lib/libm/fmodf.c lib/libm/nearbyintf.c lib/libm/ef_sqrt.c
LIBM_OBJ = $(addprefix $(BUILD)/,$(LIBM_SRC:.c=.o))
OBJ = $(PY_CORE_O) $(LIBM_OBJ)

$(LIBM_OBJ): CFLAGS += -include libm_port.h
$(LIBM_OBJ): libm_port.h

.PHONY: applet clean-applet
applet: $(BUILD)/libmicropython.a
	$(MAKE) -f applet.mk CROSS_COMPILE="$(CROSS_COMPILE)" MICROPY_DIR="$(MICROPY_DIR)" BUILD="$(BUILD)"

clean: clean-applet
clean-applet:
	$(MAKE) -f applet.mk clean

include $(TOP)/py/mkrules.mk
