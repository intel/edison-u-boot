#ifndef _ASM_X86_MPSPEC_H
#define _ASM_X86_MPSPEC_H

/* from apicdef.h */
# define MAX_IO_APICS 64
# define MAX_LOCAL_APIC 256

#define SFI_MTMR_MAX_NUM 8

#define MP_PROCESSOR        0
#define MP_BUS          1
#define MP_IOAPIC       2
#define MP_INTSRC       3
#define MP_LINTSRC      4

#define BITS_PER_BYTE 8

#define BITS_TO_LONGS(nr)   DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

#define PHYSID_ARRAY_SIZE   BITS_TO_LONGS(MAX_LOCAL_APIC)

#define MAX_MP_BUSSES      260

struct physid_mask {
	unsigned long mask[PHYSID_ARRAY_SIZE];
};

typedef struct physid_mask physid_mask_t;

#define PHYSID_MASK_ALL     { {[0 ... PHYSID_ARRAY_SIZE-1] = ~0UL} }
#define PHYSID_MASK_NONE    { {[0 ... PHYSID_ARRAY_SIZE-1] = 0UL} }

#define physids_empty(map)                  \
	bitmap_empty((map).mask, MAX_LOCAL_APIC)

enum mp_irq_source_types {
	mp_INT = 0,
	mp_NMI = 1,
	mp_SMI = 2,
	mp_ExtINT = 3
};

#define MP_APIC_ALL 0xFF

enum mp_bustype {
	MP_BUS_ISA = 1,
	MP_BUS_EISA,
	MP_BUS_PCI,
	MP_BUS_MCA,
};

#define MPC_APIC_USABLE     0x01

struct mpc_ioapic {
	unsigned char type;
	unsigned char apicid;
	unsigned char apicver;
	unsigned char flags;
	unsigned int apicaddr;
};

struct mpc_intsrc {
	unsigned char type;
	unsigned char irqtype;
	unsigned short irqflag;
	unsigned char srcbus;
	unsigned char srcbusirq;
	unsigned char dstapic;
	unsigned char dstirq;
};

#define MAX_IRQ_SOURCES     256

#endif
