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


#include "bvb/bvb_util.h"

uint32_t bvb_be32toh(uint32_t in) {
  uint8_t* d = (uint8_t*) &in;
  uint32_t ret;
  ret  = ((uint32_t) d[0]) << 24;
  ret |= ((uint32_t) d[1]) << 16;
  ret |= ((uint32_t) d[2]) << 8;
  ret |= ((uint32_t) d[3]);
  return ret;
}

uint64_t bvb_be64toh(uint64_t in) {
  uint8_t* d = (uint8_t*) &in;
  uint64_t ret;
  ret  = ((uint64_t) d[0]) << 56;
  ret |= ((uint64_t) d[1]) << 48;
  ret |= ((uint64_t) d[2]) << 40;
  ret |= ((uint64_t) d[3]) << 32;
  ret |= ((uint64_t) d[4]) << 24;
  ret |= ((uint64_t) d[5]) << 16;
  ret |= ((uint64_t) d[6]) << 8;
  ret |= ((uint64_t) d[7]);
  return ret;
}

void bvb_boot_image_header_to_host_byte_order(const BvbBootImageHeader* src,
                                              BvbBootImageHeader* dest) {
  bvb_memcpy(dest, src, sizeof(BvbBootImageHeader));

  dest->header_version_major = bvb_be32toh(dest->header_version_major);
  dest->header_version_minor = bvb_be32toh(dest->header_version_minor);

  dest->authentication_data_block_size =
      bvb_be64toh(dest->authentication_data_block_size);
  dest->auxilary_data_block_size = bvb_be64toh(dest->auxilary_data_block_size);
  dest->payload_data_block_size = bvb_be64toh(dest->payload_data_block_size);

  dest->algorithm_type = bvb_be32toh(dest->algorithm_type);

  dest->hash_offset = bvb_be64toh(dest->hash_offset);
  dest->hash_size = bvb_be64toh(dest->hash_size);

  dest->signature_offset = bvb_be64toh(dest->signature_offset);
  dest->signature_size = bvb_be64toh(dest->signature_size);

  dest->public_key_offset = bvb_be64toh(dest->public_key_offset);
  dest->public_key_size = bvb_be64toh(dest->public_key_size);

  dest->properties_offset = bvb_be64toh(dest->properties_offset);
  dest->properties_size = bvb_be64toh(dest->properties_size);

  dest->rollback_index = bvb_be64toh(dest->rollback_index);

  dest->kernel_offset = bvb_be64toh(dest->kernel_offset);
  dest->kernel_size = bvb_be64toh(dest->kernel_size);

  dest->initrd_offset = bvb_be64toh(dest->initrd_offset);
  dest->initrd_size = bvb_be64toh(dest->initrd_size);

  dest->kernel_addr = bvb_be64toh(dest->kernel_addr);
  dest->initrd_addr = bvb_be64toh(dest->initrd_addr);
}

void bvb_rsa_public_key_header_to_host_byte_order(
    const BvbRSAPublicKeyHeader* src,
    BvbRSAPublicKeyHeader* dest) {
  bvb_memcpy(dest, src, sizeof(BvbRSAPublicKeyHeader));

  dest->key_num_bits = bvb_be32toh(dest->key_num_bits);
  dest->n0inv = bvb_be32toh(dest->n0inv);
}

int bvb_safe_add_to(uint64_t *value, uint64_t value_to_add) {
  uint64_t original_value;

  bvb_assert(value != NULL);

  original_value = *value;

  *value += value_to_add;
  if (*value < original_value) {
    bvb_debug("%s: overflow: 0x%016" PRIx64 " + 0x%016" PRIx64 "\n",
              __FUNCTION__, original_value, value_to_add);
    return 0;
  }

  return 1;
}

int bvb_safe_add(uint64_t* out_result, uint64_t a, uint64_t b) {
  uint64_t dummy;
  if (out_result == NULL)
    out_result = &dummy;
  *out_result = a;
  return bvb_safe_add_to(out_result, b);
}
