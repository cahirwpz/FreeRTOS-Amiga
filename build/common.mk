export TOPDIR

# Current directory without common prefix
DIR = $(patsubst $(TOPDIR)/%,%,$(CURDIR)/)

# Pass "VERBOSE=1" at command line to display command being invoked by GNU Make
ifneq ($(VERBOSE), 1)
.SILENT:
endif

# Disable all built-in recipes
.SUFFIXES:

# Recursive rules for subdirectories
format-%:
	@echo "[MAKE] format $(DIR)$*"
	$(MAKE) -C $* format

build-%: before-%
	@echo "[MAKE] build $(DIR)$*"
	$(MAKE) -C $* build

PHONY-TARGETS += $(SUBDIR:%=before-%)

clean-%:
	@echo "[MAKE] clean $(DIR)$*"
	$(MAKE) -C $* clean

# Define main rules of the build system
build: $(DEPENDENCY-FILES) $(SUBDIR:%=build-%) $(BUILD-FILES)

clean: $(SUBDIR:%=clean-%)
	$(RM) $(CLEAN-FILES)
	$(RM) $(BUILD-FILES)
	$(RM) *~

FORMAT-RECURSE ?= $(SUBDIR:%=format-%)

format: $(FORMAT-RECURSE)
ifneq ($(FORMAT-FILES),)
	@echo "[FORMAT] $(FORMAT-FILES)"
	$(FORMAT) -i $(FORMAT-FILES)
endif

PHONY-TARGETS += all build clean format dummy

.PHONY: $(PHONY-TARGETS)
.PRECIOUS: $(PRECIOUS-FILES)

include $(TOPDIR)/build/tools.mk
