// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/binary_test_util.h"

#include <string>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
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

absl::Span<const FieldDescriptor::Type> PackableFieldTypes() {
  static constexpr FieldDescriptor::Type kTypes[] = {
      FieldDescriptor::TYPE_DOUBLE,   FieldDescriptor::TYPE_FLOAT,
      FieldDescriptor::TYPE_INT64,    FieldDescriptor::TYPE_UINT64,
      FieldDescriptor::TYPE_INT32,    FieldDescriptor::TYPE_UINT32,
      FieldDescriptor::TYPE_FIXED64,  FieldDescriptor::TYPE_FIXED32,
      FieldDescriptor::TYPE_SFIXED64, FieldDescriptor::TYPE_SFIXED32,
      FieldDescriptor::TYPE_BOOL,     FieldDescriptor::TYPE_SINT32,
      FieldDescriptor::TYPE_SINT64,   FieldDescriptor::TYPE_ENUM,
  };
  return kTypes;
}

absl::Span<const FieldDescriptor::Type> LengthDelimitedFieldTypes() {
  static constexpr FieldDescriptor::Type kTypes[] = {
      FieldDescriptor::TYPE_STRING,
      FieldDescriptor::TYPE_BYTES,
      FieldDescriptor::TYPE_MESSAGE,
  };
  return kTypes;
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
  // The edition identifier used in test names ("Editions_Proto2") only differs
  // from a valid gtest parameter name by its underscore.
  return ParamName(GetEditionIdentifier(*message));
}

std::string ParamName(FieldDescriptor::Type type) {
  return UpperCaseTypeName(type);
}

std::string ParamName(int value) {
  ABSL_CHECK_GE(value, 0) << "Negative parameters have no valid gtest name";
  return absl::StrCat(value);
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
