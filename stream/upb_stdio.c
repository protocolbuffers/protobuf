/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_stdio.h"

#include <stddef.h>
#include <stdlib.h>
#include "upb_string.h"

// We can make this configurable if necessary.
#define BLOCK_SIZE 4096

struct upb_stdio {
  upb_bytesrc bytesrc;
  upb_bytesink bytesink;
  FILE *file;
};

void upb_stdio_reset(upb_stdio *stdio, FILE* file) {
  stdio->file = file;
}

static upb_strlen_t upb_stdio_read(upb_bytesrc *src, void *buf,
                                   upb_strlen_t count, upb_status *status) {
  upb_stdio *stdio = (upb_stdio*)src;
  assert(count > 0);
  size_t read = fread(buf, 1, count, stdio->file);
  if(read < (size_t)count) {
    // Error or EOF.
    if(feof(stdio->file)) {
      upb_seterr(status, UPB_EOF, "");
      return read;
    } else if(ferror(stdio->file)) {
      upb_seterr(status, UPB_ERROR, "Error reading from stdio stream.");
      return -1;
    }
  }
  return read;
}

static bool upb_stdio_getstr(upb_bytesrc *src, upb_string *str,
                             upb_status *status) {
  upb_strlen_t read = upb_stdio_read(
      src, upb_string_getrwbuf(str, BLOCK_SIZE), BLOCK_SIZE, status);
  if (read <= 0) return false;
  upb_string_getrwbuf(str, read);
  return true;
}

int32_t upb_stdio_put(upb_bytesink *sink, upb_string *str) {
  upb_stdio *stdio = (upb_stdio*)((char*)sink - offsetof(upb_stdio, bytesink));
  upb_strlen_t len = upb_string_len(str);
  upb_strlen_t written = fwrite(upb_string_getrobuf(str), 1, len, stdio->file);
  if(written < len) {
    // Error or EOF.
    stdio->bytesink.eof = feof(stdio->file);
    if(ferror(stdio->file)) {
      upb_seterr(&stdio->bytesink.status, UPB_ERROR,
                 "Error writing to stdio stream.");
      return 0;
    }
  }
  return written;
}

upb_stdio *upb_stdio_new() {
  static upb_bytesrc_vtbl bytesrc_vtbl = {
    upb_stdio_read,
    upb_stdio_getstr,
  };

  //static upb_bytesink_vtbl bytesink_vtbl = {
  //  upb_stdio_put
  //};

  upb_stdio *stdio = malloc(sizeof(*stdio));
  upb_bytesrc_init(&stdio->bytesrc, &bytesrc_vtbl);
  //upb_bytesink_init(&stdio->bytesink, &bytesink_vtbl);
  return stdio;
}

void upb_stdio_free(upb_stdio *stdio) {
  free(stdio);
}

upb_bytesrc* upb_stdio_bytesrc(upb_stdio *stdio) { return &stdio->bytesrc; }
upb_bytesink* upb_stdio_bytesink(upb_stdio *stdio) { return &stdio->bytesink; }
