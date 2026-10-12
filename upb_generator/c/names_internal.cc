// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/c/names_internal.h"

#include <string>

#include "absl/strings/string_view.h"
#include "upb_generator/common/names.h"

namespace upb {
namespace generator {

std::string CApiHeaderFilename(absl::string_view proto_filename,
                               bool bootstrap) {
  if (bootstrap) {
    if (proto_filename == "third_party/protobuf/json_enumvalue_options.proto" ||
        proto_filename == "google/protobuf/json_enumvalue_options.proto" ||
        proto_filename == "src/google/protobuf/json_enumvalue_options.proto") {
      return "upb/reflection/"
             "json_enumvalue_options_bootstrap.h";
    } else if (proto_filename == "net/proto2/proto/descriptor.proto" ||
               proto_filename == "google/protobuf/descriptor.proto" ||
               proto_filename == "src/google/protobuf/descriptor.proto") {
      return "upb/reflection/descriptor_bootstrap.h";
    } else {
      return "upb_generator/plugin_bootstrap.h";
    }
  }
  return StripExtension(proto_filename) + ".upb.h";
}

}  // namespace generator
}  // namespace upb
