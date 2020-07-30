TOPDIR = $(realpath ..)

PORT_DIR = portable/m68k-amiga

SOURCES = \
	  FreeRTOS_CLI.c
FORMAT-EXCLUDE = $(SOURCES)

LIBNAME = freertos-plus.lib

include $(TOPDIR)/build/build.lib.mk

CPPFLAGS = -I$(TOPDIR)/FreeRTOS-Plus/include \
	   -I$(TOPDIR)/FreeRTOS-Plus/$(PORT_DIR) \
           -I$(TOPDIR)/FreeRTOS/include \
	   -I$(TOPDIR)/FreeRTOS/$(PORT_DIR) \
	   -I$(TOPDIR)/include \
	   -I$(TOPDIR)

# vim: ts=8 sw=8 noet
