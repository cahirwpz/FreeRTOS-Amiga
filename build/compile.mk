ifndef SOURCES
$(error SOURCES is not set)
endif

SOURCES_C = $(filter %.c,$(SOURCES))
SOURCES_ASM = $(filter %.S,$(SOURCES))
OBJECTS += $(SOURCES_C:%.c=%.o) $(SOURCES_ASM:%.S=%.o)
DEPFILES = $(foreach f,$(SOURCES_C) $(SOURCES_ASM), \
	    $(dir $(f))$(patsubst %.c,.%.D,$(patsubst %.S,.%.D,$(notdir $(f)))))

HOSTCC = gcc -nostdinc

define emit_dep_rule
CFILE = $(1)
DFILE = $(dir $(1))$(patsubst %.c,.%.D,$(patsubst %.S,.%.D,$(notdir $(1))))
$$(DFILE): $$(CFILE)
	@echo "[DEP] $$(DIR)$$@"
	$$(HOSTCC) $$(CPPFLAGS) -MM -MG $$^ -o $$@
endef

$(foreach file,$(SOURCES),$(eval $(call emit_dep_rule,$(file))))

ifeq ($(words $(findstring $(MAKECMDGOALS), download clean distclean)), 0)
  -include $(DEPFILES)
endif

BUILD-FILES += $(OBJECTS)
CLEAN-FILES += $(DEPFILES) $(OBJECTS)
PRECIOUS-FILES += $(OBJECTS)
