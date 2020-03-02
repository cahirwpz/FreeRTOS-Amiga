TOPDIR = $(realpath ..)

SOURCES = \
	gen/divsi3.S \
	gen/modsi3.S \
	gen/mulsi3.S \
	gen/udivsi3.S \
	gen/umodsi3.S \
	stdio/hexdump.c \
	stdio/kvprintf.c \
	stdio/printf.c \
	stdio/putchar.S \
	string/bzero.S \
	string/ffs.S \
	string/memcpy.S \
	string/memset.S \
	string/strcat.S \
	string/strchr.S \
	string/strcmp.S \
	string/strcpy.S \
	string/strlen.S \
	string/strncmp.S \
	string/strncpy.S \
	string/strrchr.S

BUILD-FILES = c.lib

all: build

include $(TOPDIR)/build/build.gcc.mk

c.lib: $(OBJECTS)

# vim: ts=8 sw=8 noet
