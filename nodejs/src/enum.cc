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
#include "enum.h"

using namespace v8;
using namespace node;

namespace protobuf_js {

JS_OBJECT_INIT(ProtoEnum);

Persistent<Function> ProtoEnum::constructor;

// TODO: make sure enum name is shown in .toString() for messages.

void ProtoEnum::Init(Handle<Object> exports) {
  NanEscapableScope();
  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
  tpl->ReadOnlyPrototype();
  tpl->SetClassName(NanNew<String>("ProtoEnum"));
  tpl->InstanceTemplate()->SetInternalFieldCount(JS_OBJECT_WRAP_SLOTS);
  NanAssignPersistent(constructor, tpl->GetFunction());
}

NAN_METHOD(ProtoEnum::New) {
  NanEscapableScope();
  if (args.IsConstructCall()) {
    ProtoEnum* self = new ProtoEnum();
    self->Wrap<ProtoEnum>(args.This());
    if (!self->HandleCtorArgs(args)) {
      NanReturnUndefined();
    }
    self->FillEnumValues(args.This());
    NanReturnValue(args.This());
  } else {
    NanThrowError("Must be called as constructor");
    NanReturnUndefined();
  }
}

bool ProtoEnum::HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args) {
  NanEscapableScope();
  if (args.Length() != 1) {
    NanThrowError("Expected one constructor arg: an EnumDescriptor instance");
    return false;
  }
  if (!args[0]->IsObject()) {
    NanThrowError("First constructor arg must be an object");
    return false;
  }
  Local<Object> enumdesc_obj = NanNew(args[0]->ToObject());
  if (enumdesc_obj->GetPrototype() != EnumDescriptor::prototype) {
    NanThrowError("Expected an EnumDescriptor instance as constructor arg");
    return false;
  }

  NanAssignPersistent(enumdesc_obj_, enumdesc_obj);
  enumdesc_ = EnumDescriptor::unwrap(enumdesc_obj);

  return true;
}

void ProtoEnum::FillEnumValues(v8::Handle<v8::Object> this_) {
  NanEscapableScope();
  for (upb::EnumDef::Iterator it(enumdesc_->enumdef());
       !it.Done(); it.Next()) {
    this_->Set(NanNew<String>(it.name()),
               NanNew<Integer>(it.number()));
  }
}

}  // namespace protobuf_js
