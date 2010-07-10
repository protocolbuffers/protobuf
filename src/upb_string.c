/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_string.h"

#include <stdlib.h>

#define UPB_STRING_UNFINALIZED -1

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
  str->size = 0;
  str->len = UPB_STRING_UNFINALIZED;
  upb_atomic_refcount_init(&str->refcount, 1);
  return str;
}

void _upb_string_free(upb_string *str) {
  if(str->ptr) free(str->ptr);
  free(str);
}

char *upb_string_getrwbuf(upb_string *str, upb_strlen_t len) {
  assert(str->len == UPB_STRING_UNFINALIZED);
  if (str->size < len) {
    str->size = upb_round_up_pow2(len);
    str->ptr = realloc(str->ptr, str->size);
  }
  str->len = len;
  return str->ptr;
}
