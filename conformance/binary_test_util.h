// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_BINARY_TEST_UTIL_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_BINARY_TEST_UTIL_H__

#include "absl/types/span.h"
#include "google/protobuf/descriptor.h"

// Helpers shared by the gtest-based binary conformance tests: the field-type
// tables the tests are parameterized over.  Deliberately gtest-free.

namespace google {
namespace protobuf {
namespace conformance {

// Every field type except TYPE_GROUP (the 17 types the test messages have
// singular, repeated, packed and unpacked fields of), in the order the legacy
// valid-data tests ran them: DOUBLE, FLOAT, INT64, UINT64, INT32, UINT32,
// FIXED64, FIXED32, SFIXED64, SFIXED32, BOOL, SINT32, SINT64, STRING, BYTES,
// ENUM, MESSAGE.  This is the type axis of every test parameterized over "all
// field types" (premature EOF, valid data, ...).
absl::Span<const FieldDescriptor::Type> AllFieldTypesExceptGroup();

// The subset of AllFieldTypesExceptGroup() that can be packed
// (FieldDescriptor::IsTypePackable(): every numeric type, BOOL and ENUM), in
// the same order.
absl::Span<const FieldDescriptor::Type> PackableFieldTypes();

// The subset of AllFieldTypesExceptGroup() encoded length-delimited, i.e. the
// non-packable ones: STRING, BYTES, MESSAGE, in the same order.
absl::Span<const FieldDescriptor::Type> LengthDelimitedFieldTypes();

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_BINARY_TEST_UTIL_H__
