TOPDIR = $(realpath ../..)

SOURCES = atoi.c \
	  gets.c \
	  malloc.c \
	  printf.c \
	  syscalls.c \
	  gen/divsi3.S \
	  gen/mulsi3.S \
	  gen/modsi3.S \
	  gen/udivsi3.S \
	  gen/umodsi3.S \
	  string/memcpy.S \
	  string/memset.S \
	  string/strchr.S \
	  string/strcmp.S \
	  string/strcpy.S \
	  string/strlen.S

LIBNAME = c.lib

BUILD-FILES += crt0.o
CLEAN-FILES += crt0.o

CPPFLAGS = -I$(TOPDIR)/xv6/include

include $(TOPDIR)/build/build.lib.mk

# vim: ts=8 sw=8 noet
