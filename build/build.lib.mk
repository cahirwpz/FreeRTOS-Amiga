all: build

ifdef LIBNAME
BUILD-FILES += $(LIBNAME)
endif

%.lib:
	@echo "[AR] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(AR) rcs $@ $^
	$(RANLIB) $@

SOURCES_H = $(shell find include -type f -iname '*.h' 2>/dev/null)

include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/flags.mk
include $(TOPDIR)/build/common.mk

$(LIBNAME): $(OBJECTS)
