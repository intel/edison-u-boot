#include <common.h>
#include <dwc3-uboot.h>
#include <asm/cache.h>
#include <usb.h>
#include <linux/usb/gadget.h>
#include <intel_scu_ipc.h>
#include <watchdog.h>
#include <asm/u-boot-x86.h>

DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_WATCHDOG_HEARTBEAT
#define WATCHDOG_HEARTBEAT 30
#else
#define WATCHDOG_HEARTBEAT CONFIG_WATCHDOG_HEARTBEAT
#endif

enum {
	SCU_WATCHDOG_START = 0,
	SCU_WATCHDOG_STOP,
	SCU_WATCHDOG_KEEPALIVE,
	SCU_WATCHDOG_SET_ACTION_ON_TIMEOUT
};

#define IPC_CMD(cmd, sub) (sub << 12 | cmd)

static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_HIGH,
	.base = CONFIG_SYS_USB_OTG_BASE,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
};


int usb_gadget_handle_interrupts(int controller_index)
{
	dwc3_uboot_handle_interrupt(controller_index);
	WATCHDOG_RESET();
	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	if (index == 0 && init == USB_INIT_DEVICE)
		return dwc3_uboot_init(&dwc3_device_data);
	else
		return -EINVAL;
}


int board_usb_cleanup(int index, enum usb_init_type init)
{
	if (index == 0 && init == USB_INIT_DEVICE) {
		dwc3_uboot_exit(index);
		return 0;
	} else
		return -EINVAL;
}

void watchdog_reset(void)
{
	ulong now = timer_get_us();

	/* do not flood SCU */
	if (unlikely((now - gd->arch.tsc_prev) > (WATCHDOG_HEARTBEAT * 1000000)))
	{
		gd->arch.tsc_prev = now;
		intel_scu_ipc_send_command(IPC_CMD(IPCMSG_WATCHDOG_TIMER,
					SCU_WATCHDOG_KEEPALIVE));
	}
}

int watchdog_disable(void)
{
	return (intel_scu_ipc_simple_command(IPCMSG_WATCHDOG_TIMER,
				SCU_WATCHDOG_STOP));
}

int watchdog_init(void)
{
	return (intel_scu_ipc_simple_command(IPCMSG_WATCHDOG_TIMER,
				SCU_WATCHDOG_START));
}
