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

#ifndef BVB_PROPERTY_H_
#define BVB_PROPERTY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bvb_boot_image_header.h"

/* Convenience function for looking up the value for a property with
 * name |key| in a Brillo boot image. If |key| is NUL-terminated,
 * |key_size| may be set to 0.
 *
 * The |image_data| parameter must be a pointer to a Brillo Boot Image
 * of size |image_size|.
 *
 * This function returns a pointer to the value inside the passed-in
 * image or NULL if not found. Note that the value is always
 * guaranteed to be followed by a NUL byte.
 *
 * If the value was found and |out_value_size| is not NULL, the size
 * of the value is returned there.
 *
 * This function is O(n) in number of properties so if you need to
 * look up a lot of values, you may want to build a more efficient
 * lookup-table by manually walking all properties yourself.
 *
 * Before using this function, you MUST verify |image_data| with
 * bvb_verify_boot_image() and reject it unless it's signed by a known
 * good public key.
 */
const char* bvb_lookup_property(const uint8_t* image_data, size_t image_size,
                                const char* key, size_t key_size,
                                size_t* out_value_size);

/* Like bvb_lookup_property() but parses the value as an unsigned
 * 64-bit integer. Both decimal and hexadecimal representations
 * (e.g. "0x2a") are supported. Returns 0 on failure and non-zero on
 * success. On success, the parsed value is returned in |out_value|.
 */
int bvb_lookup_property_uint64(const uint8_t* image_data, size_t image_size,
                               const char* key, size_t key_size,
                               uint64_t* out_value);

#ifdef __cplusplus
}
#endif

#endif  /* BVB_PROPERTY_H_ */
