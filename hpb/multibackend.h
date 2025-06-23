// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_MULTIBACKEND_H__
#define GOOGLE_PROTOBUF_HPB_MULTIBACKEND_H__

#define HPB_INTERNAL_BACKEND_UPB 1
#define HPB_INTERNAL_BACKEND_CPP 2

namespace hpb {
namespace internal {
namespace backend {
namespace upb {}
namespace cpp {}
}  // namespace backend
}  // namespace internal

#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
namespace backend = hpb::internal::backend::upb;
#elif HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_CPP
namespace backend = hpb::internal::backend::cpp;
#else
#error "Unsupported hpb backend"
#endif
}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_MULTIBACKEND_H__
