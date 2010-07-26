NAME := embios
TARGETS := ipodnano2g

CROSS   := arm-none-eabi-
CC      := $(CROSS)gcc
AS      := $(CROSS)as
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
UCLPACK := ucl2e10singleblk

CFLAGS  ?= -Os -fno-pie -fno-stack-protector -fomit-frame-pointer -I. -ffunction-sections -fdata-sections
LDFLAGS ?= "$(shell $(CC) -print-libgcc-file-name)" --gc-sections

preprocess = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#" | sed -e "s:^..*:$(dir $(1))&:")

define TARGET_template
SRC_$(1) := $$(call preprocess,SOURCES,-D TARGET=$(1))
OBJ_$(1) := $$(SRC_$(1):%.c=build/$(1)/%.o)
OBJ_$(1) := $$(OBJ_$(1):%.S=build/$(1)/%.o)

-include $$(OBJ_$(1):%=%.dep)

$(1): build/$(1)/$(NAME)-$(1).ucl

build/$(1)/$(NAME)-$(1).ucl: build/$(1)/$(NAME).bin
	$(UCLPACK) $$^ $$@

build/$(1)/$(NAME).bin: build/$(1)/$(NAME).elf
	$(OBJCOPY) -O binary $$^ $$@

build/$(1)/$(NAME).elf: target/$(1)/ls.x build/$(1)/target/$(1)/crt0.o $$(OBJ_$(1))
	$(LD) $(LDFLAGS) -o $$@ -T target/$(1)/ls.x $$(OBJ_$(1))

build/$(1)/%.o: %.c
ifeq ($(shell uname),WindowsNT)
	@-if not exist $$(subst /,\,$$(dir $$@)) md $$(subst /,\,$$(dir $$@))
else
	@-mkdir -p $$(dir $$@)
endif
	$(CC) -c $(CFLAGS) -Itarget/$(1) -D TARGET=$(1) -o $$@ $$<
	@$(CC) -MM $(CFLAGS) -Itarget/$(1) -D TARGET=$(1) $$< > $$@.dep
	@mv -f $$@.dep $$@.dep.tmp
	@sed -e "s|.*:|$$@:|" < $$@.dep.tmp > $$@.dep
	@sed -e "s/.*://" -e "s/\\$$$$//" < $$@.dep.tmp | fmt -1 | sed -e "s/^ *//" -e "s/$$$$/:/" >> $$@.dep
	@rm -f $$@.dep.tmp

build/$(1)/%.o: %.S
ifeq ($(shell uname),WindowsNT)
	@-if not exist $$(subst /,\,$$(dir $$@)) md $$(subst /,\,$$(dir $$@))
else
	@-mkdir -p $$(dir $$@)
endif
	$(CC) -c $(CFLAGS) -Itarget/$(1) -D TARGET=$(1) -o $$@ $$<
	@$(CC) -MM $(CFLAGS) -Itarget/$(1) -D TARGET=$(1) $$< > $$@.dep
	@mv -f $$@.dep $$@.dep.tmp
	@sed -e "s|.*:|$$@:|" < $$@.dep.tmp > $$@.dep
	@sed -e "s/.*://" -e "s/\\$$$$//" < $$@.dep.tmp | fmt -1 | sed -e "s/^ *//" -e "s/$$$$/:/" >> $$@.dep
	@rm -f $$@.dep.tmp
endef

all: $(TARGETS)

$(foreach target,$(TARGETS),$(eval $(call TARGET_template,$(target))))

clean:
	rm -rf build

.PHONY: all clean $(TARGETS)
