// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/implicit_weak_message.h"

#include <string>

#include "google/protobuf/class_data.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"

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

class ImplicitWeakPrivateAccess {
 public:
  static constexpr auto GenerateParseTable(const ClassData* class_data) {
    return CreateStubTcParseTable<ImplicitWeakMessage,
                                  ImplicitWeakMessage::ParseImpl>(class_data);
  }

  static constexpr auto GenerateClassData() {
    return ClassData{
        nullptr,  // is_initialized (always true)
        ImplicitWeakMessage::MergeImpl,
        internal::MessageCreator(
            ImplicitWeakMessage::NewImpl<ImplicitWeakMessage>,
            sizeof(ImplicitWeakMessage), alignof(ImplicitWeakMessage)),
        &ImplicitWeakMessage::DestroyImpl,
        &ImplicitWeakMessage::ClearImpl,
        &ImplicitWeakMessage::ByteSizeLongImpl,
        &ImplicitWeakMessage::_InternalSerializeImpl,
        PROTOBUF_FIELD_OFFSET(ImplicitWeakMessage, cached_size_),
        /*type_name=*/""};
  }
};

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

void ImplicitWeakMessage::ClearImpl(MessageLite& msg) {
  static_cast<ImplicitWeakMessage&>(msg).data_->clear();
}

struct ImplicitWeakMessageDefaultType : MessageGlobalsBase {
  constexpr ImplicitWeakMessageDefaultType()
      : MessageGlobalsBase(ImplicitWeakPrivateAccess::GenerateClassData()),
        _default(ConstantInitialized{}),
        _table(ImplicitWeakPrivateAccess::GenerateParseTable(&class_data)) {}
  ~ImplicitWeakMessageDefaultType() {}
  union {
    alignas(kMaxMessageAlignment) ImplicitWeakMessage _default;  // NOLINT
  };
  TcParseTable<0> _table;  // NOLINT
};
static_assert(PROTOBUF_FIELD_OFFSET(ImplicitWeakMessageDefaultType, _default) ==
              MessageGlobalsBase::OffsetToDefault());

constexpr ImplicitWeakMessage::ImplicitWeakMessage(ConstantInitialized)
    : MessageLite(&implicit_weak_message_globals.class_data), data_(nullptr) {}

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ImplicitWeakMessageDefaultType
    implicit_weak_message_globals;

const ImplicitWeakMessage& ImplicitWeakMessage::default_instance() {
  return implicit_weak_message_globals._default;
}

const ClassData* ImplicitWeakMessage::GetClassData() const {
  return &implicit_weak_message_globals.class_data;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
