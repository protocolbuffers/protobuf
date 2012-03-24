//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>

#ifndef UPB_HPP
#define UPB_HPP

#include "upb/upb.h"
#include <iostream>

#if defined(__GXX_EXPERIMENTAL_CXX0X__) && !defined(UPB_NO_CXX11)
#define UPB_DISALLOW_CONSTRUCT_AND_DESTRUCT(class_name) \
  class_name() = delete; \
  ~class_name() = delete;
#else
#define UPB_DISALLOW_CONSTRUCT_AND_DESTRUCT(class_name) \
  class_name(); \
  ~class_name();
#endif

namespace upb {

typedef upb_success_t Success;

class Status : public upb_status {
 public:
  Status() { upb_status_init(this); }
  ~Status() { upb_status_uninit(this); }

  bool ok() const { return upb_ok(this); }
  bool eof() const { return upb_eof(this); }

  const char *GetString() const { return upb_status_getstr(this); }
  void SetEof() { upb_status_seteof(this); }
  void SetErrorLiteral(const char* msg) {
    upb_status_seterrliteral(this, msg);
  }

  void Clear() { upb_status_clear(this); }
};

typedef upb_value Value;

template <typename T> T GetValue(Value v);
template <typename T> Value MakeValue(T v);

#define UPB_VALUE_ACCESSORS(type, ctype) \
  template <> inline ctype GetValue<ctype>(Value v) { \
    return upb_value_get ## type(v); \
  } \
  template <> inline Value MakeValue<ctype>(ctype v) { \
    return upb_value_ ## type(v); \
  }

UPB_VALUE_ACCESSORS(double, double);
UPB_VALUE_ACCESSORS(float,  float);
UPB_VALUE_ACCESSORS(int32,  int32_t);
UPB_VALUE_ACCESSORS(int64,  int64_t);
UPB_VALUE_ACCESSORS(uint32, uint32_t);
UPB_VALUE_ACCESSORS(uint64, uint64_t);
UPB_VALUE_ACCESSORS(bool,   bool);

#undef UPB_VALUE_ACCESSORS

template <typename T> inline T* GetPtrValue(Value v) {
  return static_cast<T*>(upb_value_getptr(v));
}
template <typename T> inline Value MakePtrValue(T* v) {
  return upb_value_ptr(static_cast<void*>(v));
}

INLINE std::ostream& operator<<(std::ostream& out, const Status& status) {
  out << status.GetString();
  return out;
}

}  // namespace upb

#endif
