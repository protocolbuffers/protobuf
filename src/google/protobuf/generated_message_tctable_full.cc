// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <cstdint>

#include "google/protobuf/extension_set.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/message.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/unknown_field_set.h"
#include "google/protobuf/wire_format.h"

// must be last
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
using ::google::protobuf::internal::DownCast;

const char* TcParser::GenericFallback(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return GenericFallbackImpl<Message, UnknownFieldSet>(
      PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::ReflectionFallback(PROTOBUF_TC_PARAM_DECL) {
  bool must_fallback_to_generic = (ptr == nullptr);
  if (PROTOBUF_PREDICT_FALSE(must_fallback_to_generic)) {
    PROTOBUF_MUSTTAIL return GenericFallback(PROTOBUF_TC_PARAM_PASS);
  }

  SyncHasbits(msg, hasbits, table);
  uint32_t tag = data.tag();
  if (tag == 0 || (tag & 7) == WireFormatLite::WIRETYPE_END_GROUP) {
    ctx->SetLastTag(tag);
    return ptr;
  }

  auto* full_msg = DownCast<Message*>(msg);
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
  return WireFormat::_InternalParse(DownCast<Message*>(msg), ptr, ctx);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
