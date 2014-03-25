/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * (C) Copyright 2008
 * Graeme Russ, graeme.russ@gmail.com.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/u-boot-x86.h>
#include <asm/cache.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch/mmc.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Miscellaneous platform dependent initializations
 */
int cpu_init_f(void)
{
	/*
	 *timestamp_init();
	 */

	return 0;
}

int board_early_init_f(void)
{
	return 0;
}

int board_early_init_r(void)
{
	return 0;
}

void show_boot_progress(int val)
{
	outb(val, 0x80);
}

int last_stage_init(void)
{
	/*
	 *if (gd->flags & GD_FLG_COLD_BOOT)
	 *        timestamp_add_to_bootstage();
	 */

	return 0;
}

int board_final_cleanup(void)
{

	return 0;
}

void panic_puts(const char *str)
{
}

int board_mmc_init(bd_t * bis)
{
	int index = 0;
	unsigned int base = CONFIG_SYS_EMMC_PORT_BASE + (0x40000 * index);

	return tangier_sdhci_init(base, index, 4);
}
