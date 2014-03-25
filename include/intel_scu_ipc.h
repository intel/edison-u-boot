#ifndef _INTEL_SCU_IPC_H_
#define _INTEL_SCU_IPC_H_

#include <power/intel_scu_pmic.h>
 

/* IPC defines the following message types */
#define IPCMSG_BATTERY          0xEF /* Coulomb Counter Accumulator */
#define IPCMSG_MIP_ACCESS       0xEC /* IA MIP access */
#define IPCMSG_WARM_RESET   0xF0
#define IPCMSG_COLD_RESET   0xF1
#define IPCMSG_SOFT_RESET   0xF2     
#define IPCMSG_COLD_BOOT    0xF3
#define IPCMSG_FW_REVISION      0xF4 /* Get firmware revision */
#define IPCMSG_WATCHDOG_TIMER   0xF8 /* Set Kernel Watchdog Threshold */
#define IPCMSG_VRTC     0xFA     /* Set vRTC device */
#define IPCMSG_FW_UPDATE        0xFE /* Firmware update */
#define IPCMSG_OSC_CLK      0xE6 /* Turn on/off osc clock */
#define IPCMSG_S0IX_COUNTER 0xEB /* Get S0ix residency */

#define RP_WRITE_OSNIB          0xE4
#define RP_WRITE_OEMNIB         0xDF
#define IPCMSG_WRITE_OSNIB	RP_WRITE_OSNIB
#define IPCMSG_WRITE_OEMNIB	RP_WRITE_OEMNIB

#define IPC_CMD_UMIP_RD     0
#define IPC_CMD_UMIP_WR     1
#define IPC_CMD_SMIP_RD     2

#define IPC_ERR_NONE            0
#define IPC_ERR_CMD_NOT_SUPPORTED   1
#define IPC_ERR_CMD_NOT_SERVICED    2
#define IPC_ERR_UNABLE_TO_SERVICE   3
#define IPC_ERR_CMD_INVALID     4
#define IPC_ERR_CMD_FAILED      5
#define IPC_ERR_EMSECURITY      6

/* Command id associated with message IPCMSG_VRTC */
#define IPC_CMD_VRTC_SETTIME      1 /* Set time */
#define IPC_CMD_VRTC_SETALARM     2 /* Set alarm */
#define IPC_CMD_VRTC_SYNC_RTC     3 /* Sync MSIC/PMIC RTC to VRTC */
/* Issue commands to the SCU with or without data */
void intel_scu_ipc_send_command(u32 cmd);
int intel_scu_ipc_check_status(void);
int intel_scu_ipc_simple_command(int cmd, int sub);
void intel_scu_ipc_lock(void);
void intel_scu_ipc_unlock(void);
int intel_scu_ipc_command(u32 cmd, u32 sub, u8 *in, u8 inlen,
        u32 *out, u32 outlen);
int intel_scu_ipc_raw_cmd(u32 cmd, u32 sub, u8 *in, u8 inlen,
        u32 *out, u32 outlen, u32 dptr, u32 sptr);

#if 0 /* FIXME: temporary removed */
/* I2C control api */
int intel_scu_ipc_i2c_cntrl(u32 addr, u32 *data);

/* Update FW version */
int intel_scu_ipc_fw_update(void);

int intel_scu_ipc_read_mip(u8 *data, int len, int offset, int issigned);
int intel_scu_ipc_write_umip(u8 *data, int len, int offset);

extern struct blocking_notifier_head intel_scu_notifier;

static inline void intel_scu_notifier_add(struct notifier_block *nb)
{
	blocking_notifier_chain_register(&intel_scu_notifier, nb);
}

static inline void intel_scu_notifier_remove(struct notifier_block *nb)
{
	blocking_notifier_chain_unregister(&intel_scu_notifier, nb);
}

static inline int intel_scu_notifier_post(unsigned long v, void *p)
{
	return blocking_notifier_call_chain(&intel_scu_notifier, v, p);
}

#define     SCU_AVAILABLE       1
#define     SCU_DOWN        2
#endif 

#define VRTC_REG			0x8C00 
#define INT_CTRL_BASE		0x8E00 
#define EXTTIMER_ADDR_BASE	0x1B800 
#define SE_TIMER_BASE		0x1BA00 
#define IPC1_ADDR_BASE		0x1C000 
#define IPC2_ADDR_BASE		0x1C400
#define PMU_ADDR_BASE		0x1D000 

#define VRTC1_SEC	0x00
#define VRTC1_SEC_A	0x04
#define VRTC1_MIN	0x08
#define VRTC1_MIN_A	0x0C
#define VRTC1_HOU	0x10 
#define VRTC1_HOU_A	0x14
#define VRTC1_DAY_W	0x18
#define VRTC1_DAY_M	0x1C
#define VRTC1_MON	0x20
#define VRTC1_YEA	0x24

struct vrtc_info {
	u8 vrtc1_sec; 
	u8 vrtc1_sec_alarm; 
	u8 vrtc1_min; 
	u8 vrtc1_min_alarm; 
	u8 vrtc1_hour; 
	u8 vrtc1_hour_alarm; 
	u8 vrtc1_day_week; 
	u8 vrtc1_day_month;
	u8 vrtc1_month;
	u8 vrtc1_year; 
};

/* primitive function */
u8 intel_readb_scu_register(u32 module_addr, u32 offset);
struct vrtc_info read_vrtc_info(void);

void ipc_data_writel(u32 data, u32 offset); /* Write ipc data */
u32 ipc_read_status(void);
u8 ipc_data_readb(u32 offset); /* Read ipc byte data */
u32 ipc_data_readl(u32 offset); /* Read ipc u32 data */
int intel_scu_ipc_command(u32 cmd, u32 sub, u8 *in, u8 inlen, u32 *out, u32 outlen);
int init_scu_ipc(void);

enum intel_mid_cpu_type {
        INTEL_CPU_CHIP_NOTMID = 0,
	INTEL_MID_CPU_CHIP_LINCROFT,
        INTEL_MID_CPU_CHIP_PENWELL,
        INTEL_MID_CPU_CHIP_CLOVERVIEW,
        INTEL_MID_CPU_CHIP_TANGIER,
        INTEL_MID_CPU_CHIP_VALLEYVIEW2,
        INTEL_MID_CPU_CHIP_ANNIEDALE,
        INTEL_MID_CPU_CHIP_CARBONCANYON,
};

static inline enum intel_mid_cpu_type intel_mid_identify_cpu(void)
{
#ifdef CONFIG_X86_MRFLD
	return INTEL_MID_CPU_CHIP_TANGIER;
#else
	return INTEL_MID_CPU_CHIP_CLOVERVIEW;
#endif
}

int intel_scu_ipc_fw_update_init(void);
int intel_scu_ipc_fw_update(void);
u16 intel_scu_ipc_show_fw_version(void);
ssize_t write_dnx(char *buf, loff_t off, size_t count);
ssize_t write_ifwi(char *buf, loff_t off, size_t count);

#endif //_INTEL_SCU_IPC_H_

