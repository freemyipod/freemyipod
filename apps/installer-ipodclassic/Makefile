NAME := installer-ipodclassic
STACKSIZE := 4096
COMPRESS := false

EMCOREDIR ?= ../../emcore/trunk/
BOOTMENUDIR ?= ../bootmenu-ipodclassic/
LIBBOOTDIR ?= ../../libs/boot/
LIBPNGDIR ?= ../../libs/png/
LIBUIDIR ?= ../../libs/ui/
LIBMKFAT32DIR ?= ../../libs/mkfat32/
UMSBOOTDIR ?= ../../umsboot/
TOOLSDIR ?= ../../tools/

FLASHFILES = flashfiles/boot.emcorelib flashfiles/png.emcorelib flashfiles/ui.emcorelib flashfiles/mkfat32.emcorelib \
             flashfiles/bootmenu-ipodclassic.emcoreapp flashfiles/background.png flashfiles/icons.png flashfiles/rockbox.png \
             flashfiles/emcoreldr-ipodclassic.bin flashfiles/emcore-ipodclassic.ucl flashfiles/umsboot-ipodclassic.ucl

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
GENPWN := python $(EMCOREDIR)/tools/ipodcrypt.py s5l8702-genpwnage

LIBINCLUDES := -I$(LIBPNGDIR)/export -I$(LIBUIDIR)/export -I$(LIBMKFAT32DIR)/export

CFLAGS  += -Os -fno-pie -fno-stack-protector -fomit-frame-pointer -I. -I$(EMCOREDIR)/export $(LIBINCLUDES) -ffunction-sections -fdata-sections -mcpu=arm940t -DARM_ARCH=4
LDFLAGS += "$(shell $(CC) -print-libgcc-file-name)" --emit-relocs --gc-sections

preprocess = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#")
preprocesspaths = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#" | sed -e "s:^..*:$(dir $(1))&:")

REVISION := $(shell svnversion .)
REVISIONINT := $(shell echo $(REVISION) | sed -e "s/[^0-9].*$$//")

HELPERS := build/__emcore_armhelpers.o

SRC := $(call preprocesspaths,SOURCES,-I. -I..)
OBJ := $(SRC:%.c=build/%.o)
OBJ := $(OBJ:%.S=build/%.o) $(HELPERS)

all: $(NAME)

-include $(OBJ:%=%.dep)

$(NAME): build/bootstrap-ipodclassic.dfu build/$(NAME).ubi

build/bootstrap-ipodclassic.dfu: build/bootstrap.bin
	@echo [GENPWN] $<
	@$(GENPWN) $< $@

build/bootstrap.bin: build/bootstub.bin $(UMSBOOTDIR)/build/ipodclassic/umsboot.bin
	@echo [STUBEM] $@
	@$(STUBEMBED) $^ $@

build/$(NAME).ubi: $(EMCOREDIR)/build/ipodclassic/emcore.bin build/$(NAME).emcoreapp
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

build/resources.o: flashfiles.built

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

build/version.h: version.h .svn/entries
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

flashfiles/boot.emcorelib: $(LIBBOOTDIR)/build/boot.emcorelib libboot
	@echo [CP]     $@
	@cp $< $@

$(LIBPNGDIR)/build/png.emcorelib: libpng

flashfiles/png.emcorelib: $(LIBPNGDIR)/build/png.emcorelib libpng
	@echo [CP]     $@
	@cp $< $@

$(LIBUIDIR)/build/ui.emcorelib: libui

flashfiles/ui.emcorelib: $(LIBUIDIR)/build/ui.emcorelib libui
	@echo [CP]     $@
	@cp $< $@

$(LIBUIDIR)/build/mkfat32.emcorelib: libmkfat32

flashfiles/mkfat32.emcorelib: $(LIBMKFAT32DIR)/build/mkfat32.emcorelib libmkfat32
	@echo [CP]     $@
	@cp $< $@

$(UMSBOOTDIR)/build/ipodclassic/umsboot-ipodclassic.ucl: umsboot

flashfiles/umsboot-ipodclassic.ucl: $(UMSBOOTDIR)/build/ipodclassic/umsboot-ipodclassic.ucl umsboot
	@echo [CP]     $@
	@cp $< $@

$(BOOTMENUDIR)/build/bootmenu-ipodclassic.emcoreapp: bootmenu-ipodclassic

flashfiles/bootmenu-ipodclassic.emcoreapp: $(BOOTMENUDIR)/build/bootmenu-ipodclassic.emcoreapp bootmenu-ipodclassic
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

$(EMCOREDIR)/loader/ipodclassic/build/emcoreldr-ipodclassic.bin: emcoreldr-ipodclassic

flashfiles/emcoreldr-ipodclassic.bin: $(EMCOREDIR)/loader/ipodclassic/build/emcoreldr-ipodclassic.bin emcoreldr-ipodclassic
	@echo [CP]     $@
	@cp $< $@

flashfiles/emcore-ipodclassic.ucl: flashfiles/emcore-ipodclassic.bin
	@echo [UCL]    $<
	@$(UCLPACK) $< $@

$(EMCOREDIR)/build/ipodclassic/emcore.bin: emcore

flashfiles/emcore-ipodclassic.bin: $(EMCOREDIR)/build/ipodclassic/emcore.bin emcore
	@echo [EMBCFG] $@
	@$(EMCOREBOOTCFG) $< $@ "(3, '/.boot/init.emcoreapp', None, (2, 'bootmenu', None, None))"

emcore:
	@make -C $(EMCOREDIR) ipodclassic

emcoreldr-ipodclassic:
	@make -C $(EMCOREDIR)/loader/ipodclassic

bootmenu-ipodclassic:
	@make -C $(BOOTMENUDIR)

libboot:
	@make -C $(LIBBOOTDIR)

libpng:
	@make -C $(LIBPNGDIR)

libui:
	@make -C $(LIBUIDIR)

libmkfat32:
	@make -C $(LIBMKFAT32DIR)

$(UMSBOOTDIR)/build/ipodclassic/umsboot.bin: umsboot

umsboot:
	@make -C $(UMSBOOTDIR) ipodclassic

clean:
	@rm -rf build

.PHONY: all clean emcore emcoreldr-ipodclassic bootmenu-ipodclassic libboot libpng libui libmkfat32 umsboot libucl flashfiles $(NAME)
