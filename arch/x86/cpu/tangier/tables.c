/*
 * This file is part of the libpayload project.
 *
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 * Copyright (C) 2009 coresystems GmbH
 *
 * SPDX-License-Identifier:	BSD-3-Clause
 */

#include <common.h>
#include <asm/arch-coreboot/ipchecksum.h>
#include <asm/arch-coreboot/sysinfo.h>
#include <asm/arch-coreboot/tables.h>

/*
 * This needs to be in the .data section so that it's copied over during
 * relocation. By default it's put in the .bss section which is simply filled
 * with zeroes when transitioning from "ROM", which is really RAM, to other
 * RAM.
 */
struct sysinfo_t lib_sysinfo __attribute__ ((section(".data")));

int get_coreboot_info(struct sysinfo_t *info)
{
	return 0;
}
