PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= $(PKGNAME)_modes.a $(PKGNAME)_modes.p.a
SYSTEMS = x86 arm amd64
PC_FILENAME = $(PKGNAME)_modes

# list your .c files here
SRC_C	= cbc.c

PRIVATE_INCDIR = $(PKGDIR)/include

include $(L4DIR)/mk/lib.mk
