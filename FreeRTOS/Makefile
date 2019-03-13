TOPDIR = $(realpath ..)

MMAN_DIR = portable/MemMang
PORT_DIR = portable/VBCC/m68k

SOURCES = \
	  croutine.c \
	  event_groups.c \
	  list.c \
	  queue.c \
	  stream_buffer.c \
	  tasks.c \
	  timers.c \
	  $(MMAN_DIR)/heap_5.c \
	  $(PORT_DIR)/port.c \
	  $(PORT_DIR)/portasm.S

all: build

include $(TOPDIR)/build/build.vbcc.mk

build-here: freertos.lib

freertos.lib: $(OBJECTS)

# vim: ts=8 sw=8 noet
