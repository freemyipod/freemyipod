CFLAGS_ipodnano5g += -mcpu=arm1176jzf-s
TARGET_OUTPUT_ipodnano5g := build/ipodnano5g/emcore.bin build/ipodnano5g/stage1.bin
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
