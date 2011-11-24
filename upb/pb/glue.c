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

void upb_strtomsg(const char *str, size_t len, void *msg, const upb_msgdef *md,
                  upb_status *status) {
  upb_stringsrc strsrc;
  upb_stringsrc_init(&strsrc);
  upb_stringsrc_reset(&strsrc, str, len);

  upb_decoder d;
  upb_handlers *h = upb_handlers_new();
  upb_accessors_reghandlers(h, md);
  upb_decoder_init(&d, h);
  upb_handlers_unref(h);
  upb_decoder_reset(&d, upb_stringsrc_allbytes(&strsrc), msg);
  upb_decoder_decode(&d, status);

  upb_stringsrc_uninit(&strsrc);
  upb_decoder_uninit(&d);
}

void *upb_filetonewmsg(const char *fname, const upb_msgdef *md, upb_status *s) {
  void *msg = upb_stdmsg_new(md);
  size_t len;
  char *data = upb_readfile(fname, &len);
  if (!data) goto err;
  upb_strtomsg(data, len, msg, md, s);
  if (!upb_ok(s)) goto err;
  return msg;

err:
  upb_stdmsg_free(msg, md);
  return NULL;
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
upb_def **upb_load_defs_from_descriptor(const char *str, size_t len, int *n,
                                        upb_status *status) {
  upb_stringsrc strsrc;
  upb_stringsrc_init(&strsrc);
  upb_stringsrc_reset(&strsrc, str, len);

  upb_handlers *h = upb_handlers_new();
  upb_descreader_reghandlers(h);

  upb_decoder d;
  upb_decoder_init(&d, h);
  upb_handlers_unref(h);
  upb_descreader r;
  upb_descreader_init(&r);
  upb_decoder_reset(&d, upb_stringsrc_allbytes(&strsrc), &r);

  upb_decoder_decode(&d, status);
  upb_stringsrc_uninit(&strsrc);
  upb_decoder_uninit(&d);
  if (!upb_ok(status)) {
    upb_descreader_uninit(&r);
    return NULL;
  }
  upb_def **defs = upb_descreader_getdefs(&r, n);
  upb_def **defscopy = malloc(sizeof(upb_def*) * (*n));
  memcpy(defscopy, defs, sizeof(upb_def*) * (*n));
  upb_descreader_uninit(&r);

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

bool upb_load_descriptor_into_symtab(upb_symtab *s, const char *str, size_t len,
                                     upb_status *status) {
  int n;
  upb_def **defs = upb_load_defs_from_descriptor(str, len, &n, status);
  if (!defs) return false;
  bool success = upb_symtab_add(s, defs, n, status);
  for(int i = 0; i < n; i++) upb_def_unref(defs[i]);
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
