#ifndef _G_FASTBOOT_H_
#define _G_FASTBOOT_H_

#define EP_BUFFER_SIZE			4096
#define FASTBOOT_INTERFACE_CLASS	0xff
#define FASTBOOT_INTERFACE_SUB_CLASS	0x42
#define FASTBOOT_INTERFACE_PROTOCOL	0x03
#define FASTBOOT_VERSION		"0.4"

extern struct fastboot_config fb_cfg;
extern struct usb_ep *ep_in;
extern struct usb_request *req_in;
extern struct usb_ep *ep_out;
extern struct usb_request *req_out;

void rx_handler_command(struct usb_ep *ep, struct usb_request *req);
int fastboot_tx_write(const char *buffer, unsigned int buffer_size);
const char *fb_find_usb_string(unsigned int id);

#endif
