#ifndef _ANDROID_BOOTLOADER_H
#define _ANDROID_BOOTLOADER_H

struct bootloader_message {
    uint8_t command[32];
    uint8_t status[32];
    uint8_t recovery[768];
    uint8_t stage[32];
    uint8_t slot_suffix[32];
    uint8_t reserved[192];
} __attribute__((packed));

#endif
