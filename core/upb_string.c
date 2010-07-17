/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_string.h"

#include <stdlib.h>
#ifdef __GLIBC__
#include <malloc.h>
#elif defined(__APPLE__)
#include <malloc/malloc.h>
#endif

static uint32_t upb_round_up_pow2(uint32_t v) {
  // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

upb_string *upb_string_new() {
  upb_string *str = malloc(sizeof(*str));
  str->ptr = NULL;
  str->cached_mem = NULL;
#ifndef UPB_HAVE_MSIZE
  str->size = 0;
#endif
  str->src = NULL;
  upb_atomic_refcount_init(&str->refcount, 1);
  return str;
}

uint32_t upb_string_size(upb_string *str) {
#ifdef __GLIBC__
  return malloc_usable_size(str->cached_mem);
#elif defined(__APPLE__)
  return malloc_size(str->cached_mem);
#else
  return str->size;
#endif
}

static void upb_string_release(upb_string *str) {
  if(str->src) {
    upb_string_unref(str->src);
    str->src = NULL;
  }
}

void _upb_string_free(upb_string *str) {
  if(str->cached_mem) free(str->cached_mem);
  upb_string_release(str);
  free(str);
}

upb_string *upb_string_tryrecycle(upb_string *str) {
  if(str && upb_atomic_read(&str->refcount) == 1) {
    str->ptr = NULL;
    upb_string_release(str);
    return str;
  } else {
    return upb_string_new();
  }
}

char *upb_string_getrwbuf(upb_string *str, upb_strlen_t len) {
  // assert(str->ptr == NULL);
  uint32_t size = upb_string_size(str);
  if (size < len) {
    size = upb_round_up_pow2(len);
    str->cached_mem = realloc(str->cached_mem, size);
#ifndef UPB_HAVE_MSIZE
    str->size = size;
#endif
  }
  str->len = len;
  str->ptr = str->cached_mem;
  return str->cached_mem;
}

void upb_string_substr(upb_string *str, upb_string *target_str,
                       upb_strlen_t start, upb_strlen_t len) {
  if(str->ptr) *(char*)0 = 0;
  assert(str->ptr == NULL);
  str->src = upb_string_getref(target_str);
  str->ptr = upb_string_getrobuf(target_str) + start;
  str->len = len;
}

void upb_string_vprintf(upb_string *str, const char *format, va_list args) {
  // Try once without reallocating.  We have to va_copy because we might have
  // to call vsnprintf again.
  uint32_t size = UPB_MAX(upb_string_size(str), 16);
  char *buf = upb_string_getrwbuf(str, size);
  va_list args_copy;
  va_copy(args_copy, args);
  uint32_t true_size = vsnprintf(buf, size, format, args_copy);
  va_end(args_copy);

  if (true_size >= size) {
    // Need to reallocate.  We reallocate even if the sizes were equal,
    // because snprintf excludes the terminating NULL from its count.
    // We don't care about the terminating NULL, but snprintf might
    // bail out of printing even other characters if it doesn't have
    // enough space to write the NULL also.
    str = upb_string_tryrecycle(str);
    buf = upb_string_getrwbuf(str, true_size + 1);
    vsnprintf(buf, true_size + 1, format, args);
  }
  str->len = true_size;
}

upb_string *upb_string_asprintf(const char *format, ...) {
  upb_string *str = upb_string_new();
  va_list args;
  va_start(args, format);
  upb_string_vprintf(str, format, args);
  va_end(args);
  return str;
}

upb_string *upb_strdup(upb_string *s) {
  upb_string *str = upb_string_new();
  upb_strcpy(str, s);
  return str;
}
