TOPDIR = $(CURDIR)

RTOS_DIR = FreeRTOS
MMAN_DIR = FreeRTOS/portable/MemMang
PORT_DIR = FreeRTOS/portable/VBCC/m68k

SOURCES = startup.c intr.c main.c \
	  $(RTOS_DIR)/tasks.c \
	  $(RTOS_DIR)/queue.c \
	  $(RTOS_DIR)/list.c \
	  $(MMAN_DIR)/heap_1.c \
	  $(PORT_DIR)/port.c \
	  $(PORT_DIR)/portasm.S

	  # $(RTOS_DIR)/croutine.c \
	  $(RTOS_DIR)/stream_buffer.c \
	  $(RTOS_DIR)/event_groups.c \
	  $(RTOS_DIR)/timers.c \

all: build

include $(TOPDIR)/build/build.vbcc.mk

build-here: bootloader.bin freertos.adf

freertos.exe: $(OBJECTS)

freertos.adf: freertos.exe

vbcc:
	make -C external/vbcc DESTDIR=$(PWD)/toolchain

clean-here:
	$(RM) bootloader.bin
	$(RM) *.adf *.exe *~

.PHONY: build clean vbcc
