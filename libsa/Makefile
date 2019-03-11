TOPDIR = $(realpath ..)

SOURCES = \
	__divsi3.S \
	__modsi3.S \
	__mulsi3.S \
	__udivsi3.S \
	__umodsi3.S \
	bzero.S \
	dprintf.c \
	kvprintf.c \
	rawio.S

all: build

include $(TOPDIR)/build/build.vbcc.mk

build-here: sa.lib

sa.lib: $(OBJECTS)

# vim: ts=8 sw=8 noet
