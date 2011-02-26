/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_strstream.h"

#include <stdlib.h>
#include "upb_string.h"


/* upb_stringsrc **************************************************************/

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
  static upb_bytesrc_vtbl vtbl = {
    upb_stringsrc_read,
    upb_stringsrc_getstr,
  };
  upb_bytesrc_init(&s->bytesrc, &vtbl);
  s->str = NULL;
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


/* upb_stringsink *************************************************************/

void upb_stringsink_uninit(upb_stringsink *s) {
  upb_string_unref(s->str);
}

// Resets the stringsink to a state where it will append to the given string.
// The string must be newly created or recycled.  The stringsink will take a
// reference on the string, so the caller need not ensure that it outlives the
// stringsink.  A stringsink can be reset multiple times.
void upb_stringsink_reset(upb_stringsink *s, upb_string *str) {
  if (str != s->str) {
    upb_string_unref(s->str);
    s->str = upb_string_getref(str);
  }
  // Resize to 0.
  upb_string_getrwbuf(s->str, 0);
}

upb_bytesink *upb_stringsink_bytesink(upb_stringsink *s) {
  return &s->bytesink;
}

static upb_strlen_t upb_stringsink_vprintf(upb_bytesink *_sink, upb_status *s,
                                           const char *fmt, va_list args) {
  (void)s;  // No errors can occur.
  upb_stringsink *sink = (upb_stringsink*)_sink;
  return upb_string_vprintf_at(sink->str, upb_string_len(sink->str), fmt, args);
}

static upb_strlen_t upb_stringsink_putstr(upb_bytesink *_sink, upb_string *str,
                                          upb_status *s) {
  (void)s;  // No errors can occur.
  upb_stringsink *sink = (upb_stringsink*)_sink;
  upb_strcat(sink->str, str);
  return upb_string_len(str);
}

void upb_stringsink_init(upb_stringsink *s) {
  static upb_bytesink_vtbl vtbl = {
    NULL,
    upb_stringsink_putstr,
    upb_stringsink_vprintf
  };
  upb_bytesink_init(&s->bytesink, &vtbl);
  s->str = NULL;
}
