// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/explicitly_constructed.h"
#include "google/protobuf/has_bits.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_import.pb.h"

#define MESSAGE_TEST_NAME MessageTest
#define MESSAGE_FACTORY_TEST_NAME MessageFactoryTest
#define UNITTEST_PACKAGE_NAME "protobuf_unittest"
#define UNITTEST ::protobuf_unittest
#define UNITTEST_IMPORT ::protobuf_unittest_import

// Must include after the above macros.
// clang-format off
#include "google/protobuf/test_util.inc"
#include "google/protobuf/message_unittest.inc"
#include "google/protobuf/message_unittest_legacy_apis.inc"
// clang-format on
