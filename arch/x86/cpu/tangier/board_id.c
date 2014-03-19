#include <common.h>
#include <asm/arch/board_id.h>
#include <asm/intel_scu_pmic.h>

int _get_board_id(void)
{
	u8 i = 0;
	u8 data = -1;
	int value = 0;

	for (i = 0x79; i <= 0x7c; i++) {
		intel_scu_ipc_ioread8(i, &data);
		value |= (data & 0x1) << (0x7c - i);
		data = -1;
	}

	printf("[SCU_IPC_DEBUG] board ID: %x\n", value);

	return value;
}

int get_board_id(void)
{
	static int clt_board_id = CLT_BOARD_NOT_IDENTIFIED;

	if (clt_board_id != CLT_BOARD_NOT_IDENTIFIED)
		return clt_board_id;

	clt_board_id = _get_board_id();

	return clt_board_id;
}

