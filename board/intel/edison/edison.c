#include <common.h>
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

int board_usb_init(int index, enum usb_init_type init)
{
	return usb_gadget_init_udc();
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
