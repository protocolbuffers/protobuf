/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_stdio.h"

// We can make this configurable if necessary.
#define BLOCK_SIZE 4096

struct upb_stdio {
  upb_bytesrc bytesrc;
  upb_bytesink bytesink;
  FILE *file;
}

static bool upb_stdio_read(upb_stdio *stdio, upb_string *str,
                           int offset, int bytes_to_read) {
  char *buf = upb_string_getrwbuf(offset + bytes_to_read) + offset;
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

int32_t upb_bytesink_put(upb_bytesink *sink, upb_string *str) {
  upb_stdio *stdio = (upb_stdio*)sink - offsetof(upb_stdio, bytesink);
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
