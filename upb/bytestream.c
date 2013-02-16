/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/bytestream.h"

#include <stdlib.h>
#include <string.h>


/* upb_byteregion *************************************************************/

char *upb_byteregion_strdup(const upb_byteregion *r) {
  char *ret = malloc(upb_byteregion_len(r) + 1);
  upb_byteregion_copyall(r, ret);
  ret[upb_byteregion_len(r)] = '\0';
  return ret;
}

upb_byteregion *upb_byteregion_new(const void *str) {
  return upb_byteregion_newl(str, strlen(str));
}

upb_byteregion *upb_byteregion_newl(const void *str, size_t len) {
  upb_stringsrc *src = malloc(sizeof(*src));
  upb_stringsrc_init(src);
  char *ptr = malloc(len + 1);
  memcpy(ptr, str, len);
  ptr[len] = '\0';
  upb_stringsrc_reset(src, ptr, len);
  return upb_stringsrc_allbytes(src);
}

void upb_byteregion_free(upb_byteregion *r) {
  if (!r) return;
  size_t len;
  free((char*)upb_byteregion_getptr(r, 0, &len));
  upb_stringsrc_uninit((upb_stringsrc*)r->bytesrc);
  free(r->bytesrc);
}

void upb_bytesink_init(upb_bytesink *sink, upb_bytesink_vtbl *vtbl) {
  sink->vtbl = vtbl;
  upb_status_init(&sink->status);
}

void upb_bytesink_uninit(upb_bytesink *sink) {
  upb_status_uninit(&sink->status);
}

void upb_byteregion_reset(upb_byteregion *r, const upb_byteregion *src,
                          uint64_t ofs, uint64_t len) {
  assert(ofs >= upb_byteregion_startofs(src));
  assert(len <= upb_byteregion_remaining(src, ofs));
  r->bytesrc = src->bytesrc;
  r->toplevel = false;
  r->start = ofs;
  r->discard = ofs;
  r->end = ofs + len;
  r->fetch = UPB_MIN(src->fetch, r->end);
}

upb_bytesuccess_t upb_byteregion_fetch(upb_byteregion *r) {
  uint64_t fetchable = upb_byteregion_remaining(r, r->fetch);
  if (fetchable == 0) return UPB_BYTE_EOF;
  size_t fetched;
  upb_bytesuccess_t ret = upb_bytesrc_fetch(r->bytesrc, r->fetch, &fetched);
  if (ret != UPB_BYTE_OK) return false;
  r->fetch += UPB_MIN(fetched, fetchable);
  return UPB_BYTE_OK;
}


/* upb_stringsrc **************************************************************/

upb_bytesuccess_t upb_stringsrc_fetch(void *_src, uint64_t ofs, size_t *read) {
  upb_stringsrc *src = _src;
  assert(ofs < src->len);
  if (ofs == src->len) {
    upb_status_seteof(&src->bytesrc.status);
    return UPB_BYTE_EOF;
  }
  *read = src->len - ofs;
  return UPB_BYTE_OK;
}

void upb_stringsrc_copy(const void *_src, uint64_t ofs,
                        size_t len, char *dst) {
  const upb_stringsrc *src = _src;
  assert(ofs + len <= src->len);
  memcpy(dst, src->str + ofs, len);
}

void upb_stringsrc_discard(void *src, uint64_t ofs) {
  (void)src;
  (void)ofs;
}

const char *upb_stringsrc_getptr(const void *_s, uint64_t ofs, size_t *len) {
  const upb_stringsrc *src = _s;
  *len = src->len - ofs;
  return src->str + ofs;
}

void upb_stringsrc_init(upb_stringsrc *s) {
  static upb_bytesrc_vtbl vtbl = {
    &upb_stringsrc_fetch,
    &upb_stringsrc_discard,
    &upb_stringsrc_copy,
    &upb_stringsrc_getptr,
  };
  upb_bytesrc_init(&s->bytesrc, &vtbl);
  s->str = NULL;
  s->byteregion.bytesrc = &s->bytesrc;
  s->byteregion.toplevel = true;
}

void upb_stringsrc_reset(upb_stringsrc *s, const char *str, size_t len) {
  s->str = str;
  s->len = len;
  s->byteregion.start = 0;
  s->byteregion.discard = 0;
  s->byteregion.fetch = 0;
  s->byteregion.end = len;
}

void upb_stringsrc_uninit(upb_stringsrc *s) { (void)s; }

/* upb_stringsink *************************************************************/

void upb_stringsink_uninit(upb_stringsink *s) {
  free(s->str);
}

void upb_stringsink_reset(upb_stringsink *s, char *str, size_t size) {
  free(s->str);
  s->str = str;
  s->len = 0;
  s->size = size;
}

upb_bytesink *upb_stringsink_bytesink(upb_stringsink *s) {
  return &s->bytesink;
}

static int32_t upb_stringsink_vprintf(void *_s, const char *fmt, va_list args) {
  // TODO: detect realloc() errors.
  upb_stringsink *s = _s;
  int ret = upb_vrprintf(&s->str, &s->size, s->len, fmt, args);
  if (ret >= 0) s->len += ret;
  return ret;
}

int upb_stringsink_write(void *_s, const void *buf, int len) {
  // TODO: detect realloc() errors.
  upb_stringsink *s = _s;
  if (s->len + len > s->size) {
    while(s->len + len > s->size) s->size *= 2;
    s->str = realloc(s->str, s->size);
  }
  memcpy(s->str + s->len, buf, len);
  s->len += len;
  return len;
}

void upb_stringsink_init(upb_stringsink *s) {
  static upb_bytesink_vtbl vtbl = {
    upb_stringsink_write,
    upb_stringsink_vprintf
  };
  upb_bytesink_init(&s->bytesink, &vtbl);
  s->str = NULL;
}
