/*
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * Copyright 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>
#include <linux/compiler.h>
#include <version.h>
#include <g_dnl.h>
#include <cmd_boot_brillo.h>
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
#include <fb_mmc.h>
#endif

#define FASTBOOT_VERSION		"0.4"

#define FASTBOOT_INTERFACE_CLASS	0xff
#define FASTBOOT_INTERFACE_SUB_CLASS	0x42
#define FASTBOOT_INTERFACE_PROTOCOL	0x03

#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0  (0x0200)
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1  (0x0040)
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE      (0x0040)

/* The 64 defined bytes plus \0 */
#define RESPONSE_LEN	(64 + 1)

#define EP_BUFFER_SIZE			4096

/* Flashing state: lock/unlock */
#define FB_STATE_UNLOCK 0
#define FB_STATE_LOCK   1
#define FB_STATE_UNKNOWN -1

struct f_fastboot {
	struct usb_function usb_function;

	/* IN/OUT EP's and corresponding requests */
	struct usb_ep *in_ep, *out_ep;
	struct usb_request *in_req, *out_req;
};

static inline struct f_fastboot *func_to_fastboot(struct usb_function *f)
{
	return container_of(f, struct f_fastboot, usb_function);
}

static struct f_fastboot *fastboot_func;
static unsigned int download_size;
static unsigned int download_bytes;
static bool is_high_speed;

static struct usb_endpoint_descriptor fs_ep_in = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize     = TX_ENDPOINT_MAXIMUM_PACKET_SIZE,
	.bInterval          = 0x00,
};

static struct usb_endpoint_descriptor fs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1,
	.bInterval		= 0x00,
};

static struct usb_endpoint_descriptor hs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0,
	.bInterval		= 0x00,
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
};

static struct usb_descriptor_header *fb_runtime_descs[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&fs_ep_in,
	(struct usb_descriptor_header *)&hs_ep_out,
	NULL,
};

/*
 * static strings, in UTF-8
 */
static const char fastboot_name[] = "Android Fastboot";

static struct usb_string fastboot_string_defs[] = {
	[0].s = fastboot_name,
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_fastboot = {
	.language	= 0x0409,	/* en-us */
	.strings	= fastboot_string_defs,
};

static struct usb_gadget_strings *fastboot_strings[] = {
	&stringtab_fastboot,
	NULL,
};

#define BRILLO_MAX_SLOTS 4
struct gpt_partition_entries {
	char name[32];
	char type[32];
	lbaint_t size;
	uint8_t slots_num;
	char slots[BRILLO_MAX_SLOTS * 3];
};

__weak int fb_read_lock_state(uint8_t* lock_state)
{
	*lock_state = FB_STATE_UNLOCK;
	return 0;
}

__weak int fb_write_dev_key(uint8_t* devkey, unsigned int number_of_bytes)
{
	return -1;
}

__weak int fb_write_lock_state(uint8_t lock)
{
	return -1;
}

__weak const char* fb_get_product_name(void)
{
	return "";
}

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req);
static int strcmp_l1(const char *s1, const char *s2);

static void fastboot_complete(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;
	if (!status)
		return;
	printf("status: %d ep '%s' trans: %d\n", status, ep->name, req->actual);
}

static void fastboot_complete2(struct usb_ep *ep, struct usb_request *req)
{
	usb_ep_dequeue(ep, req);
	usb_ep_free_request(ep, req);
}

static int fastboot_bind(struct usb_configuration *c, struct usb_function *f)
{
	int id;
	struct usb_gadget *gadget = c->cdev->gadget;
	struct f_fastboot *f_fb = func_to_fastboot(f);
	const char *s;

	/* DYNAMIC interface numbers assignments */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	interface_desc.bInterfaceNumber = id;

	id = usb_string_id(c->cdev);
	if (id < 0)
		return id;
	fastboot_string_defs[0].id = id;
	interface_desc.iInterface = id;

	f_fb->in_ep = usb_ep_autoconfig(gadget, &fs_ep_in);
	if (!f_fb->in_ep)
		return -ENODEV;
	f_fb->in_ep->driver_data = c->cdev;

	f_fb->out_ep = usb_ep_autoconfig(gadget, &fs_ep_out);
	if (!f_fb->out_ep)
		return -ENODEV;
	f_fb->out_ep->driver_data = c->cdev;

	hs_ep_out.bEndpointAddress = fs_ep_out.bEndpointAddress;

	s = getenv("serial#");
	if (s)
		g_dnl_set_serialnumber((char *)s);

	return 0;
}

static void fastboot_unbind(struct usb_configuration *c, struct usb_function *f)
{
	memset(fastboot_func, 0, sizeof(*fastboot_func));
}

static void fastboot_disable(struct usb_function *f)
{
	struct f_fastboot *f_fb = func_to_fastboot(f);

	usb_ep_disable(f_fb->out_ep);
	usb_ep_disable(f_fb->in_ep);

	if (f_fb->out_req) {
		free(f_fb->out_req->buf);
		usb_ep_free_request(f_fb->out_ep, f_fb->out_req);
		f_fb->out_req = NULL;
	}
	if (f_fb->in_req) {
		free(f_fb->in_req->buf);
		usb_ep_free_request(f_fb->in_ep, f_fb->in_req);
		f_fb->in_req = NULL;
	}
}

static struct usb_request *fastboot_start_ep(struct usb_ep *ep)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, 0);
	if (!req)
		return NULL;

	req->length = EP_BUFFER_SIZE;
	req->buf = memalign(CONFIG_SYS_CACHELINE_SIZE, EP_BUFFER_SIZE);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	memset(req->buf, 0, req->length);
	return req;
}

static int fastboot_set_alt(struct usb_function *f,
			    unsigned interface, unsigned alt)
{
	int ret;
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_gadget *gadget = cdev->gadget;
	struct f_fastboot *f_fb = func_to_fastboot(f);

	debug("%s: func: %s intf: %d alt: %d\n",
	      __func__, f->name, interface, alt);

	/* make sure we don't enable the ep twice */
	if (gadget->speed == USB_SPEED_HIGH) {
		ret = usb_ep_enable(f_fb->out_ep, &hs_ep_out);
		is_high_speed = true;
	} else {
		ret = usb_ep_enable(f_fb->out_ep, &fs_ep_out);
		is_high_speed = false;
	}
	if (ret) {
		puts("failed to enable out ep\n");
		return ret;
	}

	f_fb->out_req = fastboot_start_ep(f_fb->out_ep);
	if (!f_fb->out_req) {
		puts("failed to alloc out req\n");
		ret = -EINVAL;
		goto err;
	}
	f_fb->out_req->complete = rx_handler_command;

	ret = usb_ep_enable(f_fb->in_ep, &fs_ep_in);
	if (ret) {
		puts("failed to enable in ep\n");
		goto err;
	}

	f_fb->in_req = fastboot_start_ep(f_fb->in_ep);
	if (!f_fb->in_req) {
		puts("failed alloc req in\n");
		ret = -EINVAL;
		goto err;
	}
	f_fb->in_req->complete = fastboot_complete;

	ret = usb_ep_queue(f_fb->out_ep, f_fb->out_req, 0);
	if (ret)
		goto err;

	return 0;
err:
	fastboot_disable(f);
	return ret;
}

static int fastboot_add(struct usb_configuration *c)
{
	struct f_fastboot *f_fb = fastboot_func;
	int status;

	debug("%s: cdev: 0x%p\n", __func__, c->cdev);

	if (!f_fb) {
		f_fb = memalign(CONFIG_SYS_CACHELINE_SIZE, sizeof(*f_fb));
		if (!f_fb)
			return -ENOMEM;

		fastboot_func = f_fb;
		memset(f_fb, 0, sizeof(*f_fb));
	}

	f_fb->usb_function.name = "f_fastboot";
	f_fb->usb_function.hs_descriptors = fb_runtime_descs;
	f_fb->usb_function.bind = fastboot_bind;
	f_fb->usb_function.unbind = fastboot_unbind;
	f_fb->usb_function.set_alt = fastboot_set_alt;
	f_fb->usb_function.disable = fastboot_disable;
	f_fb->usb_function.strings = fastboot_strings;

	status = usb_add_function(c, &f_fb->usb_function);
	if (status) {
		free(f_fb);
		fastboot_func = f_fb;
	}

	return status;
}
DECLARE_GADGET_BIND_CALLBACK(usb_dnl_fastboot, fastboot_add);

static int fastboot_tx_write(const char *buffer, unsigned int buffer_size)
{
	struct usb_request *in_req = fastboot_func->in_req;
	int ret;

	memcpy(in_req->buf, buffer, buffer_size);
	in_req->length = buffer_size;

	usb_ep_dequeue(fastboot_func->in_ep, in_req);

	ret = usb_ep_queue(fastboot_func->in_ep, in_req, 0);
	if (ret)
		printf("Error %d on queue\n", ret);
	return 0;
}

static int fastboot_tx_write_str(const char *buffer)
{
	return fastboot_tx_write(buffer, strlen(buffer));
}

static int fastboot_tx_write2(struct usb_request *req, const char *buffer, unsigned int buffer_size)
{
	int ret;

	memcpy(req->buf, buffer, buffer_size);
	req->length = buffer_size;

	usb_ep_dequeue(fastboot_func->in_ep, req);

	ret = usb_ep_queue(fastboot_func->in_ep, req, 0);
	if (ret)
		printf("Error %d on queue\n", ret);
	return 0;
}

static int fastboot_tx_write_str2(struct usb_request *req, const char *buffer)
{
	return fastboot_tx_write2(req, buffer, strlen(buffer));
}

static void compl_do_reset(struct usb_ep *ep, struct usb_request *req)
{
	do_reset(NULL, 0, 0, NULL);
}

int __weak fb_set_reboot_flag(void)
{
	return -ENOSYS;
}

static void cb_reboot(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	if (!strcmp_l1("reboot-bootloader", cmd)) {
		if (fb_set_reboot_flag()) {
			fastboot_tx_write_str("FAILCannot set reboot flag");
			return;
		}
	}
	fastboot_func->in_req->complete = compl_do_reset;
	fastboot_tx_write_str("OKAY");
}

static int strcmp_l1(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;
	return strncmp(s1, s2, strlen(s1));
}

static void var_partition_type(const char *part, char *response)
{
	block_dev_desc_t *dev = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	disk_partition_t info;

	if (!dev) {
		strcpy(response, "FAILfailed to read mmc");
		return;
	}

	if (get_partition_info_efi_by_name(dev, part, &info)) {
		strcpy(response, "FAILpartition not found");
		return;
	}

	snprintf(response, RESPONSE_LEN, "OKAY%s", info.type);
}

static void var_partition_size(const char *part, char *response)
{
	block_dev_desc_t *dev = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	disk_partition_t info;

	if (!dev) {
		fastboot_tx_write_str("FAILfailed to read mmc");
		return;
	}

	if (get_partition_info_efi_by_name(dev, part, &info)) {
		fastboot_tx_write_str("FAILpartition not found");
		return;
	}

	snprintf(response, RESPONSE_LEN, "OKAY0x%016llx", info.size * info.blksz);
}


struct get_var_command_t {
	char command_name[20];
	void (*get_var)(char *response, char *cmd, size_t chars_left);
};


static void getvar_version(char *response, char *cmd, size_t chars_left) {
	strncat(response, FASTBOOT_VERSION, chars_left);
}

static void getvar_bootloader_version(char *response, char *cmd, size_t chars_left) {
	strncat(response, U_BOOT_VERSION, chars_left);
}

static void getvar_download_size(char *response, char *cmd, size_t chars_left) {
	char str_num[12];
	sprintf(str_num, "0x%08x", CONFIG_FASTBOOT_BUF_SIZE);
	strncat(response, str_num, chars_left);
}

static void getvar_serialno(char *response, char *cmd, size_t chars_left) {
	char *s;
	s = getenv("serial#");
	if (s)
		strncat(response, s, chars_left);
	else
		strcpy(response, "FAILValue not set");
}

static void getvar_product(char *response, char *cmd, size_t chars_left) {
	strncat(response, fb_get_product_name(), chars_left);
}

static void getvar_partition_type(char *response, char *cmd, size_t chars_left) {
	var_partition_type(cmd + 15, response);
}

static void getvar_partition_size(char *response, char *cmd, size_t chars_left) {
	var_partition_size(cmd + 15, response);
}

static void getvar_has_slot(char *response, char *cmd, size_t chars_left) {
	strsep(&cmd, ":");
	if(!strcmp("boot", cmd) || !strcmp("system", cmd) || !strcmp("oem", cmd)) {
		strncat(response, "yes", chars_left);
	} else {
		strncat(response, "no", chars_left);
	}
}

static void getvar_current_slot(char *response, char *cmd, size_t chars_left) {
	char* current_slot = get_active_slot();
	strncat(response, current_slot, chars_left);
}
static void getvar_slot_suffixes(char *response, char *cmd, size_t chars_left) {
	strsep(&cmd, ":");
	snprintf(response, RESPONSE_LEN, "OKAY_a,_b");
}
static void getvar_slot_successful(char *response, char *cmd, size_t chars_left) {
	strsep(&cmd, ":");
	if(is_successful_slot(cmd))
		strncat(response, "yes", chars_left);
	else
		strncat(response, "no", chars_left);
}
static void getvar_slot_unbootable(char *response, char *cmd, size_t chars_left) {
	strsep(&cmd, ":");
	if(is_unbootable_slot(cmd)) {
		strncat(response, "yes", chars_left);
	} else {
		strncat(response, "no", chars_left);
	}
}
static void getvar_slot_retry_count(char *response, char *cmd, size_t chars_left) {
	strsep(&cmd, ":");
	int ret = get_slot_retry_count(cmd);
	if(ret >= 0){
		snprintf(response, RESPONSE_LEN, "OKAY%d", ret);
	} else {
		strcpy(response, "FAILUnable to get slot-retry-count");
	}
}

static void __create_request(char *response)
{
	struct usb_request *req_slot = fastboot_start_ep(fastboot_func->in_ep);
	req_slot->complete = fastboot_complete2;
	fastboot_tx_write_str2(req_slot, response);
}

static int __find_update_part_entry(int list_size, const char *name, size_t namelen,
				  struct gpt_partition_entries *list_of_partitions)
{
	int j;
	if (namelen <= 2) /* X_a */
		return 0;

	for (j = 0; j < list_size; j++) {
		if (strncmp(list_of_partitions[j].name, name, namelen - 2) == 0) {
			snprintf(list_of_partitions[j].slots, sizeof(list_of_partitions[j].slots),
				 "%s _%c", list_of_partitions[j].slots, name[namelen - 1]);

			list_of_partitions[j].slots_num++;
			return 1;
		}
	}
	return 0;
}

static void getvar_all(char *response, char *cmd, size_t chars_left)
{
	int i;
	int list_size = 0;
	uint8_t state = FB_STATE_UNLOCK; /* default is unlocked */
	size_t name_length = 0;

	struct gpt_partition_entries list_of_partitions[GPT_ENTRY_NUMBERS];

	disk_partition_t info;
	block_dev_desc_t *dev = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);

	if (!dev) {
		strcpy(response, "FAILfailed to read mmc");
		return;
	}

	for (i = 1; i < GPT_ENTRY_NUMBERS; i++) {
		if (get_partition_info_efi(dev, i, &info)) {
			/* no more entries in table */
			break;
		}

		name_length = strlen((char*)info.name);
		/* We assume that the suffix will be at the end with length 2, ex "*_a" */
		if (name_length > 2 && (info.name[name_length - 2] == '_')) {
			if (!__find_update_part_entry(list_size, (char*)info.name,
						name_length, list_of_partitions)) {
				struct gpt_partition_entries entry;

				snprintf(entry.type, sizeof(entry.type), "%s", info.type);
				snprintf(entry.name, name_length + 1 - 2, "%s", info.name);
				entry.size = info.size * info.blksz;
				entry.slots_num = 1;

				snprintf(entry.slots, sizeof(entry.slots), "_%c", info.name[name_length - 1]);

				list_of_partitions[list_size] = entry;
				list_size++;
			}
		} else {
			struct gpt_partition_entries entry;

			snprintf(entry.type, sizeof(entry.type), "%s", info.type);
			snprintf(entry.name, sizeof(entry.name), "%s", info.name);
			entry.size = info.size * info.blksz;
			entry.slots_num = 0;
			list_of_partitions[list_size] = entry;

			list_size++;
		}
	}

	for (i = 0; i < list_size; i++) {
		if (list_of_partitions[i].slots_num > 0) {
			snprintf(response, RESPONSE_LEN, "INFOpartition \"%s\": has %d slots \"%s\"",
				 list_of_partitions[i].name,
				 list_of_partitions[i].slots_num,
				 list_of_partitions[i].slots);
		} else {
			snprintf(response, RESPONSE_LEN, "INFOpartition \"%s\": has no slots",
				 list_of_partitions[i].name);
		}
		__create_request(response);

		snprintf(response, RESPONSE_LEN, "INFOpartition \"%s\": type %s",
			 list_of_partitions[i].name, list_of_partitions[i].type);
		__create_request(response);

		snprintf(response, RESPONSE_LEN, "INFOpartition \"%s\": size 0x%016llx",
			 list_of_partitions[i].name, list_of_partitions[i].size);
		__create_request(response);
	}

	snprintf(response, RESPONSE_LEN, "INFOversion: %s", FASTBOOT_VERSION);
	__create_request(response);

	snprintf(response, RESPONSE_LEN, "INFOversion-bootloader: %s", U_BOOT_VERSION);
	__create_request(response);

	snprintf(response, RESPONSE_LEN, "INFOdownloadsize: 0x%08x", CONFIG_FASTBOOT_BUF_SIZE);
	__create_request(response);

	snprintf(response, RESPONSE_LEN, "INFOmax-download-size: 0x%08x", CONFIG_FASTBOOT_BUF_SIZE);
	__create_request(response);

	snprintf(response, RESPONSE_LEN, "INFOserialno: %s", getenv("serial#"));
	__create_request(response);

	snprintf(response, RESPONSE_LEN, "INFOproduct: %s", fb_get_product_name());
	__create_request(response);

	fb_read_lock_state(&state);

	snprintf(response, RESPONSE_LEN, "INFOdevice is %s", state == 0 ? "locked" : "unlocked");
	__create_request(response);

	__create_request("OKAY");
}

static struct get_var_command_t list_of_commands[] = {
	{
		"version",
		getvar_version,
	},
	{
		"version-bootloader",
		getvar_bootloader_version,
	},
	{
		"downloadsize",
		getvar_download_size,
	},
	{
		"max-download-size",
		getvar_download_size,
	},
	{
		"serialno",
		getvar_serialno,
	},
	{
		"product",
		getvar_product,
	},
	{
		"partition-type",
		getvar_partition_type,
	},
	{
		"partition-size",
		getvar_partition_size,
	},
	{
		"has-slot",
		getvar_has_slot,
	},
	{
		"slot-retry-count",
		getvar_slot_retry_count,
	},
	{
		"current-slot",
		getvar_current_slot,
	},
	{
		"slot-suffixes",
		getvar_slot_suffixes,
	},
	{
		"slot-successful",
		getvar_slot_successful,
	},
	{
		"slot-unbootable",
		getvar_slot_unbootable,
	},
	{
		"all",
		getvar_all,
	},
};


static void cb_getvar(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];
	size_t chars_left;
	int i;
	struct get_var_command_t* command = NULL;

	strcpy(response, "OKAY");
	chars_left = sizeof(response) - strlen(response) - 1;

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing variable\n");
		fastboot_tx_write_str("FAILmissing var");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(list_of_commands); i++) {
		if (!strcmp_l1(list_of_commands[i].command_name, cmd)) {
			if (list_of_commands[i].get_var) {
				command = &list_of_commands[i];
				break;
			}
		}
	}

	if (command != NULL) {
		command->get_var(response, cmd, chars_left);
		if(!strcmp_l1(command->command_name, "all"))
			return;
	} else {
		error("unknown variable: %s\n", cmd);
		strcpy(response, "FAILVariable not implemented");
	}

	fastboot_tx_write_str(response);
}

static unsigned int rx_bytes_expected(unsigned int maxpacket)
{
	int rx_remain = download_size - download_bytes;
	int rem = 0;
	if (rx_remain < 0)
		return 0;
	if (rx_remain > EP_BUFFER_SIZE)
		return EP_BUFFER_SIZE;
	if (rx_remain < maxpacket) {
		rx_remain = maxpacket;
	} else if (rx_remain % maxpacket != 0) {
		rem = rx_remain % maxpacket;
		rx_remain = rx_remain + (maxpacket - rem);
	}
	return rx_remain;
}

#define BYTES_PER_DOT	0x20000
static void rx_handler_dl_image(struct usb_ep *ep, struct usb_request *req)
{
	char response[RESPONSE_LEN];
	unsigned int transfer_size = download_size - download_bytes;
	const unsigned char *buffer = req->buf;
	unsigned int buffer_size = req->actual;
	unsigned int pre_dot_num, now_dot_num;
	unsigned int max;

	if (req->status != 0) {
		printf("Bad status: %d\n", req->status);
		return;
	}

	if (buffer_size < transfer_size)
		transfer_size = buffer_size;

	memcpy((void *)CONFIG_FASTBOOT_BUF_ADDR + download_bytes,
	       buffer, transfer_size);

	pre_dot_num = download_bytes / BYTES_PER_DOT;
	download_bytes += transfer_size;
	now_dot_num = download_bytes / BYTES_PER_DOT;

	if (pre_dot_num != now_dot_num) {
		putc('.');
		if (!(now_dot_num % 74))
			putc('\n');
	}

	/* Check if transfer is done */
	if (download_bytes >= download_size) {
		/*
		 * Reset global transfer variable, keep download_bytes because
		 * it will be used in the next possible flashing command
		 */
		download_size = 0;
		req->complete = rx_handler_command;
		req->length = EP_BUFFER_SIZE;

		sprintf(response, "OKAY");
		fastboot_tx_write_str(response);

		printf("\ndownloading of %d bytes finished\n", download_bytes);
	} else {
		max = is_high_speed ? hs_ep_out.wMaxPacketSize :
				fs_ep_out.wMaxPacketSize;
		req->length = rx_bytes_expected(max);
		if (req->length < ep->maxpacket)
			req->length = ep->maxpacket;
	}

	req->actual = 0;
	usb_ep_queue(ep, req, 0);
}

static void cb_download(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];
	unsigned int max;

	strsep(&cmd, ":");
	download_size = simple_strtoul(cmd, NULL, 16);
	download_bytes = 0;

	printf("Starting download of %d bytes\n", download_size);

	if (0 == download_size) {
		sprintf(response, "FAILdata invalid size");
	} else if (download_size > CONFIG_FASTBOOT_BUF_SIZE) {
		download_size = 0;
		sprintf(response, "FAILdata too large");
	} else {
		sprintf(response, "DATA%08x", download_size);
		req->complete = rx_handler_dl_image;
		max = is_high_speed ? hs_ep_out.wMaxPacketSize :
			fs_ep_out.wMaxPacketSize;
		req->length = rx_bytes_expected(max);
		if (req->length < ep->maxpacket)
			req->length = ep->maxpacket;
	}
	fastboot_tx_write_str(response);
}

static void do_bootm_on_complete(struct usb_ep *ep, struct usb_request *req)
{
	char boot_addr_start[12];
	char *bootm_args[] = { "bootm", boot_addr_start, NULL };

	puts("Booting kernel..\n");

	sprintf(boot_addr_start, "0x%lx", load_addr);
	do_bootm(NULL, 0, 2, bootm_args);

	/* This only happens if image is somehow faulty so we start over */
	do_reset(NULL, 0, 0, NULL);
}

static void cb_boot(struct usb_ep *ep, struct usb_request *req)
{
	fastboot_func->in_req->complete = do_bootm_on_complete;
	fastboot_tx_write_str("OKAY");
}

static void do_exit_on_complete(struct usb_ep *ep, struct usb_request *req)
{
	g_dnl_trigger_detach();
}

static void cb_continue(struct usb_ep *ep, struct usb_request *req)
{
	fastboot_func->in_req->complete = do_exit_on_complete;
	fastboot_tx_write_str("OKAY");
}

#ifdef CONFIG_FASTBOOT_FLASH
#define CONFIG_FASTBOOT_BVB_DEVKEY "bvb_devkey"

static void cb_flash(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];
	uint8_t state = FB_STATE_UNLOCK; // default is unlocked

	fb_read_lock_state(&state);

	if (state == FB_STATE_LOCK) {
		fastboot_tx_write_str("FAILDevice is locked");
		return;
	}

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing partition name\n");
		fastboot_tx_write_str("FAILmissing partition name");
		return;
	}
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	if (strcmp(cmd, CONFIG_FASTBOOT_BVB_DEVKEY) == 0) {
		if (fb_write_dev_key((uint8_t *)CONFIG_FASTBOOT_BUF_ADDR, download_bytes) == 0)
			fastboot_tx_write_str("OKAY");
		else
			fastboot_tx_write_str("FAILUnable to flash dev key");
		return;
	}

	strcpy(response, "FAILno flash device defined");
	fb_mmc_flash_write(cmd, (void *)CONFIG_FASTBOOT_BUF_ADDR,
			   download_bytes, response);
#endif /* CONFIG_FASTBOOT_FLASH_MMC_DEV */
	fastboot_tx_write_str(response);
}
#endif

static void cb_set_active(struct usb_ep *ep, struct usb_request *req)
{
	unsigned long slot;
	char *cmd = req->buf;
	strsep(&cmd, ":");
	if(strncmp("_a", cmd, 2) == 0) {
		slot = 0;
	} else if(strncmp("_b", cmd, 2) == 0) {
		slot = 1;
	} else {
		fastboot_tx_write_str("FAILinvalid slot number");
		return;
	}

	if (ab_set_active(slot)) {
		fastboot_tx_write_str("FAILcouldn't set_active");
		return;
	}

	fastboot_tx_write_str("OKAY");
}

__weak const char* fb_get_wipe_userdata_message(void)
{
	return "Default implementation does not erase userdata";
}

__weak bool fb_get_wipe_userdata_response(void)
{
	return false;
}

static void fastboot_complete_with_user_action(struct usb_ep * ep, struct usb_request *req)
{
	char response[RESPONSE_LEN];
	uint8_t state = FB_STATE_UNKNOWN;

	usb_ep_dequeue(ep, req);
	usb_ep_free_request(ep, req);

	if (!fb_get_wipe_userdata_response()) {
		error("cannot change device locking state without erasing userdata\n");
		fastboot_tx_write_str("FAILmust erase userdata before changing device state");
		return;
	}

	printf("Wiping userdata...\n");
	response[0] = 0; /* clear response buffer */
	fb_mmc_erase("userdata", response);
	if (strncmp(response, "OKAY", 4)) {
		fastboot_tx_write_str(response);
		return;
	}

	fb_read_lock_state(&state);
	state = state == FB_STATE_LOCK ? FB_STATE_UNLOCK : FB_STATE_LOCK;

	if (fb_write_lock_state(state)) {
		snprintf(response, RESPONSE_LEN, "FAILCannot %slock device",
		         state == FB_STATE_UNLOCK ? "un" : "");
	} else {
		strcpy(response, "OKAY");
	}

	fastboot_tx_write_str(response);
}

static void cb_flashing(struct usb_ep *ep, struct usb_request *req)
{
	uint8_t curr_state = FB_STATE_UNKNOWN, new_state;
	char *cmd = req->buf;
	char response[RESPONSE_LEN];
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	struct usb_request *req1, *req2;
	const char* user_msg;
#endif
	strsep(&cmd, " ");

	if (!cmd) {
		error("missing lock state\n");
		fastboot_tx_write_str("FAILmissing lock state");
		return;
	}

	fb_read_lock_state(&curr_state);

	if (!strcmp_l1("lock", cmd)) {
		new_state = FB_STATE_LOCK;
	} else if (!strcmp_l1("unlock", cmd)) {
		new_state = FB_STATE_UNLOCK;
	} else {
		fastboot_tx_write_str("FAILflashing state unknown");
		return;
	}

	if (curr_state == new_state) {
		fastboot_tx_write_str("OKAY");
		return;
	}

#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	printf("Changing device to this state requires wiping userdata\n");
	snprintf(response, RESPONSE_LEN, "INFOChanging device to this state requires wiping userdata");
	req1 = fastboot_start_ep(fastboot_func->in_ep);
	req1->complete = fastboot_complete2;
	fastboot_tx_write_str2(req1, response);


	user_msg = fb_get_wipe_userdata_message();
	printf("%s\n", user_msg);
	snprintf(response, RESPONSE_LEN, "INFO%s", user_msg);
	req2 = fastboot_start_ep(fastboot_func->in_ep);
	req2->complete = fastboot_complete_with_user_action;
	fastboot_tx_write_str2(req2, response);
#else
	if (fb_write_lock_state(new_state)) {
		snprintf(response, RESPONSE_LEN, "FAILCannot %slock device",
		         new_state == FB_STATE_UNLOCK ? "un" : "");
	} else {
		strcpy(response, "OKAY");
	}

	fastboot_tx_write_str(response);
#endif
}

static void cb_oem(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	if (strncmp("set_active", cmd + 4, 10) == 0) {
		req->buf += 4;
		cb_set_active(ep, req);
		req->buf -= 4;
	}
	else if (strncmp("unlock", cmd + 4, 8) == 0) {
		fastboot_tx_write_str("FAILnot implemented");
	}
	else {
		fastboot_tx_write_str("FAILunknown oem command");
	}
}

#ifdef CONFIG_FASTBOOT_FLASH
static void cb_erase(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing partition name");
		fastboot_tx_write_str("FAILmissing partition name");
		return;
	}

	strcpy(response, "FAILno flash device defined");

#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	fb_mmc_erase(cmd, response);
#endif
	fastboot_tx_write_str(response);
}
#endif

struct cmd_dispatch_info {
	char *cmd;
	void (*cb)(struct usb_ep *ep, struct usb_request *req);
};

static const struct cmd_dispatch_info cmd_dispatch_info[] = {
	{
		.cmd = "reboot",
		.cb = cb_reboot,
	}, {
		.cmd = "getvar:",
		.cb = cb_getvar,
	}, {
		.cmd = "download:",
		.cb = cb_download,
	}, {
		.cmd = "boot",
		.cb = cb_boot,
	}, {
		.cmd = "continue",
		.cb = cb_continue,
	},
#ifdef CONFIG_FASTBOOT_FLASH
	{
		.cmd = "flashing",
		.cb = cb_flashing,
	},
	{
		.cmd = "flash",
		.cb = cb_flash,
	}, {
		.cmd = "erase",
		.cb = cb_erase,
	},
#endif
	{
		.cmd = "oem",
		.cb = cb_oem,
	},
	{
		.cmd = "set_active",
		.cb = cb_set_active,
	},
};

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req)
{
	char *cmdbuf = req->buf;
	void (*func_cb)(struct usb_ep *ep, struct usb_request *req) = NULL;
	int i;

	if (req->status != 0 || req->length == 0)
		return;

	for (i = 0; i < ARRAY_SIZE(cmd_dispatch_info); i++) {
		if (!strcmp_l1(cmd_dispatch_info[i].cmd, cmdbuf)) {
			func_cb = cmd_dispatch_info[i].cb;
			break;
		}
	}

	if (!func_cb) {
		error("unknown command: %s\n", cmdbuf);
		fastboot_tx_write_str("FAILunknown command");
	} else {
		if (req->actual < req->length) {
			u8 *buf = (u8 *)req->buf;
			buf[req->actual] = 0;
			func_cb(ep, req);
		} else {
			error("buffer overflow\n");
			fastboot_tx_write_str("FAILbuffer overflow");
		}
	}

	*cmdbuf = '\0';
	req->actual = 0;
	usb_ep_queue(ep, req, 0);
}
