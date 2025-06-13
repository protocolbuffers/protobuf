// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/hash/hash_testing.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/explicitly_constructed.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/has_bits.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_import.pb.h"
#include "google/protobuf/unittest_lite.pb.h"

#define MESSAGE_TEST_NAME MessageTest
#define MESSAGE_FACTORY_TEST_NAME MessageFactoryTest
#define UNITTEST_PACKAGE_NAME "proto2_unittest"
#define UNITTEST ::proto2_unittest
#define UNITTEST_IMPORT ::proto2_unittest_import

// Must include after the above macros.
// clang-format off
#include "google/protobuf/message_unittest.inc"
#include "google/protobuf/message_unittest_legacy_apis.inc"
// clang-format on
