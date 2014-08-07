/*
 * cmd_dfu.c -- dfu command
 *
 * Copyright (C) 2015
 * Lukasz Majewski <l.majewski@majess.pl>
 *
 * Copyright (C) 2012 Samsung Electronics
 * authors: Andrzej Pietrasiewicz <andrzej.p@samsung.com>
 *	    Lukasz Majewski <l.majewski@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <watchdog.h>
#include <dfu.h>
#include <g_dnl.h>
#include <usb.h>
#include <net.h>

static int do_dfu(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	bool dfu_reset = false;

	if (argc < 4)
		return CMD_RET_USAGE;

	char *usb_controller = argv[1];
	char *interface = argv[2];
	char *devstring = argv[3];
#ifdef CONFIG_DFU_TIMEOUT
	ulong dfu_timeout = 0 * 1000;
#endif
	int ret, i = 0;
#ifdef CONFIG_DFU_TFTP
	unsigned long addr = 0;
	if (!strcmp(argv[1], "tftp")) {
		if (argc == 5)
			addr = simple_strtoul(argv[4], NULL, 0);

		return update_tftp(addr, interface, devstring);
	}
#endif

	ret = dfu_init_env_entities(interface, devstring);
	if (ret)
		goto done;

	ret = CMD_RET_SUCCESS;
	if (argc > 4 && strcmp(argv[4], "list") == 0) {
		dfu_show_entities();
		goto done;
	}
#ifdef CONFIG_DFU_TIMEOUT
	if (argc > 4)
		dfu_timeout = simple_strtoul(argv[4], NULL, 0) * 1000 ;
#endif

	int controller_index = simple_strtoul(usb_controller, NULL, 0);
	board_usb_init(controller_index, USB_INIT_DEVICE);
	g_dnl_clear_detach();
	g_dnl_register("usb_dnl_dfu");
#ifdef CONFIG_DFU_TIMEOUT
	ulong time_activity_start = get_timer(0);
	ulong time_inactivity_start = time_activity_start + dfu_timeout;
	ulong next_print_time = 0;
#endif
	while (1) {
		if (g_dnl_detach()) {
			/*
			 * Check if USB bus reset is performed after detach,
			 * which indicates that -R switch has been passed to
			 * dfu-util. In this case reboot the device
			 */
			if (dfu_usb_get_reset()) {
				dfu_reset = true;
				goto exit;
			}

			/*
			 * This extra number of usb_gadget_handle_interrupts()
			 * calls is necessary to assure correct transmission
			 * completion with dfu-util
			 */
			if (++i == 10000)
				goto exit;
		}

#ifdef CONFIG_DFU_TIMEOUT
		if (dfu_enum_done() && (time_inactivity_start != time_activity_start))
		{
			time_inactivity_start = time_activity_start;
			debug("\nDFU connection established\n");
		}
#endif

		if (ctrlc())
			goto exit;

#ifdef CONFIG_DFU_TIMEOUT
		if (time_activity_start != time_inactivity_start)
		{
			ulong cur_time = get_timer(time_activity_start);

			if ( cur_time > dfu_timeout )
			{
				debug("\nInactivity Timeout, Abort Dfu\n");
				goto exit;
			}
			if ( next_print_time == 0 || (get_timer(next_print_time)> 800))
			{
				debug("\rAborting in %lu sec", (dfu_timeout - cur_time)/1000);
				next_print_time = get_timer(0);
			}
		}
#endif
		WATCHDOG_RESET();
		usb_gadget_handle_interrupts(controller_index);
	}
exit:
	g_dnl_unregister();
	board_usb_cleanup(controller_index, USB_INIT_DEVICE);
done:
	dfu_free_entities();

	if (dfu_reset)
		run_command("reset", 0);

	g_dnl_clear_detach();

	return ret;
}

U_BOOT_CMD(dfu, CONFIG_SYS_MAXARGS, 1, do_dfu,
	"Device Firmware Upgrade",
#ifdef CONFIG_DFU_TIMEOUT
	"<USB_controller> <interface> <dev> [list|timeout]\n"
#else
	"<USB_controller> <interface> <dev> [list]\n"
#endif
	"  - device firmware upgrade via <USB_controller>\n"
	"    on device <dev>, attached to interface\n"
	"    <interface>\n"
	"    [list] - list available alt settings\n"
#ifdef CONFIG_DFU_TIMEOUT
	"    [timeout] - specify inactivity timeout in sec, doesn't work whit list"
#endif
#ifdef CONFIG_DFU_TFTP
	"dfu tftp <interface> <dev> [<addr>]\n"
	"  - device firmware upgrade via TFTP\n"
	"    on device <dev>, attached to interface\n"
	"    <interface>\n"
	"    [<addr>] - address where FIT image has been stored\n"
#endif
);
