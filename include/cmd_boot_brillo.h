#ifndef _CMD_BOOT_BRILLO_H_
#define	_CMD_BOOT_BRILLO_H_

int ab_set_active(int slot);
char* get_active_slot(void);
bool is_successful_slot(char *suffix);
bool is_unbootable_slot (char *suffix);
int get_slot_retry_count(char *suffix);

#endif
