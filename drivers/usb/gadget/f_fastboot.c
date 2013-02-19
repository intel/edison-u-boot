/*
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * Copyright (c) 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <errno.h>
#include <usb/fastboot.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/compiler.h>

#include "g_fastboot.h"

#define CONFIGURATION_NORMAL      1
#define BULK_ENDPOINT 1
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0  (0x0200)
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1  (0x0040)
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE      (0x0040)

static struct usb_string def_usb_fb_strings[] = {
	{ FB_STR_PRODUCT_IDX,		"Default Product" },
	{ FB_STR_SERIAL_IDX,		"1234567890" },
	{ FB_STR_CONFIG_IDX,		"Android Fastboot" },
	{ FB_STR_INTERFACE_IDX,		"Android Fastboot" },
	{ FB_STR_MANUFACTURER_IDX,	"Default Manufacturer" },
	{ FB_STR_PROC_REV_IDX,		"Default 1.0" },
	{ FB_STR_PROC_TYPE_IDX,		"Emulator" },
	{  }
};

static struct usb_gadget_strings def_fb_strings = {
	.language	= 0x0409, /* en-us */
	.strings	= def_usb_fb_strings,
};

static struct usb_gadget_strings *vendor_fb_strings;

static unsigned int gadget_is_connected;

static u8 ep0_buffer[512];
static u8 ep_out_buffer[EP_BUFFER_SIZE];
static u8 ep_in_buffer[EP_BUFFER_SIZE];
static int current_config;

/* e1 */
static struct usb_endpoint_descriptor fs_ep_in = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN, /* IN */
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize     = TX_ENDPOINT_MAXIMUM_PACKET_SIZE,
	.bInterval          = 0x00,
};

/* e2 */
static struct usb_endpoint_descriptor fs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT, /* OUT */
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1,
	.bInterval		= 0x00,
};

static struct usb_endpoint_descriptor hs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT, /* OUT */
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0,
	.bInterval		= 0x00,
};

const char *fb_find_usb_string(unsigned int id)
{
	struct usb_string *s;

	for (s = vendor_fb_strings->strings; s && s->s; s++) {
		if (s->id == id)
			break;
	}
	if (!s || !s->s) {
		for (s = def_fb_strings.strings; s && s->s; s++) {
			if (s->id == id)
				break;
		}
	}
	if (!s)
		return NULL;
	return s->s;
}

static struct usb_gadget *g;
static struct usb_request *ep0_req;

struct usb_ep *ep_in;
struct usb_request *req_in;

struct usb_ep *ep_out;
struct usb_request *req_out;

static void fastboot_ep0_complete(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;

	if (!status)
		return;
	printf("ep0 status %d\n", status);
}

static int fastboot_bind(struct usb_gadget *gadget)
{

	g = gadget;
	ep0_req = usb_ep_alloc_request(g->ep0, 0);
	if (!ep0_req)
		goto err;
	ep0_req->buf = ep0_buffer;
	ep0_req->complete = fastboot_ep0_complete;

	ep_in = usb_ep_autoconfig(gadget, &fs_ep_in);
	if (!ep_in)
		goto err;
	ep_in->driver_data = ep_in;

	ep_out = usb_ep_autoconfig(gadget, &fs_ep_out);
	if (!ep_out)
		goto err;
	ep_out->driver_data = ep_out;

	hs_ep_out.bEndpointAddress = fs_ep_out.bEndpointAddress;
	return 0;
err:
	return -1;
}

static void fastboot_unbind(struct usb_gadget *gadget)
{
	usb_ep_free_request(g->ep0, ep0_req);
	ep_in->driver_data = NULL;
	ep_out->driver_data = NULL;
}

/* This is the TI USB vendor id a product ID from TI's internal tree */
#define DEVICE_VENDOR_ID  0x0451
#define DEVICE_PRODUCT_ID 0xd022
#define DEVICE_BCD        0x0100

struct usb_device_descriptor fb_descriptor = {
	.bLength            = sizeof(fb_descriptor),
	.bDescriptorType    = USB_DT_DEVICE,
	.bcdUSB             = 0x200,
	.bMaxPacketSize0    = 0x40,
	.idVendor           = DEVICE_VENDOR_ID,
	.idProduct          = DEVICE_PRODUCT_ID,
	.bcdDevice          = DEVICE_BCD,
	.iManufacturer      = FB_STR_MANUFACTURER_IDX,
	.iProduct           = FB_STR_PRODUCT_IDX,
	.iSerialNumber      = FB_STR_SERIAL_IDX,
	.bNumConfigurations = 1,
};

#define TOT_CFG_DESC_LEN	(USB_DT_CONFIG_SIZE + USB_DT_INTERFACE_SIZE + \
		USB_DT_ENDPOINT_SIZE + USB_DT_ENDPOINT_SIZE)

static struct usb_config_descriptor config_desc = {
	.bLength		= USB_DT_CONFIG_SIZE,
	.bDescriptorType	= USB_DT_CONFIG,
	.wTotalLength		= cpu_to_le16(TOT_CFG_DESC_LEN),
	.bNumInterfaces		= 1,
	.bConfigurationValue	= CONFIGURATION_NORMAL,
	.iConfiguration		= FB_STR_CONFIG_IDX,
	.bmAttributes		= 0xc0,
	.bMaxPower		= 0x32,
};

static struct usb_interface_descriptor interface_desc = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= 0x00,
	.bAlternateSetting	= 0x00,
	.bNumEndpoints		= 0x02,
	.bInterfaceClass	= FASTBOOT_INTERFACE_CLASS,
	.bInterfaceSubClass	= FASTBOOT_INTERFACE_SUB_CLASS,
	.bInterfaceProtocol	= FASTBOOT_INTERFACE_PROTOCOL,
	.iInterface		= FB_STR_INTERFACE_IDX,
};

static struct usb_qualifier_descriptor qual_desc = {
	.bLength		= sizeof(qual_desc),
	.bDescriptorType	= USB_DT_DEVICE_QUALIFIER,
	.bcdUSB			= 0x200,
	.bMaxPacketSize0	= 0x40,
	.bNumConfigurations	= 1,
};

static int fastboot_setup_get_descr(struct usb_gadget *gadget,
		const struct usb_ctrlrequest *ctrl)
{
	u16 w_value	= le16_to_cpu(ctrl->wValue);
	u16 w_length	= le16_to_cpu(ctrl->wLength);
	u16 val;
	int ret;
	u32 bytes_remaining;
	u32 bytes_total;
	u32 this_inc;

	val = w_value >> 8;

	switch (val) {
	case USB_DT_DEVICE:

		memcpy(ep0_buffer, &fb_descriptor, sizeof(fb_descriptor));
		ep0_req->length = min(w_length, sizeof(fb_descriptor));
		ret = usb_ep_queue(gadget->ep0, ep0_req, 0);
		break;

	case USB_DT_CONFIG:

		bytes_remaining = min(w_length, sizeof(ep0_buffer));
		bytes_total = 0;

		/* config */
		this_inc = min(bytes_remaining, USB_DT_CONFIG_SIZE);
		bytes_remaining -= this_inc;
		memcpy(ep0_buffer + bytes_total, &config_desc, this_inc);
		bytes_total += this_inc;

		/* interface */
		this_inc = min(bytes_remaining, USB_DT_INTERFACE_SIZE);
		bytes_remaining -= this_inc;
		memcpy(ep0_buffer + bytes_total, &interface_desc, this_inc);
		bytes_total += this_inc;

		/* ep in */
		this_inc = min(bytes_remaining, USB_DT_ENDPOINT_SIZE);
		bytes_remaining -= this_inc;
		memcpy(ep0_buffer + bytes_total, &fs_ep_in, this_inc);
		bytes_total += this_inc;

		/* ep out */
		this_inc = min(bytes_remaining, USB_DT_ENDPOINT_SIZE);

		if (gadget->speed == USB_SPEED_HIGH)
			memcpy(ep0_buffer + bytes_total, &hs_ep_out,
					this_inc);
		else
			memcpy(ep0_buffer + bytes_total, &fs_ep_out,
					this_inc);
		bytes_total += this_inc;

		ep0_req->length = bytes_total;
		ret = usb_ep_queue(gadget->ep0, ep0_req, 0);
		break;

	case USB_DT_STRING:

		ret = usb_gadget_get_string(vendor_fb_strings,
				w_value & 0xff, ep0_buffer);
		if (ret < 0)
			ret = usb_gadget_get_string(&def_fb_strings,
					w_value & 0xff, ep0_buffer);
		if (ret < 0)
			break;

		ep0_req->length = ret;
		ret = usb_ep_queue(gadget->ep0, ep0_req, 0);
		break;

	case USB_DT_DEVICE_QUALIFIER:

		memcpy(ep0_buffer, &qual_desc, sizeof(qual_desc));
		ep0_req->length = min(w_length, sizeof(qual_desc));
		ret = usb_ep_queue(gadget->ep0, ep0_req, 0);
		 break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int fastboot_setup_get_conf(struct usb_gadget *gadget,
		const struct usb_ctrlrequest *ctrl)
{
	u16 w_length	= le16_to_cpu(ctrl->wLength);

	if (w_length == 0)
		return -1;

	ep0_buffer[0] = current_config;
	ep0_req->length = 1;
	return usb_ep_queue(gadget->ep0, ep0_req, 0);
}

static void fastboot_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;

	if (status)
		printf("status: %d ep_in trans: %d\n",
				status,
				req->actual);
}

static int fastboot_disable_ep(struct usb_gadget *gadget)
{
	if (req_out) {
		usb_ep_free_request(ep_out, req_out);
		req_out = NULL;
	}
	if (req_in) {
		usb_ep_free_request(ep_in, req_in);
		req_in = NULL;
	}
	usb_ep_disable(ep_out);
	usb_ep_disable(ep_in);

	return 0;
}

static int fastboot_enable_ep(struct usb_gadget *gadget)
{
	int ret;

	/* make sure we don't enable the ep twice */
	if (gadget->speed == USB_SPEED_HIGH)
		ret = usb_ep_enable(ep_out, &hs_ep_out);
	else
		ret = usb_ep_enable(ep_out, &fs_ep_out);
	if (ret) {
		printf("failed to enable out ep\n");
		goto err;
	}

	req_out = usb_ep_alloc_request(ep_out, 0);
	if (!req_out) {
		printf("failed to alloc out req\n");
		goto err;
	}

	ret = usb_ep_enable(ep_in, &fs_ep_in);
	if (ret) {
		printf("failed to enable in ep\n");
		goto err;
	}
	req_in = usb_ep_alloc_request(ep_in, 0);
	if (!req_in) {
		printf("failed alloc req in\n");
		goto err;
	}

	req_out->complete = rx_handler_command;
	req_out->buf = ep_out_buffer;
	req_out->length = sizeof(ep_out_buffer);

	req_in->buf = ep_in_buffer;
	req_in->length = sizeof(ep_in_buffer);

	ret = usb_ep_queue(ep_out, req_out, 0);
	if (ret)
		goto err;

	return 0;
err:
	fastboot_disable_ep(gadget);
	return -1;
}

static int fastboot_set_interface(struct usb_gadget *gadget, u32 enable)
{
	if (enable && req_out)
		return 0;
	if (!enable && !req_out)
		return 0;

	if (enable)
		return fastboot_enable_ep(gadget);
	else
		return fastboot_disable_ep(gadget);
}

static int fastboot_setup_out_req(struct usb_gadget *gadget,
		const struct usb_ctrlrequest *req)
{
	switch (req->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		switch (req->bRequest) {
		case USB_REQ_SET_CONFIGURATION:

			ep0_req->length = 0;
			if (req->wValue == CONFIGURATION_NORMAL) {
				current_config = CONFIGURATION_NORMAL;
				fastboot_set_interface(gadget, 1);
				return usb_ep_queue(gadget->ep0,
						ep0_req, 0);
			}
			if (req->wValue == 0) {
				current_config = 0;
				fastboot_set_interface(gadget, 0);
				return usb_ep_queue(gadget->ep0,
						ep0_req, 0);
			}
			return -1;
			break;
		default:
			return -1;
		};

	case USB_RECIP_INTERFACE:
		switch (req->bRequest) {
		case USB_REQ_SET_INTERFACE:

			ep0_req->length = 0;
			if (!fastboot_set_interface(gadget, 1))
				return usb_ep_queue(gadget->ep0,
						ep0_req, 0);
			return -1;
			break;
		default:
			return -1;
		}

	case USB_RECIP_ENDPOINT:
		switch (req->bRequest) {
		case USB_REQ_CLEAR_FEATURE:

			return usb_ep_queue(gadget->ep0, ep0_req, 0);
			break;
		default:
			return -1;
		}
	}
	return -1;
}

static int fastboot_setup(struct usb_gadget *gadget,
		const struct usb_ctrlrequest *req)
{
	if ((req->bRequestType & USB_TYPE_MASK) != USB_TYPE_STANDARD)
		return -1;

	if ((req->bRequestType & USB_DIR_IN) == 0)
		/* host-to-device */
		return fastboot_setup_out_req(gadget, req);

	/* device-to-host */
	if ((req->bRequestType & USB_RECIP_MASK) == USB_RECIP_DEVICE) {
		switch (req->bRequest) {
		case USB_REQ_GET_DESCRIPTOR:
			return fastboot_setup_get_descr(gadget, req);
			break;

		case USB_REQ_GET_CONFIGURATION:
			return fastboot_setup_get_conf(gadget, req);
			break;
		default:
			return -1;
		}
	}
	return -1;
}

static void fastboot_disconnect(struct usb_gadget *gadget)
{
	fastboot_disable_ep(gadget);
	gadget_is_connected = 0;
}

struct usb_gadget_driver fast_gadget = {
	.bind		= fastboot_bind,
	.unbind		= fastboot_unbind,
	.setup		= fastboot_setup,
	.disconnect	= fastboot_disconnect,
};

static int udc_is_probbed;

int fastboot_init(void)
{
	int ret;

	ret = fastboot_board_init(&fb_cfg, &vendor_fb_strings);
	if (ret)
		return ret;
	if (!vendor_fb_strings)
		return -EINVAL;

	ret = usb_gadget_init_udc();
	if (ret) {
		printf("gadget probe failed\n");
		return 1;
	}
	udc_is_probbed = 1;

	ret = usb_gadget_register_driver(&fast_gadget);
	if (ret) {
		printf("Add gadget failed\n");
		goto err;
	}

	gadget_is_connected = 1;
	usb_gadget_handle_interrupts();
	return 0;

err:
	fastboot_shutdown();
	return 1;
}

int fastboot_poll(void)
{
	usb_gadget_handle_interrupts();

	if (gadget_is_connected)
		return 0;
	else
		return 1;
}

void fastboot_shutdown(void)
{
	if (!udc_is_probbed)
		return;
	udc_is_probbed = 0;
	usb_gadget_exit_udc();
}

int fastboot_tx_write(const char *buffer, unsigned int buffer_size)
{
	int ret;

	if (req_in->complete == NULL)
		req_in->complete = fastboot_complete_in;

	memcpy(req_in->buf, buffer, buffer_size);
	req_in->length = buffer_size;
	ret = usb_ep_queue(ep_in, req_in, 0);
	if (ret)
		printf("Error %d on queue\n", ret);
	return 0;
}
