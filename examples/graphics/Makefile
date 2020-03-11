TOPDIR = $(realpath ../..)

PROGRAM = graphics
SOURCES = main.c
SOURCES_GEN = data/simpsons-bg.c data/bart.c data/homer.c data/marge.c \
	      data/screen.c
OBJECTS = ../startup.o ../fault.o ../trap.o

include $(TOPDIR)/build/build.prog.mk

data/simpsons-bg.c: PNG2C += --name simpsons --palette
data/bart.c: PNG2C += --mask
data/homer.c: PNG2C += --mask
data/marge.c: PNG2C += --mask
data/screen.c:
	$(GENSTRUCT) --bitmap 320x176x4 screen0 > $@
	$(GENSTRUCT) --bitmap 320x176x4 screen1 >> $@
