#include <iostream>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

int main() {
  const ::google::protobuf::Descriptor* d =
      ::google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(
          "google.protobuf.FeatureSet");
  if (d == nullptr) {
    std::cerr << "FeatureSet descriptor not found!" << std::endl;
    return 1;
  }
  const ::google::protobuf::FieldDescriptor* f = d->FindFieldByName("field_presence");
  if (f == nullptr) {
    std::cerr << "field_presence field not found!" << std::endl;
    return 1;
  }
  std::cout << "INITIAL: Has feature_support: "
            << f->options().has_feature_support() << std::endl;

  // Let's copy the whole FileDescriptorProto of descriptor.proto
  const ::google::protobuf::FileDescriptor* file = d->file();
  ::google::protobuf::FileDescriptorProto fdp;
  file->CopyTo(&fdp);

  std::string serialized;
  if (!fdp.SerializeToString(&serialized)) {
    std::cerr << "Failed to serialize FileDescriptorProto" << std::endl;
    return 1;
  }

  ::google::protobuf::FileDescriptorProto fdp2;
  if (!fdp2.ParseFromString(serialized)) {
    std::cerr << "Failed to parse FileDescriptorProto" << std::endl;
    return 1;
  }

  // Find field_presence in parsed fdp2
  const ::google::protobuf::DescriptorProto* parsed_feature_set = nullptr;
  for (const auto& message_type : fdp2.message_type()) {
    if (message_type.name() == "FeatureSet") {
      parsed_feature_set = &message_type;
      break;
    }
  }
  if (parsed_feature_set == nullptr) {
    std::cerr << "Parsed FeatureSet not found!" << std::endl;
    return 1;
  }

  const ::google::protobuf::FieldDescriptorProto* parsed_field_presence = nullptr;
  for (const auto& field : parsed_feature_set->field()) {
    if (field.name() == "field_presence") {
      parsed_field_presence = &field;
      break;
    }
  }
  if (parsed_field_presence == nullptr) {
    std::cerr << "Parsed field_presence not found!" << std::endl;
    return 1;
  }

  std::cout << "PARSED: fdp options has features: "
            << fdp2.options().has_features()
            << std::endl;  // wait, FileOptions has features
  std::cout << "PARSED: field_presence has options: "
            << parsed_field_presence->has_options() << std::endl;
  if (parsed_field_presence->has_options()) {
    std::cout << "PARSED: field_presence options has feature_support: "
              << parsed_field_presence->options().has_feature_support()
              << std::endl;
  }

  return 0;
}
