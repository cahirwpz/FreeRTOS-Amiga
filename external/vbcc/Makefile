CURL = curl -L
RM = rm -f

VASM = vasm1_8e
VLINK = vlink0_16b
VBCC = vbcc0_9fP1
NDK = NDK_3.9

PREFIX ?= $(PWD)/local
TARGET = m68k-amigaos

all: vasm-install vlink-install vbcc-install ndk-install

clean: vasm-clean vlink-clean vbcc-clean ndk-clean
	$(RM) *~

### recipes

%: %.tar.gz
	tar xvzf $^
	touch $@

### VASM

vasm.tar.gz:
	$(CURL) -o $@ "http://server.owl.de/~frank/tags/$(VASM).tar.gz"

vasm-build: vasm/vasmm68k_mot vasm/vobjdump
vasm/vasmm68k_mot vasm/vobjdump: vasm
	make -C vasm CPU=m68k SYNTAX=mot

vasm-install: vasm-build
	mkdir -p "$(PREFIX)/$(TARGET)/bin"
	mkdir -p "$(PREFIX)/bin"
	cp "vasm/vasmm68k_mot" "$(PREFIX)/$(TARGET)/bin"
	cp "vasm/vobjdump" "$(PREFIX)/bin"
	$(RM) "$(PREFIX)/bin/vasm"
	echo "#!/bin/sh" >> "$(PREFIX)/bin/vasm"
	echo "$(PREFIX)/$(TARGET)/bin/vasmm68k_mot " \
	     "-I$(PREFIX)/$(TARGET)/ndk/include \"\$$@\"" \
		>> "$(PREFIX)/bin/vasm"
	chmod a+x $(PREFIX)/bin/vasm

vasm-clean:
	$(RM) -r vasm
	$(RM) vasm.tar.gz

### VLINK

vlink.tar.gz:
	$(CURL) -o $@ "http://server.owl.de/~frank/tags/$(VLINK).tar.gz"

vlink-build: vlink/vlink
vlink/vlink: vlink
	make -C vlink

vlink-install: vlink-build
	cp "vlink/vlink" "$(PREFIX)/bin/vlink"

vlink-clean:
	$(RM) -r vlink
	$(RM) vlink.tar.gz

### VBCC

vbcc.tar.gz:
	$(CURL) -o $@ "http://server.owl.de/~frank/tags/$(VBCC).tar.gz"

vbcc: vbcc.tar.gz
	tar xvzf $^
	patch -p0 < vbcc-etc.patch
	touch $@

vbcc-build: vbcc
	mkdir -p vbcc/bin
	yes '' | make -C vbcc TARGET=m68k ETCDIR='\"$(PREFIX)/etc/\"'

vbcc-install: vbcc-build
	cp "vbcc/bin/vbccm68k" "$(PREFIX)/$(TARGET)/bin"
	cp "vbcc/bin/vc" "$(PREFIX)/bin"
	cp "vbcc/bin/vprof" "$(PREFIX)/bin"
	mkdir -p "$(PREFIX)/etc"
	sed -e 's#%TARGET%#$(TARGET)#g;s#%PREFIX%#$(PREFIX)#g' vc.config.in \
		> "$(PREFIX)/etc/vc.config"

vbcc-clean:
	$(RM) -r vbcc
	$(RM) vbcc.tar.gz

### NDK

$(NDK).lha:
	$(CURL) -o $@ "http://www.haage-partner.de/download/AmigaOS/NDK39.lha"

ndk: $(NDK).lha
	lha x $^
	$(RM) $(NDK).info
	mv $(NDK) ndk
	touch $@

ndk-install: ndk
	mkdir -p "$(PREFIX)/$(TARGET)/include"
	cp -r "ndk/Include/include_i/" "$(PREFIX)/$(TARGET)/include"

ndk-clean:
	$(RM) -r ndk
	$(RM) $(NDK).lha

### other stuff

.PHONY: vasm-build vlink-build vbcc-build \
	vasm-install vlink-install vbcc-install vclib-install ndk-install \
	vasm-clean vlink-clean vbcc-clean vclib-clean ndk-clean
