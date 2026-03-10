// Copyright (c) 2024 Google LLC
// All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/common/cpp_to_upb_def.h"

#include <string>

#include "google/protobuf/descriptor.upb.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/descriptor.h"
#include "upb/base/status.hpp"
#include "upb/mem/arena.hpp"
#include "upb/mini_table/field.h"
#include "upb/reflection/def.hpp"

namespace upb::generator {

using google::protobuf::Descriptor;
using google::protobuf::EnumDescriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::FileDescriptor;

// Internal helper that takes a filedesc and emits a upb proto;
// used for defpool tracking and dependency management
google_protobuf_FileDescriptorProto* ToUpbProto(const FileDescriptor* file,
                                       upb::Arena* arena) {
  google::protobuf::FileDescriptorProto proto;
  file->CopyTo(&proto);
  std::string serialized_proto = proto.SerializeAsString();
  google_protobuf_FileDescriptorProto* upb_proto = google_protobuf_FileDescriptorProto_parse(
      serialized_proto.data(), serialized_proto.size(), arena->ptr());
  ABSL_CHECK(upb_proto) << "Failed to parse proto";
  return upb_proto;
}

void AddFile(const FileDescriptor* file, upb::DefPool* pool) {
  // Avoid adding the same file twice.
  const std::string name(file->name());
  if (pool->FindFileByName(name.c_str())) return;

  // Like a google::protobuf::DescriptorPool, a upb::DefPool requires that all
  // dependencies are added first.
  for (int i = 0; i < file->dependency_count(); i++) {
    AddFile(file->dependency(i), pool);
  }

  upb::Arena tmp_arena;
  upb::Status status;
  ABSL_CHECK(pool->AddFile(ToUpbProto(file, &tmp_arena), &status))
      << status.error_message();
}

upb::MessageDefPtr FindMessageDef(upb::DefPool& pool,
                                  const Descriptor* descriptor) {
  const std::string name(descriptor->full_name());
  upb::MessageDefPtr message_def = pool.FindMessageByName(name.c_str());
  ABSL_CHECK(message_def) << "No message named " << name;
  return message_def;
}

upb::EnumDefPtr FindEnumDef(upb::DefPool& pool,
                            const EnumDescriptor* enum_descriptor) {
  const std::string name(enum_descriptor->full_name());
  upb::EnumDefPtr enum_def = pool.FindEnumByName(name.c_str());
  ABSL_CHECK(enum_def) << "No enum named " << name;
  return enum_def;
}

upb::FieldDefPtr FindBaseFieldDef(upb::DefPool& pool,
                                  const FieldDescriptor* field) {
  ABSL_CHECK(!field->is_extension());
  upb::MessageDefPtr message_def =
      FindMessageDef(pool, field->containing_type());
  upb::FieldDefPtr field_def = message_def.FindFieldByNumber(field->number());
  ABSL_CHECK(field_def) << "No field with number " << field->number()
                        << " in message " << message_def.full_name();
  return field_def;
}

upb::FieldDefPtr FindExtensionDef(upb::DefPool& pool,
                                  const FieldDescriptor* field) {
  ABSL_CHECK(field->is_extension());
  const std::string name(field->full_name());
  return pool.FindExtensionByName(name.c_str());
}

const FieldDescriptor* FindFieldDescriptor(
    const Descriptor* message, const upb_MiniTableField* field_def) {
  int field_number = upb_MiniTableField_Number(field_def);
  const FieldDescriptor* field = message->FindFieldByNumber(field_number);
  ABSL_CHECK(field) << "No field in message " << message->full_name()
                    << " with number " << field_number;
  return field;
}

}  // namespace upb::generator
