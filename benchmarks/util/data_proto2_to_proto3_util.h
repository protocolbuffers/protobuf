#ifndef PROTOBUF_BENCHMARKS_UTIL_DATA_PROTO2_TO_PROTO3_UTIL_H_
#define PROTOBUF_BENCHMARKS_UTIL_DATA_PROTO2_TO_PROTO3_UTIL_H_

#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"

using google::protobuf::FieldDescriptor;
using google::protobuf::Message;
using google::protobuf::Reflection;

namespace google {
namespace protobuf {
namespace util {

class DataStripper {
 public:
  void StripMessage(Message *message) {
    std::vector<const FieldDescriptor*> set_fields;
    const Reflection* reflection = message->GetReflection();
    reflection->ListFields(*message, &set_fields);

    for (size_t i = 0; i < set_fields.size(); i++) {
      const FieldDescriptor* field = set_fields[i];
      if (ShouldBeClear(field)) {
        reflection->ClearField(message, field);
        continue;
      }
      if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
        if (field->is_repeated()) {
          for (int j = 0; j < reflection->FieldSize(*message, field); j++) {
            StripMessage(reflection->MutableRepeatedMessage(message, field, j));
          }
        } else {
          StripMessage(reflection->MutableMessage(message, field));
        }
      }
    }

    reflection->MutableUnknownFields(message)->Clear();
  }
 private:
  virtual bool ShouldBeClear(const FieldDescriptor *field) = 0;
};

class GogoDataStripper : public DataStripper {
 private:
  virtual bool ShouldBeClear(const FieldDescriptor *field) {
    return field->type() == FieldDescriptor::TYPE_GROUP;
  }
};

class Proto3DataStripper : public DataStripper {
 private:
  virtual bool ShouldBeClear(const FieldDescriptor *field) {
    return field->type() == FieldDescriptor::TYPE_GROUP ||
           field->is_extension();
  }
};

}  // namespace util
}  // namespace protobuf
}  // namespace google

#endif  // PROTOBUF_BENCHMARKS_UTIL_DATA_PROTO2_TO_PROTO3_UTIL_H_
