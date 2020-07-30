TOPDIR = $(realpath ../..)

PROGRAM = filesys
SOURCES = main.c filesys.c reader.c shell.c
OBJECTS = ../startup.o ../fault.o ../trap.o 
LAUNCHOPTS = --window serial
ADF-EXTRA = $(wildcard *.c *.h)

include $(TOPDIR)/build/build.prog.mk
