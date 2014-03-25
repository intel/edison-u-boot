/*
 *      Intel_SCU 0.2:  An Intel SCU IOH Based Watchdog Device
 *			for Intel part #(s):
 *				- AF82MP20 PCH
 *
 *      Copyright (C) 2009-2010 Intel Corporation. All rights reserved.
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of version 2 of the GNU General
 *      Public License as published by the Free Software Foundation.
 *
 *      This program is distributed in the hope that it will be
 *      useful, but WITHOUT ANY WARRANTY; without even the implied
 *      warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *      PURPOSE.  See the GNU General Public License for more details.
 *      You should have received a copy of the GNU General Public
 *      License along with this program; if not, write to the Free
 *      Software Foundation, Inc., 59 Temple Place - Suite 330,
 *      Boston, MA  02111-1307, USA.
 *      The full GNU General Public License is included in this
 *      distribution in the file called COPYING.
 *
 */

#include <common.h>
#include <asm/io.h>
#include <asm-generic/errno.h>
#include <intel_scu_ipc.h>

#ifdef CONFIG_X86_MRFLD
#define				IPC_WATCHDOG 0xF8
enum {
	SCU_WATCHDOG_START = 0,
	SCU_WATCHDOG_STOP,
	SCU_WATCHDOG_KEEPALIVE,
	SCU_WATCHDOG_SET_ACTION_ON_TIMEOUT
};

int watchdog_timer_disable(void)
{
	int ret = 0;
	int error = 0;

	ret = intel_scu_ipc_simple_command(IPC_WATCHDOG, SCU_WATCHDOG_STOP);
	if (ret) {
		printf("Error stopping watchdog: %x\n", ret);
		error = -EIO;
	}
	return error;
}
#else
#define IPC_SET_WATCHDOG_TIMER	0xF8
#define IPC_SET_SUB_DISABLE     0x01
int watchdog_timer_disable(void)
{
	int ret;

	ret = intel_scu_ipc_command(IPC_SET_WATCHDOG_TIMER,
					IPC_SET_SUB_DISABLE, NULL, 0, NULL, 0);
	if (ret) {
		printf("Error sending disable ipc: %x\n", ret);
	}

	return ret;
}
#endif
