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
  if(str == NULL || upb_atomic_read(&str->refcount) > 1) {
    return upb_string_new();
  } else {
    str->ptr = NULL;
    upb_string_release(str);
    return str;
  }
}

char *upb_string_getrwbuf(upb_string *str, upb_strlen_t len) {
  assert(str->ptr == NULL);
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
  return str->ptr;
}

void upb_string_substr(upb_string *str, upb_string *target_str,
                       upb_strlen_t start, upb_strlen_t len) {
  assert(str->ptr == NULL);
  str->src = upb_string_getref(target_str);
  str->ptr = upb_string_getrobuf(target_str) + start;
  str->len = len;
}
