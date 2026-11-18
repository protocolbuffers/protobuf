// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdint>

#include "absl/base/optimization.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/unknown_field_set.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/wire_format_lite.h"

// must be last
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

const char* TcParser::GenericFallback(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return GenericFallbackImpl<Message>(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::ReflectionFallback(PROTOBUF_TC_PARAM_DECL) {
  SyncHasbits(msg, hasbits, table);
  uint32_t tag = data.tag();
  if (tag == 0 || (tag & 7) == WireFormatLite::WIRETYPE_END_GROUP) {
    ctx->SetLastTag(tag);
    return ptr;
  }

  auto* full_msg = DownCastMessage<Message>(msg);
  auto* descriptor = full_msg->GetDescriptor();
  auto* reflection = full_msg->GetReflection();
  int field_number = WireFormatLite::GetTagFieldNumber(tag);
  const FieldDescriptor* field = descriptor->FindFieldByNumber(field_number);

  // If that failed, check if the field is an extension.
  if (field == nullptr && descriptor->IsExtensionNumber(field_number)) {
    if (ctx->data().pool == nullptr) {
      field = reflection->FindKnownExtensionByNumber(field_number);
    } else {
      field = ctx->data().pool->FindExtensionByNumber(descriptor, field_number);
    }
  }

  return WireFormat::_InternalParseAndMergeField(full_msg, ptr, ctx, tag,
                                                 reflection, field);
}

const char* TcParser::ReflectionParseLoop(PROTOBUF_TC_PARAM_DECL) {
  (void)data;
  (void)table;
  (void)hasbits;
  // Call into the wire format reflective parse loop.
  return WireFormat::_InternalParse(DownCastMessage<Message>(msg), ptr, ctx);
}

const char* TcParser::MessageSetWireFormatParseLoop(
    PROTOBUF_TC_PARAM_NO_DATA_DECL) {
  PROTOBUF_MUSTTAIL return MessageSetWireFormatParseLoopImpl<Message>(
      PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
