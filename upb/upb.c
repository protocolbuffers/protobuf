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
#include "upb/descriptor_const.h"
#include "upb/upb.h"
#include "upb/bytestream.h"

#define alignof(t) offsetof(struct { char c; t x; }, x)
#define TYPE_INFO(wire_type, ctype, inmemory_type, is_numeric) \
    {alignof(ctype), sizeof(ctype), wire_type, UPB_TYPE(inmemory_type), \
     #ctype, is_numeric},

const upb_type_info upb_types[] = {
  // END_GROUP is not real, but used to signify the pseudo-field that
  // ends a group from within the group.
  TYPE_INFO(UPB_WIRE_TYPE_END_GROUP,   void*,     MESSAGE, false)   // ENDGROUP
  TYPE_INFO(UPB_WIRE_TYPE_64BIT,       double,    DOUBLE,  true)    // DOUBLE
  TYPE_INFO(UPB_WIRE_TYPE_32BIT,       float,     FLOAT,   true)    // FLOAT
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int64_t,   INT64,   true)    // INT64
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      uint64_t,  UINT64,  true)    // UINT64
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int32_t,   INT32,   true)    // INT32
  TYPE_INFO(UPB_WIRE_TYPE_64BIT,       uint64_t,  UINT64,  true)    // FIXED64
  TYPE_INFO(UPB_WIRE_TYPE_32BIT,       uint32_t,  UINT32,  true)    // FIXED32
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      bool,      BOOL,    true)    // BOOL
  TYPE_INFO(UPB_WIRE_TYPE_DELIMITED,   void*,     STRING,  false)   // STRING
  TYPE_INFO(UPB_WIRE_TYPE_START_GROUP, void*,     MESSAGE, false)   // GROUP
  TYPE_INFO(UPB_WIRE_TYPE_DELIMITED,   void*,     MESSAGE, false)   // MESSAGE
  TYPE_INFO(UPB_WIRE_TYPE_DELIMITED,   void*,     STRING,  false)   // BYTES
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      uint32_t,  UINT32,  true)    // UINT32
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      uint32_t,  INT32,   true)    // ENUM
  TYPE_INFO(UPB_WIRE_TYPE_32BIT,       int32_t,   INT32,   true)    // SFIXED32
  TYPE_INFO(UPB_WIRE_TYPE_64BIT,       int64_t,   INT64,   true)    // SFIXED64
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int32_t,   INT32,   true)    // SINT32
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int64_t,   INT64,   true)    // SINT64
};

#ifdef NDEBUG
upb_value UPB_NO_VALUE = {{0}};
#else
upb_value UPB_NO_VALUE = {{0}, -1};
#endif

void upb_status_init(upb_status *status) {
  status->buf = NULL;
  status->bufsize = 0;
  upb_status_clear(status);
}

void upb_status_uninit(upb_status *status) {
  free(status->buf);
}

void upb_status_seterrf(upb_status *s, const char *msg, ...) {
  s->code = UPB_ERROR;
  va_list args;
  va_start(args, msg);
  upb_vrprintf(&s->buf, &s->bufsize, 0, msg, args);
  va_end(args);
  s->str = s->buf;
}

void upb_status_seterrliteral(upb_status *status, const char *msg) {
  status->error = true;
  status->str = msg;
  status->space = NULL;
}

void upb_status_copy(upb_status *to, const upb_status *from) {
  to->error = from->error;
  to->eof = from->eof;
  to->code = from->code;
  to->space = from->space;
  if (from->str == from->buf) {
    if (to->bufsize < from->bufsize) {
      to->bufsize = from->bufsize;
      to->buf = realloc(to->buf, to->bufsize);
    }
    memcpy(to->buf, from->buf, from->bufsize);
    to->str = to->buf;
  } else {
    to->str = from->str;
  }
}

const char *upb_status_getstr(const upb_status *_status) {
  // Function is logically const but can modify internal state to materialize
  // the string.
  upb_status *status = (upb_status*)_status;
  if (status->str == NULL && status->space) {
    if (status->space->code_to_string) {
      status->space->code_to_string(status->code, status->buf, status->bufsize);
      status->str = status->buf;
    } else {
      upb_status_seterrf(status, "No message, error space=%s, code=%d\n",
                         status->space->name, status->code);
    }
  }
  return status->str;
}

void upb_status_clear(upb_status *status) {
  status->error = false;
  status->eof = false;
  status->code = 0;
  status->space = NULL;
  status->str = NULL;
}

void upb_status_setcode(upb_status *status, upb_errorspace *space, int code) {
  status->code = code;
  status->space = space;
  status->str = NULL;
}

void upb_status_fromerrno(upb_status *status) {
  if (errno != 0 && !upb_errno_is_wouldblock()) {
    status->error = true;
    upb_status_setcode(status, &upb_posix_errorspace, errno);
  }
}

bool upb_errno_is_wouldblock() {
  return
#ifdef EAGAIN
      errno == EAGAIN ||
#endif
#ifdef EWOULDBLOCK
      errno == EWOULDBLOCK ||
#endif
      false;
}

bool upb_posix_codetostr(int code, char *buf, size_t len) {
  if (strerror_r(code, buf, len) == -1) {
    if (errno == EINVAL) {
      return snprintf(buf, len, "Invalid POSIX error number %d\n", code) >= len;
    } else if (errno == ERANGE) {
      return false;
    }
    assert(false);
  }
  return true;
}

upb_errorspace upb_posix_errorspace = {"POSIX", &upb_posix_codetostr};

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
    *size = (ofs + true_len + 1);
    char *newbuf = realloc(*buf, *size);
    if (!newbuf) return -1;
    vsnprintf(newbuf + ofs, true_len + 1, fmt, args);
    *buf = newbuf;
  }
  return true_len;
}
