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

#include <stdarg.h>
#include <stdlib.h>

#include "common.h"

#include "bvb/bvb_sysdeps.h"

int bvb_memcmp(const void* src1, const void* src2, size_t n) {
  return memcmp(src1, src2, n);
}

void* bvb_memcpy(void* dest, const void* src, size_t n) {
  return memcpy(dest, src, (size_t)n);
}

void* bvb_memset(void* dest, const int c, size_t n) {
  return memset(dest, c, n);
}

size_t bvb_strlen(const char* str) {
  return strlen(str);
}

int bvb_safe_memcmp(const void* s1, const void* s2, size_t n) {
  const unsigned char* us1 = s1;
  const unsigned char* us2 = s2;
  int result = 0;

  if (0 == n)
    return 0;

  /*
   * Code snippet without data-dependent branch due to Nate Lawson
   * (nate@root.org) of Root Labs.
   */
  while (n--)
    result |= *us1++ ^ *us2++;

  return result != 0;
}

#ifdef BVB_HAS_STDIO
void bvb_error(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "ERROR: ");
  vfprintf(stderr, format, ap);
  va_end(ap);
  exit(1);
}

#ifdef BVB_ENABLE_DEBUG
void bvb_debug(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "DEBUG: ");
  vfprintf(stderr, format, ap);
  va_end(ap);
}
#endif
#else

void bvb_error(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
}

#ifdef BVB_ENABLE_DEBUG
void bvb_debug(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
}
#endif

#endif

void* bvb_malloc(size_t size) {
  return malloc(size);
}

void bvb_free(void* ptr) {
  free(ptr);
}
