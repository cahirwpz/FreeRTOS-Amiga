TOPDIR = $(realpath ../..)

PROGRAM = graphics
SOURCES = main.c
SOURCES_GEN = data/simpsons-bg.c data/bart.c data/homer.c data/marge.c \
	      data/screen.c
OBJECTS = ../startup.o ../fault.o ../trap.o

PNG2C.simpsons-bg := --name simpsons --palette
PNG2C.bart := --mask
PNG2C.homer := --mask
PNG2C.marge := --mask

include $(TOPDIR)/build/build.prog.mk

data/screen.c:
	$(GENSTRUCT) --bitmap 320x176x4 screen0 > $@
	$(GENSTRUCT) --bitmap 320x176x4 screen1 >> $@
