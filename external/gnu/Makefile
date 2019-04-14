CURL = curl -L
RM = rm -f

ISL = isl-0.21
MPFR = mpfr-4.0.2
GMP = gmp-6.1.2
MPC = mpc-1.1.0
BINUTILS = binutils-2.32
GCC = gcc-8.3.0

ROOTDIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
HOSTDIR := $(ROOTDIR)/host
PREFIX ?= $(ROOTDIR)/local
TARGET = m68k-elf

all: gcc-install

clean: gmp-clean mpfr-clean mpc-clean isl-clean binutils-clean gcc-clean
	$(RM) -r host
	$(RM) *~

### The GNU Multiple Precision Arithmetic Library

$(GMP).tar.xz:
	$(CURL) -o $@ "https://gmplib.org/download/gmp/$(GMP).tar.xz"

gmp-unpack: $(GMP)/.unpack
$(GMP)/.unpack: $(GMP).tar.xz
	tar xvJf $^
	touch $@

gmp-configure: $(GMP)/.configure
$(GMP)/.configure: $(GMP)/.unpack
	cd $(GMP) && ./configure \
		--disable-shared --prefix=$(HOSTDIR)
	touch $@

gmp-build: $(GMP)/.build
$(GMP)/.build: $(GMP)/.configure
	cd $(GMP) && $(MAKE)
	touch $@

gmp-install: $(GMP)/.install
$(GMP)/.install: $(GMP)/.build
	cd $(GMP) && $(MAKE) install
	touch $@

gmp-clean:
	$(RM) -r $(GMP)

.PHONY: gmp-unpack gmp-configure gmp-build gmp-install gmp-clean

### Library for multiple-precision floating-point computations

$(MPFR).tar.xz:
	$(CURL) -o $@ "https://www.mpfr.org/mpfr-current/$(MPFR).tar.xz"

mpfr-unpack: $(MPFR)/.unpack
$(MPFR)/.unpack: $(MPFR).tar.xz
	tar xvJf $^
	touch $@

mpfr-configure: $(MPFR)/.configure
$(MPFR)/.configure: $(MPFR)/.unpack
	cd $(MPFR) && ./configure \
		--disable-shared --prefix=$(HOSTDIR) --with-gmp=$(HOSTDIR)
	touch $@

mpfr-build: $(MPFR)/.build
$(MPFR)/.build: $(MPFR)/.configure
	cd $(MPFR) && $(MAKE)
	touch $@

mpfr-install: $(MPFR)/.install
$(MPFR)/.install: $(MPFR)/.build
	cd $(MPFR) && $(MAKE) install
	touch $@

mpfr-clean:
	$(RM) -r $(MPFR)

.PHONY: mpfr-unpack mpfr-configure mpfr-build mpfr-install mpfr-clean

### Library for the arithmetic of complex numbers

$(MPC).tar.xz:
	$(CURL) -o $@ "https://ftp.gnu.org/gnu/mpc/$(MPC).tar.gz"

mpc-unpack: $(MPC)/.unpack
$(MPC)/.unpack: $(MPC).tar.xz
	tar xvJf $^
	touch $@

mpc-configure: $(MPC)/.configure
$(MPC)/.configure: $(MPC)/.unpack $(GMP)/.install $(MPFR)/.install
	cd $(MPC) && ./configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-gmp=$(HOSTDIR) \
		--with-mpfr=$(HOSTDIR)
	touch $@

mpc-build: $(MPC)/.build
$(MPC)/.build: $(MPC)/.configure
	cd $(MPC) && $(MAKE)
	touch $@

mpc-install: $(MPC)/.install
$(MPC)/.install: $(MPC)/.build
	cd $(MPC) && $(MAKE) install
	touch $@

mpc-clean:
	$(RM) -r $(MPC)

.PHONY: mpc-unpack mpc-configure mpc-build mpc-install mpc-clean

### Integer Set Library

$(ISL).tar.xz:
	$(CURL) -o $@ "http://isl.gforge.inria.fr/$(ISL).tar.xz"

isl-unpack: $(ISL)/.unpack
$(ISL)/.unpack: $(ISL).tar.xz
	tar xvJf $^
	touch $@

isl-configure: $(ISL)/.configure
$(ISL)/.configure: $(ISL)/.unpack $(GMP)/.install
	cd $(ISL) && ./configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-gmp-prefix=$(HOSTDIR)
	touch $@

isl-build: $(ISL)/.build
$(ISL)/.build: $(ISL)/.configure
	cd $(ISL) && $(MAKE)
	touch $@

isl-install: $(ISL)/.install
$(ISL)/.install: $(ISL)/.build
	cd $(ISL) && $(MAKE) install
	touch $@

isl-clean:
	$(RM) -r $(ISL)

.PHONY: isl-unpack isl-configure isl-build isl-install isl-clean

### GNU Binutils

$(BINUTILS).tar.xz:
	$(CURL) -o $@ "https://ftp.gnu.org/gnu/binutils/$(BINUTILS).tar.xz"

binutils-unpack: $(BINUTILS)/.unpack
$(BINUTILS)/.unpack: $(BINUTILS).tar.xz
	tar xvJf $^
	touch $@

binutils-configure: $(BINUTILS)/.configure
$(BINUTILS)/.configure: $(BINUTILS)/.unpack $(ISL)/.install $(MPC)/.install
	cd $(BINUTILS) && ./configure \
		--target=m68k-elf \
		--disable-nls \
		--prefix=$(PREFIX) \
		--with-gmp=$(HOSTDIR) \
		--with-mpfr=$(HOSTDIR) \
		--with-mpc=$(HOSTDIR) \
		--with-isl=$(HOSTDIR)
	touch $@

binutils-build: $(BINUTILS)/.build
$(BINUTILS)/.build: $(BINUTILS)/.configure
	cd $(BINUTILS) && $(MAKE)
	touch $@

binutils-install: $(BINUTILS)/.install
$(BINUTILS)/.install: $(BINUTILS)/.build
	cd $(BINUTILS) && $(MAKE) install
	touch $@

binutils-clean:
	$(RM) -r $(BINUTILS)

.PHONY: binutils-unpack binutils-configure binutils-build binutils-install binutils-clean

### GNU Compiler Collection

$(GCC).tar.xz:
	$(CURL) -o $@ "https://ftp.gnu.org/gnu/gcc/$(GCC)/$(GCC).tar.xz"

gcc-unpack: $(GCC)/.unpack
$(GCC)/.unpack: $(GCC).tar.xz
	tar xvJf $^
	touch $@

gcc-configure: $(GCC)/.configure
$(GCC)/.configure: $(GCC)/.unpack $(BINUTILS)/.install
	cd $(GCC) && PATH=$(PREFIX)/bin:$$PATH ./configure \
		--target=m68k-elf \
		--disable-nls \
		--enable-languages=c \
		--without-headers \
		--prefix=$(PREFIX) \
		--with-gmp=$(HOSTDIR) \
		--with-mpfr=$(HOSTDIR) \
		--with-mpc=$(HOSTDIR) \
		--with-isl=$(HOSTDIR)
	touch $@

gcc-build: $(GCC)/.build
$(GCC)/.build: $(GCC)/.configure
	cd $(GCC) && $(MAKE) all-gcc
	touch $@

gcc-install: $(GCC)/.install
$(GCC)/.install: $(GCC)/.build
	cd $(GCC) && $(MAKE) install-gcc
	touch $@

gcc-clean:
	$(RM) -r $(GCC)

.PHONY: gcc-unpack gcc-configure gcc-build gcc-install gcc-clean
