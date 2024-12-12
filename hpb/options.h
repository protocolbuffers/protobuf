// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_OPTIONS_H__
#define GOOGLE_PROTOBUF_HPB_OPTIONS_H__

#include <variant>

#include "google/protobuf/hpb/extension.h"
namespace hpb {

struct UpbParseOptions {
  int parse_options = 0;
  const ExtensionRegistry* extension_registry = nullptr;
};

struct CppParseOptions {};

using ParseOptions = std::variant<UpbParseOptions, CppParseOptions>;

}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_OPTIONS_H__
