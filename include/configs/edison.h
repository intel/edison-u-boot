#ifndef _EDISON_H
#define _EDISON_H

/*-----------------------------------------------------------------------
 * DEBUG
 */

#define DEBUG

/*
 *#define CONFIG_PRE_CONSOLE_BUFFER
 *#define CONFIG_PRE_CON_BUF_SZ (1024*1024*2)
 *#define CONFIG_PRE_CON_BUF_ADDR 0x29200000
 */

/*-----------------------------------------------------------------------
 * Serial
 */

#define CONFIG_SERIAL
#define CONFIG_SYS_TNG_SERIAL
#define CONFIG_SYS_TNG_SERIAL2
#define CONFIG_BAUDRATE                         115200

/*
* MMC
 */
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_SDHCI
#define CONFIG_TANGIER_SDHCI
#define CONFIG_CMD_MMC
#define CONFIG_MMC_SDMA
/*#define CONFIG_MMC_TRACE*/

/************************************************************
 * DISK Partition support
 ************************************************************/
#define CONFIG_EFI_PARTITION
#define CONFIG_DOS_PARTITION
#define CONFIG_MAC_PARTITION

/*
 * part uuid mmc 0 disk_uuid
 * gpt write mmc 0 uuid_disk=${disk_uuid}\;name=boot,start=33571840,size=128M,uuid=80868086-8086-8086-8086-000000000000\;
 * gpt write mmc 0 uuid_disk=${disk_uuid}\;name=boot2,start=33571840,size=128M,uuid=80868086-8086-8086-8086-000000000000\;name=panic,size=8M,uuid=80868086-8086-8086-8086-000000000001;
 */
#define CONFIG_CMD_GPT
#define CONFIG_CMD_PART
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_EXT4_WRITE
#define CONFIG_PARTITION_UUIDS

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_PROMPT			"boot > "
#define CONFIG_SYS_CBSIZE			256
#define CONFIG_SYS_PBSIZE			(CONFIG_SYS_CBSIZE + \
						 sizeof(CONFIG_SYS_PROMPT) + \
						 16)
#define CONFIG_SYS_MAXARGS			16
#define CONFIG_SYS_BARGSIZE			CONFIG_SYS_CBSIZE

/*-----------------------------------------------------------------------
 * Board Features
 */

#define CONFIG_SYS_NO_FLASH
#define CONFIG_INHERIT_GDT

/*-----------------------------------------------------------------------
 * Memory
 */

#define CONFIG_SYS_LOAD_ADDR                   0x100000
#define CONFIG_PHYSMEM

#define CONFIG_SYS_CACHELINE_SIZE        64

#define CONFIG_NR_DRAM_BANKS                    3

#define CONFIG_SYS_STACK_SIZE			(32 * 1024)

#define CONFIG_SYS_CAR_ADDR			0x19200000
#define CONFIG_SYS_CAR_SIZE			(16 * 1024)

#define CONFIG_SYS_MONITOR_BASE		        CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_MONITOR_LEN			(256 * 1024)

#define CONFIG_SYS_MALLOC_LEN			( 128 * 1024 * 1024)

#define CONFIG_SYS_HEAP_SIZE                   (128*1024*1024)
#define CONFIG_SYS_HEAP_MINI_SIZE              (5*1024*1024)

#define CONFIG_SYS_MEMTEST_START		0x00100000
#define CONFIG_SYS_MEMTEST_END			0x01000000

/*-----------------------------------------------------------------------
 * CPU Features
 */

#define CONFIG_SYS_X86_TSC_TIMER
#define CONFIG_SYS_NUM_IRQS			16
#define CONFIG_PCI
#define CONFIG_SYS_PCAT_INTERRUPTS

#define CONFIG_CMD_GETTIME

/*-----------------------------------------------------------------------
 * Environment
 */
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV                  0
#define CONFIG_SYS_MMC_ENV_PART                 0
#define CONFIG_ENV_SIZE                         (64*1024)
#define CONFIG_ENV_OFFSET	5242880
#define CONFIG_ENV_OFFSET_REDUND 7340032
#define CONFIG_CMD_SAVEENV
#define CONFIG_CMD_RUN

/*-----------------------------------------------------------------------
 * USB
 */
#define iounmap(x)
#define CONFIG_USB_DWC3
#define CONFIG_USB_DWC3_UDC_REGS (void *) 0xf9100000
#define CONFIG_USB_DWC3_UDC_REGS_END (void *) 0xf9100400


#define CONFIG_USB_DWC3_GADGET
#define CONFIG_USB_DEVICE
#define CONFIG_USB_GADGET
#define CONFIG_USB_GADGET_VBUS_DRAW   2
#define CONFIG_USB_GADGET_DUALSPEED

#define CONFIG_USBDOWNLOAD_GADGET
#define CONFIG_G_DNL_MANUFACTURER "Intel"
#define CONFIG_G_DNL_VENDOR_NUM 0x8087
#define CONFIG_G_DNL_PRODUCT_NUM 0xb6b6

#define CONFIG_DFU_FUNCTION
#define CONFIG_CMD_DFU
#define CONFIG_DFU_MMC
#define CONFIG_DFU_RAM

/*-----------------------------------------------------------------------
 * SCU
 */

#define CONFIG_INTEL_SCU
#define CONFIG_INTEL_SCU_WATCHDOG
#define CONFIG_SCU_BASE_ADDR                0xff000000
#define CONFIG_SCU_IPC_BASE         0xff009000
#define CONFIG_SCU_I2C_BASE         0xff00d000
#define CONFIG_CPU_CHIP 4
#define CONFIG_X86_MRFLD


#endif
