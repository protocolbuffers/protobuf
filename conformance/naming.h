#ifndef GOOGLE_PROTOBUF_CONFORMANCE_NAMING_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_NAMING_H__

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_legacy.h"

// This file contains functions for generating test names and other identifiers
// for conformance tests.

namespace google {
namespace protobuf {
namespace conformance {

// Returns the edition identifier for the given message.  This is used to
// generate test names from parameterized descriptors.
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

// Returns the format identifier for the given wire format.  This is used to
// generate test names from parameterized wire formats.
inline absl::string_view GetFormatIdentifier(::conformance::WireFormat format) {
  switch (format) {
    case ::conformance::PROTOBUF:
      return "Protobuf";
    case ::conformance::JSON:
      return "Json";
    case ::conformance::TEXT_FORMAT:
      return "TextFormat";
    default:
      ABSL_LOG(FATAL) << "Unknown wire format";
  }
  return "";
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_NAMING_H__
