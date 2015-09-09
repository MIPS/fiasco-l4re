PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

DTBS                  = virt-omap5.dtb virt-exynos5.dtb
DEL_ON_CLEAN          = $(DTBS)
INSTALL_TARGET        = $(DTBS)
REQUIRE_HOST_TOOLS    = dtc
DTC                  ?= dtc

vpath %.dts $(SRC_DIR)

include $(L4DIR)/mk/lib.mk

%.dtb: %.dts $(SRC_DIR)/Makefile
	@$(BUILD_MESSAGE)
	$(VERBOSE)$(DTC) -O dtb -b 0 -o $@ $<
