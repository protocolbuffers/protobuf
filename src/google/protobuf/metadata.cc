#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/unknown_field_set.h"

namespace google {
namespace protobuf {
namespace internal {

template void InternalMetadata::DoClear<UnknownFieldSet>();
template void InternalMetadata::DoMergeFrom<UnknownFieldSet>(
    const UnknownFieldSet& other);
template void InternalMetadata::DoSwap<UnknownFieldSet>(UnknownFieldSet* other);
template void InternalMetadata::DeleteOutOfLineHelper<UnknownFieldSet>();

}  // namespace internal
}  // namespace protobuf
}  // namespace google
