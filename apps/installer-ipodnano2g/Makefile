NAME := installer-ipodnano2g
STACKSIZE := 4096
COMPRESS := false
BASENAME ?= $(NAME)
FATNAME ?= INSTAL~1BOO

EMCOREDIR ?= ../../emcore/trunk/
BOOTMENUDIR ?= ../bootmenu-ipodnano2g/
LIBBOOTDIR ?= ../../libs/boot/
LIBPNGDIR ?= ../../libs/png/
LIBUIDIR ?= ../../libs/ui/
UMSBOOTDIR ?= ../../umsboot/
NOTEBOOTDIR ?= ../../noteboot/
TOOLSDIR ?= ../../tools/

FLASHFILES = flashfiles/boot.emcorelib flashfiles/png.emcorelib flashfiles/ui.emcorelib flashfiles/crapple.png \
             flashfiles/bootmenu-ipodnano2g.emcoreapp flashfiles/background.png flashfiles/icons.png flashfiles/rockbox.png \
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
preprocesspaths = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#" | sed -e "s:^..*:$(dir $(1))&:")

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

build/resources.o: flashfiles.built

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

libucl:
	@make -C libucl CFLAGS="$(CFLAGS) -I../$(EMCOREDIR)/export"

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

$(UMSBOOTDIR)/build/ipodnano2g/umsboot-ipodnano2g.ucl: umsboot

flashfiles/umsboot-ipodnano2g.ucl: $(UMSBOOTDIR)/build/ipodnano2g/umsboot-ipodnano2g.ucl umsboot
	@echo [CP]     $@
	@cp $< $@

$(BOOTMENUDIR)/build/bootmenu-ipodnano2g.emcoreapp: bootmenu-ipodnano2g

flashfiles/bootmenu-ipodnano2g.emcoreapp: $(BOOTMENUDIR)/build/bootmenu-ipodnano2g.emcoreapp bootmenu-ipodnano2g
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

flashfiles/crapple.png: $(BOOTMENUDIR)/images/crapple.png
	@echo [CP]     $@
	@cp $< $@

$(EMCOREDIR)/loader/ipodnano2g/build/emcoreldr-ipodnano2g.dfu: emcoreldr-ipodnano2g

flashfiles/emcoreldr-ipodnano2g.dfu: $(EMCOREDIR)/loader/ipodnano2g/build/emcoreldr-ipodnano2g.dfu emcoreldr-ipodnano2g
	@echo [CP]     $@
	@cp $< $@

flashfiles/emcore-ipodnano2g.ucl: flashfiles/emcore-ipodnano2g.bin
	@echo [UCL]    $<
	@$(UCLPACK) $< $@

$(EMCOREDIR)/build/ipodnano2g/emcore.bin: emcore

flashfiles/emcore-ipodnano2g.bin: $(EMCOREDIR)/build/ipodnano2g/emcore.bin emcore
	@echo [EMBCFG] $@
	@$(EMCOREBOOTCFG) $< $@ "(3, '/.boot/init.emcoreapp', None, (2, 'bootmenu', None, None))"

emcore:
	@make -C $(EMCOREDIR) ipodnano2g

emcoreldr-ipodnano2g:
	@make -C $(EMCOREDIR)/loader/ipodnano2g

bootmenu-ipodnano2g:
	@make -C $(BOOTMENUDIR)

libboot:
	@make -C $(LIBBOOTDIR)

libpng:
	@make -C $(LIBPNGDIR)

libui:
	@make -C $(LIBUIDIR)

umsboot:
	@make -C $(UMSBOOTDIR) ipodnano2g

clean:
	@rm -rf build

.PHONY: all clean emcore emcoreldr-ipodnano2g bootmenu-ipodnano2g libboot libpng libui umsboot libucl flashfiles $(NAME)
