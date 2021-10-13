CORES = $(shell nproc)
TARGET = m68k-elf
PREFIX = ${HOME}/.local
ASFLAGS_FOR_TARGET = --pcrel
CFLAGS_FOR_TARGET = -mpcrel -fomit-frame-pointer -ffixed-a5 -fshort-enums
BINUTILS_VERSION = binutils-2.36
GCC_VERSION = gcc-11.1.0
NEWLIB_VERSION = newlib-4.1.0

install: binutils gcc newlib libgcc

binutils: build/binutils/Makefile
	$(MAKE) -C build/binutils -j${CORES}
	$(MAKE) -C build/binutils install

gcc: binutils build/gcc/Makefile
	$(MAKE) -C build/gcc -j${CORES} all-gcc
	$(MAKE) -C build/gcc install-gcc

newlib: gcc build/newlib/Makefile
	$(MAKE) -C build/newlib -j${CORES}
	$(MAKE) -C build/newlib install

libgcc: newlib
	$(MAKE) -C build/gcc -j${CORES} all-target-libgcc
	$(MAKE) -C build/gcc install-target-libgcc

clean:
	rm -rf build

src/${BINUTILS_VERSION}/configure:
	mkdir -p src
	cd src; wget https://ftp.gnu.org/pub/gnu/binutils/${BINUTILS_VERSION}.tar.xz -O - | tar xJ

src/${GCC_VERSION}/configure:
	mkdir -p src
	cd src; wget https://ftp.gnu.org/pub/gnu/gcc/${GCC_VERSION}/${GCC_VERSION}.tar.xz -O - | tar xJ

src/${NEWLIB_VERSION}/configure:
	mkdir -p src
	cd src; wget ftp://sourceware.org/pub/newlib/${NEWLIB_VERSION}.tar.gz -O - | tar xz

build/binutils/Makefile: src/${BINUTILS_VERSION}/configure
	mkdir -p build/binutils
	cd build/binutils; ../../src/${BINUTILS_VERSION}/configure --prefix=${PREFIX} --target=${TARGET}

build/gcc/Makefile: src/${GCC_VERSION}/configure
	mkdir -p build/gcc
	cd build/gcc; ../../src/${GCC_VERSION}/configure --prefix=${PREFIX} --target=${TARGET} --with-cpu=68000 --disable-multilib --enable-languages=c --with-newlib --disable-lto --disable-libmudflap --disable-libssp --disable-libgomp --disable-threads --disable-nls --disable-libquadmath --with-gnu-as --with-gnu-ld --without-headers

build/newlib/Makefile: src/${NEWLIB_VERSION}/configure
	mkdir -p build/newlib
	cd build/newlib; ../../src/${NEWLIB_VERSION}/configure --prefix=${PREFIX} --target=${TARGET} --with-cpu=68000 --disable-multilib --disable-newlib-multithread --disable-newlib-io-float --enable-lite-exit --disable-newlib-supplied-syscalls
