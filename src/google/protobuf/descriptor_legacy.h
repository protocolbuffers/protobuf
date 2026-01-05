#ifndef GOOGLE_PROTOBUF_DESCRIPTOR_LEGACY_H__
#define GOOGLE_PROTOBUF_DESCRIPTOR_LEGACY_H__

#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

// TODO Remove this deprecated API entirely.
class PROTOBUF_FUTURE_ADD_EARLY_WARN_UNUSED FileDescriptorLegacy {
 public:
  explicit FileDescriptorLegacy(const FileDescriptor* file) : file_(file) {}

  // Edition shouldn't be depended on unless dealing with raw unbuilt
  // descriptors, which will expose it via FileDescriptorProto.edition.
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD Edition edition() const {
    return file_->edition();
  }

 private:
  const FileDescriptor* file_;
};
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_DESCRIPTOR_LEGACY_H__
