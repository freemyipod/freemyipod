CFLAGS += -O2 -Wall -Wextra -Werror $(shell pkg-config --cflags --libs libusb-1.0 fuse)
SOURCES = util.c usb.c emcore.c cache.c fuse.c emcorefs.c
TARGET = build/emcorefs

all: $(TARGET)

build:
	@mkdir $@

$(TARGET): build
	gcc $(CFLAGS) -o $(TARGET) $(SOURCES)

debug: build
	gcc $(CFLAGS) -DDEBUG -g -o $(TARGET) $(SOURCES)

test:
	@mkdir -p mountpoint
	$(TARGET) -s mountpoint/

testdebug:
	@mkdir -p mountpoint
	$(TARGET) -d -s mountpoint/

testonly:
	gcc $(CFLAGS) -DTEST_ONLY -DDEBUG -g -o $(TARGET) $(SOURCES)

clean:
	@rm -rf build
