#include "google/protobuf/metadata_lite.h"

#include <string>

#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/unknown_field_map.h"
#include "google/protobuf/unknown_field_set.h"

namespace google {
namespace protobuf {
namespace internal {

void InternalMetadata::SwapSlow(InternalMetadata* other) {
  Arena* arena = this->arena();
  ABSL_DCHECK_EQ(arena, other->arena());
  UnknownFieldMap& map = arena != nullptr
                             ? UnknownFieldMap::ArenaUnknownFieldMap(*arena)
                             : UnknownFieldMap::GlobalUnknownFieldMap();

  if (have_unknown_fields() && other->have_unknown_fields()) {
    map.Swap(this, other);
    return;
  }

  auto* with_unknown = have_unknown_fields() ? this : other;
  auto* without_unknown = have_unknown_fields() ? other : this;
  map.Transfer(without_unknown, with_unknown);

  with_unknown->ptr_ &= ~kUnknownFieldsTagMask;
  without_unknown->ptr_ |= kUnknownFieldsTagMask;
}

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

template <typename T>
void InternalMetadata::DeleteOutOfLineHelper() {
  ABSL_DCHECK_EQ(arena(), nullptr);
  UnknownFieldMap::GlobalUnknownFieldMap().DeleteUnknownFieldSet<T>(this);
  ptr_ = 0;
}

template void InternalMetadata::DeleteOutOfLineHelper<std::string>();
template void InternalMetadata::DeleteOutOfLineHelper<UnknownFieldSet>();

template <typename T>
T* InternalMetadata::mutable_unknown_fields_slow() {
  Arena* arena = this->arena();
  ptr_ |= kUnknownFieldsTagMask;
  UnknownFieldMap& map = arena != nullptr
                             ? UnknownFieldMap::ArenaUnknownFieldMap(*arena)
                             : UnknownFieldMap::GlobalUnknownFieldMap();
  return map.MutableUnknownFieldSet<T>(this);
}

template std::string*
InternalMetadata::mutable_unknown_fields_slow<std::string>();
template UnknownFieldSet*
InternalMetadata::mutable_unknown_fields_slow<UnknownFieldSet>();

template <typename T>
const T& InternalMetadata::unknown_fields_slow() const {
  ABSL_CHECK(have_unknown_fields());
  Arena* arena = this->arena();
  UnknownFieldMap& map = arena != nullptr
                             ? UnknownFieldMap::ArenaUnknownFieldMap(*arena)
                             : UnknownFieldMap::GlobalUnknownFieldMap();
  return *map.GetUnknownFieldSet<T>(this);
}

template const std::string& InternalMetadata::unknown_fields_slow<std::string>()
    const;
template const UnknownFieldSet&
InternalMetadata::unknown_fields_slow<UnknownFieldSet>() const;

}  // namespace internal
}  // namespace protobuf
}  // namespace google
