// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/wire_format.h"

#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_mset.pb.h"
#include "google/protobuf/unittest_mset_wire_format.pb.h"
#include "google/protobuf/unittest_proto3_arena.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#define UNITTEST ::protobuf_unittest
#define UNITTEST_IMPORT ::protobuf_unittest_import
#define UNITTEST_PACKAGE_NAME "protobuf_unittest"
#define PROTO2_WIREFORMAT_UNITTEST ::proto2_wireformat_unittest
#define PROTO3_ARENA_UNITTEST ::proto3_arena_unittest

// Must include after defining UNITTEST, etc.
// clang-format off
#include "google/protobuf/test_util.inc"
#include "google/protobuf/wire_format_unittest.inc"
// clang-format on

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace {


}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
