#ifndef PROTOBUF_BENCHMARKS_UTIL_SCHEMA_PROTO2_TO_PROTO3_UTIL_H_
#define PROTOBUF_BENCHMARKS_UTIL_SCHEMA_PROTO2_TO_PROTO3_UTIL_H_

#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

#include <sstream>
#include <algorithm>

using google::protobuf::Descriptor;
using google::protobuf::DescriptorProto;
using google::protobuf::FileDescriptorProto;
using google::protobuf::FieldDescriptorProto;
using google::protobuf::Message;
using google::protobuf::EnumValueDescriptorProto;

namespace google {
namespace protobuf {
namespace util {

class SchemaGroupStripper {

 public:
  static void StripFile(const FileDescriptor* old_file,
                        FileDescriptorProto *file) {
    for (int i = file->mutable_message_type()->size() - 1; i >= 0; i--) {
      if (IsMessageSet(old_file->message_type(i))) {
        file->mutable_message_type()->DeleteSubrange(i, 1);
        continue;
      }
      StripMessage(old_file->message_type(i), file->mutable_message_type(i));
    }
    for (int i = file->mutable_extension()->size() - 1; i >= 0; i--) {
      auto field = old_file->extension(i);
      if (field->type() == FieldDescriptor::TYPE_GROUP ||
          IsMessageSet(field->message_type()) ||
          IsMessageSet(field->containing_type())) {
        file->mutable_extension()->DeleteSubrange(i, 1);
      }
    }
  }

 private:
  static bool IsMessageSet(const Descriptor *descriptor) {
    if (descriptor != nullptr
        && descriptor->options().message_set_wire_format()) {
      return true;
    }
    return false;
  }

  static void StripMessage(const Descriptor *old_message,
                           DescriptorProto *new_message) {
    for (int i = new_message->mutable_field()->size() - 1; i >= 0; i--) {
      if (old_message->field(i)->type() == FieldDescriptor::TYPE_GROUP ||
          IsMessageSet(old_message->field(i)->message_type())) {
        new_message->mutable_field()->DeleteSubrange(i, 1);
      }
    }
    for (int i = new_message->mutable_extension()->size() - 1; i >= 0; i--) {
      auto field_type_name = new_message->mutable_extension(i)->type_name();
      if (old_message->extension(i)->type() == FieldDescriptor::TYPE_GROUP ||
          IsMessageSet(old_message->extension(i)->containing_type()) ||
          IsMessageSet(old_message->extension(i)->message_type())) {
        new_message->mutable_extension()->DeleteSubrange(i, 1);
      }
    }
    for (int i = 0; i < new_message->mutable_nested_type()->size(); i++) {
      StripMessage(old_message->nested_type(i),
                   new_message->mutable_nested_type(i));
    }
  }

};

class SchemaAddZeroEnumValue {

 public:
  SchemaAddZeroEnumValue()
      : total_added_(0) {
  }

  void ScrubFile(FileDescriptorProto *file) {
    for (int i = 0; i < file->enum_type_size(); i++) {
      ScrubEnum(file->mutable_enum_type(i));
    }
    for (int i = 0; i < file->mutable_message_type()->size(); i++) {
      ScrubMessage(file->mutable_message_type(i));
    }
  }

 private:
  void ScrubEnum(EnumDescriptorProto *enum_type) {
    if (enum_type->value(0).number() != 0) {
      bool has_zero = false;
      for (int j = 0; j < enum_type->value().size(); j++) {
        if (enum_type->value(j).number() == 0) {
          EnumValueDescriptorProto temp_enum_value;
          temp_enum_value.CopyFrom(enum_type->value(j));
          enum_type->mutable_value(j)->CopyFrom(enum_type->value(0));
          enum_type->mutable_value(0)->CopyFrom(temp_enum_value);
          has_zero = true;
          break;
        }
      }
      if (!has_zero) {
        enum_type->mutable_value()->Add();
        for (int i = enum_type->mutable_value()->size() - 1; i > 0; i--) {
          enum_type->mutable_value(i)->CopyFrom(
              *enum_type->mutable_value(i - 1));
        }
        enum_type->mutable_value(0)->set_number(0);
        enum_type->mutable_value(0)->set_name("ADDED_ZERO_VALUE_" +
                                              std::to_string(total_added_++));
      }
    }

  }

  void ScrubMessage(DescriptorProto *message_type) {
    for (int i = 0; i < message_type->mutable_enum_type()->size(); i++) {
      ScrubEnum(message_type->mutable_enum_type(i));
    }
    for (int i = 0; i < message_type->mutable_nested_type()->size(); i++) {
      ScrubMessage(message_type->mutable_nested_type(i));
    }
  }

  int total_added_;
};

}  // namespace util
}  // namespace protobuf
}  // namespace google

#endif  // PROTOBUF_BENCHMARKS_UTIL_SCHEMA_PROTO2_TO_PROTO3_UTIL_H_
