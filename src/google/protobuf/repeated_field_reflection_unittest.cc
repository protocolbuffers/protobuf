// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: tgs@google.com (Tom Szymanski)
//
// Test reflection methods for aggregate access to Repeated[Ptr]Fields.
// This test proto2 methods on a proto2 layout.

#include "google/protobuf/unittest.pb.h"  // IWYU pragma: keep, used in UNITTEST

#define REFLECTION_TEST RepeatedFieldReflectionTest
#define UNITTEST_PACKAGE_NAME "protobuf_unittest"
#define UNITTEST ::protobuf_unittest
#define UNITTEST_IMPORT ::protobuf_unittest_import

// Must include after the above macros.
// clang-format off
#include "google/protobuf/test_util.inc"
#include "google/protobuf/repeated_field_reflection_unittest.inc"
// clang-format on
