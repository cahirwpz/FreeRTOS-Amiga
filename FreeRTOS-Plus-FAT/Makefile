TOPDIR = $(realpath ..)

PORT_DIR = portable/m68k-amiga

SOURCES = \
	ff_crc.c \
	ff_dev_support.c \
	ff_dir.c \
	ff_error.c \
	ff_fat.c \
	ff_file.c \
	ff_format.c \
	ff_ioman.c \
	ff_locking.c \
	ff_memory.c \
	ff_stdio.c \
	ff_string.c \
	ff_sys.c \
	ff_time.c \

FORMAT-EXCLUDE = $(SOURCES)

LIBNAME = freertos-plus-fat.lib

include $(TOPDIR)/build/build.lib.mk

CPPFLAGS = -I$(TOPDIR)/FreeRTOS-Plus-FAT/include \
	   -I$(TOPDIR)/FreeRTOS-Plus-FAT/$(PORT_DIR) \
           -I$(TOPDIR)/FreeRTOS/include \
	   -I$(TOPDIR)/FreeRTOS/$(PORT_DIR) \
	   -I$(TOPDIR)/include \
	   -I$(TOPDIR)

# vim: ts=8 sw=8 noet
