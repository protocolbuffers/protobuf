// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#ifndef UPB_PROTOS_PROTOS_H_
#define UPB_PROTOS_PROTOS_H_
#include "google/protobuf/hpb/hpb.h"
namespace protos {
namespace internal {
using hpb::internal::CreateMessageProxy;
using hpb::internal::ExtensionIdentifier;
using hpb::internal::GetArena;
using hpb::internal::GetInternalMsg;
using hpb::internal::PrivateAccess;
using hpb::internal::Serialize;
using hpb::internal::SetExtension;
}  // namespace internal


using hpb::Arena;
using hpb::Ptr;
}  // namespace protos
#endif
