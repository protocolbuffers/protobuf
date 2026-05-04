// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.upb.h"
#include <gtest/gtest.h>
#include "testing/fuzzing/fuzztest.h"
#include "upb/base/status.hpp"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.h"
#include "upb/reflection/def.hpp"

namespace upb_test {

// Fuzz the upb DefPool with arbitrary FileDescriptorSet protos, exercising
// dependency index validation.  This covers code paths in file_def.c that
// validate public_dependency and weak_dependency indices, and the accessors
// that use those indices to look up the deps array.
void FuzzDefPoolAddFile(const google::protobuf::FileDescriptorSet& set) {
  upb::DefPool defpool;

  for (const auto& file : set.file()) {
    std::string serialized;
    (void)file.SerializeToString(&serialized);

    upb::Arena arena;
    upb::Status status;
    google_protobuf_FileDescriptorProto* proto =
        google_protobuf_FileDescriptorProto_parse(
            serialized.data(), serialized.size(), arena.ptr());
    if (!proto) continue;

    upb::FileDefPtr file_def = defpool.AddFile(proto, &status);
    if (!file_def) continue;

    // Exercise the dependency accessors that use stored indices to index into
    // the deps array.  Before the lower-bound validation fix, a negative
    // public_dependency or weak_dependency value would pass the bounds check
    // and cause an out-of-bounds read here.
    for (int i = 0; i < upb_FileDef_PublicDependencyCount(file_def.ptr());
         i++) {
      upb_FileDef_PublicDependency(file_def.ptr(), i);
    }
    for (int i = 0; i < upb_FileDef_WeakDependencyCount(file_def.ptr()); i++) {
      upb_FileDef_WeakDependency(file_def.ptr(), i);
    }
  }
}
FUZZ_TEST(FuzzTest, FuzzDefPoolAddFile);

}  // namespace upb_test
