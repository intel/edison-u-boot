#include <common.h>
#include <asm/bootm.h>
#include <asm/zimage.h>
#include <android_image.h>
#include <command.h>
#include <errno.h>
#include <memalign.h>

#define BOOT_SIGNATURE_MAX_SIZE 4096

#ifndef CONFIG_BRILLO_MMC_BOOT_DEVICE
#define CONFIG_BRILLO_MMC_BOOT_DEVICE CONFIG_FASTBOOT_FLASH_MMC_DEV
#endif

#define METADATA_PARTITION_NAME "misc"
#define SLOT_SUFFIX_STR "androidboot.slot_suffix="

/* Boot control data starts at 2048 byte offset from start of misc */
#define BOOTCTRL_METADATA_OFFSET    2048

#define BOOTCTRL_SUFFIX_A           "_a"
#define BOOTCTRL_SUFFIX_B           "_b"

#define BOOT_CONTROL_VERSION    1

struct slot_metadata {
	uint8_t priority : 4;
	uint8_t tries_remaining : 3;
	uint8_t successful_boot : 1;
	uint8_t padding[3];
} __attribute__((packed));

struct boot_ctrl {
	/* Magic for identification - 'A' 'B' 'B' (Boot Contrl Magic) */
	uint8_t magic[3];

	/* Version of struct. */
	uint8_t version;

	/* Information about each slot. */
	struct slot_metadata slot_info[2];
} __attribute__((packed));

/* To be overridden by whatever the board's implementation is of
 * 'adb reboot fastboot' and the like.
 */
__weak char* board_get_reboot_target(void) { return NULL; }

/* Properly initialise the metadata partition */
static void ab_init_default_metadata(struct boot_ctrl *ctrl)
{
	ctrl->magic[0] = 'A';
	ctrl->magic[1] = 'B';
	ctrl->magic[2] = 'B';
	ctrl->version = 1;

	ctrl->slot_info[0].priority = 2;
	ctrl->slot_info[0].tries_remaining = 0;
	ctrl->slot_info[0].successful_boot = 1;

	ctrl->slot_info[1].priority = 1;
	ctrl->slot_info[1].tries_remaining = 1;
	ctrl->slot_info[1].successful_boot = 0;
}

static int ab_read_metadata(block_dev_desc_t *dev, disk_partition_t *meta_part,
		struct boot_ctrl *metadata)
{
	void *tmp;
	lbaint_t blkcnt, start;

	blkcnt = BLOCK_CNT(sizeof(*metadata), dev);
	start = meta_part->start + BLOCK_CNT(BOOTCTRL_METADATA_OFFSET, dev);
	tmp = calloc(blkcnt, dev->blksz);

	if (!tmp)
		return -ENOMEM;

	if (dev->block_read(dev->dev, start, blkcnt, tmp) != blkcnt) {
		free(tmp);
		return -EIO;
	}

	memcpy(metadata, tmp, sizeof(*metadata));
	free(tmp);

	/* Check if the metadata is correct. */
	if (metadata->magic[0] != 'A' ||
			metadata->magic[1] != 'B' ||
			metadata->magic[2] != 'B') {
		printf("WARNING: A/B Selector Metadata is not initialised or corrupted,"
			" using defaults\n");
		ab_init_default_metadata(metadata);
	}

	return 0;
}

static int ab_write_metadata(block_dev_desc_t *dev, disk_partition_t *meta_part,
		struct boot_ctrl *metadata)
{
	void *tmp;
	lbaint_t blkcnt, start;

	blkcnt = BLOCK_CNT(sizeof(*metadata), dev);
	start = meta_part->start + BLOCK_CNT(BOOTCTRL_METADATA_OFFSET, dev);
	tmp = calloc(blkcnt, dev->blksz);

	if (!tmp)
		return -ENOMEM;

	memcpy(tmp, metadata, sizeof(*metadata));

	if (dev->block_write(dev->dev, start, blkcnt, tmp) != blkcnt) {
		free(tmp);
		return -EIO;
	}

	free(tmp);
	return 0;
}

/* Loads an android boot image from the given device and named
 * partition into CONFIG_SYS_LOAD_ADDR.
 */
static int load_boot_image(block_dev_desc_t *dev, const char *part_name)
{
	struct andr_img_hdr *hdr = (struct andr_img_hdr*)CONFIG_SYS_LOAD_ADDR;
	void *rest;
	disk_partition_t boot_part;
	lbaint_t hdr_blkcnt, rest_blkcnt;

	if (get_partition_info_efi_by_name(dev, part_name, &boot_part))
		return -ENOENT;

	hdr_blkcnt = BLOCK_CNT(sizeof(*hdr), dev);
	if (dev->block_read(dev->dev, boot_part.start, hdr_blkcnt,
			(void*)CONFIG_SYS_LOAD_ADDR) != hdr_blkcnt)
		return -EIO;

	if (android_image_check_header(hdr))
		return -EINVAL;

	if (hdr->page_size < 2048 || hdr->page_size > 16384
			|| hdr->page_size & (hdr->page_size - 1))
		return -EINVAL;

	rest = ((void*)hdr) + (hdr_blkcnt * dev->blksz);

	rest_blkcnt = BLOCK_CNT(hdr->page_size
		+ ROUND(hdr->kernel_size,  hdr->page_size)
		+ ROUND(hdr->ramdisk_size, hdr->page_size)
		+ ROUND(hdr->second_size,  hdr->page_size)
		+ BOOT_SIGNATURE_MAX_SIZE,
		dev) - hdr_blkcnt;

	if (dev->block_read(dev->dev, boot_part.start + hdr_blkcnt,
			rest_blkcnt, rest) != rest_blkcnt)
		return -EIO;

	return 0;
}

static void append_to_bootargs(const char *str) {
	char *bootargs = getenv("bootargs");
	if (!bootargs) {
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
	struct andr_img_hdr *hdr = (struct andr_img_hdr*)CONFIG_SYS_LOAD_ADDR;
	ulong load_address;
	char *bootargs = getenv("bootargs");

	if (bootargs) {
		bootargs = strdup(bootargs);
		setenv("bootargs", hdr->cmdline);
		append_to_bootargs(" ");
		append_to_bootargs(bootargs);
		free(bootargs);
	} else
		setenv("bootargs", hdr->cmdline);

	struct boot_params *params =
		load_zimage((void*)CONFIG_SYS_LOAD_ADDR + hdr->page_size,
			hdr->kernel_size, &load_address);
	setup_zimage(params, ((void*)params) + 0x9000, 0,
		(ulong)(((void*)hdr) + ROUND(hdr->kernel_size, hdr->page_size)
		        + hdr->page_size),
		hdr->ramdisk_size);
	boot_linux_kernel((ulong)params, load_address, false);
}

static void brillo_do_fastboot(void)
{
	char *fastboot_args[] = {NULL, "0"};
	cmd_tbl_t *fastboot_cmd = find_cmd("fastboot");
	if (fastboot_cmd)
		fastboot_cmd->cmd(fastboot_cmd, 0, 2, fastboot_args);
}

static void brillo_do_reset(void)
{
	char *reset_args[] = {NULL};
	cmd_tbl_t *reset_cmd = find_cmd("reset");
	if (reset_cmd)
		reset_cmd->cmd(reset_cmd, 0, 1, reset_args);
	hang();
}

static int brillo_boot_ab(void)
{
	struct boot_ctrl metadata;
	block_dev_desc_t *dev;
	disk_partition_t meta_part;
	int ret, index, slots_by_priority[2] = {0, 1};
	char *suffixes[] = { BOOTCTRL_SUFFIX_A, BOOTCTRL_SUFFIX_B };
	char *boot_part = "boot" BOOTCTRL_SUFFIX_A;

	if (!(dev = get_dev("mmc", CONFIG_BRILLO_MMC_BOOT_DEVICE)))
		return -ENODEV;

	if (get_partition_info_efi_by_name(dev, METADATA_PARTITION_NAME,
			&meta_part))
		return -ENOENT;

	if ((ret = ab_read_metadata(dev, &meta_part, &metadata)))
		return ret;

	if (metadata.slot_info[1].priority > metadata.slot_info[0].priority) {
		slots_by_priority[0] = 1;
		slots_by_priority[1] = 0;
	}

	for (index = 0; index < ARRAY_SIZE(slots_by_priority); index++) {
		int slot_num = slots_by_priority[index];
		struct slot_metadata *slot = &metadata.slot_info[slot_num];

		if (slot->successful_boot == 0 && slot->tries_remaining == 0) {
			/* This is a failed slot, lower priority to zero */
			slot->priority = 0;
		} else {
			/* either previously successful or tries
			   remaining. attempt to boot. */
			snprintf(boot_part, strlen(boot_part) + 1, "boot%s",
				suffixes[slot_num]);
			if (load_boot_image(dev, boot_part)) {
				/* Failed to load, mark as failed */
				slot->tries_remaining = 0;
				slot->priority = 0;
				continue;
			}
			if (slot->tries_remaining > 0)
				slot->tries_remaining--;

			append_to_bootargs(" " SLOT_SUFFIX_STR);
			append_to_bootargs(suffixes[slot_num]);

			ab_write_metadata(dev, &meta_part, &metadata);
			boot_image();

			/* return from boot_image() indicates that we
			   failed to start booting. Continue to next
			   slot, or possibly on to recovery if no other
			   slots are valid. */
		}
	}

	/* Failed to boot any partition. Write metadata in case we
	   updated priority of failed partitions. */
	ab_write_metadata(dev, &meta_part, &metadata);

	return -ENOENT;
}

static int do_boot_brillo(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
#ifdef CONFIG_BRILLO_FASTBOOT_ONLY
	brillo_do_fastboot();
	brillo_do_reset();
#endif

	char *reboot_target = board_get_reboot_target();
	if (!strcmp(reboot_target, "fastboot")) {
		brillo_do_fastboot();
		brillo_do_reset();
	}

	brillo_boot_ab();

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
