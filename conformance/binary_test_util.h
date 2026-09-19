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
//
// TODO: b/410121831 - The value tables of TestValidDataForType() and friends
// still live in binary_json_conformance_suite.cc and move here when their test
// groups migrate.

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

// Returns the first field of `message`, in declaration order, that is a member
// of a (non-synthetic) oneof and whose type is `type`, e.g. oneof_uint32 (111)
// for UINT32.  With `exclusive` set, returns the first oneof member whose type
// is *not* `type` instead, which the oneof tests use to find a second member
// of the same oneof to override.  Check-fails if there is no such field.
const FieldDescriptor* absl_nonnull GetFieldForOneofType(
    const Descriptor& message, FieldDescriptor::Type type,
    bool exclusive = false);

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

// The inputs of the valid-data tests in binary_merge_test.cc, which check that
// several occurrences of a field in one payload are combined correctly.  The
// legacy BinaryAndJsonConformanceSuite still sends these same inputs for its
// binary->JSON leg of the tests until JSON matching is available in the gtest
// suites, so they are built here once and used by both sides.
// TODO: b/410122158 - Move them into binary_merge_test.cc once the JSON leg
// has migrated.

// The input of a merge test together with the single occurrence of the field
// that the parsed input must be equivalent to (and, for the byte-exact test,
// serialize to).
struct MergeTestData {
  Wire input;
  Wire expected;
};

// RepeatedScalarMessageMerge: `message`'s singular message field
// (optional_nested_message) twice, each holding a `corecursive` submessage
// that sets optional_int32, one of optional_int64 / optional_uint32, and
// repeated_int32.  A parser must merge the two submessages rather than
// replace the first with the second.
Wire RepeatedScalarMessageMergeInput(const Descriptor& message);

// ValidDataMap.STRING.MESSAGE.MergeValue: two entries of `message`'s
// map_string_nested_message field with the same (empty) key and different
// message values.  A later map entry replaces an earlier one with the same
// key, so `expected` is the second entry alone.
MergeTestData MapMessageValueMergeData(const Descriptor& message);

// ValidDataOneof.MESSAGE.Merge: `message`'s oneof message field
// (oneof_nested_message) twice, each holding a `corecursive` submessage.
// Repeated occurrences of a oneof message field are merged like any other
// message field, so `expected` is a single occurrence holding the merged
// submessage.
MergeTestData OneofMessageMergeData(const Descriptor& message);

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
// name.  Every overload yields a non-empty string of letters and digits only,
// so components can be joined with "_" (see TupleParamName() in
// message_type_fixtures.h) and the result is always a valid gtest name.
// Add an overload here for each new parameter type the conformance tests are
// instantiated over.

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

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_BINARY_TEST_UTIL_H__
