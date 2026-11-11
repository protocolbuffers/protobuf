// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/metadata_lite.h"

#include <cstdint>
#include <string>

#include "google/protobuf/arena.h"
#include "google/protobuf/class_data.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/unknown_field_set.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#if defined(PROTOBUF_SMALLER_MESSAGE_LITE) && !defined(PROTOBUF_CUSTOM_VTABLE)
#error "PROTOBUF_SMALLER_MESSAGE_LITE requires PROTOBUF_CUSTOM_VTABLE"
#endif

namespace google {
namespace protobuf {
namespace internal {

template <>
void InternalMetadata::DoClear<std::string>() {
  mutable_unknown_fields<std::string>()->clear();
}

template <>
void InternalMetadata::DoMergeFrom<std::string>(const std::string& other) {
  mutable_unknown_fields<std::string>()->append(other);
}

template <>
void InternalMetadata::DoSwap<std::string>(std::string* other) {
  mutable_unknown_fields<std::string>()->swap(*other);
}

#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
inline Arena* InternalMetadata::arena() const {
  static constexpr ptrdiff_t kArenaOffset =
      PROTOBUF_FIELD_OFFSET(MessageLite, _arena_) -
      PROTOBUF_FIELD_OFFSET(MessageLite, _internal_metadata_);
  return *reinterpret_cast<Arena* const*>(reinterpret_cast<const char*>(this) +
                                          kArenaOffset);
}
#endif

template <>
void InternalMetadata::DeleteOutOfLineHelper<std::string>() {
  delete PtrValue<Container<std::string>>();
  // TODO:  This store is load-bearing.  Since we are destructing
  // the message at this point, see if we can eliminate it.
  ptr_ = nullptr;
}

template <typename T>
T* InternalMetadata::mutable_unknown_fields_slow() {
  Container<T>* container = Arena::Create<Container<T>>(arena()
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
                                                            ,
                                                        class_data()
#endif
  );
  ptr_ = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(container) |
                                 kUnknownFieldsTagMask);
  return &(container->unknown_fields);
}

template std::string*
InternalMetadata::mutable_unknown_fields_slow<std::string>();
template UnknownFieldSet*
InternalMetadata::mutable_unknown_fields_slow<UnknownFieldSet>();

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
