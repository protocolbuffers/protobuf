#include "rust/cpp_kernel/compare.h"

#include <string>

#include "absl/log/absl_check.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/util/message_differencer.h"

static std::string SerializeDeterministically(const google::protobuf::MessageLite& m) {
  std::string serialized;
  {
    google::protobuf::io::StringOutputStream output_stream(&serialized);
    google::protobuf::io::CodedOutputStream coded_stream(&output_stream);
    coded_stream.SetSerializationDeterministic(true);
    // TODO: Remove this suppression.
    (void)m.SerializePartialToCodedStream(&coded_stream);
  }
  return serialized;
}

extern "C" {

bool proto2_rust_messagelite_equals(const google::protobuf::MessageLite* msg1,
                                    const google::protobuf::MessageLite* msg2) {
  return SerializeDeterministically(*msg1) == SerializeDeterministically(*msg2);
}

bool proto2_rust_messagelite_partially_equals(
    const google::protobuf::MessageLite* actual, const google::protobuf::MessageLite* expected) {
  const google::protobuf::Message* actual_message =
      google::protobuf::DynamicCastMessage<google::protobuf::Message>(actual);
  const google::protobuf::Message* expected_message =
      google::protobuf::DynamicCastMessage<google::protobuf::Message>(expected);
  ABSL_CHECK(actual_message != nullptr);
  ABSL_CHECK(expected_message != nullptr);

  google::protobuf::util::MessageDifferencer differencer;
  differencer.set_scope(google::protobuf::util::MessageDifferencer::PARTIAL);
  return differencer.Compare(*expected_message, *actual_message);
}

}  // extern "C"
