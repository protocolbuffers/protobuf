/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb_strstream.h"

#include <stdlib.h>


/* upb_stringsrc **************************************************************/

size_t upb_stringsrc_fetch(void *_src, uint64_t ofs, upb_status *s) {
  upb_stringsrc *src = _src;
  size_t bytes = src->len - ofs;
  if (bytes == 0) s->code = UPB_EOF;
  return bytes;
}

void upb_stringsrc_read(void *_src, uint64_t src_ofs, size_t len, char *dst) {
  upb_stringsrc *src = _src;
  memcpy(dst, src->str + src_ofs, len);
}

const char *upb_stringsrc_getptr(void *_src, uint64_t ofs, size_t *len) {
  upb_stringsrc *src = _src;
  *len = src->len - ofs;
  return src->str + ofs;
}

void upb_stringsrc_init(upb_stringsrc *s) {
  static upb_bytesrc_vtbl vtbl = {
    &upb_stringsrc_fetch,
    &upb_stringsrc_read,
    &upb_stringsrc_getptr,
    NULL, NULL, NULL, NULL
  };
  upb_bytesrc_init(&s->bytesrc, &vtbl);
  s->str = NULL;
}

void upb_stringsrc_reset(upb_stringsrc *s, const char *str, size_t len) {
  s->str = str;
  s->len = len;
}

void upb_stringsrc_uninit(upb_stringsrc *s) { (void)s; }

upb_bytesrc *upb_stringsrc_bytesrc(upb_stringsrc *s) {
  return &s->bytesrc;
}


/* upb_stringsink *************************************************************/

void upb_stringsink_uninit(upb_stringsink *s) {
  free(s->str);
}

// Resets the stringsink to a state where it will append to the given string.
// The string must be newly created or recycled.  The stringsink will take a
// reference on the string, so the caller need not ensure that it outlives the
// stringsink.  A stringsink can be reset multiple times.
void upb_stringsink_reset(upb_stringsink *s, char *str, size_t size) {
  free(s->str);
  s->str = str;
  s->len = 0;
  s->size = size;
}

upb_bytesink *upb_stringsink_bytesink(upb_stringsink *s) {
  return &s->bytesink;
}

static int32_t upb_stringsink_vprintf(void *_s, upb_status *status,
                                      const char *fmt, va_list args) {
  (void)status;  // TODO: report realloc() errors.
  upb_stringsink *s = _s;
  int ret = upb_vrprintf(&s->str, &s->size, s->len, fmt, args);
  if (ret >= 0) s->len += ret;
  return ret;
}

bool upb_stringsink_write(void *_s, const char *buf, size_t len,
                          upb_status *status) {
  (void)status;  // TODO: report realloc() errors.
  upb_stringsink *s = _s;
  if (s->len + len > s->size) {
    while(s->len + len > s->size) s->size *= 2;
    s->str = realloc(s->str, s->size);
  }
  memcpy(s->str + s->len, buf, len);
  s->len += len;
  return true;
}

void upb_stringsink_init(upb_stringsink *s) {
  static upb_bytesink_vtbl vtbl = {
    upb_stringsink_write,
    upb_stringsink_vprintf
  };
  upb_bytesink_init(&s->bytesink, &vtbl);
  s->str = NULL;
}
