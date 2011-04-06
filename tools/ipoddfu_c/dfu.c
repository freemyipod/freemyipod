#include <stdio.h>
#include <string.h>

#include <libusb-1.0/libusb.h>

#include "usb.h"

int dfu_write(const unsigned char i, const unsigned int length, const unsigned char *data) {
	return usb_control_transfer(0x21, 1, i, 0, (unsigned char *) data, length);
}

int dfu_read(unsigned char result[6]) {
	return usb_control_transfer(0xa1, 3, 0, 0, result, 6);
}

int dfu_send(const unsigned long int size, const unsigned char *data) {
	unsigned char result[6];
	unsigned int i;
	int res;
	
	printf("Uploading... ");
	fflush(stdout);

	for (i = 0; i < (size + 4 + 2047) / 2048; ++i) {
		res = dfu_write(i, ((i + 1) * 2048 > size + 4) ? (size + 4) - (i * 2048) : 2048, (unsigned char *) (data + (i * 2048)));
		
		if (LIBUSB_SUCCESS > res) {
			return res;
		}
		
		memset(result, 0, sizeof(result));
		
		while ('\x05' != result[4]) {
			res = dfu_read(result);
		
			if (LIBUSB_SUCCESS > res) {
				return res;
			}
		}
		
		printf("#");
		fflush(stdout);
	}
	
	res = dfu_write(i, 0, NULL);

	if (LIBUSB_SUCCESS > res) {
		return res;
	}
	
	memset(result, 0, sizeof(result));

	i = 0;

	while ('\x02' != result[4] && i++ < 1000) {
		dfu_read(result);
		
		if (LIBUSB_SUCCESS > res) {
			return res;
		}
	}

	if (1000 == i || '\x02' == result[4]) {
		printf(" failed: %d / %d\n", result[4], result[0]);
	}
	else {
		printf(" OK\n");
	}
	
	return 0;
}
