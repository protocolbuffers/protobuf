/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_strstream.h"

#include <stdlib.h>
#include "upb_string.h"

static upb_strlen_t upb_stringsrc_read(upb_bytesrc *_src, void *buf,
                                       upb_strlen_t count, upb_status *status) {
  upb_stringsrc *src = (upb_stringsrc*)_src;
  if (src->offset == upb_string_len(src->str)) {
    status->code = UPB_EOF;
    return -1;
  } else {
    upb_strlen_t to_read = UPB_MIN(count, upb_string_len(src->str) - src->offset);
    memcpy(buf, upb_string_getrobuf(src->str) + src->offset, to_read);
    src->offset += to_read;
    return to_read;
  }
}

static bool upb_stringsrc_getstr(upb_bytesrc *_src, upb_string *str,
                                 upb_status *status) {
  upb_stringsrc *src = (upb_stringsrc*)_src;
  if (src->offset == upb_string_len(src->str)) {
    status->code = UPB_EOF;
    return false;
  } else {
    upb_strlen_t len = upb_string_len(src->str) - src->offset;
    upb_string_substr(str, src->str, src->offset, len);
    src->offset += len;
    assert(src->offset == upb_string_len(src->str));
    return true;
  }
}

void upb_stringsrc_init(upb_stringsrc *s) {
  static upb_bytesrc_vtbl bytesrc_vtbl = {
    upb_stringsrc_read,
    upb_stringsrc_getstr,
  };
  s->str = NULL;
  upb_bytesrc_init(&s->bytesrc, &bytesrc_vtbl);
}

void upb_stringsrc_reset(upb_stringsrc *s, upb_string *str) {
  if (str != s->str) {
    upb_string_unref(s->str);
    s->str = upb_string_getref(str);
  }
  s->offset = 0;
}

void upb_stringsrc_uninit(upb_stringsrc *s) {
  upb_string_unref(s->str);
}


upb_bytesrc *upb_stringsrc_bytesrc(upb_stringsrc *s) {
  return &s->bytesrc;
}
