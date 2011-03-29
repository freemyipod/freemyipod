NAME := bootstrap_ipodclassic_itunes
DFUIMAGE ?= ../../../apps/installer-ipodclassic/build/bootstrap-ipodclassic.dfu

ifeq ($(shell uname),WindowsNT)
CCACHE :=
else
CCACHE := $(shell which ccache)
endif

CC      := $(CCACHE) gcc
LD      := $(CCACHE) gcc

CFLAGS  += -Os -fomit-frame-pointer "-DDFUIMAGE=\"$(DFUIMAGE)\""
LDFLAGS += -Wl,-s

preprocess = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#")
preprocesspaths = $(shell $(CC) $(PPCFLAGS) $(2) -E -P -x c $(1) | grep -v "^\#" | sed -e "s:^..*:$(dir $(1))&:" | sed -e "s:^\\./::")

REVISION := $(shell svnversion .)
REVISIONINT := $(shell echo $(REVISION) | sed -e "s/[^0-9].*$$//")

SRC := $(call preprocesspaths,SOURCES,-I. -I..)
OBJ := $(SRC:%.c=build/%.o)
OBJ := $(OBJ:%.S=build/%.o)

all: $(NAME)

-include $(OBJ:%=%.dep)

$(NAME): build/$(NAME).exe

build/resources.o: $(DFUIMAGE)

build/$(NAME).exe: $(OBJ)
	@echo [LD]     $@
	@$(LD) -o $@ $(OBJ) $(LDFLAGS)

build/%.o: %.c build/version.h
	@echo [CC]     $<
ifeq ($(shell uname),WindowsNT)
	@-if not exist $(subst /,\,$(dir $@)) md $(subst /,\,$(dir $@))
else
	@-mkdir -p $(dir $@)
endif
	@$(CC) -c $(CFLAGS) -o $@ $<
	@$(CC) -MM $(CFLAGS) $< > $@.dep.tmp
	@sed -e "s|.*:|$@:|" < $@.dep.tmp > $@.dep
ifeq ($(shell uname),WindowsNT)
	@sed -e "s/.*://" -e "s/\\$$//" < $@.dep.tmp | fmt -1 | sed -e "s/^ *//" -e "s/$$/:/" >> $@.dep
else
	@sed -e 's/.*://' -e 's/\\$$//' < $@.dep.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $@.dep
endif
	@rm -f $@.dep.tmp

build/%.o: %.S build/version.h
	@echo [CC]     $<
ifeq ($(shell uname),WindowsNT)
	@-if not exist $(subst /,\,$(dir $@)) md $(subst /,\,$(dir $@))
else
	@-mkdir -p $(dir $@)
endif
	@$(CC) -c $(CFLAGS) -o $@ $<
	@$(CC) -MM $(CFLAGS) $< > $@.dep.tmp
	@sed -e "s|.*:|$@:|" < $@.dep.tmp > $@.dep
ifeq ($(shell uname),WindowsNT)
	@sed -e "s/.*://" -e "s/\\$$//" < $@.dep.tmp | fmt -1 | sed -e "s/^ *//" -e "s/$$/:/" >> $@.dep
else
	@sed -e 's/.*://' -e 's/\\$$//' < $@.dep.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $@.dep
endif
	@rm -f $@.dep.tmp

build/version.h: version.h .svn/entries
	@echo [PP]     $<
ifeq ($(shell uname),WindowsNT)
	@-if not exist build md build
	@sed -e "s/\$$REVISION\$$/$(REVISION)/" -e "s/\$$REVISIONINT\$$/$(REVISIONINT)/" < $< > $@
else
	@-mkdir -p build
	@sed -e 's/\$$REVISION\$$/$(REVISION)/' -e 's/\$$REVISIONINT\$$/$(REVISIONINT)/' < $< > $@
endif

clean:
	@rm -rf build

.PHONY: all clean $(NAME)
