/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/bytestream.h"
#include "upb/descriptor.h"
#include "upb/msg.h"
#include "upb/pb/decoder.h"
#include "upb/pb/glue.h"
#include "upb/pb/textprinter.h"

void upb_strtomsg(const char *str, size_t len, void *msg, upb_msgdef *md,
                  upb_status *status) {
  upb_stringsrc strsrc;
  upb_stringsrc_init(&strsrc);
  upb_stringsrc_reset(&strsrc, str, len);

  upb_decoder d;
  upb_decoder_initformsgdef(&d, md);
  upb_decoder_reset(&d, upb_stringsrc_bytesrc(&strsrc), 0, UINT64_MAX, msg);
  upb_decoder_decode(&d, status);

  upb_stringsrc_uninit(&strsrc);
  upb_decoder_uninit(&d);
}

#if 0
void upb_msgtotext(upb_string *str, upb_msg *msg, upb_msgdef *md,
                   bool single_line) {
  upb_stringsink strsink;
  upb_stringsink_init(&strsink);
  upb_stringsink_reset(&strsink, str);

  upb_textprinter *p = upb_textprinter_new();
  upb_handlers *h = upb_handlers_new();
  upb_textprinter_reghandlers(h, md);
  upb_textprinter_reset(p, upb_stringsink_bytesink(&strsink), single_line);

  upb_status status = UPB_STATUS_INIT;
  upb_msg_runhandlers(msg, md, h, p, &status);
  // None of {upb_msg_runhandlers, upb_textprinter, upb_stringsink} should be
  // capable of returning an error.
  assert(upb_ok(&status));
  upb_status_uninit(&status);

  upb_stringsink_uninit(&strsink);
  upb_textprinter_free(p);
  upb_handlers_unref(h);
}
#endif

// TODO: read->load.
upb_def **upb_load_descriptor(const char *str, size_t len, int *n,
                              upb_status *status) {
  upb_stringsrc strsrc;
  upb_stringsrc_init(&strsrc);
  upb_stringsrc_reset(&strsrc, str, len);

  upb_handlers *h = upb_handlers_new();
  upb_descreader_reghandlers(h);

  upb_decoder d;
  upb_decoder_initforhandlers(&d, h);
  upb_handlers_unref(h);
  upb_descreader r;
  upb_descreader_init(&r);
  upb_decoder_reset(&d, upb_stringsrc_bytesrc(&strsrc), 0, UINT64_MAX, &r);

  upb_decoder_decode(&d, status);
  upb_def **defs = upb_descreader_getdefs(&r, n);
  upb_def **defscopy = malloc(sizeof(upb_def*) * (*n));
  memcpy(defscopy, defs, sizeof(upb_def*) * (*n));

  upb_descreader_uninit(&r);
  upb_stringsrc_uninit(&strsrc);
  upb_decoder_uninit(&d);

  // Set default accessors and layouts on all messages.
  for(int i = 0; i < *n; i++) {
    upb_def *def = defscopy[i];
    upb_msgdef *md = upb_dyncast_msgdef(def);
    if (!md) continue;
    // For field in msgdef:
    upb_msg_iter i;
    for(i = upb_msg_begin(md); !upb_msg_done(i); i = upb_msg_next(md, i)) {
      upb_fielddef *f = upb_msg_iter_field(i);
      upb_fielddef_setaccessor(f, upb_stdmsg_accessor(f));
    }
    upb_msgdef_layout(md);
  }

  return defscopy;
}

void upb_read_descriptor(upb_symtab *s, const char *str, size_t len,
                         upb_status *status) {
  int n;
  upb_def **defs = upb_load_descriptor(str, len, &n, status);
  if (upb_ok(status)) upb_symtab_add(s, defs, n, status);
  for(int i = 0; i < n; i++) upb_def_unref(defs[i]);
  free(defs);
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

void upb_read_descriptorfile(upb_symtab *symtab, const char *fname,
                             upb_status *status) {
  size_t len;
  char *data = upb_readfile(fname, &len);
  if (!data) {
    upb_status_setf(status, UPB_ERROR, "Couldn't read file: %s", fname);
    return;
  }
  upb_read_descriptor(symtab, data, len, status);
  free(data);
}
