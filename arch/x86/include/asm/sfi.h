/* Copyright (c) 2012 Intel 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _SFI_H
#define _SFI_H

#include <asm/arch/sysinfo.h>

/* Memory type definitions */
enum sfi_mem_type {
	SFI_MEM_RESERVED,
	SFI_LOADER_CODE,
	SFI_LOADER_DATA,
	SFI_BOOT_SERVICE_CODE,
	SFI_BOOT_SERVICE_DATA,
	SFI_RUNTIME_SERVICE_CODE,
	SFI_RUNTIME_SERVICE_DATA,
	SFI_MEM_CONV,
	SFI_MEM_UNUSABLE,
	SFI_ACPI_RECLAIM,
	SFI_ACPI_NVS,
	SFI_MEM_MMIO,
	SFI_MEM_IOPORT,
	SFI_PAL_CODE,
	SFI_MEM_TYPEMAX,
};

#define SFI_SYST_MAGIC 0x54535953
#define SFI_MMAP_MAGIC 0x50414d4d

struct sfi_mem_entry {
	enum sfi_mem_type type;
	u64 phy_start;
	u64 vir_start;
	u64 pages;
	u64 attrib;
} __attribute__ ((packed));

struct sfi_apic_table_entry {
	u64 phys_addr;		/* phy base addr for APIC reg */
} __attribute__ ((packed));

struct sfi_timer_table_entry {
	u64 phys_addr;		/* phy base addr for the timer */
	u32 freq_hz;		/* in HZ */
	u32 irq;
} __attribute__ ((packed));

struct sfi_table_header {
	char signature[4];
	u32 length;
	u8 revision;
	u8 checksum;
	char oem_id[6];
	char oem_table_id[8];
} __attribute__ ((packed));

struct sfi_quad_word {
	u32 low;
	u32 high;
};

struct sfi_table {
	struct sfi_table_header header;
	union {
		u64 pentry[1];
		struct sfi_quad_word entry[1];
	};
} __attribute__ ((packed));

#define SFI_TBL_HEADER_LEN      24

#define SFI_GET_NUM_ENTRIES(ptable, entry_type) \
	((ptable->header.length - sizeof(struct sfi_table_header)) / \
	(sizeof(entry_type)))

#define SFI_GET_ENTRY_NUM(ptable, entry) \
	((ptable->header.length - SFI_TBL_HEADER_LEN) / \
	(sizeof(struct entry)))

#define SFI_BASE_ADDR		0x000E0000
#define SFI_LENGTH		0x00020000

extern int sfi_mtimer_num;

void sfi_parse_mtmr(void);
struct sfi_timer_table_entry *sfi_get_mtmr(int hint);
void fill_memranges_from_e820(struct sysinfo_t *info);

#ifdef CONFIG_X86_IO_APIC
int sfi_parse_ioapic(void);
#endif /* CONFIG_X86_IO_APIC */

#ifdef CONFIG_DETECT_RAM_SIZE_WORKAROUND
unsigned long long sfi_get_max_usable_ram(struct sysinfo_t *info);
#endif

#ifdef CONFIG_SFI
extern phys_size_t sfi_get_ram_size(void);
extern void sfi_setup_e820(struct boot_params *bp);
#endif

#endif /* _SFI_H */
