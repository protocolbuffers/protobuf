// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/binary_test_util.h"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/binary_wireformat.h"
#include "conformance/naming.h"
#include "google/protobuf/descriptor.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "editions/golden/test_messages_proto3_editions.pb.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/wire_format_lite.h"

namespace google {
namespace protobuf {
namespace conformance {
const FieldDescriptor* absl_nonnull GetFieldForType(const Descriptor& message,
                                                    FieldDescriptor::Type type,
                                                    bool repeated,
                                                    Packedness packedness) {
  for (int i = 0; i < message.field_count(); i++) {
    const FieldDescriptor* field = message.field(i);
    if (field->type() == type && field->is_repeated() == repeated) {
      if ((packedness == Packedness::kPacked && !field->is_packed()) ||
          (packedness == Packedness::kUnpacked && field->is_packed())) {
        continue;
      }
      return field;
    }
  }

  absl::string_view packed_string = "";
  const absl::string_view repeated_string =
      repeated ? "Repeated " : "Singular ";
  if (packedness == Packedness::kPacked) {
    packed_string = "Packed ";
  }
  if (packedness == Packedness::kUnpacked) {
    packed_string = "Unpacked ";
  }
  ABSL_LOG(FATAL) << "Couldn't find field with type: " << repeated_string
                  << packed_string << FieldDescriptor::TypeName(type) << " for "
                  << message.full_name();
}

const FieldDescriptor* absl_nonnull GetFieldForMapType(
    const Descriptor& message, FieldDescriptor::Type key_type,
    FieldDescriptor::Type value_type) {
  for (int i = 0; i < message.field_count(); i++) {
    const FieldDescriptor* field = message.field(i);
    if (!field->is_map()) continue;
    const Descriptor* entry = field->message_type();
    if (entry->map_key()->type() == key_type &&
        entry->map_value()->type() == value_type) {
      return field;
    }
  }
  ABSL_LOG(FATAL) << "Couldn't find map field with type: "
                  << FieldDescriptor::TypeName(key_type) << " and "
                  << FieldDescriptor::TypeName(value_type) << " for "
                  << message.full_name();
}

const FieldDescriptor* absl_nonnull GetFieldForOneofType(
    const Descriptor& message, FieldDescriptor::Type type, OneofMember member) {
  const bool of_type = member == OneofMember::kOfType;
  for (int i = 0; i < message.field_count(); i++) {
    const FieldDescriptor* field = message.field(i);
    if (field->real_containing_oneof() != nullptr &&
        (field->type() == type) == of_type) {
      return field;
    }
  }
  ABSL_LOG(FATAL) << "Couldn't find oneof field with type"
                  << (of_type ? ": " : " other than: ")
                  << FieldDescriptor::TypeName(type) << " for "
                  << message.full_name();
}

WireType WireTypeForFieldType(FieldDescriptor::Type type) {
  // WireType's enumerators are defined as WireFormatLite's, so the cast is
  // value-preserving.
  return static_cast<WireType>(
      ::google::protobuf::internal::WireFormatLite::WireTypeForFieldType(
          static_cast<::google::protobuf::internal::WireFormatLite::FieldType>(type)));
}

std::string UpperCaseTypeName(FieldDescriptor::Type type) {
  return absl::AsciiStrToUpper(FieldDescriptor::TypeName(type));
}

Wire GetDefaultValue(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_BOOL:
      return Varint(0);
    case FieldDescriptor::TYPE_SINT32:
      return SInt32(0);
    case FieldDescriptor::TYPE_SINT64:
      return SInt64(0);
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
      return Fixed32(0);
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      return Fixed64(0);
    case FieldDescriptor::TYPE_FLOAT:
      return Float(0);
    case FieldDescriptor::TYPE_DOUBLE:
      return Double(0);
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_MESSAGE:
      return LengthPrefixed("");
    case FieldDescriptor::TYPE_GROUP:
      return Wire();
  }
  ABSL_LOG(FATAL) << "Unknown field type: " << static_cast<int>(type);
}

Wire GetNonDefaultValue(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_BOOL:
      return Varint(1);
    case FieldDescriptor::TYPE_SINT32:
      return SInt32(1);
    case FieldDescriptor::TYPE_SINT64:
      return SInt64(1);
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
      return Fixed32(1);
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      return Fixed64(1);
    case FieldDescriptor::TYPE_FLOAT:
      return Float(1);
    case FieldDescriptor::TYPE_DOUBLE:
      return Double(1);
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      return LengthPrefixed("a");
    case FieldDescriptor::TYPE_MESSAGE:
      return LengthPrefixed(VarintField(1, 1234));
    case FieldDescriptor::TYPE_GROUP:
      return Wire();
  }
  ABSL_LOG(FATAL) << "Unknown field type: " << static_cast<int>(type);
}

bool IsDefaultValue(FieldDescriptor::Type type, const Wire& payload) {
  if (type == FieldDescriptor::TYPE_MESSAGE ||
      type == FieldDescriptor::TYPE_GROUP) {
    return false;
  }
  return payload == GetDefaultValue(type);
}

bool HasImplicitPresence(const Descriptor& message) {
  return !GetFieldForType(message, FieldDescriptor::TYPE_INT32,
                          /*repeated=*/false)
              ->has_presence();
}

absl::Span<const ValidDataCase> ValidDataCases(FieldDescriptor::Type type) {
  constexpr int64_t kInt64Min = std::numeric_limits<int64_t>::min();
  constexpr int64_t kInt64Max = std::numeric_limits<int64_t>::max();
  constexpr uint64_t kUint64Max = std::numeric_limits<uint64_t>::max();
  constexpr int32_t kInt32Min = std::numeric_limits<int32_t>::min();
  constexpr int32_t kInt32Max = std::numeric_limits<int32_t>::max();
  constexpr uint32_t kUint32Max = std::numeric_limits<uint32_t>::max();

  // The tables are the legacy suite's, verbatim (including the entries whose
  // input isn't canonical: over-long varints, out-of-range varints that must
  // be truncated or clamped, and non-zero bools).  Their order is part of the
  // test names, so entries may only be appended.
  switch (type) {
    case FieldDescriptor::TYPE_DOUBLE: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {Double(0), Double(0)},
          {Double(0.1), Double(0.1)},
          {Double(1.7976931348623157e+308), Double(1.7976931348623157e+308)},
          {Double(2.22507385850720138309e-308),
           Double(2.22507385850720138309e-308)},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_FLOAT: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {Float(0), Float(0)},
          {Float(0.1), Float(0.1)},
          {Float(1.00000075e-36), Float(1.00000075e-36)},
          {Float(3.402823e+38), Float(3.402823e+38)},  // 3.40282347e+38
          {Float(1.17549435e-38f), Float(1.17549435e-38)},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_INT64: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {Varint(0), Varint(0)},
          {Varint(12345), Varint(12345)},
          {Varint(kInt64Max), Varint(kInt64Max)},
          {Varint(kInt64Min), Varint(kInt64Min)},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_UINT64: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {Varint(0), Varint(0)},
          {Varint(12345), Varint(12345)},
          {Varint(kUint64Max), Varint(kUint64Max)},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_INT32: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {Varint(0), Varint(0)},
          {Varint(12345), Varint(12345)},
          {LongVarint(12345, 2), Varint(12345)},
          {LongVarint(12345, 7), Varint(12345)},
          {Varint(kInt32Max), Varint(kInt32Max)},
          {Varint(kInt32Min), Varint(kInt32Min)},
          {Varint(1LL << 33), Varint(0)},
          {Varint((1LL << 33) - 1), Varint(-1)},
          {Varint(kInt64Max), Varint(-1)},
          {Varint(kInt64Min + 1), Varint(1)},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_UINT32: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {Varint(0), Varint(0)},
          {Varint(12345), Varint(12345)},
          {LongVarint(12345, 2), Varint(12345)},
          {LongVarint(12345, 7), Varint(12345)},
          {Varint(kUint32Max), Varint(kUint32Max)},  // UINT32_MAX
          {Varint(1LL << 33), Varint(0)},
          {Varint((1LL << 33) + 1), Varint(1)},
          {Varint((1LL << 33) - 1), Varint((1LL << 32) - 1)},
          {Varint(kInt64Max), Varint((1LL << 32) - 1)},
          {Varint(kInt64Min + 1), Varint(1)},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_FIXED64: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {Fixed64(0), Fixed64(0)},
          {Fixed64(12345), Fixed64(12345)},
          {Fixed64(kUint64Max), Fixed64(kUint64Max)},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_FIXED32: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {Fixed32(0), Fixed32(0)},
          {Fixed32(12345), Fixed32(12345)},
          {Fixed32(kUint32Max), Fixed32(kUint32Max)},  // UINT32_MAX
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_SFIXED64: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {Fixed64(0), Fixed64(0)},
          {Fixed64(12345), Fixed64(12345)},
          {Fixed64(kInt64Max), Fixed64(kInt64Max)},
          {Fixed64(kInt64Min), Fixed64(kInt64Min)},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_SFIXED32: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {Fixed32(0), Fixed32(0)},
          {Fixed32(12345), Fixed32(12345)},
          {Fixed32(kInt32Max), Fixed32(kInt32Max)},
          {Fixed32(kInt32Min), Fixed32(kInt32Min)},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_BOOL: {
      // Bools should be serialized as 0 for false and 1 for true.  Parsers
      // should also interpret any nonzero value as true.
      // clang-format off
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {Varint(0), Varint(0)},
          {Varint(1), Varint(1)},
          {Varint(-1), Varint(1)},
          {Varint(12345678), Varint(1)},
          {Varint(1LL << 33), Varint(1)},
          {Varint(kInt64Max), Varint(1)},
          {Varint(kInt64Min), Varint(1)},
      };
      // clang-format on
      return *kCases;
    }
    case FieldDescriptor::TYPE_SINT32: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {SInt32(0), SInt32(0)},
          {SInt32(12345), SInt32(12345)},
          {SInt32(kInt32Max), SInt32(kInt32Max)},
          {SInt32(kInt32Min), SInt32(kInt32Min)},
          {SInt64(kInt32Max + 2LL), SInt32(1)},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_SINT64: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {SInt64(0), SInt64(0)},
          {SInt64(12345), SInt64(12345)},
          {SInt64(kInt64Max), SInt64(kInt64Max)},
          {SInt64(kInt64Min), SInt64(kInt64Min)},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_STRING: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {LengthPrefixed(""), LengthPrefixed("")},
          {LengthPrefixed("Hello world!"), LengthPrefixed("Hello world!")},
          {LengthPrefixed("\'\"\?\\\a\b\f\n\r\t\v"),
           LengthPrefixed("\'\"\?\\\a\b\f\n\r\t\v")},  // escape
          // U+8C37 U+6B4C ("Google" in Chinese), as UTF-8.
          {LengthPrefixed("\xE8\xB0\xB7\xE6\xAD\x8C"),
           LengthPrefixed("\xE8\xB0\xB7\xE6\xAD\x8C")},
          // U+1F601 (grinning face with smiling eyes), as UTF-8.
          {LengthPrefixed("\xF0\x9F\x98\x81"),
           LengthPrefixed("\xF0\x9F\x98\x81")},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_BYTES: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {LengthPrefixed(""), LengthPrefixed("")},
          {LengthPrefixed("Hello world!"), LengthPrefixed("Hello world!")},
          {LengthPrefixed("\x01\x02"), LengthPrefixed("\x01\x02")},
          {LengthPrefixed("\xfb"), LengthPrefixed("\xfb")},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_ENUM: {
      // clang-format off
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {Varint(0), Varint(0)},
          {Varint(1), Varint(1)},
          {Varint(2), Varint(2)},
          {Varint(-1), Varint(-1)},
          {Varint(kInt64Max), Varint(-1)},
          {Varint(kInt64Min + 1), Varint(1)},
      };
      // clang-format on
      return *kCases;
    }
    case FieldDescriptor::TYPE_MESSAGE: {
      static const auto* const kCases = new std::vector<ValidDataCase>{
          {LengthPrefixed(""), LengthPrefixed("")},
          {LengthPrefixed(VarintField(1, 1234)),
           LengthPrefixed(VarintField(1, 1234))},
      };
      return *kCases;
    }
    case FieldDescriptor::TYPE_GROUP:
      break;
  }
  ABSL_LOG(FATAL) << "No valid-data cases for field type "
                  << FieldDescriptor::TypeName(type);
}

absl::Span<const MapType> ValidDataMapTypes() {
  static constexpr MapType kTypes[] = {
      {FieldDescriptor::TYPE_INT32, FieldDescriptor::TYPE_INT32},
      {FieldDescriptor::TYPE_INT64, FieldDescriptor::TYPE_INT64},
      {FieldDescriptor::TYPE_UINT32, FieldDescriptor::TYPE_UINT32},
      {FieldDescriptor::TYPE_UINT64, FieldDescriptor::TYPE_UINT64},
      {FieldDescriptor::TYPE_SINT32, FieldDescriptor::TYPE_SINT32},
      {FieldDescriptor::TYPE_SINT64, FieldDescriptor::TYPE_SINT64},
      {FieldDescriptor::TYPE_FIXED32, FieldDescriptor::TYPE_FIXED32},
      {FieldDescriptor::TYPE_FIXED64, FieldDescriptor::TYPE_FIXED64},
      {FieldDescriptor::TYPE_SFIXED32, FieldDescriptor::TYPE_SFIXED32},
      {FieldDescriptor::TYPE_SFIXED64, FieldDescriptor::TYPE_SFIXED64},
      {FieldDescriptor::TYPE_INT32, FieldDescriptor::TYPE_FLOAT},
      {FieldDescriptor::TYPE_INT32, FieldDescriptor::TYPE_DOUBLE},
      {FieldDescriptor::TYPE_BOOL, FieldDescriptor::TYPE_BOOL},
      {FieldDescriptor::TYPE_STRING, FieldDescriptor::TYPE_STRING},
      {FieldDescriptor::TYPE_STRING, FieldDescriptor::TYPE_BYTES},
      {FieldDescriptor::TYPE_STRING, FieldDescriptor::TYPE_ENUM},
      {FieldDescriptor::TYPE_STRING, FieldDescriptor::TYPE_MESSAGE},
  };
  return kTypes;
}

absl::Span<const FieldDescriptor::Type> ValidDataOneofTypes() {
  static constexpr FieldDescriptor::Type kTypes[] = {
      FieldDescriptor::TYPE_UINT32,  FieldDescriptor::TYPE_BOOL,
      FieldDescriptor::TYPE_UINT64,  FieldDescriptor::TYPE_FLOAT,
      FieldDescriptor::TYPE_DOUBLE,  FieldDescriptor::TYPE_STRING,
      FieldDescriptor::TYPE_BYTES,   FieldDescriptor::TYPE_ENUM,
      FieldDescriptor::TYPE_MESSAGE,
  };
  return kTypes;
}

absl::Span<const FieldDescriptor::Type> AllFieldTypesExceptGroup() {
  static constexpr FieldDescriptor::Type kTypes[] = {
      FieldDescriptor::TYPE_DOUBLE,   FieldDescriptor::TYPE_FLOAT,
      FieldDescriptor::TYPE_INT64,    FieldDescriptor::TYPE_UINT64,
      FieldDescriptor::TYPE_INT32,    FieldDescriptor::TYPE_UINT32,
      FieldDescriptor::TYPE_FIXED64,  FieldDescriptor::TYPE_FIXED32,
      FieldDescriptor::TYPE_SFIXED64, FieldDescriptor::TYPE_SFIXED32,
      FieldDescriptor::TYPE_BOOL,     FieldDescriptor::TYPE_SINT32,
      FieldDescriptor::TYPE_SINT64,   FieldDescriptor::TYPE_STRING,
      FieldDescriptor::TYPE_BYTES,    FieldDescriptor::TYPE_ENUM,
      FieldDescriptor::TYPE_MESSAGE,
  };
  return kTypes;
}

std::vector<FieldDescriptor::Type> PackableFieldTypes() {
  std::vector<FieldDescriptor::Type> types;
  for (FieldDescriptor::Type type : AllFieldTypesExceptGroup()) {
    if (FieldDescriptor::IsTypePackable(type)) types.push_back(type);
  }
  return types;
}

std::vector<FieldDescriptor::Type> LengthDelimitedFieldTypes() {
  std::vector<FieldDescriptor::Type> types;
  for (FieldDescriptor::Type type : AllFieldTypesExceptGroup()) {
    if (!FieldDescriptor::IsTypePackable(type)) types.push_back(type);
  }
  return types;
}

std::vector<const Descriptor*> AllTestMessageTypes() {
  return {
      protobuf_test_messages::proto3::TestAllTypesProto3::descriptor(),
      protobuf_test_messages::proto2::TestAllTypesProto2::descriptor(),
      protobuf_test_messages::editions::proto3::TestAllTypesProto3::
          descriptor(),
      protobuf_test_messages::editions::proto2::TestAllTypesProto2::
          descriptor(),
  };
}

std::vector<const Descriptor*> Proto3TestMessageTypes() {
  return {
      protobuf_test_messages::proto3::TestAllTypesProto3::descriptor(),
      protobuf_test_messages::editions::proto3::TestAllTypesProto3::
          descriptor(),
  };
}

std::vector<const Descriptor*> Proto2TestMessageTypes() {
  return {
      protobuf_test_messages::proto2::TestAllTypesProto2::descriptor(),
      protobuf_test_messages::editions::proto2::TestAllTypesProto2::
          descriptor(),
  };
}

std::string ParamName(absl::string_view text) {
  std::string name;
  for (char c : text) {
    if (absl::ascii_isalnum(c)) name.push_back(c);
  }
  ABSL_CHECK(!name.empty())
      << "No letters or digits in parameter name: " << text;
  return name;
}

std::string ParamName(const Descriptor* absl_nonnull message) {
  return GetEditionParamName(*message);
}

std::string ParamName(FieldDescriptor::Type type) {
  return UpperCaseTypeName(type);
}

std::string ParamName(int value) {
  ABSL_CHECK_GE(value, 0) << "Negative parameters have no valid gtest name";
  return absl::StrCat(value);
}
std::string ParamName(const MapType& type) {
  return absl::StrCat(ParamName(type.key), "_", ParamName(type.value));
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
