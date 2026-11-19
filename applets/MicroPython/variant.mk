# Include before py/mkenv.mk, which otherwise supplies BUILD=build.
VARIANTS := minimal core basic extra
VARIANT ?= core
BUILD ?= build-$(VARIANT)

ifeq ($(VARIANT),minimal)
ROM_LEVEL := MICROPY_CONFIG_ROM_LEVEL_MINIMUM
else ifeq ($(VARIANT),core)
ROM_LEVEL := MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES
else ifeq ($(VARIANT),basic)
ROM_LEVEL := MICROPY_CONFIG_ROM_LEVEL_BASIC_FEATURES
else ifeq ($(VARIANT),extra)
ROM_LEVEL := MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES
else
$(error Unknown VARIANT '$(VARIANT)'. Use minimal, core, basic, or extra)
endif
