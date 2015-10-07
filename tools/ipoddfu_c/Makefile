TARGET = ipoddfu
PKGCONFIG ?= pkg-config
CFLAGS = -O2 -Wall -Wextra -Werror -g $(shell $(PKGCONFIG) --cflags libusb-1.0)
LDFLAGS = $(shell $(PKGCONFIG) --libs-only-L libusb-1.0)
LDLIBS = $(shell $(PKGCONFIG) --libs-only-l libusb-1.0)

all: $(TARGET)

$(TARGET): usb.o dfu.o crc32.o misc.o

clean:
	rm -rf $(TARGET) *.o
