/*
 * Copyright 2014 Broadcom Corporation.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <errno.h>
#include <fb_mmc.h>
#include <part.h>
#include <aboot.h>
#include <sparse_format.h>
#include <mmc.h>
#include <memalign.h>

#ifndef CONFIG_FASTBOOT_GPT_NAME
#define CONFIG_FASTBOOT_GPT_NAME GPT_ENTRY_NAME
#endif

#define ONE_MiB (1024 * 1024)

/* The 64 defined bytes plus the '\0' */
#define RESPONSE_LEN	(64 + 1)

static char *response_str;

void fastboot_fail(const char *s)
{
	strncpy(response_str, "FAIL\0", 5);
	strncat(response_str, s, RESPONSE_LEN - 4 - 1);
}

void fastboot_okay(const char *s)
{
	strncpy(response_str, "OKAY\0", 5);
	strncat(response_str, s, RESPONSE_LEN - 4 - 1);
}

__weak int board_verify_gpt_parts(struct gpt_frag *frag)
{
	return 0;
}

__weak int board_populate_mbr_boot_code(legacy_mbr *mbr)
{
	return 0;
}

static int verify_gpt_frag(struct gpt_frag *frag, unsigned int frag_size)
{
	if (frag->magic != GPT_FRAG_MAGIC)
		return -EINVAL;
	if (frag->start_lba != 0)
		return -EINVAL;
	if (frag->num_parts > GPT_ENTRY_NUMBERS)
		return -EINVAL;

	if (frag_size != sizeof(*frag) +
			sizeof(frag->parts[0]) * frag->num_parts)
		return -EINVAL;

	if (board_verify_gpt_parts(frag))
		return -EINVAL;

	return 0;
}

static int build_and_write_gpt(block_dev_desc_t *dev, void *_frag,
		unsigned int frag_size)
{
	struct gpt_frag *frag = _frag;
	struct gpt_frag_part *i, *expandable_part;
	lbaint_t one_mib_blocks = BLOCK_CNT(ONE_MiB, dev);
	lbaint_t loc, usable_blocks, part_blocks;
	size_t bufsz = dev->blksz * 2 +
		sizeof(gpt_entry) * GPT_ENTRY_NUMBERS;
	void *buf = NULL;
	legacy_mbr *mbr;
	gpt_header *gpt_h;
	gpt_entry *gpt_e;
	int ret = -EINVAL;

	if (verify_gpt_frag(frag, frag_size))
		return -EINVAL;

	buf = calloc(1, bufsz);
	if (!buf)
		return -ENOMEM;

	mbr = (legacy_mbr*)buf;
	gpt_h = buf + dev->blksz;
	gpt_e = buf + dev->blksz * 2;

	/* Populate MBR */
	if ((ret = board_populate_mbr_boot_code(mbr)))
		goto error;

	mbr->partition_record[0].sys_ind = EFI_PMBR_OSTYPE_EFI_GPT;
	mbr->partition_record[0].start_sect = 1;
	if (dev->lba > 0xffffffff)
		mbr->partition_record[0].nr_sects = 0xffffffff;
	else
		mbr->partition_record[0].nr_sects = dev->lba - 1;
	mbr->signature = MSDOS_MBR_SIGNATURE;

	/* Populate GPT Header */
	gpt_h->signature = GPT_HEADER_SIGNATURE;
	gpt_h->revision = GPT_HEADER_REVISION_V1;
	gpt_h->header_size = sizeof(*gpt_h);
	gpt_h->my_lba = GPT_PRIMARY_PARTITION_TABLE_LBA;
	gpt_h->alternate_lba = dev->lba - 1;
	gpt_h->first_usable_lba = BLOCK_CNT(bufsz, dev);
	gpt_h->last_usable_lba = dev->lba - BLOCK_CNT(bufsz, dev);
	gen_rand_uuid((void*)&gpt_h->disk_guid);
	gpt_h->partition_entry_lba = 2;
	gpt_h->num_partition_entries = frag->num_parts;
	gpt_h->sizeof_partition_entry = GPT_ENTRY_SIZE;

	/* There may be one partition with size <= 0. We will adjust
	 * this partition to take up the remainder of the disk not
	 * used by other partitiones */
	usable_blocks = gpt_h->last_usable_lba - one_mib_blocks + 1;
	usable_blocks = (usable_blocks / one_mib_blocks) * one_mib_blocks;

	part_blocks = 0;
	expandable_part = NULL;
	for(i = frag->parts; i < frag->parts + frag->num_parts; i++) {
		if (i->size_mib > 0)
			part_blocks += one_mib_blocks * i->size_mib;
		else if (!expandable_part)
			expandable_part = i;
		else
			goto error;
	}

	if (expandable_part)
		expandable_part->size_mib =
			(usable_blocks - part_blocks) / one_mib_blocks;

	/* Populate GPT Entries Array */
	loc = one_mib_blocks;
	for(i = frag->parts; i < frag->parts + frag->num_parts; i++, gpt_e++) {
		lbaint_t ending_lba =
			loc + i->size_mib * one_mib_blocks - 1;

		memcpy(&gpt_e->partition_type_guid, i->type_guid,
			sizeof(efi_guid_t));
		memcpy(&gpt_e->unique_partition_guid, i->part_guid,
			sizeof(efi_guid_t));
		gpt_e->starting_lba = loc;
		gpt_e->ending_lba = ending_lba;
		memcpy(gpt_e->partition_name, i->label,
			sizeof(gpt_e->partition_name));

		loc = ending_lba + 1;
	}

	gpt_h->partition_entry_array_crc32 = crc32(0, buf + dev->blksz * 2,
		gpt_h->num_partition_entries * gpt_h->sizeof_partition_entry);
	gpt_h->header_crc32 = crc32(0, (void*)gpt_h, gpt_h->header_size);

	ret = write_mbr_and_gpt_partitions(dev, buf);
error:
	free(buf);
	return ret;
}

static int get_partition_info_efi_by_name_or_alias(block_dev_desc_t *dev_desc,
		const char *name, disk_partition_t *info)
{
	int ret;

	ret = get_partition_info_efi_by_name(dev_desc, name, info);
	if (ret) {
		/* strlen("fastboot_partition_alias_") + 32(part_name) + 1 */
		char env_alias_name[25 + 32 + 1];
		char *aliased_part_name;

		/* check for alias */
		strcpy(env_alias_name, "fastboot_partition_alias_");
		strncat(env_alias_name, name, 32);
		aliased_part_name = getenv(env_alias_name);
		if (aliased_part_name != NULL)
			ret = get_partition_info_efi_by_name(dev_desc,
					aliased_part_name, info);
	}
	return ret;
}

static void write_raw_image(block_dev_desc_t *dev_desc, disk_partition_t *info,
		const char *part_name, void *buffer,
		unsigned int download_bytes)
{
	lbaint_t blkcnt;
	lbaint_t blks;

	/* determine number of blocks to write */
	blkcnt = ((download_bytes + (info->blksz - 1)) & ~(info->blksz - 1));
	blkcnt = blkcnt / info->blksz;

	if (blkcnt > info->size) {
		error("too large for partition: '%s'\n", part_name);
		fastboot_fail("too large for partition");
		return;
	}

	puts("Flashing Raw Image\n");

	blks = dev_desc->block_write(dev_desc->dev, info->start, blkcnt,
				     buffer);
	if (blks != blkcnt) {
		error("failed writing to device %d\n", dev_desc->dev);
		fastboot_fail("failed writing to device");
		return;
	}

	printf("........ wrote " LBAFU " bytes to '%s'\n", blkcnt * info->blksz,
	       part_name);
	fastboot_okay("");
}

void fb_mmc_flash_write(const char *cmd, void *download_buffer,
			unsigned int download_bytes, char *response)
{
	block_dev_desc_t *dev_desc;
	disk_partition_t info;

	/* initialize the response buffer */
	response_str = response;

	dev_desc = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		error("invalid mmc device\n");
		fastboot_fail("invalid mmc device");
		return;
	}

	if (strcmp(cmd, CONFIG_FASTBOOT_GPT_NAME) == 0) {
		printf("%s: updating MBR, Primary and Backup GPT(s)\n",
		       __func__);
		if (build_and_write_gpt(dev_desc, download_buffer,
				download_bytes)) {
			printf("%s: Failed to flash GPT\n", __func__);
			fastboot_fail("failed to flash GPT");
			return;
		}
		printf("........ success\n");
		fastboot_okay("");
		return;
	} else if (get_partition_info_efi_by_name_or_alias(dev_desc, cmd, &info)) {
		error("cannot find partition: '%s'\n", cmd);
		fastboot_fail("cannot find partition");
		return;
	}

	if (is_sparse_image(download_buffer))
		write_sparse_image(dev_desc, &info, cmd, download_buffer,
				   download_bytes);
	else
		write_raw_image(dev_desc, &info, cmd, download_buffer,
				download_bytes);
}

void fb_mmc_erase(const char *cmd, char *response)
{
	int ret;
	block_dev_desc_t *dev_desc;
	disk_partition_t info;
	lbaint_t blks, blks_start, blks_size, grp_size;
	struct mmc *mmc = find_mmc_device(CONFIG_FASTBOOT_FLASH_MMC_DEV);

	if (mmc == NULL) {
		error("invalid mmc device");
		fastboot_fail("invalid mmc device");
		return;
	}

	/* initialize the response buffer */
	response_str = response;

	dev_desc = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		error("invalid mmc device");
		fastboot_fail("invalid mmc device");
		return;
	}

	ret = get_partition_info_efi_by_name_or_alias(dev_desc, cmd, &info);
	if (ret) {
		error("cannot find partition: '%s'", cmd);
		fastboot_fail("cannot find partition");
		return;
	}

	/* Align blocks to erase group size to avoid erasing other partitions */
	grp_size = mmc->erase_grp_size;
	blks_start = (info.start + grp_size - 1) & ~(grp_size - 1);
	if (info.size >= grp_size)
		blks_size = (info.size - (blks_start - info.start)) &
				(~(grp_size - 1));
	else
		blks_size = 0;

	printf("Erasing blocks " LBAFU " to " LBAFU " due to alignment\n",
	       blks_start, blks_start + blks_size);

	blks = dev_desc->block_erase(dev_desc->dev, blks_start, blks_size);
	if (blks != blks_size) {
		error("failed erasing from device %d", dev_desc->dev);
		fastboot_fail("failed erasing from device");
		return;
	}

	printf("........ erased " LBAFU " bytes from '%s'\n",
	       blks_size * info.blksz, cmd);
	fastboot_okay("");
}
