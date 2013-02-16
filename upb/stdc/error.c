/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Handling of errno.
 */

#include "upb/stdc/error.h"

#include <string.h>

void upb_status_fromerrno(upb_status *status, int code) {
  if (code != 0 && !upb_errno_is_wouldblock(code)) {
    status->error = true;
    upb_status_setcode(status, &upb_stdc_errorspace, code);
  }
}

bool upb_errno_is_wouldblock(int code) {
  return
#ifdef EAGAIN
      code == EAGAIN ||
#endif
#ifdef EWOULDBLOCK
      code == EWOULDBLOCK ||
#endif
      false;
}

bool upb_stdc_codetostr(int code, char *buf, size_t len) {
  // strerror() may use static buffers and is not guaranteed to be thread-safe,
  // but it appears that it is not subject to buffer overflows in practice, and
  // it used by other portable and high-quality software like Lua.  For more
  // discussion see: http://thread.gmane.org/gmane.comp.lang.lua.general/89506
  char *err = strerror(code);
  if (strlen(err) >= len) return false;
  strcpy(buf, err);
  return true;
}

upb_errorspace upb_stdc_errorspace = {"stdc", &upb_stdc_codetostr};
