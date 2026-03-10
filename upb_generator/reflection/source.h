// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_GENERATOR_REFLECTION_SOURCE_H__
#define GOOGLE_UPB_UPB_GENERATOR_REFLECTION_SOURCE_H__

#include "google/protobuf/compiler/code_generator.h"
#include "upb/reflection/def.hpp"
#include "upb_generator/reflection/context.h"

namespace upb {
namespace generator {
namespace reflection {

void GenerateReflectionSource(upb::FileDefPtr file, const Options& options,
                              google::protobuf::compiler::GeneratorContext* context);

}  // namespace reflection
}  // namespace generator
}  // namespace upb

#endif  // GOOGLE_UPB_UPB_GENERATOR_REFLECTION_SOURCE_H__
