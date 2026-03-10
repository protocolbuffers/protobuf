// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: timdn@google.com (Tim Niemueller)

#include <pybind11/pybind11.h>

#include <memory>

#include "google/protobuf/message.h"
#include "google/protobuf/internal/self_recursive.pb.h"
#include "third_party/pybind11_protobuf/native_proto_caster.h"

namespace google::protobuf::python {

namespace py = pybind11;

void invoke_callback_on_message(py::object callback,
                                const google::protobuf::Message& exemplar) {
  std::shared_ptr<google::protobuf::Message> new_msg(exemplar.New());
  callback(new_msg);
}

PYBIND11_MODULE(pybind11_test_module, m) {
  pybind11_protobuf::ImportNativeProtoCasters();
  google::protobuf::LinkMessageReflection<
      google::protobuf::python::internal::SelfRecursive>();

  m.def("invoke_callback_on_message", &invoke_callback_on_message,
        py::arg("callback"), py::arg("message"));
}

}  // namespace protobuf
}  // namespace google::python
