/*
 * Copyright 2014 Broadcom Corporation.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FB_MMC_H_
#define __FB_MMC_H_

#define GPT_FRAG_MAGIC 0x6a8b0da1

struct gpt_frag_part {
	int32_t size_mib;
	uint8_t label[72];
	uint8_t type_guid[16];
	uint8_t part_guid[16];
} __attribute__((packed));

struct gpt_frag {
	uint32_t magic;
	uint32_t start_lba;
	uint32_t num_parts;
	struct gpt_frag_part parts[0];
} __attribute__((packed));


void fb_mmc_flash_write(const char *cmd, void *download_buffer,
			unsigned int download_bytes, char *response);
void fb_mmc_erase(const char *cmd, char *response);

#endif
