.DEFAULT_GOAL := all
MICROPY_DIR ?= upstream
CROSS_COMPILE ?= m68k-linux-gnu-
# No selection builds all levels; VARIANT=... builds just that level.
SELECTED_VARIANTS := $(if $(strip $(VARIANT)),$(VARIANT),minimal core basic extra)
include variant.mk
# A shared custom directory cannot safely hold several variants concurrently.
ifneq ($(filter command line,$(origin BUILD)),)
ifneq ($(filter-out $(VARIANT),$(SELECTED_VARIANTS) $(filter $(VARIANTS),$(MAKECMDGOALS))),)
$(error BUILD override requires a single matching VARIANT=minimal, core, basic, or extra)
endif
endif
include $(MICROPY_DIR)/py/mkenv.mk

MICROPY_ROM_TEXT_COMPRESSION ?= 1
QSTR_DEFS = qstrdefsport.h
include $(TOP)/py/py.mk
# Upstream's splitter keeps stale fragments when a source loses every
# registration. Reset generated fragments on port-configuration changes so
# disabling a module also removes its registration and old archive contents.
PORT_CONFIG_STAMP = $(BUILD)/port-config.stamp
QSTR_GLOBAL_DEPENDENCIES += Makefile variant.mk $(PORT_CONFIG_STAMP)

$(PORT_CONFIG_STAMP): mpconfigport.h Makefile variant.mk
	$(Q)$(MKDIR) -p $(BUILD)
	$(Q)$(RM) -rf $(addprefix $(HEADER_BUILD)/,qstr module root_pointer compress)
	$(Q)$(RM) -f $(BUILD)/libmicropython.a
	$(Q)$(TOUCH) $@

CFLAGS += -DMICROPY_CONFIG_ROM_LEVEL=$(ROM_LEVEL)
CFLAGS += -I../../os3k -I. -I$(TOP) -I$(BUILD) -m68000 -msoft-float -malign-int -msep-data -fomit-frame-pointer
# OS3K supplies libc calls directly, without glibc fortify wrappers.
CFLAGS += -U_FORTIFY_SOURCE
CFLAGS += -fno-store-merging -Os -DNDEBUG -ffunction-sections -fdata-sections

# Use basenames so source paths cannot escape the variant build directory.
LIBM_SRC := $(filter-out thumb_vfp_sqrtf.c,$(notdir $(wildcard $(TOP)/lib/libm/*.c)))
LIBM_OBJ := $(addprefix $(BUILD)/lib/libm/,$(LIBM_SRC:.c=.o))

# Keep guarded sources listed so disabled modules regenerate their metadata
# and replace old archive members correctly during incremental builds.
SRC_EXTMOD_C = \
	extmod/modasyncio.c \
	extmod/modbinascii.c \
	extmod/moddeflate.c \
	extmod/modframebuf.c \
	extmod/modhashlib.c \
	extmod/modheapq.c \
	extmod/modjson.c \
	extmod/modos.c \
	extmod/modplatform.c \
	extmod/modrandom.c \
	extmod/modre.c \
	extmod/modselect.c \
	extmod/moductypes.c

# Upstream's standard streams use our output HAL and unsupported-input stubs.
SRC_SHARED_C = shared/readline/readline.c shared/runtime/pyexec.c $(if $(filter extra,$(VARIANT)),shared/runtime/sys_stdio_mphal.c)
SRC_QSTR += $(SRC_EXTMOD_C) $(SRC_SHARED_C)
OBJ = $(PY_CORE_O) $(LIBM_OBJ) $(addprefix $(BUILD)/,$(SRC_EXTMOD_C:.c=.o) $(SRC_SHARED_C:.c=.o))

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
