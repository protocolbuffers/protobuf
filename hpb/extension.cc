// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/hpb/extension.h"

#include "upb/mini_table/extension_registry.h"

namespace hpb {
namespace internal {
upb_ExtensionRegistry* GetUpbExtensions(
    const ExtensionRegistry& extension_registry) {
  return extension_registry.registry_;
}

}  // namespace internal
}  // namespace hpb
