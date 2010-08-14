NAME ?= oobe

ILOADER = ../..
ASSETPATH = oobe
SOURCEPATH = source

UCL2E10SINGLEBLK = ucl2e10singleblk
COMPILECONFIG = python $(ILOADER)/tools/compileconfig.py
ZIP = zip -r

CONFIG = $(ASSETPATH)/iloader.cfg
CONFIGUCL = $(ASSETPATH)/iloader.cfg.ucl
CONFIGSRC = $(SOURCEPATH)/iloader.conf
ASSETS = $(ASSETPATH)/welcome.ucl $(ASSETPATH)/notheme.ucl

all: $(CONFIGUCL) $(ASSETS)

$(CONFIGUCL): $(CONFIG)
	@echo [UCL]    $<
	@$(UCL2E10SINGLEBLK) $< $@

$(CONFIG): $(CONFIGSRC)
	@echo [CCONF]  $<
	@$(COMPILECONFIG) $< $@

$(ASSETPATH)/%.ucl: $(SOURCEPATH)/%.bmp
	@echo [UCL]    $<
	@$(UCL2E10SINGLEBLK) $< $@

.PHONY: all clean dist

clean:
	@$(RM) $(CONFIG) $(ASSETS) $(CONFIGUCL)

dist: all
	@$(RM) $(CONFIG) $(ASSETS)
