ifndef SOURCES
$(error SOURCES is not set)
endif

SOURCES_C = $(filter %.c,$(SOURCES))
SOURCES_ASM = $(filter %.S,$(SOURCES))
FORMAT-FILES = $(filter-out $(FORMAT-EXCLUDE),$(SOURCES_C) $(SOURCES_H))
OBJECTS += $(SOURCES_C:%.c=%.o) $(SOURCES_ASM:%.S=%.o)

DEPENDENCY-FILES = $(foreach f, $(SOURCES_C),\
		     $(dir $(f))$(patsubst %.c,.%.D,$(notdir $(f))))

.%.D: %.c
	@echo "[DEP] $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -MT $*.o -MM -MG $^ -MF $@

%.o: %.c
	@echo "[CC] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(realpath $<)

%.o: %.S
	@echo "[AS] $(DIR)$< -> $(DIR)$@"
	$(AS) -Felf $(CPPFLAGS) $(ASFLAGS) -o $@ $(realpath $<)

%.S: %.c
	@echo "[CC] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -S -o $@ $(realpath $<)

ifeq ($(words $(findstring $(MAKECMDGOALS), clean)), 0)
  -include $(DEPENDENCY-FILES)
endif

BUILD-FILES += $(OBJECTS)
CLEAN-FILES += $(DEPENDENCY-FILES) $(OBJECTS)
CLEAN-FILES += $(DEPFILES) $(OBJECTS)
CLEAN-FILES += $(patsubst %,%~,$(SOURCES_C) $(SOURCES_ASM))
PRECIOUS-FILES += $(OBJECTS)
