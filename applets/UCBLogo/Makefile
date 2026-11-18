UPSTREAM ?= upstream
CORE = coms error eval files graphics init intern libloc lists logodata math mem paren parse print wrksp nographics
PORT_OBJ = applet.o runtime.o softmath.o setjmp.o $(addprefix build/,$(addsuffix .o,$(CORE)))

OBJ = $(PORT_OBJ)
TARGET = UCBLogo.OS3KApp
INT = build/prepared
include ../../Makefile.common

CFLAGS += -Ibuild -include build/config.h -DHAVE_CONFIG_H
CFLAGS += -fno-short-enums -fno-builtin -U_FORTIFY_SOURCE

build/prepared: prepare.py port_config.h port_stdio.h $(TOP)/Makefile.common
	python3 prepare.py "$(UPSTREAM)" build
	touch $@

build/%.o: build/prepared
	$(CC) $(CPPFLAGS) $(CFLAGS) -include port_stdio.h -c build/$*.c -o $@

applet.o runtime.o softmath.o: build/prepared
applet.o: ../NeoFontTerminal/glyphs.h

../NeoFontTerminal/glyphs.h: ../NeoFontTerminal/fontgen.py ../NeoFontTerminal/glyphs.png
	+$(MAKE) -C ../NeoFontTerminal glyphs.h

setjmp.o: ../MicroPython/os3k/setjmp.asm
	$(AS) $(ASFLAGS) $< -o $@
	$(OBJCOPY) --redefine-sym setjmp=_setjmp $@

$(TARGET): Makefile

HEAP_KIB ?= 48
PROFILE_GC ?= 0
runtime.o: CPPFLAGS += -DLOGO_HEAP_KIB=$(HEAP_KIB) $(if $(filter 1,$(PROFILE_GC)),-DLOGO_PROFILE_GC)
