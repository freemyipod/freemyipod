# Standalone SRAM loader; also included by target.mk for kernel builds.
CROSS ?= arm-none-eabi-
CC := $(CROSS)gcc
LD := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
N5G_STAGE1_FLAGS = -mcpu=arm1176jzf-s -marm -ffreestanding -DASM_FILE -DTARGET_ipodnano5g
ifneq ($(N5G_NOCACHE),)
N5G_STAGE1_FLAGS += -DN5G_NOCACHE
endif
.PHONY: n5g-stage1
n5g-stage1: build/ipodnano5g/stage1.bin
build/ipodnano5g/stage1.elf: target/ipodnano5g/stage1.S target/ipodnano5g/stage1.ld target/ipodnano5g/s5l8730.h
	@mkdir -p $(dir $@)
	$(CC) $(N5G_STAGE1_FLAGS) -Itarget/ipodnano5g -c $< -o build/ipodnano5g/stage1.o
	$(LD) -T target/ipodnano5g/stage1.ld -Map build/ipodnano5g/stage1.map -o $@ build/ipodnano5g/stage1.o
build/ipodnano5g/stage1.bin: build/ipodnano5g/stage1.elf
	$(OBJCOPY) -O binary $< $@
