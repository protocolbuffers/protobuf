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
#include "upb/descriptor/reader.h"
#include "upb/pb/decoder.h"

upb_def **upb_load_defs_from_descriptor(const char *str, size_t len, int *n,
                                        void *owner, upb_status *status) {
  // Create handlers.
  const upb_handlers *reader_h = upb_descreader_newhandlers(&reader_h);
  upb_pbdecodermethodopts opts;
  upb_pbdecodermethodopts_init(&opts, reader_h);
  const upb_pbdecodermethod *decoder_m =
      upb_pbdecodermethod_new(&opts, &decoder_m);

  upb_env env;
  upb_env_init(&env);
  upb_env_reporterrorsto(&env, status);

  upb_descreader *reader = upb_descreader_create(&env, reader_h);
  upb_pbdecoder *decoder =
      upb_pbdecoder_create(&env, decoder_m, upb_descreader_input(reader));

  // Push input data.
  bool ok = upb_bufsrc_putbuf(str, len, upb_pbdecoder_input(decoder));

  upb_def **ret = NULL;

  if (!ok) goto cleanup;
  upb_def **defs = upb_descreader_getdefs(reader, owner, n);
  ret = malloc(sizeof(upb_def*) * (*n));
  memcpy(ret, defs, sizeof(upb_def*) * (*n));

cleanup:
  upb_env_uninit(&env);
  upb_handlers_unref(reader_h, &reader_h);
  upb_pbdecodermethod_unref(decoder_m, &decoder_m);
  return ret;
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
  char *buf = malloc(size + 1);
  if(size && fread(buf, size, 1, f) != 1) goto error;
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
