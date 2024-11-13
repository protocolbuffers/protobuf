#include "rust/cpp_kernel/compare.h"

#include <string>

#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/message_lite.h"

static std::string SerializeDeterministically(const google::protobuf::MessageLite& m) {
  std::string serialized;
  {
    google::protobuf::io::StringOutputStream output_stream(&serialized);
    google::protobuf::io::CodedOutputStream coded_stream(&output_stream);
    coded_stream.SetSerializationDeterministic(true);
    m.SerializePartialToCodedStream(&coded_stream);
  }
  return serialized;
}

extern "C" {

bool proto2_rust_messagelite_equals(const google::protobuf::MessageLite* msg1,
                                    const google::protobuf::MessageLite* msg2) {
  return SerializeDeterministically(*msg1) == SerializeDeterministically(*msg2);
}

}  // extern "C"
