#include "google/protobuf/metadata_lite.h"

#include <string>

#include "google/protobuf/unknown_field_set.h"

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

template UnknownFieldSet*
InternalMetadata::mutable_unknown_fields_slow<UnknownFieldSet>();

}  // namespace internal
}  // namespace protobuf
}  // namespace google
