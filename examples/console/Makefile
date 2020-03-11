TOPDIR = $(realpath ../..)

PROGRAM = console
SOURCES = main.c console.c
SOURCES_GEN = data/drdos8x8.c data/koi8r.8x8.c data/lat2-08.c data/pointer.c \
	      data/screen.c
OBJECTS = ../startup.o ../fault.o ../trap.o

include $(TOPDIR)/build/build.prog.mk

data/drdos8x8.c: PSF2COPTS = --name console
data/koi8r.8x8.c: PSF2COPTS = --name console
data/lat2-08.c: PSF2COPTS = --name console
data/pointer.c: PNG2COPTS = --sprite --palette
data/screen.c:
	$(GENSTRUCT) --bitmap 640x256x1 screen > $@
