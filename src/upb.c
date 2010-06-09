/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 */

#include <stdarg.h>
#include <stddef.h>

#include "upb.h"

void upb_seterr(upb_status *status, enum upb_status_code code,
                const char *msg, ...)
{
  if(upb_ok(status)) {  // The first error is the most interesting.
    status->code = code;
    va_list args;
    va_start(args, msg);
    vsnprintf(status->msg, UPB_ERRORMSG_MAXLEN, msg, args);
    va_end(args);
  }
}
