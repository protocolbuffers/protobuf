//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>

#ifndef UPB_HPP
#define UPB_HPP

#include "upb/upb.h"
#include <iostream>

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

class Value : public upb_value {
 public:
  Value(const upb_value& val) { *this = val; }
  Value() {}
};

INLINE std::ostream& operator<<(std::ostream& out, const Status& status) {
  out << status.GetString();
  return out;
}

}  // namespace upb

#endif
