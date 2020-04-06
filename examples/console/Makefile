TOPDIR = $(realpath ../..)

PROGRAM = console
SOURCES = main.c console.c
SOURCES_GEN = data/drdos8x8.c data/koi8r.8x8.c data/lat2-08.c data/pointer.c
OBJECTS = ../startup.o ../fault.o ../trap.o

PSF2C.drdos8x8 := --name console
PSF2C.koi8r.8x8 := --name console
PSF2C.lat2-08 := --name console
PNG2C.pointer := --sprite pointer_spr,16,1 --palette pointer_pal,4

include $(TOPDIR)/build/build.prog.mk
