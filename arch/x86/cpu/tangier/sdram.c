/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * (C) Copyright 2010,2011
 * Graeme Russ, <graeme.russ@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <asm/e820.h>
#include <asm/u-boot-x86.h>
#include <asm/global_data.h>
#include <asm/processor.h>
#include <asm/sections.h>
#include <asm/arch/sysinfo.h>
#include <asm/arch/tables.h>
#ifdef CONFIG_SFI
#include <asm/sfi.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

unsigned install_e820_map(unsigned max_entries, struct e820entry *entries)
{
#ifdef CONFIG_SFI
	return sfi_setup_e820(max_entries, entries);
#else
	return 0;
#endif
}

/*
 * This function looks for the highest region of memory lower than 4GB which
 * has enough space for U-Boot where U-Boot is aligned on a page boundary. It
 * overrides the default implementation found elsewhere which simply picks the
 * end of ram, wherever that may be. The location of the stack, the relocation
 * address, and how far U-Boot is moved by relocation are set in the global
 * data structure.
 */
ulong board_get_usable_ram_top(ulong total_size)
{
	uintptr_t dest_addr = 0x000000003F4FFFFF;
/*
 *    int i;
 *
 *    for (i = 0; i < lib_sysinfo.n_memranges; i++) {
 *        struct memrange *memrange = &lib_sysinfo.memrange[i];
 *        [> Force U-Boot to relocate to a page aligned address. <]
 *        uint64_t start = roundup(memrange->base, 1 << 12);
 *        uint64_t end = memrange->base + memrange->size;
 *
 *        [> Ignore non-memory regions. <]
 *        if (memrange->type != CB_MEM_RAM)
 *            continue;
 *
 *        [> Filter memory over 4GB. <]
 *        if (end > 0xffffffffULL)
 *            end = 0x100000000ULL;
 *        [> Skip this region if it's too small. <]
 *        if (end - start < total_size)
 *            continue;
 *
 *        [> Use this address if it's the largest so far. <]
 *        if (end > dest_addr)
 *            dest_addr = end;
 *    }
 *
 *    [> If no suitable area was found, return an error. <]
 *    if (!dest_addr)
 *        panic("No available memory found for relocation");
 */

	return (ulong)dest_addr;
}

int dram_init_f(void)
{
/*
 *    int i;
 *    phys_size_t ram_size = 0;
 *
 *    for (i = 0; i < lib_sysinfo.n_memranges; i++) {
 *        struct memrange *memrange = &lib_sysinfo.memrange[i];
 *        unsigned long long end = memrange->base + memrange->size;
 *
 *        if (memrange->type == CB_MEM_RAM && end > ram_size)
 *            ram_size = end;
 *    }
 *    gd->ram_size = ram_size;
 *    if (ram_size == 0)
 *        return -1;
 */
#ifdef CONFIG_SFI
	gd->ram_size = sfi_get_ram_size();
#endif
	return 0;
}

int dram_init_banksize(void)
{
/*
 *    int i, j;
 *
 *    if (CONFIG_NR_DRAM_BANKS) {
 *        for (i = 0, j = 0; i < lib_sysinfo.n_memranges; i++) {
 *            struct memrange *memrange = &lib_sysinfo.memrange[i];
 *
 *            if (memrange->type == CB_MEM_RAM) {
 *                gd->bd->bi_dram[j].start = memrange->base;
 *                gd->bd->bi_dram[j].size = memrange->size;
 *                j++;
 *                if (j >= CONFIG_NR_DRAM_BANKS)
 *                    break;
 *            }
 *        }
 *    }
 */
/*
 *0:      0000000000000000-0000000000097FFF (   0K -  608K) ram
 *3:      0000000000100000-0000000003FFFFFF (   1M -   64M) ram
 *5:      0000000006000000-000000003F4FFFFF (  96M - 1013M) ram
 */
	gd->bd->bi_dram[0].start = 0x0;
	gd->bd->bi_dram[0].size = 0x97FFF;

	gd->bd->bi_dram[1].start = 0x100000;
	gd->bd->bi_dram[1].size = 0x3FFFFFF - gd->bd->bi_dram[1].start;

	gd->bd->bi_dram[2].start = 0x6000000;
	gd->bd->bi_dram[2].size = 0x3F4FFFFF  -  gd->bd->bi_dram[2].start;

	return 0;
}

int dram_init(void)
{
	return dram_init_banksize();
}
