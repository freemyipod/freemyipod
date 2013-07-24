NAME := installer-ipodnano4g
STACKSIZE := 4096
COMPRESS := false
AUTOBUILD_FLASHFILES ?= true

EMCOREDIR ?= ../../emcore/trunk/
BOOTMENUDIR ?= ../bootmenu-ipodnano4g/
LIBBOOTDIR ?= ../../libs/boot/
LIBPNGDIR ?= ../../libs/png/
LIBUIDIR ?= ../../libs/ui/
LIBMKFAT32DIR ?= ../../libs/mkfat32/
UMSBOOTDIR ?= ../../umsboot/
TOOLSDIR ?= ../../tools/

FLASHFILES = flashfiles/boot.emcorelib flashfiles/png.emcorelib flashfiles/ui.emcorelib flashfiles/mkfat32.emcorelib \
             flashfiles/bootmenu-ipodnano4g.emcoreapp flashfiles/background.png flashfiles/icons.png flashfiles/rockbox.png \
             flashfiles/emcoreldr-ipodnano4g.bin flashfiles/emcore-ipodnano4g.ucl flashfiles/umsboot-ipodnano4g.ucl

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
UCLPACK := ucl2e10singleblk
STUBEMBED := python tools/stubembed.py
EMCOREBOOTCFG := python $(EMCOREDIR)/tools/emcorebootcfg.py
EMCOREEMBEDAPP := python $(EMCOREDIR)/tools/emcoreembedapp.py
GENPWN := python $(EMCOREDIR)/tools/ipodcrypt.py s5l8720-genpwnage

LIBINCLUDES := -I$(LIBPNGDIR)/export -I$(LIBUIDIR)/export -I$(LIBMKFAT32DIR)/export

CFLAGS  += -Os -fno-pie -fno-stack-protector -fomit-frame-pointer -I. -I$(EMCOREDIR)/export $(LIBINCLUDES) -ffunction-sections -fdata-sections -mcpu=arm1176jz-s -DARM_ARCH=6 -marm
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

$(NAME): build/bootstrap-ipodnano4g.dfu build/$(NAME).ubi

build/bootstrap-ipodnano4g.dfu: build/bootstrap.bin
	@echo [GENPWN] $<
	@$(GENPWN) $< $@

build/bootstrap.bin: build/bootstub.bin $(UMSBOOTDIR)/build/ipodnano4g/umsboot.bin
	@echo [STUBEM] $@
	@$(STUBEMBED) $^ $@

build/$(NAME).ubi: $(EMCOREDIR)/build/ipodnano4g/emcore.bin build/$(NAME).emcoreapp
	@echo [EMBAPP] $@
	@$(EMCOREEMBEDAPP) $^ $@

build/bootstub.bin: build/bootstub.elf
	@echo [OC]     $<
	@$(OBJCOPY) -O binary $^ $@

build/bootstub.elf: bootstub/ls.x build/bootstub/bootstub.o
	@echo [LD]     $@
	@$(LD) $(LDFLAGS) -o $@ -T bootstub/ls.x build/bootstub/bootstub.o

build/$(NAME).emcoreapp: build/$(NAME).elf
	@echo [EMCAPP] $<
ifeq ($(COMPRESS),true)
	@$(ELF2ECA) -z -s $(STACKSIZE) -o $@ $^
else
	@$(ELF2ECA) -s $(STACKSIZE) -o $@ $^
endif

ifeq ($(AUTOBUILD_FLASHFILES),true)
build/resources.o: $(FLASHFILES)
else
build/resources.o: flashfiles.built
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

flashfiles: $(FLASHFILES)
	@touch flashfiles.built

$(LIBBOOTDIR)/build/boot.emcorelib: libboot
	@$(MAKE) -C $(LIBBOOTDIR)

flashfiles/boot.emcorelib: $(LIBBOOTDIR)/build/boot.emcorelib
	@echo [CP]     $@
	@cp $< $@

$(LIBPNGDIR)/build/png.emcorelib: libpng
	@$(MAKE) -C $(LIBPNGDIR)

flashfiles/png.emcorelib: $(LIBPNGDIR)/build/png.emcorelib
	@echo [CP]     $@
	@cp $< $@

$(LIBUIDIR)/build/ui.emcorelib: libui
	@$(MAKE) -C $(LIBUIDIR)

flashfiles/ui.emcorelib: $(LIBUIDIR)/build/ui.emcorelib
	@echo [CP]     $@
	@cp $< $@

$(LIBMKFAT32DIR)/build/mkfat32.emcorelib: libmkfat32
	@$(MAKE) -C $(LIBMKFAT32DIR)

flashfiles/mkfat32.emcorelib: $(LIBMKFAT32DIR)/build/mkfat32.emcorelib
	@echo [CP]     $@
	@cp $< $@

$(UMSBOOTDIR)/build/ipodnano4g/umsboot-ipodnano4g.ucl $(UMSBOOTDIR)/build/ipodnano4g/umsboot.bin: umsboot
	@$(MAKE) -C $(UMSBOOTDIR) ipodnano4g

flashfiles/umsboot-ipodnano4g.ucl: $(UMSBOOTDIR)/build/ipodnano4g/umsboot-ipodnano4g.ucl
	@echo [CP]     $@
	@cp $< $@

$(BOOTMENUDIR)/build/bootmenu-ipodnano4g.emcoreapp: bootmenu-ipodnano4g
	@$(MAKE) -C $(BOOTMENUDIR)

flashfiles/bootmenu-ipodnano4g.emcoreapp: $(BOOTMENUDIR)/build/bootmenu-ipodnano4g.emcoreapp
	@echo [CP]     $@
	@cp $< $@

flashfiles/background.png: $(BOOTMENUDIR)/images/background.png
	@echo [CP]     $@
	@cp $< $@

flashfiles/icons.png: $(BOOTMENUDIR)/images/icons.png
	@echo [CP]     $@
	@cp $< $@

flashfiles/rockbox.png: $(BOOTMENUDIR)/images/rockbox.png
	@echo [CP]     $@
	@cp $< $@

$(EMCOREDIR)/loader/ipodnano4g/build/emcoreldr-ipodnano4g.bin: emcoreldr-ipodnano4g
	@$(MAKE) -C $(EMCOREDIR)/loader/ipodnano4g

flashfiles/emcoreldr-ipodnano4g.bin: $(EMCOREDIR)/loader/ipodnano4g/build/emcoreldr-ipodnano4g.bin
	@echo [CP]     $@
	@cp $< $@

flashfiles/emcore-ipodnano4g.ucl: flashfiles/emcore-ipodnano4g.bin
	@echo [UCL]    $<
	@$(UCLPACK) $< $@

$(EMCOREDIR)/build/ipodnano4g/emcore.bin: emcore
	@$(MAKE) -C $(EMCOREDIR) ipodnano4g

flashfiles/emcore-ipodnano4g.bin: $(EMCOREDIR)/build/ipodnano4g/emcore.bin
	@echo [EMBCFG] $@
	@$(EMCOREBOOTCFG) $< $@ "(3, '/.boot/init.emcoreapp', None, (2, 'bootmenu', None, None))"

clean:
	@rm -rf build

.PHONY: all clean emcore emcoreldr-ipodnano4g bootmenu-ipodnano4g libboot libpng libui libmkfat32 umsboot libucl flashfiles $(NAME)
