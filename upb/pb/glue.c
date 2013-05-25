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
  // Create handlers.
  const upb_handlers *reader_h = upb_descreader_gethandlers(&reader_h);
  const upb_handlers *decoder_h =
      upb_pbdecoder_gethandlers(reader_h, false, &decoder_h);

  // Create pipeline.
  upb_pipeline pipeline;
  upb_pipeline_init(&pipeline, NULL, 0, upb_realloc, NULL);
  upb_pipeline_donateref(&pipeline, reader_h, &reader_h);
  upb_pipeline_donateref(&pipeline, decoder_h, &decoder_h);

  // Create sinks.
  upb_sink *reader_sink = upb_pipeline_newsink(&pipeline, reader_h);
  upb_sink *decoder_sink = upb_pipeline_newsink(&pipeline, decoder_h);
  upb_pbdecoder *d = upb_sink_getobj(decoder_sink);
  upb_pbdecoder_resetsink(d, reader_sink);

  // Push input data.
  bool ok = upb_bytestream_putstr(decoder_sink, str, len);

  if (status) upb_status_copy(status, upb_pipeline_status(&pipeline));
  if (!ok) {
    upb_pipeline_uninit(&pipeline);
    return NULL;
  }

  upb_descreader *r = upb_sink_getobj(reader_sink);
  upb_def **defs = upb_descreader_getdefs(r, owner, n);
  upb_def **defscopy = malloc(sizeof(upb_def*) * (*n));
  memcpy(defscopy, defs, sizeof(upb_def*) * (*n));
  upb_pipeline_uninit(&pipeline);

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
