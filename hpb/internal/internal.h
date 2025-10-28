// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_INTERNAL_INTERNAL_H__
#define GOOGLE_PROTOBUF_HPB_INTERNAL_INTERNAL_H__

#include "hpb/multibackend.h"
#if HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_UPB
#include "hpb/backend/upb/internal.h"
#elif HPB_INTERNAL_BACKEND == HPB_INTERNAL_BACKEND_CPP
#include "hpb/backend/cpp/internal.h"
#else
#error "Unsupported hpb backend"
#endif

#endif  // GOOGLE_PROTOBUF_HPB_INTERNAL_INTERNAL_H__
