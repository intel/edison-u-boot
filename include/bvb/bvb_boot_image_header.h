/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if !defined (BVB_INSIDE_BVB_REFIMPL_H) && !defined (BVB_REFIMPL_COMPILATION)
#error "Never include this file directly, include bvb_refimpl.h instead."
#endif

#ifndef BVB_BOOT_IMAGE_HEADER_H_
#define BVB_BOOT_IMAGE_HEADER_H_

#include "bvb_sysdeps.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Size of the Brillo boot image header. */
#define BVB_BOOT_IMAGE_HEADER_SIZE 8192

/* Magic for the Brillo boot image header. */
#define BVB_MAGIC "BVB0"
#define BVB_MAGIC_LEN 4

/* The current MAJOR and MINOR versions used - keep in sync with bvbtool. */
#define BVB_MAJOR_VERSION 1
#define BVB_MINOR_VERSION 0

/* Maximum number of bytes in the kernel command-line before substitution. */
#define BVB_KERNEL_CMDLINE_MAX_LEN 4096

/* Algorithms that can be used in the Brillo boot image for
 * verification. An algorithm consists of a hash type and a signature
 * type.
 *
 * The data used to calculate the hash is the four blocks mentioned in
 * the documentation for |BvbBootImageHeader| except for the data in
 * the "Authentication data" block.
 *
 * For signatures with RSA keys, PKCS v1.5 padding is used. The public
 * key data is stored in the auxilary data block, see
 * |BvbRSAPublicKeyHeader| for the serialization format.
 *
 * Each algorithm type is described below:
 *
 * BVB_ALGORITHM_TYPE_NONE: There is no hash, no signature of the
 * data, and no public key. The data cannot be verified. The fields
 * |hash_size|, |signature_size|, and |public_key_size| must be zero.
 *
 * BVB_ALGORITHM_TYPE_SHA256_RSA2048: The hash function used is
 * SHA-256, resulting in 32 bytes of hash digest data. This hash is
 * signed with a 2048-bit RSA key. The field |hash_size| must be 32,
 * |signature_size| must be 256, and the public key data must have
 * |key_num_bits| set to 2048.
 *
 * BVB_ALGORITHM_TYPE_SHA256_RSA4096: Like above, but only with
 * a 4096-bit RSA key and |signature_size| set to 512.
 *
 * BVB_ALGORITHM_TYPE_SHA256_RSA8192: Like above, but only with
 * a 8192-bit RSA key and |signature_size| set to 1024.
 *
 * BVB_ALGORITHM_TYPE_SHA512_RSA2048: The hash function used is
 * SHA-512, resulting in 64 bytes of hash digest data. This hash is
 * signed with a 2048-bit RSA key. The field |hash_size| must be 64,
 * |signature_size| must be 256, and the public key data must have
 * |key_num_bits| set to 2048.
 *
 * BVB_ALGORITHM_TYPE_SHA512_RSA4096: Like above, but only with
 * a 4096-bit RSA key and |signature_size| set to 512.
 *
 * BVB_ALGORITHM_TYPE_SHA512_RSA8192: Like above, but only with
 * a 8192-bit RSA key and |signature_size| set to 1024.
 */
typedef enum {
  BVB_ALGORITHM_TYPE_NONE,
  BVB_ALGORITHM_TYPE_SHA256_RSA2048,
  BVB_ALGORITHM_TYPE_SHA256_RSA4096,
  BVB_ALGORITHM_TYPE_SHA256_RSA8192,
  BVB_ALGORITHM_TYPE_SHA512_RSA2048,
  BVB_ALGORITHM_TYPE_SHA512_RSA4096,
  BVB_ALGORITHM_TYPE_SHA512_RSA8192,
  _BVB_ALGORITHM_NUM_TYPES
} BvbAlgorithmType;

/* The header for a serialized RSA public key.
 *
 * The size of the key is given by |key_num_bits|, for example 2048
 * for a RSA-2048 key. By definition, a RSA public key is the pair (n,
 * e) where |n| is the modulus (which can be represented in
 * |key_num_bits| bits) and |e| is the public exponent. The exponent
 * is not stored since it's assumed to always be 65537.
 *
 * To optimize verification, the key block includes two precomputed
 * values, |n0inv| (fits in 32 bits) and |rr| and can always be
 * represented in |key_num_bits|.

 * The value |n0inv| is the value -1/n[0] (mod 2^32). The value |rr|
 * is (2^key_num_bits)^2 (mod n).
 *
 * Following this header is |key_num_bits| bits of |n|, then
 * |key_num_bits| bits of |rr|. Both values are stored with most
 * significant bit first. Each serialized number takes up
 * |key_num_bits|/8 bytes.
 *
 * All fields in this struct are stored in network byte order when
 * serialized.  To generate a copy with fields swapped to native byte
 * order, use the function bvb_rsa_public_key_header_to_host_byte_order().
 *
 * The bvb_RSAVerify() function expects a key in this serialized
 * format.
 *
 * The 'bvbtool extract_public_key' command can be used to generate a
 * serialized RSA public key.
 */
typedef struct BvbRSAPublicKeyHeader {
  uint32_t key_num_bits;
  uint32_t n0inv;
} __attribute__((packed)) BvbRSAPublicKeyHeader;

/* The header for a serialized property.
 *
 * Following this header is |key_num_bytes| bytes of key data,
 * followed by a NUL byte, then |value_num_bytes| bytes of value data,
 * followed by a NUL byte and then enough padding to make the combined
 * size a multiple of 8.
 *
 * Headers with keys beginning with "brillo." are reserved for use in
 * the Brillo project and must not be used by others. Well-known
 * headers include
 *
 *   brillo.device_tree: The property value is a device-tree blob.
 */
typedef struct BvbPropertyHeader {
  uint64_t key_num_bytes;
  uint64_t value_num_bytes;
} __attribute__((packed)) BvbPropertyHeader;

/* Binary format for header of the boot image used in Brillo.
 *
 * The Brillo boot image consists of four blocks:
 *
 *  +-----------------------------------------+
 *  | Header data - fixed size                |
 *  +-----------------------------------------+
 *  | Authentication data - variable size     |
 *  +-----------------------------------------+
 *  | Auxilary data - variable size           |
 *  +-----------------------------------------+
 *  | Payload data - variable size            |
 *  +-----------------------------------------+
 *
 * The "Header data" block is described by this struct and is always
 * |BVB_BOOT_IMAGE_HEADER_SIZE| bytes long.
 *
 * The "Authentication data" block is |authentication_data_block_size|
 * bytes long and contains the hash and signature used to authenticate
 * the boot image. The type of the hash and signature is defined by
 * the |algorithm_type| field.
 *
 * The "Auxilary data" is |auxilary_data_block_size| bytes long and
 * contains the auxilary data including the public key used to make
 * the signature and properties.
 *
 * The public key is at offset |public_key_offset| with size
 * |public_key_size| in this block. The size of the public key data is
 * defined by the |algorithm_type| field. The format of the public key
 * data is described in the |BvbRSAPublicKeyHeader| struct.
 *
 * The properties starts at |properties_offset| from the beginning of
 * the "Auxliary Data" block and take up |properties_size| bytes. Each
 * property is stored as |BvbPropertyHeader| with key, NUL, value,
 * NUL, and padding following. The number of properties can be
 * determined by walking this data until |properties_size| is
 * exhausted.
 *
 * The "Payload data" block is |payload_data_block_size| bytes
 * long. This is where the kernel, initramfs, and other data is
 * stored.
 *
 * The size of each of the "Authentication data" and "Auxilary data"
 * blocks must be divisible by 64. This is to ensure proper alignment.
 *
 * Properties are free-form key/value pairs stored in a part of the
 * boot partition subject to the same integrity checks as the rest of
 * the boot partition. See the documentation for |BvbPropertyHeader|
 * for well-known properties. See bvb_property_lookup() and
 * bvb_property_lookup_uint64() for convenience functions to look up
 * property values.
 *
 * This struct is versioned, see the |header_version_major| and
 * |header_version_minor| fields. Compatibility is guaranteed only
 * within the same major version.
 *
 * All fields are stored in network byte order when serialized. To
 * generate a copy with fields swapped to native byte order, use the
 * function bvb_boot_image_header_to_host_byte_order().
 *
 * Before reading and/or using any of this data, you MUST verify it
 * using bvb_verify_boot_image() and reject it unless it's signed by a
 * known good public key.
 */
typedef struct BvbBootImageHeader {
  /*   0: Four bytes equal to "BVB0" (BVB_MAGIC). */
  uint8_t magic[BVB_MAGIC_LEN];
  /*   4: The major version of the boot image header. */
  uint32_t header_version_major;
  /*   8: The minor version of the boot image header. */
  uint32_t header_version_minor;

  /*  12: The size of the signature block. */
  uint64_t authentication_data_block_size;
  /*  20: The size of the public key block. */
  uint64_t auxilary_data_block_size;
  /*  28: The size of the payload block. */
  uint64_t payload_data_block_size;

  /*  36: The verification algorithm used, see |BvbAlgorithmType| enum. */
  uint32_t algorithm_type;

  /*  40: Offset into the "Authentication data" block of hash data. */
  uint64_t hash_offset;
  /*  48: Length of the hash data. */
  uint64_t hash_size;

  /*  56: Offset into the "Authentication data" block of signature data. */
  uint64_t signature_offset;
  /*  64: Length of the signature data. */
  uint64_t signature_size;

  /*  72: Offset into the "Auxilary data" block of public key data. */
  uint64_t public_key_offset;
  /*  80: Length of the public key data. */
  uint64_t public_key_size;

  /*  88: Offset into the "Auxilary data" block of property data. */
  uint64_t properties_offset;
  /*  96: Length of property data. */
  uint64_t properties_size;

  /* 104: The rollback index which can be used to prevent rollback to
   *  older versions.
   */
  uint64_t rollback_index;

  /* 112: Offset into the "Payload data" block of kernel image. */
  uint64_t kernel_offset;
  /* 120: Length of the kernel image. */
  uint64_t kernel_size;

  /* 128: Offset into the "Payload data" block of initial ramdisk. */
  uint64_t initrd_offset;
  /* 136: Length of the initial ramdisk. */
  uint64_t initrd_size;

  /* 144: Physical kernel load address. */
  uint64_t kernel_addr;

  /* 152: Physical initial ramdisk load address. */
  uint64_t initrd_addr;

  /* 160: The NUL-terminated kernel command-line string, passed to the
   * Linux kernel.
   *
   * Limited substitution will be performed by the boot loader - the
   * following variables are supported:
   *
   *   $(ANDROID_SYSTEM_PARTUUID) - this place-holder will be replaced
   *   with the image UUID/GUID of the corresponding system_X image of
   *   the booted slot (where _X is the slot to boot).
   *
   *   $(ANDROID_BOOT_PARTUUID) - this place-holder will be replaced
   *   with the image UUID/GUID of the boot image of the booted slot.
   *
   * For example, the snippet "root=PARTUUID=$(ANDROID_SYSTEM_PARTUUID)"
   * in this field can be used to instruct the Linux kernel to use the
   * system image in the booted slot as the root filesystem.
   */
  uint8_t kernel_cmdline[BVB_KERNEL_CMDLINE_MAX_LEN];

  /* 4256: Padding to ensure struct is size BVB_BOOT_IMAGE_HEADER_SIZE
   * bytes. This must be set to zeroes.
   */
  uint8_t reserved[3936];
} __attribute__((packed)) BvbBootImageHeader;

#ifdef __cplusplus
}
#endif

#endif  /* BVB_BOOT_IMAGE_HEADER_H_ */
