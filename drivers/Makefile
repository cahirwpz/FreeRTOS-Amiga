TOPDIR = $(realpath ..)

SOURCES = \
	  cia-frame.c \
	  cia-icr.c \
	  cia-line.c \
	  cia-timer.c \
	  event.c \
	  floppy.c \
	  floppy-mfm.c \
	  mouse.c \
	  serial.c

all: build

include $(TOPDIR)/build/build.vbcc.mk

build-here: drivers.lib

drivers.lib: $(OBJECTS)

CLEAN-FILES += drivers.lib

# vim: ts=8 sw=8 noet
