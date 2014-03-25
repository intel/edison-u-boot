#include <common.h>
#include <asm/io.h>
#include <asm-generic/errno.h>
#include <intel_scu_ipc.h>

#define IPC_WWBUF_SIZE    20
#define IPC_RWBUF_SIZE    20

#define IPCMSG_PCNTRL           0xFF

#define IPC_CMD_PCNTRL_W      0
#define IPC_CMD_PCNTRL_R      1
#define IPC_CMD_PCNTRL_M      2

static int pwr_reg_rdwr(u16 * addr, u8 * data, u32 count, u32 cmd, u32 sub)
{
	int i, j, err, inlen = 0, outlen = 0;

	u8 wbuf[IPC_WWBUF_SIZE] = { };
	u8 rbuf[IPC_RWBUF_SIZE] = { };

	memset(wbuf, 0, sizeof(wbuf));

	if (intel_mid_identify_cpu() == INTEL_MID_CPU_CHIP_LINCROFT) {
		for (i = 0, j = 0; i < count; i++) {
			wbuf[inlen++] = addr[i] & 0xff;
			wbuf[inlen++] = (addr[i] >> 8) & 0xff;

			if (sub != IPC_CMD_PCNTRL_R) {
				wbuf[inlen++] = data[j++];
				outlen = 0;
			}

			if (sub == IPC_CMD_PCNTRL_M)
				wbuf[inlen++] = data[j++];
		}
	} else {
		for (i = 0; i < count; i++) {
			wbuf[inlen++] = addr[i] & 0xff;
			wbuf[inlen++] = (addr[i] >> 8) & 0xff;
		}

		if (sub == IPC_CMD_PCNTRL_R) {
			outlen = count > 0 ? ((count - 1) / 4) + 1 : 0;
		} else if (sub == IPC_CMD_PCNTRL_W) {
			if (count == 3)
				inlen += 2;

			for (i = 0; i < count; i++)
				wbuf[inlen++] = data[i] & 0xff;

			if (count == 3)
				inlen -= 2;

			outlen = 0;
		} else if (sub == IPC_CMD_PCNTRL_M) {
			wbuf[inlen++] = data[0] & 0xff;
			wbuf[inlen++] = data[1] & 0xff;
			outlen = 0;
		}		//else
		//      pr_err("IPC command not supported\n");
	}

	err = intel_scu_ipc_command(cmd, sub, wbuf, inlen,
				    (u32 *) rbuf, outlen);

	if (sub == IPC_CMD_PCNTRL_R) {
		if (intel_mid_identify_cpu() == INTEL_MID_CPU_CHIP_LINCROFT) {
			for (i = 0, j = 2; i < count; i++, j += 3)
				data[i] = rbuf[j];
		} else {
			for (i = 0; i < count; i++)
				data[i] = rbuf[i];
		}
	}

	return err;
}

int intel_scu_ipc_ioread8(u16 addr, u8 * data)
{
	return pwr_reg_rdwr(&addr, data, 1, IPCMSG_PCNTRL, IPC_CMD_PCNTRL_R);
}

//EXPORT_SYMBOL(intel_scu_ipc_ioread8);

int intel_scu_ipc_iowrite8(u16 addr, u8 data)
{
	return pwr_reg_rdwr(&addr, &data, 1, IPCMSG_PCNTRL, IPC_CMD_PCNTRL_W);
}

//EXPORT_SYMBOL(intel_scu_ipc_iowrite8);

int intel_scu_ipc_iowrite32(u16 addr, u32 data)
{
	u16 x[4] = { addr, addr + 1, addr + 2, addr + 3 };
	return pwr_reg_rdwr(x, (u8 *) & data, 4, IPCMSG_PCNTRL,
			    IPC_CMD_PCNTRL_W);
}

//EXPORT_SYMBOL(intel_scu_ipc_iowrite32);

int intel_scu_ipc_readv(u16 * addr, u8 * data, int len)
{
	if (len < 1 || len > 8)
		return -EINVAL;

	return pwr_reg_rdwr(addr, data, len, IPCMSG_PCNTRL, IPC_CMD_PCNTRL_R);
}

//EXPORT_SYMBOL(intel_scu_ipc_readv);

int intel_scu_ipc_writev(u16 * addr, u8 * data, int len)
{
	if (len < 1 || len > 4)
		return -EINVAL;

	return pwr_reg_rdwr(addr, data, len, IPCMSG_PCNTRL, IPC_CMD_PCNTRL_W);
}

//EXPORT_SYMBOL(intel_scu_ipc_writev);

int intel_scu_ipc_update_register(u16 addr, u8 bits, u8 mask)
{
	u8 data[2] = { bits, mask };
	return pwr_reg_rdwr(&addr, data, 1, IPCMSG_PCNTRL, IPC_CMD_PCNTRL_M);
}

//EXPORT_SYMBOL(intel_scu_ipc_update_register);
