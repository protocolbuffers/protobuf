// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_BINARY_TEST_UTIL_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_BINARY_TEST_UTIL_H__

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/binary_wireformat.h"
#include "google/protobuf/descriptor.h"

// Helpers shared by the gtest-based binary conformance tests.  They look up
// the test-message fields exercised for a given field type, define the value
// sets the tests are parameterized over, and name those values for gtest.
//
// This header is deliberately gtest-free, so that the existing conformance
// suite can share it while both exist.  The fixtures built on top of these
// helpers live in message_type_fixtures.h.
//
// TODO: GetFieldForMapType(), GetFieldForOneofType(),
// GetDefaultValue(), GetNonDefaultValue() and friends still live in
// binary_json_conformance_suite.cc and move here when their test groups
// migrate.

namespace google {
namespace protobuf {
namespace conformance {

// Restricts GetFieldForType() to packed or unpacked repeated fields.  Named
// Packedness rather than Packed because Packed() is already the wire-format
// builder in binary_wireformat.h.
enum class Packedness {
  kUnspecified = 0,
  kPacked = 1,
  kUnpacked = 2,
};

// Returns the first field of `message`, in declaration order, whose type is
// `type` and whose cardinality matches `repeated`.  When `packedness` isn't
// kUnspecified, only fields whose is_packed() matches it are considered.
// That is how proto2 and proto3 messages, with their different packing
// defaults, select their [packed = true] and [packed = false] test fields.
//
//   const FieldDescriptor* field = GetFieldForType(
//       message, FieldDescriptor::TYPE_INT32, /*repeated=*/true,
//       Packedness::kPacked);
//
// Check-fails if `message` has no such field.  In particular, kPacked with
// `repeated` false is always fatal, since singular fields are never packed.
const FieldDescriptor* absl_nonnull GetFieldForType(
    const Descriptor& message, FieldDescriptor::Type type, bool repeated,
    Packedness packedness = Packedness::kUnspecified);

// Returns `field`'s number as the wire-format builders of binary_wireformat.h
// take it.  FieldDescriptor::number() is an int, but field numbers are never
// negative.
inline uint32_t FieldNumber(const FieldDescriptor& field) {
  return static_cast<uint32_t>(field.number());
}

// Returns the wire type a non-packed value of `type` is encoded with.
WireType WireTypeForFieldType(FieldDescriptor::Type type);

// Returns the upper-case name of `type` as it appears in conformance test
// names, e.g. "INT32", "SFIXED64" or "MESSAGE".
std::string UpperCaseTypeName(FieldDescriptor::Type type);

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

// Returns all four TestAllTypes message types the binary conformance tests run
// over, in this order: Proto3, Proto2, Editions_Proto3, Editions_Proto2.
//
//   const Descriptor* message_ =
//       PB_TEST_CHOOSE_VALUE(AllTestMessageTypes(), MessageTypeName);
//
// The editions variants are skipped at runtime by the ConformanceTest fixture
// when --maximum_edition doesn't cover them (see MessageUnderTest() in
// test_fixture.h).
//
// PB_TEST_CHOOSE_VALUE evaluates this during static initialization, which is
// fine: the generated descriptor() accessors build their descriptors lazily.
std::vector<const Descriptor*> AllTestMessageTypes();

// Returns the proto2-style subset of AllTestMessageTypes(), in this order:
// Proto2, Editions_Proto2.  For tests whose expectations depend on proto2
// semantics, for example closed enums or required fields.
std::vector<const Descriptor*> Proto2TestMessageTypes();

// Printers for the PB_TEST_CHOOSE_VALUE dimensions the conformance suites are
// parameterized over (see protobuf_test.h):
//
//   const Descriptor* message_ =
//       PB_TEST_CHOOSE_VALUE(AllTestMessageTypes(), MessageTypeName);
//
// Each yields a non-empty string of letters and digits only, so that the name
// is also a valid component of a failure list entry.  The `index` argument of
// the printer signature is unused.  Add a printer here for each new value type
// the suites are parameterized over.

// The edition identifier of `message` as used in conformance test names (see
// GetEditionIdentifier() in naming.h), minus its underscore: "Proto3",
// "Proto2", "EditionsProto3", "EditionsProto2" or "EditionUnstable".
std::string MessageTypeName(const Descriptor* absl_nonnull message,
                            size_t index);

// The upper-case type name, e.g. "INT32"; see UpperCaseTypeName().
std::string FieldTypeName(FieldDescriptor::Type type, size_t index);

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_BINARY_TEST_UTIL_H__
