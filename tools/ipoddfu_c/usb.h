int usb_init(void);
int usb_find(unsigned char *reattach);
int usb_open(libusb_device *dev, unsigned char *reattach);
int usb_bulk_transfer(const unsigned char endpoint, unsigned char *data, const unsigned int length);
int usb_control_transfer(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength);
int usb_close(const unsigned char reattach);
void usb_exit(void);
