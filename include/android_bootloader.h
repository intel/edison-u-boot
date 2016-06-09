#ifndef _ANDROID_BOOTLOADER_H
#define _ANDROID_BOOTLOADER_H

struct bootloader_message {
    uint8_t command[32];
    uint8_t status[32];
    uint8_t recovery[768];
    uint8_t stage[32];
    uint8_t reserved[1184];
} __attribute__((packed));

struct bootloader_message_ab {
    struct bootloader_message message;
    char slot_suffix[32];
    // Round up the entire struct to 4096-byte.
    char reserved[2016];
} __attribute__((packed));

#endif
