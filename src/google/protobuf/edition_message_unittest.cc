// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// This test is meant to verify the interaction of the most common and
// representative edition features. Each new edition feature must have its own
// unit test and we'll selectively accept new features when we believe doing so
// improves test coverage in a meaningful way.
//
// Note that new features that break the backward compatibility poses challenges
// to shared unit tests infrastructure this test uses. It may force us to split
// the shared tests. Keep the shared unit tests (message_unittest.inc)
// representative without sacrificing test coverage.

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/edition_unittest.pb.h"
#include "google/protobuf/explicitly_constructed.h"
#include "google/protobuf/has_bits.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/unittest_import.pb.h"

#define MESSAGE_TEST_NAME EditionMessageTest
#define MESSAGE_FACTORY_TEST_NAME EditionMessageFactoryTest
#define UNITTEST_PACKAGE_NAME "edition_unittest"
#define UNITTEST ::edition_unittest
#define UNITTEST_IMPORT ::protobuf_unittest_import

// Must include after the above macros.
// clang-format off
#include "google/protobuf/test_util.inc"
#include "google/protobuf/message_unittest.inc"
// clang-format on
