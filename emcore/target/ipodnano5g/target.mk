CFLAGS_ipodnano5g += -mcpu=arm1176jzf-s
TARGET_OUTPUT_ipodnano5g := build/ipodnano5g/emcore.bin build/ipodnano5g/stage1.bin
ifeq ($(N5G_NOCACHE),1)
CFLAGS_ipodnano5g += -DN5G_NOCACHE
endif
include target/ipodnano5g/stage1.mk
