#include <common.h>
#include <linux/compat.h>
#include <asm/io.h>
#include <asm-generic/errno.h>
#include <intel_scu_ipc.h>

#define IPC_MIP_BASE     0xFFFD8000	/* sram base address for mip accessing */
#define IPC_MIP_MAX_ADDR 0x1000

static void *intel_mip_base = (void *)IPC_MIP_BASE;

static int read_mip(u8 * data, int len, int offset, int issigned)
{
	int ret;
	u32 sptr, dptr, cmd, cmdid, data_off;

	dptr = offset;
	sptr = (len + 3) / 4;

	cmdid = issigned ? IPC_CMD_SMIP_RD : IPC_CMD_UMIP_RD;
	cmd = 4 << 16 | cmdid << 12 | IPCMSG_MIP_ACCESS;

	do {
		ret = intel_scu_ipc_raw_cmd(cmd, 0, NULL, 0, &data_off, 1,
					    dptr, sptr);
		if (ret == -EIO)
			mdelay(20);	//msleep(20);
	} while (ret == -EIO);

	if (!ret)
		memcpy(data, intel_mip_base + data_off, len);

	return ret;
}

int intel_scu_ipc_read_mip(u8 * data, int len, int offset, int issigned)
{
	int ret;

	/* Only SMIP read for Cloverview is supported */
	if ((intel_mid_identify_cpu() == INTEL_MID_CPU_CHIP_CLOVERVIEW)
	    && (issigned != 1))
		return -EINVAL;

	if (issigned != 1)
		return -EINVAL;

	if (!intel_mip_base)
		return -ENODEV;

	if (offset + len > IPC_MIP_MAX_ADDR)
		return -EINVAL;

	intel_scu_ipc_lock();
	ret = read_mip(data, len, offset, issigned);
	intel_scu_ipc_unlock();

	return ret;
}

//EXPORT_SYMBOL(intel_scu_ipc_read_mip);

int intel_scu_ipc_write_umip(u8 * data, int len, int offset)
{
	int ret, offset_align;
	int len_align = 0;
	u32 dptr, sptr, cmd;
	u8 *buf = NULL;

	/* Cloverview don't need UMIP access through IPC */
	if (intel_mid_identify_cpu() == INTEL_MID_CPU_CHIP_CLOVERVIEW)
		return -EINVAL;

	if (!intel_mip_base)
		return -ENODEV;

	if (offset + len > IPC_MIP_MAX_ADDR)
		return -EINVAL;

	intel_scu_ipc_lock();

	offset_align = offset & (~0x3);
	len_align = (len + (offset - offset_align) + 3) & (~0x3);

	u8 rbuf[len_align];
	if (len != len_align) {
		buf = rbuf;
		ret = read_mip(buf, len_align, offset_align, 0);
		if (ret)
			goto fail;
		memcpy(buf + offset - offset_align, data, len);
	} else {
		buf = data;
	}

	dptr = offset_align;
	sptr = len_align / 4;
	cmd = IPC_CMD_UMIP_WR << 12 | IPCMSG_MIP_ACCESS;

	memcpy(intel_mip_base, buf, len_align);

	do {
		ret = intel_scu_ipc_raw_cmd(cmd, 0, NULL, 0, NULL, 0,
					    dptr, sptr);
		if (ret == -EIO)
			mdelay(20);	//msleep(20);
	} while (ret == -EIO);

fail:
	intel_scu_ipc_unlock();

	return ret;
}

//EXPORT_SYMBOL(intel_scu_ipc_write_umip);
