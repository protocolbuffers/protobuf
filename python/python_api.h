// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC nor the names of its
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

#ifndef PYUPB_PYTHON_H__
#define PYUPB_PYTHON_H__

// We restrict ourselves to the limited API, so that a single build can be
// ABI-compatible with a wide range of Python versions.
//
// The build system will define Py_LIMITED_API as appropriate (see BUILD). We
// only want to define it for our distribution packages, since we can do some
// extra assertions when Py_LIMITED_API is not defined.  Also Py_LIMITED_API is
// incompatible with Py_DEBUG.

// #define Py_LIMITED_API <val>  // Defined by build system when appropriate.

#include "Python.h"

// Ideally we could restrict ourselves to the limited API of 3.7, but this is
// a very important function that was not officially added to the limited API
// until 3.10.  Without this function, there is no way of getting data from a
// Python `str` object without a copy.
//
// While this function was not *officially* added to the limited API until
// Python 3.10, In practice it has been stable since Python 3.1.
//   https://bugs.python.org/issue41784
//
// On Linux/ELF and macOS/Mach-O, we can get away with using this function with
// the limited API prior to 3.10.

#if (defined(__linux__) || defined(__APPLE__)) && defined(Py_LIMITED_API) && \
    Py_LIMITED_API < 0x03100000
PyAPI_FUNC(const char*)
    PyUnicode_AsUTF8AndSize(PyObject* unicode, Py_ssize_t* size);
#endif

#endif  // PYUPB_PYTHON_H__
