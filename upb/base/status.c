// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/base/status.h"

#include <errno.h>
#include <float.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// Must be last.
#include "upb/port/def.inc"

void upb_Status_Clear(upb_Status* status) {
  if (!status) return;
  status->ok = true;
  status->msg[0] = '\0';
}

bool upb_Status_IsOk(const upb_Status* status) { return status->ok; }

const char* upb_Status_ErrorMessage(const upb_Status* status) {
  return status->msg;
}

void upb_Status_SetErrorMessage(upb_Status* status, const char* msg) {
  if (!status) return;
  status->ok = false;
  strncpy(status->msg, msg, _kUpb_Status_MaxMessage - 1);
  status->msg[_kUpb_Status_MaxMessage - 1] = '\0';
}

void upb_Status_SetErrorFormat(upb_Status* status, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  upb_Status_VSetErrorFormat(status, fmt, args);
  va_end(args);
}

void upb_Status_VSetErrorFormat(upb_Status* status, const char* fmt,
                                va_list args) {
  if (!status) return;
  status->ok = false;
  vsnprintf(status->msg, sizeof(status->msg), fmt, args);
  status->msg[_kUpb_Status_MaxMessage - 1] = '\0';
}

void upb_Status_VAppendErrorFormat(upb_Status* status, const char* fmt,
                                   va_list args) {
  size_t len;
  if (!status) return;
  status->ok = false;
  len = strlen(status->msg);
  vsnprintf(status->msg + len, sizeof(status->msg) - len, fmt, args);
  status->msg[_kUpb_Status_MaxMessage - 1] = '\0';
}
