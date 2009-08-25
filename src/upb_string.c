/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stdio.h>
#include "upb_string.h"

struct upb_string *upb_strreadfile(const char *filename) {
  FILE *f = fopen(filename, "rb");
  if(!f) return false;
  if(fseek(f, 0, SEEK_END) != 0) goto error;
  long size = ftell(f);
  if(size < 0) goto error;
  if(fseek(f, 0, SEEK_SET) != 0) goto error;
  struct upb_string *s = upb_string_new();
  upb_string_resize(s, size);
  if(fread(s->ptr, size, 1, f) != 1) goto error;
  fclose(f);
  return s;

error:
  fclose(f);
  return NULL;
}
