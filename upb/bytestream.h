/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2013 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * This file contains the standard ByteStream msgdef and some useful routines
 * surrounding it.
 *
 * This is a mixed C/C++ interface that offers a full API to both languages.
 * See the top-level README for more information.
 */

#ifndef UPB_BYTESTREAM_H_
#define UPB_BYTESTREAM_H_

#include "upb/sink.h"
#include "upb/bytestream.upb.h"

#define UPB_BYTESTREAM_BYTES &upb_bytestream_fields[0]

// A convenience method that handles the start/end calls and tracks overall
// success.
UPB_INLINE bool upb_bytestream_putstr(upb_sink *s, const char *buf, size_t n) {
  bool ret =
      upb_sink_startmsg(s) &&
      upb_sink_startstr(s, UPB_BYTESTREAM_BYTES_STARTSTR, n) &&
      upb_sink_putstring(s, UPB_BYTESTREAM_BYTES_STRING, buf, n) == n &&
      upb_sink_endstr(s, UPB_BYTESTREAM_BYTES_ENDSTR);
  if (ret) upb_sink_endmsg(s);
  return ret;
}

#ifdef __cplusplus

namespace upb {

inline bool PutStringToBytestream(Sink* s, const char* buf, size_t n) {
  return upb_bytestream_putstr(s, buf, n);
}

template <class T> bool PutStringToBytestream(Sink* s, T str) {
  return upb_bytestream_putstr(s, str.c_str(), str.size());
}

}  // namespace upb

#endif

#endif  // UPB_BYTESTREAM_H_
