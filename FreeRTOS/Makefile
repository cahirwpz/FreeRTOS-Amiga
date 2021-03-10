TOPDIR = $(realpath ..)

SOURCES = croutine.c \
	  event_groups.c \
	  list.c \
	  queue.c \
	  stream_buffer.c \
	  tasks.c \
	  timers.c

FORMAT-EXCLUDE = $(SOURCES) $(SOURCES_H)

LIBNAME = freertos.lib

CFLAGS.event_groups := -Wno-unused-but-set-variable
CFLAGS.tasks := -Wno-unused-parameter
CFLAGS.queue := -Wno-unused-parameter

CPPFLAGS = -I$(TOPDIR)/FreeRTOS/include

include $(TOPDIR)/build/build.lib.mk

# vim: ts=8 sw=8 noet
