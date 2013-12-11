#include <common.h>
#include <asm/bootparam.h>
#include <asm/sfi.h>

#define SFI_BASE_ADDR		0x000E0000
#define SFI_LENGTH		0x00020000
#define SFI_TABLE_LENGTH	16

static int sfi_table_check(struct sfi_table_header *sbh)
{
	char chksum = 0;
	char *pos = (char *)sbh;
	int i;

	if (sbh->length < SFI_TABLE_LENGTH)
		return -1;

	if (sbh->length > SFI_LENGTH)
		return -1;

	for (i = 0; i < sbh->length; i++)
		chksum += *pos++;

	if (chksum)
		printf("sfi: Invalid checksum\n");

	/* checksum is ok if zero */
	return chksum;
}

static unsigned long sfi_search_mmap(void)
{
	u32 i = 0;
	u32 *pos = (u32 *)SFI_BASE_ADDR;
	u32 *end = (u32 *)(SFI_BASE_ADDR + SFI_LENGTH);
	struct sfi_table_header *sbh;
	struct sfi_table *sb;
	u32 sys_entry_cnt = 0;

	/* Find SYST table */
	for (; pos < end; pos += 4) {
		if (*pos == SFI_SYST_MAGIC) {
			if (!sfi_table_check((struct sfi_table_header *)pos))
				break;
		}
	}

	if (pos >= end) {
		printf("failed to locate SFI SYST table\n");
		return 0;
	}

	/* map table pointers */
	sb = (struct sfi_table *)pos;
	sbh = (struct sfi_table_header *)sb;

	sys_entry_cnt = (sbh->length - sizeof(struct sfi_table_header)) >> 3;

	/* Search through each SYST entry for MMAP table */
	for (i = 0; i < sys_entry_cnt; i++) {
		sbh = (struct sfi_table_header *)sb->entry[i].low;
		if (*(u32 *)sbh->signature == SFI_MMAP_MAGIC) {
			if (!sfi_table_check((struct sfi_table_header *)sbh))
				return (unsigned long) sbh;
		}
	}

	return 0;
}

void sfi_setup_e820(struct boot_params *bp)
{
	struct sfi_table *sb;
	struct sfi_mem_entry *mentry;
	unsigned long long start, end, size;
	int i, num, type, total;

	bp->e820_entries = 0;
	total = 0;

	/* search for sfi mmap table */
	sb = (struct sfi_table *)sfi_search_mmap();
	if (!sb) {
		printf("failed to locate SFI MMAP table\n");
		return;
	}
	printf("will use sfi mmap table for e820 table\n");
	num = SFI_GET_ENTRY_NUM(sb, sfi_mem_entry);
	mentry = (struct sfi_mem_entry *)sb->pentry;

	for (i = 0; i < num; i++) {
		start = mentry->phy_start;
		size = mentry->pages << 12;
		end = start + size;

		if (start > end)
			continue;

		/* translate SFI mmap type to E820 map type */
		switch (mentry->type) {
		case SFI_MEM_CONV:
			type = E820_RAM;
			break;
		case SFI_MEM_UNUSABLE:
		case SFI_RUNTIME_SERVICE_DATA:
			mentry++;
			continue;
		default:
			type = E820_RESERVED;
		}

		if (total == E820MAX)
			break;
		bp->e820_map[total].addr = start;
		bp->e820_map[total].size = size;
		bp->e820_map[total++].type = type;

		mentry++;
	}

	bp->e820_entries = total;
}

phys_size_t sfi_get_ram_size(void)
{
	struct sfi_table *sb;
	struct sfi_mem_entry *mentry;
	unsigned long long start, end, size;
	int i, num;
	phys_size_t ram = 0;

	/* search for sfi mmap table */
	sb = (struct sfi_table *)sfi_search_mmap();
	if (!sb) {
		printf("failed to locate SFI MMAP table\n");
		return 0;
	}
	printf("will use sfi mmap table for e820 table\n");
	num = SFI_GET_ENTRY_NUM(sb, sfi_mem_entry);
	mentry = (struct sfi_mem_entry *)sb->pentry;

	for (i = 0; i < num; i++, mentry++) {
		if (mentry->type != SFI_MEM_CONV)
			continue;

		start = mentry->phy_start;
		size = mentry->pages << 12;
		end = start + size;

		if (start > end)
			continue;

		if (ram < end)
			ram = end;
	}

	/* round up to 512mb */
	ram = (ram + (512 * 1024 * 1024 - 1)) & ~(512 * 1024 * 1024 - 1);

	printf("ram size %llu\n", ram);

	return ram;
}
