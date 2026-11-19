.DEFAULT_GOAL := all
MICROPY_DIR ?= upstream
CROSS_COMPILE ?= m68k-linux-gnu-
# No selection builds all levels; VARIANT=... builds just that level.
SELECTED_VARIANTS := $(if $(strip $(VARIANT)),$(VARIANT),minimal core basic)
include variant.mk
# A shared custom directory cannot safely hold several variants concurrently.
ifneq ($(filter command line,$(origin BUILD)),)
ifneq ($(filter-out $(VARIANT),$(SELECTED_VARIANTS) $(filter $(VARIANTS),$(MAKECMDGOALS))),)
$(error BUILD override requires a single matching VARIANT=minimal, core, or basic)
endif
endif
include $(MICROPY_DIR)/py/mkenv.mk

MICROPY_ROM_TEXT_COMPRESSION ?= 1
QSTR_DEFS = qstrdefsport.h
include $(TOP)/py/py.mk
QSTR_GLOBAL_DEPENDENCIES += Makefile variant.mk

CFLAGS += -DMICROPY_CONFIG_ROM_LEVEL=$(ROM_LEVEL)
CFLAGS += -I../../os3k -I. -I$(TOP) -I$(BUILD) -m68000 -msoft-float -malign-int -msep-data -fomit-frame-pointer
# OS3K supplies libc calls directly, without glibc fortify wrappers.
CFLAGS += -U_FORTIFY_SOURCE
CFLAGS += -fno-store-merging -Os -DNDEBUG -ffunction-sections -fdata-sections

# Use basenames so source paths cannot escape the variant build directory.
LIBM_SRC := $(filter-out thumb_vfp_sqrtf.c,$(notdir $(wildcard $(TOP)/lib/libm/*.c)))
LIBM_OBJ := $(addprefix $(BUILD)/lib/libm/,$(LIBM_SRC:.c=.o))

SRC_EXTMOD_C = \
	extmod/modbinascii.c \
	extmod/modhashlib.c \
	extmod/modjson.c \
	extmod/modplatform.c \
	extmod/modrandom.c \
	extmod/modre.c

SRC_QSTR += $(SRC_EXTMOD_C)
OBJ = $(PY_CORE_O) $(LIBM_OBJ) $(addprefix $(BUILD)/,$(SRC_EXTMOD_C:.c=.o))

# Rebuild when variant mappings, compiler flags, or source selections change.
$(OBJ): Makefile variant.mk ../../os3k/setjmp.h

$(LIBM_OBJ): CFLAGS += -include libm_port.h
$(LIBM_OBJ): libm_port.h

ifneq ($(filter clean,$(MAKECMDGOALS)),)
ifneq ($(filter-out clean,$(MAKECMDGOALS)),)
.NOTPARALLEL:
endif
endif

.PHONY: all applet variant $(VARIANTS) clean-applet
all: $(SELECTED_VARIANTS)

$(VARIANTS):
	$(MAKE) VARIANT=$@ BUILD="$(if $(filter command line,$(origin BUILD)),$(BUILD),build-$@)" variant

applet: variant

variant: $(BUILD)/libmicropython.a
	$(MAKE) -f applet.mk CROSS_COMPILE="$(CROSS_COMPILE)" MICROPY_DIR="$(MICROPY_DIR)" VARIANT="$(VARIANT)" BUILD="$(BUILD)"
	cmp -s $(BUILD)/MicroPython.OS3KApp MicroPython-$(VARIANT).OS3KApp || cp $(BUILD)/MicroPython.OS3KApp MicroPython-$(VARIANT).OS3KApp

# Intermediates (including ELF/relocation files) live entirely in these directories.
# Upstream's clean recipe below also removes a custom BUILD directory.
clean: clean-applet
clean-applet:
	$(RM) -rf $(addprefix build-,$(VARIANTS))
	$(RM) -f $(addprefix MicroPython-,$(addsuffix .OS3KApp,$(VARIANTS)))

include $(TOP)/py/mkrules.mk
