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

#include <limits>
#include <sstream>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/strutil.h>

#include <google/protobuf/compiler/csharp/csharp_field_base.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_writer.h>

using google::protobuf::internal::scoped_ptr;

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

FieldGeneratorBase::FieldGeneratorBase(const FieldDescriptor* descriptor,
                                       int fieldOrdinal)
    : SourceGeneratorBase(descriptor->file()),
      descriptor_(descriptor),
      fieldOrdinal_(fieldOrdinal) {
}

FieldGeneratorBase::~FieldGeneratorBase() {
}

void FieldGeneratorBase::AddDeprecatedFlag(Writer* writer) {
  if (descriptor_->options().deprecated())
  {
    writer->WriteLine("[global::System.ObsoleteAttribute()]");
  }
}

void FieldGeneratorBase::AddNullCheck(Writer* writer) {
  AddNullCheck(writer, "value");
}

void FieldGeneratorBase::AddNullCheck(Writer* writer, const std::string& name) {
  if (is_nullable_type()) {
    writer->WriteLine("  pb::ThrowHelper.ThrowIfNull($0$, \"$0$\");", name);
  }
}

void FieldGeneratorBase::AddPublicMemberAttributes(Writer* writer) {
  AddDeprecatedFlag(writer);
}

std::string FieldGeneratorBase::property_name() {
  return GetPropertyName(descriptor_);
}

std::string FieldGeneratorBase::name() {
  return UnderscoresToCamelCase(GetFieldName(descriptor_), false);
}

std::string FieldGeneratorBase::type_name() {
  switch (descriptor_->type()) {
    case FieldDescriptor::TYPE_ENUM:
      return GetClassName(descriptor_->enum_type());
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_GROUP:
      return GetClassName(descriptor_->message_type());
    case FieldDescriptor::TYPE_DOUBLE:
      return "double";
    case FieldDescriptor::TYPE_FLOAT:
      return "float";
    case FieldDescriptor::TYPE_INT64:
      return "long";
    case FieldDescriptor::TYPE_UINT64:
      return "ulong";
    case FieldDescriptor::TYPE_INT32:
      return "int";
    case FieldDescriptor::TYPE_FIXED64:
      return "ulong";
    case FieldDescriptor::TYPE_FIXED32:
      return "uint";
    case FieldDescriptor::TYPE_BOOL:
      return "bool";
    case FieldDescriptor::TYPE_STRING:
      return "string";
    case FieldDescriptor::TYPE_BYTES:
      return "pb::ByteString";
    case FieldDescriptor::TYPE_UINT32:
      return "uint";
    case FieldDescriptor::TYPE_SFIXED32:
      return "int";
    case FieldDescriptor::TYPE_SFIXED64:
      return "long";
    case FieldDescriptor::TYPE_SINT32:
      return "int";
    case FieldDescriptor::TYPE_SINT64:
      return "long";
    default:
      GOOGLE_LOG(FATAL)<< "Unknown field type.";
      return "";
  }
}

bool FieldGeneratorBase::has_default_value() {
  switch (descriptor_->type()) {
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_GROUP:
      return true;
    case FieldDescriptor::TYPE_DOUBLE:
      return descriptor_->default_value_double() != 0.0;
    case FieldDescriptor::TYPE_FLOAT:
      return descriptor_->default_value_float() != 0.0;
    case FieldDescriptor::TYPE_INT64:
      return descriptor_->default_value_int64() != 0L;
    case FieldDescriptor::TYPE_UINT64:
      return descriptor_->default_value_uint64() != 0L;
    case FieldDescriptor::TYPE_INT32:
      return descriptor_->default_value_int32() != 0;
    case FieldDescriptor::TYPE_FIXED64:
      return descriptor_->default_value_uint64() != 0L;
    case FieldDescriptor::TYPE_FIXED32:
      return descriptor_->default_value_uint32() != 0;
    case FieldDescriptor::TYPE_BOOL:
      return descriptor_->default_value_bool();
    case FieldDescriptor::TYPE_STRING:
      return true;
    case FieldDescriptor::TYPE_BYTES:
      return true;
    case FieldDescriptor::TYPE_UINT32:
      return descriptor_->default_value_uint32() != 0;
    case FieldDescriptor::TYPE_SFIXED32:
      return descriptor_->default_value_int32() != 0;
    case FieldDescriptor::TYPE_SFIXED64:
      return descriptor_->default_value_int64() != 0L;
    case FieldDescriptor::TYPE_SINT32:
      return descriptor_->default_value_int32() != 0;
    case FieldDescriptor::TYPE_SINT64:
      return descriptor_->default_value_int64() != 0L;
    default:
      GOOGLE_LOG(FATAL)<< "Unknown field type.";
      return true;
  }
}

bool FieldGeneratorBase::is_nullable_type() {
  switch (descriptor_->type()) {
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
      GOOGLE_LOG(FATAL)<< "Unknown field type.";
      return true;
  }
}

inline bool IsNaN(double value) {
  // NaN is never equal to anything, even itself.
  return value != value;
}

bool AllPrintableAscii(const std::string& text) {
  for(int i = 0; i < text.size(); i++) {
    if (text[i] < 0x20 || text[i] > 0x7e) {
      return false;
    }
  }
  return true;
}

std::string FieldGeneratorBase::GetStringDefaultValueInternal() {
  if (!descriptor_->has_default_value()) {
    return "\"\"";
  }
  if (AllPrintableAscii(descriptor_->default_value_string())) {
    // All chars are ASCII and printable.  In this case we only
    // need to escape quotes and backslashes.
    std::string temp = descriptor_->default_value_string();
    temp = StringReplace(temp, "\\", "\\\\", true);
    temp = StringReplace(temp, "'", "\\'", true);
    temp = StringReplace(temp, "\"", "\\\"", true);
    return "\"" + temp + "\"";
  }
  if (use_lite_runtime()) {
    return "pb::ByteString.FromBase64(\""
        + StringToBase64(descriptor_->default_value_string())
        + "\").ToStringUtf8()";
  }
  return "(string) " + GetClassName(descriptor_->containing_type())
      + ".Descriptor.Fields[" + SimpleItoa(descriptor_->index())
      + "].DefaultValue";
}

std::string FieldGeneratorBase::GetBytesDefaultValueInternal() {
  if (!descriptor_->has_default_value()) {
    return "pb::ByteString.Empty";
  }
  if (use_lite_runtime()) {
    return "pb::ByteString.FromBase64(\"" + StringToBase64(descriptor_->default_value_string()) + "\")";
  }
  return "(pb::ByteString) "+ GetClassName(descriptor_->containing_type()) +
      ".Descriptor.Fields[" + SimpleItoa(descriptor_->index()) + "].DefaultValue";
}

std::string FieldGeneratorBase::default_value() {
  switch (descriptor_->type()) {
    case FieldDescriptor::TYPE_ENUM:
      return type_name() + "." + descriptor_->default_value_enum()->name();
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_GROUP:
      return type_name() + ".DefaultInstance";
    case FieldDescriptor::TYPE_DOUBLE: {
      double value = descriptor_->default_value_double();
      if (value == numeric_limits<double>::infinity()) {
        return "double.PositiveInfinity";
      } else if (value == -numeric_limits<double>::infinity()) {
        return "double.NegativeInfinity";
      } else if (IsNaN(value)) {
        return "double.NaN";
      }
      return SimpleDtoa(value) + "D";
    }
    case FieldDescriptor::TYPE_FLOAT: {
      float value = descriptor_->default_value_float();
      if (value == numeric_limits<float>::infinity()) {
        return "float.PositiveInfinity";
      } else if (value == -numeric_limits<float>::infinity()) {
        return "float.NegativeInfinity";
      } else if (IsNaN(value)) {
        return "float.NaN";
      }
      return SimpleFtoa(value) + "F";
    }
    case FieldDescriptor::TYPE_INT64:
      return SimpleItoa(descriptor_->default_value_int64()) + "L";
    case FieldDescriptor::TYPE_UINT64:
      return SimpleItoa(descriptor_->default_value_uint64()) + "UL";
    case FieldDescriptor::TYPE_INT32:
      return SimpleItoa(descriptor_->default_value_int32());
    case FieldDescriptor::TYPE_FIXED64:
      return SimpleItoa(descriptor_->default_value_uint64()) + "UL";
    case FieldDescriptor::TYPE_FIXED32:
      return SimpleItoa(descriptor_->default_value_uint32());
    case FieldDescriptor::TYPE_BOOL:
      if (descriptor_->default_value_bool()) {
        return "true";
      } else {
        return "false";
      }
    case FieldDescriptor::TYPE_STRING:
      return GetStringDefaultValueInternal();
    case FieldDescriptor::TYPE_BYTES:
      return GetBytesDefaultValueInternal();
    case FieldDescriptor::TYPE_UINT32:
      return SimpleItoa(descriptor_->default_value_uint32());
    case FieldDescriptor::TYPE_SFIXED32:
      return SimpleItoa(descriptor_->default_value_int32());
    case FieldDescriptor::TYPE_SFIXED64:
      return SimpleItoa(descriptor_->default_value_int64()) + "L";
    case FieldDescriptor::TYPE_SINT32:
      return SimpleItoa(descriptor_->default_value_int32());
    case FieldDescriptor::TYPE_SINT64:
      return SimpleItoa(descriptor_->default_value_int64()) + "L";
    default:
      GOOGLE_LOG(FATAL)<< "Unknown field type.";
      return "";
  }
}

std::string FieldGeneratorBase::number() {
  return SimpleItoa(descriptor_->number());
}

std::string FieldGeneratorBase::message_or_group() {
  return
      (descriptor_->type() == FieldDescriptor::TYPE_GROUP) ? "Group" : "Message";
}

std::string FieldGeneratorBase::capitalized_type_name() {
  switch (descriptor_->type()) {
    case FieldDescriptor::TYPE_ENUM:
      return "Enum";
    case FieldDescriptor::TYPE_MESSAGE:
      return "Message";
    case FieldDescriptor::TYPE_GROUP:
      return "Group";
    case FieldDescriptor::TYPE_DOUBLE:
      return "Double";
    case FieldDescriptor::TYPE_FLOAT:
      return "Float";
    case FieldDescriptor::TYPE_INT64:
      return "Int64";
    case FieldDescriptor::TYPE_UINT64:
      return "UInt64";
    case FieldDescriptor::TYPE_INT32:
      return "Int32";
    case FieldDescriptor::TYPE_FIXED64:
      return "Fixed64";
    case FieldDescriptor::TYPE_FIXED32:
      return "Fixed32";
    case FieldDescriptor::TYPE_BOOL:
      return "Bool";
    case FieldDescriptor::TYPE_STRING:
      return "String";
    case FieldDescriptor::TYPE_BYTES:
      return "Bytes";
    case FieldDescriptor::TYPE_UINT32:
      return "UInt32";
    case FieldDescriptor::TYPE_SFIXED32:
      return "SFixed32";
    case FieldDescriptor::TYPE_SFIXED64:
      return "SFixed64";
    case FieldDescriptor::TYPE_SINT32:
      return "SInt32";
    case FieldDescriptor::TYPE_SINT64:
      return "SInt64";
    default:
      GOOGLE_LOG(FATAL)<< "Unknown field type.";
      return "";
  }
}

std::string FieldGeneratorBase::field_ordinal() {
  return SimpleItoa(fieldOrdinal_);
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
