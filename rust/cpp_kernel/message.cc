// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/message.h"

#include <cstddef>
#include <limits>

#include "absl/log/absl_check.h"
#include "google/protobuf/class_data.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/message_traits.h"
#include "google/protobuf/private_access.h"
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
                                   google::protobuf::rust::SerializedData* output,
                                   bool deterministic) {
  return google::protobuf::rust::SerializeMsg(m, output, deterministic);
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

void proto2_rust_Message_placement_new(
    const google::protobuf::MessageLite* default_instance, void* dst,
    size_t expected_size, size_t expected_align) {
  ABSL_CHECK_NE(default_instance, nullptr);
  ABSL_CHECK_NE(dst, nullptr);
  const google::protobuf::internal::ClassData* data =
      google::protobuf::internal::GetClassData(*default_instance);
  ABSL_CHECK_NE(data, nullptr);
  ABSL_CHECK_EQ(expected_size, size_t{data->allocation_size()});
  ABSL_CHECK_EQ(expected_align, size_t{data->alignment()});
  data->PlacementNew(dst, /*arena=*/nullptr);
}

void proto2_rust_Message_destroy_in_place(google::protobuf::MessageLite* m) {
  ABSL_CHECK_NE(m, nullptr);
  ABSL_CHECK_EQ(m->GetArena(), nullptr);
  google::protobuf::internal::PrivateAccess::DestroyInstance(*m);
}

}  // extern "C"
