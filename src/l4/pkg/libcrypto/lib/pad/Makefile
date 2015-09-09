PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= $(PKGNAME)_pad.a $(PKGNAME)_pad.p.a

SYSTEMS = x86 arm amd64
PC_FILENAME = $(PKGNAME)_pad

# list your .c files here
SRC_C	= oaep.c

PRIVATE_INCDIR = $(PKGDIR)/include

include $(L4DIR)/mk/lib.mk
