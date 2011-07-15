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

char *upb_strref_dup(struct _upb_strref *r) {
  char *ret = (char*)malloc(r->len + 1);
  upb_bytesrc_read(r->bytesrc, r->stream_offset, r->len, ret);
  ret[r->len] = '\0';
  return ret;
}

/* upb_stdio ******************************************************************/

int upb_stdio_cmpbuf(const void *_key, const void *_elem) {
  const uint64_t *ofs = _key;
  const upb_stdio_buf *buf = _elem;
  return (*ofs / BUF_SIZE) - (buf->ofs / BUF_SIZE);
}

static upb_stdio_buf *upb_stdio_findbuf(upb_stdio *s, uint64_t ofs) {
  // TODO: it is probably faster to linear search short lists, and to
  // special-case the last one or two bufs.
  return bsearch(&ofs, s->bufs, s->nbuf, sizeof(*s->bufs), &upb_stdio_cmpbuf);
}

//static upb_strlen_t upb_stdio_read(void *src, uint32_t ofs, upb_buf *b,
//                                   upb_status *status) {
//  upb_stdio *stdio = (upb_stdio*)src;
//  size_t read = fread(buf, 1, BLOCK_SIZE, stdio->file);
//  if(read < (size_t)BLOCK_SIZE) {
//    // Error or EOF.
//    if(feof(stdio->file)) {
//      upb_seterr(status, UPB_EOF, "");
//    } else if(ferror(stdio->file)) {
//      upb_status_fromerrno(s);
//      return 0;
//    }
//  }
//  b->len = read;
//  stdio->next_ofs += read;
//  return stdio->next_ofs;
//}

size_t upb_stdio_fetch(void *src, uint64_t ofs, upb_status *s) {
  (void)src;
  (void)ofs;
  (void)s;

  return 0;
}

void upb_stdio_read(void *src, uint64_t src_ofs, size_t len, char *dst) {
  upb_stdio_buf *buf = upb_stdio_findbuf(src, src_ofs);
  src_ofs -= buf->ofs;
  memcpy(dst, &buf->data[src_ofs], BUF_SIZE - src_ofs);
  len -= (BUF_SIZE - src_ofs);
  dst += (BUF_SIZE - src_ofs);
  while (len > 0) {
    ++buf;
    size_t bytes = UPB_MIN(len, BUF_SIZE);
    memcpy(dst, buf->data, bytes);
    len -= bytes;
    dst += bytes;
  }
}

const char *upb_stdio_getptr(void *src, uint64_t ofs, size_t *len) {
  upb_stdio_buf *buf = upb_stdio_findbuf(src, ofs);
  ofs -= buf->ofs;
  *len = BUF_SIZE - ofs;
  return &buf->data[ofs];
}

void upb_stdio_refregion(void *src, uint64_t ofs, size_t len) {
  upb_stdio_buf *buf = upb_stdio_findbuf(src, ofs);
  len -= (BUF_SIZE - ofs);
  ++buf->refcount;
  while (len > 0) {
    ++buf;
    ++buf->refcount;
  }
}

void upb_stdio_unrefregion(void *src, uint64_t ofs, size_t len) {
  (void)src;
  (void)ofs;
  (void)len;
}

#if 0
upb_strlen_t upb_stdio_putstr(upb_bytesink *sink, upb_string *str, upb_status *status) {
  upb_stdio *stdio = (upb_stdio*)((char*)sink - offsetof(upb_stdio, sink));
  upb_strlen_t len = upb_string_len(str);
  upb_strlen_t written = fwrite(upb_string_getrobuf(str), 1, len, stdio->file);
  if(written < len) {
    upb_status_setf(status, UPB_ERROR, "Error writing to stdio stream.");
    return -1;
  }
  return written;
}
#endif

uint32_t upb_stdio_vprintf(upb_bytesink *sink, upb_status *status,
                           const char *fmt, va_list args) {
  upb_stdio *stdio = (upb_stdio*)((char*)sink - offsetof(upb_stdio, sink));
  int written = vfprintf(stdio->file, fmt, args);
  if (written < 0) {
    upb_status_setf(status, UPB_ERROR, "Error writing to stdio stream.");
    return -1;
  }
  return written;
}

void upb_stdio_init(upb_stdio *stdio) {
  static upb_bytesrc_vtbl bytesrc_vtbl = {
    upb_stdio_fetch,
    upb_stdio_read,
    upb_stdio_getptr,
    upb_stdio_refregion,
    upb_stdio_unrefregion,
    NULL,
    NULL
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
