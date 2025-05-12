// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_ARENA_H__
#define GOOGLE_PROTOBUF_HPB_ARENA_H__

#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
#include "upb/mem/arena.hpp"
#elif HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_CPP
#include "google/protobuf/arena.h"
#else
#error "unsupported hpb backend"
#endif

namespace hpb {
using Arena = upb::Arena;
}

#endif  // GOOGLE_PROTOBUF_HPB_ARENA_H__
