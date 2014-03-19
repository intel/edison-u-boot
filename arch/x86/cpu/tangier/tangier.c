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

DECLARE_GLOBAL_DATA_PTR;

/*
 * Miscellaneous platform dependent initializations
 */
int cpu_init_f(void)
{
	/*
	 *timestamp_init();
	 */

	return x86_cpu_init_f();
}

int board_early_init_f(void)
{
	return 0;
}

int board_early_init_r(void)
{
	return 0;
}

int board_final_cleanup(void)
{

	return 0;
}

void panic_puts(const char *str)
{
}

int print_cpuinfo(void)
{
	return default_print_cpuinfo();
}
