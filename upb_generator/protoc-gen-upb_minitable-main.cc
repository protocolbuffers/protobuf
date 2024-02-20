// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>
#include <vector>

#include "absl/log/absl_log.h"
#include "upb/base/status.hpp"
#include "upb/base/string_view.h"
#include "upb_generator/common.h"
#include "upb_generator/file_layout.h"
#include "upb_generator/plugin.h"
#include "upb_generator/protoc-gen-upb_minitable.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace generator {

std::string SourceFilename(upb::FileDefPtr file) {
  return StripExtension(file.name()) + ".upb_minitable.c";
}

absl::string_view ToStringView(upb_StringView str) {
  return absl::string_view(str.data, str.size);
}

void GenerateFile(const DefPoolPair& pools, upb::FileDefPtr file,
                  Plugin* plugin) {
  Output h_output;
  WriteMiniTableHeader(pools, file, h_output);
  plugin->AddOutputFile(MiniTableHeaderFilename(file), h_output.output());

  Output c_output;
  WriteMiniTableSource(pools, file, c_output);
  plugin->AddOutputFile(SourceFilename(file), c_output.output());
}

bool ParseOptions(Plugin* plugin) {
  for (const auto& pair : ParseGeneratorParameter(plugin->parameter())) {
    if (pair.first == "experimental_strip_nonfunctional_codegen") {
      continue;
    } else {
      plugin->SetError(absl::Substitute("Unknown parameter: $0", pair.first));
      return false;
    }
  }

  return true;
}

int PluginMain(int argc, char** argv) {
  DefPoolPair pools;
  Plugin plugin;
  if (!ParseOptions(&plugin)) return 0;
  plugin.GenerateFilesRaw(
      [&](const UPB_DESC(FileDescriptorProto) * file_proto, bool generate) {
        upb::Status status;
        upb::FileDefPtr file = pools.AddFile(file_proto, &status);
        if (!file) {
          absl::string_view name =
              ToStringView(UPB_DESC(FileDescriptorProto_name)(file_proto));
          ABSL_LOG(FATAL) << "Couldn't add file " << name
                          << " to DefPool: " << status.error_message();
        }
        if (generate) GenerateFile(pools, file, &plugin);
      });
  return 0;
}

}  // namespace generator
}  // namespace upb

int main(int argc, char** argv) {
  return upb::generator::PluginMain(argc, argv);
}
