CCACHE  ?= $(shell which ccache)

CROSS   ?= arm-none-eabi-
ifeq ($(shell which $(CROSS)gcc),)
CROSS   := arm-elf-eabi-
endif

FLTO    ?= -flto -flto-partition=none
ifeq ($(shell $(CROSS)gcc -v --help 2>/dev/null | grep -- -flto),)
FLTO    :=
endif

CC      := $(CCACHE) $(CROSS)gcc
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump

CFLAGS_GENERAL  := -c -ffunction-sections -fdata-sections -fmessage-length=0 -Wall -I=include $(CFLAGS_GENERAL)
CFLAGS_ASM      := -x assembler-with-cpp $(CFLAGS_ASM)
CFLAGS_debug    := -O0 -g3 -gdwarf-2 $(CFLAGS_DEBUG)
CFLAGS_release  := $(FLTO) -Os -fno-pie -fno-stack-protector -fomit-frame-pointer $(CFLAGS_RELEASE)
LDFLAGS_GENERAL := -nostdlib --gc-sections $(LDFLAGS_GENERAL)
LDFLAGS_debug   := -O0 $(LDFLAGS_DEBUG)
LDFLAGS_release := $(FLTO) -Os $(LDFLAGS_RELEASE)
PPFLAGS_GENERAL := -DTARGET_$(subst /,_,$(TARGET)) -DTARGET=$(TARGET) -DCONFIG_H=target/$(TARGET)/config.h -DTARGET_H=target/$(TARGET)/target.h -Isrc
PPFLAGS_debug   := -DDEBUG
PPFLAGS_release := -DRELEASE
PPONLY_FLAGS    := -E -P -x c -DASM_FILE
SPEC_FLAGS      := -E -P -v -dD

ifeq ($(VERY_VERBOSE),)
VQ := @
else
VERBOSE := 1
endif

ifeq ($(VERBOSE),)
Q := @
endif

CWD := $(shell pwd)
relpath = $(patsubst $(CWD)/%,%,$(realpath $(1)))
preprocess = $(shell $(CC) $(2) $(PPONLY_FLAGS) $(1) | grep -v "^\#")
preprocesspaths = $(shell $(CC) $(2) $(PPONLY_FLAGS) $(1) | grep -v "^\#" | sed -e "s:^..*:$(dir $(1))&:" | sed -e "s:^\\./::")

TARGETS := $(call preprocess,TARGETS,-I.)

ifeq ($(TARGET),)
all: $(TARGETS)
define TARGET_template
$(1):
	$(Q)+$(MAKE) TARGET=$(1)
endef
$(foreach target,$(TARGETS),$(eval $(call TARGET_template,$(target))))
else

ifneq ($(TYPE),debug)
ifneq ($(TYPE),release)
$(error Please define either TYPE=debug or TYPE=release)
endif
endif

ifeq ($(LISTING),)
all: $(TARGET)
else
all: $(TARGET) LISTINGS
endif

_PPFLAGS := $(PPFLAGS_GENERAL) $(PPFLAGS_$(TYPE)) $(PPFLAGS)

define MODULE_template
ifeq ($(1),)
$$(shell echo Cannot resolve module $(2) 1>&2)
else
ifeq ($$(filter $(1),$$(INCLUDED_MODULES)),)
INCLUDED_MODULES += $(1)
ifneq ($(VERBOSE),)
$$(shell echo Including module $(1) 1>&2)
endif
-include $(1)/target.mk
ifneq ($(wildcard $(1)/SOURCES),)
SOURCES += $(1)/SOURCES
SRC += $$(call preprocesspaths,$(1)/SOURCES,$(_PPFLAGS))
endif
ifneq ($(wildcard $(1)/DEPS),)
DEPS += $(1)/DEPS
$$(foreach module,$$(strip $$(call preprocesspaths,$(1)/DEPS,$(_PPFLAGS))),$$(eval $$(call MODULE_template,$$(call relpath,$$(module)),$$(module))))
endif
else
ifneq ($(VERY_VERBOSE),)
$$(shell echo Module $(1) is already included 1>&2)
endif
endif
endif
endef

$(eval $(call MODULE_template,src,src))

SRC := $(foreach file,$(SRC),$(call relpath,$(file)))
OBJ := $(SRC:src/%.c=build/$(TARGET)/$(TYPE)/%.o)
OBJ := $(OBJ:src/%.S=build/$(TARGET)/$(TYPE)/%.o)
_LDSCRIPT = $(LDSCRIPT:src/%.lds=build/$(TARGET)/$(TYPE)/%.lds)

_ASMFLAGS := $(CFLAGS_GENERAL) $(CFLAGS_ASM) $(CFLAGS_$(TYPE)) $(_CFLAGS) $(_PPFLAGS) $(CFLAGS) $(ASMFLAGS)
_CFLAGS := $(CFLAGS_GENERAL) $(CFLAGS_$(TYPE)) $(_CFLAGS) $(_PPFLAGS) $(CFLAGS)
_LDFLAGS := $(LDFLAGS_GENERAL) $(LDFLAGS_$(TYPE)) $(_LDFLAGS) $(LDFLAGS)

define CCRULE_template
build/$(TARGET)/$(TYPE)/%.$(1): src/%.$(2)
	$(VQ)echo $(3) $$<
	$(VQ)-mkdir -p $$(dir $$@)
	$(Q)$(CC) -Wa,-adhlns="build/$(TARGET)/$(TYPE)/$$*.lst" $(4) -o $$@ $$<
	$(VQ)$(CC) -MM $(4) $$< > $$@.dep.tmp
	$(VQ)sed -e "s|.*:|$$@:|" < $$@.dep.tmp > $$@.dep
	$(VQ)sed -e 's/.*://' -e 's/\\$$$$//' < $$@.dep.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$$$/:/' >> $$@.dep
	$(VQ)rm -f $$@.dep.tmp
endef

-include $(OBJ:%=%.dep)

build/$(TARGET)/$(TYPE)/%.bin: build/$(TARGET)/$(TYPE)/%.elf
	$(VQ)echo "[OC]    " $<
	$(Q)$(OBJCOPY) -O binary $^ $@

build/$(TARGET)/$(TYPE)/$(NAME).elf: $(_LDSCRIPT) $(OBJ) $(SOURCES) $(DEPS)
	$(VQ)echo "[LD]    " $@
	$(Q)$(LD) -Map "$@.map" $(_LDFLAGS) -o $@ -T $(_LDSCRIPT) $(OBJ)

%.lst: %
	$(VQ)echo "[OD]    " $<
	$(Q)$(OBJDUMP) -S $< > $@
	
LISTINGS: $(OBJ:%=%.lst)
	
$(eval $(call CCRULE_template,lds,lds,"[PP]    ",$(_PPFLAGS) $(PPONLY_FLAGS)))

$(eval $(call CCRULE_template,o,c,"[CC]    ",$(_CFLAGS)))

$(eval $(call CCRULE_template,o,S,"[CC]    ",$(_CFLAGS)))

endif

spec:
	$(Q)$(CC) $(_CFLAGS) $(SPEC_FLAGS) src/spec.c

clean:
	$(Q)rm -rf build

.PHONY: all spec clean $(TARGETS) SOURCES DEPS LISTINGS
