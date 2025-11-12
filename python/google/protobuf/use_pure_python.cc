// Protocol Buffers - Google's data interchange format
// Copyright 2021 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <Python.h>

namespace google {
namespace protobuf {
namespace python {

static const char* kModuleName = "use_pure_python";
static const char kModuleDocstring[] =
    "The presence of this module in a build's deps signals to\n"
    "net.google.protobuf.internal.api_implementation that the pure\n"
    "Python protobuf implementation should be used.\n";

static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                     kModuleName,
                                     kModuleDocstring,
                                     -1,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr};

extern "C" {


PyMODINIT_FUNC PyInit_use_pure_python() {
  PyObject* module = PyModule_Create(&_module);
  if (module == nullptr) {
    return nullptr;
  }


  return module;
}
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
