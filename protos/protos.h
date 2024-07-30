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
using hpb::internal::CreateMessage;
using hpb::internal::CreateMessageProxy;
using hpb::internal::DeepClone;
using hpb::internal::DeepCopy;
using hpb::internal::ExtensionIdentifier;
using hpb::internal::GetArena;
using hpb::internal::GetInternalMsg;
using hpb::internal::GetMiniTable;
using hpb::internal::GetOrPromoteExtension;
using hpb::internal::GetUpbExtensions;
using hpb::internal::HasExtensionOrUnknown;
using hpb::internal::MoveExtension;
using hpb::internal::PrivateAccess;
using hpb::internal::Serialize;
using hpb::internal::SetExtension;
}  // namespace internal
using hpb::ClearMessage;
using hpb::CloneMessage;
using hpb::CreateMessage;
using hpb::DeepCopy;
using hpb::Parse;
using hpb::Serialize;

using hpb::ClearExtension;
using hpb::ExtensionNotFoundError;
using hpb::ExtensionNumber;
using hpb::GetExtension;
using hpb::HasExtension;
using hpb::SetExtension;

using hpb::Arena;
using hpb::MessageAllocationError;
using hpb::Ptr;
}  // namespace protos
#endif
