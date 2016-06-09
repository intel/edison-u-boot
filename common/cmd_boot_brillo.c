/*
 * Copyright (c) 2015 Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * * Neither the name of the Intel Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * INTEL OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * EXPORT LAWS: THIS LICENSE ADDS NO RESTRICTIONS TO THE EXPORT LAWS
 * OF YOUR JURISDICTION. It is licensee's responsibility to comply
 * with any export regulations applicable in licensee's
 * jurisdiction. Under CURRENT (May 2000) U.S. export regulations this
 * software is eligible for export from the U.S. and can be downloaded
 * by or otherwise exported or reexported worldwide EXCEPT to
 * U.S. embargoed destinations which include Cuba, Iraq, Libya, North
 * Korea, Iran, Syria, Sudan, Afghanistan and any other country to
 * which the U.S. has embargoed goods and services.
 */

/*
 * SPDX-License-Identifier:	Intel
 */

#include <common.h>
#include <asm/bootm.h>
#include <asm/zimage.h>
#include <android_image.h>
#include <command.h>
#include <errno.h>
#include <memalign.h>
#include <android_bootloader.h>
#include <cmd_boot_brillo.h>
#include <mmc.h>
#include <fb_mmc.h>
#include "bvb/bvb_refimpl.h"

#define RESPONSE_LEN (64 + 1)

#define BOOT_SIGNATURE_MAX_SIZE 4096
#define BOOT_MAX_IMAGE_SIZE (32 * 1024 * 1024) /* 32 MiB */

#ifndef CONFIG_BRILLO_MMC_BOOT_DEVICE
#define CONFIG_BRILLO_MMC_BOOT_DEVICE CONFIG_FASTBOOT_FLASH_MMC_DEV
#endif

#define BOOT_ARG_SLOT_SUFFIX_STR "androidboot.slot_suffix="
#define BOOT_ARG_DEVICE_STATE_STR "androidboot.bvb.device_state="
#define BOOT_ARG_SLOT_CHOSEN_STR "androidboot.bvb.slot_chosen="
#define BOOT_ARG_FALLBACK_REASON_STR "androidboot.bvb.fallback_reason="

#define BOOTCTRL_SUFFIX_A           "_a"
#define BOOTCTRL_SUFFIX_B           "_b"
#define BOOTCTRL_SUFFIX_NA			"no suffix available"

#define BOOT_CONTROL_VERSION    1

#define BOOT_STATE_VAR		L"BootState"
#define BOOT_STATE_GREEN	0
#define BOOT_STATE_YELLOW	1
#define BOOT_STATE_ORANGE	2
#define BOOT_STATE_RED		3

struct slot_metadata {
	uint8_t priority : 4;
	uint8_t tries_remaining : 3;
	uint8_t successful_boot : 1;
} __attribute__((packed));

#define BOOT_CTRL_MAGIC 0x42414342

struct boot_ctrl {
	/* NUL terminated active slot suffix. */
	char slot_suffix[4];

	/* Magic for identification - '\0ABB' (Boot Contrl Magic) */
	uint32_t magic;

	/* Version of struct. */
	uint8_t version;

	/* Number of slots being managed. */
	uint8_t nb_slot : 3;

	/* Number of times left attempting to boot recovery. */
	uint8_t recovery_tries_remaining : 3;

	/* Ensure 4-bytes alignment for slot_info field. */
	uint8_t reserved0[2];

	/*  Per-slot information.  Up to 4 slots. */
	struct slot_metadata slot_info[4];

	/* Reserved for further use. */
	uint8_t reserved1[8];

	/* CRC32 of all 28 bytes preceding this field (little endian  format). */
	uint32_t crc32_le;
} __attribute__((packed));

extern bool fb_get_wipe_userdata_response(void);
int fb_read_lock_state(uint8_t* lock_state);

/* To be overridden by whatever the board's implementation is of
 * 'adb reboot fastboot' and the like.
 */
__weak char* board_get_reboot_target(void) { return NULL; }

/* Properly initialise the metadata partition */
static void ab_init_default_metadata(struct boot_ctrl *ctrl)
{
	ctrl->magic = BOOT_CTRL_MAGIC;
	ctrl->version = 1;

	ctrl->slot_info[0].priority = 0;
	ctrl->slot_info[0].tries_remaining = 0;
	ctrl->slot_info[0].successful_boot = 0;

	ctrl->slot_info[1].priority = 0;
	ctrl->slot_info[1].tries_remaining = 0;
	ctrl->slot_info[1].successful_boot = 0;

	ctrl->recovery_tries_remaining = 0;
}

static int ab_read_bootloader_message(block_dev_desc_t *dev,
		disk_partition_t *misc_part, struct bootloader_message_ab *msg)
{
	void *tmp;
	lbaint_t blkcnt;
	struct boot_ctrl *ctrl = (struct boot_ctrl*)&msg->slot_suffix;

	blkcnt = BLOCK_CNT(sizeof(*msg), dev);
	tmp = calloc(blkcnt, dev->blksz);

	if (!tmp)
		return -ENOMEM;

	if (dev->block_read(dev->dev, misc_part->start, blkcnt, tmp)
			!= blkcnt) {
		free(tmp);
		return -EIO;
	}

	memcpy(msg, tmp, sizeof(*msg));
	free(tmp);

	/* Check if the metadata is correct. */
	if (ctrl->magic != BOOT_CTRL_MAGIC) {
		printf("WARNING: A/B Selector Metadata is not initialised"
		       " or corrupted, using defaults\n");
		ab_init_default_metadata(ctrl);
	}

	return 0;
}

static int ab_write_bootloader_message(block_dev_desc_t *dev,
		disk_partition_t *misc_part, struct bootloader_message_ab *msg)
{
	void *tmp;
	lbaint_t blkcnt, i;

	blkcnt = BLOCK_CNT(sizeof(*msg), dev);
	tmp = calloc(blkcnt, dev->blksz);

	if (!tmp)
		return -ENOMEM;

	memcpy(tmp, msg, sizeof(*msg));

	for (i = 0; i < blkcnt; i++) {
		if (dev->block_write(dev->dev, misc_part->start + i, 1,
				tmp + i * dev->blksz) != 1) {
			free(tmp);
			return -EIO;
		}
	}

	free(tmp);
	return 0;
}

int ab_set_active(int slot_num)
{
	struct bootloader_message_ab message;
	struct boot_ctrl *metadata = (struct boot_ctrl*)&message.slot_suffix;
	struct slot_metadata *slot = &metadata->slot_info[slot_num];
	struct slot_metadata *other_slot = &metadata->slot_info[slot_num ^ 1];
	block_dev_desc_t *dev;
	disk_partition_t misc_part;
	int ret;

	if (slot_num < 0 || slot_num > 1)
		return -EINVAL;

	if (!(dev = get_dev("mmc", CONFIG_BRILLO_MMC_BOOT_DEVICE)))
		return -ENODEV;

	if (get_partition_info_efi_by_name(dev, "misc", &misc_part))
		return -ENOENT;

	if ((ret = ab_read_bootloader_message(dev, &misc_part, &message)))
		return ret;

	slot->priority = 15;
	slot->tries_remaining = 7;
	slot->successful_boot = 0;
	if (other_slot->priority == 15)
		other_slot->priority = 14;

	if ((ret = ab_write_bootloader_message(dev, &misc_part, &message)))
		return ret;

	return 0;
}

/* Loads an android boot image from the given device and named
 * partition into CONFIG_SYS_LOAD_ADDR.
 */
static int load_boot_image(block_dev_desc_t *dev, const char *part_name)
{
	BvbBootImageHeader *hdr = (struct BvbBootImageHeader*)CONFIG_SYS_LOAD_ADDR;
	BvbBootImageHeader *h = NULL;
	void *rest;
	int boot_state;

	uint8_t sec_flag = 0;

	BvbVerifyResult verify_result = BVB_VERIFY_RESULT_OK_NOT_SIGNED;

	disk_partition_t boot_part;
	lbaint_t hdr_blkcnt;
	lbaint_t kernel_block_start;
	lbaint_t kernel_block_count;
	lbaint_t initrd_block_count;

	if (get_partition_info_efi_by_name(dev, part_name, &boot_part))
		return -ENOENT;

	hdr_blkcnt = BLOCK_CNT(sizeof(*hdr), dev);

	verify_result = bvb_verify_boot_image(dev, boot_part);

	if (dev->block_read(dev->dev, boot_part.start, hdr_blkcnt,
			(void*)CONFIG_SYS_LOAD_ADDR) != hdr_blkcnt) {
		return -EIO;
	}

	h = bvb_malloc(sizeof(BvbBootImageHeader));
	if (h == NULL) {
		printf("Error allocating byteswapped header.\n");
	}

	bvb_boot_image_header_to_host_byte_order(hdr, h);

	rest = ((void*)hdr) + (hdr_blkcnt * dev->blksz);

	kernel_block_start = hdr_blkcnt + BLOCK_CNT(h->authentication_data_block_size +
						    h->auxilary_data_block_size +
						    h->kernel_offset, dev);

	kernel_block_count = BLOCK_CNT(h->kernel_size, dev);
	initrd_block_count = BLOCK_CNT(h->initrd_size, dev);

	if (fb_read_lock_state(&sec_flag)) {
		sec_flag = 0;
	}


	switch (sec_flag) {
		case 1:	/* lock state */
			printf("Device is locked.\n");
			if(!verify_result)
				boot_state = BOOT_STATE_GREEN;
			else
				boot_state = BOOT_STATE_RED;
			break;
		default:
			printf("Device is unlocked.\n");
			boot_state = BOOT_STATE_ORANGE;
			break;
	}

	switch (boot_state) {
		case BOOT_STATE_RED:
			printf("BOOT_STATE_RED\n");
			break;
		case BOOT_STATE_GREEN:
			printf("BOOT_STATE_GREEN\n");
			break;
		case BOOT_STATE_ORANGE:
			printf("BOOT_STATE_ORANGE\n");
			break;
		case BOOT_STATE_YELLOW:
			printf("BOOT_STATE_YELLOW\n");
			break;
		default:
			printf("BOOT_STATE_RED\n");
			break;
	}

	if (dev->block_read(dev->dev, boot_part.start + kernel_block_start,
			kernel_block_count + initrd_block_count, rest) != kernel_block_count + initrd_block_count)
		return -EIO;

	if(h != NULL)
		bvb_free(h);

	return 0;
}

static void append_to_bootargs(const char *str) {
	char *bootargs = getenv("bootargs");
	if (!bootargs) {
		if (*str == ' ') str++;
		setenv("bootargs", str);
		return;
	}
	int newlen = strlen(str) + strlen(bootargs) + 1;
	char *newbootargs = malloc(newlen);
	snprintf(newbootargs, newlen, "%s%s", bootargs, str);
	setenv("bootargs", newbootargs);
	free(newbootargs);
}

/* Boots to the image already successfully loaded at
 * CONFIG_SYS_LOAD_ADDR. If the function returns then booting has
 * failed.
 */
static void boot_image(void)
{
	struct BvbBootImageHeader *h = (struct BvbBootImageHeader*)CONFIG_SYS_LOAD_ADDR;
	BvbBootImageHeader *hdr = NULL;

	ulong load_address;
	char *bootargs = getenv("bootargs");
	hdr = bvb_malloc(sizeof(BvbBootImageHeader));
	if (hdr == NULL) {
		printf("Error allocating byteswapped header.\n");
	}
	bvb_boot_image_header_to_host_byte_order((const BvbBootImageHeader *) h, hdr);

	if (bootargs) {
		bootargs = strdup(bootargs);
		setenv("bootargs", (const char*)hdr->kernel_cmdline);
		append_to_bootargs(" ");
		append_to_bootargs(bootargs);
		free(bootargs);
	} else
		setenv("bootargs", (const char*)hdr->kernel_cmdline);

	struct boot_params *params = load_zimage((void*)CONFIG_SYS_LOAD_ADDR + sizeof(*hdr),
						 hdr->kernel_size,
						 &load_address);
	setup_zimage(params, ((void*)params) + 0x9000, 0,
		     (ulong)(((void*)h) + sizeof(*hdr) + hdr->kernel_size),
		     hdr->initrd_size);
	boot_linux_kernel((ulong)params, load_address, false);
}

static void boot_recovery_image(block_dev_desc_t *dev, struct boot_ctrl *metadata,
		disk_partition_t *misc_part, struct bootloader_message_ab *message)
{
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	char response[RESPONSE_LEN];

	printf("Entering recovery mode requires erasing userdata\n");
	printf("Press RM button for YES or FW button for NO\n");
	if (!fb_get_wipe_userdata_response()) {
		printf("Cannot enter recovery mode without erasing userdata\n");
		return;
	}

	printf("Wiping userdata...\n");
	response[0] = 0; /* clear response buffer */
	fb_mmc_erase("userdata", response);
	if (strncmp(response, "OKAY", 4)) {
		printf("Error while erasing userdata\n");
		return;
	}
#endif
	if (metadata->recovery_tries_remaining > 0) {
		metadata->recovery_tries_remaining--;
		if (load_boot_image(dev, "recovery")) {
			/* Failed to load, set remaining tries to zero */
			metadata->recovery_tries_remaining = 0;
			ab_write_bootloader_message(dev, misc_part, message);
			return;
		}
		ab_write_bootloader_message(dev, misc_part, message);
		boot_image();
		/* Failed to boot, set remaining tries to zero */
		metadata->recovery_tries_remaining = 0;
		ab_write_bootloader_message(dev, misc_part, message);
	}
}

static void brillo_do_recovery(void)
{
	struct bootloader_message_ab message;
	struct boot_ctrl *metadata = (struct boot_ctrl*)&message.slot_suffix;
	struct slot_metadata *slot;
	block_dev_desc_t *dev;
	disk_partition_t misc_part;
	int index;

	if (!(dev = get_dev("mmc", CONFIG_BRILLO_MMC_BOOT_DEVICE)))
		return;

	if (get_partition_info_efi_by_name(dev, "misc", &misc_part))
		return;

	if (ab_read_bootloader_message(dev, &misc_part, &message))
		return;

	/* Slots need to be marked as failed before loading recovery mode */
	for (index = 0; index < ARRAY_SIZE(metadata->slot_info); index++) {
		slot = &metadata->slot_info[index];
		slot->successful_boot = 0;
		slot->tries_remaining = 0;
		slot->priority = 0;
	}

	boot_recovery_image(dev, metadata, &misc_part, &message);
}

static void brillo_do_fastboot(void)
{
	char *fastboot_args[] = {NULL, "0"};
	cmd_tbl_t *fastboot_cmd = find_cmd("fastboot");
	if (fastboot_cmd) {
		printf("FASTBOOT MODE...\n");
		fastboot_cmd->cmd(fastboot_cmd, 0, 2, fastboot_args);
	}
}

static void brillo_do_reset(void)
{
	char *reset_args[] = {NULL};
	cmd_tbl_t *reset_cmd = find_cmd("reset");
	if (reset_cmd)
		reset_cmd->cmd(reset_cmd, 0, 1, reset_args);
	hang();
}

static void brillo_setup_bootargs(void)
{
	char serial_arg[56] = "androidboot.serialno=";
	char *serial;

	serial = getenv("serial#");
	if (serial) {
		strncat(serial_arg, serial, sizeof(serial_arg) - strlen(serial_arg) - 1);
		setenv("bootargs", serial_arg);
	}
}

char* get_active_slot(void) {
	struct bootloader_message_ab message;
	struct boot_ctrl *metadata = (struct boot_ctrl*)&message.slot_suffix;
	block_dev_desc_t *dev;
	disk_partition_t misc_part;

	char *suffixes[] = { BOOTCTRL_SUFFIX_A, BOOTCTRL_SUFFIX_B };

	if (!(dev = get_dev("mmc", CONFIG_BRILLO_MMC_BOOT_DEVICE)))
		return BOOTCTRL_SUFFIX_NA;

	if (get_partition_info_efi_by_name(dev, "misc", &misc_part))
		return BOOTCTRL_SUFFIX_NA;

	if (ab_read_bootloader_message(dev, &misc_part, &message))
		return BOOTCTRL_SUFFIX_NA;

	if(metadata->slot_info[1].priority > metadata->slot_info[0].priority)
		return suffixes[1];

	return suffixes[0];
}

int get_slot_retry_count(char *suffix) {
	struct bootloader_message_ab message;
	struct boot_ctrl *metadata = (struct boot_ctrl*)&message.slot_suffix;
	block_dev_desc_t *dev;
	disk_partition_t misc_part;
	int i = 0;
	int ret;
	char *suffixes[] = { BOOTCTRL_SUFFIX_A, BOOTCTRL_SUFFIX_B };

	if (!(dev = get_dev("mmc", CONFIG_BRILLO_MMC_BOOT_DEVICE)))
		return -ENODEV;

	if (get_partition_info_efi_by_name(dev, "misc", &misc_part))
		return -ENOENT;

	if ((ret = ab_read_bootloader_message(dev, &misc_part, &message)))
		return ret;

	for(i = 0; i < 2; i++){
		if(!strcmp(suffixes[i], suffix)) {
			return metadata->slot_info[i].tries_remaining;
		}
	}
	return -EINVAL;
}

bool is_successful_slot (char *suffix) {
	struct bootloader_message_ab message;
	struct boot_ctrl *metadata = (struct boot_ctrl*)&message.slot_suffix;
	block_dev_desc_t *dev;
	disk_partition_t misc_part;
	int i = 0;

	char *suffixes[] = { BOOTCTRL_SUFFIX_A, BOOTCTRL_SUFFIX_B };

	if (!(dev = get_dev("mmc", CONFIG_BRILLO_MMC_BOOT_DEVICE)))
		return false;

	if (get_partition_info_efi_by_name(dev, "misc", &misc_part))
		return false;

	if (ab_read_bootloader_message(dev, &misc_part, &message))
		return false;

	for(i = 0; i < 2; i++){
		if(!strcmp(suffixes[i], suffix)) {
			if(metadata->slot_info[i].successful_boot == 1) {
				return true;
			}
		}
	}

	return false;
}

bool is_unbootable_slot (char *suffix) {
	struct bootloader_message_ab message;
	struct boot_ctrl *metadata = (struct boot_ctrl*)&message.slot_suffix;
	block_dev_desc_t *dev;
	disk_partition_t misc_part;
	int i = 0;

	char *suffixes[] = { BOOTCTRL_SUFFIX_A, BOOTCTRL_SUFFIX_B };

	if (!(dev = get_dev("mmc", CONFIG_BRILLO_MMC_BOOT_DEVICE)))
		return false;

	if (get_partition_info_efi_by_name(dev, "misc", &misc_part))
		return false;

	if (ab_read_bootloader_message(dev, &misc_part, &message))
		return false;

	for(i = 0; i < 2; i++){
		if(!strcmp(suffixes[i], suffix)) {
			return (metadata->slot_info[i].priority == 0);
		}
	}

	return false;
}

static void __mark_slot_as_failed(struct slot_metadata *slot, char *slot_name)
{
	slot->successful_boot = 0;
	slot->tries_remaining = 0;
	slot->priority = 0;
	printf("WARNING: failed to boot %s slot!\n", slot_name);
}

/*	Fallback reasons
 *
 *	This must be set only if slot_chosen is set to "fallback" or "recovery"
 *	and explains why the slot with the highest priority didn’t work. Possible values include:
 */
#define FBR_UNKOWN 0
#define FBR_TRY_COUNT_EXHAUSTED 1
#define FBR_READ_FAILED FBR_TRY_COUNT_EXHAUSTED << 1
#define FBR_HASH_MISMATCH FBR_READ_FAILED << 1
#define FBR_SIGNATURE_INVALID FBR_HASH_MISMATCH << 1
#define FBR_UNKOWN_PUBLIC_KEY FBR_SIGNATURE_INVALID << 1
#define FBR_INVALID_ROLLBACK_INDEX FBR_UNKOWN_PUBLIC_KEY << 1

static int brillo_boot_ab(void)
{
	struct bootloader_message_ab message;
	struct boot_ctrl *metadata = (struct boot_ctrl*)&message.slot_suffix;
	block_dev_desc_t *dev;
	disk_partition_t misc_part;
	int ret, index, slots_by_priority[2] = {0, 1};
	char *suffixes[] = { BOOTCTRL_SUFFIX_A, BOOTCTRL_SUFFIX_B };
	char boot_part[8];
	char *old_bootargs;
#ifdef CONFIG_EDISON_ENABLE_EMMC_PWR_ON_WP
	struct mmc *mmc;
#endif
	uint8_t lock_state = 0; /* Lock state is default unlocked */
	uint8_t fallback_reason = FBR_UNKOWN;

	if (!(dev = get_dev("mmc", CONFIG_BRILLO_MMC_BOOT_DEVICE)))
		return -ENODEV;

	if (get_partition_info_efi_by_name(dev, "misc", &misc_part))
		return -ENOENT;

	if ((ret = ab_read_bootloader_message(dev, &misc_part, &message)))
		return ret;

	if (metadata->slot_info[1].priority > metadata->slot_info[0].priority) {
		slots_by_priority[0] = 1;
		slots_by_priority[1] = 0;
	}

#ifdef CONFIG_EDISON_ENABLE_EMMC_PWR_ON_WP
	/* Enable power on write protection on the first 8MB of the booting MMC
	 * Use this space for protecting MBR, GPT header, u-boot and security
	 * partitions. Additional partitions can be placed under the write
	 * protected area by using more than one 8MB WP block in the function
	 * below or by changing the GPT layout of the first 8MB.
	 * Once Power On Write Protect is enabled for a specific WP block that
	 * block remains read-only until the next device's power cycle. If this
	 * option is enabled and you want to write data in the WP block then the
	 * only way to do it is to enter in FASTBOOT from device's Power Off state.
	 * You can achieve that by pressing and holding RM button while executing
	 * a power cycle on the device. Release the RM button once fastboot logs
	 * appeared in the serial console or more than 5 seconds passed since
	 * power on.
	 */
	mmc = find_mmc_device(dev->dev);
	if (!mmc)
		printf("WARNING: Cannot access EMMC device, power on write protection is disabled\n");
	else
		if (mmc_usr_power_on_wp(mmc, 0ULL, 8 * 1024 * 1024))
			printf("WARNING: Cannot enable power on write protection\n");
#endif

	for (index = 0; index < ARRAY_SIZE(slots_by_priority); index++) {
		int slot_num = slots_by_priority[index];
		struct slot_metadata *slot = &metadata->slot_info[slot_num];

		if (slot->successful_boot == 0 && slot->tries_remaining == 0) {
			fallback_reason = FBR_TRY_COUNT_EXHAUSTED;
			/* This is a failed slot, lower priority to zero */
			slot->priority = 0;
		} else {
			if (!slot->priority)
				continue;

			/* either previously successful or tries
			   remaining. attempt to boot. */
			snprintf(boot_part, sizeof(boot_part), "boot%s",
				suffixes[slot_num]);
			if (load_boot_image(dev, boot_part)) {
				/* Failed to load, mark as failed */
				__mark_slot_as_failed(slot, suffixes[slot_num]);
				fallback_reason = FBR_READ_FAILED;
				continue;
			}

			if (slot->tries_remaining > 0)
				slot->tries_remaining--;
			metadata->recovery_tries_remaining = 7;
			ab_write_bootloader_message(dev, &misc_part, &message);

			old_bootargs = strdup(getenv("bootargs"));
			append_to_bootargs(" " BOOT_ARG_SLOT_SUFFIX_STR);
			append_to_bootargs(suffixes[slot_num]);

			fb_read_lock_state(&lock_state);

			append_to_bootargs(" " BOOT_ARG_DEVICE_STATE_STR);

			if (lock_state == 1)
				append_to_bootargs("locked");
			else
				append_to_bootargs("unlocked");

			append_to_bootargs(" " BOOT_ARG_SLOT_CHOSEN_STR);
			if (slots_by_priority[index] == 0) {
				append_to_bootargs("highest");
			} else {
				append_to_bootargs("fallback");
				switch (fallback_reason) {
					case FBR_READ_FAILED:
						append_to_bootargs(" " BOOT_ARG_FALLBACK_REASON_STR);
						append_to_bootargs("read_failed");
						break;
					case FBR_TRY_COUNT_EXHAUSTED:
						append_to_bootargs(" " BOOT_ARG_FALLBACK_REASON_STR);
						append_to_bootargs("try_count_exhausted");
						break;
					default:
						break;
				}
			}

			boot_image();

			/* Failed to boot, mark as failed */
			setenv("bootargs", old_bootargs);
			__mark_slot_as_failed(slot, suffixes[slot_num]);
			ab_write_bootloader_message(dev, &misc_part, &message);
		}
	}

	/* Failed to boot any partition. Try recovery. */
	boot_recovery_image(dev, metadata, &misc_part, &message);

	return -ENOENT;
}

static int do_boot_brillo(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
#ifdef CONFIG_BRILLO_FASTBOOT_ONLY
	brillo_do_fastboot();
	brillo_do_reset();
	hang();
#endif

	char *reboot_target = board_get_reboot_target();
	if (!strcmp(reboot_target, "recovery")) {
		brillo_do_recovery();
		goto boot_failed;
	} else if (!strcmp(reboot_target, "fastboot")) {
		brillo_do_fastboot();
		brillo_do_reset();
	}

	brillo_setup_bootargs();

	brillo_boot_ab();

boot_failed:
	/* Return from boot_ab means normal and recovery via disk
	 * failed. Here we do fastboot as 'Diskless Recovery' */
	brillo_do_fastboot();

	brillo_do_reset();

	hang();

	return -ENOENT;
}

U_BOOT_CMD(
	boot_brillo, 1, 0, do_boot_brillo,
	"Boot to Brillo",
	""
);

static int brillo_fb_write_security_partition(struct security_flags sec_flag)
{
	void *tmp;
	lbaint_t blkcnt, i;
	block_dev_desc_t *dev;
	disk_partition_t security_part;

	if (!(dev = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV))) {
		printf("ERROR: cannot access the eMMC\n");
		return -ENODEV;
	}

	if (get_partition_info_efi_by_name(dev, "security", &security_part)) {
		printf("ERROR: cannot find security partition\n");
		return -ENOENT;
	}


	blkcnt = BLOCK_CNT(sizeof(sec_flag), dev);
	tmp = calloc(blkcnt, dev->blksz);

	if (!tmp)
		return -ENOMEM;

	memcpy(tmp, &sec_flag, sizeof(sec_flag));

	for (i = 0; i < blkcnt; i++) {
		if (dev->block_write(dev->dev, security_part.start + i, 1,
				     tmp + i * dev->blksz) != 1) {
			free(tmp);
			return -EIO;
		}
	}

	free(tmp);
	return 0;
}

static int brillo_fb_read_security_partition(struct security_flags *security_data)
{
	void *tmp;
	lbaint_t blkcnt;
	block_dev_desc_t *dev;
	disk_partition_t security_partition;

	if (!(dev = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV))) {
		printf("ERROR: cannot access the eMMC\n");
		return -ENODEV;
	}

	if (get_partition_info_efi_by_name(dev, "security", &security_partition)) {
		printf("ERROR: cannot find security partition\n");
		return -ENOENT;
	}

	blkcnt = BLOCK_CNT(sizeof(struct security_flags), dev);
	tmp = calloc(blkcnt, dev->blksz);

	if (!tmp)
		return -ENOMEM;

	if (dev->block_read(dev->dev, security_partition.start, blkcnt, tmp) != blkcnt) {
		free(tmp);
		return -EIO;
	}

	memcpy(security_data, tmp, sizeof(struct security_flags));
	free(tmp);
	return 0;
}

int fb_write_lock_state(uint8_t lock)
{
	struct security_flags sec;
	int ret;
	ret = brillo_fb_read_security_partition(&sec);
	if (ret)
		return ret;
	sec.lock = lock;

	return brillo_fb_write_security_partition(sec);
}

int fb_write_dev_key(uint8_t* devkey, unsigned int number_of_bytes)
{
	struct security_flags sec;
	int ret;

	if (number_of_bytes > BVB_DEVKEY_MAX) {
		printf("ERROR: key size is bigger than %d\n", BVB_DEVKEY_MAX);
		return -ENOMEM;
	}

	ret = brillo_fb_read_security_partition(&sec);
	if (ret)
		return ret;

	memcpy(sec.devkey, devkey, number_of_bytes);
	sec.devkey_length = number_of_bytes;

	return brillo_fb_write_security_partition(sec);
}

int fb_read_dev_key(uint8_t* dev_key, uint16_t *dev_key_length)
{
	struct security_flags sec_flag;
	int ret;

	ret = brillo_fb_read_security_partition(&sec_flag);
	if (ret)
		return ret;

	if (sec_flag.devkey_length > BVB_DEVKEY_MAX) {
		printf("ERROR: key size is bigger than %d\n", BVB_DEVKEY_MAX);
		return -ENOMEM;
	}

	memcpy(dev_key, sec_flag.devkey, sec_flag.devkey_length);
	*dev_key_length = sec_flag.devkey_length;

	return 0;
}


int fb_read_lock_state(uint8_t* lock_state)
{
	struct security_flags sec_flag;
	int ret;

	ret = brillo_fb_read_security_partition(&sec_flag);
	if (ret)
		return ret;
	*lock_state = sec_flag.lock;
	return 0;
}
