/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "upb/upb.h"

bool upb_dumptostderr(void *closure, const upb_status* status) {
  UPB_UNUSED(closure);
  fprintf(stderr, "%s\n", upb_status_errmsg(status));
  return false;
}

// Guarantee null-termination and provide ellipsis truncation.
// It may be tempting to "optimize" this by initializing these final
// four bytes up-front and then being careful never to overwrite them,
// this is safer and simpler.
static void nullz(upb_status *status) {
  const char *ellipsis = "...";
  size_t len = strlen(ellipsis);
  assert(sizeof(status->msg) > len);
  memcpy(status->msg + sizeof(status->msg) - len, ellipsis, len);
}

void upb_status_clear(upb_status *status) {
  upb_status blank = UPB_STATUS_INIT;
  upb_status_copy(status, &blank);
}

bool upb_ok(const upb_status *status) { return status->ok_; }

upb_errorspace *upb_status_errspace(const upb_status *status) {
  return status->error_space_;
}

int upb_status_errcode(const upb_status *status) { return status->code_; }

const char *upb_status_errmsg(const upb_status *status) { return status->msg; }

void upb_status_seterrmsg(upb_status *status, const char *msg) {
  if (!status) return;
  status->ok_ = false;
  strncpy(status->msg, msg, sizeof(status->msg));
  nullz(status);
}

void upb_status_seterrf(upb_status *status, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  upb_status_vseterrf(status, fmt, args);
  va_end(args);
}

void upb_status_vseterrf(upb_status *status, const char *fmt, va_list args) {
  if (!status) return;
  status->ok_ = false;
  vsnprintf(status->msg, sizeof(status->msg), fmt, args);
  nullz(status);
}

void upb_status_seterrcode(upb_status *status, upb_errorspace *space,
                           int code) {
  if (!status) return;
  status->ok_ = false;
  status->error_space_ = space;
  status->code_ = code;
  space->set_message(status, code);
}

void upb_status_copy(upb_status *to, const upb_status *from) {
  if (!to) return;
  *to = *from;
}
