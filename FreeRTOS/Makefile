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

all: build

include $(TOPDIR)/build/build.gcc.mk

CPPFLAGS = -I$(TOPDIR)/FreeRTOS/include \
	   -I$(TOPDIR)/FreeRTOS/portable/m68k-amiga \
	   -I$(TOPDIR)/include \
	   -I$(TOPDIR)

event_groups.o: CFLAGS += -Wno-unused-but-set-variable

build-here: freertos.lib

freertos.lib: $(OBJECTS)

# vim: ts=8 sw=8 noet
