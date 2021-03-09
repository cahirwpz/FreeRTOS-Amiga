TOPDIR = $(realpath ..)

SOURCES = \
	  blt-copy.c \
	  blt-line.c \
	  bootcons.c \
	  bootcons-putc.S \
	  cia-frame.c \
	  cia-icr.c \
	  cia-line.c \
	  cia-timer.c \
	  ff.c \
	  ffsystem.c \
	  ffunicode.c \
	  floppy.c \
	  floppy-mfm.c \
	  keyboard.c \
	  memory-file.c \
	  mouse.c \
	  serial.c \
	  serial-file.c \
	  sprite.c

FORMAT-EXCLUDE = ff.c ffsystem.c ffunicode.c \
                 include/diskio.h include/ff.h include/ffconf.h

LIBNAME = drivers.lib

include $(TOPDIR)/build/build.lib.mk

# vim: ts=8 sw=8 noet
