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
    if (IsDescriptorProto(proto_filename)) {
      return "upb/reflection/descriptor_bootstrap.h";
    } else {
      return "upb_generator/plugin_bootstrap.h";
    }
  }
  return StripExtension(proto_filename) + ".upb.h";
}

}  // namespace generator
}  // namespace upb
