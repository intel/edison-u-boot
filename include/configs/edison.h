#ifndef _EDISON_H
#define _EDISON_H

/* to be used instead of TNG_SERIAL2
#define CONFIG_CONS_INDEX		1
*/
/*
 *#define CONFIG_PRE_CONSOLE_BUFFER
 *#define CONFIG_PRE_CON_BUF_SZ (1024*1024*2)
 *#define CONFIG_PRE_CON_BUF_ADDR 0x29200000
 */
#define DEBUG
#define CONFIG_SERIAL
#define CONFIG_SYS_TNG_SERIAL
#define CONFIG_SYS_TNG_SERIAL2
#define CONFIG_BAUDRATE                         115200
#define CONFIG_PHYSMEM
#define CONFIG_SYS_NO_FLASH
#undef CONFIG_BOARD_EARLY_INIT_F
#undef CONFIG_MODEM_SUPPOT

#define CONFIG_SYS_HEAP_SIZE                   (128*1024*1024)
#define CONFIG_SYS_HEAP_MINI_SIZE              (5*1024*1024)
#define CONFIG_SYS_LOAD_ADDR                   0x100000
#define CONFIG_PCI
#define CONFIG_SYS_PCAT_INTERRUPTS

/*
* MMC
 */
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_SDHCI
#define CONFIG_S5P_SDHCI
#define CONFIG_CMD_MMC
#define CONFIG_MMC_SDMA
/*#define CONFIG_MMC_TRACE*/



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

#define CONFIG_SYS_MEMTEST_START		0x00100000
#define CONFIG_SYS_MEMTEST_END			0x01000000

/*-----------------------------------------------------------------------
 * Board Features
 */


/*-----------------------------------------------------------------------
 * CPU Features
 */

#define CONFIG_SYS_X86_TSC_TIMER
#define CONFIG_SYS_NUM_IRQS			16

/*
 *#define CONFIG_SYS_LDSCRIPT                     arch/x86/cpu/tangier/u-boot.lds
 */

#define CONFIG_SYS_STACK_SIZE			(32 * 1024)
#define CONFIG_SYS_CAR_ADDR			0x19200000
#define CONFIG_SYS_CAR_SIZE			(16 * 1024)
#define CONFIG_SYS_MONITOR_BASE		        CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_MONITOR_LEN			(256 * 1024)
#define CONFIG_SYS_MALLOC_LEN			(0x20000 + 128 * 1024)

/*-----------------------------------------------------------------------
 * Environment
 */


#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_SIZE                         2048
#define CONFIG_SYS_MMC_ENV_DEV                  1
#define CONFIG_SYS_MMC_ENV_PART                 0
#define CONFIG_NR_DRAM_BANKS                    3

#endif
