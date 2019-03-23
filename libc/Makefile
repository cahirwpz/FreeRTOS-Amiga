TOPDIR = $(realpath ..)

SOURCES = \
	dhexdump.c \
	dprintf.c \
	dputchar.S \
	gen/divsi3.S \
	gen/modsi3.S \
	gen/mulsi3.S \
	gen/udivsi3.S \
	gen/umodsi3.S \
	stdio/kvprintf.c \
	string/bzero.S \
	string/memcpy.S \
	string/memset.S \
	string/strlen.S

all: build

include $(TOPDIR)/build/build.vbcc.mk

build-here: c.lib

c.lib: $(OBJECTS)

CLEAN-FILES += c.lib

# vim: ts=8 sw=8 noet
