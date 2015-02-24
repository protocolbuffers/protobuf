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

#include <nan.h>

#include "defs.h"
#include "int64.h"
#include "readonlyarray.h"
#include "util.h"
#include "message.h"
#include "enum.h"
#include "repeatedfield.h"
#include "map.h"
#include "encode_decode.h"

using namespace v8;
using namespace node;

namespace protobuf_js {

void init(Handle<Object> exports) {
  Int64::Init(exports);
  ReadOnlyArray::Init(exports);
  Descriptor::Init(exports);
  FieldDescriptor::Init(exports);
  OneofDescriptor::Init(exports);
  EnumDescriptor::Init(exports);
  DescriptorPool::Init(exports);
  RepeatedField::Init(exports);
  Map::Init(exports);
  ProtoMessage::Init(exports);
  ProtoEnum::Init(exports);
}

}  // namespace protobuf_js

// On OS X builds, these are required. TODO: figure out which macro defs we need
// to flip to eliminate this dependence.
extern "C" {
void upb_lock() {}
void upb_unlock() {}
}

NODE_MODULE(google_protobuf_cpp, protobuf_js::init)
