// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

#include <Python.h>

namespace google {
namespace protobuf {
namespace python {

// Version constant.
// This is either 0 for python, 1 for CPP V1, 2 for CPP V2.
//
// 0 is default and is equivalent to
//   PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=python
//
// 1 is set with -DPYTHON_PROTO2_CPP_IMPL_V1 and is equivalent to
//   PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=cpp
// and
//   PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION_VERSION=1
//
// 2 is set with -DPYTHON_PROTO2_CPP_IMPL_V2 and is equivalent to
//   PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=cpp
// and
//   PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION_VERSION=2
#ifdef PYTHON_PROTO2_CPP_IMPL_V1
#if PY_MAJOR_VERSION >= 3
#error "PYTHON_PROTO2_CPP_IMPL_V1 is not supported under Python 3."
#endif
static int kImplVersion = 1;
#else
#ifdef PYTHON_PROTO2_CPP_IMPL_V2
static int kImplVersion = 2;
#else
#ifdef PYTHON_PROTO2_PYTHON_IMPL
static int kImplVersion = 0;
#else

// The defaults are set here.  Python 3 uses the fast C++ APIv2 by default.
// Python 2 still uses the Python version by default until some compatibility
// issues can be worked around.
#if PY_MAJOR_VERSION >= 3
static int kImplVersion = 2;
#else
static int kImplVersion = 0;
#endif

#endif  // PYTHON_PROTO2_PYTHON_IMPL
#endif  // PYTHON_PROTO2_CPP_IMPL_V2
#endif  // PYTHON_PROTO2_CPP_IMPL_V1

static const char* kImplVersionName = "api_version";

static const char* kModuleName = "_api_implementation";
static const char kModuleDocstring[] =
"_api_implementation is a module that exposes compile-time constants that\n"
"determine the default API implementation to use for Python proto2.\n"
"\n"
"It complements api_implementation.py by setting defaults using compile-time\n"
"constants defined in C, such that one can set defaults at compilation\n"
"(e.g. with blaze flag --copt=-DPYTHON_PROTO2_CPP_IMPL_V2).";

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef _module = {
  PyModuleDef_HEAD_INIT,
  kModuleName,
  kModuleDocstring,
  -1,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
#define INITFUNC PyInit__api_implementation
#define INITFUNC_ERRORVAL NULL
#else
#define INITFUNC init_api_implementation
#define INITFUNC_ERRORVAL
#endif

extern "C" {
  PyMODINIT_FUNC INITFUNC() {
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&_module);
#else
    PyObject *module = Py_InitModule3(
        const_cast<char*>(kModuleName),
        NULL,
        const_cast<char*>(kModuleDocstring));
#endif
    if (module == NULL) {
      return INITFUNC_ERRORVAL;
    }

    // Adds the module variable "api_version".
    if (PyModule_AddIntConstant(
        module,
        const_cast<char*>(kImplVersionName),
        kImplVersion))
#if PY_MAJOR_VERSION < 3
      return;
#else
      { Py_DECREF(module); return NULL; }

    return module;
#endif
  }
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
