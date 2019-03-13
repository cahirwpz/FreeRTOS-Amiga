# Common tools used by actions
RM = rm -v -f
FSUTIL = $(TOPDIR)/tools/fsutil.py

# Current directory without common prefix
DIR = $(patsubst $(TOPDIR)/%,%,$(CURDIR)/)

# Pass "VERBOSE=1" at command line to display command being invoked by GNU Make
ifneq ($(VERBOSE), 1)
.SILENT:
endif

# Disable all built-in recipes
.SUFFIXES:

# Define our own recipes
%.S: %.c
	@echo "[CC] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -S -o $@ $(realpath $<)

%.o: %.c
	@echo "[CC] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(realpath $<)

%.o: %.S
	@echo "[AS] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(realpath $<)

%.lib:
	@echo "[LD] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(LD) -r -o $@ $^

%.bin: %.S
	@echo "[AS] $(DIR)$< -> $(DIR)$@"
	$(AS) -Fbin $(CPPFLAGS) $(ASFLAGS) -o $@ $(realpath $<)

%.exe:
	@echo "[LD] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(LD) $(LDFLAGS) -gc-all -mtype -o $@ $^

%.adf: $(TOPDIR)/bootloader.bin
	@echo "[ADF] $(filter-out %bootloader.bin,$^) -> $(DIR)$@"
	$(FSUTIL) -b $(TOPDIR)/bootloader.bin create $@ \
		$(filter-out %bootloader.bin,$^)

# Generate recursive rules for subdirectories
define emit_subdir_rule
$(1)-$(2):
	@echo "[MAKE] $(2) $(DIR)$(1)"
	$(MAKE) -C $(1) $(2)
PHONY-TARGETS += $(1)-$(2)
endef

$(foreach dir,$(SUBDIR),$(eval $(call emit_subdir_rule,$(dir),build)))
$(foreach dir,$(SUBDIR),$(eval $(call emit_subdir_rule,$(dir),clean)))

build-recursive: $(SUBDIR:%=%-build)
clean-recursive: $(SUBDIR:%=%-clean)

# Define main rules of the build system
build: build-dependencies build-recursive $(BUILD-FILES) build-here

clean: clean-recursive clean-here
	$(RM) $(CLEAN-FILES)
	$(RM) $(BUILD-FILES)
	$(RM) *~

PHONY-TARGETS += all
PHONY-TARGETS += build build-dependencies build-here
PHONY-TARGETS += clean clean-here

.PHONY: $(PHONY-TARGETS)
.PRECIOUS: $(BUILD-FILES)
