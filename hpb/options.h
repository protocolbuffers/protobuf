// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_OPTIONS_H__
#define GOOGLE_PROTOBUF_HPB_OPTIONS_H__

#include "hpb/multibackend.h"

namespace hpb {

struct ParseOptions {
  bool alias_string = false;

#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
  bool generated_registry = true;
#endif
};

inline ParseOptions ParseOptionsDefault() { return ParseOptions(); }

#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
inline ParseOptions ParseOptionsWithEmptyRegistry() {
  ParseOptions options{
      .alias_string = false,
      .generated_registry = false,
  };
  return options;
}
#endif

}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_OPTIONS_H__
