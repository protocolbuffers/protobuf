#ifndef GOOGLE_PROTOBUF_CONFORMANCE_V2_NAMING_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_V2_NAMING_H__

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_legacy.h"

namespace google {
namespace protobuf {

inline std::string GetEditionIdentifier(const Descriptor& message) {
  switch (FileDescriptorLegacy(message.file()).edition()) {
    case Edition::EDITION_PROTO3:
      return "Proto3";
    case Edition::EDITION_PROTO2:
      return "Proto2";
    default: {
      std::string id = "Editions";
      if (message.name() == "TestAllTypesProto2") {
        absl::StrAppend(&id, "_Proto2");
      } else if (message.name() == "TestAllTypesProto3") {
        absl::StrAppend(&id, "_Proto3");
      }
      return id;
    }
  }
}

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_V2_NAMING_H__
