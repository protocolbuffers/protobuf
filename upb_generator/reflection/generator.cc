// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "upb/reflection/def.hpp"
#include "upb_generator/plugin.h"
#include "upb_generator/reflection/context.h"
#include "upb_generator/reflection/header.h"
#include "upb_generator/reflection/source.h"

namespace upb {
namespace generator {
namespace reflection {
void GenerateFile(upb::FileDefPtr file, const Options& options,
                  Plugin* plugin) {
  GenerateReflectionHeader(file, options, plugin);
  GenerateReflectionSource(file, options, plugin);
}

bool ParseOptions(Plugin* plugin, Options* options) {
  for (const auto& pair : ParseGeneratorParameter(plugin->parameter())) {
    if (pair.first == "dllexport_decl") {
      options->dllexport_decl = pair.second;
    } else {
      plugin->SetError(absl::Substitute("Unknown parameter: $0", pair.first));
      return false;
    }
  }

  return true;
}

}  // namespace reflection
}  // namespace generator
}  // namespace upb

int main(int argc, char** argv) {
  upb::generator::Plugin plugin;
  upb::generator::reflection::Options options;
  if (!ParseOptions(&plugin, &options)) return 0;
  plugin.GenerateFiles([&](upb::FileDefPtr file) {
    upb::generator::reflection::GenerateFile(file, options, &plugin);
  });
  return 0;
}
