#ifndef _EDISON_H
#define _EDISON_H

#define CONFIG_WATCHDOG
#define CONFIG_WATCHDOG_HEARTBEAT 30

#define CONFIG_SFI

/*-----------------------------------------------------------------------
 * Misc
 */

#define CONFIG_CMD_ITEST
#define CONFIG_CMD_ASKENV
#define CONFIG_CMD_BDI
#define CONFIG_CMD_BSP
#define CONFIG_CMD_BOOTD
#define CONFIG_CMD_CACHE
#define CONFIG_CMD_CONSOLE
/*
 *#define CONFIG_CMD_DATE
 */
#define CONFIG_CMD_DIAG
#define CONFIG_CMD_ECHO
#define CONFIG_CMD_EDITENV
#define CONFIG_CMD_ELF
#define CONFIG_CMD_ENV_CALLBACK
#define CONFIG_CMD_ENV_FLAGS
#define CONFIG_CMD_ENV_EXISTS
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
#define CONFIG_CMD_MEMORY
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
#define CONFIG_CMD_SOURCE
/*
 *#define CONFIG_CMD_SPI
 */
#define CONFIG_CMD_TIMER

/*
 *common/cmd_date.c|49| undefined reference to `rtc_reset'
 *common/cmd_date.c|52| undefined reference to `rtc_get'
 *common/cmd_date.c|61| undefined reference to `rtc_set'
 *common/cmd_date.c|70| undefined reference to `rtc_get'
 *|| common/built-in.o: In function `do_fuse':
 *common/cmd_fuse.c|75| undefined reference to `fuse_read'
 *common/cmd_fuse.c|93| undefined reference to `fuse_sense'
 *common/cmd_fuse.c|112| undefined reference to `fuse_prog'
 *common/cmd_fuse.c|126| undefined reference to `fuse_override'
 *|| common/built-in.o: In function `do_i2c_read':
 *common/cmd_i2c.c|245| undefined reference to `i2c_read'
 *|| common/built-in.o: In function `do_i2c_write':
 *common/cmd_i2c.c|286| undefined reference to `i2c_write'
 *|| common/built-in.o: In function `do_i2c_md':
 *common/cmd_i2c.c|370| undefined reference to `i2c_read'
 *|| common/built-in.o: In function `do_i2c_mw':
 *common/cmd_i2c.c|452| undefined reference to `i2c_write'
 *|| common/built-in.o: In function `do_i2c_crc':
 *common/cmd_i2c.c|523| undefined reference to `i2c_read'
 *|| common/built-in.o: In function `do_i2c_probe':
 *common/cmd_i2c.c|711| undefined reference to `i2c_probe'
 *|| common/built-in.o: In function `do_i2c_loop':
 *common/cmd_i2c.c|788| undefined reference to `i2c_read'
 *|| common/built-in.o: In function `do_i2c_reset':
 *common/cmd_i2c.c|1527| undefined reference to `i2c_init'
 *|| common/built-in.o: In function `mod_i2c_mem':
 *common/cmd_i2c.c|603| undefined reference to `i2c_read'
 *common/cmd_i2c.c|649| undefined reference to `i2c_write'
 *|| common/built-in.o: In function `do_md5sum':
 *common/cmd_md5sum.c|146| undefined reference to `md5_wd'
 *|| common/built-in.o: In function `do_spi':
 *common/cmd_spi.c|106| undefined reference to `spi_setup_slave'
 *common/cmd_spi.c|112| undefined reference to `spi_claim_bus'
 *common/cmd_spi.c|113| undefined reference to `spi_xfer'
 *common/cmd_spi.c|123| undefined reference to `spi_release_bus'
 *common/cmd_spi.c|124| undefined reference to `spi_free_slave'
 *|| common/built-in.o: In function `jumptable_init':
 *include/_exports.h|25| undefined reference to `i2c_write'
 *include/_exports.h|26| undefined reference to `i2c_read'
 *include/_exports.h|27| undefined reference to `spi_init'
 *include/_exports.h|28| undefined reference to `spi_setup_slave'
 *include/_exports.h|29| undefined reference to `spi_free_slave'
 *include/_exports.h|30| undefined reference to `spi_claim_bus'
 *include/_exports.h|31| undefined reference to `spi_release_bus'
 *include/_exports.h|32| undefined reference to `spi_xfer'
 *|| common/built-in.o: In function `handle_exception':
 *common/kgdb.c|332| undefined reference to `kgdb_longjmp'
 *common/kgdb.c|344| undefined reference to `kgdb_trap'
 *common/kgdb.c|346| undefined reference to `kgdb_setjmp'
 *common/kgdb.c|349| undefined reference to `kgdb_enter'
 *common/kgdb.c|401| undefined reference to `kgdb_setjmp'
 *common/kgdb.c|420| undefined reference to `kgdb_getregs'
 *common/kgdb.c|428| undefined reference to `kgdb_putregs'
 *common/kgdb.c|498| undefined reference to `kgdb_exit'
 *common/kgdb.c|512| undefined reference to `kgdb_putreg'
 *|| common/built-in.o: In function `kgdb_error':
 *common/kgdb.c|554| undefined reference to `kgdb_longjmp'
 *|| common/built-in.o: In function `do_kgdb':
 *common/kgdb.c|590| undefined reference to `kgdb_breakpoint'
 *
 */

/*
 *#define CONFIG_CMD_TRACE
 */
/*
 *#define CONFIG_CMD_DATE
 */
#define CONFIG_CMD_ECHO
/*
 *#define CONFIG_CMD_GPIO
 */
#define CONFIG_CMD_LOADB
#define CONFIG_CMD_LOADS
#define CONFIG_CMD_IRQ
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_MISC
#define CONFIG_CMD_PCI
#define CONFIG_CMD_SOURCE
/*
 *#define CONFIG_CMD_TIME
 *#define CONFIG_CMD_GETTIME
 */
/*
 *#define CONFIG_CMD_USB
 */
#define CONFIG_CMD_FAT
/*
 *#define EARLY_TRACE
 *#define FTRACE
 */

/*
 *void rtc_reset(void);
 *int rtc_set(struct rtc_time *tmp);
 *int rtc_get(struct rtc_time *tmp);
 *
 *inline int gpio_request(unsigned gpio, const char *label)
 *inline int gpio_get_value(unsigned gpio)
 *inline int gpio_set_value(unsigned gpio, int val)
 *int gpio_direction_input(unsigned gpio)
 *int gpio_direction_output(unsigned gpio, int value)
 *int gpio_free(unsigned gpio)
 */

/*
 *common/cmd_date.c|49| undefined reference to `rtc_reset'
 *common/cmd_date.c|52| undefined reference to `rtc_get'
 *common/cmd_date.c|61| undefined reference to `rtc_set'
 *common/cmd_date.c|70| undefined reference to `rtc_get'
 *|| common/built-in.o: In function `do_gpio':
 *common/cmd_gpio.c|60| undefined reference to `gpio_request'
 *common/cmd_gpio.c|67| undefined reference to `gpio_direction_input'
 *common/cmd_gpio.c|68| undefined reference to `gpio_get_value'
 *common/cmd_gpio.c|73| undefined reference to `gpio_get_value'
 *common/cmd_gpio.c|76| undefined reference to `gpio_direction_output'
 *common/cmd_gpio.c|81| undefined reference to `gpio_free'
 *|| common/built-in.o: In function `do_trace':
 *common/cmd_trace.c|102| undefined reference to `trace_print_stats'
 *|| common/built-in.o: In function `create_call_list':
 *common/cmd_trace.c|66| undefined reference to `trace_list_calls'
 *|| common/built-in.o: In function `do_trace':
 *common/cmd_trace.c|95| undefined reference to `trace_set_enabled'
 *|| common/built-in.o: In function `create_func_list':
 *common/cmd_trace.c|42| undefined reference to `trace_list_functions'
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
#define CONFIG_SYS_CBSIZE			2048
#define CONFIG_SYS_PBSIZE			(CONFIG_SYS_CBSIZE + \
						 sizeof(CONFIG_SYS_PROMPT) + \
						 16)
#define CONFIG_SYS_MAXARGS			128
#define CONFIG_SYS_BARGSIZE			CONFIG_SYS_CBSIZE
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_HUSH_PARSER

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
#define CONFIG_CMD_SAVEENV
#define CONFIG_CMD_RUN
#define CONFIG_SUPPORT_EMMC_BOOT

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
#define CONFIG_SCU_BASE_ADDR                0xff000000
#define CONFIG_SCU_IPC_BASE         0xff009000
#define CONFIG_SCU_I2C_BASE         0xff00d000
#define CONFIG_CPU_CHIP 4
#define CONFIG_X86_MRFLD


#endif
