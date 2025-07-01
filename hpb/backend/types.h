// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_BACKEND_TYPES_H__
#define GOOGLE_PROTOBUF_HPB_BACKEND_TYPES_H__

#include "hpb/multibackend.h"
#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
#include "upb/mem/arena.hpp"
#elif HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_CPP
#include "google/protobuf/arena.h"
#endif

namespace hpb {
namespace internal {
namespace backend {
#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
namespace upb {
using Arena = ::upb::Arena;
}
#elif HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_CPP
namespace cpp {
using Arena = google::protobuf::Arena;
}
#else
#error "Unsupported hpb backend"
#endif

}  // namespace backend
}  // namespace internal
}  // namespace hpb
#endif  // GOOGLE_PROTOBUF_HPB_BACKEND_TYPES_H__
