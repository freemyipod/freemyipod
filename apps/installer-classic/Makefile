NAME := installer-classic

EMBIOSDIR ?= ../../embios/trunk/
ILOADERDIR ?= ../iloader/
UMSBOOTDIR ?= ../../umsboot/
TOOLSDIR ?= ../../tools/

BITMAPS = build/sidepane.ucl build/warning.ucl build/installing.ucl build/formatting.ucl build/copying.ucl build/flashing.ucl

FLASHFILES = flashfiles/iloader.cfg.ucl flashfiles/iloader.embiosapp.ucl \
             flashfiles/embiosldr-ipodclassic.bin flashfiles/embios-ipodclassic.ucl flashfiles/umsboot-ipodclassic.ucl

CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
AS      := $(CROSS)as
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
UCLPACK := ucl2e10singleblk
GENINSTALLER := python tools/geninstaller.py
STUBEMBED := python tools/stubembed.py
COMPILECONFIG := python $(ILOADERDIR)/tools/compileconfig.py
EMBIOSBOOTCFG := python $(EMBIOSDIR)/tools/embiosbootcfg.py
EMBIOSEMBEDAPP := python $(EMBIOSDIR)/tools/embiosembedapp.py
GENPWN := python $(EMBIOSDIR)/tools/ipodcrypt.py s5l8702-genpwnage

CFLAGS  += -Os -fno-pie -fno-stack-protector -fomit-frame-pointer -I. -I$(EMBIOSDIR)/export -ffunction-sections -fdata-sections -mcpu=arm940t
LDFLAGS += "$(shell $(CC) -print-libgcc-file-name)" --gc-sections

preprocess = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#")
preprocesspaths = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#" | sed -e "s:^..*:$(dir $(1))&:")

REVISION := $(shell svnversion .)
REVISIONINT := $(shell echo $(REVISION) | sed -e "s/[^0-9].*$$//")

HELPERS := build/__embios_armhelpers.o

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

build/$(NAME).ubi: $(EMBIOSDIR)/build/ipodclassic/embios.bin build/$(NAME).embiosapp.ucl
	@echo [EMBAPP] $@
	@$(EMBIOSEMBEDAPP) $^ $@ --run-from=0x08f00000 --compressed

build/$(NAME).embiosapp.ucl: build/$(NAME).embiosapp
	@echo [UCL]    $<
	@$(UCLPACK) $< $@

build/$(NAME).embiosapp: build/$(NAME).plain.embiosapp $(BITMAPS) flashfiles.built
	@echo [GENINS] $<
	@$(GENINSTALLER) $< $@ /notes/$(BASENAME).bootnote fsfiles

build/bootstub.bin: build/bootstub.elf
	@echo [OC]     $<
	@$(OBJCOPY) -O binary $^ $@

build/bootstub.elf: bootstub/ls.x build/bootstub/bootstub.o
	@echo [LD]     $@
	@$(LD) $(LDFLAGS) -o $@ -T bootstub/ls.x build/bootstub/bootstub.o

build/%.ucl: oobe/%.bmp
	@echo [UCL]    $<
ifeq ($(shell uname),WindowsNT)
	@-if not exist $(subst /,\,$(dir $@)) md $(subst /,\,$(dir $@))
else
	@-mkdir -p $(dir $@)
endif
	@$(UCLPACK) $< $@

build/$(NAME).plain.embiosapp: build/$(NAME).elf
	@echo [OC]     $<
	@$(OBJCOPY) -O binary $^ $@

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

build/__embios_%.o: $(EMBIOSDIR)/export/%.S
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

$(UMSBOOTDIR)/build/ipodclassic/umsboot-ipodclassic.ucl: umsboot

flashfiles/umsboot-ipodclassic.ucl: $(UMSBOOTDIR)/build/ipodclassic/umsboot-ipodclassic.ucl umsboot
	@echo [CP]     $@
	@cp $< $@

flashfiles/iloader.cfg: flashfiles/iloader.conf
	@echo [COMCFG] $@
	@$(COMPILECONFIG) $< $@

flashfiles/iloader.cfg.ucl: flashfiles/iloader.cfg
	@echo [UCL]    $<
	@$(UCLPACK) $< $@

$(ILOADERDIR)/build/iloader.embiosapp.ucl: iloader

flashfiles/iloader.embiosapp.ucl: $(ILOADERDIR)/build/iloader.embiosapp.ucl iloader
	@echo [CP]     $@
	@cp $< $@

$(EMBIOSDIR)/loader/ipodclassic/build/embiosldr-ipodclassic.bin: embiosldr-ipodclassic

flashfiles/embiosldr-ipodclassic.bin: $(EMBIOSDIR)/loader/ipodclassic/build/embiosldr-ipodclassic.bin embiosldr-ipodclassic
	@echo [CP]     $@
	@cp $< $@

flashfiles/embios-ipodclassic.ucl: flashfiles/embios-ipodclassic.bin
	@echo [UCL]    $<
	@$(UCLPACK) $< $@

$(EMBIOSDIR)/build/ipodclassic/embios.bin: embios

flashfiles/embios-ipodclassic.bin: $(EMBIOSDIR)/build/ipodclassic/embios.bin embios
	@echo [EMBCFG] $@
	@$(EMBIOSBOOTCFG) $< $@ --file=/iLoader/boot.embiosapp --file-compressed --file-run-from=0x08000000 \
	                        --flash=iloader --flash-compressed --flash-run-from=0x09e00000

embios:
	@make -C $(EMBIOSDIR) ipodclassic

embiosldr-ipodclassic:
	@make -C $(EMBIOSDIR)/loader/ipodclassic

iloader:
	@make -C $(ILOADERDIR)

$(UMSBOOTDIR)/build/ipodclassic/umsboot.bin: umsboot

umsboot:
	@make -C $(UMSBOOTDIR) ipodclassic

clean:
	@rm -rf build

.PHONY: all clean iloader embios embiosldr-ipodclasic umsboot libucl flashfiles $(NAME)
