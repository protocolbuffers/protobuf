#include "benchmarks.pb.h"
#include "datasets/google_message1/proto2/benchmark_message1_proto2.pb.h"
#include "datasets/google_message1/proto3/benchmark_message1_proto3.pb.h"
#include "datasets/google_message2/benchmark_message2.pb.h"
#include "datasets/google_message3/benchmark_message3.pb.h"
#include "datasets/google_message4/benchmark_message4.pb.h"

#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"

#include <fstream>

using google::protobuf::FieldDescriptor;
using google::protobuf::Message;
using google::protobuf::Reflection;


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

std::string ReadFile(const std::string& name) {
  std::ifstream file(name.c_str());
  GOOGLE_CHECK(file.is_open()) << "Couldn't find file '"
      << name
      << "', please make sure you are running this command from the benchmarks"
      << " directory.\n";
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

int main(int argc, char *argv[]) {
  if (argc % 2 == 0 || argc == 1) {
    std::cerr << "Usage: [input_files] [output_file_names] where " <<
        "input_files are one to one mapping to output_file_names." <<
        std::endl;
    return 1;
  }

  for (int i = argc / 2; i > 0; i--) {
    const std::string &input_file = argv[i];
    const std::string &output_file = argv[i + argc / 2];

    std::cerr << "Generating " << input_file
        << " to " << output_file << std::endl;
    benchmarks::BenchmarkDataset dataset;
    Message* message;
    std::string dataset_payload = ReadFile(input_file);
    GOOGLE_CHECK(dataset.ParseFromString(dataset_payload))
      << "Can' t parse data file " << input_file;

    if (dataset.message_name() == "benchmarks.proto3.GoogleMessage1") {
      message = new benchmarks::proto3::GoogleMessage1;
    } else if (dataset.message_name() == "benchmarks.proto2.GoogleMessage1") {
      message = new benchmarks::proto2::GoogleMessage1;
    } else if (dataset.message_name() == "benchmarks.proto2.GoogleMessage2") {
      message = new benchmarks::proto2::GoogleMessage2;
    } else if (dataset.message_name() ==
        "benchmarks.google_message3.GoogleMessage3") {
      message = new benchmarks::google_message3::GoogleMessage3;
    } else if (dataset.message_name() ==
        "benchmarks.google_message4.GoogleMessage4") {
      message = new benchmarks::google_message4::GoogleMessage4;
    } else {
      std::cerr << "Unknown message type: " << dataset.message_name();
      exit(1);
    }

    for (int i = 0; i < dataset.payload_size(); i++) {
      message->ParseFromString(dataset.payload(i));
      DataGroupStripper::StripMessage(message);
      dataset.set_payload(i, message->SerializeAsString());
    }

    std::ofstream ofs(output_file);
    ofs << dataset.SerializeAsString();
    ofs.close();
  }


  return 0;
}
