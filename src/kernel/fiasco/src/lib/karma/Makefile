include $(srcdir)/Makeconf
include $(srcdir)/Makerules.global

vpath %.cc $(VPATH_LIBKARMA)

all: $(notdir $(TARGET))($(OBJECTS))

$(OBJECTS): $(objbase)/.Compiler-config

$(notdir $(TARGET)): $(OBJECTS)

-include $(wildcard .*.d)
