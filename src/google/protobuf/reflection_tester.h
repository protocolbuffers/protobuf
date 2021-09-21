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

#ifndef GOOGLE_PROTOBUF_REFLECTION_TESTER_H__
#define GOOGLE_PROTOBUF_REFLECTION_TESTER_H__

#include <google/protobuf/message.h>

// Must be included last.
#include <google/protobuf/port_def.inc>

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
  void ModifyMapFieldsViaReflection(Message* message);
  void RemoveLastMapsViaReflection(Message* message);
  void ReleaseLastMapsViaReflection(Message* message);
  void SwapMapsViaReflection(Message* message);
  void MutableUnknownFieldsOfMapFieldsViaReflection(Message* message);
  void ExpectMapFieldsSetViaReflection(const Message& message);
  void ExpectMapFieldsSetViaReflectionIterator(Message* message);
  void ExpectClearViaReflection(const Message& message);
  void ExpectClearViaReflectionIterator(Message* message);
  void GetMapValueViaMapReflection(Message* message,
                                   const std::string& field_name,
                                   const MapKey& map_key, MapValueRef* map_val);
  Message* GetMapEntryViaReflection(Message* message,
                                    const std::string& field_name, int index);
  MapIterator MapBegin(Message* message, const std::string& field_name);
  MapIterator MapEnd(Message* message, const std::string& field_name);
  int MapSize(const Message& message, const std::string& field_name);

 private:
  const FieldDescriptor* F(const std::string& name);

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

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_REFLECTION_TESTER_H__
