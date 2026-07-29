#include "google/protobuf/message.h"

#include <cstddef>
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
  return m->ParseFromString(input.AsStringView());
}

bool proto2_rust_Message_parse_dont_enforce_required(
    google::protobuf::MessageLite* m, google::protobuf::rust::PtrAndLen input) {
  return m->ParsePartialFromString(input.AsStringView());
}

bool proto2_rust_Message_serialize(const google::protobuf::MessageLite* m,
                                   google::protobuf::rust::SerializedData* output) {
  return google::protobuf::rust::SerializeMsg(m, output);
}

size_t proto2_rust_Message_serialized_len(const google::protobuf::MessageLite* m) {
  return m->ByteSizeLong();
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

void proto2_rust_Message_take_from(google::protobuf::MessageLite* dst,
                                   google::protobuf::MessageLite* src) {
  // Rust guarantees that dst and src do not alias.

  dst->Clear();
  if constexpr (kHasFullRuntime) {
    if (auto* dst_msg = google::protobuf::DynamicCastMessage<google::protobuf::Message>(dst)) {
      // Rust's TakeFrom trait bounds (MutProxied = Self::Proxied) guarantee at
      // compile time that dst and src point to instances of the exact same
      // C++ message class. Therefore, if dst is a google::protobuf::Message, src is
      // guaranteed to also be a google::protobuf::Message, allowing us to safely
      // static_cast and skip a second runtime dynamic_cast check.
      auto* src_msg = static_cast<google::protobuf::Message*>(src);
      if (const auto* reflection = dst_msg->GetReflection()) {
        // TODO: Use a generic Move operation here instead of Swap,
        // which will be more efficient if the protos are in different arenas.
        reflection->Swap(dst_msg, src_msg);
        return;
      }
    }
  }
  dst->CheckTypeAndMergeFrom(*src);
  src->Clear();
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
