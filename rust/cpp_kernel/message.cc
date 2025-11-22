#include "google/protobuf/message.h"

#include <limits>

#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/serialized_data.h"
#include "rust/cpp_kernel/strings.h"

constexpr bool kHasFullRuntime = true;

extern "C" {

void proto2_rust_Message_delete(google::protobuf::MessageLite* m) { delete m; }

void proto2_rust_Message_clear(google::protobuf::MessageLite* m) { m->Clear(); }

bool proto2_rust_Message_parse(google::protobuf::MessageLite* m,
                               google::protobuf::rust::PtrAndLen input) {
  if (input.len > std::numeric_limits<int>::max()) {
    return false;
  }
  return m->ParseFromString(
      absl::string_view(input.ptr, static_cast<int>(input.len)));
}

bool proto2_rust_Message_parse_dont_enforce_required(
    google::protobuf::MessageLite* m, google::protobuf::rust::PtrAndLen input) {
  if (input.len > std::numeric_limits<int>::max()) {
    return false;
  }
  return m->ParsePartialFromString(
      absl::string_view(input.ptr, static_cast<int>(input.len)));
}

bool proto2_rust_Message_serialize(const google::protobuf::MessageLite* m,
                                   google::protobuf::rust::SerializedData* output) {
  return google::protobuf::rust::SerializeMsg(m, output);
}

void proto2_rust_Message_copy_from(google::protobuf::MessageLite* dst,
                                   const google::protobuf::MessageLite& src) {
  dst->Clear();
  dst->CheckTypeAndMergeFrom(src);
}

void proto2_rust_Message_merge_from(google::protobuf::MessageLite* dst,
                                    const google::protobuf::MessageLite& src) {
  dst->CheckTypeAndMergeFrom(src);
}

// Returns a pointer to the descriptor of the message, or nullptr if
// the message is not google::protobuf::Message.
const void* proto2_rust_Message_get_descriptor(const google::protobuf::MessageLite* m) {
  if constexpr (kHasFullRuntime) {
    auto msg = google::protobuf::DynamicCastMessage<google::protobuf::Message>(m);
    if (msg == nullptr) {
      return nullptr;
    }
    return msg->GetDescriptor();
  }
  return nullptr;
}

}  // extern "C"
