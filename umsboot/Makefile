NAME := umsboot

ifeq ($(shell uname),WindowsNT)
CCACHE :=
else
CCACHE := $(shell which ccache)
endif

CROSS   ?= arm-elf-eabi-
CC      := $(CCACHE) $(CROSS)gcc
AS      := $(CROSS)as
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
UCLPACK := ucl2e10singleblk

CFLAGS  += -Os -fno-pie -fno-stack-protector -fomit-frame-pointer -I. -Iexport -ffunction-sections -fdata-sections -marm
LDFLAGS += "$(shell $(CC) -print-libgcc-file-name)" --gc-sections

preprocess = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#")
preprocesspaths = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#" | sed -e "s:^..*:$(dir $(1))&:" | sed -e "s:^\\./::")

REVISION := $(shell svnversion .)
REVISIONINT := $(shell echo $(REVISION) | sed -e "s/[^0-9].*$$//")

TARGETS := $(call preprocess,TARGETS,-I.)

define TARGET_template
-include target/$(1)/target.mk

SRC_$(1) := $$(call preprocesspaths,SOURCES,-DTARGET_$(1) -Itarget/$(1) -I.)
OBJ_$(1) := $$(SRC_$(1):%.c=build/$(1)/%.o)
OBJ_$(1) := $$(OBJ_$(1):%.S=build/$(1)/%.o)

-include $$(OBJ_$(1):%=%.dep)

$(1): build/$(1)/$(NAME)-$(1).ucl

build/$(1)/$(NAME)-$(1).ucl: build/$(1)/$(NAME).bin
	@echo [UCL]    $$<
	@$(UCLPACK) $$^ $$@

build/$(1)/$(NAME).bin: build/$(1)/$(NAME).elf
	@echo [OC]     $$<
	@$(OBJCOPY) -O binary $$^ $$@

build/$(1)/$(NAME).elf: target/$(1)/ls.x build/$(1)/target/$(1)/crt0.o $$(OBJ_$(1))
	@echo [LD]     $$@
	@$(LD) $(LDFLAGS) $$(LDFLAGS_$(1)) -o $$@ -T target/$(1)/ls.x $$(OBJ_$(1))

build/$(1)/%.o: %.c build/version.h
	@echo [CC]     $$<
ifeq ($(shell uname),WindowsNT)
	@-if not exist $$(subst /,\,$$(dir $$@)) md $$(subst /,\,$$(dir $$@))
else
	@-mkdir -p $$(dir $$@)
endif
	@$(CC) -c $(CFLAGS) $$(CFLAGS_$(1)) -DTARGET_$(1) -DTARGET=\"$(1)\" -DCONFIG_H=\"target/$(1)/config.h\" -DTARGET_H=\"target/$(1)/target.h\" -o $$@ $$<
	@$(CC) -MM $(CFLAGS) $$(CFLAGS_$(1)) -DTARGET_$(1) -DTARGET=\"$(1)\" -DCONFIG_H=\"target/$(1)/config.h\" -DTARGET_H=\"target/$(1)/target.h\" $$< > $$@.dep.tmp
	@sed -e "s|.*:|$$@:|" < $$@.dep.tmp > $$@.dep
ifeq ($(shell uname),WindowsNT)
	@sed -e "s/.*://" -e "s/\\$$$$//" < $$@.dep.tmp | fmt -1 | sed -e "s/^ *//" -e "s/$$$$/:/" >> $$@.dep
else
	@sed -e 's/.*://' -e 's/\\$$$$//' < $$@.dep.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$$$/:/' >> $$@.dep
endif
	@rm -f $$@.dep.tmp

build/$(1)/%.o: %.S build/version.h
	@echo [CC]     $$<
ifeq ($(shell uname),WindowsNT)
	@-if not exist $$(subst /,\,$$(dir $$@)) md $$(subst /,\,$$(dir $$@))
else
	@-mkdir -p $$(dir $$@)
endif
	@$(CC) -c $(CFLAGS) $$(CFLAGS_$(1)) -DTARGET_$(1) -DTARGET=\"$(1)\" -DCONFIG_H=\"target/$(1)/config.h\" -DTARGET_H=\"target/$(1)/target.h\" -o $$@ $$<
	@$(CC) -MM $(CFLAGS) $$(CFLAGS_$(1)) -DTARGET_$(1) -DTARGET=\"$(1)\" -DCONFIG_H=\"target/$(1)/config.h\" -DTARGET_H=\"target/$(1)/target.h\" $$< > $$@.dep.tmp
	@sed -e "s|.*:|$$@:|" < $$@.dep.tmp > $$@.dep
ifeq ($(shell uname),WindowsNT)
	@sed -e "s/.*://" -e "s/\\$$$$//" < $$@.dep.tmp | fmt -1 | sed -e "s/^ *//" -e "s/$$$$/:/" >> $$@.dep
else
	@sed -e 's/.*://' -e 's/\\$$$$//' < $$@.dep.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$$$/:/' >> $$@.dep
endif
	@rm -f $$@.dep.tmp
endef

all: $(TARGETS)

$(foreach target,$(TARGETS),$(eval $(call TARGET_template,$(target))))

build/version.h: version.h ../.svn/entries
	@echo [PP]     $<
ifeq ($(shell uname),WindowsNT)
	@-if not exist build md build
	@sed -e "s/\$$REVISION\$$/$(REVISION)/" -e "s/\$$REVISIONINT\$$/$(REVISIONINT)/" < $< > $@
else
	@-mkdir -p build
	@sed -e 's/\$$REVISION\$$/$(REVISION)/' -e 's/\$$REVISIONINT\$$/$(REVISIONINT)/' < $< > $@
endif

clean:
	@rm -rf build

.PHONY: all clean $(TARGETS)
