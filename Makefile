.DEFAULT_GOAL := help
CROSS ?= arm-none-eabi-
PYTHON ?= python3
.PHONY: help n5g-stage1
help:
	@echo "Use make n5g-stage1 to build the nano5g SRAM loader."
n5g-stage1:
	+$(MAKE) -C emcore -f target/ipodnano5g/stage1.mk CROSS="$(CROSS)" n5g-stage1
