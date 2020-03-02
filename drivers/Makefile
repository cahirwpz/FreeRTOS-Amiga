TOPDIR = $(realpath ..)

SOURCES = \
	  cia-frame.c \
	  cia-icr.c \
	  cia-line.c \
	  cia-timer.c \
	  event.c \
	  floppy.c \
	  floppy-mfm.c \
	  keyboard.c \
	  mouse.c \
	  serial.c

BUILD-FILES = drivers.lib

all: build

include $(TOPDIR)/build/build.gcc.mk

drivers.lib: $(OBJECTS)

# vim: ts=8 sw=8 noet
