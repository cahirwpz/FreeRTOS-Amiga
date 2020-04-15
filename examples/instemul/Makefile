TOPDIR = $(realpath ../..)

PROGRAM = instemul
SOURCES = main.c rand.c
OBJECTS = ../startup.o ../fault.o ../trap.o

CFLAGS.rand := -m68060

include $(TOPDIR)/build/build.prog.mk
