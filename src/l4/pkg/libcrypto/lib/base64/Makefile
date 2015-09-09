PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= $(PKGNAME)_base64.o.a $(PKGNAME)_base64.o.p.a
SYSTEMS = x86 arm amd64
PC_FILENAME = $(PKGNAME)_base64

# list your .c files here
SRC_C	= base64.c

PRIVATE_INCDIR = $(PKGDIR)/include

include $(L4DIR)/mk/lib.mk
