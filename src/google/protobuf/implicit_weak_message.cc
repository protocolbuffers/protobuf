// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/implicit_weak_message.h"

#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

PROTOBUF_PRAGMA_INIT_SEG

namespace google {
namespace protobuf {
namespace internal {

const char* ImplicitWeakMessage::ParseImpl(ImplicitWeakMessage* msg,
                                           const char* ptr, ParseContext* ctx) {
  return ctx->AppendString(ptr, msg->data_);
}

struct ImplicitWeakMessageDefaultType {
  constexpr ImplicitWeakMessageDefaultType()
      : instance(ConstantInitialized{}) {}
  ~ImplicitWeakMessageDefaultType() {}
  union {
    ImplicitWeakMessage instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ImplicitWeakMessageDefaultType
    implicit_weak_message_default_instance;

const ImplicitWeakMessage* ImplicitWeakMessage::default_instance() {
  return reinterpret_cast<ImplicitWeakMessage*>(
      &implicit_weak_message_default_instance);
}

const MessageLite::ClassData* ImplicitWeakMessage::GetClassData() const {
  struct Data {
    ClassData header;
    char name[1];
  };
  static const auto table =
      internal::CreateStubTcParseTable<ImplicitWeakMessage, ParseImpl>(
          &implicit_weak_message_default_instance.instance);
  static constexpr Data data = {
      {
          &table.header,
          nullptr,  // on_demand_register_arena_dtor
          nullptr,  // is_initialized (always true)
          PROTOBUF_FIELD_OFFSET(ImplicitWeakMessage, cached_size_),
          true,
      },
      ""};
  return &data.header;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
