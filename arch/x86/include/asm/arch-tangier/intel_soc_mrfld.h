#ifndef __ASM_X86_INTEL_SOC_MRFLD_H__

#define __ASM_X86_INTEL_SOC_MRFLD_H__

#define PMU_PSH_LSS_00			0
#define PMU_SDIO0_LSS_01		1
#define PMU_EMMC0_LSS_02		2
#define PMU_RESERVED_LSS_03		3
#define PMU_SDIO1_LSS_04		4
#define PMU_HSI_LSS_05			5
#define PMU_SECURITY_LSS_06		6
#define PMU_RESERVED_LSS_07		7
#define PMU_USB_MPH_LSS_08		8
#define PMU_USB3_LSS_09			9
#define PMU_AUDIO_LSS_10		10
#define PMU_RESERVED_LSS_11		11
#define PMU_RESERVED_LSS_12		12
#define PMU_RESERVED_LSS_13		13
#define PMU_RESERVED_LSS_14		14
#define PMU_RESERVED_LSS_15		15
#define PMU_RESERVED_LSS_16		16
#define PMU_SSP3_LSS_17			17
#define PMU_SSP5_LSS_18			18
#define PMU_SSP6_LSS_19			19
#define PMU_I2C1_LSS_20			20
#define PMU_I2C2_LSS_21			21
#define PMU_I2C3_LSS_22			22
#define PMU_I2C4_LSS_23			23
#define PMU_I2C5_LSS_24			24
#define PMU_GP_DMA_LSS_25		25
#define PMU_I2C6_LSS_26			26
#define PMU_I2C7_LSS_27			27
#define PMU_USB_OTG_LSS_28		28
#define PMU_RESERVED_LSS_29		29
#define PMU_RESERVED_LSS_30		30
#define PMU_UART0_LSS_31		31
#define PMU_UART1_LSS_31		31
#define PMU_UART2_LSS_31		31

#define PMU_I2C8_LSS_33			33
#define PMU_I2C9_LSS_34			34
#define PMU_SSP4_LSS_35			35
#define PMU_PMW_LSS_36			36

#define EMMC0_LSS			PMU_EMMC0_LSS_02

struct mrst_pmu_reg {
	u32 pm_sts;             /* 0x00 */
	u32 pm_cmd;             /* 0x04 */
	u32 pm_ics;             /* 0x08 */
	u32 _resv1;
	u32 pm_wkc[2];          /* 0x10 */
	u32 pm_wks[2];          /* 0x18 */
	u32 pm_ssc[4];          /* 0x20 */
	u32 pm_sss[4];          /* 0x30 */
	u32 pm_wssc[4];         /* 0x40 */
	u32 pm_c3c4;            /* 0x50 */
	u32 pm_c5c6;            /* 0x54 */
	u32 pm_msic;            /* 0x58 */
};

union pmu_pm_status {
	struct {
		u32 pmu_rev:8;
		u32 pmu_busy:1;
		u32 mode_id:4;
		u32 Reserved:19;
	} pmu_status_parts;
	u32 pmu_status_value;
};

#endif
