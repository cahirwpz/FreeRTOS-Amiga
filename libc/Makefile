TOPDIR = $(realpath ..)

SOURCES = \
	ctype/ctype.c \
	gen/divsi3.S \
	gen/modsi3.S \
	gen/mulsi3.S \
	gen/udivsi3.S \
	gen/umodsi3.S \
	gen/longjmp.S \
	gen/setjmp.S \
	stdio/dprintf.c \
	stdio/kvprintf.c \
	stdio/snprintf.c \
	stdlib/atoi.c \
	stdlib/malloc.c \
	stdlib/rand_r.c \
	stdlib/strtol.c \
	stdlib/strtoul.c \
	string/bzero.S \
	string/ffs.S \
	string/memcmp.S \
	string/memcpy.S \
	string/memset.S \
	string/strcat.S \
	string/strchr.S \
	string/strcmp.S \
	string/strcpy.S \
	string/strcspn.c \
	string/strlen.S \
	string/strncmp.S \
	string/strncpy.S \
	string/strrchr.S \
	string/strspn.c \
	string/strtok_r.c \
	sys/chdir.c \
	sys/close.c \
	sys/dup.c \
	sys/execv.c \
	sys/exit.c \
	sys/fstat.c \
	sys/kill.c \
	sys/mkdir.c \
	sys/open.c \
	sys/pipe.c \
	sys/read.c \
	sys/sbrk.c \
	sys/stat.c \
	sys/unlink.c \
	sys/vfork.c \
	sys/wait.c \
	sys/write.c

CPPFLAGS = -D_USERSPACE

FORMAT-EXCLUDE = include/asm.h include/sys/cdefs.h

LIBNAME = c.lib

include $(TOPDIR)/build/build.lib.mk

# vim: ts=8 sw=8 noet
