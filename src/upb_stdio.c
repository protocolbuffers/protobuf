/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb_stdio.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// We can make this configurable if necessary.
#define BUF_SIZE 32768


/* upb_bytesrc methods ********************************************************/

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


/* upb_bytesink methods *******************************************************/

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
