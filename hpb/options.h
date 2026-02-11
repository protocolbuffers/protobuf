// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_OPTIONS_H__
#define GOOGLE_PROTOBUF_HPB_OPTIONS_H__

#include "hpb/extension.h"
#include "hpb/multibackend.h"

namespace hpb {

struct ParseOptions {
  // If true, the parsed proto may alias the input string instead of copying.
  // Aliased data could include string fields, unknown fields, and possibly
  // other data.
  //
  // REQUIRES: the input string outlives the resulting proto.
  bool alias_string = false;

#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
  // For the upb backend, the user can determine which extension registry
  // they wish to use. Unless there are compelling reasons to do otherwise,
  // we recommend using the generated registry, which uses linker arrays
  // and intelligently performs tree shaking when possible.
  const ExtensionRegistry& extension_registry =
      ExtensionRegistry::generated_registry();
#endif
};

inline ParseOptions DefaultParseOptions() { return ParseOptions(); }

#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
// The empty registry is provided here as a convenience for extant users.
// Prefer the generated registry.
inline ParseOptions ParseOptionsWithEmptyRegistry() {
  ParseOptions options{
      .alias_string = false,
      .extension_registry = ExtensionRegistry::empty_registry(),
  };
  return options;
}
#endif

}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_OPTIONS_H__
