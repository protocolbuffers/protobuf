// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/util/type_resolver_util.h"

#include <string>
#include <vector>

#include "google/protobuf/source_context.pb.h"
#include "google/protobuf/type.pb.h"
#include "google/protobuf/wrappers.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/io/strtod.h"
#include "google/protobuf/util/type_resolver.h"

// clang-format off
#include "google/protobuf/port_def.inc"
// clang-format on

namespace google {
namespace protobuf {
namespace util {
namespace {
using google::protobuf::Any;
using google::protobuf::BoolValue;
using google::protobuf::BytesValue;
using google::protobuf::DoubleValue;
using google::protobuf::Enum;
using google::protobuf::EnumValue;
using google::protobuf::Field;
using google::protobuf::FloatValue;
using google::protobuf::Int32Value;
using google::protobuf::Int64Value;
using google::protobuf::Option;
using google::protobuf::StringValue;
using google::protobuf::Syntax;
using google::protobuf::Type;
using google::protobuf::UInt32Value;
using google::protobuf::UInt64Value;

template <typename WrapperT, typename T>
static WrapperT WrapValue(T value) {
  WrapperT wrapper;
  wrapper.set_value(value);
  return wrapper;
}

void ConvertOptionField(const Reflection* reflection, const Message& options,
                        const FieldDescriptor* field, int index, Option* out) {
  out->set_name(field->is_extension() ? field->full_name() : field->name());
  Any* value = out->mutable_value();
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE:
      value->PackFrom(
          field->is_repeated()
              ? reflection->GetRepeatedMessage(options, field, index)
              : reflection->GetMessage(options, field));
      return;
    case FieldDescriptor::CPPTYPE_DOUBLE:
      value->PackFrom(WrapValue<DoubleValue>(
          field->is_repeated()
              ? reflection->GetRepeatedDouble(options, field, index)
              : reflection->GetDouble(options, field)));
      return;
    case FieldDescriptor::CPPTYPE_FLOAT:
      value->PackFrom(WrapValue<FloatValue>(
          field->is_repeated()
              ? reflection->GetRepeatedFloat(options, field, index)
              : reflection->GetFloat(options, field)));
      return;
    case FieldDescriptor::CPPTYPE_INT64:
      value->PackFrom(WrapValue<Int64Value>(
          field->is_repeated()
              ? reflection->GetRepeatedInt64(options, field, index)
              : reflection->GetInt64(options, field)));
      return;
    case FieldDescriptor::CPPTYPE_UINT64:
      value->PackFrom(WrapValue<UInt64Value>(
          field->is_repeated()
              ? reflection->GetRepeatedUInt64(options, field, index)
              : reflection->GetUInt64(options, field)));
      return;
    case FieldDescriptor::CPPTYPE_INT32:
      value->PackFrom(WrapValue<Int32Value>(
          field->is_repeated()
              ? reflection->GetRepeatedInt32(options, field, index)
              : reflection->GetInt32(options, field)));
      return;
    case FieldDescriptor::CPPTYPE_UINT32:
      value->PackFrom(WrapValue<UInt32Value>(
          field->is_repeated()
              ? reflection->GetRepeatedUInt32(options, field, index)
              : reflection->GetUInt32(options, field)));
      return;
    case FieldDescriptor::CPPTYPE_BOOL:
      value->PackFrom(WrapValue<BoolValue>(
          field->is_repeated()
              ? reflection->GetRepeatedBool(options, field, index)
              : reflection->GetBool(options, field)));
      return;
    case FieldDescriptor::CPPTYPE_STRING: {
      const std::string& val =
          field->is_repeated()
              ? reflection->GetRepeatedString(options, field, index)
              : reflection->GetString(options, field);
      if (field->type() == FieldDescriptor::TYPE_STRING) {
        value->PackFrom(WrapValue<StringValue>(val));
      } else {
        value->PackFrom(WrapValue<BytesValue>(val));
      }
      return;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      const EnumValueDescriptor* val =
          field->is_repeated()
              ? reflection->GetRepeatedEnum(options, field, index)
              : reflection->GetEnum(options, field);
      value->PackFrom(WrapValue<Int32Value>(val->number()));
      return;
    }
  }
}

// Implementation details for Convert*Options.
void ConvertOptionsInternal(const Message& options,
                            RepeatedPtrField<Option>& output) {
  const Reflection* reflection = options.GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(options, &fields);
  for (const FieldDescriptor* field : fields) {
    if (field->is_repeated()) {
      const int size = reflection->FieldSize(options, field);
      for (int i = 0; i < size; ++i) {
        ConvertOptionField(reflection, options, field, i, output.Add());
      }
    } else {
      ConvertOptionField(reflection, options, field, -1, output.Add());
    }
  }
}

void ConvertMessageOptions(const MessageOptions& options,
                           RepeatedPtrField<Option>& output) {
  return ConvertOptionsInternal(options, output);
}

void ConvertFieldOptions(const FieldOptions& options,
                         RepeatedPtrField<Option>& output) {
  return ConvertOptionsInternal(options, output);
}

void ConvertEnumOptions(const EnumOptions& options,
                        RepeatedPtrField<Option>& output) {
  return ConvertOptionsInternal(options, output);
}

void ConvertEnumValueOptions(const EnumValueOptions& options,
                             RepeatedPtrField<Option>& output) {
  return ConvertOptionsInternal(options, output);
}

std::string DefaultValueAsString(const FieldDescriptor& descriptor) {
  switch (descriptor.cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return absl::StrCat(descriptor.default_value_int32());
      break;
    case FieldDescriptor::CPPTYPE_INT64:
      return absl::StrCat(descriptor.default_value_int64());
      break;
    case FieldDescriptor::CPPTYPE_UINT32:
      return absl::StrCat(descriptor.default_value_uint32());
      break;
    case FieldDescriptor::CPPTYPE_UINT64:
      return absl::StrCat(descriptor.default_value_uint64());
      break;
    case FieldDescriptor::CPPTYPE_FLOAT:
      return io::SimpleFtoa(descriptor.default_value_float());
      break;
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return io::SimpleDtoa(descriptor.default_value_double());
      break;
    case FieldDescriptor::CPPTYPE_BOOL:
      return descriptor.default_value_bool() ? "true" : "false";
      break;
    case FieldDescriptor::CPPTYPE_STRING:
      if (descriptor.type() == FieldDescriptor::TYPE_BYTES) {
        return absl::CEscape(descriptor.default_value_string());
      } else {
        return descriptor.default_value_string();
      }
      break;
    case FieldDescriptor::CPPTYPE_ENUM:
      return descriptor.default_value_enum()->name();
      break;
    case FieldDescriptor::CPPTYPE_MESSAGE:
      ABSL_DLOG(FATAL) << "Messages can't have default values!";
      break;
  }
  return "";
}

template <typename T>
std::string GetTypeUrl(absl::string_view url_prefix, const T& descriptor) {
  return absl::StrCat(url_prefix, "/", descriptor.full_name());
}

void ConvertFieldDescriptor(absl::string_view url_prefix,
                            const FieldDescriptor& descriptor, Field* field) {
  field->set_kind(static_cast<Field::Kind>(descriptor.type()));
  switch (descriptor.label()) {
    case FieldDescriptor::LABEL_OPTIONAL:
      field->set_cardinality(Field::CARDINALITY_OPTIONAL);
      break;
    case FieldDescriptor::LABEL_REPEATED:
      field->set_cardinality(Field::CARDINALITY_REPEATED);
      break;
    case FieldDescriptor::LABEL_REQUIRED:
      field->set_cardinality(Field::CARDINALITY_REQUIRED);
      break;
  }
  field->set_number(descriptor.number());
  field->set_name(descriptor.name());
  field->set_json_name(descriptor.json_name());
  if (descriptor.has_default_value()) {
    field->set_default_value(DefaultValueAsString(descriptor));
  }
  if (descriptor.type() == FieldDescriptor::TYPE_MESSAGE ||
      descriptor.type() == FieldDescriptor::TYPE_GROUP) {
    field->set_type_url(GetTypeUrl(url_prefix, *descriptor.message_type()));
  } else if (descriptor.type() == FieldDescriptor::TYPE_ENUM) {
    field->set_type_url(GetTypeUrl(url_prefix, *descriptor.enum_type()));
  }
  if (descriptor.containing_oneof() != nullptr) {
    field->set_oneof_index(descriptor.containing_oneof()->index() + 1);
  }
  if (descriptor.is_packed()) {
    field->set_packed(true);
  }

  ConvertFieldOptions(descriptor.options(), *field->mutable_options());
}

Syntax ConvertSyntax(Edition edition) {
  if (edition >= Edition::EDITION_2023) {
    return Syntax::SYNTAX_EDITIONS;
  }
  // TODO This should propagate proto3 as expected.
  return Syntax::SYNTAX_PROTO2;
}

void ConvertEnumDescriptor(const EnumDescriptor& descriptor, Enum* enum_type) {
  enum_type->Clear();
  enum_type->set_syntax(ConvertSyntax(descriptor.file()->edition()));

  enum_type->set_name(descriptor.full_name());
  enum_type->mutable_source_context()->set_file_name(descriptor.file()->name());
  for (int i = 0; i < descriptor.value_count(); ++i) {
    const EnumValueDescriptor& value_descriptor = *descriptor.value(i);
    EnumValue* value = enum_type->mutable_enumvalue()->Add();
    value->set_name(value_descriptor.name());
    value->set_number(value_descriptor.number());

    ConvertEnumValueOptions(value_descriptor.options(),
                            *value->mutable_options());
  }

  ConvertEnumOptions(descriptor.options(), *enum_type->mutable_options());
}

void ConvertDescriptor(absl::string_view url_prefix,
                       const Descriptor& descriptor, Type* type) {
  type->Clear();
  type->set_name(descriptor.full_name());
  type->set_syntax(ConvertSyntax(descriptor.file()->edition()));
  for (int i = 0; i < descriptor.field_count(); ++i) {
    ConvertFieldDescriptor(url_prefix, *descriptor.field(i),
                           type->add_fields());
  }
  for (int i = 0; i < descriptor.oneof_decl_count(); ++i) {
    type->add_oneofs(descriptor.oneof_decl(i)->name());
  }
  type->mutable_source_context()->set_file_name(descriptor.file()->name());
  ConvertMessageOptions(descriptor.options(), *type->mutable_options());
}

class DescriptorPoolTypeResolver : public TypeResolver {
 public:
  DescriptorPoolTypeResolver(absl::string_view url_prefix,
                             const DescriptorPool* pool)
      : url_prefix_(url_prefix), pool_(pool) {}

  absl::Status ResolveMessageType(const std::string& type_url,
                                  Type* type) override {
    std::string type_name;
    absl::Status status = ParseTypeUrl(type_url, &type_name);
    if (!status.ok()) {
      return status;
    }

    const Descriptor* descriptor = pool_->FindMessageTypeByName(type_name);
    if (descriptor == nullptr) {
      return absl::NotFoundError(
          absl::StrCat("Invalid type URL, unknown type: ", type_name));
    }
    ConvertDescriptor(url_prefix_, *descriptor, type);
    return absl::Status();
  }

  absl::Status ResolveEnumType(const std::string& type_url,
                               Enum* enum_type) override {
    std::string type_name;
    absl::Status status = ParseTypeUrl(type_url, &type_name);
    if (!status.ok()) {
      return status;
    }

    const EnumDescriptor* descriptor = pool_->FindEnumTypeByName(type_name);
    if (descriptor == nullptr) {
      return absl::InvalidArgumentError(
          absl::StrCat("Invalid type URL, unknown type: ", type_name));
    }
    ConvertEnumDescriptor(*descriptor, enum_type);
    return absl::Status();
  }

 private:
  absl::Status ParseTypeUrl(absl::string_view type_url,
                            std::string* type_name) {
    absl::string_view stripped = type_url;
    if (!absl::ConsumePrefix(&stripped, url_prefix_) ||
        !absl::ConsumePrefix(&stripped, "/")) {
      return absl::InvalidArgumentError(
          absl::StrCat("Invalid type URL, type URLs must be of the form '",
                       url_prefix_, "/<typename>', got: ", type_url));
    }
    *type_name = std::string(stripped);
    return absl::Status();
  }

  std::string url_prefix_;
  const DescriptorPool* pool_;
};

}  // namespace

TypeResolver* NewTypeResolverForDescriptorPool(absl::string_view url_prefix,
                                               const DescriptorPool* pool) {
  return new DescriptorPoolTypeResolver(url_prefix, pool);
}

// Performs a direct conversion from a descriptor to a type proto.
Type ConvertDescriptorToType(absl::string_view url_prefix,
                             const Descriptor& descriptor) {
  Type type;
  ConvertDescriptor(url_prefix, descriptor, &type);
  return type;
}

// Performs a direct conversion from an enum descriptor to a type proto.
Enum ConvertDescriptorToType(const EnumDescriptor& descriptor) {
  Enum enum_type;
  ConvertEnumDescriptor(descriptor, &enum_type);
  return enum_type;
}

}  // namespace util
}  // namespace protobuf
}  // namespace google
