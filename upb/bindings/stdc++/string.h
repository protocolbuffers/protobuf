// Copyright 2014 Google Inc. All Rights Reserved.
// Author: haberman@google.com (Josh Haberman)
//
// upb - a minimalist implementation of protocol buffers.

#ifndef UPB_STDCPP_H_
#define UPB_STDCPP_H_

#include "upb/sink.h"

namespace upb {

template <class T>
class FillStringHandler {
 public:
  static void SetHandler(BytesHandler* handler) {
    upb_byteshandler_setstartstr(handler, &FillStringHandler::StartString,
                                 NULL);
    upb_byteshandler_setstring(handler, &FillStringHandler::StringBuf, NULL);
  }

 private:
  // TODO(haberman): add UpbBind/UpbMakeHandler support to BytesHandler so these
  // can be prettier callbacks.
  static void* StartString(void *c, const void *hd, size_t size) {
    T* str = static_cast<T*>(c);
    str->clear();
    return c;
  }

  static size_t StringBuf(void* c, const void* hd, const char* buf, size_t n,
                          const BufferHandle* h) {
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
    FillStringHandler<T>::SetHandler(&handler_);
    input_.Reset(&handler_, target);
  }

  BytesSink* input() { return &input_; }

 private:
  BytesHandler handler_;
  BytesSink input_;
};

}  // namespace upb

#endif  // UPB_STDCPP_H_
