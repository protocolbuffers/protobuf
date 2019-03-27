
#ifndef UPB_STDCPP_H_
#define UPB_STDCPP_H_

#include "upb/sink.h"

#include "upb/port_def.inc"

namespace upb {

template <class T>
class FillStringHandler {
 public:
  static void SetHandler(upb_byteshandler* handler) {
    upb_byteshandler_setstartstr(handler, &FillStringHandler::StartString,
                                 NULL);
    upb_byteshandler_setstring(handler, &FillStringHandler::StringBuf, NULL);
  }

 private:
  // TODO(haberman): add UpbBind/UpbMakeHandler support to BytesHandler so these
  // can be prettier callbacks.
  static void* StartString(void *c, const void *hd, size_t size) {
    UPB_UNUSED(hd);
    UPB_UNUSED(size);

    T* str = static_cast<T*>(c);
    str->clear();
    return c;
  }

  static size_t StringBuf(void* c, const void* hd, const char* buf, size_t n,
                          const upb_bufhandle* h) {
    UPB_UNUSED(hd);
    UPB_UNUSED(h);

    T* str = static_cast<T*>(c);
    try {
      str->append(buf, n);
      return n;
    } catch (const std::exception&) {
      return 0;
    }
  }
};

class StringSink {
 public:
  template <class T>
  explicit StringSink(T* target) {
    // TODO(haberman): we need to avoid rebuilding a new handler every time,
    // but with class globals disallowed for google3 C++ this is tricky.
    upb_byteshandler_init(&handler_);
    FillStringHandler<T>::SetHandler(&handler_);
    input_.Reset(&handler_, target);
  }

  BytesSink input() { return input_; }

 private:
  upb_byteshandler handler_;
  BytesSink input_;
};

}  // namespace upb

#include "upb/port_undef.inc"

#endif  // UPB_STDCPP_H_
