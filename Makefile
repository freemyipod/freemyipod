# Explicit nano5g build entry points. Other builds remain in their own directories.
.DEFAULT_GOAL := help
CROSS ?= arm-none-eabi-
HOSTCC ?= cc
PYTHON ?= python3

.PHONY: help all nano5g n5g n5g-image n5g-kernel n5g-stage1 clean nano5g-clean
help:
	@echo "Use make nano5g for the nano5g RAM-boot image; make -C emcore for the original targets."
all:
	+$(MAKE) -C emcore all
nano5g n5g n5g-image:
	+$(MAKE) -C emcore TARGETS=ipodnano5g CROSS="$(CROSS)" HOSTCC="$(HOSTCC)" PYTHON="$(PYTHON)" ipodnano5g
n5g-kernel:
	+$(MAKE) -C emcore TARGETS=ipodnano5g CROSS="$(CROSS)" HOSTCC="$(HOSTCC)" build/ipodnano5g/emcore.bin
n5g-stage1:
	+$(MAKE) -C emcore TARGETS=ipodnano5g CROSS="$(CROSS)" n5g-stage1
clean:
	+$(MAKE) -C emcore clean
nano5g-clean:
	+$(MAKE) -C emcore TARGETS=ipodnano5g nano5g-clean
