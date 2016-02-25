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

#ifndef BVB_SYSDEPS_H_
#define BVB_SYSDEPS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Change these includes to match your platform to bring in the
 * equivalent types available in a normal C runtime, as well as
 * printf()-format specifiers such as PRIx64.
 */
#include <stddef.h>
#include <compiler.h>
#include <inttypes.h>

#ifdef BVB_ENABLE_DEBUG
/* Aborts the program if |expr| is false.
 *
 * This has no effect unless BVB_ENABLE_DEBUG is defined.
 */
#define bvb_assert(expr) do { if (!(expr)) { \
    bvb_error("assert fail: %s at %s:%d\n", \
              #expr, __FILE__, __LINE__); }} while(0)
#else
#define bvb_assert(expr)
#endif

/* Size in bytes used for word-alignment.
 *
 * Change this to match your architecture - must be a power of two.
 */
#define BVB_WORD_ALIGNMENT_SIZE 8

/* Aborts the program if |addr| is not word-aligned.
 *
 * This has no effect unless BVB_ENABLE_DEBUG is defined.
 */
#define bvb_assert_word_aligned(addr) \
    bvb_assert((((uintptr_t) addr) & (BVB_WORD_ALIGNMENT_SIZE-1)) == 0)

/* Compare |n| bytes in |src1| and |src2|.
 *
 * Returns an integer less than, equal to, or greater than zero if the
 * first |n| bytes of |src1| is found, respectively, to be less than,
 * to match, or be greater than the first |n| bytes of |src2|. */
int bvb_memcmp(const void* src1, const void* src2, size_t n);

/* Copy |n| bytes from |src| to |dest|. */
void* bvb_memcpy(void* dest, const void* src, size_t n);

/* Set |n| bytes starting at |s| to |c|.  Returns |dest|. */
void* bvb_memset(void* dest, const int c, size_t n);

/* Compare |n| bytes starting at |s1| with |s2| and return 0 if they
 * match, 1 if they don't.  Returns 0 if |n|==0, since no bytes
 * mismatched.
 *
 * Time taken to perform the comparison is only dependent on |n| and
 * not on the relationship of the match between |s1| and |s2|.
 *
 * Note that unlike bvb_memcmp(), this only indicates inequality, not
 * whether |s1| is less than or greater than |s2|.
 */
int bvb_safe_memcmp(const void* s1, const void* s2, size_t n);

#ifdef BVB_ENABLE_DEBUG
/* printf()-style function, used for diagnostics.
 *
 * This has no effect unless BVB_ENABLE_DEBUG is defined.
 */
void bvb_debug(const char* format, ...) __attribute__((format(printf, 1, 2)));
#else
static inline void bvb_debug(const char* format, ...)
    __attribute__((format(printf, 1, 2)));
static inline void bvb_debug(const char* format, ...) {}
#endif

/* Prints out a message (defined by |format|, printf()-style) and
 * aborts the program or reboots the device.
 *
 * Unlike bvb_debug(), this function does not depend on BVB_ENABLE_DEBUG.
 */
void bvb_error(const char* format, ...) __attribute__((format(printf, 1, 2)));

/* Allocates |size| bytes. Returns NULL if no memory is available,
 * otherwise a pointer to the allocated memory.
 *
 * The memory is not initialized.
 *
 * The pointer returned is guaranteed to be word-aligned.
 *
 * The memory should be freed with bvb_free() when you are done with it.
 */
void* bvb_malloc(size_t size);

/* Frees memory previously allocated with bvb_malloc(). */
void bvb_free(void* ptr);

/* Returns the lenght of |str|, excluding the terminating NUL-byte. */
size_t bvb_strlen(const char* str);

#ifdef __cplusplus
}
#endif

#endif  /* BVB_SYSDEPS_H_ */
