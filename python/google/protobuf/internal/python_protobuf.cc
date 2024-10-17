// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: qrczak@google.com (Marcin Kowalczyk)

#include "google/protobuf/python_protobuf.h"

namespace google {
namespace protobuf {
namespace python {

static const Message* GetCProtoInsidePyProtoStub(PyObject* msg) {
  return nullptr;
}
static Message* MutableCProtoInsidePyProtoStub(PyObject* msg) {
  return nullptr;
}

// This is initialized with a default, stub implementation.
// If python-google.protobuf.cc is loaded, the function pointer is overridden
// with a full implementation.
const Message* (*GetCProtoInsidePyProtoPtr)(PyObject* msg) =
    GetCProtoInsidePyProtoStub;
Message* (*MutableCProtoInsidePyProtoPtr)(PyObject* msg) =
    MutableCProtoInsidePyProtoStub;

const Message* GetCProtoInsidePyProto(PyObject* msg) {
  return GetCProtoInsidePyProtoPtr(msg);
}
Message* MutableCProtoInsidePyProto(PyObject* msg) {
  return MutableCProtoInsidePyProtoPtr(msg);
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
