#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"

using google::protobuf::FieldDescriptor;
using google::protobuf::Message;
using google::protobuf::Reflection;

namespace google {
namespace protobuf {
namespace util {

class DataGroupStripper {
 public:
  static void StripMessage(Message *message) {
    std::vector<const FieldDescriptor*> set_fields;
    const Reflection* reflection = message->GetReflection();
    reflection->ListFields(*message, &set_fields);

    for (size_t i = 0; i < set_fields.size(); i++) {
      const FieldDescriptor* field = set_fields[i];
      if (field->type() == FieldDescriptor::TYPE_GROUP) {
        reflection->ClearField(message, field);
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
};

}  // namespace util
}  // namespace protobuf
}  // namespace google

