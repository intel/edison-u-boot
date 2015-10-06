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

/* To be overridden by whatever the board's implementation is of
 * 'adb reboot fastboot' and the like.
 */
__weak char* board_get_reboot_target(void) { return NULL; }

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

/* Boots to the image already successfully loaded at
 * CONFIG_SYS_LOAD_ADDR. If the function returns then booting has
 * failed.
 */
static void boot_image(void)
{
	struct andr_img_hdr *hdr = (struct andr_img_hdr*)CONFIG_SYS_LOAD_ADDR;
	ulong load_address;
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

static void brillo_boot_recovery(void)
{
	if (load_boot_image(get_dev("mmc", CONFIG_BRILLO_MMC_BOOT_DEVICE),
			"recovery"))
		return;
	boot_image();
}

static void brillo_boot_ab(void)
{
	/* TODO: implement A/B */
	if (load_boot_image(get_dev("mmc", CONFIG_BRILLO_MMC_BOOT_DEVICE),
			"boot"))
		return;
	boot_image();
}

static int do_boot_brillo(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	char *reboot_target = board_get_reboot_target();
	if (!strcmp(reboot_target, "fastboot")) {
		brillo_do_fastboot();
		brillo_do_reset();
	} else if (!strcmp(reboot_target, "recovery")) {
		brillo_boot_recovery();
		brillo_do_reset();
	}

	brillo_boot_ab();

	brillo_boot_recovery();

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
