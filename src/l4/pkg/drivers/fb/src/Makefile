PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

DEFINES        := -DL4_DRIVERS
SYSTEMS        := mips-l4f
SRC_C          := fb.c fb-virtual.c
SRC_C_mips     := l4_cirrusfb.c emul_time.c vbus_pci.c
SRC_C          += $(SRC_C_$(ARCH))
TARGET         := libdrivers_fb.o.a
PC_FILENAME    := libdrivers-fb
REQUIRES_LIBS  := libio l4re_c l4re_c-util libio-vbus

PRIVATE_INCDIR += $(PKGDIR)/fb/src

# Bring in Linux contrib and headers
DEFINES        += -D__KERNEL__ -DCONFIG_PCI -UMODULE
#DEFINES        += -DCIRRUSFB_DEBUG -DDEBUG
WARNINGS = -Wall -Wstrict-prototypes $(call checkcc,-Wno-pointer-sign)

# gather Linux sources
PRIVATE_INCDIR += $(PKGDIR)/fb/linux_include $(PKGDIR)/fb/linux_include/uapi
VPATH += $(PKGDIR)/fb/contrib

SRC_C_mips-l4f  += video/modedb.c video/fbcmap.c video/cirrusfb.c \
                   video/cfbcopyarea.c video/cfbfillrect.c video/cfbimgblt.c

include $(L4DIR)/mk/lib.mk
