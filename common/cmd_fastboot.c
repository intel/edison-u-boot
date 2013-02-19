#include <common.h>
#include <command.h>
#include <usb/fastboot.h>

static int do_fastboot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = 1;

	if (!fastboot_init()) {
		printf("Fastboot entered...\n");

		ret = 0;

		while (1) {
			if (fastboot_poll())
				break;
		}
	}

	fastboot_shutdown();
	return ret;
}

U_BOOT_CMD(
	fastboot,	1,	1,	do_fastboot,
	"fastboot- use USB Fastboot protocol\n",
	""
);
