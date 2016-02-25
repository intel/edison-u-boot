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


#ifdef BVB_INSIDE_BVB_REFIMPL_H
#error "You can't include bvb_sha.h in the public header bvb_refimpl.h."
#endif

#ifndef BVB_REFIMPL_COMPILATION
#error "Never include this file, it may only be used from internal bvb code."
#endif

#ifndef BVB_SHA_H_
#define BVB_SHA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bvb_sysdeps.h"

/* Size in bytes of a SHA-256 digest. */
#define BVB_SHA256_DIGEST_SIZE 32

/* Block size in bytes of a SHA-256 digest. */
#define BVB_SHA256_BLOCK_SIZE 64

/* Size in bytes of a SHA-512 digest. */
#define BVB_SHA512_DIGEST_SIZE 64

/* Block size in bytes of a SHA-512 digest. */
#define BVB_SHA512_BLOCK_SIZE 128

/* Data structure used for SHA-256. */
typedef struct {
  uint32_t h[8];
  uint32_t tot_len;
  uint32_t len;
  uint8_t block[2 * BVB_SHA256_BLOCK_SIZE];
  uint8_t buf[BVB_SHA256_DIGEST_SIZE];  /* Used for storing the final digest. */
} BvbSHA256Ctx;

/* Data structure used for SHA-512. */
typedef struct {
  uint64_t h[8];
  uint32_t tot_len;
  uint32_t len;
  uint8_t block[2 * BVB_SHA512_BLOCK_SIZE];
  uint8_t buf[BVB_SHA512_DIGEST_SIZE];  /* Used for storing the final digest. */
} BvbSHA512Ctx;

/* Initializes the SHA-256 context. */
void bvb_sha256_init(BvbSHA256Ctx* ctx);

/* Updates the SHA-256 context with |len| bytes from |data|. */
void bvb_sha256_update(BvbSHA256Ctx* ctx, const uint8_t* data, uint32_t len);

/* Returns the SHA-256 digest. */
uint8_t* bvb_sha256_final(BvbSHA256Ctx* ctx);

/* Initializes the SHA-512 context. */
void bvb_sha512_init(BvbSHA512Ctx* ctx);

/* Updates the SHA-512 context with |len| bytes from |data|. */
void bvb_sha512_update(BvbSHA512Ctx* ctx, const uint8_t* data, uint32_t len);

/* Returns the SHA-512 digest. */
uint8_t* bvb_sha512_final(BvbSHA512Ctx* ctx);

#ifdef __cplusplus
}
#endif

#endif  /* BVB_SHA_H_ */
