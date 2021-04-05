ifndef SOURCES
$(error SOURCES is not set)
endif

SOURCES_C = $(filter %.c,$(SOURCES))
SOURCES_ASM = $(filter %.S,$(SOURCES))
FORMAT-FILES = $(filter-out $(FORMAT-EXCLUDE),$(SOURCES_C) $(SOURCES_H))
OBJECTS += $(SOURCES_C:%.c=%.o) $(SOURCES_ASM:%.S=%.o)

DEPENDENCY-FILES += $(foreach f, $(SOURCES_C),\
		      $(dir $(f))$(patsubst %.c,.%.D,$(notdir $(f))))
DEPENDENCY-FILES += $(foreach f, $(SOURCES_ASM),\
		      $(dir $(f))$(patsubst %.S,.%.D,$(notdir $(f))))

$(DEPENDENCY-FILES): $(SOURCES_GEN)

.%.D: %.S
	@echo "[DEP] $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -MT $*.o -MM -MG $(realpath $<) -MF $@

.%.D: %.c
	@echo "[DEP] $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -MT $*.o -MM -MG $(realpath $<) -MF $@

%.o: %.c
	@echo "[CC] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CFLAGS.$*) $(CPPFLAGS) -c -o $@ $(realpath $<)

%.o: %.asm
	@echo "[VASM] $(DIR)$< -> $(DIR)$@"
	$(VASM) -Felf $(CPPFLAGS) $(VASMFLAGS) -o $@ $(realpath $<)

%.o: %.S
	@echo "[AS] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $(realpath $<)

%.S: %.c
	@echo "[CC] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CFLAGS.$*) $(CPPFLAGS) -S -o $@ $(realpath $<)

%.c: %.png
	@echo "[PNG2C] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(PNG2C) $(PNG2C.$(notdir $*)) $(realpath $<) > $@

%.c: %.psfu
	@echo "[PSF2C] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(PSF2C) $(PSF2C.$(notdir $*)) $(realpath $<) > $@

%.c: %.wav
	@echo "[WAV2C] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(WAV2C) $(WAV2C.$(notdir $*)) $(realpath $<) > $@

ifeq ($(words $(findstring $(MAKECMDGOALS), clean)), 0)
  -include $(DEPENDENCY-FILES)
endif

BUILD-FILES += $(OBJECTS)
CLEAN-FILES += $(DEPENDENCY-FILES) $(OBJECTS) $(SOURCES_GEN)
CLEAN-FILES += $(DEPFILES) $(OBJECTS)
CLEAN-FILES += $(addsuffix ~,$(SOURCES_C) $(SOURCES_H) $(SOURCES_ASM))
PRECIOUS-FILES += $(OBJECTS)
