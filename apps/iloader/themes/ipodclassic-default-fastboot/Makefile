NAME ?= iloader-theme-ipodclassic-default-fastboot

ILOADER = ../..
ASSETPATH = iLoader
SOURCEPATH = source

UCL2E10SINGLEBLK = ucl2e10singleblk
COMPILECONFIG = python $(ILOADER)/tools/compileconfig.py
ZIP = zip -r

CONFIG = iLoader/iloader.cfg
CONFIGSRC = $(SOURCEPATH)/iloader.conf
ASSETS = $(ASSETPATH)/theme.ucl
ZIPFILE = $(NAME).zip

all: $(ZIPFILE)

$(ZIPFILE): $(ASSETS) $(CONFIG)
	@echo [ZIP]    $@
	@rm -f $(ZIPFILE)
ifeq ($(shell uname),WindowsNT)
	@$(ZIP) $@ iLoader
else
	@$(ZIP) --exclude='*/.svn/*' $@ iLoader
endif

$(CONFIG): $(CONFIGSRC)
	@echo [CCONF]  $<
	@$(COMPILECONFIG) $< $@

$(ASSETPATH)/%.ucl: $(SOURCEPATH)/%.bmp
	@echo [UCL]    $<
	@$(UCL2E10SINGLEBLK) $< $@

.PHONY: all clean dist

clean:
	@$(RM) $(CONFIG) $(ASSETS) $(ZIPFILE)

dist: all
	@$(RM) $(CONFIG) $(ASSETS)
