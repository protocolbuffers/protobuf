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

#ifndef __PROTOBUF_NODEJS_SRC_UTIL_H__
#define __PROTOBUF_NODEJS_SRC_UTIL_H__

#include <nan.h>
#include <math.h>
#include <string>
#include <iterator>
#include <map>

#include "upb.h"

namespace protobuf_js {

// Helper to check arguments in a method definition exported to Javascript.
//
// Use as:
//
// Handle<Value> Obj::MyMethod(const Arguments& args) {
//   HandleScope scope;
//   CheckArgs checkargs(args, STRING, INTEGER);
//   if (!checkargs.ok()) {
//     return scope.Close(Undefined());
//   }
// }
struct CheckArgs {
  enum ArgType {
    STRING,
    INTEGER,
    FLOAT,
    OBJECT,
    ARRAY,
  };

  CheckArgs(_NAN_METHOD_ARGS_TYPE args) {
    threw_ = false;
    CheckLen(args, 0);
  }
  CheckArgs(_NAN_METHOD_ARGS_TYPE args,
            ArgType arg1) {
    threw_ = false;
    CheckLen(args, 1);
    Check(args, 0, arg1);
  }
  CheckArgs(_NAN_METHOD_ARGS_TYPE args,
            ArgType arg1, ArgType arg2) {
    threw_ = false;
    CheckLen(args, 2);
    Check(args, 0, arg1);
    Check(args, 1, arg2);
  }
  CheckArgs(_NAN_METHOD_ARGS_TYPE args,
            ArgType arg1, ArgType arg2,
            ArgType arg3) {
    threw_ = false;
    CheckLen(args, 3);
    Check(args, 0, arg1);
    Check(args, 1, arg2);
    Check(args, 2, arg3);
  }
  CheckArgs(_NAN_METHOD_ARGS_TYPE args,
            ArgType arg1, ArgType arg2,
            ArgType arg3, ArgType arg4) {
    threw_ = false;
    CheckLen(args, 4);
    Check(args, 0, arg1);
    Check(args, 1, arg1);
    Check(args, 2, arg2);
    Check(args, 3, arg4);
  }

  bool ok() { return !threw_; }

 private:
  bool threw_;

  void CheckLen(_NAN_METHOD_ARGS_TYPE args, int length) {
    if (args.Length() != length) {
      NanThrowError("Incorrect number of arguments");
      threw_ = true;
    }
  }
  void Check(_NAN_METHOD_ARGS_TYPE args, int index, ArgType type) {
    v8::Local<v8::Value> value = args[index];
    switch (type) {
      case STRING:
        if (!value->IsString()) {
          NanThrowError("Expected string");
          threw_ = true;
        }
        break;
      case INTEGER:
        if (!value->IsInt32() || !value->IsUint32() ||
            (value->IsNumber() &&
             floor(value->NumberValue()) != value->NumberValue())) {
          NanThrowError("Expected integer");
          threw_ = true;
        }
        break;
      case FLOAT:
        if (!value->IsInt32() || !value->IsUint32() || !value->IsNumber()) {
          NanThrowError("Expected number");
          threw_ = true;
        }
        break;
      case OBJECT:
        if (!value->IsObject()) {
          NanThrowError("Expected object");
          threw_ = true;
        }
        break;
      case ARRAY:
        if (!value->IsArray()) {
          NanThrowError("Expected array");
          threw_ = true;
        }
        break;
    }
  }
};

// Helper to allow copyable persistent handles -- NAN doesn't abstract this.
// N.B. that template typedefs don't exist in C++03, so we use a #define
// instead.
//
// Why? Node v0.12 makes v8::Persistent noncopyable by default. This causes
// issues when holding v8 object references in C++ STL containers, for example.
// So we define this macro to use the copyable variant when required.
#if (NODE_MODULE_VERSION < 0x000C) // Node v0.10
#define PERSISTENT(type) v8::Persistent<type>
#else
#define PERSISTENT(type) v8::Persistent<type, v8::CopyablePersistentTraits<type> >
#endif

}  // namespace protobuf_js

// This must come outside the protobuf_js namespace:

#if (NODE_MODULE_VERSION >= 0x000C) // Node v0.12
// Define a new NanNew override to allow NanNew(persistent_handle) with
// CopyablePersistentTraits.
template<typename T>
NAN_INLINE v8::Local<T> NanNew(
    const PERSISTENT(T) & arg1) {
  return v8::Local<T>::New(v8::Isolate::GetCurrent(), arg1);
}

// Define new NanAssignPersistent overrides to allow assigning to copyable
// variants of Persistent handles.
template<typename T>
NAN_INLINE void NanAssignPersistent(
  PERSISTENT(T) & handle
, v8::Handle<T> obj) {
  handle.Reset(v8::Isolate::GetCurrent(), obj);
}

template<typename T>
NAN_INLINE void NanAssignPersistent(
  PERSISTENT(T) & handle
, const v8::Persistent<T>& obj) {
  handle.Reset(v8::Isolate::GetCurrent(), obj);
}
#endif

// Define a NanNew-like helper to create unsigned integers.
#if (NODE_MODULE_VERSION >= 0x000C) // Node v0.12
NAN_INLINE v8::Local<v8::Integer> NanNewInt32(int32_t val) {
  return v8::Integer::New(v8::Isolate::GetCurrent(), val)->ToInteger();
}
NAN_INLINE v8::Local<v8::Integer> NanNewUInt32(uint32_t val) {
  return v8::Integer::NewFromUnsigned(v8::Isolate::GetCurrent(), val)->ToInteger();
}
#else
NAN_INLINE v8::Local<v8::Integer> NanNewInt32(int32_t val) {
  return v8::Integer::New(val)->ToInteger();
}
NAN_INLINE v8::Local<v8::Integer> NanNewUInt32(uint32_t val) {
  return v8::Integer::NewFromUnsigned(val)->ToInteger();
}
#endif

// Utility to create a Buffer. NanNewBufferHandle() returns an instance of a
// node::Buffer, which is a SlowBuffer; we want a true Buffer to allow the user
// to perform fast slicing, etc.
//
// See
// http://www.samcday.com.au/blog/2011/03/03/creating-a-proper-buffer-in-a-node-c-addon/
// for details.
NAN_INLINE v8::Local<v8::Value> NewNodeBuffer(const char* data,
                                               size_t length) {
  v8::Local<v8::Value> slowbuf = data ?
      NanNewBufferHandle(data, length) : NanNewBufferHandle(length);

  v8::Local<v8::Object> global = NanGetCurrentContext()->Global();
  v8::Local<v8::Function> buffer_ctor =
      global->Get(NanNew<v8::String>("Buffer")).As<v8::Function>();
  v8::Handle<v8::Value> args[3] =
    { slowbuf, NanNew<v8::Integer>(length), NanNew<v8::Integer>(0) };
  v8::Local<v8::Object> buf = buffer_ctor->NewInstance(3, args);
  return buf;
}

#endif  //__PROTOBUF_NODEJS_SRC_UTIL_H__
