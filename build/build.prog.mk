ifndef PROGRAM 
$(error PROGRAM is not set)
endif

LIBS    ?= $(TOPDIR)/drivers/drivers.lib \
	   $(TOPDIR)/FreeRTOS/freertos.lib \
	   $(TOPDIR)/FreeRTOS-Plus/freertos-plus.lib \
	   $(TOPDIR)/kernel/kernel.lib \
	   $(TOPDIR)/libc/c.lib
LDFLAGS  = --emit-relocs -T $(TOPDIR)/amiga.ld

BUILD-FILES += $(PROGRAM).exe $(PROGRAM).adf
CLEAN-FILES += $(PROGRAM).elf $(PROGRAM).elf.map $(PROGRAM).rom

all: build

include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/flags.mk
include $(TOPDIR)/build/common.mk

$(PROGRAM).elf: $(OBJECTS) $(LIBS)
	@echo "[LD] $(addprefix $(DIR),$(OBJECTS)) $(LIBS) -> $(DIR)$@"
	$(LD) $(LDFLAGS) -Map $@.map -o $@ $^

$(PROGRAM).adf: $(TOPDIR)/bootloader.bin $(PROGRAM).exe $(ADF-EXTRA)
	@echo "[ADF] $(addprefix $(DIR),$(filter-out %bootloader.bin,$^)) -> $(DIR)$@"
	$(FSUTIL) -b $(TOPDIR)/bootloader.bin create $@ \
		$(filter-out %bootloader.bin,$^)

%.exe: %.elf
	@echo "[ELF2HUNK] $(DIR)$< -> $(DIR)$@"
	$(ELF2HUNK) $< $@

%.o: %
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) -I binary -O elf32-m68k -B 68000 $^ $@

%.rom.asm: $(TOPDIR)/a500rom.asm
	@echo "[SED] $^ -> $(DIR)$@"
	sed -e 's,$$(TOPDIR),$(TOPDIR),g' \
	    -e 's,$$(PROGRAM),$(PROGRAM),g' \
	    $(TOPDIR)/a500rom.asm > $@

%.rom: %.rom.asm %.exe
	@echo "[AS] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(VASM) -Fbin $(VASMFLAGS) -o $@ $(realpath $<)

$(TOPDIR)/%.lib: dummy
	$(MAKE) -C $(dir $@) $(notdir $@)

$(TOPDIR)/%.bin: dummy
	$(MAKE) -C $(dir $@) $(notdir $@)

run-floppy: $(PROGRAM).elf $(PROGRAM).adf
	$(LAUNCH) $(LAUNCHOPTS) \
	  -f $(PROGRAM).adf -e $(PROGRAM).elf

run-rom: $(PROGRAM).rom $(PROGRAM).elf $(PROGRAM).adf
	$(LAUNCH) $(LAUNCHOPTS) \
	  -r $(PROGRAM).rom -e $(PROGRAM).elf -f $(PROGRAM).adf

debug-floppy: $(PROGRAM).elf $(PROGRAM).adf
	$(LAUNCH) $(LAUNCHOPTS) \
	  -d -f $(PROGRAM).adf -e $(PROGRAM).elf

debug-rom: $(PROGRAM).rom $(PROGRAM).elf $(PROGRAM).adf
	$(LAUNCH) $(LAUNCHOPTS) \
	  -d -r $(PROGRAM).rom -e $(PROGRAM).elf -f $(PROGRAM).adf

.PHONY: debug-floppy debug-rom run-floppy run-rom
