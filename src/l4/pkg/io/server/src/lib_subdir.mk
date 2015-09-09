ifneq ($(SYSTEM),)
# Io is C++11
SRC_CC_IS_CXX11    := c++0x
PRIVATE_INCDIR     += $(PKGDIR)/server/src

# do not egenerate PC files for this lib
PC_FILENAMES       :=
TARGET             := builtin.o.a
NOTARGETSTOINSTALL :=1
SUBDIRS            += $(SUBDIRS_$(ARCH)) $(SUBDIRS_$(OSYSTEM))
SUBDIR_TARGETS     := $(addsuffix /OBJ-$(SYSTEM)/builtin.o.a,$(SUBDIRS))
SUBDIR_OBJS         = $(addprefix $(OBJ_DIR)/,$(SUBDIR_TARGETS))
OBJS_builtin.o.a   += $(SUBDIR_OBJS)

# the all target must be first!
all::

# our bultin.o.a dependency
$(TARGET): $(SUBDIR_OBJS)

$(SUBDIR_OBJS): $(OBJ_DIR)/%/OBJ-$(SYSTEM)/builtin.o.a: %
	$(VERBOSE)$(MAKE) $(MAKECMDGOALS) -C $(SRC_DIR)/$* $(MKFLAGS)
endif

include $(L4DIR)/mk/lib.mk
