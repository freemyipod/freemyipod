GCC ?= gcc
CFLAGS += -O2 -Wall -Wextra -Werror $(shell pkg-config --cflags libusb-1.0 fuse)
LIBS = $(shell pkg-config --libs libusb-1.0 fuse)

SOURCES_COMMON = util.c usb.c emcore.c cache.c fuse.c
SOURCES_EMCOREFS = emcorefs.c
SOURCES_EMCORE_TEST = emcore-test.c

TARGET_EMCOREFS = build/emcorefs
TARGET_EMCORE_TEST = build/emcore-test
TARGETS = $(TARGET_EMCOREFS) $(TARGET_EMCORE_TEST)

all: $(TARGETS)

$(TARGET_EMCOREFS): $(SOURCES_COMMON) $(SOURCES_EMCOREFS)
	@mkdir -p build
	$(GCC) $(CFLAGS) -o $(TARGET_EMCOREFS) $(SOURCES_COMMON) $(SOURCES_EMCOREFS) $(LIBS)

debug: $(SOURCES_COMMON) $(SOURCES_EMCOREFS)
	@mkdir -p build
	$(GCC) $(CFLAGS) -DDEBUG -g -o $(TARGET_EMCOREFS) $(SOURCES_COMMON) $(SOURCES_EMCOREFS) $(LIBS)

$(TARGET_EMCORE_TEST): $(SOURCES_COMMON) $(SOURCES_EMCORE_TEST)
	@mkdir -p build
	$(GCC) $(CFLAGS) -DDEBUG -DDEBUG_USB_PACKETS -g -o $(TARGET_EMCORE_TEST) $(SOURCES_COMMON) $(SOURCES_EMCORE_TEST) $(LIBS)

clean:
	@rm -rf build
