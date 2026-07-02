// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_STUBS_CALLBACK_H_
#define GOOGLE_PROTOBUF_STUBS_CALLBACK_H_

#include <type_traits>

#include "google/protobuf/port_def.inc"

// ===================================================================
// emulates google3/base/callback.h

namespace google {
namespace protobuf {

class PROTOBUF_EXPORT Closure {
 public:
  Closure() {}
  Closure(const Closure&) = delete;
  Closure& operator=(const Closure&) = delete;
  virtual ~Closure();

  virtual void Run() = 0;
};

// A function which does nothing.  Useful for creating no-op callbacks, e.g.:
//   Closure* nothing = NewCallback(&DoNothing);
void PROTOBUF_EXPORT DoNothing();

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_STUBS_CALLBACK_H_
