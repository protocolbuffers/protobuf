/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/pb/glue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "upb/bytestream.h"
#include "upb/descriptor/reader.h"
#include "upb/pb/decoder.h"

upb_def **upb_load_defs_from_descriptor(const char *str, size_t len, int *n,
                                        void *owner, upb_status *status) {
  upb_stringsrc strsrc;
  upb_stringsrc_init(&strsrc);
  upb_stringsrc_reset(&strsrc, str, len);

  const upb_handlers *h = upb_descreader_newhandlers(&h);
  upb_decoderplan *p = upb_decoderplan_new(h, false);
  upb_decoder d;
  upb_decoder_init(&d);
  upb_handlers_unref(h, &h);
  upb_descreader r;
  upb_descreader_init(&r);
  upb_decoder_resetplan(&d, p);
  upb_decoder_resetinput(&d, upb_stringsrc_allbytes(&strsrc), &r);

  upb_success_t ret = upb_decoder_decode(&d);
  if (status) upb_status_copy(status, upb_decoder_status(&d));
  upb_stringsrc_uninit(&strsrc);
  upb_decoder_uninit(&d);
  upb_decoderplan_unref(p);
  if (ret != UPB_OK) {
    upb_descreader_uninit(&r);
    return NULL;
  }
  upb_def **defs = upb_descreader_getdefs(&r, owner, n);
  upb_def **defscopy = malloc(sizeof(upb_def*) * (*n));
  memcpy(defscopy, defs, sizeof(upb_def*) * (*n));
  upb_descreader_uninit(&r);

  return defscopy;
}

bool upb_load_descriptor_into_symtab(upb_symtab *s, const char *str, size_t len,
                                     upb_status *status) {
  int n;
  upb_def **defs = upb_load_defs_from_descriptor(str, len, &n, &defs, status);
  if (!defs) return false;
  bool success = upb_symtab_add(s, defs, n, &defs, status);
  free(defs);
  return success;
}

char *upb_readfile(const char *filename, size_t *len) {
  FILE *f = fopen(filename, "rb");
  if(!f) return NULL;
  if(fseek(f, 0, SEEK_END) != 0) goto error;
  long size = ftell(f);
  if(size < 0) goto error;
  if(fseek(f, 0, SEEK_SET) != 0) goto error;
  char *buf = malloc(size);
  if(fread(buf, size, 1, f) != 1) goto error;
  fclose(f);
  if (len) *len = size;
  return buf;

error:
  fclose(f);
  return NULL;
}

bool upb_load_descriptor_file_into_symtab(upb_symtab *symtab, const char *fname,
                                          upb_status *status) {
  size_t len;
  char *data = upb_readfile(fname, &len);
  if (!data) {
    if (status) upb_status_seterrf(status, "Couldn't read file: %s", fname);
    return false;
  }
  bool success = upb_load_descriptor_into_symtab(symtab, data, len, status);
  free(data);
  return success;
}
