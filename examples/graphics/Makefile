TOPDIR = $(realpath ../..)

PROGRAM = graphics
SOURCES = main.c
SOURCES_GEN = data/simpsons-bg.c data/bart.c data/homer.c data/marge.c \
	      data/screen.c
OBJECTS = ../startup.o ../fault.o ../trap.o

PNG2C.simpsons-bg := --bitmap simpsons_bm,320x176x4 --palette simpsons_pal,16
PNG2C.bart := --bitmap bart_bm,256x31x4,+mask
PNG2C.homer := --bitmap homer_bm,192x43x4,+mask
PNG2C.marge := --bitmap marge_bm,192x56x4,+mask

include $(TOPDIR)/build/build.prog.mk

data/screen.c:
	$(GENSTRUCT) --bitmap 320x176x4 screen0 > $@
	$(GENSTRUCT) --bitmap 320x176x4 screen1 >> $@
