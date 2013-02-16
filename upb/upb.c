/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "upb/upb.h"

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

bool upb_ok(const upb_status *status) { return !status->error; }
bool upb_eof(const upb_status *status) { return status->eof_; }

void upb_status_seterrf(upb_status *status, const char *msg, ...) {
  if (!status) return;
  status->error = true;
  status->space = NULL;
  va_list args;
  va_start(args, msg);
  upb_vrprintf(&status->buf, &status->bufsize, 0, msg, args);
  va_end(args);
  status->str = status->buf;
}

void upb_status_seterrliteral(upb_status *status, const char *msg) {
  if (!status) return;
  status->error = true;
  status->str = msg;
  status->space = NULL;
}

void upb_status_copy(upb_status *to, const upb_status *from) {
  if (!to) return;
  to->error = from->error;
  to->eof_ = from->eof_;
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
  if (!status) return;
  status->error = false;
  status->eof_ = false;
  status->code = 0;
  status->space = NULL;
  status->str = NULL;
}

void upb_status_setcode(upb_status *status, upb_errorspace *space, int code) {
  if (!status) return;
  status->code = code;
  status->space = space;
  status->str = NULL;
}

void upb_status_seteof(upb_status *status) {
  if (!status) return;
  status->eof_ = true;
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
    *size = (ofs + true_len + 1);
    char *newbuf = realloc(*buf, *size);
    if (!newbuf) return -1;
    vsnprintf(newbuf + ofs, true_len + 1, fmt, args);
    *buf = newbuf;
  }
  return true_len;
}
