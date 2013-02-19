#include <common.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include "misc.h"
#if 0
int snprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);
	if (i > size)
		printf("*** Wrote too much bytes into the buffer ***\n");
	return i;
}
#endif
