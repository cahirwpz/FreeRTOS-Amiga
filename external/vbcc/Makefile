CURL = curl -L
RM = rm -f

VASM = vasm1_8e
NDK = NDK_3.9

PREFIX ?= $(PWD)/local
TARGET = m68k-amigaos

all: vasm-install ndk-install

clean: vasm-clean ndk-clean
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

### NDK

$(NDK).lha:
	$(CURL) -o $@ "http://www.haage-partner.de/download/AmigaOS/NDK39.lha"

ndk: $(NDK).lha
	lha x $^
	$(RM) $(NDK).info
	mv $(NDK) ndk
	touch $@

ndk-install: ndk
	mkdir -p "$(PREFIX)/$(TARGET)"
	rm -rf "$(PREFIX)/$(TARGET)/include"
	cp -r "ndk/Include/include_i" "$(PREFIX)/$(TARGET)/include" 

ndk-clean:
	$(RM) -r ndk
	$(RM) $(NDK).lha

### other stuff

.PHONY: vasm-build vasm-install ndk-install vasm-clean ndk-clean
