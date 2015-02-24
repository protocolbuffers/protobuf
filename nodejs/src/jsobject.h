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

#ifndef __PROTOBUF_NODEJS_SRC_JSOBJECT_H__
#define __PROTOBUF_NODEJS_SRC_JSOBJECT_H__

#include <nan.h>

namespace protobuf_js {

// This is a replacement for node::JSObject that does dynamic typechecking and
// throws errors on type mismatches. This eliminates a lot of boilerplate when
// taking an arbitrary v8::Value from the user and converting to Object to
// validating the object type to unwrapping it.

#define JS_OBJECT(type)                                              \
    static const char* type_id;                                      \
    static type* unwrap(v8::Handle<v8::Value> value) {               \
      return JSObject::Unwrap<type>(value);                          \
    }

#define JS_OBJECT_INIT(type)                                         \
  const char* type :: type_id = #type;

#define JS_OBJECT_WRAP_SLOTS   2

class JSObject {
 public:
  JSObject()  {}
  ~JSObject()  {}

  typedef char* TypeID;

  template<typename T>
  void Wrap(v8::Handle<v8::Object> object) {
    NanEscapableScope();
    // We need the first two internal fields for (i) the TypeID and (ii) the
    // C++ object pointer.
    assert(object->InternalFieldCount() >= JS_OBJECT_WRAP_SLOTS);
#if (NODE_MODULE_VERSION < 0x000C)  // Node v0.10
    object->SetPointerInInternalField(
            0, GetTypeIDPointer(T::type_id));
    object->SetPointerInInternalField(1, this);
#else
    object->SetAlignedPointerInInternalField(
            0, GetTypeIDPointer(T::type_id));
    object->SetAlignedPointerInInternalField(1, this);
#endif
    NanAssignPersistent(handle_, object);
  }

  template<typename T>
  static T* Unwrap(v8::Handle<v8::Value> value) {
    NanEscapableScope();
    if (!value->IsObject()) {
      NanThrowError("Expected object");
      return NULL;
    }
    v8::Local<v8::Object> object = value->ToObject();
    if (object->InternalFieldCount() < JS_OBJECT_WRAP_SLOTS) {
      NanThrowError("Object does not seem to be a wrapped C++ object");
      return NULL;
    }
#if (NODE_MODULE_VERSION < 0x000C)  // Node v0.10
    void* obj_type_id =
        object->GetPointerFromInternalField(0);
    T* obj = reinterpret_cast<T*>(
        object->GetPointerFromInternalField(1));
#else
    void* obj_type_id =
        object->GetAlignedPointerFromInternalField(0);
    T* obj = reinterpret_cast<T*>(
        object->GetAlignedPointerFromInternalField(1));
#endif
    if (obj_type_id != GetTypeIDPointer(T::type_id)) {
      NanThrowError("Object is not of the correct type");
      return NULL;
    }
    return obj;
  }

  v8::Handle<v8::Object> object() {
    NanEscapableScope();
    return NanEscapeScope(NanNew(handle_));
  }

 private:
  v8::Persistent<v8::Object> handle_;

  static void* GetTypeIDPointer(const char* type_id) {
    // Node.js v0.12 and above supports storing only *aligned* pointers in
    // internal object slots. Presumably because some tagged-pointer stuff is
    // going on under the covers. The static string pointers we use as type IDs
    // aren't necessarily aligned. So we simply left-shift by a few bits.
    static const int kPtrShift = 3;
    return const_cast<void*>(reinterpret_cast<const void*>(
            reinterpret_cast<intptr_t>(type_id) << kPtrShift));
  }
};

}  // namespace protobuf_js

#endif  // __PROTOBUF_NODEJS_SRC_JSOBJECT_H__
