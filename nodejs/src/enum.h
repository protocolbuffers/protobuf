// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef __PROTOBUF_NODEJS_SRC_ENUM_H__
#define __PROTOBUF_NODEJS_SRC_ENUM_H__

#include <nan.h>
#include "defs.h"
#include "util.h"
#include "jsobject.h"

namespace protobuf_js {

class ProtoEnum : public JSObject {
 public:
  JS_OBJECT(ProtoEnum);

  static void Init(v8::Handle<v8::Object> exports);

  static v8::Persistent<v8::Function> constructor;

 private:
  ProtoEnum() : enumdesc_(NULL) {}
  ~ProtoEnum() {}

  static NAN_METHOD(New);

  bool HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args);
  void FillEnumValues(v8::Handle<v8::Object> this_);

  EnumDescriptor* enumdesc_;
  v8::Persistent<v8::Object> enumdesc_obj_;
};

}  // namespace protobuf_js

#endif  // __PROTOBUF_NODEJS_SRC_ENUM_H__
