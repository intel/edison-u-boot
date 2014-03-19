#ifndef _APIC_H_
#define _APIC_H_

#include <asm/mpspec.h>

#define IO_APIC_DEFAULT_PHYS_BASE	0xfec00000
#define	APIC_DEFAULT_PHYS_BASE		0xfee00000

#define ALL             (0xff << 24)
#define NONE            (0)
#define DISABLED        (1 << 16)
#define ENABLED         (0 << 16)
#define TRIGGER_EDGE    (0 << 15)
#define TRIGGER_LEVEL   (1 << 15)
#define POLARITY_HIGH   (0 << 13)
#define POLARITY_LOW    (1 << 13)
#define PHYSICAL_DEST   (0 << 11)
#define LOGICAL_DEST    (1 << 11)
#define ExtINT          (7 << 8)
#define NMI             (4 << 8)
#define SMI             (2 << 8)
#define INT             (1 << 8)

#define	SET_APIC_LOGICAL_ID(x)	((x) << 24)

#define	APIC_LVR					0x30
#define		APIC_INTEGRATED(x)		((x) & 0xF0u)
#define		GET_APIC_VERSION(x)		((x) & 0xFFu)
#define		GET_APIC_MAXLVT(x)		(((x) >> 16) & 0xFFu)
#define	APIC_TASKPRI				0x80
#define	APIC_EOI					0xB0
#define	APIC_LDR					0xD0
#define	APIC_DFR					0xE0
#define	APIC_SPIV					0xF0

#define	APIC_TPRI_MASK				0xFFu
#define	APIC_LDR_MASK				(0xFFu << 24)
#define	APIC_DFR_VALUE				0xFFFFFFFFul
#define APIC_VECTOR_MASK			0x000FF
#define	APIC_SPIV_APIC_ENABLED		(1 << 8)
#define	APIC_SPIV_FOCUS_DISABLED	(1 << 9)

#define LOCAL_TIMER_VECTOR			0xef
#define ERROR_APIC_VECTOR			0xfe
#define SPURIOUS_APIC_VECTOR		0xff

#define	APIC_ISR					0x100
#define	APIC_ESR					0x280
#define	APIC_LVTCMCI				0x2f0
#define		APIC_DM_NMI				0x00400
#define	APIC_LVTT					0x320
#define	APIC_LVTTHMR				0x330
#define	APIC_LVTPC					0x340
#define	APIC_LVT0					0x350
#define		APIC_LVT_TIMER_PERIODIC	(1 << 17)
#define		SET_APIC_TIMER_BASE(x)	(((x) << 18))
#define		APIC_TIMER_BASE_DIV		0x2
#define	APIC_LVT1					0x360
#define	APIC_LVTERR					0x370
#define	APIC_TMICT					0x380
#ifdef CONFIG_X86_MRFLD
#define         APIC_TMICT_INIT_CNT                     0x144b50
#else
#define         APIC_TMICT_INIT_CNT                     0xf3c00
#endif
#define	APIC_TMCCT					0x390
#define	APIC_TDCR					0x3E0
#define		APIC_TDR_DIV_TMBASE	(1 << 2)
#define		APIC_TDR_DIV_1		0xB
#define		APIC_TDR_DIV_2		0x0
#define		APIC_TDR_DIV_4		0x1
#define		APIC_TDR_DIV_8		0x2
#define		APIC_TDR_DIV_16		0x3
#define		APIC_TDR_DIV_32		0x8
#define		APIC_TDR_DIV_64		0x9
#define		APIC_TDR_DIV_128	0xA
#define APIC_LVT_MASKED				(1 << 16)
#define	APIC_DM_EXTINT				0x00700

#define LAPIC_ID 0x020

extern unsigned int lapic_timer_frequency;

extern void apic_init(void);
extern void apic_ack_irq(void);
extern void apic_timer_setup(unsigned int clocks, int oneshot, int irqen);
extern void apic_spurious_isr(void);
extern void apic_mask_irq(void);
extern void apic_unmask_irq(void);

#endif /* _APIC_H_ */
