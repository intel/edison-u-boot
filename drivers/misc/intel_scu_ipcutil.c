#include <common.h>
#include <asm/io.h>
#include <asm-generic/errno.h>
#include <intel_scu_ipc.h>
#include <intel_scu_ipcutil.h>
#include <malloc.h>

#ifdef CONFIG_INTEL_SCU_WATCHDOG_DEBUG
#define pr_debug(x,args...) printf(x,##args);
#define pr_err(x,args...) printf(x,##args);
#define pr_info(x,args...) printf(x,##args);
#define pr_warn(x,args...) printf(x,##args);
#else
#define pr_debug(x, args...) 
#define pr_err(x, args...) 
#define pr_info(x,args...) 
#define pr_warn(x,args...) 
#endif

#define MAX_FW_SIZE 264192

#define PMIT_RESET1_OFFSET		14
#define PMIT_RESET2_OFFSET		15

#define IPC_RESIDENCY_CMD_ID_START	0
#define IPC_RESIDENCY_CMD_ID_DUMP	2

#define SRAM_ADDR_S0IX_RESIDENCY	0xFFFF71E0
#define ALL_RESIDENCY_DATA_SIZE		12

#define DUMP_OSNIB

#define OSHOB_EXTEND_DESC_SIZE	52  /* OSHOB header+osnib+oem info: 52 bytes.*/

#define OSHOB_HEADER_MAGIC_SIZE	4   /* Size (bytes) of magic number in OSHOB */
				    /* header.                               */

#define OSHOB_MAGIC_NUMBER	"$OH$"	/* If found when reading the first   */
					/* 4 bytes of the OSOHB zone, it     */
					/* means that the new extended OSHOB */
					/* is going to be used.              */

#define OSHOB_REV_MAJ_DEFAULT	0	/* Default revision number of OSHOB. */
#define OSHOB_REV_MIN_DEFAULT	1	/* If 0.1 the default OSHOB is used  */
					/* instead of the extended one.      */

/* Defines for the SCU buffer included in OSHOB structure. */
#define OSHOB_SCU_BUF_BASE_DW_SIZE	1   /* In dwords. By default SCU     */
					    /* buffer size is 1 dword.       */

#define OSHOB_SCU_BUF_MRFLD_DW_SIZE (4*OSHOB_SCU_BUF_BASE_DW_SIZE)
					    /* In dwords. On Merrifield the  */
					    /* SCU trace buffer size is      */
					    /* 4 dwords.                     */
#define OSHOB_DEF_FABRIC_ERR_MRFLD_SIZE   50    /* In DWORDS. For Merrifield.*/
                                        /* Fabric error log size (in DWORDS).*/
                                        /* From offs 0x44 to 0x10C.          */
                                        /* Used in default OSHOB.            */

#define OSNIB_SIZE		32	/* Size (bytes) of the default OSNIB.*/

#define OSNIB_INTEL_RSVD_SIZE	24	/* Size (bytes) of Intel RESERVED in */
					/* OSNIB.                            */
#define OSNIB_OEM_RSVD_SIZE	96	/* Size (bytes) of OEM RESERVED      */
					/* in OSNIB.                         */
#define OSNIB_NVRAM_SIZE	128	/* Size (bytes) of NVRAM in OSNIB.   */

#define OSHOB_DEF_FABRIC_ERR_SIZE   50  /* In DWORDS.                        */
                                        /* Fabric error log size (in DWORDS).*/
                                        /* From offs 0x44 to 0x10C.          */
                                        /* Used in default OSHOB.            */
#define OSHOB_FABRIC_ERROR1_SIZE  12    /* 1st part of Fabric error dump     */
#define OSHOB_FABRIC_ERROR2_SIZE  9     /* 2nd part of Fabric error dump     */
#define OSHOB_RESERVED_DEBUG_SIZE 5     /* Reserved for debug                */

/* Size (bytes) of the default OSHOB structure. Includes the default OSNIB   */
/* size.                                                                     */
#define OSHOB_SIZE      (68 + (4*OSHOB_SCU_BUF_BASE_DW_SIZE) + \
                            (4*OSHOB_DEF_FABRIC_ERR_SIZE))      /* In bytes. */

#define OSHOB_MRFLD_SIZE (68 + (4*OSHOB_SCU_BUF_MRFLD_DW_SIZE) + \
                            (4*OSHOB_DEF_FABRIC_ERR_MRFLD_SIZE))/* In bytes. */

/* SCU buffer size is give in dwords. So it is x4 here to get the total      */
/* number of bytes.                                                          */

#define SCU_TRACE_HEADER_SIZE    16     /* SCU trace header                  */

#define CHAABI_DEBUG_DATA_SIZE   5      /* Reserved for chaabi debug         */

#define OSHOB_RESERVED_SIZE      184    /* Reserved                          */

struct chip_reset_event {
        int id;
        const char *reset_ev1_name;
        const char *reset_ev2_name;
};

static struct chip_reset_event chip_reset_events[] = {
        { INTEL_MID_CPU_CHIP_CARBONCANYON, "RESETSRC0", "RESETSRC1" },
        { INTEL_MID_CPU_CHIP_TANGIER, "RESETSRC0", "RESETSRC1" },
        { INTEL_MID_CPU_CHIP_CLOVERVIEW, "RESETIRQ1", "RESETIRQ2" },
        { INTEL_MID_CPU_CHIP_PENWELL, "RESETIRQ1", "RESETIRQ2" },
};

/* OSNIB allocation. */
struct scu_ipc_osnib {
	u8 target_mode;        /* Target mode.                      */
	u8 wd_count;           /* Software watchdog.                */
	u8 alarm;              /* RTC alarm.                        */
	u8 wakesrc;            /* WAKESRC.                          */
        u8 reset_ev1;          /* RESETIRQ1 or RESETSRC0.           */
        u8 reset_ev2;          /* RESETIRQ2 or RESETSRC1.           */
	u8 spare;              /* Spare.                            */
	u8 intel_reserved[OSNIB_INTEL_RSVD_SIZE]; /* INTEL RESERVED */
			       /* (offsets 7 to 20).                */
        u8 checksum;           /* CHECKSUM.                         */
	u8 oem_reserved[OSNIB_OEM_RSVD_SIZE];     /* OEM RESERVED   */
			       /* (offsets 21 to 30).               */
        u8 nvram[OSNIB_NVRAM_SIZE];               /* NVRAM          */
                               /* (offsets 128 to 255).             */
};

/* Default OSHOB allocation. */
struct scu_ipc_oshob {
	u32 scutxl;             /* SCUTxl offset position.      */
	u32 iatxl;              /* IATxl offset.                */
	u32 bocv;               /* BOCV offset.                 */
	u8 osnibr[OSNIB_SIZE];  /* OSNIB area offset.           */
	u32 pmit;               /* PMIT offset.                 */
	u32 pemmcmhki;          /* PeMMCMHKI offset.            */
	u32 osnibw_ptr;         /* OSNIB Write at offset 0x34.  */
        u32 fab_err_log[OSHOB_DEF_FABRIC_ERR_SIZE]; /* Fabric   */
                                /* error log buffer.            */
};

/* Extended OSHOB allocation. version 1.3 */
struct scu_ipc_oshob_extend {
	u32 magic;              /* MAGIC number.                           */
	u8  rev_major;          /* Revision major.                         */
	u8  rev_minor;          /* Revision minor.                         */
	u16 oshob_size;         /* OSHOB size.                             */
	u32 head_reserved;      /* OSHOB RESERVED.                         */
	u32 scutxl;             /* SCUTxl offset position.                 */
				/* If on MRFLD platform, next param may be */
				/* shifted by                              */
				/* (OSHOB_SCU_BUF_MRFLD_DW_SIZE - 1) bytes.*/
	u32 iatxl;              /* IATxl.                                  */
	u32 bocv;               /* BOCV.                                   */

	u16 intel_size;         /* Intel size (in OSNIB area).             */
	u16 oem_size;           /* OEM size (of OEM area).                 */
	u32 r_intel_ptr;        /* Read Intel pointer.                     */
	u32 w_intel_ptr;        /* Write Intel pointer.                    */
	u32 r_oem_ptr;          /* Read OEM pointer.                       */
	u32 w_oem_ptr;          /* Write OEM pointer.                      */

	u32 pmit;               /* PMIT.                       */
	u32 pemmcmhki;          /* PeMMCMHKI.                  */

	/* OSHOB as defined for CLOVERVIEW */
        u32 nvram_size;         /* NVRAM max size in bytes     */
	u32 fabricerrlog1[OSHOB_FABRIC_ERROR1_SIZE]; /* fabric error data */
	u8  vrtc_alarm_dow;     /* Alarm sync                  */
	u8  vrtc_alarm_dom;     /* Alarm sync                  */
	u8  vrtc_alarm_month;   /* Alarm sync                  */
	u8  vrtc_alarm_year;    /* Alarm sync                  */
	u32 reserved_debug[OSHOB_RESERVED_DEBUG_SIZE];/* Reserved Debug data */
	u32 reserved2;          /* Reserved                    */
	u32 fabricerrlog2[OSHOB_FABRIC_ERROR2_SIZE]; /* fabric error data2 */
	u32 sculogbufferaddr;   /* phys addr of scu log buffer   */
	u32 sculogbuffersize;   /* size of scu log buffer      */
};

/* Extended OSHOB allocation. version 1.4. */
struct scu_ipc_oshob_extend_v14 {
        u32 magic;              /* MAGIC number.                           */
        u8  rev_major;          /* Revision major.                         */
        u8  rev_minor;          /* Revision minor.                         */
        u16 oshob_size;         /* OSHOB size.                             */

        u32 scutxl;             /* SCUTxl offset position.                 */
                                /* If on MRFLD platform, next param may be */
                                /* shifted by                              */
                                /* (OSHOB_SCU_BUF_MRFLD_DW_SIZE - 1) bytes.*/
        u32 iatxl;              /* IATxl.                                  */
        u32 bocv;               /* BOCV.                                   */

        u32 osnib_ptr;          /* The unique OSNIB pointer.               */

        u32 pmit;               /* PMIT.                                   */
        u8  scutraceheader[SCU_TRACE_HEADER_SIZE];   /* SCU trace header   */
        u32 fabricerrlog[OSHOB_DEF_FABRIC_ERR_SIZE]; /* fabric error data  */
        u32 chaabidebugdata[CHAABI_DEBUG_DATA_SIZE]; /* fabric error data  */
        u32 pmuemergency;       /* pmu emergency                           */
        u32 sculogbufferaddr;   /* scu log buffer address                  */
        u32 sculogbuffersize;   /* size of scu log buffer                  */
        u32 oshob_reserved[OSHOB_RESERVED_SIZE];     /* oshob reserved     */
};

struct scu_ipc_oshob_info {
        __u32   oshob_base;     /* Base address of OSHOB. Use ioremap to     */
                                /* remap for access.                         */
        __u8    oshob_majrev;   /* Major revision number of OSHOB structure. */
        __u8    oshob_minrev;   /* Minor revision number of OSHOB structure. */
        __u16   oshob_size;     /* Total size (bytes) of OSHOB structure.    */
        __u32   scu_trace[OSHOB_SCU_BUF_BASE_DW_SIZE*4]; /* SCU trace buffer.*/
                                /* Set to max SCU buffer size (dwords) to    */
                                /* adapt to MRFLD. On other platforms, only  */
                                /* the first dword is stored and read.       */
        __u32   ia_trace;       /* IA trace buffer.                          */
        __u16   osnib_size;     /* Total size (bytes) of OSNIB structure.    */
        __u16   oemnib_size;    /* Total size (bytes) of OEMNIB area.        */
        __u32   osnibr_ptr;     /* Pointer to Intel read zone.               */
        __u32   osnibw_ptr;     /* Pointer to Intel write zone.              */
        __u32   oemnibr_ptr;    /* Pointer to OEM read zone.                 */
        __u32   oemnibw_ptr;    /* Pointer to OEM write zone.                */
        __u32   scu_trace_buf;  /* SCU extended trace buffer                 */
        __u32   scu_trace_size; /* SCU extended trace buffer size            */
        __u32   nvram_addr;     /* NV ram phys addr                          */
        __u32   nvram_size;     /* NV ram size in bytes                      */

        int (*scu_ipc_write_osnib)(u8 *data, int len, int offset);
        int (*scu_ipc_read_osnib)(u8 *data, int len, int offset);

        int platform_type;     /* Identifies the platform (list of supported */
                               /* platforms is given in intel-mid.h).        */

        u16 offs_add;          /* The additional shift bytes to consider     */
                               /* giving the offset at which the OSHOB params*/
                               /* will be read. If MRFLD it must be set to   */
                               /* take into account the extra SCU dwords.    */

};

/* Structure for OSHOB info */
static struct scu_ipc_oshob_info *oshob_info;

static unsigned int osc_clk0_mode;

int intel_scu_ipc_osc_clk(u8 clk, unsigned int khz)
{
	/* SCU IPC COMMAND(osc clk on/off) definition:
	 * ipc_wbuf[0] = clock to act on {0, 1, 2, 3}
	 * ipc_wbuf[1] =
	 * bit 0 - 1:on  0:off
	 * bit 1 - if 1, read divider setting from bits 3:2 as follows:
	 * bit [3:2] - 00: clk/1, 01: clk/2, 10: clk/4, 11: reserved
	 */
	unsigned int base_freq;
	unsigned int div;
	u8 ipc_wbuf[2];
	int ipc_ret;

	if (clk > 3)
		return -EINVAL;

	ipc_wbuf[0] = clk;
	ipc_wbuf[1] = 0;
	if (khz) {
#ifdef CONFIG_CTP_CRYSTAL_38M4
		base_freq = 38400;
#else
		base_freq = 19200;
#endif
		div = fls(base_freq / khz) - 1;
		if (div >= 3 || (1 << div) * khz != base_freq)
			return -EINVAL;	/* Allow only exact frequencies */
		ipc_wbuf[1] = 0x03 | (div << 2);
	}

	ipc_ret = intel_scu_ipc_command(IPCMSG_OSC_CLK, 0,
					ipc_wbuf, 2, NULL, 0);
 	if (ipc_ret != 0)
		pr_err("%s: failed to set osc clk(%d) output\n", __func__, clk);

	return ipc_ret;
}

/*
 * OSC_CLK_AUDIO is connected to the MSIC as well as Audience, so it should be
 * turned on if any one of them requests it to be on and it should be turned off
 * only if no one needs it on.
 */
int intel_scu_ipc_set_osc_clk0(unsigned int enable, enum clk0_mode mode)
{
	int ret = 0, clk_enable;
	static const unsigned int clk_khz = 19200;

	pr_info("set_clk0 request %s for Mode 0x%x\n",
				enable ? "ON" : "OFF", mode);

	if (mode == CLK0_QUERY) {
		ret = osc_clk0_mode;
		goto out;
	}
	if (enable) {
		/* if clock is already on, just add new user */
		if (osc_clk0_mode) {
			osc_clk0_mode |= mode;
			goto out;
		}
		osc_clk0_mode |= mode;
		pr_info("set_clk0: enabling clk, mode 0x%x\n", osc_clk0_mode);
		clk_enable = 1;
	} else {
		osc_clk0_mode &= ~mode;
		pr_info("set_clk0: disabling clk, mode 0x%x\n", osc_clk0_mode);
		/* others using the clock, cannot turn it of */
		if (osc_clk0_mode)
			goto out;
		clk_enable = 0;
	}
	pr_info("configuring OSC_CLK_AUDIO now\n");
	ret = intel_scu_ipc_osc_clk(OSC_CLK_AUDIO, clk_enable ? clk_khz : 0);
out:
	return ret;
}

#define MSIC_VPROG1_CTRL        0xD6
#define MSIC_VPROG2_CTRL        0xD7

#define MSIC_VPROG2_ON          0x36 /*1.200V and Auto mode*/
#define MSIC_VPROG1_ON          0xF6 /*2.800V and Auto mode*/
#define MSIC_VPROG_OFF          0x24 /*1.200V and OFF*/

/* Defines specific of MRFLD platform (CONFIG_X86_MRFLD). */
#define MSIC_VPROG1_MRFLD_CTRL	0xAC
#define MSIC_VPROG2_MRFLD_CTRL	0xAD

#define MSIC_VPROG1_MRFLD_ON	0xC1	/* 2.80V */
#define MSIC_VPROG2_MRFLD_ON	0xC1	/* 2.80V */
#define MSIC_VPROG_MRFLD_OFF	0	/* OFF */
/* End of MRFLD specific.*/

/* Helpers to turn on/off msic vprog1 and vprog2 */
int intel_scu_ipc_msic_vprog1(int on)
{
        if ((oshob_info->platform_type == INTEL_MID_CPU_CHIP_TANGIER) ||
                (oshob_info->platform_type == INTEL_MID_CPU_CHIP_ANNIEDALE) ||
                (oshob_info->platform_type == INTEL_MID_CPU_CHIP_CARBONCANYON))
                return intel_scu_ipc_iowrite8(MSIC_VPROG1_MRFLD_CTRL,
                        on ? MSIC_VPROG1_MRFLD_ON : MSIC_VPROG_MRFLD_OFF);
        else
                return intel_scu_ipc_iowrite8(MSIC_VPROG1_CTRL,
                        on ? MSIC_VPROG1_ON : MSIC_VPROG_OFF);
}

int intel_scu_ipc_msic_vprog2(int on)
{
        if ((oshob_info->platform_type == INTEL_MID_CPU_CHIP_TANGIER) ||
                (oshob_info->platform_type == INTEL_MID_CPU_CHIP_ANNIEDALE) ||
                (oshob_info->platform_type == INTEL_MID_CPU_CHIP_CARBONCANYON))
                return intel_scu_ipc_iowrite8(MSIC_VPROG2_MRFLD_CTRL,
                        on ? MSIC_VPROG2_MRFLD_ON : MSIC_VPROG_MRFLD_OFF);
        else
                return intel_scu_ipc_iowrite8(MSIC_VPROG2_CTRL,
                        on ? MSIC_VPROG2_ON : MSIC_VPROG_OFF);
}

#define IPCMSG_GET_HOBADDR      0xE5

int intel_scu_ipc_read_oshob(u8 *data, int len, int offset)
{
	int ret = 0, i;
	void __iomem *oshob_addr;
	u32 oshob_base;
	u8 *ptr = data;

	ret = intel_scu_ipc_command(IPCMSG_GET_HOBADDR, 0, NULL, 0,
			&oshob_base, 1);

	if (ret < 0) {
		pr_err("ipc_read_oshob cmd failed!!\n");
		goto exit;
	}

	/* At this stage, we still don't know which OSHOB type (old or new) */
	/* an be used, and the size of resource to be remapped depends on   */
	/* the type of OSHOB structure to be used.                          */
	/* So just remap the minimum size to get the needed bytes of the    */
	/* OSHOB zone.                                                      */
	oshob_addr = (void __iomem *) oshob_base;

	if (!oshob_addr) {
		pr_err("ipc_read_oshob: addr ioremap failed!\n");
		ret = -ENOMEM;
		goto exit;
	}

	
	for (i = 0; i < len; i = i+1) {
		*ptr = readb(oshob_addr + offset + i);
		pr_debug("addr(remapped)=%8x, offset=%2x, value=%2x\n",
			(u32)(oshob_addr + i),
			offset + i, *ptr);
		ptr++;
	}

exit:
	return ret;
}

/* This function is used for the default OSNIB. */
int intel_scu_ipc_read_osnib(u8 *data, int len, int offset)
{
	int i, ret = 0;
	u32 osnibw_ptr;
	u32 oshob_base;
	u8 *ptr, check = 0;
	u16 struct_offs;
	void __iomem *oshob_addr, *osnibr_addr, *osnibw_addr;

	pr_debug("OSHOB base addr value is %x\n", oshob_info->oshob_base);

	ret = intel_scu_ipc_command(IPCMSG_GET_HOBADDR, 0, NULL, 0,
			&oshob_base, 1);

	if (ret < 0) {
		pr_err("ipc_read_oshob cmd failed!!\n");
		goto exit;
	}

	/* At this stage, we still don't know which OSHOB type (old or new) */
	/* an be used, and the size of resource to be remapped depends on	*/
	/* the type of OSHOB structure to be used.							*/
	/* So just remap the minimum size to get the needed bytes of the	*/
	/* OSHOB zone.														*/
	oshob_addr = (void __iomem *) oshob_base;
 
	if (!oshob_addr) {
		pr_err("ioremap failed!\n");
		ret = -ENOMEM;
		goto exit;
	}

	struct_offs = offsetof(struct scu_ipc_oshob, osnibr) +
			    oshob_info->offs_add;
	osnibr_addr = oshob_addr + struct_offs;

	if (!osnibr_addr) {
		pr_err("Bad osnib address!\n");
		ret = -EFAULT;
		goto exit;
	}

	pr_debug("OSNIB read addr (remapped) is %x\n",
						(unsigned int)osnibr_addr);

	/* Make a chksum verification for osnib */
	for (i = 0; i < oshob_info->osnib_size; i++)
		check += readb(osnibr_addr + i);
	if (check) {
		pr_err("WARNING!!! osnib chksum verification faild, reset all osnib data!\n");
		struct_offs = offsetof(struct scu_ipc_oshob, osnibw_ptr) +
				    oshob_info->offs_add;
		osnibw_ptr = readl(oshob_addr + struct_offs);
		osnibw_addr = (void __iomem *)osnibw_ptr;
		
		if (osnibw_addr) {
			for (i = 0; i < oshob_info->osnib_size; i++)
				writeb(0, osnibw_addr + i);

			intel_scu_ipc_raw_cmd(IPCMSG_WRITE_OSNIB,
				0, NULL, 0, NULL, 0, 0, 0xFFFFFFFF);
 		}
	}

	ptr = data;
	for (i = 0; i < len; i++) {
		*ptr = readb(osnibr_addr + offset + i);
		pr_debug("addr(remapped)=%8x, offset=%2x, value=%2x\n",
			(u32)(osnibr_addr+offset+i), offset+i, *ptr);
		ptr++;
	}

exit:
	return ret;
}

/* This function is used for the default OSNIB. */
int intel_scu_ipc_write_osnib(u8 *data, int len, int offset)
{
	int i;
	int ret = 0;
	u32 osnibw_ptr;
	u32 oshob_base;
	u8 osnib_data[oshob_info->osnib_size];
	u8 check = 0, chksum = 0;
	u16 struct_offs;
	void __iomem *oshob_addr, *osnibw_addr, *osnibr_addr;

	pr_debug("OSHOB base addr value is 0x%8x\n", oshob_info->oshob_base);

	intel_scu_ipc_lock();

	ret = intel_scu_ipc_command(IPCMSG_GET_HOBADDR, 0, NULL, 0,
			&oshob_base, 1);

	if (ret < 0) {
		pr_err("ipc_read_oshob cmd failed!!\n");
		goto exit;
	}

	/* At this stage, we still don't know which OSHOB type (old or new) */
	/* an be used, and the size of resource to be remapped depends on	*/
	/* the type of OSHOB structure to be used.							*/
	/* So just remap the minimum size to get the needed bytes of the	*/
	/* OSHOB zone.														*/
	oshob_addr = (void __iomem *) oshob_base;

	if (!oshob_addr) {
		pr_err("ioremap failed!\n");
		ret = -ENOMEM;
		goto exit;
	}

	/*Dump osnib data for generate chksum */
	struct_offs = offsetof(struct scu_ipc_oshob, osnibr) +
			    oshob_info->offs_add;
	osnibr_addr = oshob_addr + struct_offs;

	pr_debug("OSNIB read addr (remapped) in OSHOB at %x\n",
						(unsigned int)osnibr_addr);

	for (i = 0; i < oshob_info->osnib_size; i++) {
		osnib_data[i] = readb(osnibr_addr + i);
		check += osnib_data[i];
	}
	memcpy(osnib_data + offset, data, len);

	if (check) {
		pr_err("WARNING!!! OSNIB data chksum verification FAILED!\n");
	} else {
		/* generate chksum */
		for (i = 0; i < oshob_info->osnib_size - 1; i++)
			chksum += osnib_data[i];
		osnib_data[oshob_info->osnib_size - 1] = ~chksum + 1;
	}

	struct_offs = offsetof(struct scu_ipc_oshob, osnibw_ptr) +
			    oshob_info->offs_add;
	osnibw_ptr = readl(oshob_addr + struct_offs);
	if (osnibw_ptr == 0) { /* workaround here for BZ 2914 */
		osnibw_ptr = 0xFFFF3400;
		pr_err("ERR: osnibw ptr from oshob is 0, manually set it here\n");
	}

	pr_debug("POSNIB write address: %x\n", osnibw_ptr);

	osnibw_addr = (void __iomem *)osnibw_ptr;

	if (!osnibw_addr) {
		pr_err("ioremap failed!\n");
		ret = -ENOMEM;
		goto unmap_oshob_addr;
	}

	for (i = 0; i < oshob_info->osnib_size; i++)
		writeb(*(osnib_data + i), (osnibw_addr + i));

	ret = intel_scu_ipc_raw_cmd(IPCMSG_WRITE_OSNIB, 0, NULL, 0, NULL,
			0, 0, 0xFFFFFFFF);
	if (ret < 0)
		pr_err("ipc_write_osnib failed!!\n");

unmap_oshob_addr:
exit:
	intel_scu_ipc_unlock();

	return ret;
}

/* This function is used for the extended OSHOB/OSNIB. */
int intel_scu_ipc_read_osnib_extend(u8 *data, int len, int offset)
{
	int i, ret = 0;
	u8 *ptr, check = 0;
	void __iomem *oshob_addr, *osnibr_addr, *osnibw_addr;
	u32 sptr_dw_mask;

	oshob_addr = (void __iomem *)oshob_info->oshob_base;

	if (!oshob_addr) {
		pr_err("ipc_read_osnib_extend: ioremap failed!\n");
		ret = -ENOMEM;
		goto exit;
	}

	pr_debug(
		"ipc_read_osnib_extend: remap OSNIB addr=0x%x size %d\n",
		oshob_info->osnibr_ptr, oshob_info->osnib_size);

	osnibr_addr = (void __iomem *)oshob_info->osnibr_ptr;

	if (!osnibr_addr) {
		pr_err("ipc_read_osnib_extend: ioremap of osnib failed!\n");
		ret = -ENOMEM;
		goto unmap_oshob_addr;
	}

	/* Make a chksum verification for osnib */
	for (i = 0; i < oshob_info->osnib_size; i++)
		check += readb(osnibr_addr + i);

	if (check) {
		pr_err("ipc_read_osnib_extend: WARNING!!! osnib chksum verification faild, reset all osnib data!\n");
		pr_debug(
			"ipc_read_osnib_extend: remap osnibw ptr addr=0x%x size %d\n",
			oshob_info->osnibw_ptr, oshob_info->osnib_size);

		osnibw_addr = (void __iomem *)oshob_info->osnibw_ptr;

		if (!osnibw_addr) {
			pr_err("ipc_read_osnib_extend: cannot remap osnib write ptr\n");
			goto unmap_oshob_addr;
		}

		for (i = 0; i < oshob_info->osnib_size; i++)
			writeb(0, osnibw_addr + i);

		/* Send command. The mask to be written identifies which      */
		/* double words of the OSNIB osnib_size bytes will be written.*/
		/* So the mask is coded on 4 bytes.                           */
		sptr_dw_mask = 0xFFFFFFFF;
		intel_scu_ipc_raw_cmd(RP_WRITE_OSNIB,
			0, NULL, 0, NULL, 0, 0, sptr_dw_mask);
	}

	ptr = data;
	pr_debug("ipc_read_osnib_extend: OSNIB content:\n");
	for (i = 0; i < len; i++) {
		*ptr = readb(osnibr_addr + offset + i);
		pr_debug("addr(remapped)=%8x, offset=%2x, value=%2x\n",
			(u32)(osnibr_addr+offset+i), offset+i, *ptr);
		ptr++;
	}

unmap_oshob_addr:
exit:
	return ret;
}

/* This function is used for the extended OSHOB/OSNIB. */
int intel_scu_ipc_write_osnib_extend(u8 *data, int len, int offset)
{
	int i;
	int ret = 0;
	u8 *posnib_data, *ptr;
	u8 check = 0, chksum = 0;
	void __iomem *oshob_addr, *osnibw_addr, *osnibr_addr;
	u32 sptr_dw_mask;

	intel_scu_ipc_lock();

	pr_debug(
		"ipc_write_osnib_extend: remap OSHOB addr 0x%8x size %d\n",
		oshob_info->oshob_base, oshob_info->oshob_size);

	oshob_addr = (void __iomem *) oshob_info->oshob_base;
	
	if (!oshob_addr) {
		pr_err("ipc_write_osnib_extend: ioremap failed!\n");
		ret = -ENOMEM;
		goto exit;
	}

	osnibr_addr = (void __iomem *)oshob_info->osnibr_ptr;

	if (!osnibr_addr) {
		pr_err("ipc_write_osnib_extend: ioremap of osnib failed!\n");
		ret = -ENOMEM;
		goto exit;
	}

	/* Dump osnib data for generate chksum */
	posnib_data = malloc(oshob_info->osnib_size);

	if (posnib_data == NULL) {
		pr_err("ipc_write_osnib_extend: The buffer for getting OSNIB is NULL\n");
		ret = -EFAULT;
		goto unmap_oshob_addr;
	}

	ptr = posnib_data;
	for (i = 0; i < oshob_info->osnib_size; i++) {
		*ptr = readb(osnibr_addr + i);
		check += *ptr;
		ptr++;
	}

	memcpy(posnib_data + offset, data, len);

	if (check) {
		pr_err("ipc_write_osnib_extend: WARNING!!! OSNIB data chksum verification FAILED!\n");
	} else {
		/* generate chksum.  */
		pr_debug("ipc_write_osnib_extend: generating checksum\n");
		for (i = 0; i < oshob_info->osnib_size - 1; i++)
			chksum += *(posnib_data + i);
		/* Fill checksum at the CHECKSUM offset place in OSNIB. */
		*(posnib_data +
		    offsetof(struct scu_ipc_osnib, checksum)) = ~chksum + 1;
	}

	pr_debug(
		"ipc_write_osnib_extend: remap osnibw ptr addr=0x%x size %d\n",
		oshob_info->osnibw_ptr, oshob_info->osnib_size);

	osnibw_addr = (void __iomem *)oshob_info->osnibw_ptr;

	if (!osnibw_addr) {
		pr_err("scu_ipc_write_osnib_extend: ioremap failed!\n");
		ret = -ENOMEM;
		goto exit_osnib;
	}

	for (i = 0; i < oshob_info->osnib_size; i++)
		writeb(*(posnib_data + i), (osnibw_addr + i));

	/* Send command. The mask to be written identifies which            */
	/* double words of the OSNIB osnib_size bytes will be written.*/
	/* So the mask is coded on 4 bytes.                                 */
	sptr_dw_mask = 0xFFFFFFFF;
	ret = intel_scu_ipc_raw_cmd(RP_WRITE_OSNIB, 0, NULL, 0,
			NULL, 0, 0, sptr_dw_mask);
	if (ret < 0)
		pr_err("scu_ipc_write_osnib_extend: ipc_write_osnib failed!!\n");

exit_osnib:
unmap_oshob_addr:
	free(posnib_data);
exit:
	intel_scu_ipc_unlock();

	return ret;
}

/*
 * This writes the reboot reason in the OSNIB (factor and avoid any overlap)
 */
int intel_scu_ipc_write_osnib_rr(u8 rr)
{
	pr_info("intel_scu_ipc_write_osnib_rr: reboot reason %x\n", rr);

	return oshob_info->scu_ipc_write_osnib(
			&rr,
			1,
			offsetof(struct scu_ipc_osnib, target_mode));
}

/*
 * This reads the reboot reason from the OSNIB (factor)
 */
int intel_scu_ipc_read_osnib_rr(u8 *rr)
{
	pr_debug("intel_scu_ipc_read_osnib_rr: read reboot reason\n");
	return oshob_info->scu_ipc_read_osnib(
			rr,
			1,
			offsetof(struct scu_ipc_osnib, target_mode));
}

int intel_scu_ipc_read_oshob_extend_param(void __iomem *poshob_addr)
{
	u16 struct_offs;
	int buff_size;

	/* Get defined OSNIB space size. */
	oshob_info->osnib_size = readw(
			    poshob_addr +
			    offsetof(struct scu_ipc_oshob_extend, intel_size));

	if (oshob_info->osnib_size == 0) {
		pr_err("ipc_read_oshob_extend_param: OSNIB size is null!\n");
		return -EFAULT;
	}

	/* Get defined OEM space size. */
	oshob_info->oemnib_size = readw(
			    poshob_addr +
			    offsetof(struct scu_ipc_oshob_extend, oem_size));

	if (oshob_info->oemnib_size == 0) {
		pr_err("ipc_read_oshob_extend_param: OEMNIB size is null!\n");
		return -EFAULT;
	}

	/* Set SCU and IA trace buffers. Size calculated in bytes here. */
	if (oshob_info->platform_type == INTEL_MID_CPU_CHIP_TANGIER)
		buff_size = OSHOB_SCU_BUF_MRFLD_DW_SIZE*4;
	else
		buff_size = OSHOB_SCU_BUF_BASE_DW_SIZE*4;

	intel_scu_ipc_read_oshob(
		(u8 *)(oshob_info->scu_trace),
		buff_size,
		offsetof(struct scu_ipc_oshob_extend, scutxl));

	struct_offs = offsetof(struct scu_ipc_oshob_extend, iatxl) +
			    oshob_info->offs_add;
	oshob_info->ia_trace = readl(poshob_addr + struct_offs);

	/* Set pointers */
	struct_offs = offsetof(struct scu_ipc_oshob_extend, r_intel_ptr) +
			    oshob_info->offs_add;
	oshob_info->osnibr_ptr = readl(poshob_addr + struct_offs);

	if (!oshob_info->osnibr_ptr) {
		pr_err("ipc_read_oshob_extend_param: R_INTEL_POINTER is NULL!\n");
		return -ENOMEM;
	}

	struct_offs = offsetof(struct scu_ipc_oshob_extend, w_intel_ptr) +
			    oshob_info->offs_add;
	oshob_info->osnibw_ptr = readl(poshob_addr + struct_offs);

	if (oshob_info->osnibw_ptr == 0) {
		/* workaround here for BZ 2914 */
		oshob_info->osnibw_ptr = 0xFFFF3400;
		pr_err(
		    "ipc_read_oshob_extend_param: ERR: osnibw from oshob is 0, manually set it here\n");
	}

        pr_info("(extend oshob) osnib read ptr = 0x%8x\n",
                        oshob_info->osnibr_ptr);
        pr_info("(extend oshob) osnib write ptr = 0x%8x\n",
                        oshob_info->osnibw_ptr);

	struct_offs = offsetof(struct scu_ipc_oshob_extend, r_oem_ptr) +
			    oshob_info->offs_add;
	oshob_info->oemnibr_ptr = readl(poshob_addr + struct_offs);

	if (!oshob_info->oemnibr_ptr) {
		pr_err("ipc_read_oshob_extend_param: R_OEM_POINTER is NULL!\n");
		return -ENOMEM;
	}

	struct_offs = offsetof(struct scu_ipc_oshob_extend, w_oem_ptr) +
			    oshob_info->offs_add;
	oshob_info->oemnibw_ptr = readl(poshob_addr + struct_offs);

	if (!oshob_info->oemnibw_ptr) {
		pr_err("ipc_read_oshob_extend_param: W_OEM_POINTER is NULL!\n");
		return -ENOMEM;
	}

	oshob_info->scu_ipc_write_osnib =
					&intel_scu_ipc_write_osnib_extend;
	oshob_info->scu_ipc_read_osnib =
					&intel_scu_ipc_read_osnib_extend;

	pr_info(
		"Using EXTENDED OSHOB structure size = %d bytes\n",
		oshob_info->oshob_size);
	pr_info(
		"OSNIB Intel size = %d bytes OEMNIB size = %d bytes\n",
		oshob_info->osnib_size, oshob_info->oemnib_size);

	if (oshob_info->platform_type == INTEL_MID_CPU_CHIP_CLOVERVIEW) {
		if ((oshob_info->oshob_majrev >= 1) &&
		    (oshob_info->oshob_minrev >= 1)) {
			/* CLVP and correct version of the oshob. */
			oshob_info->scu_trace_buf =
				readl(poshob_addr +
				      offsetof(struct scu_ipc_oshob_extend,
					       sculogbufferaddr));
			oshob_info->scu_trace_size =
				readl(poshob_addr +
				      offsetof(struct scu_ipc_oshob_extend,
					       sculogbuffersize));
		}
	}
	return 0;
}

int intel_scu_ipc_read_oshob_extend_param_v14(void __iomem *poshob_addr)
{
        u16 struct_offs;
        int buff_size;

        /* set intel OSNIB space size. */
        oshob_info->osnib_size = OSNIB_SIZE;

        /* set OEM OSNIB space size. */
        oshob_info->oemnib_size = OSNIB_OEM_RSVD_SIZE;

        /* Set SCU and IA trace buffers. Size calculated in bytes here. */
        if (oshob_info->platform_type == INTEL_MID_CPU_CHIP_TANGIER)
                buff_size = OSHOB_SCU_BUF_MRFLD_DW_SIZE*4;
        else
                buff_size = OSHOB_SCU_BUF_BASE_DW_SIZE*4;

        intel_scu_ipc_read_oshob(
                (u8 *)(oshob_info->scu_trace),
                buff_size,
                offsetof(struct scu_ipc_oshob_extend_v14, scutxl));

        struct_offs = offsetof(struct scu_ipc_oshob_extend_v14, iatxl) +
                            oshob_info->offs_add;
        oshob_info->ia_trace = readl(poshob_addr + struct_offs);

        /* Set pointers */
        struct_offs = offsetof(struct scu_ipc_oshob_extend_v14, osnib_ptr) +
                            oshob_info->offs_add;
        oshob_info->osnibr_ptr = readl(poshob_addr + struct_offs);

        if (!oshob_info->osnibr_ptr) {
                pr_err("ipc_read_oshob_extend_param_v14: R_INTEL_POINTER is NULL!\n");
                return -ENOMEM;
        }

        /* write and read pointer are the same */
        oshob_info->osnibw_ptr = oshob_info->osnibr_ptr;

        pr_info("(latest extend oshob) osnib ptr = 0x%8x\n",
                oshob_info->osnibr_ptr);

        /* OEM NIB point at offset OSNIB_SIZE */
        oshob_info->oemnibr_ptr = oshob_info->osnibr_ptr + OSNIB_SIZE;

        /* write and read pinter are the same */
        oshob_info->oemnibw_ptr = oshob_info->oemnibr_ptr;

        /* we use tha same function for all extended OSHOB structure */
        oshob_info->scu_ipc_write_osnib =
                                        &intel_scu_ipc_write_osnib_extend;
        oshob_info->scu_ipc_read_osnib =
                                        &intel_scu_ipc_read_osnib_extend;

        pr_info(
                "Using latest extended oshob structure size = %d bytes\n",
                oshob_info->oshob_size);
        pr_info(
                "OSNIB Intel size = %d bytes OEMNIB size = %d bytes\n",
                oshob_info->osnib_size, oshob_info->oemnib_size);

        struct_offs = offsetof(struct scu_ipc_oshob_extend_v14,
                            sculogbufferaddr) + oshob_info->offs_add;
        oshob_info->scu_trace_buf = readl(poshob_addr + struct_offs);

        struct_offs = offsetof(struct scu_ipc_oshob_extend_v14,
                            sculogbuffersize) + oshob_info->offs_add;
        oshob_info->scu_trace_size = readl(poshob_addr + struct_offs);

        /* NVRAM after Intel and OEM OSNIB */
        oshob_info->nvram_addr = oshob_info->oemnibr_ptr + OSNIB_OEM_RSVD_SIZE;
        oshob_info->nvram_size = OSNIB_NVRAM_SIZE;

        return 0;
}

int intel_scu_ipc_read_oshob_def_param(void __iomem *poshob_addr)
{
	u16 struct_offs;
	int ret = 0;
	int buff_size;

	oshob_info->oshob_majrev = OSHOB_REV_MAJ_DEFAULT;
	oshob_info->oshob_minrev = OSHOB_REV_MIN_DEFAULT;
	oshob_info->osnib_size = OSNIB_SIZE;
	oshob_info->oemnib_size = 0;

	/* Set OSHOB total size */
	if (oshob_info->platform_type == INTEL_MID_CPU_CHIP_TANGIER)
		oshob_info->oshob_size = OSHOB_MRFLD_SIZE;
	else
		oshob_info->oshob_size = OSHOB_SIZE;

	/* Set SCU and IA trace buffers. Size calculated in bytes here. */
	if (oshob_info->platform_type == INTEL_MID_CPU_CHIP_TANGIER)
		buff_size = OSHOB_SCU_BUF_MRFLD_DW_SIZE*4;
	else
		buff_size = OSHOB_SCU_BUF_BASE_DW_SIZE*4;

	ret = intel_scu_ipc_read_oshob(
		(u8 *)(oshob_info->scu_trace),
		buff_size,
		offsetof(struct scu_ipc_oshob, scutxl));

	if (ret != 0) {
		pr_err("Cannot get scutxl data from OSHOB\n");
		return ret;
	}

	struct_offs = offsetof(struct scu_ipc_oshob, iatxl) +
			    oshob_info->offs_add;
	oshob_info->ia_trace = readl(poshob_addr + struct_offs);

	oshob_info->scu_ipc_write_osnib =
					&intel_scu_ipc_write_osnib;
	oshob_info->scu_ipc_read_osnib =
					&intel_scu_ipc_read_osnib;

	struct_offs = offsetof(struct scu_ipc_oshob, osnibr) +
			    oshob_info->offs_add;
	oshob_info->osnibr_ptr = (unsigned long)(poshob_addr + struct_offs);

	pr_info("Using DEFAULT OSHOB structure size = %d bytes\n",
					oshob_info->oshob_size);

	pr_debug("Using DEFAULT OSHOB structure OSNIB read ptr %x\n",
		oshob_info->osnibr_ptr);

	return ret;
}

int intel_scu_ipc_read_oshob_info(void)
{
	int i, ret = 0;
	u32 oshob_base = 0;
	void __iomem *oshob_addr;
	char oshob_magic[4];

	ret = intel_scu_ipc_command(IPCMSG_GET_HOBADDR, 0, NULL, 0,
			&oshob_base, 1);
	if (ret < 0) {
		pr_err("ipc_read_oshob cmd failed!!\n");
		goto exit;
	}

	/* At this stage, we still don't know which OSHOB type (old or new) */
	/* an be used, and the size of resource to be remapped depends on   */
	/* the type of OSHOB structure to be used.                          */
	/* So just remap the minimum size to get the needed bytes of the    */
	/* OSHOB zone.                                                      */
	oshob_addr = (void __iomem *) oshob_base;

	if (!oshob_addr) {
		pr_err("oshob addr ioremap failed!\n");
		ret = -ENOMEM;
		goto exit;
	}

	pr_info("(oshob) base addr = 0x%8x\n", oshob_base);

	oshob_info->oshob_base = oshob_base;

	oshob_info->platform_type = intel_mid_identify_cpu();

	/*
	 * Buffer is allocated using kmalloc. Memory is not initialized and
	 * these fields are not updated in all the branches.
	 */
	oshob_info->scu_trace_buf = 0;
	oshob_info->scu_trace_size = 0;

	if (oshob_info->platform_type == INTEL_MID_CPU_CHIP_TANGIER) {
		pr_info("(oshob) identified platform = INTEL_MID_CPU_CHIP_TANGIER\n");

		/* By default we already have 1 dword reserved in the OSHOB */
		/* structures for SCU buffer. For Merrifield, SCU size to   */
		/* consider is OSHOB_SCU_BUF_MRFLD_DW_SIZE dwords. So with  */
		/* Merrifield, when calculating structures offsets, we have */
		/* to add (OSHOB_SCU_BUF_MRFLD_DW_SIZE - 1) dwords, with    */
		/* the offsets calculated in bytes.                         */
		oshob_info->offs_add = (OSHOB_SCU_BUF_MRFLD_DW_SIZE - 1)*4;
	} else
		oshob_info->offs_add = 0;

	pr_debug("(oshob) additional offset = 0x%x\n", oshob_info->offs_add);

	/* Extract magic number that will help identifying the good OSHOB  */
	/* that is going to be used.                                       */
	for (i = 0; i < OSHOB_HEADER_MAGIC_SIZE; i = i+1)
		oshob_magic[i] = readb(oshob_addr + i);

	pr_debug("(oshob) OSHOB magic = %x %x %x %x\n",
		oshob_magic[0], oshob_magic[1], oshob_magic[2], oshob_magic[3]);

	if (strncmp(oshob_magic, OSHOB_MAGIC_NUMBER,
		    OSHOB_HEADER_MAGIC_SIZE) == 0) {
                /* Get OSHOB version and size which are commoon to all */
                /* extended OSHOB structure. */
                oshob_info->oshob_majrev = readb(oshob_addr +
                        offsetof(struct scu_ipc_oshob_extend, rev_major));
                oshob_info->oshob_minrev = readb(oshob_addr +
                        offsetof(struct scu_ipc_oshob_extend, rev_minor));
                oshob_info->oshob_size = readw(oshob_addr +
                        offsetof(struct scu_ipc_oshob_extend, oshob_size));

                pr_info("(oshob) oshob version = %x.%x\n",
                        oshob_info->oshob_majrev, oshob_info->oshob_minrev);

                if ((oshob_info->oshob_majrev >= 1) &&
                    (oshob_info->oshob_minrev >= 4)) {
                        if (intel_scu_ipc_read_oshob_extend_param_v14(
                                        oshob_addr) != 0) {
                                ret = -EFAULT;
                                goto unmap_oshob;
                        }
                } else {
                        if (intel_scu_ipc_read_oshob_extend_param(
                                        oshob_addr) != 0) {
                                ret = -EFAULT;
                                goto unmap_oshob;
                        }
                }

                if ((oshob_info->platform_type ==
                                INTEL_MID_CPU_CHIP_TANGIER) ||
                        (oshob_info->platform_type ==
                                INTEL_MID_CPU_CHIP_ANNIEDALE) ||
                        (oshob_info->platform_type ==
                                INTEL_MID_CPU_CHIP_CARBONCANYON)) {
                        pr_debug("(extend oshob) SCU buffer size is %d bytes\n",
                                OSHOB_SCU_BUF_MRFLD_DW_SIZE*4);
                } else {
                        pr_debug("(extend oshob) SCU buffer size is %d bytes\n",
                                OSHOB_SCU_BUF_BASE_DW_SIZE*4);
                }
	} else {
		ret = intel_scu_ipc_read_oshob_def_param(oshob_addr);

		if (oshob_info->platform_type == INTEL_MID_CPU_CHIP_TANGIER) {
			pr_info("(default oshob) SCU buffer size is %d bytes\n",
				OSHOB_SCU_BUF_MRFLD_DW_SIZE*4);
		} else {
			pr_info("(default oshob) SCU buffer size is %d bytes\n",
				OSHOB_SCU_BUF_BASE_DW_SIZE*4);
		}
	}

unmap_oshob:
exit:
	return ret;
}

/*
 * This writes the OEMNIB buffer in the internal RAM of the SCU.
 */
int intel_scu_ipc_write_oemnib(u8 *oemnib, int len, int offset)
{
	int i;
	int ret = 0;
	void __iomem *oshob_addr, *oemnibw_addr;
	u32 sptr_dw_mask;

	if (oemnib == NULL) {
		pr_err("ipc_write_oemnib: passed buffer for writting OEMNIB is NULL\n");
		return -EINVAL;
	}

	intel_scu_ipc_lock();

	pr_debug("ipc_write_oemnib: remap OSHOB addr 0x%8x size %d\n",
		oshob_info->oshob_base, oshob_info->oshob_size);

	oshob_addr = (void __iomem *)oshob_info->oshob_base;

	if (!oshob_addr) {
		pr_err("ipc_write_oemnib: ioremap failed!\n");
		ret = -ENOMEM;
		goto exit;
	}

	if ((len == 0) || (len > oshob_info->oemnib_size)) {
		pr_err(
			"ipc_write_oemnib: bad OEMNIB data length (%d) to write (max=%d bytes)\n",
			    len, oshob_info->oemnib_size);
		ret = -EINVAL;
		goto unmap_oshob_addr;
	}

	/* offset shall start at 0 from the OEMNIB base address and shall */
	/* not exceed the OEMNIB allowed size.                            */
	if ((offset < 0) || (offset >= oshob_info->oemnib_size) ||
	    (len + offset > oshob_info->oemnib_size)) {
		pr_err(
			"ipc_write_oemnib: Bad OEMNIB data offset/len for writing (offset=%d , len=%d)\n",
			offset, len);
		ret = -EINVAL;
		goto unmap_oshob_addr;
	}

	pr_debug("ipc_write_oemnib: POEMNIB remap oemnibw ptr 0x%x size %d\n",
		oshob_info->oemnibw_ptr, oshob_info->oemnib_size);

	oemnibw_addr = (void __iomem *)oshob_info->oemnibw_ptr;
	
	if (!oemnibw_addr) {
		pr_err("ipc_write_oemnib: ioremap failed!\n");
		ret = -ENOMEM;
		goto unmap_oshob_addr;
	}

	for (i = 0; i < len; i++)
		writeb(*(oemnib + i), (oemnibw_addr + offset + i));

	/* Send command. The mask to be written identifies which double */
	/* words of the OSNIB oemnib_size bytes will be written.        */
	/* So the mask is coded on 4 bytes.                             */
	sptr_dw_mask = 0xFFFFFFFF;
        if ((oshob_info->oshob_majrev >= 1) &&
            (oshob_info->oshob_minrev >= 4)) {
                sptr_dw_mask = 0xFFFFFFFF;
                /* OEM NIB lies on region 1, 2, and 3 */
                ret = intel_scu_ipc_raw_cmd(RP_WRITE_OSNIB, 0, NULL, 0,
                                NULL, 0, 1, sptr_dw_mask);
                if (ret < 0) {
                        pr_err("ipc_write_oemnib: ipc_write_osnib failed!!\n");
                        goto unmap_oemnibw_addr;
                }
                ret = intel_scu_ipc_raw_cmd(RP_WRITE_OSNIB, 0, NULL, 0,
                                NULL, 0, 2, sptr_dw_mask);
                if (ret < 0) {
                        pr_err("ipc_write_oemnib: ipc_write_osnib failed!!\n");
                        goto unmap_oemnibw_addr;
                }
                ret = intel_scu_ipc_raw_cmd(RP_WRITE_OSNIB, 0, NULL, 0,
                                NULL, 0, 3, sptr_dw_mask);
                if (ret < 0) {
                        pr_err("ipc_write_oemnib: ipc_write_osnib failed!!\n");
                        goto unmap_oemnibw_addr;
                }
        } else {
                ret = intel_scu_ipc_raw_cmd(RP_WRITE_OEMNIB, 0, NULL, 0,
                                NULL, 0, 0, sptr_dw_mask);
                if (ret < 0) {
                        pr_err("ipc_write_oemnib: ipc_write_osnib failed!!\n");
                        goto unmap_oemnibw_addr;
                }
        }
	if (ret < 0)
		pr_err("ipc_write_oemnib: ipc_write_osnib failed!!\n");

unmap_oshob_addr:
unmap_oemnibw_addr:
exit:

	intel_scu_ipc_unlock();

	return ret;
}

/*
 * This reads the OEMNIB from the internal RAM of the SCU.
 */
static int intel_scu_ipc_read_oemnib(u8 *oemnib, int len, int offset)
{
	int i, ret = 0;
	u8 *ptr;
	void __iomem *oshob_addr, *oemnibr_addr;

	if (oemnib == NULL) {
		pr_err("ipc_read_oemnib: passed buffer for reading OEMNIB is NULL\n");
		return -EINVAL;
	}

	pr_debug("ipc_read_oemnib: remap OSHOB base addr 0x%x size %d\n",
		oshob_info->oshob_base, oshob_info->oshob_size);

	oshob_addr = (void __iomem *)oshob_info->oshob_base;

	if (!oshob_addr) {
		pr_err("ipc_read_oemnib: ioremap failed!\n");
		ret = -ENOMEM;
		goto exit;
	}

	if ((len == 0) || (len > oshob_info->oemnib_size)) {
		pr_err("ipc_read_oemnib: Bad OEMNIB data length (%d) to be read (max=%d bytes)\n",
			    len, oshob_info->oemnib_size);
		ret = -EINVAL;
		goto unmap_oshob_addr;
	}

	/* offset shall start at 0 from the OEMNIB base address and shall */
	/* not exceed the OEMNIB allowed size.                            */
	if ((offset < 0) || (offset >= oshob_info->oemnib_size) ||
	    (len + offset > oshob_info->oemnib_size)) {
		pr_err(
		"ipc_read_oemnib: Bad OEMNIB data offset/len to read (offset=%d ,len=%d)\n",
		offset, len);
		ret = -EINVAL;
		goto unmap_oshob_addr;
	}

	pr_debug("ipc_read_oemnib: POEMNIB remap oemnibr ptr 0x%x size %d\n",
		oshob_info->oemnibr_ptr, oshob_info->oemnib_size);

	oemnibr_addr = (void __iomem *)oshob_info->oemnibr_ptr;

	if (!oemnibr_addr) {
		pr_err("ipc_read_oemnib: ioremap of oemnib failed!\n");
		ret = -ENOMEM;
		goto unmap_oshob_addr;
	}

	ptr = oemnib;
	pr_debug("ipc_read_oemnib: OEMNIB content:\n");
	for (i = 0; i < len; i++) {
		*ptr = readb(oemnibr_addr + offset + i);
		pr_debug("addr(remapped)=%8x, offset=%2x, value=%2x\n",
			(u32)(oemnibr_addr+offset+i), offset+i, *ptr);
		ptr++;
	}

unmap_oshob_addr:
exit:
	return ret;
}

int intel_scu_ipc_cmd_oemnib(int cmd, const char *arg)
{
	int ret = 0;
	int length = 0;
	u8  *oemnib_buf;
	pr_info("intel_scu_ipc_cmd_oemnib : cmd = %x\n", cmd);

	oemnib_buf = malloc(oshob_info->oemnib_size+1);

	if(oemnib_buf == NULL)
	{
		pr_err(
		"Cannot allocate buffer\n");
		return -ENOMEM;
	}

	memset(oemnib_buf, 0x00, oshob_info->oemnib_size+1);

	if (arg != NULL)
	{
		length = strlen(arg);
		if (length > oshob_info->oemnib_size) {
			printf("write size exceeds oemnib size. ignore exceeded data.\n");
			length = oshob_info->oemnib_size;
		}
		memcpy(oemnib_buf, arg, length);
	}

	if (cmd)
		ret = intel_scu_ipc_read_oemnib(
						oemnib_buf,
						oshob_info->oemnib_size,
						0);
	else
		ret = intel_scu_ipc_write_oemnib(
						oemnib_buf,
						length,
						0);

	printf("intel_scu_ipc_cmd_oemnib : done => %s\n", oemnib_buf);

	free(oemnib_buf);

	return ret;
}

/*
 * This reads the PMIT from the OSHOB (pointer to interrupt tree)
 */
#ifdef DUMP_OSNIB
static int intel_scu_ipc_read_oshob_it_tree(u32 *ptr)
{
	u16 struct_offs;

	pr_debug("intel_scu_ipc_read_oshob_it_tree: read IT tree\n");

	if ((oshob_info->oshob_majrev == OSHOB_REV_MAJ_DEFAULT) &&
	    (oshob_info->oshob_minrev == OSHOB_REV_MIN_DEFAULT)) {
		struct_offs = offsetof(struct scu_ipc_oshob, pmit) +
				    oshob_info->offs_add;
	} else if ((oshob_info->oshob_majrev >= 1) &&
		(oshob_info->oshob_minrev >= 4)) {
		struct_offs = offsetof(struct scu_ipc_oshob_extend_v14, pmit) +
				oshob_info->offs_add;
	} else {
		struct_offs = offsetof(struct scu_ipc_oshob_extend, pmit) +
				    oshob_info->offs_add;
	}
	return intel_scu_ipc_read_oshob(
			(u8 *) ptr,
			4,
			struct_offs);
}
#endif

/*
 * This reads the RESETIRQ1 or RESETSRC0 from the OSNIB
 */
#ifdef DUMP_OSNIB
static int intel_scu_ipc_read_osnib_reset_ev1(u8 *rev1)
{
        int i;

        for (i = 0; i < ARRAY_SIZE(chip_reset_events); i++) {
                if (chip_reset_events[i].id == oshob_info->platform_type) {
                        pr_debug(
                                "intel_scu_ipc_read_osnib_rst_ev1: read %s\n",
                                chip_reset_events[i].reset_ev1_name);

                        return oshob_info->scu_ipc_read_osnib(
                                    rev1,
                                    1,
                                    offsetof(struct scu_ipc_osnib, reset_ev1));
                }
        }

        pr_err("intel_scu_ipc_read_osnib_reset_ev1: param not found\n");
        return -EFAULT;
}
#endif

/*
 * This reads the RESETIRQ2 or RESETSRC1 from the OSNIB
 */
#ifdef DUMP_OSNIB
static int intel_scu_ipc_read_osnib_reset_ev2(u8 *rev2)
{
        int i;

        for (i = 0 ; i < ARRAY_SIZE(chip_reset_events); i++) {
                if (chip_reset_events[i].id == oshob_info->platform_type) {
                        pr_debug(
                                "intel_scu_ipc_read_osnib_rst_ev2: read %s\n",
                                chip_reset_events[i].reset_ev2_name);

                        return oshob_info->scu_ipc_read_osnib(
                                rev2,
                                1,
                                offsetof(struct scu_ipc_osnib, reset_ev2));
                }
        }

        pr_err("intel_scu_ipc_read_osnib_reset_ev2: param not found\n");
        return -EFAULT;
}
#endif

/*
 * This reads the WD from the OSNIB
 */
int intel_scu_ipc_read_osnib_wd(u8 *wd)
{
	pr_debug("intel_scu_ipc_read_osnib_wd: read WATCHDOG\n");

	return oshob_info->scu_ipc_read_osnib(
			wd,
			1,
			offsetof(struct scu_ipc_osnib, wd_count));
}

/*
 * This writes the WD in the OSNIB
 */
int intel_scu_ipc_write_osnib_wd(u8 *wd)
{
	pr_info("intel_scu_ipc_write_osnib_wd: write WATCHDOG %x\n", *wd);

	return oshob_info->scu_ipc_write_osnib(
			wd,
			1,
			offsetof(struct scu_ipc_osnib, wd_count));
}

/*
 * Get SCU trace buffer physical address if available
 */
u32 intel_scu_ipc_get_scu_trace_buffer(void)
{
	if (oshob_info == NULL)
		return 0;
	return oshob_info->scu_trace_buf;
}

/*
 * Get SCU trace buffer size
 */
u32 intel_scu_ipc_get_scu_trace_buffer_size(void)
{
	if (oshob_info == NULL)
		return 0;
	return oshob_info->scu_trace_size;
}

/*
 * Get SCU fabric error buffer1 offset
 */
u32 intel_scu_ipc_get_fabricerror_buf1_offset(void)
{
        if (oshob_info == NULL)
                return 0;

        if (oshob_info->platform_type == INTEL_MID_CPU_CHIP_CLOVERVIEW)
                return offsetof(struct scu_ipc_oshob_extend, fabricerrlog1);
        else if ((oshob_info->platform_type == INTEL_MID_CPU_CHIP_TANGIER) ||
                (oshob_info->platform_type == INTEL_MID_CPU_CHIP_ANNIEDALE))
                if ((oshob_info->oshob_majrev >= 1) &&
                    (oshob_info->oshob_minrev >= 4)) {
                        return offsetof(struct scu_ipc_oshob_extend_v14,
                                        fabricerrlog) + oshob_info->offs_add;
                } else {
                        return offsetof(struct scu_ipc_oshob,
                                        fab_err_log) + oshob_info->offs_add;
                }
        else {
                pr_err("scu_ipc_get_fabricerror_buf_offset: platform not recognized!\n");
                return 0;
        }
}

/*
 * Get SCU fabric error buffer2 offset
 */
u32 intel_scu_ipc_get_fabricerror_buf2_offset(void)
{
	return offsetof(struct scu_ipc_oshob_extend,
			fabricerrlog2);
}


/*
 * This reads the ALARM from the OSNIB
 */
#ifdef DUMP_OSNIB
static int intel_scu_ipc_read_osnib_alarm(u8 *alarm)
{
	pr_debug("intel_scu_ipc_read_osnib_alarm: read ALARM\n");

	return oshob_info->scu_ipc_read_osnib(
			alarm,
			1,
			offsetof(struct scu_ipc_osnib, alarm));
}
#endif

/*
 * This reads the WAKESRC from the OSNIB
 */
#ifdef DUMP_OSNIB
static int intel_scu_ipc_read_osnib_wakesrc(u8 *wksrc)
{
	pr_debug("intel_scu_ipc_read_osnib_wakesrc: read WAKESRC\n");

	return oshob_info->scu_ipc_read_osnib(
			wksrc,
			1,
			offsetof(struct scu_ipc_osnib, wakesrc));
}
#endif

int intel_scu_ipc_oshob_init(void)
{
        int ret, i;
        u16 struct_offs;

#ifdef DUMP_OSNIB
        u8 rr, reset_ev1, reset_ev2, wd, alarm, wakesrc, *ptr;
        u32 pmit, scu_trace[OSHOB_SCU_BUF_BASE_DW_SIZE*4], ia_trace;
        int buff_size;
#endif

        /* Identify the type and size of OSHOB to be used. */
        ret = intel_scu_ipc_read_oshob_info();

        if (ret != 0) {
                pr_err("Cannot init ipc module: oshob info not read\n");
                goto exit;
        }

#ifdef DUMP_OSNIB
        /* Dumping reset events from the interrupt tree */
        ret = intel_scu_ipc_read_oshob_it_tree(&pmit);

        if (ret != 0) {
                pr_err("Cannot read interrupt tree\n");
                goto exit;
        }

        ptr = (u8 *) (pmit + PMIT_RESET1_OFFSET);

        if (!ptr) {
                pr_err("Cannot remap PMIT\n");
                ret = -ENOMEM;
                goto exit;
        }

        pr_debug("PMIT addr 0x%8x remapped to 0x%8x\n", pmit, (u32)ptr);
        reset_ev1 = readb(ptr);
        reset_ev2 = readb(ptr+1);
        for (i = 0; i < ARRAY_SIZE(chip_reset_events); i++) {
                if (chip_reset_events[i].id == oshob_info->platform_type) {
                        pr_warn("[BOOT] %s=0x%02x %s=0x%02x (PMIT interrupt tree)\n",
                                chip_reset_events[i].reset_ev1_name,
                                reset_ev1,
                                chip_reset_events[i].reset_ev2_name,
                                reset_ev2);
                }
        }

        /* Dumping OSHOB content */
        if ((oshob_info->oshob_majrev == OSHOB_REV_MAJ_DEFAULT) &&
            (oshob_info->oshob_minrev == OSHOB_REV_MIN_DEFAULT)) {
                /* Use default OSHOB here. Calculate in bytes here. */
                if ((oshob_info->platform_type ==
                                INTEL_MID_CPU_CHIP_TANGIER) ||
                        (oshob_info->platform_type ==
                                INTEL_MID_CPU_CHIP_ANNIEDALE) ||
                        (oshob_info->platform_type ==
                                INTEL_MID_CPU_CHIP_CARBONCANYON))
                        buff_size = OSHOB_SCU_BUF_MRFLD_DW_SIZE*4;
                else
                        buff_size = OSHOB_SCU_BUF_BASE_DW_SIZE*4;

                ret = intel_scu_ipc_read_oshob(
                        (u8 *)(scu_trace),
                        buff_size,
                        offsetof(struct scu_ipc_oshob, scutxl));

                if (ret != 0) {
                        pr_err("Cannot read SCU data\n");
                        goto exit;
                }

                struct_offs = offsetof(struct scu_ipc_oshob, iatxl) +
                                oshob_info->offs_add;
                ret = intel_scu_ipc_read_oshob(
                            (u8 *)(&ia_trace),
                            4,
                            struct_offs);

                if (ret != 0) {
                        pr_err("Cannot read IA data\n");
                        goto exit;
                }
            } else {
                /* Use extended OSHOB here. Calculate in bytes here. */
                if ((oshob_info->platform_type ==
                                INTEL_MID_CPU_CHIP_TANGIER) ||
                        (oshob_info->platform_type ==
                                INTEL_MID_CPU_CHIP_ANNIEDALE) ||
                        (oshob_info->platform_type ==
                                INTEL_MID_CPU_CHIP_CARBONCANYON))
                        buff_size = OSHOB_SCU_BUF_MRFLD_DW_SIZE*4;
                else
                        buff_size = OSHOB_SCU_BUF_BASE_DW_SIZE*4;

                if ((oshob_info->oshob_majrev >= 1) &&
                    (oshob_info->oshob_minrev >= 4)) {
                        ret = intel_scu_ipc_read_oshob(
                                (u8 *)(scu_trace),
                                buff_size,
                                offsetof(struct scu_ipc_oshob_extend_v14,
                                                                scutxl));
                } else {
                        ret = intel_scu_ipc_read_oshob(
                                (u8 *)(scu_trace),
                                buff_size,
                                offsetof(struct scu_ipc_oshob_extend, scutxl));
                }

                if (ret != 0) {
                        pr_err("Cannot read SCU data\n");
                        goto exit;
                }

                if ((oshob_info->oshob_majrev >= 1) &&
                    (oshob_info->oshob_minrev >= 4)) {
                        struct_offs = offsetof(struct scu_ipc_oshob_extend_v14,
                                                iatxl) + oshob_info->offs_add;
                } else {
                        struct_offs = offsetof(struct scu_ipc_oshob_extend,
                                                iatxl) + oshob_info->offs_add;
                }

                ret = intel_scu_ipc_read_oshob(
                                (u8 *)(&ia_trace),
                                4,
                                struct_offs);
                if (ret != 0) {
                        pr_err("Cannot read IA data\n");
                        goto exit;
                }
        }

        if ((oshob_info->platform_type ==
                        INTEL_MID_CPU_CHIP_TANGIER) ||
                (oshob_info->platform_type ==
                        INTEL_MID_CPU_CHIP_ANNIEDALE) ||
                (oshob_info->platform_type ==
                        INTEL_MID_CPU_CHIP_CARBONCANYON)) {
                for (i = 0; i < OSHOB_SCU_BUF_MRFLD_DW_SIZE; i++)
                        pr_warn("[BOOT] SCU_TR[%d]=0x%08x\n", i, scu_trace[i]);
        } else
                pr_warn("[BOOT] SCU_TR=0x%08x (oshob)\n", scu_trace[0]);

        pr_warn("[BOOT] IA_TR=0x%08x (oshob)\n", ia_trace);

        /* Dumping OSNIB content */
        ret = 0;
        ret |= intel_scu_ipc_read_osnib_rr(&rr);
        ret |= intel_scu_ipc_read_osnib_reset_ev1(&reset_ev1);
        ret |= intel_scu_ipc_read_osnib_reset_ev2(&reset_ev2);
        ret |= intel_scu_ipc_read_osnib_wd(&wd);
        ret |= intel_scu_ipc_read_osnib_alarm(&alarm);
        ret |= intel_scu_ipc_read_osnib_wakesrc(&wakesrc);

        if (ret) {
                pr_err("Cannot read OSNIB content\n");
                goto exit;
        }

        pr_warn("[BOOT] RR=0x%02x WD=0x%02x ALARM=0x%02x (osnib)\n",
                rr, wd, alarm);

        for (i = 0; i < ARRAY_SIZE(chip_reset_events); i++) {
                if (chip_reset_events[i].id == oshob_info->platform_type) {
                        pr_warn("[BOOT] WAKESRC=0x%02x %s=0x%02x %s=0x%02x (osnib)\n",
                                wakesrc,
                                chip_reset_events[i].reset_ev1_name,
                                reset_ev1,
                                chip_reset_events[i].reset_ev2_name,
                                reset_ev2);
                }
        }

#endif /* DUMP_OSNIB */

exit:
        return ret;
}

struct secure_rtc_tod* intel_scu_ipc_read_srtc(void)
{
    u32 result[4];
    int status;
    struct ipc_mailbox ipc_mb;
    struct secure_rtc_tod *srtc_tod;

    memset(result, 0xAA, sizeof(result));
    memset(&ipc_mb, 0, sizeof(ipc_mb));
    ipc_mb.opcode = SRTC_OPCODE;
    ipc_mb.status = 0;
    ipc_mb.ostype = 0;
    ipc_mb.oper   = 0;

    printf("ipc_mb opcode = 0x%02x, status 0x%02x ostype 0x%02x oper 0x%02x\n",
		ipc_mb.opcode, ipc_mb.status, ipc_mb.ostype, ipc_mb.oper);
    status = intel_scu_ipc_command(OEM_IPC_COMMAND, 0, (u8*)&ipc_mb, 8, result, 4);
    if (status < 0) {
        printf("IPC FAILURE error=%d : opcode = 0x%02x\n", status, SRTC_OPCODE);
        return 0;
    }
    srtc_tod = (struct secure_rtc_tod *)&result[1];
    return srtc_tod;
}

int intel_scu_ipc_increase_mtc(u8 ostype)
{
    u32 result[4];
    int status;
    struct ipc_mailbox ipc_mb;

    memset(result, 0xAA, sizeof(result));
    memset(&ipc_mb, 0, sizeof(ipc_mb));
    ipc_mb.opcode = MTC_OPCODE;
    ipc_mb.status = 0;
    ipc_mb.ostype = ostype;
    ipc_mb.oper   = 1;

    printf("ipc_mb opcode = 0x%02x, status 0x%02x ostype 0x%02x oper 0x%02x\n",
		ipc_mb.opcode, ipc_mb.status, ipc_mb.ostype, ipc_mb.oper);
    status = intel_scu_ipc_command(OEM_IPC_COMMAND, 0, (u8*)&ipc_mb, 8, result, 4);
    if (status < 0) {
        printf("IPC FAILURE error=%d : opcode = 0x%02x\n", status, MTC_OPCODE);
        return status;
    }
    return result[1];
}

int intel_scu_ipc_read_mtc(u8 ostype)
{
    u32 result[4];
    int status;
    struct ipc_mailbox ipc_mb;

    memset(result, 0xAA, sizeof(result));
    memset(&ipc_mb, 0, sizeof(ipc_mb));
    ipc_mb.opcode = MTC_OPCODE;
    ipc_mb.status = 0;
    ipc_mb.ostype = ostype;
    ipc_mb.oper   = 0;

    printf("ipc_mb opcode = 0x%02x, status 0x%02x ostype 0x%02x oper 0x%02x\n",
		ipc_mb.opcode, ipc_mb.status, ipc_mb.ostype, ipc_mb.oper);
    status = intel_scu_ipc_command(OEM_IPC_COMMAND, 0, (u8*)&ipc_mb, 8, result, 4);
    if (status < 0) {
        printf("IPC FAILURE error=%d : opcode = 0x%02x\n", status, MTC_OPCODE);
        return status;
    }
    return result[1];
}

u8 intel_scu_ipc_set_tamper(void)
{
    u32 result[4];
    int status;
    struct ipc_mailbox ipc_mb;

    memset(result, 0xAA, sizeof(result));
    memset(&ipc_mb, 0, sizeof(ipc_mb));
    ipc_mb.opcode = TAMPER_OPCODE;
    ipc_mb.status = 1;
    ipc_mb.ostype = 0;
    ipc_mb.oper   = 0;

    printf("ipc_mb opcode = 0x%02x, status 0x%02x ostype 0x%02x oper 0x%02x\n",
		ipc_mb.opcode, ipc_mb.status, ipc_mb.ostype, ipc_mb.oper);
    status = intel_scu_ipc_command(OEM_IPC_COMMAND, 0, (u8*)&ipc_mb, 8, result, 4);
    if (status < 0) {
        printf("IPC FAILURE error=%d : opcode = 0x%02x\n", status, TAMPER_OPCODE);
        return status;
    }
    return ((result[0] & 0xFF00) ? 1 : 0);
}

u8 intel_scu_ipc_read_tamper(void)
{
    u32 result[4];
    int status;
    struct ipc_mailbox ipc_mb;

    memset(result, 0xAA, sizeof(result));
    memset(&ipc_mb, 0, sizeof(ipc_mb));
    ipc_mb.opcode = TAMPER_OPCODE;
    ipc_mb.status = 0;
    ipc_mb.ostype = 0;
    ipc_mb.oper   = 0;

    printf("ipc_mb opcode = 0x%02x, status 0x%02x ostype 0x%02x oper 0x%02x\n",
		ipc_mb.opcode, ipc_mb.status, ipc_mb.ostype, ipc_mb.oper);
    status = intel_scu_ipc_command(OEM_IPC_COMMAND, 0, (u8*)&ipc_mb, 8, result, 4);
    if (status < 0) {
        printf("IPC FAILURE error=%d : opcode = 0x%02x\n", status, TAMPER_OPCODE);
        return status;
    }
    return ((result[0] & 0xFF00) ? 1 : 0);
}

int intel_scu_ipc_verify_img(u8 imgtype, u8 keyindex, u32 ddraddr)
{
    u32 result[4];
    int status;
    struct ipc_mailbox ipc_mb;

    memset(result, 0xAA, sizeof(result));
    memset(&ipc_mb, 0, sizeof(ipc_mb));
    ipc_mb.opcode  = IMG_VERIFY_OPCODE;
    ipc_mb.status  = 0;         // reserved
    ipc_mb.ostype  = imgtype;   // image type
    ipc_mb.oper    = keyindex;  // key index
    ipc_mb.data[0] = ddraddr;   // ddr address of the images

    printf("ipc_mb opcode = 0x%02x, status 0x%02x ostype 0x%02x oper 0x%02x addr 0x%08x\n",
		ipc_mb.opcode, ipc_mb.status, ipc_mb.ostype, ipc_mb.oper, ipc_mb.data[0]);
    status = intel_scu_ipc_command(OEM_IPC_COMMAND, 0, (u8*)&ipc_mb, 8, result, 4);
    if (status < 0) {
        printf("IPC FAILURE error=%d : opcode = 0x%02x\n", status, IMG_VERIFY_OPCODE);
        return status;
    }
    return result[1];
}
