#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 23982 2009-12-13 21:30:28Z bluebrother $
#

DIR = ../build/
OUT = $(DIR)libucl/

ifeq ($(shell uname),WindowsNT)
CCACHE :=
else
CCACHE := $(shell which ccache)
endif

CROSS   ?= arm-elf-eabi-
CC      := $(CCACHE) $(CROSS)gcc
AR      := $(CROSS)ar
RANLIB  := $(CROSS)ranlib

OBJS = $(OUT)/alloc.o $(OUT)/n2b_99.o $(OUT)/n2b_d.o $(OUT)/n2b_ds.o $(OUT)/n2b_to.o $(OUT)/n2d_99.o	\
	$(OUT)/n2d_d.o $(OUT)/n2d_ds.o $(OUT)/n2d_to.o $(OUT)/n2e_99.o $(OUT)/n2e_d.o $(OUT)/n2e_ds.o $(OUT)/n2e_to.o $(OUT)/ucl_crc.o	\
	$(OUT)/ucl_dll.o $(OUT)/ucl_init.o $(OUT)/ucl_ptr.o $(OUT)/ucl_str.o $(OUT)/ucl_util.o

$(DIR)/libucl.a: $(OBJS)
	@echo [AR]     $@
	@$(AR) ruv $@ $^
	@echo [RANLIB] $@
	@$(RANLIB) $@

$(OUT)/%.o: %.c
	@echo [CC]     $<
ifeq ($(shell uname),WindowsNT)
	@-if not exist $(subst /,\,$(dir $@)) md $(subst /,\,$(dir $@))
else
	@-mkdir -p $(dir $@)
endif
	@$(CC) $(CFLAGS) -c $< -o $@
    
clean:
	rm -f $(OUT)/libucl.a
