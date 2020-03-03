ifndef PROGRAM 
$(error PROGRAM is not set)
endif

LIBS    ?= $(TOPDIR)/drivers/drivers.lib \
	   $(TOPDIR)/FreeRTOS/freertos.lib \
	   $(TOPDIR)/libc/c.lib
LDFLAGS  = --emit-relocs -T $(TOPDIR)/amiga.ld

BUILD-FILES = $(PROGRAM).exe $(PROGRAM).adf
CLEAN-FILES = $(PROGRAM).elf $(PROGRAM).elf.map $(PROGRAM).rom

all: build

build-before: $(TOPDIR)/bootloader.bin

include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/flags.mk
include $(TOPDIR)/build/common.mk

$(PROGRAM).elf: $(OBJECTS) $(LIBS)
	@echo "[LD] $(addprefix $(DIR),$(OBJECTS)) $(LIBS) -> $(DIR)$@"
	$(LD) $(LDFLAGS) -Map $@.map -o $@ $^

$(PROGRAM).exe: $(PROGRAM).elf
	@echo "[ELF2HUNK] $(DIR)$< -> $(DIR)$@"
	$(ELF2HUNK) $< $@

$(PROGRAM).adf: $(TOPDIR)/bootloader.bin $(PROGRAM).exe
	@echo "[ADF] $(addprefix $(DIR),$(filter-out %bootloader.bin,$^)) -> $(DIR)$@"
	$(FSUTIL) -b $(TOPDIR)/bootloader.bin create $@ \
		$(filter-out %bootloader.bin,$^)

%.rom.S: $(TOPDIR)/a500rom.S
	@echo "[SED] $^ -> $(DIR)$@"
	sed -e 's,$$(TOPDIR),$(TOPDIR),g' \
	    -e 's,$$(PROGRAM),$(PROGRAM),g' \
	    $(TOPDIR)/a500rom.S > $@

%.rom: %.rom.S %.exe
	@echo "[AS] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(AS) -Fbin $(ASFLAGS) -o $@ $(realpath $<)

$(TOPDIR)/%.lib:
	$(MAKE) -C $(dir $@) $(notdir $@)

$(TOPDIR)/%.bin:
	$(MAKE) -C $(dir $@) $(notdir $@)

run-floppy: $(PROGRAM).elf $(PROGRAM).adf
	$(LAUNCH) -f $(PROGRAM).adf -e $(PROGRAM).elf

run-rom: $(PROGRAM).rom $(PROGRAM).elf $(PROGRAM).adf
	$(LAUNCH) -r $(PROGRAM).rom -e $(PROGRAM).elf -f $(PROGRAM).adf

debug-floppy: $(PROGRAM).elf $(PROGRAM).adf
	$(LAUNCH) -d -f $(PROGRAM).adf -e $(PROGRAM).elf

debug-rom: $(PROGRAM).rom $(PROGRAM).elf $(PROGRAM).adf
	$(LAUNCH) -d -r $(PROGRAM).rom -e $(PROGRAM).elf -f $(PROGRAM).adf

.PHONY: debug-floppy debug-rom run-floppy run-rom
