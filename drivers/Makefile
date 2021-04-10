TOPDIR = $(realpath ..)

SOURCES = \
	  blt-copy.c \
	  blt-line.c \
	  bitmap.c \
	  cia-frame.c \
	  cia-icr.c \
	  cia-line.c \
	  cia-timer.c \
	  console.c \
	  copper.c \
	  floppy.c \
	  floppy-mfm.c \
	  input.c \
	  keyboard.c \
	  memdev.c \
	  mouse.c \
	  palette.c \
	  parallel-putc.S \
	  serial.c \
	  sprite.c \
	  tty.c

SOURCES_GEN = data/drdos8x8.c \
	      data/koi8r.8x8.c \
	      data/lat2-08.c \
	      data/pointer.c

PSF2C.drdos8x8 := --name console
PSF2C.koi8r.8x8 := --name console
PSF2C.lat2-08 := --name console
PNG2C.pointer := --sprite pointer_spr,16,1 --palette pointer_pal,4

LIBNAME = drivers.lib

include $(TOPDIR)/build/build.lib.mk

# vim: ts=8 sw=8 noet
