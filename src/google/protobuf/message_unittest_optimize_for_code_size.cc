// Copyright 2007 Google Inc.  All rights reserved.
// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/unittest_import_optimize_for_code_size.pb.h"
#include "google/protobuf/unittest_optimize_for_code_size.pb.h"  // IWYU pragma: keep

#define MESSAGE_TEST_NAME MessageTestOptimizeForCodeSize
#define MESSAGE_FACTORY_TEST_NAME MessageFactoryTestOptimizeForCodeSize
#define UNITTEST_PACKAGE_NAME "proto2_unittest_optimize_for_code_size"
#define UNITTEST ::proto2_unittest_optimize_for_code_size
#define UNITTEST_IMPORT ::proto2_unittest_import_optimize_for_code_size

// Must include after the above macros.
// clang-format off
#include "google/protobuf/message_unittest.inc"
#include "google/protobuf/message_unittest_legacy_apis.inc"
// clang-format on
