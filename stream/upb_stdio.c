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

static bool upb_stdio_read(upb_stdio *stdio, upb_string *str,
                           int offset, size_t bytes_to_read) {
  char *buf = upb_string_getrwbuf(str, offset + bytes_to_read) + offset;
  size_t read = fread(buf, 1, bytes_to_read, stdio->file);
  if(read < bytes_to_read) {
    // Error or EOF.
    stdio->bytesrc.eof = feof(stdio->file);
    if(ferror(stdio->file)) {
      upb_seterr(&stdio->bytesrc.status, UPB_STATUS_ERROR,
                 "Error reading from stdio stream.");
      return false;
    }
    // Resize to actual read size.
    upb_string_getrwbuf(str, offset + read);
  }
  return true;
}

bool upb_stdio_get(upb_bytesrc *src, upb_string *str, upb_strlen_t minlen) {
  // We ignore "minlen" since the stdio interfaces always return a full read
  // unless they are at EOF.
  (void)minlen;
  return upb_stdio_read((upb_stdio*)src, str, 0, BLOCK_SIZE);
}

bool upb_stdio_append(upb_bytesrc *src, upb_string *str, upb_strlen_t len) {
  return upb_stdio_read((upb_stdio*)src, str, upb_string_len(str), len);
}

int32_t upb_stdio_put(upb_bytesink *sink, upb_string *str) {
  upb_stdio *stdio = (upb_stdio*)((char*)sink - offsetof(upb_stdio, bytesink));
  upb_strlen_t len = upb_string_len(str);
  size_t written = fwrite(upb_string_getrobuf(str), 1, len, stdio->file);
  if(written < len) {
    // Error or EOF.
    stdio->bytesink.eof = feof(stdio->file);
    if(ferror(stdio->file)) {
      upb_seterr(&stdio->bytesink.status, UPB_STATUS_ERROR,
                 "Error writing to stdio stream.");
      return 0;
    }
  }
  return written;
}

static upb_bytesrc_vtable upb_stdio_bytesrc_vtbl = {
  (upb_bytesrc_get_fptr)upb_stdio_get,
  (upb_bytesrc_append_fptr)upb_stdio_append,
};

static upb_bytesink_vtable upb_stdio_bytesink_vtbl = {
  upb_stdio_put
};

upb_stdio *upb_stdio_new() {
  upb_stdio *stdio = malloc(sizeof(*stdio));
  upb_bytesrc_init(&stdio->bytesrc, &upb_stdio_bytesrc_vtbl);
  upb_bytesink_init(&stdio->bytesink, &upb_stdio_bytesink_vtbl);
  return stdio;
}

void upb_stdio_free(upb_stdio *stdio) {
  free(stdio);
}

upb_bytesrc* upb_stdio_bytesrc(upb_stdio *stdio) { return &stdio->bytesrc; }
upb_bytesink* upb_stdio_bytesink(upb_stdio *stdio) { return &stdio->bytesink; }
