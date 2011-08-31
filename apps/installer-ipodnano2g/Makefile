NAME := installer-ipodnano2g
STACKSIZE := 4096
COMPRESS := false
AUTOBUILD_FLASHFILES ?= true
BASENAME ?= $(NAME)
FATNAME ?= INSTAL~1BOO

EMCOREDIR ?= ../../emcore/trunk/
UNINSTDIR ?= ../uninstaller-ipodnano2g/
BOOTMENUDIR ?= ../bootmenu-ipodnano2g/
LIBBOOTDIR ?= ../../libs/boot/
LIBPNGDIR ?= ../../libs/png/
LIBUIDIR ?= ../../libs/ui/
LIBMKFAT32DIR ?= ../../libs/mkfat32/
UMSBOOTDIR ?= ../../umsboot/
NOTEBOOTDIR ?= ../../noteboot/
TOOLSDIR ?= ../../tools/

FLASHFILES = flashfiles/boot.emcorelib flashfiles/png.emcorelib flashfiles/ui.emcorelib flashfiles/mkfat32.emcorelib \
             flashfiles/uninstaller-ipodnano2g.emcoreapp flashfiles/bootmenu-ipodnano2g.emcoreapp \
	     flashfiles/emcoreldr-ipodnano2g.dfu flashfiles/emcore-ipodnano2g.ucl flashfiles/umsboot-ipodnano2g.ucl

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
CRYPTFW := python $(EMCOREDIR)/tools/ipodcrypt.py s5l8701-cryptfirmware
GENNOTE := python $(NOTEBOOTDIR)/gennote.py
SCRAMBLE := python $(TOOLSDIR)/scramble.py

LIBINCLUDES := -I$(LIBBOOTDIR)/export -I$(LIBPNGDIR)/export -I$(LIBUIDIR)/export

CFLAGS  += -Os -fno-pie -fno-stack-protector -fomit-frame-pointer -I. -I$(EMCOREDIR)/export $(LIBINCLUDES) -ffunction-sections -fdata-sections -mcpu=arm940t -DARM_ARCH=4 -DBASENAME=$(BASENAME)
LDFLAGS += "$(shell $(CC) -print-libgcc-file-name)" --emit-relocs --gc-sections

preprocess = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#")
preprocesspaths = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#" | sed -e "s:^..*:$(dir $(1))&:" | sed -e "s:^\\./::")

REVISION := $(shell svnversion .)
REVISIONINT := $(shell echo $(REVISION) | sed -e "s/[^0-9].*$$//")

HELPERS := build/__emcore_armhelpers.o
LIBS := build/libucl.a

SRC := $(call preprocesspaths,SOURCES,-I. -I..)
OBJ := $(SRC:%.c=build/%.o)
OBJ := $(OBJ:%.S=build/%.o) $(HELPERS)

all: $(NAME)

-include $(OBJ:%=%.dep)

$(NAME): build/$(BASENAME).bootnote build/$(BASENAME).ipodx

build/$(BASENAME).ipodx: build/$(NAME).fw
	@echo [SCRAMB] $<
	@$(SCRAMBLE) $< $@ --signature=nn2x --targetid=62

build/$(NAME).fw: build/$(NAME).bin
	@echo [CFW]    $<
	@$(CRYPTFW) $< $@

build/$(BASENAME).bootnote: build/$(NAME).bin
	@echo [GENNOT] $<
	@$(GENNOTE) $< "$(FATNAME)" $@

build/$(NAME).bin: build/bootstub.bin build/$(NAME).ubi
	@echo [STUBEM] $@
	@$(STUBEMBED) $^ $@

build/$(NAME).ubi: $(EMCOREDIR)/build/ipodnano2g/emcore.bin build/$(NAME).emcoreapp
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

build/$(NAME).elf: ls.x $(OBJ) $(LIBS)
	@echo [LD]     $@
	@$(LD) $(LDFLAGS) -o $@ -T ls.x $(OBJ) $(LIBS)

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

build/libucl.a: libucl
	@$(MAKE) -C libucl CFLAGS="$(CFLAGS) -I../$(EMCOREDIR)/export"

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

$(UMSBOOTDIR)/build/ipodnano2g/umsboot-ipodnano2g.ucl $(UMSBOOTDIR)/build/ipodnano2g/umsboot.bin: umsboot
	@$(MAKE) -C $(UMSBOOTDIR) ipodnano2g

flashfiles/umsboot-ipodnano2g.ucl: $(UMSBOOTDIR)/build/ipodnano2g/umsboot-ipodnano2g.ucl
	@echo [CP]     $@
	@cp $< $@

$(UNINSTDIR)/build/uninstaller-ipodnano2g.emcoreapp: uninstaller-ipodnano2g
	@$(MAKE) -C $(UNINSTDIR)

flashfiles/uninstaller-ipodnano2g.emcoreapp: $(UNINSTDIR)/build/uninstaller-ipodnano2g.emcoreapp
	@echo [CP]     $@
	@cp $< $@

$(BOOTMENUDIR)/build/bootmenu-ipodnano2g.emcoreapp: bootmenu-ipodnano2g
	@$(MAKE) -C $(BOOTMENUDIR)

flashfiles/bootmenu-ipodnano2g.emcoreapp: $(BOOTMENUDIR)/build/bootmenu-ipodnano2g.emcoreapp
	@echo [CP]     $@
	@cp $< $@

$(EMCOREDIR)/loader/ipodnano2g/build/emcoreldr-ipodnano2g.dfu: emcoreldr-ipodnano2g
	@$(MAKE) -C $(EMCOREDIR)/loader/ipodnano2g

flashfiles/emcoreldr-ipodnano2g.dfu: $(EMCOREDIR)/loader/ipodnano2g/build/emcoreldr-ipodnano2g.dfu
	@echo [CP]     $@
	@cp $< $@

flashfiles/emcore-ipodnano2g.ucl: flashfiles/emcore-ipodnano2g.bin
	@echo [UCL]    $<
	@$(UCLPACK) $< $@

$(EMCOREDIR)/build/ipodnano2g/emcore.bin: emcore
	@$(MAKE) -C $(EMCOREDIR) ipodnano2g

flashfiles/emcore-ipodnano2g.bin: $(EMCOREDIR)/build/ipodnano2g/emcore.bin
	@echo [EMBCFG] $@
	@$(EMCOREBOOTCFG) $< $@ "(3, '/.boot/init.emcoreapp', None, (2, 'bootmenu', None, None))"

clean:
	@rm -rf build

.PHONY: all clean emcore emcoreldr-ipodnano2g bootmenu-ipodnano2g uninstaller-ipodnano2g libboot libpng libui libmkfat32 umsboot libucl flashfiles $(NAME)
