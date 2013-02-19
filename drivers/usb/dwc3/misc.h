#ifndef dwc3_misc_h
#define dwc3_misc_h

#include <linux/compiler.h>
#include <malloc.h>
#include <asm/io.h>
#include <errno.h>

#define DWC3_USB_REGS_SIZE (CONFIG_USB_DWC3_UDC_REGS - \
		CONFIG_USB_DWC3_UDC_REGS_END + 1)
#define DWC3_WRAPPER_REGS_SIZE (CONFIG_USB_DWC3_WRAP_REGS - \
		CONFIG_USB_DWC3_WRAP_REGS_END + 1)

#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))
#define lower_32_bits(n) ((u32)(n))

extern struct dwc3 *global_dwc3;

typedef int mutex_t;
typedef int spinlock_t;
/*
enum {
	false   = 0,
	true    = 1
};
typedef unsigned int bool;
*/
#define min_t(type, x, y) ({		\
		type __min1 = (x);		\
		type __min2 = (y);		\
		__min1 < __min2 ? __min1: __min2; })

#define __init
#define __devinit
#define __devinitconst
#define __devexit

static inline void *dma_alloc_coherent(void *dev, size_t size,
		dma_addr_t *dma_handle,	gfp_t gfp)
{
	void *p;

	p = malloc(size);
	*dma_handle = (unsigned long)p;
	return p;
}


static inline void dma_free_coherent(struct device *dev, size_t size,
		void *vaddr, dma_addr_t bus)
{
	free(vaddr);
}

static inline void kfree(void *p)
{
	free(p);
}

static inline void *kzalloc(unsigned int size, unsigned int flags)
{
	void *p;

	p = malloc(size);
	memset(p, 0, size);
	return p;
}

#define GFP_KERNEL	0

#define MAX_ERRNO       4095

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)

static inline void *ERR_PTR(long error)
{
	return (void *) error;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

static inline long IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

#define dev_err(dev, format, ...) printf(format, ## __VA_ARGS__)
#if 0
#define dev_dbg(dev, format, ...) printf(format, ## __VA_ARGS__)
#define dev_vdbg(dev, format, ...) printf(format, ## __VA_ARGS__)
#define pr_debug(format, ...) printf(format, ## __VA_ARGS__)
#else
static inline void dwc3_valength_dummy(void *p, char *fmt, ...) {}
#define dev_dbg(dev, format, ...) dwc3_valength_dummy(dev, format, ## __VA_ARGS__)
#define dev_vdbg(dev, format, ...) dwc3_valength_dummy(dev, format, ## __VA_ARGS__)
#define pr_debug(format, ...) dwc3_valength_dummy(NULL, format, ## __VA_ARGS__)
#endif

static inline void pm_runtime_enable(struct device *dev) {}
static inline void pm_runtime_get_sync(struct device *dev) {}
static inline void pm_runtime_forbid(struct device *dev) {}
static inline void pm_runtime_allow(struct device *dev) {}

#define PTR_ALIGN(p, a)         ((typeof(p))ALIGN((unsigned long)(p), (a)))

#define PAGE_SHIFT      12
#define PAGE_SIZE       (1UL << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))

struct platform_device {
	struct device dev;
};

static inline void __arch_iounmap(void *p) {}

#define cpu_relax()     asm volatile("" ::: "memory")

enum dma_data_direction {
	DMA_BIDIRECTIONAL       = 0,
	DMA_TO_DEVICE           = 1,
	DMA_FROM_DEVICE         = 2,
};

static inline dma_addr_t dma_map_single(struct device *dev, void *ptr,
		size_t size,
		enum dma_data_direction dir)
{
	return (unsigned long)ptr;
}

static inline void dma_unmap_single(struct device *dev, dma_addr_t addr,
		size_t size,
		enum dma_data_direction dir)
{
}

static inline void dma_sync_single_for_device(struct device *dev,
		dma_addr_t addr, size_t size,
		enum dma_data_direction dir)
{
}
static inline void dma_sync_single_for_cpu(struct device *dev, dma_addr_t addr,
		size_t size,
		enum dma_data_direction dir)
{
}

static inline int device_register(struct device *dev)
{
	return 0;
}

static inline void device_unregister(struct device *dev)
{
}

static inline int dev_set_name(struct device *dev, const char *fmt, ...)
{
	return 0;
}

#define spin_lock(x)
#define spin_unlock(x)
#define spin_lock_irqsave(x, y) y = 0;
#define spin_unlock_irqrestore(x, y)
#if 0
static inline void mdelay(unsigned int msec)
{
	int i;

	/* XXX VirtIO currently hangs on udelay(10+) */
	for (i = 0; i < msec; i++)
		udelay(5);
}
#endif
static inline void msleep(unsigned int msec)
{
	mdelay(msec);
}

static inline void usleep_range(unsigned long min, unsigned long max)
{
	udelay(min);
}

#define container_of(ptr, type, member) ({                      \
		const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
		(type *)( (char *)__mptr - offsetof(type,member) );})

#define __WARN_printf(arg...)   do { printf("WarnON() in %s: %s(%d)\n", __FILE__, __func__, __LINE__); printf(arg); } while (0)

#define WARN(condition, format...) ({                                           \
		int __ret_warn_on = !!(condition);                              \
		if (unlikely(__ret_warn_on))                                    \
		__WARN_printf(format);                                  \
		unlikely(__ret_warn_on);                                        \
		})

#define WARN_ONCE(condition, format...) ({                      \
		static bool __warned;                                   \
		int __ret_warn_once = !!(condition);                    \
		\
		if (unlikely(__ret_warn_once))                          \
		if (WARN(!__warned, format))                    \
		__warned = true;                        \
		unlikely(__ret_warn_once);                              \
	})

#define dev_WARN_ONCE(dev, condition, format, arg...) \
	WARN_ONCE(condition, format, ## arg)

#define WARN_ON(condition) ({                                           \
		int __ret_warn_on = !!(condition);                              \
		if (unlikely(__ret_warn_on))                                    \
		printf("WarnON() in %s: %s(%d)\n", __FILE__, __func__, __LINE__);  \
		unlikely(__ret_warn_on);                                        \
		})

#define WARN_ON_ONCE(condition) ({                              \
		static bool __warned;                                   \
		int __ret_warn_once = !!(condition);                    \
		\
		if (unlikely(__ret_warn_once))                          \
		if (WARN_ON(!__warned))                         \
		__warned = true;                        \
		unlikely(__ret_warn_once);                              \
	})

#define BUILD_BUG_ON_NOT_POWER_OF_2(n)                  \
	BUILD_BUG_ON((n) == 0 || (((n) & ((n) - 1)) != 0))
#if 0
/* never do isoc */
static inline int usb_endpoint_xfer_isoc(
		const struct usb_endpoint_descriptor *epd)
{
	return 0;
}
static inline int usb_endpoint_type(const struct usb_endpoint_descriptor *epd)
{
	return epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
}
#endif

enum irqreturn {
	IRQ_NONE,
	IRQ_HANDLED,
	IRQ_WAKE_THREAD,
};

typedef enum irqreturn irqreturn_t;

int usb_gadget_probe_driver(struct usb_gadget_driver *driver,
		int (*bind)(struct usb_gadget *));
int usb_gadget_unregister_driver(struct usb_gadget_driver *driver);

struct dwc3;
int snprintf(char *buf, size_t size, const char *fmt, ...);
int __devinit dwc3_probe(struct platform_device *pdev);
int __devexit dwc3_remove(struct platform_device *pdev);

extern unsigned int dwc3_cable_connected;
extern unsigned int dwc3_high_speed;

#define USB_GADGET_DELAYED_STATUS       0x7fff  /* Impossibly large value */

#endif
