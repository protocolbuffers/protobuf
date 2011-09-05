/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#ifndef UPB_HPP
#define UPB_HPP

#include "upb/upb.h"

namespace upb {

class Status : public upb_status {
 public:
  Status() { upb_status_init(this); }
  ~Status() { upb_status_uninit(this); }

  const char *GetString() const { return upb_status_getstr(this); }
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
