CURL = curl -L
RM = rm -f
INSTALL = install

CC = gcc -g
CFLAGS = -O2 -Wall

PREFIX ?= $(PWD)/local
GETTEXT_PATH = $(wildcard /usr/local/Cellar/gettext/*)

VER = 3.0.0

all: install

fs-uae-$(VER).tar.gz:
	$(CURL) -o $@ "https://fs-uae.net/stable/$(VER)/fs-uae-$(VER).tar.gz"

fs-uae/.unpack: fs-uae-$(VER).tar.gz
	tar xvzf $^
	mv fs-uae-$(VER) fs-uae
	touch $@

patch: .pc/applied-patches
.pc/applied-patches: fs-uae/.unpack
	quilt push -a

configure: fs-uae/.configure
fs-uae/.configure: .pc/applied-patches
	cd fs-uae && PATH=$(GETTEXT_PATH)/bin:$$PATH ./configure \
		--prefix=$(PREFIX) \
		--enable-gdbstub \
		--without-libmpeg2 \
		--disable-a2065 \
		--disable-action-replay \
		--disable-ahi \
		--disable-arcadia \
		--disable-builtin-slirp \
		--disable-gfxboard \
		--disable-jit \
		--disable-lua \
		--disable-netplay \
		--disable-pearpc-cpu \
		--disable-ppc \
		--disable-prowizard \
		--disable-slirp \
		--disable-qemu-cpu \
		--disable-qemu-slirp \
		--disable-uaenet \
		--disable-uaescsi \
		--disable-uaeserial \
		--disable-dms \
		--disable-zip
	touch $@

build: fs-uae/.build
fs-uae/.build: fs-uae/.configure
	cd fs-uae && $(MAKE)
	touch $@

install: fs-uae/.install
fs-uae/.install: fs-uae/.build
	cd fs-uae && $(MAKE) install
	touch $@

force-build:
	$(RM) fs-uae/.build fs-uae/.install
	$(MAKE) install

clean:
	cd fs-uae && rm -f .install .build .configure
	-$(MAKE) -C fs-uae clean
	-quilt pop -a

.PHONY: patch configure build install force-build
