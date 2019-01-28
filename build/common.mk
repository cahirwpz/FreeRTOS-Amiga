# Common tools used by actions
RM = rm -v -f

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

# Define main rules of the build system
build: build-dependencies $(BUILD-FILES) build-here

clean: clean-here
	$(RM) $(CLEAN-FILES)
	$(RM) $(BUILD-FILES)
	$(RM) *~

PHONY-TARGETS += all
PHONY-TARGETS += build build-dependencies build-here
PHONY-TARGETS += clean clean-here

.PHONY: $(PHONY-TARGETS)
.PRECIOUS: $(BUILD-FILES)
