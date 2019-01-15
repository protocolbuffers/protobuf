
#include "upb/sink.h"

bool upb_bufsrc_putbuf(const char *buf, size_t len, upb_bytessink sink) {
  void *subc;
  bool ret;
  upb_bufhandle handle = UPB_BUFHANDLE_INIT;
  handle.buf = buf;
  ret = upb_bytessink_start(sink, len, &subc);
  if (ret && len != 0) {
    ret = (upb_bytessink_putbuf(sink, subc, buf, len, &handle) >= len);
  }
  if (ret) {
    ret = upb_bytessink_end(sink);
  }
  return ret;
}
