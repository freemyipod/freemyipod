NAME := installer-nano2g

EMBIOSDIR ?= ../../embios/trunk/
ILOADERDIR ?= ../iloader/
UNINSTDIR ?= ../uninstaller-nano2g/
TOOLSDIR ?= ../../tools/

BITMAPS = build/backdrop.ucl build/welcome.ucl build/repartition.ucl build/installing.ucl build/badpartition.ucl \
          build/cancelled.ucl build/repartitioning.ucl build/preparing.ucl build/installfiles.ucl build/flashing.ucl

FLASHFILES = flashfiles/uninstaller-nano2g.embiosapp.ucl flashfiles/iloader.cfg.ucl flashfiles/iloader.embiosapp.ucl \
             flashfiles/embiosldr-ipodnano2g.dfu flashfiles/embios-ipodnano2g.ucl

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
CRYPTFW := python $(EMBIOSDIR)/tools/ipodcrypt.py nano2g-cryptfirmware
SCRAMBLE := python $(TOOLSDIR)/scramble.py

CFLAGS  += -Os -fno-pie -fno-stack-protector -fomit-frame-pointer -I. -I$(EMBIOSDIR)/export -ffunction-sections -fdata-sections -mcpu=arm940t
LDFLAGS += "$(shell $(CC) -print-libgcc-file-name)" --gc-sections

preprocess = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#")
preprocesspaths = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#" | sed -e "s:^..*:$(dir $(1))&:")

REVISION := $(shell svnversion .)
REVISIONINT := $(shell echo $(REVISION) | sed -e "s/[^0-9].*$$//")

HELPERS := build/__embios_armhelpers.o
LIBS := build/libucl.a

SRC := $(call preprocesspaths,SOURCES,-I. -I..)
OBJ := $(SRC:%.c=build/%.o)
OBJ := $(OBJ:%.S=build/%.o) $(HELPERS)

all: $(NAME)

-include $(OBJ:%=%.dep)

$(NAME): build/$(NAME).ipodx

build/$(NAME).ipodx: build/$(NAME).fw
	@echo [SCRAMB] $<
	@$(SCRAMBLE) $< $@ --signature=nn2x --targetid=62

build/$(NAME).fw: build/$(NAME).bin
	@echo [CFW]    $<
	@$(CRYPTFW) $< $@

build/$(NAME).bin: build/bootstub.bin build/$(NAME).integrated.bin
	@echo [STUBEM] $@
	@$(STUBEMBED) $^ $@

build/$(NAME).integrated.bin: $(EMBIOSDIR)/build/ipodnano2g/embios.bin build/$(NAME).embiosapp
	@echo [EMBAPP] $@
	@$(EMBIOSEMBEDAPP) $^ $@ --run-from=0x08000000

build/$(NAME).embiosapp: build/$(NAME).plain.embiosapp $(BITMAPS) flashfiles.built
	@echo [GENINS] $<
	@$(GENINSTALLER) $< $@ 

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

build/__embios_%.o: $(EMBIOSDIR)/export/%.S
	@echo [CC]     $<
ifeq ($(shell uname),WindowsNT)
	@-if not exist $(subst /,\,$(dir $@)) md $(subst /,\,$(dir $@))
else
	@-mkdir -p $(dir $@)
endif
	@$(CC) -c $(CFLAGS) -o $@ $<

build/libucl.a: libucl

libucl:
	@make -C libucl CFLAGS="$(CFLAGS) -I../$(EMBIOSDIR)/export"

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

$(UNINSTDIR)/build/uninstaller-nano2g.embiosapp.ucl: uninstaller-nano2g

flashfiles/uninstaller-nano2g.embiosapp.ucl: $(UNINSTDIR)/build/uninstaller-nano2g.embiosapp.ucl uninstaller-nano2g
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

$(EMBIOSDIR)/loader/ipodnano2g/build/embiosldr-ipodnano2g.dfu: embiosldr-ipodnano2g

flashfiles/embiosldr-ipodnano2g.dfu: $(EMBIOSDIR)/loader/ipodnano2g/build/embiosldr-ipodnano2g.dfu embiosldr-ipodnano2g
	@echo [CP]     $@
	@cp $< $@

flashfiles/embios-ipodnano2g.ucl: flashfiles/embios-ipodnano2g.bin
	@echo [UCL]    $<
	@$(UCLPACK) $< $@

$(EMBIOSDIR)/build/ipodnano2g/embios.bin: embios

flashfiles/embios-ipodnano2g.bin: $(EMBIOSDIR)/build/ipodnano2g/embios.bin embios
	@echo [EMBCFG] $@
	@$(EMBIOSBOOTCFG) $< $@ --file=/iLoader/boot.embiosapp --file-compressed --file-run-from=0x08000000 \
	                        --flash=iloader --flash-compressed --flash-run-from=0x09e00000

embios:
	@make -C $(EMBIOSDIR) ipodnano2g

embiosldr-ipodnano2g:
	@make -C $(EMBIOSDIR)/loader/ipodnano2g

iloader:
	@make -C $(ILOADERDIR)

uninstaller-nano2g:
	@make -C $(UNINSTDIR)

clean:
	@rm -rf build

.PHONY: all clean uninstaller-nano2g iloader embios embiosldr-ipodnano2g libucl flashfiles $(NAME)
