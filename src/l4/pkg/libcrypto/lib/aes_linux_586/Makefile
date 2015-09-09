PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= $(PKGNAME)_aes_linux_586.o.a $(PKGNAME)_aes_linux_586.o.p.a
SYSTEMS = x86
PC_FILENAME = $(PKGNAME)_aes_linux_586

# list your .c files here
SRC_C	= aes.c
SRC_S	= aes-i586-asm.S

PRIVATE_INCDIR = $(PKGDIR)/include

include $(L4DIR)/mk/lib.mk
