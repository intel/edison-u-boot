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
#include <asm/msr.h>
#include <asm/arch/intel-mid.h>
#include <asm/arch/timestamp.h>
#include <asm/arch/intel_soc_mrfld.h>
#include <intel_scu_ipc.h>
#include <u-boot/md5.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Miscellaneous platform dependent initializations
 */
int arch_cpu_init(void)
{
#ifdef CONFIG_SYS_X86_TSC_TIMER
	timer_set_base(rdtsc());
#endif

	return x86_cpu_init_f();
}

int board_early_init_f(void)
{
	return 0;
}

static int pmu_read_status(void)
{
	struct mrst_pmu_reg *const pmu_reg =
		(struct mrst_pmu_reg *)CONFIG_SYS_TANGIER_PMU_BASE;
	int pmu_busy_retry = 500000;
	u32 temp;
	union pmu_pm_status result;

	do {
		temp = readl(&pmu_reg->pm_sts);

		/* extract the busy bit */
		result.pmu_status_value = temp;

		if (result.pmu_status_parts.pmu_busy == 0)
			return 0;

		udelay(1);
	} while (--pmu_busy_retry);

	return -EBUSY;
}

int power_init_board(void)
{
	struct mrst_pmu_reg *const pmu_reg =
		(struct mrst_pmu_reg *)CONFIG_SYS_TANGIER_PMU_BASE;
	u32 pm_ssc[4];
	int i;

	/* check pmu status. */
	if (pmu_read_status()) {
		printf("WARNING : PMU is busy.\n");
		return -EBUSY;
	}

	/* read pmu values. */
	for (i = 0 ; i < 4 ; i++)
		pm_ssc[i] = readl(&pmu_reg->pm_sss[i]);

	/* modify pmu values. */
	/* enable SDIO0, sdhci for SD card*/
	pm_ssc[0] &= ~(0x3<<(PMU_SDIO0_LSS_01*2));

	/* wrtie modified pmu values. */
	for (i = 0 ; i < 4 ; i++)
		writel(pm_ssc[i], &pmu_reg->pm_ssc[i]);

	/* update modified pmu values. */
	writel(0x00002201, &pmu_reg->pm_cmd);

	/* check pmu status. */
	if (pmu_read_status()) {
		printf("WARNING : PMU is busy.\n");
		return -EBUSY;
	}

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

int board_late_init(void)
{
	if (!getenv("serial#")) {

		struct mmc *mmc = find_mmc_device(0);
		unsigned char emmc_ssn[16];
		char ssn[33];
		char usb_gadget_addr[18];

		if (mmc) {
			int i;

			md5((unsigned char *)mmc->cid, sizeof(mmc->cid), emmc_ssn);

			for (i = 0; i < 16; i++)
				snprintf(&(ssn[2*i]), 3, "%02x", emmc_ssn[i]);

			snprintf(&(usb_gadget_addr[0]), sizeof(usb_gadget_addr),
					"02:00:86:%02x:%02x:%02x", emmc_ssn[13], emmc_ssn[14],
					emmc_ssn[15]);
			setenv("usb0addr", usb_gadget_addr);
			setenv("serial#", ssn);
#if defined(CONFIG_CMD_SAVEENV) && !defined(CONFIG_ENV_IS_NOWHERE)
			saveenv();
#endif
		}
	}

	if (!getenv("hardware_id")) {
		union ipc_ifwi_version v;
		int ret;
		char hardware_id[4];

		ret = intel_scu_ipc_command(IPCMSG_GET_FW_REVISION, 1,
				NULL, 0, (u32 *) &(v.raw[0]), 4);
		if (ret < 0) {
			printf("Can't retrieve hardware revision\n");
		}

		snprintf(hardware_id, sizeof(hardware_id), "%02X", v.fw.hardware_id);
		setenv("hardware_id", hardware_id);
#if defined(CONFIG_CMD_SAVEENV) && !defined(CONFIG_ENV_IS_NOWHERE)
		saveenv();
#endif
	}


	return 0;
}

void panic_puts(const char *str)
{
}

int print_cpuinfo(void)
{
	return default_print_cpuinfo();
}

int board_mmc_init(bd_t * bis)
{
	int err;

	/* add sdhci for eMMC */
	err = tangier_sdhci_init(CONFIG_SYS_EMMC_PORT_BASE, 0, 4);

	if (err)
		return err;

	/* add sdhci for SD card */
	return tangier_sdhci_init(CONFIG_SYS_SD_PORT_BASE, 0, 4);
}

/* ovveride get_tbclk_mhz code see tsc_timer */
/* Get the speed of the TSC timer in MHz */
unsigned __attribute__((no_instrument_function)) long get_tbclk_mhz(void)
{
	u32 ratio , bus_freq;
	u64 platform_info = native_read_msr(MSR_PLATFORM_INFO);
	u64 msr_fsb_freq = native_read_msr(MSR_FSB_FREQ);

	/* compute and correct ratio if necessary */
	ratio = ((platform_info >> 8) & 0xff);
	if(!ratio)
	{
		ratio = 4;
		debug("Read a zero ratio, force tsc ratio to 4 ...\n");
	}

	/* compute fsb */
	bus_freq = (u32) (msr_fsb_freq & 0x7);
	/* lookup real bus freq in kHz according to its index */
	switch(bus_freq)
	{
	case 2:
		bus_freq = FSB_FREQ_133SKU;
		break;
	case 3:
		bus_freq = FSB_FREQ_167SKU;
		break;
	case 4:
		bus_freq = FSB_FREQ_83SKU;
		break;
	case 5:
		bus_freq = FSB_FREQ_400SKU;
		break;
	case 6:
		bus_freq = FSB_FREQ_267SKU;
		break;
	case 7:
		bus_freq = FSB_FREQ_333SKU;
		break;
	default:  /* handle also 0 and 1 */
		bus_freq = FSB_FREQ_100SKU;
		break;
	}
	// return Freq in Mhz
	return ((bus_freq * ratio)/1000);
}

void reset_cpu(ulong addr)
{
	intel_scu_ipc_simple_command(IPCMSG_COLD_RESET, 0);
}
