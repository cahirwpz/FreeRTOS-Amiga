TOPDIR = $(CURDIR)

SUBDIR = libsa FreeRTOS
SOURCES = startup.c main.c

all: build

include $(TOPDIR)/build/build.vbcc.mk

build-here: bootloader.bin freertos.adf

freertos.exe: $(OBJECTS) libsa/sa.lib FreeRTOS/freertos.lib

freertos.adf: freertos.exe

vbcc:
	make -C external/vbcc DESTDIR=$(PWD)/toolchain

run: freertos.adf
	./launch $^

clean-here:
	$(RM) bootloader.bin
	$(RM) *.adf *.exe *~

.PHONY: vbcc run
