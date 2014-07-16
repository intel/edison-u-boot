/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * made from cmd_ext2, which was:
 *
 * (C) Copyright 2004
 * esd gmbh <www.esd-electronics.com>
 * Reinhard Arlt <reinhard.arlt@esd-electronics.com>
 *
 * made from cmd_reiserfs by
 *
 * (C) Copyright 2003 - 2004
 * Sysgo Real-Time Solutions, AG <www.elinos.com>
 * Pavel Bartusek <pba@sysgo.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <config.h>
#include <command.h>
#include <part.h>
#include <vsprintf.h>
#include <malloc.h>

#ifndef CONFIG_PARTITION_UUIDS
#error CONFIG_PARTITION_UUIDS must be enabled for CONFIG_CMD_PART to be enabled
#endif

int do_part_info(int argc, char * const argv[])
{
	int part;
	block_dev_desc_t *dev_desc;
	disk_partition_t info;
	char buf_convert[20];
	if (argc < 2)
		return CMD_RET_USAGE;
	if (argc > 5)
		return CMD_RET_USAGE;

	part = get_device_and_partition(argv[0], argv[1], &dev_desc, &info, 0);
	if (part < 0)
		return 1;

	snprintf(buf_convert, sizeof(buf_convert), LBAF, info.start);
	if (argc > 2)
		setenv(argv[2], buf_convert);
	else
		printf("Partition start :0x%s\n", buf_convert);

	snprintf(buf_convert, sizeof(buf_convert), LBAF, info.size);
	if (argc > 3)
		setenv(argv[3], buf_convert);
	else
		printf("Partition size  :0x%s\n", buf_convert);

	snprintf(buf_convert, sizeof(buf_convert), "%lx", info.blksz);
	if (argc > 4)
		setenv(argv[4], buf_convert);
	else
		printf("Block size      :0x%s bytes\n", buf_convert);
	return 0;
}

#define MAX_SEARCH_PARTITIONS 128
int do_part_find(int argc, char * const argv[])
{
	int part, dev, i, ret=1;
	block_dev_desc_t *dev_desc;
	disk_partition_t info;
	size_t mnb_of;
	const char *value_str;
	char *dup_str = NULL;
	const char *criteria_str;
	char buf_str[4];

	if (argc < 3)
		return CMD_RET_USAGE;
	if (argc > 4)
		return CMD_RET_USAGE;

	dev = get_device(argv[0], argv[1], &dev_desc);
	if (dev < 0)
		return 1;

	/* check and split criteria:value */
	value_str = strchr(argv[2],':');
	if (!value_str)
		return CMD_RET_USAGE;

	dup_str = strdup(argv[2]);
	if(!dup_str)
		return 1;

	if ( (value_str - argv[2]) > strlen(dup_str) )
		return 1;

	dup_str[value_str - argv[2]] = 0;
	criteria_str = dup_str;
	value_str++;

	if(!strcmp(criteria_str, "uuid"))
		mnb_of = offsetof(disk_partition_t, uuid);
	else if(!strcmp(criteria_str, "label"))
		mnb_of = offsetof(disk_partition_t, name);
	else {
		printf("Bad criteria: %s\n", criteria_str);
		ret = CMD_RET_USAGE;
		goto end_of_part_find;
	}

	part=-1;
	for (i = 1; i <= MAX_SEARCH_PARTITIONS; i++) {
		ret = get_partition_info(dev_desc, i, &info);
		if (ret)
			continue;

		if(!strcmp(((char*)&info + mnb_of), value_str)) {
			part = i;
			ret = 0;
			break;
		}
	}

	if ( part == -1) {
		printf("No partition found\n");
		ret = 1;
		goto end_of_part_find;
	}

	snprintf(buf_str, sizeof(buf_str), "%d", part);
	if ( argc > 3)
		setenv(argv[3],buf_str);
	else
		printf("Partition %s correspond to %s == %s\n",
				buf_str, criteria_str, value_str);

end_of_part_find:
	if( dup_str )
		free (dup_str);

	return ret;
}

int do_part_uuid(int argc, char * const argv[])
{
	int part;
	block_dev_desc_t *dev_desc;
	disk_partition_t info;

	if (argc < 2)
		return CMD_RET_USAGE;
	if (argc > 3)
		return CMD_RET_USAGE;

	part = get_device_and_partition(argv[0], argv[1], &dev_desc, &info, 0);
	if (part < 0)
		return 1;

	if (argc > 2)
		setenv(argv[2], info.uuid);
	else
		printf("%s\n", info.uuid);

	return 0;
}

int do_part_list(int argc, char * const argv[])
{
	int ret;
	block_dev_desc_t *desc;

	if (argc != 2)
		return CMD_RET_USAGE;

	ret = get_device(argv[0], argv[1], &desc);
	if (ret < 0)
		return 1;

	print_part(desc);

	return 0;
}

int do_part(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "uuid"))
		return do_part_uuid(argc - 2, argv + 2);
	else if (!strcmp(argv[1], "list"))
		return do_part_list(argc - 2, argv + 2);
	else if (!strcmp(argv[1], "info"))
		return do_part_info(argc - 2, argv + 2);
	else if (!strcmp(argv[1], "find"))
		return do_part_find(argc - 2, argv + 2);
	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	part,	7,	1,	do_part,
	"disk partition related commands",
	"find <interface> <dev> <criteria>:<value>\n"
	"    - print first partition number corresponding to criteria:valuei\n"
	"    - criteria could be label or uuid\n"
	"part find <interface> <dev> <criteria>:<value> <varname>\n"
	"    - set environment variable to first partition number corresponding to criteria:valuei\n"
	"    - criteria could be label or uuid\n"
	"uuid <interface> <dev>:<part>\n"
	"    - print partition UUID\n"
	"part uuid <interface> <dev>:<part> <varname>\n"
	"    - set environment variable to partition UUID\n"
	"part list <interface> <dev>\n"
	"    - print a device's partition table\n"
	"part info <interface> <dev>:<part> <varname-start> <varname-size> <varname-blcksize>\n"
	"    - set environment variable varname-start to partition start in blocks\n"
	"    - set environment variable varname-size to partition size in blocks\n"
	"    - set environment variable varname-blcksize to partition block size in bytes\n"
);
