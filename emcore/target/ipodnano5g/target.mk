CFLAGS_ipodnano5g += -mcpu=arm1176jzf-s
TARGET_OUTPUT_ipodnano5g := build/ipodnano5g/emcore-nano5g.dfu build/ipodnano5g/emcore-nano5g.bin build/ipodnano5g/emcore-nano5g.nm
ifeq ($(N5G_NOCACHE),1)
CFLAGS_ipodnano5g += -DN5G_NOCACHE
endif
include target/ipodnano5g/stage1.mk

ifneq ($(N5G_BACKLIGHT_MAX),)
CFLAGS_ipodnano5g += -DN5G_BACKLIGHT_MAX=$(N5G_BACKLIGHT_MAX)
endif
ifneq ($(N5G_BACKLIGHT_BOOT),)
CFLAGS_ipodnano5g += -DN5G_BACKLIGHT_BOOT=$(N5G_BACKLIGHT_BOOT)
endif

N5G_NAND_WRITE ?= 1
NAND_DIR := target/ipodnano5g/nand
NAND_BUILD := build/ipodnano5g/nand
FMSSASM := build/host/fmssasm
FMSS_SRC := $(NAND_DIR)/fmss/readid.s $(NAND_DIR)/fmss/read.s $(NAND_DIR)/fmss/program.s $(NAND_DIR)/fmss/bootread.s $(NAND_DIR)/fmss/erase.s
ifneq ($(N5G_NAND_DISABLE),1)
CFLAGS_ipodnano5g += -I$(NAND_BUILD) -DN5G_NAND_WRITE=$(N5G_NAND_WRITE)
else
CFLAGS_ipodnano5g += -DN5G_NAND_DISABLE
PPCFLAGS_ipodnano5g += -DN5G_NAND_DISABLE
endif

$(FMSSASM): ../tools/fmssasm.c build/host/fmssasm-config
	@echo [HOSTCC] $<
	@mkdir -p $(dir $@)
	@$(HOSTCC) -O2 -o $@ $<

$(NAND_BUILD)/fmss-nano5g.h: $(FMSS_SRC) $(FMSSASM)
	@echo [FMSSASM] $@
	@mkdir -p $(dir $@)
	@$(FMSSASM) -s $@ $(FMSS_SRC)

# Compile the NAND/FTL with the regular target rules and dependency tracking.
build/ipodnano5g/$(NAND_DIR)/nand-nano5g.o: $(NAND_BUILD)/fmss-nano5g.h

.PHONY: n5g-fmssasm
n5g-fmssasm: $(FMSSASM)

TARGET_CONFIG_ipodnano5g := build/ipodnano5g/config
.PHONY: n5g-config-force
build/ipodnano5g/config: n5g-config-force
	@mkdir -p $(dir $@)
	@echo '$(CFLAGS) $(CFLAGS_ipodnano5g) $(LDFLAGS) $(CROSS)' > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

.PHONY: nano5g n5g n5g-image n5g-kernel nano5g-clean ipodnano5g
nano5g n5g n5g-image: ipodnano5g
n5g-kernel: build/ipodnano5g/emcore.bin

build/host/fmssasm-config: n5g-config-force
	@mkdir -p $(dir $@)
	@echo '$(HOSTCC)' > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

# Grouped outputs ensure either missing image is regenerated (GNU Make 4.3+).
build/ipodnano5g/emcore-nano5g.dfu build/ipodnano5g/emcore-nano5g.bin &: build/ipodnano5g/stage1.bin build/ipodnano5g/emcore.bin tools/nano5g/image.py tools/n5g.py
	@mkdir -p build/ipodnano5g
	$(PYTHON) tools/n5g.py image --stage1 build/ipodnano5g/stage1.bin --payload build/ipodnano5g/emcore.bin --stage1-elf build/ipodnano5g/stage1.elf --kernel-elf build/ipodnano5g/emcore.elf --nm $(CROSS)nm --raw build/ipodnano5g/emcore-nano5g.bin -o build/ipodnano5g/emcore-nano5g.dfu

build/ipodnano5g/emcore-nano5g.nm: build/ipodnano5g/emcore.elf
	@mkdir -p build/ipodnano5g
	$(CROSS)nm -n $< > $@

nano5g-clean:
	rm -rf build/ipodnano5g
	rm -f build/host/fmssasm build/host/fmssasm-config
