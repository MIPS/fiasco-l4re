PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= $(PKGNAME)_sha1_linux.a $(PKGNAME)_sha1_linux.p.a
SYSTEMS = x86 arm amd64
PC_FILENAME = $(PKGNAME)_sha1_linux

# list your .c files here
SRC_C	= sha1.c

PRIVATE_INCDIR = $(PKGDIR)/include

include $(L4DIR)/mk/lib.mk
