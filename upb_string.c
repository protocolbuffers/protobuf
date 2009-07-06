/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_string.h"

bool upb_strreadfile(const char *filename, struct upb_string *data) {
  FILE *f = fopen(filename, "rb");
  if(!f) return false;
  if(fseek(f, 0, SEEK_END) != 0) return false;
  long size = ftell(f);
  if(size < 0) return false;
  if(fseek(f, 0, SEEK_SET) != 0) return false;
  data->ptr = (char*)malloc(size);
  data->byte_len = size;
  if(fread(data->ptr, size, 1, f) != 1) {
    free(data->ptr);
    return false;
  }
  fclose(f);
  return true;
}
