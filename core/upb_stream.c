/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_stream.h"

#include "upb_def.h"

#define CHECKSRC(x) if(!x) goto src_err
#define CHECKSINK(x) if(!x) goto sink_err

void upb_streamdata(upb_src *src, upb_sink *sink, upb_status *status) {
  upb_fielddef *f;
  upb_string *str = NULL;
  int depth = 0;
  while(1) {
    while((f = upb_src_getdef(src)) != NULL) {
      CHECKSINK(upb_sink_putdef(sink, f));
      if(upb_issubmsg(f)) {
        upb_src_startmsg(src);
        upb_sink_startmsg(sink);
        ++depth;
      } else if(upb_isstring(f)) {
        str = upb_string_tryrecycle(str);
        CHECKSRC(upb_src_getstr(src, str));
        CHECKSINK(upb_sink_putstr(sink, str));
      } else {
        // Primitive type.
        upb_value val;
        CHECKSRC(upb_src_getval(src, upb_value_addrof(&val)));
        CHECKSINK(upb_sink_putval(sink, val));
      }
    }
    // If we're not EOF now, the loop terminated due to an error.
    CHECKSRC(upb_src_eof(src));
    if (depth == 0) break;
    --depth;
  }
  return;

src_err:
  upb_copyerr(status, upb_src_status(src));
  return;

sink_err:
  upb_copyerr(status, upb_sink_status(sink));
  return;
}
