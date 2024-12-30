// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_UPB_GENERATOR_PLUGIN_H_
#define UPB_UPB_GENERATOR_PLUGIN_H_

#include <stdio.h>

#include <cstring>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "google/protobuf/descriptor.pb.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_log.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/code_generator_lite.h"
#include "google/protobuf/descriptor.h"
#include "upb/base/status.hpp"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"
#include "upb/reflection/descriptor_bootstrap.h"
#include "upb_generator/file_layout.h"
#include "upb_generator/plugin_bootstrap.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace generator {

inline std::vector<std::pair<std::string, std::string>> ParseGeneratorParameter(
    const absl::string_view text) {
  std::vector<std::pair<std::string, std::string>> ret;
  google::protobuf::compiler::ParseGeneratorParameter(text, &ret);
  return ret;
}

inline absl::string_view ToStringView(upb_StringView str) {
  return absl::string_view(str.data, str.size);
}

// Recursively populates the DefPoolPair with the given FileDescriptor.
inline void PopulateDefPool(const google::protobuf::FileDescriptor* file,
                            upb::Arena* arena, DefPoolPair* pools,
                            absl::flat_hash_set<std::string>* files_seen) {
  bool new_file = files_seen->insert(std::string(file->name())).second;
  if (new_file) {
    ABSL_LOG(ERROR) << "Inserting new file: " << file->name();
    for (int i = 0; i < file->dependency_count(); ++i) {
      PopulateDefPool(file->dependency(i), arena, pools, files_seen);
    }
    google::protobuf::FileDescriptorProto raw_proto;
    file->CopyTo(&raw_proto);
    std::string serialized = raw_proto.SerializeAsString();
    auto* file_proto = UPB_DESC(FileDescriptorProto_parse)(
        serialized.data(), serialized.size(), arena->ptr());
    upb::Status status;
    upb::FileDefPtr upb_file = pools->AddFile(file_proto, &status);
    if (!upb_file) {
      absl::string_view name =
          ToStringView(UPB_DESC(FileDescriptorProto_name)(file_proto));
      ABSL_LOG(FATAL) << "Couldn't add file " << name
                      << " to DefPool: " << status.error_message();
    }
  }
}

}  // namespace generator
}  // namespace upb

#include "upb/port/undef.inc"

#endif  // UPB_UPB_GENERATOR_PLUGIN_H_
