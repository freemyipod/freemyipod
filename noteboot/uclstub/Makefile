NAME := uclstub

ifeq ($(shell uname),WindowsNT)
CCACHE :=
else
CCACHE := $(shell which ccache)
endif

CROSS   ?= arm-elf-eabi-
CC      := $(CCACHE) $(CROSS)gcc
AS      := $(CROSS)as
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy

CFLAGS  += -Os -fno-pie -fno-stack-protector -fomit-frame-pointer -I. -ffunction-sections -fdata-sections -mcpu=arm940t
LDFLAGS += "$(shell $(CC) -print-libgcc-file-name)" --gc-sections

SRC := ucl.S
OBJ := $(SRC:%.S=build/%.o)

all: $(NAME)

$(NAME): build/$(NAME).bin

build/$(NAME).bin: build/$(NAME).elf
	@echo [OC]     $<
	@$(OBJCOPY) -O binary $^ $@

build/$(NAME).elf: $(OBJ) ls.x
	@echo [LD]     $@
	@$(LD) $(LDFLAGS) -T ls.x -o $@ $(OBJ)

build/%.o: %.S
	@echo [CC]     $<
ifeq ($(shell uname),WindowsNT)
	@-if not exist $(subst /,\,$(dir $@)) md $(subst /,\,$(dir $@))
else
	@-mkdir -p $(dir $@)
endif
	@$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -rf build

.PHONY: all clean $(NAME)
