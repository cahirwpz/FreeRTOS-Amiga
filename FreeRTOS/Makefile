TOPDIR = $(realpath ..)

PORT_DIR = portable/m68k-amiga

SOURCES = \
	  croutine.c \
	  event_groups.c \
	  list.c \
	  queue.c \
	  stream_buffer.c \
	  tasks.c \
	  timers.c \
	  $(PORT_DIR)/heap.c \
	  $(PORT_DIR)/port.c \
	  $(PORT_DIR)/intsrv.c \
	  $(PORT_DIR)/intr.S \
	  $(PORT_DIR)/trap.S \
	  $(PORT_DIR)/portasm.S
FORMAT-EXCLUDE = $(SOURCES)

LIBNAME = freertos.lib

CFLAGS.event_groups := -Wno-unused-but-set-variable
CFLAGS.tasks := -Wno-unused-parameter
CFLAGS.queue := -Wno-unused-parameter

include $(TOPDIR)/build/build.lib.mk

CPPFLAGS = -I$(TOPDIR)/FreeRTOS/include \
	   -I$(TOPDIR)/FreeRTOS/$(PORT_DIR) \
	   -I$(TOPDIR)/include \
	   -I$(TOPDIR)

# vim: ts=8 sw=8 noet
