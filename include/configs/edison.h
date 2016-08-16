/*
 * Copyright (c) 2015 Google, Inc
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <configs/x86-common.h>

#undef CONFIG_BOOTARGS
#define CONFIG_BOOTARGS ""
#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND ""

#define CONFIG_INTEL_MID
#define CONFIG_SFI
#define CONFIG_BOARD_LATE_INIT
#define CONFIG_MD5

#undef CONFIG_CMD_SF_TEST

#undef CONFIG_TPM_TIS_BASE_ADDRESS

#undef CONFIG_CMD_IMLS

#undef CONFIG_SYS_NS16550
#undef CONFIG_X86_SERIAL
#undef CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_IS_NOWHERE
#undef CONFIG_VIDEO
#undef CONFIG_CFB_CONSOLE
#undef CONFIG_SCSI_AHCI
#undef CONFIG_CMD_SCSI
#undef CONFIG_INTEL_ICH6_GPIO
#define CONFIG_CMD_MMC
#define CONFIG_INHERIT_GDT
#define CONFIG_USB_DWC3
#define CONFIG_USB_DWC3_GADGET
#define CONFIG_USB_GADGET
#define CONFIG_USB_GADGET_DOWNLOAD
#define CONFIG_USB_GADGET_DUALSPEED
#define CONFIG_USB_GADGET_VBUS_DRAW 2
#define CONFIG_G_DNL_MANUFACTURER "Intel"
#define CONFIG_G_DNL_VENDOR_NUM 0x8087
#define CONFIG_G_DNL_PRODUCT_NUM 0x09ef
#define CONFIG_SYS_CACHELINE_SIZE 64
#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_SDHCI
#define CONFIG_WATCHDOG
#define CONFIG_CMD_FASTBOOT
#define CONFIG_USB_FUNCTION_FASTBOOT
#define CONFIG_FASTBOOT_FLASH
#define CONFIG_FASTBOOT_FLASH_MMC_DEV 0
#define CONFIG_ANDROID_BOOT_IMAGE
#define CONFIG_CMD_BOOT_BRILLO
#define CONFIG_RANDOM_UUID
#define CONFIG_FASTBOOT_GVB
#define CONFIG_SHA256

/*Status LED*/
#define CONFIG_STATUS_LED               1
#define CONFIG_BOARD_SPECIFIC_LED       1
#define STATUS_LED_GREEN                0x0
#define STATUS_LED_BLUE                 0x1
#define STATUS_LED_RED                  0x2
#define STATUS_LED_BIT                  STATUS_LED_GREEN
#define STATUS_LED_STATE                STATUS_LED_OFF
#define STATUS_LED_PERIOD               (CONFIG_SYS_HZ / 2)
#define STATUS_LED_BIT1                 STATUS_LED_BLUE
#define STATUS_LED_STATE1               STATUS_LED_OFF
#define STATUS_LED_PERIOD1              (CONFIG_SYS_HZ / 2)
#define STATUS_LED_BIT2                 STATUS_LED_RED
#define STATUS_LED_STATE2               STATUS_LED_OFF
#define STATUS_LED_PERIOD2              (CONFIG_SYS_HZ / 2)

#define STATUS_LED_BOOT                 STATUS_LED_GREEN

/* LED implementation
   ----------------------------
   normal boot      		  --> glow LED_BIT(green LED)
   fastboot                       --> glow LED_BIT1(blue LED)
   error condition/recovery boot  --> glow LED_BIT2(red LED)
*/

#define GPIO_BASE_TANGIER 		0xFF008004
#define GPDR_REG_OFFSET			0x1
#define GPSR_REG_OFFSET         	0x2
#define GPCR_REG_OFFSET			0x3
#define NGPIO				192
#define GPIO_REG_ADDR(reg,reg_offset)  (GPIO_BASE_TANGIER + (reg_offset * (NGPIO/32) *4) + (reg*4))

#define CONFIG_FASTBOOT_BUF_ADDR CONFIG_SYS_LOAD_ADDR
#define CONFIG_FASTBOOT_BUF_SIZE 0x07000000

#define CONFIG_STD_DEVICES_SETTINGS	"stdin=serial\0" \
					"stdout=serial\0" \
					"stderr=serial\0"

#endif
