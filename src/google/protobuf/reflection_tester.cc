// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#include "google/protobuf/reflection_tester.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "absl/functional/overload.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/map_field.h"
#include "google/protobuf/message.h"

// Must include last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

MapReflectionTester::MapReflectionTester(const Descriptor* base_descriptor)
    : base_descriptor_(base_descriptor) {
  const DescriptorPool* pool = base_descriptor->file()->pool();
  const absl::string_view package = base_descriptor->file()->package();

  map_enum_foo_ =
      pool->FindEnumValueByName(absl::StrCat(package, ".MAP_ENUM_FOO"));
  map_enum_bar_ =
      pool->FindEnumValueByName(absl::StrCat(package, ".MAP_ENUM_BAR"));
  map_enum_baz_ =
      pool->FindEnumValueByName(absl::StrCat(package, ".MAP_ENUM_BAZ"));

  foreign_c_ =
      pool->FindFieldByName(absl::StrCat(package, ".ForeignMessage.c"));
  map_int32_int32_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt32Int32Entry.key"));
  map_int32_int32_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt32Int32Entry.value"));
  map_int64_int64_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt64Int64Entry.key"));
  map_int64_int64_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt64Int64Entry.value"));
  map_uint32_uint32_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapUint32Uint32Entry.key"));
  map_uint32_uint32_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapUint32Uint32Entry.value"));
  map_uint64_uint64_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapUint64Uint64Entry.key"));
  map_uint64_uint64_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapUint64Uint64Entry.value"));
  map_sint32_sint32_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapSint32Sint32Entry.key"));
  map_sint32_sint32_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapSint32Sint32Entry.value"));
  map_sint64_sint64_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapSint64Sint64Entry.key"));
  map_sint64_sint64_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapSint64Sint64Entry.value"));
  map_fixed32_fixed32_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapFixed32Fixed32Entry.key"));
  map_fixed32_fixed32_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapFixed32Fixed32Entry.value"));
  map_fixed64_fixed64_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapFixed64Fixed64Entry.key"));
  map_fixed64_fixed64_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapFixed64Fixed64Entry.value"));
  map_sfixed32_sfixed32_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapSfixed32Sfixed32Entry.key"));
  map_sfixed32_sfixed32_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapSfixed32Sfixed32Entry.value"));
  map_sfixed64_sfixed64_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapSfixed64Sfixed64Entry.key"));
  map_sfixed64_sfixed64_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapSfixed64Sfixed64Entry.value"));
  map_int32_float_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt32FloatEntry.key"));
  map_int32_float_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt32FloatEntry.value"));
  map_int32_double_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt32DoubleEntry.key"));
  map_int32_double_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt32DoubleEntry.value"));
  map_bool_bool_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapBoolBoolEntry.key"));
  map_bool_bool_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapBoolBoolEntry.value"));
  map_string_string_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapStringStringEntry.key"));
  map_string_string_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapStringStringEntry.value"));
  map_int32_bytes_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt32BytesEntry.key"));
  map_int32_bytes_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt32BytesEntry.value"));
  map_int32_enum_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt32EnumEntry.key"));
  map_int32_enum_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt32EnumEntry.value"));
  map_int32_foreign_message_key_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt32ForeignMessageEntry.key"));
  map_int32_foreign_message_val_ = pool->FindFieldByName(
      absl::StrCat(package, ".TestMap.MapInt32ForeignMessageEntry.value"));

  EXPECT_FALSE(map_enum_foo_ == nullptr);
  EXPECT_FALSE(map_enum_bar_ == nullptr);
  EXPECT_FALSE(map_enum_baz_ == nullptr);
  EXPECT_FALSE(map_int32_int32_key_ == nullptr);
  EXPECT_FALSE(map_int32_int32_val_ == nullptr);
  EXPECT_FALSE(map_int64_int64_key_ == nullptr);
  EXPECT_FALSE(map_int64_int64_val_ == nullptr);
  EXPECT_FALSE(map_uint32_uint32_key_ == nullptr);
  EXPECT_FALSE(map_uint32_uint32_val_ == nullptr);
  EXPECT_FALSE(map_uint64_uint64_key_ == nullptr);
  EXPECT_FALSE(map_uint64_uint64_val_ == nullptr);
  EXPECT_FALSE(map_sint32_sint32_key_ == nullptr);
  EXPECT_FALSE(map_sint32_sint32_val_ == nullptr);
  EXPECT_FALSE(map_sint64_sint64_key_ == nullptr);
  EXPECT_FALSE(map_sint64_sint64_val_ == nullptr);
  EXPECT_FALSE(map_fixed32_fixed32_key_ == nullptr);
  EXPECT_FALSE(map_fixed32_fixed32_val_ == nullptr);
  EXPECT_FALSE(map_fixed64_fixed64_key_ == nullptr);
  EXPECT_FALSE(map_fixed64_fixed64_val_ == nullptr);
  EXPECT_FALSE(map_sfixed32_sfixed32_key_ == nullptr);
  EXPECT_FALSE(map_sfixed32_sfixed32_val_ == nullptr);
  EXPECT_FALSE(map_sfixed64_sfixed64_key_ == nullptr);
  EXPECT_FALSE(map_sfixed64_sfixed64_val_ == nullptr);
  EXPECT_FALSE(map_int32_float_key_ == nullptr);
  EXPECT_FALSE(map_int32_float_val_ == nullptr);
  EXPECT_FALSE(map_int32_double_key_ == nullptr);
  EXPECT_FALSE(map_int32_double_val_ == nullptr);
  EXPECT_FALSE(map_bool_bool_key_ == nullptr);
  EXPECT_FALSE(map_bool_bool_val_ == nullptr);
  EXPECT_FALSE(map_string_string_key_ == nullptr);
  EXPECT_FALSE(map_string_string_val_ == nullptr);
  EXPECT_FALSE(map_int32_bytes_key_ == nullptr);
  EXPECT_FALSE(map_int32_bytes_val_ == nullptr);
  EXPECT_FALSE(map_int32_enum_key_ == nullptr);
  EXPECT_FALSE(map_int32_enum_val_ == nullptr);
  EXPECT_FALSE(map_int32_foreign_message_key_ == nullptr);
  EXPECT_FALSE(map_int32_foreign_message_val_ == nullptr);

  std::vector<const FieldDescriptor*> all_map_descriptors = {
      map_int32_int32_key_,
      map_int32_int32_val_,
      map_int64_int64_key_,
      map_int64_int64_val_,
      map_uint32_uint32_key_,
      map_uint32_uint32_val_,
      map_uint64_uint64_key_,
      map_uint64_uint64_val_,
      map_sint32_sint32_key_,
      map_sint32_sint32_val_,
      map_sint64_sint64_key_,
      map_sint64_sint64_val_,
      map_fixed32_fixed32_key_,
      map_fixed32_fixed32_val_,
      map_fixed64_fixed64_key_,
      map_fixed64_fixed64_val_,
      map_sfixed32_sfixed32_key_,
      map_sfixed32_sfixed32_val_,
      map_sfixed64_sfixed64_key_,
      map_sfixed64_sfixed64_val_,
      map_int32_float_key_,
      map_int32_float_val_,
      map_int32_double_key_,
      map_int32_double_val_,
      map_bool_bool_key_,
      map_bool_bool_val_,
      map_string_string_key_,
      map_string_string_val_,
      map_int32_bytes_key_,
      map_int32_bytes_val_,
      map_int32_enum_key_,
      map_int32_enum_val_,
      map_int32_foreign_message_key_,
      map_int32_foreign_message_val_};
  for (const FieldDescriptor* fdesc : all_map_descriptors) {
    ABSL_CHECK(fdesc->containing_type() != nullptr) << fdesc->name();
    if (fdesc->name() == "key") {
      EXPECT_EQ(fdesc->containing_type()->map_key(), fdesc);
    } else {
      EXPECT_EQ(fdesc->name(), "value");
      EXPECT_EQ(fdesc->containing_type()->map_value(), fdesc);
    }
  }

  // Must be heap allocated.
  EXPECT_NE(long_string().capacity(), std::string().capacity());
  EXPECT_NE(long_string_2().capacity(), std::string().capacity());
}

// Shorthand to get a FieldDescriptor for a field of unittest::TestMap.
const FieldDescriptor* MapReflectionTester::F(absl::string_view name) {
  const FieldDescriptor* result = nullptr;
  result = base_descriptor_->FindFieldByName(name);
  ABSL_CHECK(result != nullptr);
  return result;
}

void MapReflectionTester::SetMapFieldsViaReflection(Message* message) {
  const Reflection* reflection = message->GetReflection();
  Message* sub_message = nullptr;
  Message* sub_foreign_message = nullptr;

  // Add first element.
  sub_message = reflection->AddMessage(message, F("map_int32_int32"));
  sub_message->GetReflection()->SetInt32(sub_message, map_int32_int32_key_, 0);
  sub_message->GetReflection()->SetInt32(sub_message, map_int32_int32_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_int64_int64"));
  sub_message->GetReflection()->SetInt64(sub_message, map_int64_int64_key_, 0);
  sub_message->GetReflection()->SetInt64(sub_message, map_int64_int64_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_uint32_uint32"));
  sub_message->GetReflection()->SetUInt32(sub_message, map_uint32_uint32_key_,
                                          0);
  sub_message->GetReflection()->SetUInt32(sub_message, map_uint32_uint32_val_,
                                          0);

  sub_message = reflection->AddMessage(message, F("map_uint64_uint64"));
  sub_message->GetReflection()->SetUInt64(sub_message, map_uint64_uint64_key_,
                                          0);
  sub_message->GetReflection()->SetUInt64(sub_message, map_uint64_uint64_val_,
                                          0);

  sub_message = reflection->AddMessage(message, F("map_sint32_sint32"));
  sub_message->GetReflection()->SetInt32(sub_message, map_sint32_sint32_key_,
                                         0);
  sub_message->GetReflection()->SetInt32(sub_message, map_sint32_sint32_val_,
                                         0);

  sub_message = reflection->AddMessage(message, F("map_sint64_sint64"));
  sub_message->GetReflection()->SetInt64(sub_message, map_sint64_sint64_key_,
                                         0);
  sub_message->GetReflection()->SetInt64(sub_message, map_sint64_sint64_val_,
                                         0);

  sub_message = reflection->AddMessage(message, F("map_fixed32_fixed32"));
  sub_message->GetReflection()->SetUInt32(sub_message, map_fixed32_fixed32_key_,
                                          0);
  sub_message->GetReflection()->SetUInt32(sub_message, map_fixed32_fixed32_val_,
                                          0);

  sub_message = reflection->AddMessage(message, F("map_fixed64_fixed64"));
  sub_message->GetReflection()->SetUInt64(sub_message, map_fixed64_fixed64_key_,
                                          0);
  sub_message->GetReflection()->SetUInt64(sub_message, map_fixed64_fixed64_val_,
                                          0);

  sub_message = reflection->AddMessage(message, F("map_sfixed32_sfixed32"));
  sub_message->GetReflection()->SetInt32(sub_message,
                                         map_sfixed32_sfixed32_key_, 0);
  sub_message->GetReflection()->SetInt32(sub_message,
                                         map_sfixed32_sfixed32_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_sfixed64_sfixed64"));
  sub_message->GetReflection()->SetInt64(sub_message,
                                         map_sfixed64_sfixed64_key_, 0);
  sub_message->GetReflection()->SetInt64(sub_message,
                                         map_sfixed64_sfixed64_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_int32_float"));
  sub_message->GetReflection()->SetInt32(sub_message, map_int32_float_key_, 0);
  sub_message->GetReflection()->SetFloat(sub_message, map_int32_float_val_,
                                         0.0);

  sub_message = reflection->AddMessage(message, F("map_int32_double"));
  sub_message->GetReflection()->SetInt32(sub_message, map_int32_double_key_, 0);
  sub_message->GetReflection()->SetDouble(sub_message, map_int32_double_val_,
                                          0.0);

  sub_message = reflection->AddMessage(message, F("map_bool_bool"));
  sub_message->GetReflection()->SetBool(sub_message, map_bool_bool_key_, false);
  sub_message->GetReflection()->SetBool(sub_message, map_bool_bool_val_, false);

  sub_message = reflection->AddMessage(message, F("map_string_string"));
  sub_message->GetReflection()->SetString(sub_message, map_string_string_key_,
                                          long_string());
  sub_message->GetReflection()->SetString(sub_message, map_string_string_val_,
                                          long_string());

  sub_message = reflection->AddMessage(message, F("map_int32_bytes"));
  sub_message->GetReflection()->SetInt32(sub_message, map_int32_bytes_key_, 0);
  sub_message->GetReflection()->SetString(sub_message, map_int32_bytes_val_,
                                          long_string());

  sub_message = reflection->AddMessage(message, F("map_int32_enum"));
  sub_message->GetReflection()->SetInt32(sub_message, map_int32_enum_key_, 0);
  sub_message->GetReflection()->SetEnum(sub_message, map_int32_enum_val_,
                                        map_enum_bar_);

  sub_message = reflection->AddMessage(message, F("map_int32_foreign_message"));
  sub_message->GetReflection()->SetInt32(sub_message,
                                         map_int32_foreign_message_key_, 0);
  sub_foreign_message = sub_message->GetReflection()->MutableMessage(
      sub_message, map_int32_foreign_message_val_, nullptr);
  sub_foreign_message->GetReflection()->SetInt32(sub_foreign_message,
                                                 foreign_c_, 0);

  // Add second element
  sub_message = reflection->AddMessage(message, F("map_int32_int32"));
  sub_message->GetReflection()->SetInt32(sub_message, map_int32_int32_key_, 1);
  sub_message->GetReflection()->SetInt32(sub_message, map_int32_int32_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_int64_int64"));
  sub_message->GetReflection()->SetInt64(sub_message, map_int64_int64_key_, 1);
  sub_message->GetReflection()->SetInt64(sub_message, map_int64_int64_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_uint32_uint32"));
  sub_message->GetReflection()->SetUInt32(sub_message, map_uint32_uint32_key_,
                                          1);
  sub_message->GetReflection()->SetUInt32(sub_message, map_uint32_uint32_val_,
                                          1);

  sub_message = reflection->AddMessage(message, F("map_uint64_uint64"));
  sub_message->GetReflection()->SetUInt64(sub_message, map_uint64_uint64_key_,
                                          1);
  sub_message->GetReflection()->SetUInt64(sub_message, map_uint64_uint64_val_,
                                          1);

  sub_message = reflection->AddMessage(message, F("map_sint32_sint32"));
  sub_message->GetReflection()->SetInt32(sub_message, map_sint32_sint32_key_,
                                         1);
  sub_message->GetReflection()->SetInt32(sub_message, map_sint32_sint32_val_,
                                         1);

  sub_message = reflection->AddMessage(message, F("map_sint64_sint64"));
  sub_message->GetReflection()->SetInt64(sub_message, map_sint64_sint64_key_,
                                         1);
  sub_message->GetReflection()->SetInt64(sub_message, map_sint64_sint64_val_,
                                         1);

  sub_message = reflection->AddMessage(message, F("map_fixed32_fixed32"));
  sub_message->GetReflection()->SetUInt32(sub_message, map_fixed32_fixed32_key_,
                                          1);
  sub_message->GetReflection()->SetUInt32(sub_message, map_fixed32_fixed32_val_,
                                          1);

  sub_message = reflection->AddMessage(message, F("map_fixed64_fixed64"));
  sub_message->GetReflection()->SetUInt64(sub_message, map_fixed64_fixed64_key_,
                                          1);
  sub_message->GetReflection()->SetUInt64(sub_message, map_fixed64_fixed64_val_,
                                          1);

  sub_message = reflection->AddMessage(message, F("map_sfixed32_sfixed32"));
  sub_message->GetReflection()->SetInt32(sub_message,
                                         map_sfixed32_sfixed32_key_, 1);
  sub_message->GetReflection()->SetInt32(sub_message,
                                         map_sfixed32_sfixed32_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_sfixed64_sfixed64"));
  sub_message->GetReflection()->SetInt64(sub_message,
                                         map_sfixed64_sfixed64_key_, 1);
  sub_message->GetReflection()->SetInt64(sub_message,
                                         map_sfixed64_sfixed64_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_int32_float"));
  sub_message->GetReflection()->SetInt32(sub_message, map_int32_float_key_, 1);
  sub_message->GetReflection()->SetFloat(sub_message, map_int32_float_val_,
                                         1.0);

  sub_message = reflection->AddMessage(message, F("map_int32_double"));
  sub_message->GetReflection()->SetInt32(sub_message, map_int32_double_key_, 1);
  sub_message->GetReflection()->SetDouble(sub_message, map_int32_double_val_,
                                          1.0);

  sub_message = reflection->AddMessage(message, F("map_bool_bool"));
  sub_message->GetReflection()->SetBool(sub_message, map_bool_bool_key_, true);
  sub_message->GetReflection()->SetBool(sub_message, map_bool_bool_val_, true);

  sub_message = reflection->AddMessage(message, F("map_string_string"));
  sub_message->GetReflection()->SetString(sub_message, map_string_string_key_,
                                          long_string_2());
  sub_message->GetReflection()->SetString(sub_message, map_string_string_val_,
                                          long_string_2());

  sub_message = reflection->AddMessage(message, F("map_int32_bytes"));
  sub_message->GetReflection()->SetInt32(sub_message, map_int32_bytes_key_, 1);
  sub_message->GetReflection()->SetString(sub_message, map_int32_bytes_val_,
                                          long_string_2());

  sub_message = reflection->AddMessage(message, F("map_int32_enum"));
  sub_message->GetReflection()->SetInt32(sub_message, map_int32_enum_key_, 1);
  sub_message->GetReflection()->SetEnum(sub_message, map_int32_enum_val_,
                                        map_enum_baz_);

  sub_message = reflection->AddMessage(message, F("map_int32_foreign_message"));
  sub_message->GetReflection()->SetInt32(sub_message,
                                         map_int32_foreign_message_key_, 1);
  sub_foreign_message = sub_message->GetReflection()->MutableMessage(
      sub_message, map_int32_foreign_message_val_, nullptr);
  sub_foreign_message->GetReflection()->SetInt32(sub_foreign_message,
                                                 foreign_c_, 1);
}

namespace {

constexpr absl::string_view kMapFields[] = {
    "map_int32_int32",
    "map_int64_int64",
    "map_uint32_uint32",
    "map_uint64_uint64",
    "map_sint32_sint32",
    "map_sint64_sint64",
    "map_fixed32_fixed32",
    "map_fixed64_fixed64",
    "map_sfixed32_sfixed32",
    "map_sfixed64_sfixed64",
    "map_int32_float",
    "map_int32_double",
    "map_bool_bool",
    "map_string_string",
    "map_int32_bytes",
    "map_int32_enum",
    "map_int32_foreign_message",
};

struct EnumVal {
  int value = 0;
  friend bool operator==(EnumVal a, EnumVal b) { return a.value == b.value; }
};

using KeyVariant =
    std::variant<int32_t, int64_t, uint32_t, uint64_t, bool, std::string>;
using ValueVariant = std::variant<int32_t, int64_t, uint32_t, uint64_t, float,
                                  double, bool, std::string, EnumVal>;

inline KeyVariant GetKeyVariant(const MapKey& key) {
  switch (key.type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return key.GetInt32Value();
    case FieldDescriptor::CPPTYPE_INT64:
      return key.GetInt64Value();
    case FieldDescriptor::CPPTYPE_UINT32:
      return key.GetUInt32Value();
    case FieldDescriptor::CPPTYPE_UINT64:
      return key.GetUInt64Value();
    case FieldDescriptor::CPPTYPE_BOOL:
      return key.GetBoolValue();
    case FieldDescriptor::CPPTYPE_STRING:
      return std::string(key.GetStringValue());
    default:
      ABSL_LOG(FATAL) << "Unsupported key type: " << key.type();
  }
}

template <typename T>
inline ValueVariant GetValueVariant(const T& ref) {
  switch (ref.type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return ref.GetInt32Value();
    case FieldDescriptor::CPPTYPE_INT64:
      return ref.GetInt64Value();
    case FieldDescriptor::CPPTYPE_UINT32:
      return ref.GetUInt32Value();
    case FieldDescriptor::CPPTYPE_UINT64:
      return ref.GetUInt64Value();
    case FieldDescriptor::CPPTYPE_FLOAT:
      return ref.GetFloatValue();
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return ref.GetDoubleValue();
    case FieldDescriptor::CPPTYPE_BOOL:
      return ref.GetBoolValue();
    case FieldDescriptor::CPPTYPE_STRING:
      return std::string(ref.GetStringValue());
    case FieldDescriptor::CPPTYPE_ENUM:
      return EnumVal{ref.GetEnumValue()};
    default:
      ABSL_LOG(FATAL) << "Unsupported value type: " << ref.type();
  }
}

inline void SetMapKey(MapKey& key, const KeyVariant& val) {
  std::visit(absl::Overload{
                 [&](int32_t v) { key.SetInt32Value(v); },
                 [&](int64_t v) { key.SetInt64Value(v); },
                 [&](uint32_t v) { key.SetUInt32Value(v); },
                 [&](uint64_t v) { key.SetUInt64Value(v); },
                 [&](bool v) { key.SetBoolValue(v); },
                 [&](const std::string& v) { key.SetStringValue(v); },
             },
             val);
}

inline MapKey MakeMapKey(const KeyVariant& val) {
  MapKey key;
  SetMapKey(key, val);
  return key;
}

inline void SetMapValue(MapValueRef ref, const ValueVariant& val) {
  std::visit(absl::Overload{
                 [&](int32_t v) { ref.SetInt32Value(v); },
                 [&](int64_t v) { ref.SetInt64Value(v); },
                 [&](uint32_t v) { ref.SetUInt32Value(v); },
                 [&](uint64_t v) { ref.SetUInt64Value(v); },
                 [&](float v) { ref.SetFloatValue(v); },
                 [&](double v) { ref.SetDoubleValue(v); },
                 [&](bool v) { ref.SetBoolValue(v); },
                 [&](const std::string& v) { ref.SetStringValue(v); },
                 [&](EnumVal v) { ref.SetEnumValue(v.value); },
             },
             val);
}

inline ValueVariant DefaultValueFor(const ValueVariant& val) {
  return std::visit([](const auto& v) -> ValueVariant { return decltype(v){}; },
                    val);
}

void SetMapFieldHelper(GenericMapRef map, const KeyVariant& k0_val,
                       const ValueVariant& val0, const KeyVariant& k1_val,
                       const ValueVariant& val1) {
  MapKey key0 = MakeMapKey(k0_val);
  MapKey key1 = MakeMapKey(k1_val);

  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
  EXPECT_FALSE(map.contains(key0));
  EXPECT_EQ(map.find(key0), map.end());

  auto res0 = map.try_emplace(key0);
  EXPECT_TRUE(res0.second);
  EXPECT_NE(res0.first, map.end());
  EXPECT_EQ(res0.first->key(), key0);
  SetMapValue(res0.first->value(), val0);

  EXPECT_FALSE(map.empty());
  EXPECT_EQ(map.size(), 1);
  EXPECT_TRUE(map.contains(key0));
  EXPECT_NE(map.find(key0), map.end());
  EXPECT_EQ(GetValueVariant(map.find(key0)->value()), val0);
  EXPECT_EQ(GetValueVariant(map.at(key0)), val0);
  EXPECT_EQ(GetValueVariant(map[key0]), val0);

  EXPECT_FALSE(map.contains(key1));

  // Accessing a non-existing key via operator[] inserts a default value.
  EXPECT_EQ(GetValueVariant(map[key1]), DefaultValueFor(val1));
  EXPECT_EQ(map.size(), 2);
  EXPECT_TRUE(map.contains(key1));

  // Set the value for key1.
  SetMapValue(map[key1], val1);

  // Duplicate insertion via try_emplace should return false and not overwrite.
  auto dup = map.try_emplace(key1);
  EXPECT_FALSE(dup.second);
  EXPECT_EQ(GetValueVariant(dup.first->value()), val1);

  EXPECT_EQ(map.size(), 2);
  EXPECT_TRUE(map.contains(key0));
  EXPECT_TRUE(map.contains(key1));
  EXPECT_EQ(GetValueVariant(map.at(key1)), val1);
  EXPECT_EQ(GetValueVariant(map[key1]), val1);

  // Check const handle
  GenericConstMapRef const_map = map;
  EXPECT_EQ(const_map.size(), 2);
  EXPECT_FALSE(const_map.empty());
  EXPECT_TRUE(const_map.contains(key0));
  EXPECT_TRUE(const_map.contains(key1));
  EXPECT_EQ(GetValueVariant(const_map.find(key0)->value()), val0);
  EXPECT_EQ(GetValueVariant(const_map.find(key1)->value()), val1);

  // Clear and populate standard map elements for downstream tests.
  map.clear();
  EXPECT_TRUE(map.empty());

  KeyVariant std_k0, std_k1;
  std::visit(absl::Overload{
                 [&](int32_t) {
                   std_k0 = int32_t{0};
                   std_k1 = int32_t{1};
                 },
                 [&](int64_t) {
                   std_k0 = int64_t{0};
                   std_k1 = int64_t{1};
                 },
                 [&](uint32_t) {
                   std_k0 = uint32_t{0};
                   std_k1 = uint32_t{1};
                 },
                 [&](uint64_t) {
                   std_k0 = uint64_t{0};
                   std_k1 = uint64_t{1};
                 },
                 [&](bool) {
                   std_k0 = false;
                   std_k1 = true;
                 },
                 [&](const std::string&) {
                   std_k0 = MapReflectionTester::long_string();
                   std_k1 = MapReflectionTester::long_string_2();
                 },
             },
             k0_val);

  ValueVariant std_v0, std_v1;
  std::visit(absl::Overload{
                 [&](int32_t) {
                   std_v0 = int32_t{0};
                   std_v1 = int32_t{1};
                 },
                 [&](int64_t) {
                   std_v0 = int64_t{0};
                   std_v1 = int64_t{1};
                 },
                 [&](uint32_t) {
                   std_v0 = uint32_t{0};
                   std_v1 = uint32_t{1};
                 },
                 [&](uint64_t) {
                   std_v0 = uint64_t{0};
                   std_v1 = uint64_t{1};
                 },
                 [&](float) {
                   std_v0 = 0.0f;
                   std_v1 = 1.0f;
                 },
                 [&](double) {
                   std_v0 = 0.0;
                   std_v1 = 1.0;
                 },
                 [&](bool) {
                   std_v0 = false;
                   std_v1 = true;
                 },
                 [&](const std::string&) {
                   std_v0 = MapReflectionTester::long_string();
                   std_v1 = MapReflectionTester::long_string_2();
                 },
                 [&](EnumVal) {
                   std_v0 = val0;
                   std_v1 = val1;
                 },
             },
             val0);

  SetMapValue(map[MakeMapKey(std_k0)], std_v0);
  SetMapValue(map[MakeMapKey(std_k1)], std_v1);
  EXPECT_EQ(map.size(), 2);
}

void ModifyMapFieldHelper(GenericMapRef map, const KeyVariant& k_val,
                          const ValueVariant& new_val) {
  MapKey key = MakeMapKey(k_val);
  EXPECT_EQ(map.size(), 2);
  EXPECT_TRUE(map.contains(key));
  auto dup = map.try_emplace(key);
  EXPECT_FALSE(dup.second);
  SetMapValue(map.at(key), new_val);
  EXPECT_EQ(GetValueVariant(map.at(key)), new_val);
  EXPECT_EQ(map.size(), 2);
}

template <typename MapRef, typename StdMap>
void ExpectMapIteratorHelper(MapRef map_ref, const StdMap& map) {
  using KeyT = typename StdMap::key_type;
  using ValT = typename StdMap::mapped_type;
  EXPECT_EQ(map_ref.size(), map.size());
  EXPECT_FALSE(map_ref.empty());

  size_t count = 0;
  for (auto entry : map_ref) {
    auto key = std::get<KeyT>(GetKeyVariant(entry.key()));
    auto val = std::get<ValT>(GetValueVariant(entry.value()));
    auto it = map.find(key);
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second, val);
    ++count;
  }
  EXPECT_EQ(count, map.size());

  // Iterator prefix/postfix ++ and arrow operator
  auto it = map_ref.begin();
  ASSERT_NE(it, map_ref.end());
  EXPECT_EQ(GetValueVariant((*it).value()), GetValueVariant(it->value()));
  EXPECT_EQ(GetKeyVariant((*it).key()), GetKeyVariant(it->key()));
  auto it_copy = it++;
  EXPECT_NE(it, it_copy);
  EXPECT_EQ(it_copy, map_ref.begin());
}

}  // namespace

void MapReflectionTester::SetMapFieldsViaMapReflection(Message* message) {
  const Reflection* reflection = message->GetReflection();

  SetMapFieldHelper(reflection->MutableMap(message, F("map_int32_int32")),
                    int32_t{10}, int32_t{11}, int32_t{20}, int32_t{21});
  SetMapFieldHelper(reflection->MutableMap(message, F("map_int64_int64")),
                    int64_t{10}, int64_t{11}, int64_t{20}, int64_t{21});
  SetMapFieldHelper(reflection->MutableMap(message, F("map_uint32_uint32")),
                    uint32_t{10}, uint32_t{11}, uint32_t{20}, uint32_t{21});
  SetMapFieldHelper(reflection->MutableMap(message, F("map_uint64_uint64")),
                    uint64_t{10}, uint64_t{11}, uint64_t{20}, uint64_t{21});
  SetMapFieldHelper(reflection->MutableMap(message, F("map_sint32_sint32")),
                    int32_t{10}, int32_t{11}, int32_t{20}, int32_t{21});
  SetMapFieldHelper(reflection->MutableMap(message, F("map_sint64_sint64")),
                    int64_t{10}, int64_t{11}, int64_t{20}, int64_t{21});
  SetMapFieldHelper(reflection->MutableMap(message, F("map_fixed32_fixed32")),
                    uint32_t{10}, uint32_t{11}, uint32_t{20}, uint32_t{21});
  SetMapFieldHelper(reflection->MutableMap(message, F("map_fixed64_fixed64")),
                    uint64_t{10}, uint64_t{11}, uint64_t{20}, uint64_t{21});
  SetMapFieldHelper(reflection->MutableMap(message, F("map_sfixed32_sfixed32")),
                    int32_t{10}, int32_t{11}, int32_t{20}, int32_t{21});
  SetMapFieldHelper(reflection->MutableMap(message, F("map_sfixed64_sfixed64")),
                    int64_t{10}, int64_t{11}, int64_t{20}, int64_t{21});
  SetMapFieldHelper(reflection->MutableMap(message, F("map_int32_float")),
                    int32_t{10}, 11.0f, int32_t{20}, 21.0f);
  SetMapFieldHelper(reflection->MutableMap(message, F("map_int32_double")),
                    int32_t{10}, 11.0, int32_t{20}, 21.0);
  SetMapFieldHelper(reflection->MutableMap(message, F("map_bool_bool")), false,
                    true, true, false);
  SetMapFieldHelper(reflection->MutableMap(message, F("map_string_string")),
                    std::string("key_a"), std::string("val_a"),
                    std::string("key_b"), std::string("val_b"));
  SetMapFieldHelper(reflection->MutableMap(message, F("map_int32_bytes")),
                    int32_t{10}, std::string("bytes_a"), int32_t{20},
                    std::string("bytes_b"));
  SetMapFieldHelper(reflection->MutableMap(message, F("map_int32_enum")),
                    int32_t{10}, EnumVal{map_enum_bar_->number()}, int32_t{20},
                    EnumVal{map_enum_baz_->number()});

  // 17. map_int32_foreign_message
  {
    auto map = reflection->MutableMap(message, F("map_int32_foreign_message"));
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);

    MapKey k0 = MakeMapKey(int32_t{10});
    MapKey k1 = MakeMapKey(int32_t{20});

    EXPECT_FALSE(map.contains(k0));
    EXPECT_EQ(map.find(k0), map.end());

    auto res0 = map.try_emplace(k0);
    EXPECT_TRUE(res0.second);
    EXPECT_NE(res0.first, map.end());
    EXPECT_EQ(res0.first->key(), k0);
    Message* sub0 = res0.first->value().MutableMessageValue();
    sub0->GetReflection()->SetInt32(sub0, foreign_c_, 11);

    EXPECT_FALSE(map.empty());
    EXPECT_EQ(map.size(), 1);
    EXPECT_TRUE(map.contains(k0));
    const Message& read_sub0 = map.find(k0)->value().GetMessageValue();
    EXPECT_EQ(read_sub0.GetReflection()->GetInt32(read_sub0, foreign_c_), 11);
    EXPECT_EQ(map.at(k0).GetMessageValue().GetReflection()->GetInt32(
                  map.at(k0).GetMessageValue(), foreign_c_),
              11);

    EXPECT_FALSE(map.contains(k1));

    // Accessing a non-existing key via operator[] inserts a default message.
    Message* sub1 = map[k1].MutableMessageValue();
    EXPECT_EQ(map.size(), 2);
    EXPECT_TRUE(map.contains(k1));
    EXPECT_EQ(sub1->GetReflection()->GetInt32(*sub1, foreign_c_), 0);

    // Set the value for k1.
    sub1->GetReflection()->SetInt32(sub1, foreign_c_, 21);

    auto dup = map.try_emplace(k1);
    EXPECT_FALSE(dup.second);

    EXPECT_EQ(map.size(), 2);
    EXPECT_TRUE(map.contains(k0));
    EXPECT_TRUE(map.contains(k1));
    EXPECT_EQ(map.at(k1).GetMessageValue().GetReflection()->GetInt32(
                  map.at(k1).GetMessageValue(), foreign_c_),
              21);
    EXPECT_EQ(map[k1].GetMessageValue().GetReflection()->GetInt32(
                  map[k1].GetMessageValue(), foreign_c_),
              21);

    GenericConstMapRef const_map = map;
    EXPECT_EQ(const_map.size(), 2);
    EXPECT_FALSE(const_map.empty());
    EXPECT_TRUE(const_map.contains(k0));
    EXPECT_TRUE(const_map.contains(k1));
    const Message& c_sub0 = const_map.find(k0)->value().GetMessageValue();
    EXPECT_EQ(c_sub0.GetReflection()->GetInt32(c_sub0, foreign_c_), 11);
    const Message& c_sub1 = const_map.find(k1)->value().GetMessageValue();
    EXPECT_EQ(c_sub1.GetReflection()->GetInt32(c_sub1, foreign_c_), 21);

    // Clear and set standard elements (k0=0 with c=0, k1=1 with c=1)
    map.clear();
    EXPECT_TRUE(map.empty());
    MapKey final_k0 = MakeMapKey(int32_t{0});
    MapKey final_k1 = MakeMapKey(int32_t{1});
    Message* final_sub0 = map[final_k0].MutableMessageValue();
    final_sub0->GetReflection()->SetInt32(final_sub0, foreign_c_, 0);
    Message* final_sub1 = map[final_k1].MutableMessageValue();
    final_sub1->GetReflection()->SetInt32(final_sub1, foreign_c_, 1);
    EXPECT_EQ(map.size(), 2);
  }
}

void MapReflectionTester::DeleteMapValueViaMapReflection(
    Message* message, absl::string_view field_name, MapKey map_key) {
  const Reflection* reflection = message->GetReflection();
  auto map = reflection->MutableMap(message, F(field_name));

  // We want to use the key once more after the first erase, so let's make sure
  // string keys outlive the erase.
  std::string str;
  if (map_key.type() == FieldDescriptor::CPPTYPE_STRING) {
    str = std::string(map_key.GetStringValue());
    map_key.SetStringValue(str);
  }

  EXPECT_TRUE(map.erase(map_key));
  EXPECT_FALSE(map.erase(map_key));
}

Message* MapReflectionTester::GetMapEntryViaReflection(
    Message* message, absl::string_view field_name, int index) {
  const Reflection* reflection = message->GetReflection();
  return reflection->MutableRepeatedMessage(message, F(field_name), index);
}

int MapReflectionTester::MapSize(const Message& message,
                                 absl::string_view field_name) {
  return message.GetReflection()->GetMap(message, F(field_name)).size();
}

void MapReflectionTester::ClearMapFieldsViaReflection(Message* message) {
  const Reflection* reflection = message->GetReflection();

  reflection->ClearField(message, F("map_int32_int32"));
  reflection->ClearField(message, F("map_int64_int64"));
  reflection->ClearField(message, F("map_uint32_uint32"));
  reflection->ClearField(message, F("map_uint64_uint64"));
  reflection->ClearField(message, F("map_sint32_sint32"));
  reflection->ClearField(message, F("map_sint64_sint64"));
  reflection->ClearField(message, F("map_fixed32_fixed32"));
  reflection->ClearField(message, F("map_fixed64_fixed64"));
  reflection->ClearField(message, F("map_sfixed32_sfixed32"));
  reflection->ClearField(message, F("map_sfixed64_sfixed64"));
  reflection->ClearField(message, F("map_int32_float"));
  reflection->ClearField(message, F("map_int32_double"));
  reflection->ClearField(message, F("map_bool_bool"));
  reflection->ClearField(message, F("map_string_string"));
  reflection->ClearField(message, F("map_int32_bytes"));
  reflection->ClearField(message, F("map_int32_enum"));
  reflection->ClearField(message, F("map_int32_foreign_message"));
}

void MapReflectionTester::ClearMapFieldsViaMapReflection(Message* message) {
  const Reflection* reflection = message->GetReflection();
  for (absl::string_view name : kMapFields) {
    auto map = reflection->MutableMap(message, F(name));
    map.clear();
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
    EXPECT_EQ(map.begin(), map.end());
  }
}

void MapReflectionTester::ModifyMapFieldsViaReflection(Message* message) {
  const Reflection* reflection = message->GetReflection();

  ModifyMapFieldHelper(reflection->MutableMap(message, F("map_int32_int32")),
                       int32_t{1}, int32_t{2});
  ModifyMapFieldHelper(reflection->MutableMap(message, F("map_int64_int64")),
                       int64_t{1}, int64_t{2});
  ModifyMapFieldHelper(reflection->MutableMap(message, F("map_uint32_uint32")),
                       uint32_t{1}, uint32_t{2});
  ModifyMapFieldHelper(reflection->MutableMap(message, F("map_uint64_uint64")),
                       uint64_t{1}, uint64_t{2});
  ModifyMapFieldHelper(reflection->MutableMap(message, F("map_sint32_sint32")),
                       int32_t{1}, int32_t{2});
  ModifyMapFieldHelper(reflection->MutableMap(message, F("map_sint64_sint64")),
                       int64_t{1}, int64_t{2});
  ModifyMapFieldHelper(
      reflection->MutableMap(message, F("map_fixed32_fixed32")), uint32_t{1},
      uint32_t{2});
  ModifyMapFieldHelper(
      reflection->MutableMap(message, F("map_fixed64_fixed64")), uint64_t{1},
      uint64_t{2});
  ModifyMapFieldHelper(
      reflection->MutableMap(message, F("map_sfixed32_sfixed32")), int32_t{1},
      int32_t{2});
  ModifyMapFieldHelper(
      reflection->MutableMap(message, F("map_sfixed64_sfixed64")), int64_t{1},
      int64_t{2});
  ModifyMapFieldHelper(reflection->MutableMap(message, F("map_int32_float")),
                       int32_t{1}, 2.0f);
  ModifyMapFieldHelper(reflection->MutableMap(message, F("map_int32_double")),
                       int32_t{1}, 2.0);
  ModifyMapFieldHelper(reflection->MutableMap(message, F("map_bool_bool")),
                       true, false);
  ModifyMapFieldHelper(reflection->MutableMap(message, F("map_string_string")),
                       long_string_2(), std::string("2"));
  ModifyMapFieldHelper(reflection->MutableMap(message, F("map_int32_bytes")),
                       int32_t{1}, std::string("2"));
  ModifyMapFieldHelper(reflection->MutableMap(message, F("map_int32_enum")),
                       int32_t{1}, EnumVal{map_enum_foo_->number()});

  {
    MapKey k_int32 = MakeMapKey(int32_t{1});
    auto map = reflection->MutableMap(message, F("map_int32_foreign_message"));
    EXPECT_EQ(map.size(), 2);
    EXPECT_TRUE(map.contains(k_int32));
    auto dup = map.try_emplace(k_int32);
    EXPECT_FALSE(dup.second);
    Message* sub = map.at(k_int32).MutableMessageValue();
    sub->GetReflection()->SetInt32(sub, foreign_c_, 2);
    EXPECT_EQ(sub->GetReflection()->GetInt32(map.at(k_int32).GetMessageValue(),
                                             foreign_c_),
              2);
    EXPECT_EQ(map.size(), 2);
  }
}

void MapReflectionTester::RemoveLastMapsViaReflection(Message* message) {
  const Reflection* reflection = message->GetReflection();

  std::vector<const FieldDescriptor*> output;
  reflection->ListFields(*message, &output);
  for (size_t i = 0; i < output.size(); ++i) {
    const FieldDescriptor* field = output[i];
    if (!field->is_repeated()) continue;
    reflection->RemoveLast(message, field);
  }
}

void MapReflectionTester::ReleaseLastMapsViaReflection(Message* message) {
  const Reflection* reflection = message->GetReflection();

  std::vector<const FieldDescriptor*> output;
  reflection->ListFields(*message, &output);
  for (size_t i = 0; i < output.size(); ++i) {
    const FieldDescriptor* field = output[i];
    if (!field->is_repeated()) continue;
    if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) continue;

    Message* released = reflection->ReleaseLast(message, field);
    ASSERT_TRUE(released != nullptr)
        << "ReleaseLast returned nullptr for: " << field->name();
    delete released;
  }
}

void MapReflectionTester::SwapMapsViaReflection(Message* message) {
  const Reflection* reflection = message->GetReflection();
  std::vector<const FieldDescriptor*> output;
  reflection->ListFields(*message, &output);
  for (size_t i = 0; i < output.size(); ++i) {
    const FieldDescriptor* field = output[i];
    if (!field->is_repeated()) continue;
    reflection->SwapElements(message, field, 0, 1);
  }
}

void MapReflectionTester::SwapMapsViaMapReflection(Message* message1,
                                                   Message* message2) {
  const Reflection* reflection1 = message1->GetReflection();
  const Reflection* reflection2 = message2->GetReflection();
  for (absl::string_view name : kMapFields) {
    auto map1 = reflection1->MutableMap(message1, F(name));
    auto map2 = reflection2->MutableMap(message2, F(name));
    map1.swap(map2);
  }
}

void MapReflectionTester::AssignMapsViaMapReflection(Message* dst,
                                                     Message* src) {
  const Reflection* dst_reflection = dst->GetReflection();
  const Reflection* src_reflection = src->GetReflection();
  for (absl::string_view name : kMapFields) {
    auto dst_map = dst_reflection->MutableMap(dst, F(name));
    auto src_map = src_reflection->MutableMap(src, F(name));
    dst_map.assign(src_map);
  }
}

void MapReflectionTester::EraseMapValuesViaMapReflectionIterator(
    Message* message) {
  const Reflection* reflection = message->GetReflection();
  for (absl::string_view name : kMapFields) {
    auto map = reflection->MutableMap(message, F(name));
    while (!map.empty()) {
      size_t old_size = map.size();
      auto it = map.begin();
      EXPECT_TRUE(map.erase(it));
      EXPECT_EQ(map.size(), old_size - 1);
    }
  }
}

void MapReflectionTester::MutableUnknownFieldsOfMapFieldsViaReflection(
    Message* message) {
  const Reflection* reflection = message->GetReflection();
  Message* sub_message = nullptr;

  sub_message = reflection->AddMessage(message, F("map_int32_int32"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_int64_int64"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_uint32_uint32"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_uint64_uint64"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_sint32_sint32"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_sint64_sint64"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_fixed32_fixed32"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_fixed64_fixed64"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_sfixed32_sfixed32"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_sfixed64_sfixed64"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_int32_float"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_int32_double"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_bool_bool"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_string_string"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_int32_bytes"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_int32_enum"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
  sub_message = reflection->AddMessage(message, F("map_int32_foreign_message"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              nullptr);
}

void MapReflectionTester::ExpectMapFieldsSetViaReflection(
    const Message& message) {
  const Reflection* reflection = message.GetReflection();
  const Message* sub_message;
  MapKey map_key;

  for (auto f : kMapFields) {
    ASSERT_EQ(2, reflection->FieldSize(message, F(f))) << f;
    ASSERT_EQ(2, reflection->GetMap(message, F(f)).size()) << f;
  }

  {
    absl::flat_hash_map<int32_t, int32_t> map = {{0, 0}, {1, 1}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_int32_int32")) {
        sub_message =
            &reflection->GetRepeatedMessage(message, F("map_int32_int32"), i);
        int32_t key = sub_message->GetReflection()->GetInt32(
            *sub_message, map_int32_int32_key_);
        int32_t val = sub_message->GetReflection()->GetInt32(
            *sub_message, map_int32_int32_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetInt32Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_int32_int32"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetInt32Value(), i);
        EXPECT_EQ(it->value().GetInt32Value(), map[i]);
      }
    }
  }
  {
    absl::flat_hash_map<int64_t, int64_t> map = {{0, 0}, {1, 1}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_int64_int64")) {
        sub_message =
            &reflection->GetRepeatedMessage(message, F("map_int64_int64"), i);
        int64_t key = sub_message->GetReflection()->GetInt64(
            *sub_message, map_int64_int64_key_);
        int64_t val = sub_message->GetReflection()->GetInt64(
            *sub_message, map_int64_int64_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetInt64Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_int64_int64"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetInt64Value(), i);
        EXPECT_EQ(it->value().GetInt64Value(), map[i]);
      }
    }
  }
  {
    absl::flat_hash_map<uint32_t, uint32_t> map = {{0, 0}, {1, 1}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_uint32_uint32")) {
        sub_message =
            &reflection->GetRepeatedMessage(message, F("map_uint32_uint32"), i);
        uint32_t key = sub_message->GetReflection()->GetUInt32(
            *sub_message, map_uint32_uint32_key_);
        uint32_t val = sub_message->GetReflection()->GetUInt32(
            *sub_message, map_uint32_uint32_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetUInt32Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_uint32_uint32"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetUInt32Value(), i);
        EXPECT_EQ(it->value().GetUInt32Value(), map[i]);
      }
    }
  }
  {
    absl::flat_hash_map<uint64_t, uint64_t> map = {{0, 0}, {1, 1}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_uint64_uint64")) {
        sub_message =
            &reflection->GetRepeatedMessage(message, F("map_uint64_uint64"), i);
        uint64_t key = sub_message->GetReflection()->GetUInt64(
            *sub_message, map_uint64_uint64_key_);
        uint64_t val = sub_message->GetReflection()->GetUInt64(
            *sub_message, map_uint64_uint64_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetUInt64Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_uint64_uint64"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetUInt64Value(), i);
        EXPECT_EQ(it->value().GetUInt64Value(), map[i]);
      }
    }
  }
  {
    absl::flat_hash_map<int32_t, int32_t> map = {{0, 0}, {1, 1}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_sint32_sint32")) {
        sub_message =
            &reflection->GetRepeatedMessage(message, F("map_sint32_sint32"), i);
        int32_t key = sub_message->GetReflection()->GetInt32(
            *sub_message, map_sint32_sint32_key_);
        int32_t val = sub_message->GetReflection()->GetInt32(
            *sub_message, map_sint32_sint32_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetInt32Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_sint32_sint32"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetInt32Value(), i);
        EXPECT_EQ(it->value().GetInt32Value(), map[i]);
      }
    }
  }
  {
    absl::flat_hash_map<int64_t, int64_t> map = {{0, 0}, {1, 1}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_sint64_sint64")) {
        sub_message =
            &reflection->GetRepeatedMessage(message, F("map_sint64_sint64"), i);
        int64_t key = sub_message->GetReflection()->GetInt64(
            *sub_message, map_sint64_sint64_key_);
        int64_t val = sub_message->GetReflection()->GetInt64(
            *sub_message, map_sint64_sint64_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetInt64Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_sint64_sint64"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetInt64Value(), i);
        EXPECT_EQ(it->value().GetInt64Value(), map[i]);
      }
    }
  }
  {
    absl::flat_hash_map<uint32_t, uint32_t> map = {{0, 0}, {1, 1}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_fixed32_fixed32")) {
        sub_message = &reflection->GetRepeatedMessage(
            message, F("map_fixed32_fixed32"), i);
        uint32_t key = sub_message->GetReflection()->GetUInt32(
            *sub_message, map_fixed32_fixed32_key_);
        uint32_t val = sub_message->GetReflection()->GetUInt32(
            *sub_message, map_fixed32_fixed32_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetUInt32Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_fixed32_fixed32"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetUInt32Value(), i);
        EXPECT_EQ(it->value().GetUInt32Value(), map[i]);
      }
    }
  }
  {
    absl::flat_hash_map<uint64_t, uint64_t> map = {{0, 0}, {1, 1}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_fixed64_fixed64")) {
        sub_message = &reflection->GetRepeatedMessage(
            message, F("map_fixed64_fixed64"), i);
        uint64_t key = sub_message->GetReflection()->GetUInt64(
            *sub_message, map_fixed64_fixed64_key_);
        uint64_t val = sub_message->GetReflection()->GetUInt64(
            *sub_message, map_fixed64_fixed64_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetUInt64Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_fixed64_fixed64"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetUInt64Value(), i);
        EXPECT_EQ(it->value().GetUInt64Value(), map[i]);
      }
    }
  }
  {
    absl::flat_hash_map<int32_t, int32_t> map = {{0, 0}, {1, 1}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_sfixed32_sfixed32")) {
        sub_message = &reflection->GetRepeatedMessage(
            message, F("map_sfixed32_sfixed32"), i);
        int32_t key = sub_message->GetReflection()->GetInt32(
            *sub_message, map_sfixed32_sfixed32_key_);
        int32_t val = sub_message->GetReflection()->GetInt32(
            *sub_message, map_sfixed32_sfixed32_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetInt32Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_sfixed32_sfixed32"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetInt32Value(), i);
        EXPECT_EQ(it->value().GetInt32Value(), map[i]);
      }
    }
  }
  {
    absl::flat_hash_map<int64_t, int64_t> map = {{0, 0}, {1, 1}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_sfixed64_sfixed64")) {
        sub_message = &reflection->GetRepeatedMessage(
            message, F("map_sfixed64_sfixed64"), i);
        int64_t key = sub_message->GetReflection()->GetInt64(
            *sub_message, map_sfixed64_sfixed64_key_);
        int64_t val = sub_message->GetReflection()->GetInt64(
            *sub_message, map_sfixed64_sfixed64_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetInt64Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_sfixed64_sfixed64"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetInt64Value(), i);
        EXPECT_EQ(it->value().GetInt64Value(), map[i]);
      }
    }
  }
  {
    absl::flat_hash_map<int32_t, float> map = {{0, 0.0f}, {1, 1.0f}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_int32_float")) {
        sub_message =
            &reflection->GetRepeatedMessage(message, F("map_int32_float"), i);
        int32_t key = sub_message->GetReflection()->GetInt32(
            *sub_message, map_int32_float_key_);
        float val = sub_message->GetReflection()->GetFloat(
            *sub_message, map_int32_float_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetInt32Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_int32_float"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetInt32Value(), i);
        EXPECT_EQ(it->value().GetFloatValue(), map[i]);
      }
    }
  }
  {
    absl::flat_hash_map<int32_t, double> map = {{0, 0.0}, {1, 1.0}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_int32_double")) {
        sub_message =
            &reflection->GetRepeatedMessage(message, F("map_int32_double"), i);
        int32_t key = sub_message->GetReflection()->GetInt32(
            *sub_message, map_int32_double_key_);
        double val = sub_message->GetReflection()->GetDouble(
            *sub_message, map_int32_double_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetInt32Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_int32_double"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetInt32Value(), i);
        EXPECT_EQ(it->value().GetDoubleValue(), map[i]);
      }
    }
  }
  {
    std::array<bool, 2> map = {false, true};
    std::vector<bool> keys = {false, true};
    std::vector<bool> vals = {false, true};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_bool_bool")) {
        sub_message =
            &reflection->GetRepeatedMessage(message, F("map_bool_bool"), i);
        bool key = sub_message->GetReflection()->GetBool(*sub_message,
                                                         map_bool_bool_key_);
        bool val = sub_message->GetReflection()->GetBool(*sub_message,
                                                         map_bool_bool_val_);
        EXPECT_EQ(map[key ? 1 : 0], val);
      } else {
        map_key.SetBoolValue(keys[i]);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_bool_bool"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetBoolValue(), keys[i]);
        EXPECT_EQ(it->value().GetBoolValue(), vals[i]);
      }
    }
  }
  {
    absl::flat_hash_map<std::string, std::string> map = {
        {long_string(), long_string()}, {long_string_2(), long_string_2()}};
    std::vector<std::string> keys = {long_string(), long_string_2()};
    std::vector<std::string> vals = {long_string(), long_string_2()};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_string_string")) {
        sub_message =
            &reflection->GetRepeatedMessage(message, F("map_string_string"), i);
        std::string key = sub_message->GetReflection()->GetString(
            *sub_message, map_string_string_key_);
        std::string val = sub_message->GetReflection()->GetString(
            *sub_message, map_string_string_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetStringValue(keys[i]);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_string_string"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetStringValue(), keys[i]);
        EXPECT_EQ(it->value().GetStringValue(), vals[i]);
      }
    }
  }
  {
    absl::flat_hash_map<int32_t, std::string> map = {{0, long_string()},
                                                     {1, long_string_2()}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_int32_bytes")) {
        sub_message =
            &reflection->GetRepeatedMessage(message, F("map_int32_bytes"), i);
        int32_t key = sub_message->GetReflection()->GetInt32(
            *sub_message, map_int32_bytes_key_);
        std::string val = sub_message->GetReflection()->GetString(
            *sub_message, map_int32_bytes_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetInt32Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_int32_bytes"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetInt32Value(), i);
        EXPECT_EQ(it->value().GetStringValue(), map[i]);
      }
    }
  }
  {
    absl::flat_hash_map<int32_t, const EnumValueDescriptor*> map = {
        {0, map_enum_bar_}, {1, map_enum_baz_}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_int32_enum")) {
        sub_message =
            &reflection->GetRepeatedMessage(message, F("map_int32_enum"), i);
        int32_t key = sub_message->GetReflection()->GetInt32(
            *sub_message, map_int32_enum_key_);
        const EnumValueDescriptor* val = sub_message->GetReflection()->GetEnum(
            *sub_message, map_int32_enum_val_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetInt32Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_int32_enum"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetInt32Value(), i);
        EXPECT_EQ(it->value().GetEnumValue(), map[i]->number());
      }
    }
  }
  {
    absl::flat_hash_map<int32_t, int32_t> map = {{0, 0}, {1, 1}};
    for (int i = 0; i < 2; i++) {
      if (IsRepeatedFieldValid(message, "map_int32_foreign_message")) {
        sub_message = &reflection->GetRepeatedMessage(
            message, F("map_int32_foreign_message"), i);
        int32_t key = sub_message->GetReflection()->GetInt32(
            *sub_message, map_int32_foreign_message_key_);
        const Message& foreign_message =
            sub_message->GetReflection()->GetMessage(
                *sub_message, map_int32_foreign_message_val_);
        int32_t val = foreign_message.GetReflection()->GetInt32(foreign_message,
                                                                foreign_c_);
        EXPECT_EQ(map[key], val);
      } else {
        map_key.SetInt32Value(i);
        GenericConstMapRef map_ref =
            reflection->GetMap(message, F("map_int32_foreign_message"));
        ASSERT_TRUE(map_ref.contains(map_key));
        auto it = map_ref.find(map_key);
        EXPECT_EQ(it->key().GetInt32Value(), i);
        const Message& foreign_message = it->value().GetMessageValue();
        EXPECT_EQ(foreign_message.GetReflection()->GetInt32(foreign_message,
                                                            foreign_c_),
                  map[i]);
      }
    }
  }
}

void MapReflectionTester::ExpectMapFieldsSetViaReflectionIterator(
    Message* message) {
  ExpectMapFieldsSetViaReflectionIterator(*message);

  // Also test mutable iteration, operator[], and at() on Message*
  const Reflection* reflection = message->GetReflection();
  for (absl::string_view name : kMapFields) {
    GenericMapRef mut_map = reflection->MutableMap(message, F(name));
    EXPECT_EQ(mut_map.size(), 2);
    EXPECT_FALSE(mut_map.empty());

    size_t count = 0;
    for (auto entry : mut_map) {
      EXPECT_NE(entry.key().type(), FieldDescriptor::CPPTYPE_MESSAGE);
      ++count;
    }
    EXPECT_EQ(count, 2);

    // Test conversion from MapIterator to MapConstIterator
    GenericConstMapRef::iterator cit = mut_map.begin();
    EXPECT_TRUE(cit == mut_map.begin());
    EXPECT_FALSE(cit != mut_map.begin());
    EXPECT_EQ(cit->key(), mut_map.begin()->key());
  }
}

void MapReflectionTester::ExpectMapFieldsSetViaReflectionIterator(
    const Message& message) {
  std::string serialized;
  const Reflection* reflection = message.GetReflection();

  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int32_int32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int64_int64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_uint32_uint32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_uint64_uint64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_sint32_sint32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_sint64_sint64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_fixed32_fixed32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_fixed64_fixed64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_sfixed32_sfixed32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_sfixed64_sfixed64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int32_float")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int32_double")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_bool_bool")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_string_string")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int32_bytes")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int32_enum")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int32_foreign_message")));

  // Const methods do not invalidate map.
  (void)message.DebugString();
  (void)message.ShortDebugString();
  EXPECT_TRUE(message.SerializeToString(&serialized));
  (void)message.SpaceUsedLong();
  (void)message.ByteSizeLong();

  {
    absl::flat_hash_map<int32_t, int32_t> map;
    map[0] = 0;
    map[1] = 1;
    ExpectMapIteratorHelper(reflection->GetMap(message, F("map_int32_int32")),
                            map);
  }
  {
    absl::flat_hash_map<int64_t, int64_t> map;
    map[0] = 0;
    map[1] = 1;
    ExpectMapIteratorHelper(reflection->GetMap(message, F("map_int64_int64")),
                            map);
  }
  {
    absl::flat_hash_map<uint32_t, uint32_t> map;
    map[0] = 0;
    map[1] = 1;
    ExpectMapIteratorHelper(reflection->GetMap(message, F("map_uint32_uint32")),
                            map);
  }
  {
    absl::flat_hash_map<uint64_t, uint64_t> map;
    map[0] = 0;
    map[1] = 1;
    ExpectMapIteratorHelper(reflection->GetMap(message, F("map_uint64_uint64")),
                            map);
  }
  {
    absl::flat_hash_map<int32_t, int32_t> map;
    map[0] = 0;
    map[1] = 1;
    ExpectMapIteratorHelper(reflection->GetMap(message, F("map_sint32_sint32")),
                            map);
  }
  {
    absl::flat_hash_map<int64_t, int64_t> map;
    map[0] = 0;
    map[1] = 1;
    ExpectMapIteratorHelper(reflection->GetMap(message, F("map_sint64_sint64")),
                            map);
  }
  {
    absl::flat_hash_map<uint32_t, uint32_t> map;
    map[0] = 0;
    map[1] = 1;
    ExpectMapIteratorHelper(
        reflection->GetMap(message, F("map_fixed32_fixed32")), map);
  }
  {
    absl::flat_hash_map<uint64_t, uint64_t> map;
    map[0] = 0;
    map[1] = 1;
    ExpectMapIteratorHelper(
        reflection->GetMap(message, F("map_fixed64_fixed64")), map);
  }
  {
    absl::flat_hash_map<int32_t, int32_t> map;
    map[0] = 0;
    map[1] = 1;
    ExpectMapIteratorHelper(
        reflection->GetMap(message, F("map_sfixed32_sfixed32")), map);
  }
  {
    absl::flat_hash_map<int64_t, int64_t> map;
    map[0] = 0;
    map[1] = 1;
    ExpectMapIteratorHelper(
        reflection->GetMap(message, F("map_sfixed64_sfixed64")), map);
  }
  {
    absl::flat_hash_map<int32_t, float> map;
    map[0] = 0.0f;
    map[1] = 1.0f;
    ExpectMapIteratorHelper(reflection->GetMap(message, F("map_int32_float")),
                            map);
  }
  {
    absl::flat_hash_map<int32_t, double> map;
    map[0] = 0.0;
    map[1] = 1.0;
    ExpectMapIteratorHelper(reflection->GetMap(message, F("map_int32_double")),
                            map);
  }
  {
    absl::flat_hash_map<bool, bool> map;
    map[false] = false;
    map[true] = true;
    ExpectMapIteratorHelper(reflection->GetMap(message, F("map_bool_bool")),
                            map);
  }
  {
    absl::flat_hash_map<std::string, std::string> map;
    map[long_string()] = long_string();
    map[long_string_2()] = long_string_2();
    ExpectMapIteratorHelper(reflection->GetMap(message, F("map_string_string")),
                            map);
  }
  {
    absl::flat_hash_map<int32_t, std::string> map;
    map[0] = long_string();
    map[1] = long_string_2();
    ExpectMapIteratorHelper(reflection->GetMap(message, F("map_int32_bytes")),
                            map);
  }
  {
    absl::flat_hash_map<int32_t, EnumVal> map;
    map[0] = EnumVal{map_enum_bar_->number()};
    map[1] = EnumVal{map_enum_baz_->number()};
    ExpectMapIteratorHelper(reflection->GetMap(message, F("map_int32_enum")),
                            map);
  }
  {
    absl::flat_hash_map<int32_t, int32_t> map;
    map[0] = 0;
    map[1] = 1;
    GenericConstMapRef map_ref =
        reflection->GetMap(message, F("map_int32_foreign_message"));
    EXPECT_EQ(map_ref.size(), 2);
    int size = 0;
    for (auto iter = map_ref.begin(); iter != map_ref.end(); ++iter, ++size) {
      const Message& sub = iter->value().GetMessageValue();
      EXPECT_EQ(map[iter->key().GetInt32Value()],
                sub.GetReflection()->GetInt32(sub, foreign_c_));
    }
    EXPECT_EQ(size, 2);
  }
}

void MapReflectionTester::ExpectClearViaReflection(const Message& message) {
  const Reflection* reflection = message.GetReflection();
  for (absl::string_view name : kMapFields) {
    const FieldDescriptor* field = F(name);
    auto map = reflection->GetMap(message, field);
    EXPECT_EQ(0, reflection->FieldSize(message, field));
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(0, map.size());
    EXPECT_EQ(map.begin(), map.end());
  }
  EXPECT_TRUE(reflection->GetMapData(message, F("map_int32_foreign_message"))
                  ->IsMapValid());
}

void MapReflectionTester::ExpectClearViaReflectionIterator(
    const Message& message) {
  const Reflection* reflection = message.GetReflection();
  for (absl::string_view name : kMapFields) {
    const FieldDescriptor* field = F(name);
    GenericConstMapRef map = reflection->GetMap(message, field);
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
    EXPECT_EQ(map.begin(), map.end());

    int count = 0;
    for (auto entry : map) {
      (void)entry;
      ++count;
    }
    EXPECT_EQ(count, 0);
  }
}

void MapReflectionTester::ExpectClearViaReflectionIterator(Message* message) {
  ExpectClearViaReflectionIterator(*message);
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
