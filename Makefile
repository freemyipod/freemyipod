.DEFAULT_GOAL := help
CROSS ?= arm-none-eabi-
HOSTCC ?= cc
PYTHON ?= python3
N5G_BUILD := emcore/build/ipodnano5g
.PHONY: help n5g-stage1 n5g-kernel n5g-image
help:
	@echo "Use make n5g-image to build the nano5g RAM-boot image."
n5g-stage1:
	+$(MAKE) -C emcore -f target/ipodnano5g/stage1.mk CROSS="$(CROSS)" n5g-stage1
n5g-kernel:
	+$(MAKE) -C emcore CROSS="$(CROSS)" HOSTCC="$(HOSTCC)" TARGETS=ipodnano5g ipodnano5g
n5g-image: n5g-kernel
	@mkdir -p recovery-image
	$(PYTHON) emcore/tools/n5g.py image --stage1 $(N5G_BUILD)/stage1.bin --payload $(N5G_BUILD)/emcore.bin --stage1-elf $(N5G_BUILD)/stage1.elf --kernel-elf $(N5G_BUILD)/emcore.elf --nm $(CROSS)nm --raw recovery-image/emcore-nano5g.bin -o recovery-image/emcore-nano5g.dfu
	$(CROSS)nm -n $(N5G_BUILD)/emcore.elf > recovery-image/emcore-nano5g.nm
