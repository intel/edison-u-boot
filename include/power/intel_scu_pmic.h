#ifndef __INTEL_SCU_PMIC_H__
#define __INTEL_SCU_PMIC_H__

int intel_scu_ipc_ioread8(u16 addr, u8 *data);
int intel_scu_ipc_ioread32(u16 addr, u32 *data);
int intel_scu_ipc_readv(u16 *addr, u8 *data, int len);
int intel_scu_ipc_iowrite8(u16 addr, u8 data);
int intel_scu_ipc_writev(u16 *addr, u8 *data, int len);
int intel_scu_ipc_update_register(u16 addr, u8 data, u8 mask);

extern void intel_pmic_init(void);
extern void intel_pmic_exit(void);

#endif /*__INTEL_SCU_PMIC_H__ */
