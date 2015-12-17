#ifndef _EDISON_H
#define _EDISON_H

#define CONFIG_WATCHDOG
#define CONFIG_WATCHDOG_HEARTBEAT 30
#define CONFIG_SFI
#define CONFIG_BOARD_LATE_INIT

/*-----------------------------------------------------------------------
 * Misc
 */

#define CONFIG_CMD_ASKENV
#define CONFIG_CMD_BSP
#define CONFIG_CMD_CACHE
/*
 *#define CONFIG_CMD_DATE
 */
#define CONFIG_CMD_DIAG
#define CONFIG_CMD_ELF
#define CONFIG_CMD_ENV_CALLBACK
#define CONFIG_CMD_ENV_FLAGS
/*
 *#define CONFIG_CMD_GETTIME
 */
#define CONFIG_CMD_GREPENV
#define CONFIG_CMD_HASH
#define CONFIG_CMD_INI
/*
 *#define CONFIG_CMD_KGDB
 */
/*
 *#define CONFIG_CMD_MD5SUM
 */
#define CONFIG_CMD_MEMINFO
#define CONFIG_CMD_PORTIO
#define CONFIG_CMD_READ
#define CONFIG_CMD_REGINFO
/*
 *#define CONFIG_CMD_I2C
 *#define CONFIG_SYS_I2C_SPEED    50000
 */
/*
 *#define CONFIG_CMD_SHA1SUM
 */
/*
 *#define CONFIG_CMD_SPI
 */
#define CONFIG_CMD_TIMER

/*
 *#define CONFIG_CMD_TRACE
 */
/*
 *#define CONFIG_CMD_DATE
 */
/*
 *#define CONFIG_CMD_GPIO
 */
#define CONFIG_CMD_IRQ
#define CONFIG_CMD_PCI
/*
 *#define CONFIG_CMD_TIME
 *#define CONFIG_CMD_GETTIME
 */
/*
 *#define CONFIG_CMD_USB
 */
/*
 *#define EARLY_TRACE
 *#define FTRACE
 */


/*-----------------------------------------------------------------------
 * Boot
 */

#define CONFIG_ZBOOT_32
#define CONFIG_CMD_ZBOOT
#define CONFIG_AUTOBOOT
#define CONFIG_BOOTCOMMAND "run bootcmd"
#define CONFIG_BOOTDELAY 3

/*-----------------------------------------------------------------------
 * DEBUG
 */

/*
 *#define DEBUG
 */

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
#define CONFIG_BAUDRATE                         115200

/*
* MMC
 */
#define CONFIG_MD5
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_SDHCI
#define CONFIG_CMD_MMC
#define CONFIG_MMC_SDMA
/*#define CONFIG_MMC_TRACE*/

/************************************************************
 * DISK Partition support
 ************************************************************/
#define CONFIG_EFI_PARTITION
#define CONFIG_DOS_PARTITION
#define CONFIG_MAC_PARTITION

#define CONFIG_FS_FAT
#define CONFIG_CMD_FAT
#define CONFIG_FAT_MBR_SCAN
#define CONFIG_FAT_WRITE

#define CONFIG_CMD_GPT
#define CONFIG_CMD_PART
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_EXT4_WRITE
#define CONFIG_PARTITION_UUIDS
#define CONFIG_RANDOM_UUID
#define CONFIG_CMD_FS_GENERIC
 /*
 * Miscellaneous configurable options
 */
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_OF_LIBFDT
#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_CBSIZE			2048
#define CONFIG_SYS_PBSIZE			(CONFIG_SYS_CBSIZE + \
						 sizeof(CONFIG_SYS_PROMPT) + \
						 16)
#define CONFIG_SYS_MAXARGS			128
#define CONFIG_SYS_BARGSIZE			CONFIG_SYS_CBSIZE
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_HUSH_PARSER

#define CONFIG_SHA1
#define CONFIG_CMD_SHA1SUM
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
#define CONFIG_INTEL_MID

/*-----------------------------------------------------------------------
 * Environment
 */
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV                  0
#define CONFIG_SYS_MMC_ENV_PART                 0
#define CONFIG_ENV_SIZE                         (64*1024)
#define CONFIG_ENV_OFFSET	(3 * 1024 * 1024)
#define CONFIG_ENV_OFFSET_REDUND (6 * 1024 * 1024)
#define CONFIG_SUPPORT_EMMC_BOOT
#define CONFIG_CMD_SETEXPR

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

#define CONFIG_USB_GADGET_DOWNLOAD
#define CONFIG_G_DNL_MANUFACTURER "Intel"
#define CONFIG_G_DNL_VENDOR_NUM 0x8087
#define CONFIG_G_DNL_PRODUCT_NUM 0x0a99

#define CONFIG_DFU_FUNCTION
#define CONFIG_CMD_DFU
#define CONFIG_DFU_TIMEOUT
#define CONFIG_DFU_MMC
#define CONFIG_DFU_RAM

/*-----------------------------------------------------------------------
 * SCU
 */

#define CONFIG_SCU_BASE_ADDR                0xff000000
#define CONFIG_SCU_IPC_BASE         0xff009000
#define CONFIG_SCU_I2C_BASE         0xff00d000
#define CONFIG_CPU_CHIP 4
#define CONFIG_X86_MRFLD


#endif
