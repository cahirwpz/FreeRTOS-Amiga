all: build

ifdef LIBNAME
BUILD-FILES += $(LIBNAME)
endif

%.lib:
	@echo "[AR] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(AR) rcs $@ $^
	$(RANLIB) $@

include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/flags.mk
include $(TOPDIR)/build/common.mk

$(LIBNAME): $(OBJECTS)
