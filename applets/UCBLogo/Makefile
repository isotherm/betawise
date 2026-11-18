UPSTREAM ?= upstream
CORE = coms error eval files graphics init intern libloc lists logodata math mem paren parse print wrksp nographics
SOFTFLOAT ?= softfloat
SF_SOURCE = $(SOFTFLOAT)/source
SF_CORE = f64_add f64_sub f64_mul f64_div f64_lt f64_eq f64_to_i32_r_minMag \
          i32_to_f64 ui32_to_f64 s_addMagsF64 s_subMagsF64 s_roundPackToF64 \
          s_normRoundPackToF64 s_normSubnormalF64Sig s_shiftRightJam64 \
          s_countLeadingZeros32 s_countLeadingZeros64 s_countLeadingZeros8 \
          s_mul64To128 s_approxRecip32_1 s_approxRecip_1Ks softfloat_state
SF_SPECIAL = softfloat_raiseFlags s_propagateNaNF64UI
SF_OBJECTS = $(addprefix build/sf/,$(addsuffix .o,$(SF_CORE) $(SF_SPECIAL)))
SF_FLAGS = -I. -I$(SF_SOURCE)/include -I$(SF_SOURCE)/8086 -DSOFTFLOAT_FAST_INT64
PORT_OBJ = applet.o runtime.o softmath.o setjmp.o float_abi.o $(SF_OBJECTS) $(addprefix build/,$(addsuffix .o,$(CORE)))

OBJ = $(PORT_OBJ)
TARGET = UCBLogo.OS3KApp
INT = build/prepared
include ../../Makefile.common

CFLAGS += -msoft-float -Ibuild -include build/config.h -DHAVE_CONFIG_H
CFLAGS += -fno-short-enums -fno-builtin -U_FORTIFY_SOURCE
LDFLAGS += $(shell $(CC) -m68000 -print-libgcc-file-name)

build/prepared: prepare.py port_config.h port_stdio.h $(TOP)/Makefile.common
	python3 prepare.py "$(UPSTREAM)" build
	touch $@

float_abi.o: CPPFLAGS += $(SF_FLAGS)
float_abi.o: build/prepared

vpath %.c $(SF_SOURCE) $(SF_SOURCE)/8086
build/sf/%.o: %.c platform.h build/prepared
	mkdir -p build/sf
	$(CC) $(CFLAGS) $(SF_FLAGS) -c $< -o $@

build/%.o: build/prepared
	$(CC) $(CPPFLAGS) $(CFLAGS) -include port_stdio.h -c build/$*.c -o $@

applet.o runtime.o softmath.o: build/prepared
applet.o: ../NeoFontTerminal/glyphs.h

setjmp.o: ../MicroPython/os3k/setjmp.asm
	$(AS) $(ASFLAGS) $< -o $@
	$(OBJCOPY) --redefine-sym setjmp=_setjmp $@

UCBLogo.OS3KApp: Makefile

HEAP_KIB ?= 48
PROFILE_GC ?= 0
runtime.o: CPPFLAGS += -DLOGO_HEAP_KIB=$(HEAP_KIB) $(if $(filter 1,$(PROFILE_GC)),-DLOGO_PROFILE_GC)
