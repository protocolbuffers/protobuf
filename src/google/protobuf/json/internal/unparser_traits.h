// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_JSON_INTERNAL_UNPARSER_TRAITS_H__
#define GOOGLE_PROTOBUF_JSON_INTERNAL_UNPARSER_TRAITS_H__

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/type.pb.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/escaping.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/json/internal/descriptor_traits.h"
#include "google/protobuf/json/internal/untyped_message.h"
#include "google/protobuf/map_field.h"
#include "google/protobuf/stubs/status_macros.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace json_internal {
// The type traits in this file provide describe how to read from protobuf
// representation used by the JSON API, either via proto reflection or via
// something ad-hoc for type.proto.

// Helper alias templates to avoid needing to write `typename` in function
// signatures.
template <typename Traits>
using Msg = typename Traits::Msg;

// Traits for proto2-ish deserialization.
struct UnparseProto2Descriptor : Proto2Descriptor {
  // A message value that fields can be read from.
  using Msg = Message;

  static const Desc& GetDesc(const Msg& msg) { return *msg.GetDescriptor(); }

  // Appends extension fields to `fields`.
  static void FindAndAppendExtensions(const Msg& msg,
                                      std::vector<Field>& fields) {
    // Note that it is *not* correct to use ListFields for getting a list of
    // fields to write, because the way that JSON decides to print non-extension
    // fields is slightly subtle. That logic is handled elsewhere; we're only
    // here to get extensions.
    std::vector<Field> all_fields;
    msg.GetReflection()->ListFields(msg, &all_fields);

    for (Field field : all_fields) {
      if (field->is_extension()) {
        fields.push_back(field);
      }
    }
  }

  static size_t GetSize(Field f, const Msg& msg) {
    if (f->is_repeated()) {
      return msg.GetReflection()->FieldSize(msg, f);
    } else {
      return msg.GetReflection()->HasField(msg, f) ? 1 : 0;
    }
  }

  static absl::StatusOr<float> GetFloat(Field f) {
    return f->default_value_float();
  }

  static absl::StatusOr<double> GetDouble(Field f) {
    return f->default_value_double();
  }

  static absl::StatusOr<int32_t> GetInt32(Field f) {
    return f->default_value_int32();
  }

  static absl::StatusOr<uint32_t> GetUInt32(Field f) {
    return f->default_value_uint32();
  }

  static absl::StatusOr<int64_t> GetInt64(Field f) {
    return f->default_value_int64();
  }

  static absl::StatusOr<uint64_t> GetUInt64(Field f) {
    return f->default_value_uint64();
  }

  static absl::StatusOr<bool> GetBool(Field f) {
    return f->default_value_bool();
  }

  static absl::StatusOr<int32_t> GetEnumValue(Field f) {
    return f->default_value_enum()->number();
  }

  static absl::StatusOr<absl::string_view> GetString(Field f,
                                                     std::string& scratch) {
    return f->default_value_string();
  }

  static absl::StatusOr<const Msg*> GetMessage(Field f) {
    return absl::InternalError("message fields cannot have defaults");
  }

  static absl::StatusOr<float> GetFloat(Field f, const Msg& msg) {
    return msg.GetReflection()->GetFloat(msg, f);
  }

  static absl::StatusOr<double> GetDouble(Field f, const Msg& msg) {
    return msg.GetReflection()->GetDouble(msg, f);
  }

  static absl::StatusOr<int32_t> GetInt32(Field f, const Msg& msg) {
    return msg.GetReflection()->GetInt32(msg, f);
  }

  static absl::StatusOr<uint32_t> GetUInt32(Field f, const Msg& msg) {
    return msg.GetReflection()->GetUInt32(msg, f);
  }

  static absl::StatusOr<int64_t> GetInt64(Field f, const Msg& msg) {
    return msg.GetReflection()->GetInt64(msg, f);
  }

  static absl::StatusOr<uint64_t> GetUInt64(Field f, const Msg& msg) {
    return msg.GetReflection()->GetUInt64(msg, f);
  }

  static absl::StatusOr<bool> GetBool(Field f, const Msg& msg) {
    return msg.GetReflection()->GetBool(msg, f);
  }

  static absl::StatusOr<int32_t> GetEnumValue(Field f, const Msg& msg) {
    return msg.GetReflection()->GetEnumValue(msg, f);
  }

  static absl::StatusOr<absl::string_view> GetString(Field f,
                                                     std::string& scratch,
                                                     const Msg& msg) {
    return msg.GetReflection()->GetStringReference(msg, f, &scratch);
  }

  static absl::StatusOr<const Msg*> GetMessage(Field f, const Msg& msg) {
    return &msg.GetReflection()->GetMessage(msg, f);
  }

  static absl::StatusOr<float> GetFloat(Field f, const Msg& msg, size_t idx) {
    return msg.GetReflection()->GetRepeatedFloat(msg, f, idx);
  }

  static absl::StatusOr<double> GetDouble(Field f, const Msg& msg, size_t idx) {
    return msg.GetReflection()->GetRepeatedDouble(msg, f, idx);
  }

  static absl::StatusOr<int32_t> GetInt32(Field f, const Msg& msg, size_t idx) {
    return msg.GetReflection()->GetRepeatedInt32(msg, f, idx);
  }

  static absl::StatusOr<uint32_t> GetUInt32(Field f, const Msg& msg,
                                            size_t idx) {
    return msg.GetReflection()->GetRepeatedUInt32(msg, f, idx);
  }

  static absl::StatusOr<int64_t> GetInt64(Field f, const Msg& msg, size_t idx) {
    return msg.GetReflection()->GetRepeatedInt64(msg, f, idx);
  }

  static absl::StatusOr<uint64_t> GetUInt64(Field f, const Msg& msg,
                                            size_t idx) {
    return msg.GetReflection()->GetRepeatedUInt64(msg, f, idx);
  }

  static absl::StatusOr<bool> GetBool(Field f, const Msg& msg, size_t idx) {
    return msg.GetReflection()->GetRepeatedBool(msg, f, idx);
  }

  static absl::StatusOr<int32_t> GetEnumValue(Field f, const Msg& msg,
                                              size_t idx) {
    return msg.GetReflection()->GetRepeatedEnumValue(msg, f, idx);
  }

  static absl::StatusOr<absl::string_view> GetString(Field f,
                                                     std::string& scratch,
                                                     const Msg& msg,
                                                     size_t idx) {
    return msg.GetReflection()->GetRepeatedStringReference(msg, f, idx,
                                                           &scratch);
  }

  static absl::StatusOr<const Msg*> GetMessage(Field f, const Msg& msg,
                                               size_t idx) {
    // Make sure we don't use the wrong reflection API for maps.
    // If the repeated field is not valid, we don't want to create it.
    ABSL_DCHECK(
        !IsMap(f) ||
        msg.GetReflection()->GetMapData(msg, f)->IsRepeatedFieldValid());
    return &msg.GetReflection()->GetRepeatedMessage(msg, f, idx);
  }

  // MapKey
  static absl::StatusOr<int64_t> GetInt64(Field, const MapKey& key) {
    return key.GetInt64Value();
  }
  static absl::StatusOr<uint64_t> GetUInt64(Field, const MapKey& key) {
    return key.GetUInt64Value();
  }
  static absl::StatusOr<int32_t> GetInt32(Field, const MapKey& key) {
    return key.GetInt32Value();
  }
  static absl::StatusOr<uint32_t> GetUInt32(Field, const MapKey& key) {
    return key.GetUInt32Value();
  }
  static absl::StatusOr<bool> GetBool(Field, const MapKey& key) {
    return key.GetBoolValue();
  }
  static absl::StatusOr<absl::string_view> GetString(Field, std::string&,
                                                     const MapKey& key) {
    return key.GetStringValue();
  }
  static absl::StatusOr<int32_t> GetEnumValue(Field, const MapKey& key) {
    ABSL_LOG(FATAL) << "Unsupported.";
  }

  // MapValueConstRef
  static absl::StatusOr<int64_t> GetInt64(Field, const MapValueConstRef& ref) {
    return ref.GetInt64Value();
  }
  static absl::StatusOr<uint64_t> GetUInt64(Field,
                                            const MapValueConstRef& ref) {
    return ref.GetUInt64Value();
  }
  static absl::StatusOr<int32_t> GetInt32(Field, const MapValueConstRef& ref) {
    return ref.GetInt32Value();
  }
  static absl::StatusOr<uint32_t> GetUInt32(Field,
                                            const MapValueConstRef& ref) {
    return ref.GetUInt32Value();
  }
  static absl::StatusOr<bool> GetBool(Field, const MapValueConstRef& ref) {
    return ref.GetBoolValue();
  }
  static absl::StatusOr<int> GetEnumValue(Field, const MapValueConstRef& ref) {
    return ref.GetEnumValue();
  }
  static absl::StatusOr<absl::string_view> GetString(
      Field, std::string&, const MapValueConstRef& ref) {
    return ref.GetStringValue();
  }
  static absl::StatusOr<float> GetFloat(Field, const MapValueConstRef& ref) {
    return ref.GetFloatValue();
  }
  static absl::StatusOr<double> GetDouble(Field, const MapValueConstRef& ref) {
    return ref.GetDoubleValue();
  }

  static absl::StatusOr<const Msg*> GetMessage(Field,
                                               const MapValueConstRef& ref) {
    return &ref.GetMessageValue();
  }

  template <typename F>
  static absl::Status WithDecodedMessage(const Desc& desc,
                                         absl::string_view data, F body) {
    DynamicMessageFactory factory;
    std::unique_ptr<Message> unerased(factory.GetPrototype(&desc)->New());
    // TODO: Remove this suppression.
    (void)unerased->ParsePartialFromString(data);

    // Explicitly create a const reference, so that we do not accidentally pass
    // a mutable reference to `body`.
    const Msg& ref = *unerased;
    return body(ref);
  }

  static bool MapFieldUseMapReflection(Field f, const Msg& msg) {
    ABSL_DCHECK(IsMap(f));
    return !msg.GetReflection()->GetMapData(msg, f)->IsRepeatedFieldValid();
  }

  template <typename Func>
  static absl::Status ForEachMapEntry(Field f, const Msg& msg, Func func) {
    ABSL_DCHECK(MapFieldUseMapReflection(f, msg));
    const auto& type = *f->message_type();
    for (auto it = msg.GetReflection()->ConstMapBegin(&msg, f),
              end = msg.GetReflection()->ConstMapEnd(&msg, f);
         it != end; ++it) {
      RETURN_IF_ERROR(func(it, type));
    }
    return absl::OkStatus();
  }
};

struct UnparseProto3Type : Proto3Type {
  using Msg = UntypedMessage;

  static const Desc& GetDesc(const Msg& msg) { return msg.desc(); }

  static void FindAndAppendExtensions(const Msg&, std::vector<Field>&) {
    // type.proto does not support extensions.
  }

  static size_t GetSize(Field f, const Msg& msg) {
    return msg.Count(f->proto().number());
  }

  static absl::StatusOr<float> GetFloat(Field f) {
    if (f->proto().default_value().empty()) {
      return 0.0;
    }
    float x;
    if (!absl::SimpleAtof(f->proto().default_value(), &x)) {
      return absl::InternalError(absl::StrCat(
          "bad default value in type.proto: ", f->parent().proto().name()));
    }
    return x;
  }

  static absl::StatusOr<double> GetDouble(Field f) {
    if (f->proto().default_value().empty()) {
      return 0.0;
    }
    double x;
    if (!absl::SimpleAtod(f->proto().default_value(), &x)) {
      return absl::InternalError(absl::StrCat(
          "bad default value in type.proto: ", f->parent().proto().name()));
    }
    return x;
  }

  static absl::StatusOr<int32_t> GetInt32(Field f) {
    if (f->proto().default_value().empty()) {
      return 0;
    }
    int32_t x;
    if (!absl::SimpleAtoi(f->proto().default_value(), &x)) {
      return absl::InternalError(absl::StrCat(
          "bad default value in type.proto: ", f->parent().proto().name()));
    }
    return x;
  }

  static absl::StatusOr<uint32_t> GetUInt32(Field f) {
    if (f->proto().default_value().empty()) {
      return 0;
    }
    uint32_t x;
    if (!absl::SimpleAtoi(f->proto().default_value(), &x)) {
      return absl::InternalError(absl::StrCat(
          "bad default value in type.proto: ", f->parent().proto().name()));
    }
    return x;
  }

  static absl::StatusOr<int64_t> GetInt64(Field f) {
    if (f->proto().default_value().empty()) {
      return 0;
    }
    int64_t x;
    if (!absl::SimpleAtoi(f->proto().default_value(), &x)) {
      return absl::InternalError(absl::StrCat(
          "bad default value in type.proto: ", f->parent().proto().name()));
    }
    return x;
  }

  static absl::StatusOr<uint64_t> GetUInt64(Field f) {
    if (f->proto().default_value().empty()) {
      return 0;
    }
    uint64_t x;
    if (!absl::SimpleAtoi(f->proto().default_value(), &x)) {
      return absl::InternalError(absl::StrCat(
          "bad default value in type.proto: ", f->parent().proto().name()));
    }
    return x;
  }

  static absl::StatusOr<bool> GetBool(Field f) {
    if (f->proto().default_value().empty()) {
      return false;
    } else if (f->proto().default_value() == "false") {
      return false;
    } else if (f->proto().default_value() == "true") {
      return true;
    } else {
      return absl::InternalError(absl::StrCat(
          "bad default value in type.proto: ", f->parent().proto().name()));
    }
  }

  static absl::StatusOr<int32_t> GetEnumValue(Field f) {
    if (f->proto().default_value().empty()) {
      auto e = f->EnumType();
      RETURN_IF_ERROR(e.status());

      return (**e).proto().enumvalue(0).number();
    }
    return EnumNumberByName(f, f->proto().default_value(),
                            /*case_insensitive=*/false);
  }

  static absl::StatusOr<absl::string_view> GetString(Field f,
                                                     std::string& scratch) {
    absl::CUnescape(f->proto().default_value(), &scratch);
    return scratch;
  }

  static absl::StatusOr<const Msg*> GetMessage(Field f) {
    return absl::InternalError("message fields cannot have defaults");
  }

  static absl::StatusOr<float> GetFloat(Field f, const Msg& msg) {
    auto span = msg.Get<float>(f->proto().number());
    if (span.empty()) {
      return GetFloat(f);
    }
    if (span.size() > 1) {
      return absl::InternalError(absl::StrCat(
          "Attempted to read a non-repeated field with multiple values: ",
          f->parent().proto().name()));
    }
    return span[0];
  }

  static absl::StatusOr<float> GetFloat(Field f, const Msg& msg, size_t idx) {
    return msg.Get<float>(f->proto().number())[idx];
  }

  static absl::StatusOr<double> GetDouble(Field f, const Msg& msg) {
    auto span = msg.Get<double>(f->proto().number());
    if (span.empty()) {
      return GetDouble(f);
    }
    if (span.size() > 1) {
      return absl::InternalError(absl::StrCat(
          "Attempted to read a non-repeated field with multiple values: ",
          f->parent().proto().name()));
    }
    return span[0];
  }

  static absl::StatusOr<double> GetDouble(Field f, const Msg& msg, size_t idx) {
    return msg.Get<double>(f->proto().number())[idx];
  }

  static absl::StatusOr<int32_t> GetInt32(Field f, const Msg& msg) {
    auto span = msg.Get<int32_t>(f->proto().number());
    if (span.empty()) {
      return GetInt32(f);
    }
    if (span.size() > 1) {
      return absl::InternalError(absl::StrCat(
          "Attempted to read a non-repeated field with multiple values: ",
          f->parent().proto().name()));
    }
    return span[0];
  }

  static absl::StatusOr<int32_t> GetInt32(Field f, const Msg& msg, size_t idx) {
    return msg.Get<int32_t>(f->proto().number())[idx];
  }

  static absl::StatusOr<uint32_t> GetUInt32(Field f, const Msg& msg) {
    auto span = msg.Get<uint32_t>(f->proto().number());
    if (span.empty()) {
      return GetUInt32(f);
    }
    if (span.size() > 1) {
      return absl::InternalError(absl::StrCat(
          "Attempted to read a non-repeated field with multiple values: ",
          f->parent().proto().name()));
    }
    return span[0];
  }

  static absl::StatusOr<uint32_t> GetUInt32(Field f, const Msg& msg,
                                            size_t idx) {
    return msg.Get<uint32_t>(f->proto().number())[idx];
  }

  static absl::StatusOr<int64_t> GetInt64(Field f, const Msg& msg) {
    auto span = msg.Get<int64_t>(f->proto().number());
    if (span.empty()) {
      return GetInt64(f);
    }
    if (span.size() > 1) {
      return absl::InternalError(absl::StrCat(
          "Attempted to read a non-repeated field with multiple values: ",
          f->parent().proto().name()));
    }
    return span[0];
  }

  static absl::StatusOr<int64_t> GetInt64(Field f, const Msg& msg, size_t idx) {
    return msg.Get<int64_t>(f->proto().number())[idx];
  }

  static absl::StatusOr<uint64_t> GetUInt64(Field f, const Msg& msg) {
    auto span = msg.Get<uint64_t>(f->proto().number());
    if (span.empty()) {
      return GetUInt64(f);
    }
    if (span.size() > 1) {
      return absl::InternalError(absl::StrCat(
          "Attempted to read a non-repeated field with multiple values: ",
          f->parent().proto().name()));
    }
    return span[0];
  }

  static absl::StatusOr<uint64_t> GetUInt64(Field f, const Msg& msg,
                                            size_t idx) {
    return msg.Get<uint64_t>(f->proto().number())[idx];
  }

  static absl::StatusOr<bool> GetBool(Field f, const Msg& msg) {
    auto span = msg.Get<Msg::Bool>(f->proto().number());
    if (span.empty()) {
      return GetBool(f);
    }
    if (span.size() > 1) {
      return absl::InternalError(absl::StrCat(
          "Attempted to read a non-repeated field with multiple values: ",
          f->parent().proto().name()));
    }
    return span[0] == Msg::kTrue;
  }

  static absl::StatusOr<bool> GetBool(Field f, const Msg& msg, size_t idx) {
    return msg.Get<Msg::Bool>(f->proto().number())[idx] == Msg::kTrue;
  }

  static absl::StatusOr<int32_t> GetEnumValue(Field f, const Msg& msg) {
    auto span = msg.Get<int32_t>(f->proto().number());
    if (span.empty()) {
      return GetEnumValue(f);
    }
    if (span.size() > 1) {
      return absl::InternalError(absl::StrCat(
          "Attempted to read a non-repeated field with multiple values: ",
          f->parent().proto().name()));
    }
    return span[0];
  }

  static absl::StatusOr<int32_t> GetEnumValue(Field f, const Msg& msg,
                                              size_t idx) {
    return msg.Get<int32_t>(f->proto().number())[idx];
  }

  static absl::StatusOr<absl::string_view> GetString(Field f,
                                                     std::string& scratch,
                                                     const Msg& msg) {
    auto span = msg.Get<std::string>(f->proto().number());
    if (span.empty()) {
      return GetString(f, scratch);
    }
    if (span.size() > 1) {
      return absl::InternalError(absl::StrCat(
          "Attempted to read a non-repeated field with multiple values: ",
          f->parent().proto().name()));
    }
    return span[0];
  }

  static absl::StatusOr<absl::string_view> GetString(Field f,
                                                     std::string& scratch,
                                                     const Msg& msg,
                                                     size_t idx) {
    return msg.Get<std::string>(f->proto().number())[idx];
  }

  static absl::StatusOr<const Msg*> GetMessage(Field f, const Msg& msg) {
    auto span = msg.Get<Msg>(f->proto().number());
    if (span.empty()) {
      return GetMessage(f);
    }
    if (span.size() > 1) {
      return absl::InternalError(absl::StrCat(
          "Attempted to read a non-repeated field with multiple values: ",
          f->parent().proto().name()));
    }
    return &span[0];
  }

  static absl::StatusOr<const Msg*> GetMessage(Field f, const Msg& msg,
                                               size_t idx) {
    return &msg.Get<Msg>(f->proto().number())[idx];
  }

  template <typename F>
  static absl::Status WithDecodedMessage(const Desc& desc,
                                         absl::string_view data, F body) {
    io::CodedInputStream stream(reinterpret_cast<const uint8_t*>(data.data()),
                                data.size());
    auto unerased = Msg::ParseFromStream(&desc, stream);
    RETURN_IF_ERROR(unerased.status());

    // Explicitly create a const reference, so that we do not accidentally pass
    // a mutable reference to `body`.
    const Msg& ref = *unerased;
    return body(ref);
  }

  static bool MapFieldUseMapReflection(Field, const Msg&) { return false; }

  template <typename Func>
  static absl::Status ForEachMapEntry(Field f, const Msg& msg, Func func) {
    ABSL_DCHECK(false);
    return absl::InternalError("Unsupported.");
  }
};
}  // namespace json_internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
#endif  // GOOGLE_PROTOBUF_JSON_INTERNAL_UNPARSER_TRAITS_H__
