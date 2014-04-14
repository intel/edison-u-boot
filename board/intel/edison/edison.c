#include <asm/cache.h>
#include <usb.h>
#include <linux/usb/gadget.h>

int board_usb_init(int index, enum usb_init_type init)
{
    return usb_gadget_init_udc();
}
