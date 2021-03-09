TOPDIR = $(realpath ..)

SOURCES = heap.c \
	  port.c \
	  intsrv.c \
	  intr.S \
	  trap.S \
	  portasm.S

LIBNAME = kernel.lib

CPPFLAGS = -I$(TOPDIR)/kernel/include

include $(TOPDIR)/build/build.lib.mk

# vim: ts=8 sw=8 noet
