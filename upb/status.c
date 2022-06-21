/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upb/status.h"

#include <errno.h>
#include <float.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "upb/internal/upb.h"

// Must be last.
#include "upb/port_def.inc"

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
