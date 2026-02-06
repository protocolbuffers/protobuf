#include "google/protobuf/message.h"

#include <limits>

#include "google/protobuf/descriptor.h"
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

const void* proto2_rust_Message_GetExtension(const google::protobuf::MessageLite* msg,
                                             int number) {
  if constexpr (kHasFullRuntime) {
    auto m = google::protobuf::DynamicCastMessage<google::protobuf::Message>(msg);
    if (m == nullptr) {
      return nullptr;
    }
    const google::protobuf::Reflection* reflection = m->GetReflection();
    const google::protobuf::Descriptor* descriptor = m->GetDescriptor();
    if (descriptor == nullptr) {
      return nullptr;
    }
    const google::protobuf::DescriptorPool* pool = descriptor->file()->pool();
    const google::protobuf::FieldDescriptor* field =
        pool->FindExtensionByNumber(descriptor, number);
    if (field == nullptr) {
      return nullptr;
    }

    // For now, we only support singular message extensions.
    if (field->is_repeated() ||
        field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      return nullptr;
    }

    if (!reflection->HasField(*m, field)) {
      return nullptr;
    }

    const google::protobuf::Message& extension_msg = reflection->GetMessage(*m, field);
    return &extension_msg;
  }
  return nullptr;
}

bool proto2_rust_Message_HasExtension(const google::protobuf::MessageLite* msg,
                                      int number) {
  if constexpr (kHasFullRuntime) {
    auto m = google::protobuf::DynamicCastMessage<google::protobuf::Message>(msg);
    if (m == nullptr) {
      return false;
    }
    const google::protobuf::Reflection* reflection = m->GetReflection();
    const google::protobuf::Descriptor* descriptor = m->GetDescriptor();
    if (descriptor == nullptr) {
      return false;
    }
    const google::protobuf::DescriptorPool* pool = descriptor->file()->pool();
    const google::protobuf::FieldDescriptor* field =
        pool->FindExtensionByNumber(descriptor, number);
    if (field == nullptr) {
      return false;
    }
    return reflection->HasField(*m, field);
  }
  return false;
}

void* proto2_rust_Message_GetMutableExtension(google::protobuf::MessageLite* msg,
                                              int number) {
  if constexpr (kHasFullRuntime) {
    auto m = google::protobuf::DynamicCastMessage<google::protobuf::Message>(msg);
    if (m == nullptr) {
      return nullptr;
    }
    const google::protobuf::Reflection* reflection = m->GetReflection();
    const google::protobuf::Descriptor* descriptor = m->GetDescriptor();
    if (descriptor == nullptr) {
      return nullptr;
    }
    const google::protobuf::DescriptorPool* pool = descriptor->file()->pool();
    const google::protobuf::FieldDescriptor* field =
        pool->FindExtensionByNumber(descriptor, number);
    if (field == nullptr) {
      return nullptr;
    }

    if (field->is_repeated() ||
        field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      return nullptr;
    }

    return reflection->MutableMessage(m, field);
  }
  return nullptr;
}

}  // extern "C"
