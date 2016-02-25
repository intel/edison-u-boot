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

#ifndef BVB_UTIL_H_
#define BVB_UTIL_H_

#include "bvb_boot_image_header.h"
#include "bvb_sysdeps.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Converts a 32-bit unsigned integer from big-endian to host byte order. */
uint32_t bvb_be32toh(uint32_t in);

/* Converts a 64-bit unsigned integer from big-endian to host byte order. */
uint64_t bvb_be64toh(uint64_t in);

/* Adds |value_to_add| to |value| with overflow protection.
 *
 * Returns zero if the addition overflows, non-zero otherwise. In
 * either case, |value| is always modified.
 */
int bvb_safe_add_to(uint64_t *value, uint64_t value_to_add);

/* Adds |a| and |b| with overflow protection, returning the value in
 * |out_result|.
 *
 * It's permissible to pass NULL for |out_result| if you just want to
 * check that the addition would not overflow.
 *
 * Returns zero if the addition overflows, non-zero otherwise.
 */
int bvb_safe_add(uint64_t *out_result, uint64_t a, uint64_t b);

/* Copies |src| to |dest|, byte-swapping fields in the process. */
void bvb_boot_image_header_to_host_byte_order(
    const BvbBootImageHeader* src,
    BvbBootImageHeader* dest);

/* Copies |header| to |dest|, byte-swapping fields in the process. */
void bvb_rsa_public_key_header_to_host_byte_order(
    const BvbRSAPublicKeyHeader* src,
    BvbRSAPublicKeyHeader* dest);

#ifdef __cplusplus
}
#endif

#endif  /* BVB_UTIL_H_ */
