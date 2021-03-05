TOPDIR = $(realpath ..)

PORT_DIR = portable/m68k-amiga

SOURCES = \
	FreeRTOS_ARP.c \
	FreeRTOS_DHCP.c \
	FreeRTOS_DNS.c \
	FreeRTOS_IP.c \
	FreeRTOS_Sockets.c \
	FreeRTOS_Stream_Buffer.c \
	FreeRTOS_TCP_IP.c \
	FreeRTOS_TCP_WIN.c \
	FreeRTOS_UDP_IP.c
FORMAT-EXCLUDE = $(SOURCES)

LIBNAME = freertos-plus-tcp.lib

include $(TOPDIR)/build/build.lib.mk

CPPFLAGS = -I$(TOPDIR)/FreeRTOS-Plus-TCP/include \
	   -I$(TOPDIR)/FreeRTOS-Plus-TCP/portable/Compiler/GCC \
	   -I$(TOPDIR)/FreeRTOS-Plus-TCP/$(PORT_DIR) \
           -I$(TOPDIR)/FreeRTOS/include \
	   -I$(TOPDIR)/FreeRTOS/$(PORT_DIR) \
	   -I$(TOPDIR)/include \
	   -I$(TOPDIR)

# vim: ts=8 sw=8 noet
