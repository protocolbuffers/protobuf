/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "descriptor_const.h"
#include "upb.h"
#include "upb_bytestream.h"

#define alignof(t) offsetof(struct { char c; t x; }, x)
#define TYPE_INFO(wire_type, ctype, inmemory_type) \
    {alignof(ctype), sizeof(ctype), wire_type, UPB_TYPE(inmemory_type), #ctype},

const upb_type_info upb_types[] = {
  TYPE_INFO(UPB_WIRE_TYPE_END_GROUP,   void*,     MESSAGE)   // ENDGROUP (fake)
  TYPE_INFO(UPB_WIRE_TYPE_64BIT,       double,    DOUBLE)    // DOUBLE
  TYPE_INFO(UPB_WIRE_TYPE_32BIT,       float,     FLOAT)     // FLOAT
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int64_t,   INT64)     // INT64
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      uint64_t,  UINT64)    // UINT64
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int32_t,   INT32)     // INT32
  TYPE_INFO(UPB_WIRE_TYPE_64BIT,       uint64_t,  UINT64)    // FIXED64
  TYPE_INFO(UPB_WIRE_TYPE_32BIT,       uint32_t,  UINT32)    // FIXED32
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      bool,      BOOL)      // BOOL
  TYPE_INFO(UPB_WIRE_TYPE_DELIMITED,   void*,     STRING)    // STRING
  TYPE_INFO(UPB_WIRE_TYPE_START_GROUP, void*,     MESSAGE)   // GROUP
  TYPE_INFO(UPB_WIRE_TYPE_DELIMITED,   void*,     MESSAGE)   // MESSAGE
  TYPE_INFO(UPB_WIRE_TYPE_DELIMITED,   void*,     STRING)    // BYTES
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      uint32_t,  UINT32)    // UINT32
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      uint32_t,  INT32)     // ENUM
  TYPE_INFO(UPB_WIRE_TYPE_32BIT,       int32_t,   INT32)     // SFIXED32
  TYPE_INFO(UPB_WIRE_TYPE_64BIT,       int64_t,   INT64)     // SFIXED64
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int32_t,   INT32)     // SINT32
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int64_t,   INT64)     // SINT64
  TYPE_INFO(UPB_WIRE_TYPE_END_GROUP,   void*,     INT64)     // SINT64
};

#ifdef NDEBUG
upb_value UPB_NO_VALUE = {{0}};
#else
upb_value UPB_NO_VALUE = {{0}, -1};
#endif

void upb_status_init(upb_status *status) {
  status->buf = NULL;
  upb_status_clear(status);
}

void upb_status_uninit(upb_status *status) {
  free(status->buf);
}

void upb_status_setf(upb_status *s, enum upb_status_code code,
                     const char *msg, ...) {
  s->code = code;
  va_list args;
  va_start(args, msg);
  upb_vrprintf(&s->buf, &s->bufsize, 0, msg, args);
  va_end(args);
  s->str = s->buf;
}

void upb_status_copy(upb_status *to, upb_status *from) {
  to->code = from->code;
  if (from->str) {
    if (to->bufsize < from->bufsize) {
      to->bufsize = from->bufsize;
      to->buf = realloc(to->buf, to->bufsize);
      to->str = to->buf;
    }
    memcpy(to->str, from->str, from->bufsize);
  } else {
    to->str = NULL;
  }
}

void upb_status_clear(upb_status *status) {
  status->code = UPB_OK;
  status->str = NULL;
}

void upb_status_print(upb_status *status, FILE *f) {
  if(status->str) {
    fprintf(f, "code: %d, msg: %s\n", status->code, status->str);
  } else {
    fprintf(f, "code: %d, no msg\n", status->code);
  }
}

void upb_status_fromerrno(upb_status *status) {
  upb_status_setf(status, UPB_ERROR, "%s", strerror(errno));
}

int upb_vrprintf(char **buf, size_t *size, size_t ofs,
                 const char *fmt, va_list args) {
  // Try once without reallocating.  We have to va_copy because we might have
  // to call vsnprintf again.
  uint32_t len = *size - ofs;
  va_list args_copy;
  va_copy(args_copy, args);
  uint32_t true_len = vsnprintf(*buf + ofs, len, fmt, args_copy);
  va_end(args_copy);

  // Resize to be the correct size.
  if (true_len >= len) {
    // Need to print again, because some characters were truncated.  vsnprintf
    // will not write the entire string unless you give it space to store the
    // NULL terminator also.
    while (*size < (ofs + true_len + 1)) *size = UPB_MAX(*size * 2, 2);
    char *newbuf = realloc(*buf, *size);
    if (!newbuf) return -1;
    vsnprintf(newbuf + ofs, true_len + 1, fmt, args);
    *buf = newbuf;
  }
  return true_len;
}
