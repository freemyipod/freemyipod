CFLAGS := -O2 -Wall -Wextra -Werror -g -lusb-1.0

all: build ipoddfu

ipoddfu:
	gcc $(CFLAGS) -o build/ipoddfu usb.c dfu.c crc32.c misc.c ipoddfu.c

build:
	@mkdir $@

clean:
	@rm -rf build
