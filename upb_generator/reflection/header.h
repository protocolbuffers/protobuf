// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_GENERATOR_REFLECTION_HEADER_H__
#define GOOGLE_UPB_UPB_GENERATOR_REFLECTION_HEADER_H__

#include <string>

#include "google/protobuf/compiler/code_generator.h"
#include "upb/reflection/def.hpp"
#include "upb_generator/reflection/context.h"

namespace upb::generator::reflection {

std::string DefHeaderFilename(upb::FileDefPtr file);

void GenerateReflectionHeader(upb::FileDefPtr file, const Options& options,
                              google::protobuf::compiler::GeneratorContext* context);

}  // namespace upb::generator::reflection

#endif  // GOOGLE_UPB_UPB_GENERATOR_REFLECTION_HEADER_H__
