TOPDIR = $(realpath ..)

SOURCES = \
	  cia-frame.c \
	  cia-line.c \
	  cia-timer.c \
	  floppy.c \
	  serial.c

all: build

include $(TOPDIR)/build/build.vbcc.mk

build-here: drivers.lib

drivers.lib: $(OBJECTS)

CLEAN-FILES += drivers.lib

# vim: ts=8 sw=8 noet
