obj-$(CONFIG_USB_DWC3) += dwc3_core.o dwc3_ep0.o dwc3_misc.o
obj-$(CONFIG_USB_DWC3_GADGET) += dwc3_gadget.o
obj-$(CONFIG_USB_DWC3_HOST) += dwc3_host.o

ccflags-y := $(call cc-option,-Wno-unused-variable) \
		$(call cc-option,-Wno-unused-but-set-variable) \
		$(call cc-option,-Wno-unused-label)

