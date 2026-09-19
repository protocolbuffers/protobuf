// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_BINARY_TEST_UTIL_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_BINARY_TEST_UTIL_H__

#include <cstdint>
#include <string>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/binary_wireformat.h"
#include "google/protobuf/descriptor.h"

// Helpers shared by the gtest-based binary conformance tests: looking up the
// test-message fields exercised for a given field type, encoding sample
// values, the parameter sets the tests are instantiated over, and the
// stringification of those parameters for gtest names.  The lookups are ported
// from the legacy BinaryAndJsonConformanceSuite with identical semantics, since
// the requests built with them must stay byte-identical to the legacy ones.
//
// This header is deliberately gtest-free, so that the legacy suite can share
// it while both exist.  The fixtures and INSTANTIATE_TEST_SUITE_P name
// generators built on top of these helpers live in message_type_fixtures.h.

namespace google {
namespace protobuf {
namespace conformance {

// Restricts GetFieldForType() to packed or unpacked repeated fields.  (Named
// Packedness rather than Packed because Packed() is already the wire-format
// builder in binary_wireformat.h.)
enum class Packedness {
  kUnspecified = 0,
  kPacked = 1,
  kUnpacked = 2,
};

// Returns the first field of `message`, in declaration order, whose type is
// `type` and whose cardinality matches `repeated`.  When `packedness` isn't
// kUnspecified only fields whose is_packed() matches it are considered, which
// is how proto2 and proto3 messages (with their different packing defaults)
// select their [packed = true] / [packed = false] test fields.  Check-fails if
// `message` has no such field, exactly like the legacy suite; in particular,
// kPacked with `repeated` false is always fatal, since singular fields are
// never packed.
const FieldDescriptor* absl_nonnull GetFieldForType(
    const Descriptor& message, FieldDescriptor::Type type, bool repeated,
    Packedness packedness = Packedness::kUnspecified);

// `field`'s number as the wire-format builders of binary_wireformat.h take it
// (FieldDescriptor::number() is an int; field numbers are never negative).
inline uint32_t FieldNumber(const FieldDescriptor& field) {
  return static_cast<uint32_t>(field.number());
}

// Returns the first map field of `message`, in declaration order, whose keys
// are of type `key_type` and whose values are of type `value_type`, e.g.
// map_int32_int32 (56) for INT32/INT32 or map_string_nested_message (71) for
// STRING/MESSAGE.  Check-fails if `message` has no such field.
const FieldDescriptor* absl_nonnull GetFieldForMapType(
    const Descriptor& message, FieldDescriptor::Type key_type,
    FieldDescriptor::Type value_type);

// Which member of a oneof GetFieldForOneofType() selects relative to the
// requested field type.
enum class OneofMember {
  kOfType,       // The first member of the given type.
  kOfOtherType,  // The first member of any other type.
};

// Returns the first field of `message`, in declaration order, that is a member
// of a (non-synthetic) oneof and whose type is `type`, e.g. oneof_uint32 (111)
// for UINT32.  With OneofMember::kOfOtherType, returns the first oneof member
// whose type is *not* `type` instead, which the oneof tests use to find a
// second member of the same oneof to override.  Check-fails if there is no
// such field.
const FieldDescriptor* absl_nonnull GetFieldForOneofType(
    const Descriptor& message, FieldDescriptor::Type type,
    OneofMember member = OneofMember::kOfType);

// Returns the wire type a non-packed value of `type` is encoded with.
WireType WireTypeForFieldType(FieldDescriptor::Type type);

// Returns the upper-case name of `type` as it appears in legacy conformance
// test names, e.g. "INT32", "SFIXED64" or "MESSAGE".
std::string UpperCaseTypeName(FieldDescriptor::Type type);

// Returns the encoded default value of a field of `type` (zero, or an empty
// string, bytes or message), without a tag, exactly as the legacy suite
// encoded it.  TYPE_GROUP has no such single-value encoding and yields an
// empty Wire, again as in the legacy suite.
Wire GetDefaultValue(FieldDescriptor::Type type);

// Returns an encoded non-default value of a field of `type`, without a tag,
// exactly as the legacy suite encoded it: 1 for numeric types and enums, "a"
// for strings and bytes, and a message whose field 1 is the varint 1234.
// TYPE_GROUP yields an empty Wire, as in the legacy suite.
Wire GetNonDefaultValue(FieldDescriptor::Type type);

// Whether `payload`, the encoding of a value of `type` without its tag, is the
// encoding of the type's default value (zero, false, the empty string or
// bytes), i.e. what a proto3 serializer omits from a singular field.  Always
// false for TYPE_MESSAGE and TYPE_GROUP.  This is the legacy suite's
// IsProto3Default(); note that only the canonical encoding counts (a
// ten-byte varint 0 doesn't).
bool IsDefaultValue(FieldDescriptor::Type type, const Wire& payload);

// Whether the singular scalar fields of `message` have implicit presence, i.e.
// whether it follows proto3 semantics: true for TestAllTypesProto3 and its
// editions equivalent, false for the proto2 ones and TestAllTypesEdition2023.
// Decided by the presence of the message's first singular INT32 field
// (optional_int32 in every TestAllTypes), so it check-fails for a message
// without one.  This is what the legacy suite's `run_proto3_tests_` flag
// selected.
bool HasImplicitPresence(const Descriptor& message);

// One case of the valid-data tests (ValidDataScalar.<TYPE>[i] and friends):
// `input` is the encoding of a value as sent to the testee and `expected` is
// the canonical encoding a conforming parser must be equivalent to after
// reading it, e.g. an over-long varint and its minimal form, or a 64-bit
// varint and its truncation to 32 bits.  Both are without a tag.
struct ValidDataCase {
  Wire input;
  Wire expected;
};

// The valid-data cases of `type`, in the legacy order: case i is what the
// tests named ValidDataScalar.<TYPE>[i] and ValidDataScalarBinary.<TYPE>[i]
// send, and the repeated-field tests send all of them in this order.  The
// legacy BinaryAndJsonConformanceSuite iterates the same tables for the
// binary->JSON legs it still runs, so the requests of both stay identical.
// Check-fails for TYPE_GROUP, which has no cases.
absl::Span<const ValidDataCase> ValidDataCases(FieldDescriptor::Type type);

// The key and value types of a map field, selecting one of the
// map_<key>_<value> fields of the test messages (see GetFieldForMapType()).
struct MapType {
  FieldDescriptor::Type key;
  FieldDescriptor::Type value;
};

// The key/value type combinations of the map valid-data tests
// (ValidDataMap.<KEY>.<VALUE>.*), in the legacy order.
absl::Span<const MapType> ValidDataMapTypes();

// The field types of the oneof valid-data tests (ValidDataOneof.<TYPE>.* and
// ValidDataOneofBinary.<TYPE>.*), in the legacy order.  The legacy
// BinaryAndJsonConformanceSuite iterates the same table for the binary->JSON
// legs it still runs, so the requests of both stay identical.
absl::Span<const FieldDescriptor::Type> ValidDataOneofTypes();

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

// All four TestAllTypes message types the binary conformance tests run over,
// in the legacy order: Proto3, Proto2, Editions_Proto3, Editions_Proto2.  The
// editions variants are skipped at runtime by the ConformanceTest fixture when
// --maximum_edition doesn't cover them (see MessageUnderTest() and
// CONFORMANCE_SKIP_IF_UNSUPPORTED in test_environment.h).
//
// Calling this from INSTANTIATE_TEST_SUITE_P is fine: gtest evaluates the
// parameter generators during test registration (inside InitGoogleTest() /
// RUN_ALL_TESTS()), not during static initialization, so calling the
// generated messages' descriptor() accessors there is unproblematic.
std::vector<const Descriptor*> AllTestMessageTypes();

// The proto3-style subset of AllTestMessageTypes(), in the legacy order:
// Proto3, Editions_Proto3.  This is what the legacy suite ran under
// `run_proto3_tests_`, for tests whose expectations depend on proto3 semantics
// (e.g. UTF-8 validation of string fields).  Like AllTestMessageTypes(), safe
// to call from INSTANTIATE_TEST_SUITE_P.
std::vector<const Descriptor*> Proto3TestMessageTypes();

// The proto2-style subset of AllTestMessageTypes(), in the legacy order:
// Proto2, Editions_Proto2.  This is what the legacy binary/JSON suite ran when
// `run_proto3_tests_` was false, and what the legacy text-format suite ran for
// the two types named `TestAllTypesProto2`, for tests whose expectations
// depend on proto2 semantics (e.g. closed enums, required fields).  Like
// AllTestMessageTypes(), safe to call from INSTANTIATE_TEST_SUITE_P.
std::vector<const Descriptor*> Proto2TestMessageTypes();

// ParamName() renders one test parameter as a component of a gtest parameter
// name.  Every overload yields a non-empty string of letters and digits (plus
// "_" between the parts of a compound parameter), so components can be joined
// with "_" (see TupleParamName() in message_type_fixtures.h) and the result is
// always a valid gtest name.  Add an overload here for each new parameter type
// the conformance tests are instantiated over.

// `text` with every character that isn't a letter or digit removed, e.g.
// "Editions_Proto2" becomes "EditionsProto2".  `text` must contain at least one
// letter or digit.
std::string ParamName(absl::string_view text);

// The edition identifier of `message` as used in conformance test names (see
// GetEditionIdentifier() in naming.h), minus its underscore: "Proto3",
// "Proto2", "EditionsProto3", "EditionsProto2" or "EditionUnstable".
std::string ParamName(const Descriptor* absl_nonnull message);

// The upper-case type name, e.g. "INT32"; see UpperCaseTypeName().
std::string ParamName(FieldDescriptor::Type type);

// The decimal representation of `value`, which must not be negative.
std::string ParamName(int value);

// The key and value type names joined by "_", e.g. "INT32_INT32" or
// "STRING_MESSAGE".
std::string ParamName(const MapType& type);

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_BINARY_TEST_UTIL_H__
