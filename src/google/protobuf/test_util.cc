// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifdef _WIN32
// Verify that #including windows.h does not break anything (e.g. because
// windows.h #defines GetMessage() as a macro).
#include <windows.h>
#endif

#include "google/protobuf/test_util.h"

namespace google {
namespace protobuf {

}  // namespace protobuf
}  // namespace google
