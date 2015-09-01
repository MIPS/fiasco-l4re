PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

TARGET   = libgfxbitmap.a 
SRC_C    = font.c bitmap.c

FONTS    = lat0-12.psf lat0-14.psf lat0-16.psf vgafont.psf
OBJS    += $(FONTS:.psf=.o)

include $(L4DIR)/mk/lib.mk

%.o: %.psf
	@$(COMP_MESSAGE)
ifneq ($(ARCH),mips)
	$(VERBOSE)$(LD) -r --oformat $(OFORMAT) -o $@ -b binary $^
else
#	Work around issue with ld and objcopy setting default arch to mips:3000
	$(VERBOSE)$(TOUCH) fake.c
	$(CC) $(CARCHFLAGS) -nostdlib -r -o $@ fake.c -Wl,--format=binary $^
	$(VERBOSE)$(RM) fake.c
endif

%.psf: $(SRC_DIR)/%.psf
	@ln -s $? $@
