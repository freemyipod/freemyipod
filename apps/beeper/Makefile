NAME := beeper
STACKSIZE := 4096
COMPRESS := true

EMCOREDIR ?= ../../emcore/trunk/

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
ELF2ECA := $(CROSS)elf2emcoreapp

LIBINCLUDES := 

CFLAGS  += -Os -fno-pie -fno-stack-protector -fomit-frame-pointer -I. -I$(EMCOREDIR)/export $(LIBINCLUDES) -ffunction-sections -fdata-sections -mcpu=arm940t -DARM_ARCH=4 -marm
LDFLAGS += "$(shell $(CC) -print-libgcc-file-name)" --emit-relocs --gc-sections

preprocess = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#")
preprocesspaths = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#" | sed -e "s:^..*:$(dir $(1))&:" | sed -e "s:^\\./::")

REVISION := $(shell svnversion .)
REVISIONINT := $(shell echo $(REVISION) | sed -e "s/[^0-9].*$$//")

HELPERS := build/__emcore_armhelpers.o

SRC := $(call preprocesspaths,SOURCES,-I. -I..)
OBJ := $(SRC:%.c=build/%.o)
OBJ := $(OBJ:%.S=build/%.o) $(HELPERS)

all: $(NAME)

-include $(OBJ:%=%.dep)

$(NAME): build/$(NAME).emcoreapp

build/$(NAME).emcoreapp: build/$(NAME).elf
	@echo [EMCAPP] $<
ifeq ($(COMPRESS),true)
	@$(ELF2ECA) -z -s $(STACKSIZE) -o $@ $^
else
	@$(ELF2ECA) -s $(STACKSIZE) -o $@ $^
endif

build/$(NAME).elf: ls.x $(OBJ)
	@echo [LD]     $@
	@$(LD) $(LDFLAGS) -o $@ -T ls.x $(OBJ)

build/%.o: %.c build/version.h
	@echo [CC]     $<
ifeq ($(shell uname),WindowsNT)
	@-if not exist $(subst /,\,$(dir $@)) md $(subst /,\,$(dir $@))
else
	@-mkdir -p $(dir $@)
endif
	@$(CC) -c $(CFLAGS) -o $@ $<
	@$(CC) -MM $(CFLAGS) $< > $@.dep.tmp
	@sed -e "s|.*:|$@:|" < $@.dep.tmp > $@.dep
ifeq ($(shell uname),WindowsNT)
	@sed -e "s/.*://" -e "s/\\$$//" < $@.dep.tmp | fmt -1 | sed -e "s/^ *//" -e "s/$$/:/" >> $@.dep
else
	@sed -e 's/.*://' -e 's/\\$$//' < $@.dep.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $@.dep
endif
	@rm -f $@.dep.tmp

build/%.o: %.S build/version.h
	@echo [CC]     $<
ifeq ($(shell uname),WindowsNT)
	@-if not exist $(subst /,\,$(dir $@)) md $(subst /,\,$(dir $@))
else
	@-mkdir -p $(dir $@)
endif
	@$(CC) -c $(CFLAGS) -o $@ $<
	@$(CC) -MM $(CFLAGS) $< > $@.dep.tmp
	@sed -e "s|.*:|$@:|" < $@.dep.tmp > $@.dep
ifeq ($(shell uname),WindowsNT)
	@sed -e "s/.*://" -e "s/\\$$//" < $@.dep.tmp | fmt -1 | sed -e "s/^ *//" -e "s/$$/:/" >> $@.dep
else
	@sed -e 's/.*://' -e 's/\\$$//' < $@.dep.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $@.dep
endif
	@rm -f $@.dep.tmp

build/__emcore_%.o: $(EMCOREDIR)/export/%.c
	@echo [CC]     $<
ifeq ($(shell uname),WindowsNT)
	@-if not exist $(subst /,\,$(dir $@)) md $(subst /,\,$(dir $@))
else
	@-mkdir -p $(dir $@)
endif
	@$(CC) -c $(CFLAGS) -o $@ $<

build/__emcore_%.o: $(EMCOREDIR)/export/%.S
	@echo [CC]     $<
ifeq ($(shell uname),WindowsNT)
	@-if not exist $(subst /,\,$(dir $@)) md $(subst /,\,$(dir $@))
else
	@-mkdir -p $(dir $@)
endif
	@$(CC) -c $(CFLAGS) -o $@ $<

build/version.h: version.h ../../.svn/entries
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

.PHONY: all clean $(NAME)
