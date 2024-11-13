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

// Since we could be merging Translation units, we must check if this was done
// before.
#ifndef PROTOBUF_PRAGMA_INIT_SEG_DONE
PROTOBUF_PRAGMA_INIT_SEG
#define PROTOBUF_PRAGMA_INIT_SEG_DONE
#endif

namespace google {
namespace protobuf {
namespace internal {

const char* ImplicitWeakMessage::ParseImpl(ImplicitWeakMessage* msg,
                                           const char* ptr, ParseContext* ctx) {
  return ctx->AppendString(ptr, msg->data_);
}

void ImplicitWeakMessage::MergeImpl(MessageLite& self,
                                    const MessageLite& other) {
  const std::string* other_data =
      static_cast<const ImplicitWeakMessage&>(other).data_;
  if (other_data != nullptr) {
    static_cast<ImplicitWeakMessage&>(self).data_->append(*other_data);
  }
}

struct ImplicitWeakMessageDefaultType {
  constexpr ImplicitWeakMessageDefaultType()
      : instance(ConstantInitialized{}) {}
  ~ImplicitWeakMessageDefaultType() {}
  union {
    ImplicitWeakMessage instance;
  };
};

constexpr ImplicitWeakMessage::ImplicitWeakMessage(ConstantInitialized)
    : MessageLite(class_data_.base()), data_(nullptr) {}

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ImplicitWeakMessageDefaultType
    implicit_weak_message_default_instance;

const ImplicitWeakMessage& ImplicitWeakMessage::default_instance() {
  return implicit_weak_message_default_instance.instance;
}

const TcParseTable<0> ImplicitWeakMessage::table_ =
    internal::CreateStubTcParseTable<ImplicitWeakMessage, ParseImpl>(
        class_data_.base());

constexpr ClassDataLite<1> ImplicitWeakMessage::class_data_ = {
    {
        &implicit_weak_message_default_instance.instance,
        &table_.header,
        nullptr,  // on_demand_register_arena_dtor
        nullptr,  // is_initialized (always true)
        MergeImpl,
        internal::MessageCreator(NewImpl<ImplicitWeakMessage>,
                                 sizeof(ImplicitWeakMessage),
                                 alignof(ImplicitWeakMessage)),
        &DestroyImpl,
        GetClearImpl<ImplicitWeakMessage>(),
        &ByteSizeLongImpl,
        &_InternalSerializeImpl,
        PROTOBUF_FIELD_OFFSET(ImplicitWeakMessage, cached_size_),
        true,
    },
    ""};

const ClassData* ImplicitWeakMessage::GetClassData() const {
  return class_data_.base();
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
