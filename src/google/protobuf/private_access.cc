// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/private_access.h"

#include "google/protobuf/extension_set.h"
#include "google/protobuf/message_lite.h"

namespace google {
namespace protobuf {
namespace internal {

ExtensionSet* PrivateAccess::GetExtensionSet(MessageLite& msg) {
  auto* tc_table = msg.GetTcParseTable();
  if (tc_table->extension_offset == 0) return nullptr;
  return reinterpret_cast<ExtensionSet*>(reinterpret_cast<char*>(&msg) +
                                         tc_table->extension_offset);
}

const ExtensionSet* PrivateAccess::GetExtensionSet(const MessageLite& msg) {
  auto* tc_table = msg.GetTcParseTable();
  if (tc_table->extension_offset == 0) return nullptr;
  return reinterpret_cast<const ExtensionSet*>(
      reinterpret_cast<const char*>(&msg) + tc_table->extension_offset);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
