TOPDIR = $(realpath ..)

SOURCES = \
	__divsi3.S \
	__modsi3.S \
	__mulsi3.S \
	__udivsi3.S \
	__umodsi3.S \
	bzero.S \
	dhexdump.c \
	dprintf.c \
	dputchar.S \
	kvprintf.c \
	memcpy.S \
	memset.S \
	strlen.S

all: build

include $(TOPDIR)/build/build.vbcc.mk

build-here: c.lib

c.lib: $(OBJECTS)

CLEAN-FILES += c.lib

# vim: ts=8 sw=8 noet
