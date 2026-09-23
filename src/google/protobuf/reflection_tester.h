// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_REFLECTION_TESTER_H__
#define GOOGLE_PROTOBUF_REFLECTION_TESTER_H__

#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/protobuf/map_field.h"
#include "google/protobuf/message.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

// Provides APIs to test protocol buffers reflectively.
class MapReflectionTester {
 public:
  // base_descriptor must be a descriptor for TestMap, which is used for
  // MapReflectionTester to fetch the FieldDescriptors needed to use the
  // reflection interface.
  explicit MapReflectionTester(const Descriptor* base_descriptor);

  void SetMapFieldsViaReflection(Message* message);
  void SetMapFieldsViaMapReflection(Message* message);
  void ClearMapFieldsViaReflection(Message* message);
  void ClearMapFieldsViaMapReflection(Message* message);
  void ModifyMapFieldsViaReflection(Message* message);
  void RemoveLastMapsViaReflection(Message* message);
  void ReleaseLastMapsViaReflection(Message* message);
  void SwapMapsViaReflection(Message* message);
  void SwapMapsViaMapReflection(Message* message1, Message* message2);
  void AssignMapsViaMapReflection(Message* dst, Message* src);
  void EraseMapValuesViaMapReflectionIterator(Message* message);
  void MutableUnknownFieldsOfMapFieldsViaReflection(Message* message);
  void ExpectMapFieldsSetViaReflection(const Message& message);
  void ExpectMapFieldsSetViaReflectionIterator(Message* message);
  void ExpectMapFieldsSetViaReflectionIterator(const Message& message);
  void ExpectClearViaReflection(const Message& message);
  void ExpectClearViaReflectionIterator(Message* message);
  void ExpectClearViaReflectionIterator(const Message& message);
  void DeleteMapValueViaMapReflection(Message* message,
                                      absl::string_view field_name,
                                      MapKey map_key);
  Message* GetMapEntryViaReflection(Message* message,
                                    absl::string_view field_name, int index);

  bool InsertOrLookupMapValue(Message* message, absl::string_view field_name,
                              const MapKey& map_key, MapValueRef* map_val) {
    auto res = message->GetReflection()
                   ->MutableMap(message, F(field_name))
                   .try_emplace(map_key);
    *map_val = res.first->value();
    return res.second;
  }

  bool LookupMapValue(Message* message, absl::string_view field_name,
                      const MapKey& map_key,
                      MapValueConstRef* map_value_const_ref) {
    return LookupMapValue(*message, field_name, map_key, map_value_const_ref);
  }

  bool LookupMapValue(const Message& message, absl::string_view field_name,
                      const MapKey& map_key,
                      MapValueConstRef* map_value_const_ref) {
    auto map = message.GetReflection()->GetMap(message, F(field_name));
    auto it = map.find(map_key);
    if (it == map.end()) return false;
    *map_value_const_ref = it->value();
    return true;
  }

  bool IsRepeatedFieldValid(const Message& message,
                            absl::string_view field_name) {
    const internal::MapFieldBase& map_field =
        message.GetReflection()->GetRaw<internal::MapFieldBase>(message,
                                                                F(field_name));
    return map_field.IsRepeatedFieldValid();
  }

  int MapSize(const Message& message, absl::string_view field_name);

  static absl::optional<MapValueConstRef> LookupMapValue(
      const Reflection& reflection, const Message& message,
      const FieldDescriptor& descriptor, const MapKey& map_key) {
    auto map = reflection.GetMap(message, &descriptor);
    auto it = map.find(map_key);
    if (it == map.end()) {
      return absl::nullopt;
    }
    return it->value();
  }

  static std::string long_string() {
    return "This is a very long string that goes in the heap";
  }
  static std::string long_string_2() {
    return "This is another very long string that goes in the heap";
  }

 private:
  const FieldDescriptor* F(absl::string_view name);

  const Descriptor* base_descriptor_;

  const EnumValueDescriptor* map_enum_bar_;
  const EnumValueDescriptor* map_enum_baz_;
  const EnumValueDescriptor* map_enum_foo_;

  const FieldDescriptor* foreign_c_;
  const FieldDescriptor* map_int32_int32_key_;
  const FieldDescriptor* map_int32_int32_val_;
  const FieldDescriptor* map_int64_int64_key_;
  const FieldDescriptor* map_int64_int64_val_;
  const FieldDescriptor* map_uint32_uint32_key_;
  const FieldDescriptor* map_uint32_uint32_val_;
  const FieldDescriptor* map_uint64_uint64_key_;
  const FieldDescriptor* map_uint64_uint64_val_;
  const FieldDescriptor* map_sint32_sint32_key_;
  const FieldDescriptor* map_sint32_sint32_val_;
  const FieldDescriptor* map_sint64_sint64_key_;
  const FieldDescriptor* map_sint64_sint64_val_;
  const FieldDescriptor* map_fixed32_fixed32_key_;
  const FieldDescriptor* map_fixed32_fixed32_val_;
  const FieldDescriptor* map_fixed64_fixed64_key_;
  const FieldDescriptor* map_fixed64_fixed64_val_;
  const FieldDescriptor* map_sfixed32_sfixed32_key_;
  const FieldDescriptor* map_sfixed32_sfixed32_val_;
  const FieldDescriptor* map_sfixed64_sfixed64_key_;
  const FieldDescriptor* map_sfixed64_sfixed64_val_;
  const FieldDescriptor* map_int32_float_key_;
  const FieldDescriptor* map_int32_float_val_;
  const FieldDescriptor* map_int32_double_key_;
  const FieldDescriptor* map_int32_double_val_;
  const FieldDescriptor* map_bool_bool_key_;
  const FieldDescriptor* map_bool_bool_val_;
  const FieldDescriptor* map_string_string_key_;
  const FieldDescriptor* map_string_string_val_;
  const FieldDescriptor* map_int32_bytes_key_;
  const FieldDescriptor* map_int32_bytes_val_;
  const FieldDescriptor* map_int32_enum_key_;
  const FieldDescriptor* map_int32_enum_val_;
  const FieldDescriptor* map_int32_foreign_message_key_;
  const FieldDescriptor* map_int32_foreign_message_val_;
};

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_REFLECTION_TESTER_H__
