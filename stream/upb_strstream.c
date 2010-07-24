/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_strstream.h"

#include <stdlib.h>
#include "upb_string.h"

struct upb_stringsrc {
  upb_bytesrc bytesrc;
  upb_string *str;
};

void upb_stringsrc_reset(upb_stringsrc *s, upb_string *str) {
  if (str != s->str) {
    if (s->str) upb_string_unref(s->str);
    s->str = upb_string_getref(str);
  }
  s->bytesrc.eof = false;
}

void upb_stringsrc_free(upb_stringsrc *s) {
  if (s->str) upb_string_unref(s->str);
  free(s);
}

static bool upb_stringsrc_get(upb_stringsrc *src, upb_string *str,
                              upb_strlen_t minlen) {
  // We ignore "minlen" since we always return the entire string.
  (void)minlen;
  upb_string_substr(str, src->str, 0, upb_string_len(src->str));
  src->bytesrc.eof = true;
  return true;
}

static bool upb_stringsrc_append(upb_stringsrc *src, upb_string *str,
                                 upb_strlen_t len) {
  // Unimplemented; since we return the string via "get" all in one go,
  // this method probably isn't very useful.
  (void)src;
  (void)str;
  (void)len;
  return false;
}

static upb_bytesrc_vtable upb_stringsrc_vtbl = {
  (upb_bytesrc_get_fptr)upb_stringsrc_get,
  (upb_bytesrc_append_fptr)upb_stringsrc_append,
};

upb_stringsrc *upb_stringsrc_new() {
  upb_stringsrc *s = malloc(sizeof(*s));
  upb_bytesrc_init(&s->bytesrc, &upb_stringsrc_vtbl);
  return s;
}

upb_bytesrc *upb_stringsrc_bytesrc(upb_stringsrc *s) {
  return &s->bytesrc;
}
