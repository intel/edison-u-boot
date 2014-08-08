#ifndef _INTEL_SCU_IPC_H_
#define _INTEL_SCU_IPC_H_

/* IPC defines the following message types */
#define IPCMSG_WARM_RESET   0xF0
#define IPCMSG_COLD_RESET   0xF1
#define IPCMSG_SOFT_RESET   0xF2
#define IPCMSG_COLD_BOOT    0xF3
#define IPCMSG_GET_FW_REVISION	0xF4
#define IPCMSG_WATCHDOG_TIMER   0xF8	/* Set Kernel Watchdog Threshold */

#define IPC_ERR_NONE            0
#define IPC_ERR_CMD_NOT_SUPPORTED   1
#define IPC_ERR_CMD_NOT_SERVICED    2
#define IPC_ERR_UNABLE_TO_SERVICE   3
#define IPC_ERR_CMD_INVALID     4
#define IPC_ERR_CMD_FAILED      5
#define IPC_ERR_EMSECURITY      6

/* Command id associated with message IPCMSG_VRTC */
#define IPC_CMD_VRTC_SETTIME      1	/* Set time */
#define IPC_CMD_VRTC_SETALARM     2	/* Set alarm */
#define IPC_CMD_VRTC_SYNC_RTC     3	/* Sync MSIC/PMIC RTC to VRTC */

union ipc_ifwi_version {
	u8 raw[16];
	struct {
		u16 ifwi_minor;
		u8 ifwi_major;
		u8 hardware_id;
		u32 reserved[3];
	} fw __attribute__((packed));
} __attribute__((packed));

/* Issue commands to the SCU with or without data */
void intel_scu_ipc_send_command(u32 cmd);
int intel_scu_ipc_check_status(void);
int intel_scu_ipc_simple_command(int cmd, int sub);
void intel_scu_ipc_lock(void);
void intel_scu_ipc_unlock(void);
int intel_scu_ipc_command(u32 cmd, u32 sub, u8 * in, u8 inlen,
		u32 * out, u32 outlen);
int intel_scu_ipc_raw_cmd(u32 cmd, u32 sub, u8 * in, u8 inlen,
		u32 * out, u32 outlen, u32 dptr, u32 sptr);


void ipc_data_writel(u32 data, u32 offset);	/* Write ipc data */
u32 ipc_read_status(void);
u8 ipc_data_readb(u32 offset);	/* Read ipc byte data */
u32 ipc_data_readl(u32 offset);	/* Read ipc u32 data */
int intel_scu_ipc_command(u32 cmd, u32 sub, u8 * in, u8 inlen, u32 * out,
		u32 outlen);
int init_scu_ipc(void);

enum intel_mid_cpu_type {
	INTEL_CPU_CHIP_NOTMID = 0,
	INTEL_MID_CPU_CHIP_LINCROFT,
	INTEL_MID_CPU_CHIP_PENWELL,
	INTEL_MID_CPU_CHIP_CLOVERVIEW,
	INTEL_MID_CPU_CHIP_TANGIER,
};

static inline enum intel_mid_cpu_type intel_mid_identify_cpu(void)
{
	return INTEL_MID_CPU_CHIP_TANGIER;
}


#endif //_INTEL_SCU_IPC_H_
