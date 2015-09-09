PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= $(PKGNAME)_aes_linux.o.a $(PKGNAME)_aes_linux.o.p.a
SYSTEMS = x86 arm amd64
PC_FILENAME = $(PKGNAME)_aes_linux

# list your .c files here
SRC_C	= aes.c

PRIVATE_INCDIR = $(PKGDIR)/include

include $(L4DIR)/mk/lib.mk
