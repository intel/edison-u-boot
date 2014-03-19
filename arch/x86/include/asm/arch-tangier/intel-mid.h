#ifndef _ASM_X86_INTEL_MID_H
#define _ASM_X86_INTEL_MID_H

/*
 * Penwell uses spread spectrum clock, so the freq number is not exactly
 * the same as reported by MSR based on SDM.
 * CLVP A0 has 100MHz FSB and CLVP B0 has 133MHz FSB.
 */
#define FSB_FREQ_83SKU  83200
#define FSB_FREQ_100SKU 99840
#define FSB_FREQ_133SKU 133000

#define FSB_FREQ_167SKU 167000
#define FSB_FREQ_200SKU 200000
#define FSB_FREQ_267SKU 267000
#define FSB_FREQ_333SKU 333000
#define FSB_FREQ_400SKU 400000

/* Bus Select SoC Fuse value */
#define BSEL_SOC_FUSE_MASK      0x7
#define BSEL_SOC_FUSE_001       0x1 /* FSB 133MHz */
#define BSEL_SOC_FUSE_101       0x5 /* FSB 100MHz */
#define BSEL_SOC_FUSE_111       0x7 /* FSB 83MHz */

extern unsigned int loops_per_jiffy;

extern unsigned long calibrate_tsc(void);

#endif /* _ASM_X86_INTEL_MID_H */
