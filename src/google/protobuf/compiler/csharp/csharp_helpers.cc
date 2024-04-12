// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/csharp/csharp_helpers.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/csharp/csharp_enum_field.h"
#include "google/protobuf/compiler/csharp/csharp_field_base.h"
#include "google/protobuf/compiler/csharp/csharp_map_field.h"
#include "google/protobuf/compiler/csharp/csharp_message_field.h"
#include "google/protobuf/compiler/csharp/csharp_options.h"
#include "google/protobuf/compiler/csharp/csharp_primitive_field.h"
#include "google/protobuf/compiler/csharp/csharp_repeated_enum_field.h"
#include "google/protobuf/compiler/csharp/csharp_repeated_message_field.h"
#include "google/protobuf/compiler/csharp/csharp_repeated_primitive_field.h"
#include "google/protobuf/compiler/csharp/csharp_wrapper_field.h"
#include "google/protobuf/compiler/csharp/names.h"
#include "google/protobuf/descriptor.pb.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

CSharpType GetCSharpType(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32:
      return CSHARPTYPE_INT32;
    case FieldDescriptor::TYPE_INT64:
      return CSHARPTYPE_INT64;
    case FieldDescriptor::TYPE_UINT32:
      return CSHARPTYPE_UINT32;
    case FieldDescriptor::TYPE_UINT64:
      return CSHARPTYPE_UINT32;
    case FieldDescriptor::TYPE_SINT32:
      return CSHARPTYPE_INT32;
    case FieldDescriptor::TYPE_SINT64:
      return CSHARPTYPE_INT64;
    case FieldDescriptor::TYPE_FIXED32:
      return CSHARPTYPE_UINT32;
    case FieldDescriptor::TYPE_FIXED64:
      return CSHARPTYPE_UINT64;
    case FieldDescriptor::TYPE_SFIXED32:
      return CSHARPTYPE_INT32;
    case FieldDescriptor::TYPE_SFIXED64:
      return CSHARPTYPE_INT64;
    case FieldDescriptor::TYPE_FLOAT:
      return CSHARPTYPE_FLOAT;
    case FieldDescriptor::TYPE_DOUBLE:
      return CSHARPTYPE_DOUBLE;
    case FieldDescriptor::TYPE_BOOL:
      return CSHARPTYPE_BOOL;
    case FieldDescriptor::TYPE_ENUM:
      return CSHARPTYPE_ENUM;
    case FieldDescriptor::TYPE_STRING:
      return CSHARPTYPE_STRING;
    case FieldDescriptor::TYPE_BYTES:
      return CSHARPTYPE_BYTESTRING;
    case FieldDescriptor::TYPE_GROUP:
      return CSHARPTYPE_MESSAGE;
    case FieldDescriptor::TYPE_MESSAGE:
      return CSHARPTYPE_MESSAGE;

      // No default because we want the compiler to complain if any new
      // types are added.
  }
  ABSL_LOG(FATAL) << "Can't get here.";
  return (CSharpType) -1;
}

// Convert a string which is expected to be SHOUTY_CASE (but may not be *precisely* shouty)
// into a PascalCase string. Precise rules implemented:

// Previous input character      Current character         Case
// Any                           Non-alphanumeric          Skipped
// None - first char of input    Alphanumeric              Upper
// Non-letter (e.g. _ or 1)      Alphanumeric              Upper
// Numeric                       Alphanumeric              Upper
// Lower letter                  Alphanumeric              Same as current
// Upper letter                  Alphanumeric              Lower
std::string ShoutyToPascalCase(absl::string_view input) {
  std::string result;
  // Simple way of implementing "always start with upper"
  char previous = '_';
  for (int i = 0; i < input.size(); i++) {
    char current = input[i];
    if (!absl::ascii_isalnum(current)) {
      previous = current;
      continue;
    }
    if (!absl::ascii_isalnum(previous)) {
      result += absl::ascii_toupper(current);
    } else if (absl::ascii_isdigit(previous)) {
      result += absl::ascii_toupper(current);
    } else if (absl::ascii_islower(previous)) {
      result += current;
    } else {
      result += absl::ascii_tolower(current);
    }
    previous = current;
  }
  return result;
}

// Attempt to remove a prefix from a value, ignoring casing and skipping underscores.
// (foo, foo_bar) => bar - underscore after prefix is skipped
// (FOO, foo_bar) => bar - casing is ignored
// (foo_bar, foobarbaz) => baz - underscore in prefix is ignored
// (foobar, foo_barbaz) => baz - underscore in value is ignored
// (foo, bar) => bar - prefix isn't matched; return original value
std::string TryRemovePrefix(absl::string_view prefix, absl::string_view value) {
  // First normalize to a lower-case no-underscores prefix to match against
  std::string prefix_to_match = "";
  for (size_t i = 0; i < prefix.size(); i++) {
    if (prefix[i] != '_') {
      prefix_to_match += absl::ascii_tolower(prefix[i]);
    }
  }

  // This keeps track of how much of value we've consumed
  size_t prefix_index, value_index;
  for (prefix_index = 0, value_index = 0;
      prefix_index < prefix_to_match.size() && value_index < value.size();
      value_index++) {
    // Skip over underscores in the value
    if (value[value_index] == '_') {
      continue;
    }
    if (absl::ascii_tolower(value[value_index]) != prefix_to_match[prefix_index++]) {
      // Failed to match the prefix - bail out early.
      return std::string(value);
    }
  }

  // If we didn't finish looking through the prefix, we can't strip it.
  if (prefix_index < prefix_to_match.size()) {
    return std::string(value);
  }

  // Step over any underscores after the prefix
  while (value_index < value.size() && value[value_index] == '_') {
    value_index++;
  }

  // If there's nothing left (e.g. it was a prefix with only underscores afterwards), don't strip.
  if (value_index == value.size()) {
    return std::string(value);
  }

  return std::string(value.substr(value_index));
}

// Format the enum value name in a pleasant way for C#:
// - Strip the enum name as a prefix if possible
// - Convert to PascalCase.
// For example, an enum called Color with a value of COLOR_BLUE should
// result in an enum value in C# called just Blue
std::string GetEnumValueName(absl::string_view enum_name,
                             absl::string_view enum_value_name) {
  std::string stripped = TryRemovePrefix(enum_name, enum_value_name);
  std::string result = ShoutyToPascalCase(stripped);
  // Just in case we have an enum name of FOO and a value of FOO_2... make sure the returned
  // string is a valid identifier.
  if (absl::ascii_isdigit(result[0])) {
    return absl::StrCat("_", result);
  }
  return result;
}

uint GetGroupEndTag(const Descriptor* descriptor) {
  const Descriptor* containing_type = descriptor->containing_type();
  if (containing_type != NULL) {
    const FieldDescriptor* field;
    for (int i = 0; i < containing_type->field_count(); i++) {
      field = containing_type->field(i);
      if (field->type() == FieldDescriptor::Type::TYPE_GROUP &&
          field->message_type() == descriptor) {
        return internal::WireFormatLite::MakeTag(
            field->number(), internal::WireFormatLite::WIRETYPE_END_GROUP);
      }
    }
    for (int i = 0; i < containing_type->extension_count(); i++) {
      field = containing_type->extension(i);
      if (field->type() == FieldDescriptor::Type::TYPE_GROUP &&
          field->message_type() == descriptor) {
        return internal::WireFormatLite::MakeTag(
            field->number(), internal::WireFormatLite::WIRETYPE_END_GROUP);
      }
    }
  } else {
    const FileDescriptor* containing_file = descriptor->file();
    if (containing_file != NULL) {
      const FieldDescriptor* field;
      for (int i = 0; i < containing_file->extension_count(); i++) {
        field = containing_file->extension(i);
        if (field->type() == FieldDescriptor::Type::TYPE_GROUP &&
            field->message_type() == descriptor) {
          return internal::WireFormatLite::MakeTag(
              field->number(), internal::WireFormatLite::WIRETYPE_END_GROUP);
        }
      }
    }
  }

  return 0;
}

std::string GetFullExtensionName(const FieldDescriptor* descriptor) {
  if (descriptor->extension_scope()) {
    return absl::StrCat(GetClassName(descriptor->extension_scope()),
                        ".Extensions.", GetPropertyName(descriptor));
  }

  return absl::StrCat(GetExtensionClassUnqualifiedName(descriptor->file()), ".",
                      GetPropertyName(descriptor));
}

// Groups are hacky:  The name of the field is just the lower-cased name
// of the group type.  In C#, though, we would like to retain the original
// capitalization of the type name.
std::string GetFieldName(const FieldDescriptor* descriptor) {
  if (descriptor->type() == FieldDescriptor::TYPE_GROUP) {
    return descriptor->message_type()->name();
  } else {
    return descriptor->name();
  }
}

std::string GetFieldConstantName(const FieldDescriptor* field) {
  return absl::StrCat(GetPropertyName(field), "FieldNumber");
}

std::string GetPropertyName(const FieldDescriptor* descriptor) {
  // Names of members declared or overridden in the message.
  static const auto& reserved_member_names = *new absl::flat_hash_set<absl::string_view>({
    "Types",
    "Descriptor",
    "Equals",
    "ToString",
    "GetHashCode",
    "WriteTo",
    "Clone",
    "CalculateSize",
    "MergeFrom",
    "OnConstruction",
    "Parser"
    });

  // TODO(jtattermusch): consider introducing csharp_property_name field option
  std::string property_name = UnderscoresToPascalCase(GetFieldName(descriptor));
  // Avoid either our own type name or reserved names.
  // There are various ways of ending up with naming collisions, but we try to avoid obvious
  // ones. In particular, we avoid the names of all the members we generate.
  // Note that we *don't* add an underscore for MemberwiseClone or GetType. Those generate
  // warnings, but not errors; changing the name now could be a breaking change.
  if (property_name == descriptor->containing_type()->name()
      || reserved_member_names.find(property_name) != reserved_member_names.end()) {
    absl::StrAppend(&property_name, "_");
  }
  return property_name;
}

std::string GetOneofCaseName(const FieldDescriptor* descriptor) {
  // The name in a oneof case enum is the same as for the property, but as we always have a "None"
  // value as well, we need to reserve that by appending an underscore.
  std::string property_name = GetPropertyName(descriptor);
  return property_name == "None" ? "None_" : property_name;
}

// TODO: c&p from Java protoc plugin
// For encodings with fixed sizes, returns that size in bytes.  Otherwise
// returns -1.
int GetFixedSize(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32   : return -1;
    case FieldDescriptor::TYPE_INT64   : return -1;
    case FieldDescriptor::TYPE_UINT32  : return -1;
    case FieldDescriptor::TYPE_UINT64  : return -1;
    case FieldDescriptor::TYPE_SINT32  : return -1;
    case FieldDescriptor::TYPE_SINT64  : return -1;
    case FieldDescriptor::TYPE_FIXED32 : return internal::WireFormatLite::kFixed32Size;
    case FieldDescriptor::TYPE_FIXED64 : return internal::WireFormatLite::kFixed64Size;
    case FieldDescriptor::TYPE_SFIXED32: return internal::WireFormatLite::kSFixed32Size;
    case FieldDescriptor::TYPE_SFIXED64: return internal::WireFormatLite::kSFixed64Size;
    case FieldDescriptor::TYPE_FLOAT   : return internal::WireFormatLite::kFloatSize;
    case FieldDescriptor::TYPE_DOUBLE  : return internal::WireFormatLite::kDoubleSize;

    case FieldDescriptor::TYPE_BOOL    : return internal::WireFormatLite::kBoolSize;
    case FieldDescriptor::TYPE_ENUM    : return -1;

    case FieldDescriptor::TYPE_STRING  : return -1;
    case FieldDescriptor::TYPE_BYTES   : return -1;
    case FieldDescriptor::TYPE_GROUP   : return -1;
    case FieldDescriptor::TYPE_MESSAGE : return -1;

    // No default because we want the compiler to complain if any new
    // types are added.
  }
  ABSL_LOG(FATAL) << "Can't get here.";
  return -1;
}

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string StringToBase64(absl::string_view input) {
  std::string result;
  size_t remaining = input.size();
  const unsigned char* src = (const unsigned char*)input.data();
  while (remaining > 2) {
    result += base64_chars[src[0] >> 2];
    result += base64_chars[((src[0] & 0x3) << 4) | (src[1] >> 4)];
    result += base64_chars[((src[1] & 0xf) << 2) | (src[2] >> 6)];
    result += base64_chars[src[2] & 0x3f];
    remaining -= 3;
    src += 3;
  }
  switch (remaining) {
    case 2:
      result += base64_chars[src[0] >> 2];
      result += base64_chars[((src[0] & 0x3) << 4) | (src[1] >> 4)];
      result += base64_chars[(src[1] & 0xf) << 2];
      result += '=';
      src += 2;
      break;
    case 1:
      result += base64_chars[src[0] >> 2];
      result += base64_chars[((src[0] & 0x3) << 4)];
      result += '=';
      result += '=';
      src += 1;
      break;
  }
  return result;
}

std::string FileDescriptorToBase64(const FileDescriptor* descriptor) {
  std::string fdp_bytes;
  FileDescriptorProto fdp;
  descriptor->CopyTo(&fdp);
  fdp.SerializeToString(&fdp_bytes);
  return StringToBase64(fdp_bytes);
}

FieldGeneratorBase* CreateFieldGenerator(const FieldDescriptor* descriptor,
                                         int presenceIndex,
                                         const Options* options) {
  switch (descriptor->type()) {
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
      if (descriptor->is_repeated()) {
        if (descriptor->is_map()) {
          return new MapFieldGenerator(descriptor, presenceIndex, options);
        } else {
          return new RepeatedMessageFieldGenerator(descriptor, presenceIndex, options);
        }
      } else {
        if (IsWrapperType(descriptor)) {
          if (descriptor->real_containing_oneof()) {
            return new WrapperOneofFieldGenerator(descriptor, presenceIndex, options);
          } else {
            return new WrapperFieldGenerator(descriptor, presenceIndex, options);
          }
        } else {
          if (descriptor->real_containing_oneof()) {
            return new MessageOneofFieldGenerator(descriptor, presenceIndex, options);
          } else {
            return new MessageFieldGenerator(descriptor, presenceIndex, options);
          }
        }
      }
    case FieldDescriptor::TYPE_ENUM:
      if (descriptor->is_repeated()) {
        return new RepeatedEnumFieldGenerator(descriptor, presenceIndex, options);
      } else {
        if (descriptor->real_containing_oneof()) {
          return new EnumOneofFieldGenerator(descriptor, presenceIndex, options);
        } else {
          return new EnumFieldGenerator(descriptor, presenceIndex, options);
        }
      }
    default:
      if (descriptor->is_repeated()) {
        return new RepeatedPrimitiveFieldGenerator(descriptor, presenceIndex, options);
      } else {
        if (descriptor->real_containing_oneof()) {
          return new PrimitiveOneofFieldGenerator(descriptor, presenceIndex, options);
        } else {
          return new PrimitiveFieldGenerator(descriptor, presenceIndex, options);
        }
      }
  }
}

bool IsNullable(const FieldDescriptor* descriptor) {
  if (descriptor->is_repeated()) {
    return true;
  }

  switch (descriptor->type()) {
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_BOOL:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
      return false;

    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      return true;

    default:
      ABSL_LOG(FATAL) << "Unknown field type.";
      return true;
  }
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
