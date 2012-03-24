/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/bytestream.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// We can make this configurable if necessary.
#define BUF_SIZE 32768

char *upb_byteregion_strdup(const struct _upb_byteregion *r) {
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


/* upb_stdio ******************************************************************/

int upb_stdio_cmpbuf(const void *_key, const void *_elem) {
  const uint64_t *ofs = _key;
  const upb_stdio_buf *buf = _elem;
  return (*ofs / BUF_SIZE) - (buf->ofs / BUF_SIZE);
}

static upb_stdio_buf *upb_stdio_findbuf(const upb_stdio *s, uint64_t ofs) {
  // TODO: it is probably faster to linear search short lists, and to
  // special-case the last one or two bufs.
  return bsearch(&ofs, s->bufs, s->nbuf, sizeof(*s->bufs), &upb_stdio_cmpbuf);
}

static upb_stdio_buf *upb_stdio_rotatebufs(upb_stdio *s) {
  upb_stdio_buf **reuse = NULL;  // XXX
  int num_reused = 0, num_inuse = 0;

  // Could sweep only a subset of bufs if this was a hotspot.
  for (int i = 0; i < s->nbuf; i++) {
    upb_stdio_buf *buf = s->bufs[i];
    if (buf->refcount > 0) {
      s->bufs[num_inuse++] = buf;
    } else {
      reuse[num_reused++] = buf;
    }
  }
  assert(num_reused + num_inuse == s->nbuf);
  memcpy(s->bufs + num_inuse, reuse, num_reused * sizeof(upb_stdio_buf*));
  if (num_reused == 0) {
    ++s->nbuf;
    s->bufs = realloc(s->bufs, s->nbuf * sizeof(*s->bufs));
    s->bufs[s->nbuf-1] = malloc(sizeof(upb_stdio_buf) + BUF_SIZE);
    return s->bufs[s->nbuf-1];
  }
  return s->bufs[s->nbuf-num_reused];
}

void upb_stdio_discard(void *src, uint64_t ofs) {
  (void)src;
  (void)ofs;
}

upb_bytesuccess_t upb_stdio_fetch(void *src, uint64_t ofs, size_t *bytes_read) {
  (void)ofs;
  upb_stdio *stdio = (upb_stdio*)src;
  upb_stdio_buf *buf = upb_stdio_rotatebufs(stdio);
retry:
  *bytes_read = fread(&buf->data, 1, BUF_SIZE, stdio->file);
  buf->len = *bytes_read;
  if (*bytes_read < (size_t)BUF_SIZE) {
    // Error or EOF.
    if (feof(stdio->file)) {
      upb_status_seteof(&stdio->src.status);
      return UPB_BYTE_EOF;
    }
    if (ferror(stdio->file)) {
#ifdef EINTR
      // If we encounter a client who doesn't want to retry EINTR, we can easily
      // add a boolean property of the stdio that controls this behavior.
      if (errno == EINTR) {
        clearerr(stdio->file);
        goto retry;
      }
#endif
      upb_status_fromerrno(&stdio->src.status);
      return upb_errno_is_wouldblock() ? UPB_BYTE_WOULDBLOCK : UPB_BYTE_ERROR;
    }
    assert(false);
  }
  return UPB_BYTE_OK;
}

void upb_stdio_copy(const void *src, uint64_t ofs, size_t len, char *dst) {
  upb_stdio_buf *buf = upb_stdio_findbuf(src, ofs);
  ofs -= buf->ofs;
  memcpy(dst, buf->data + ofs, BUF_SIZE - ofs);
  len -= (BUF_SIZE - ofs);
  dst += (BUF_SIZE - ofs);
  while (len > 0) {
    ++buf;
    size_t bytes = UPB_MIN(len, BUF_SIZE);
    memcpy(dst, buf->data, bytes);
    len -= bytes;
    dst += bytes;
  }
}

const char *upb_stdio_getptr(const void *src, uint64_t ofs, size_t *len) {
  upb_stdio_buf *buf = upb_stdio_findbuf(src, ofs);
  ofs -= buf->ofs;
  *len = BUF_SIZE - ofs;
  return &buf->data[ofs];
}

#if 0
upb_strlen_t upb_stdio_putstr(upb_bytesink *sink, upb_string *str, upb_status *status) {
  upb_stdio *stdio = (upb_stdio*)((char*)sink - offsetof(upb_stdio, sink));
  upb_strlen_t len = upb_string_len(str);
  upb_strlen_t written = fwrite(upb_string_getrobuf(str), 1, len, stdio->file);
  if (written < len) {
    upb_status_setf(status, UPB_ERROR, "Error writing to stdio stream.");
    return -1;
  }
  return written;
}

uint32_t upb_stdio_vprintf(upb_bytesink *sink, upb_status *status,
                           const char *fmt, va_list args) {
  upb_stdio *stdio = (upb_stdio*)((char*)sink - offsetof(upb_stdio, sink));
  int written = vfprintf(stdio->file, fmt, args);
  if (written < 0) {
    upb_status_seterrf(status, "Error writing to stdio stream.");
    return -1;
  }
  return written;
}
#endif

void upb_stdio_init(upb_stdio *stdio) {
  static upb_bytesrc_vtbl bytesrc_vtbl = {
    &upb_stdio_fetch,
    &upb_stdio_discard,
    &upb_stdio_copy,
    &upb_stdio_getptr,
  };
  upb_bytesrc_init(&stdio->src, &bytesrc_vtbl);

  //static upb_bytesink_vtbl bytesink_vtbl = {
  //  upb_stdio_putstr,
  //  upb_stdio_vprintf
  //};
  //upb_bytesink_init(&stdio->bytesink, &bytesink_vtbl);
}

void upb_stdio_reset(upb_stdio* stdio, FILE *file) {
  stdio->file = file;
  stdio->should_close = false;
}

void upb_stdio_open(upb_stdio *stdio, const char *filename, const char *mode,
                    upb_status *s) {
  FILE *f = fopen(filename, mode);
  if (!f) {
    upb_status_fromerrno(s);
    return;
  }
  setvbuf(stdio->file, NULL, _IONBF, 0);  // Disable buffering; we do our own.
  upb_stdio_reset(stdio, f);
  stdio->should_close = true;
}

void upb_stdio_uninit(upb_stdio *stdio) {
  // Can't report status; caller should flush() to ensure data is written.
  if (stdio->should_close) fclose(stdio->file);
  stdio->file = NULL;
}

upb_bytesrc* upb_stdio_bytesrc(upb_stdio *stdio) { return &stdio->src; }
upb_bytesink* upb_stdio_bytesink(upb_stdio *stdio) { return &stdio->sink; }


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
