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

constexpr auto ImplicitWeakMessage::InternalGenerateClassData_(
    const MessageLite& prototype, const TcParseTableBase* tc_table) {
  return ClassDataLite{
      ClassData{
          &prototype,
#ifndef PROTOBUF_MESSAGE_GLOBALS
          &table_.header,
#else
          tc_table,
#endif              // PROTOBUF_MESSAGE_GLOBALS
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
      /*type_name=*/""};
}

constexpr auto ImplicitWeakMessage::InternalGenerateParseTable_(
    const ClassData* class_data) {
  return CreateStubTcParseTable<ImplicitWeakMessage, ParseImpl>(class_data);
}

#ifndef PROTOBUF_MESSAGE_GLOBALS
struct ImplicitWeakMessageDefaultType : MessageGlobalsBase {
  constexpr ImplicitWeakMessageDefaultType()
      : _default(ConstantInitialized{}) {}
  ~ImplicitWeakMessageDefaultType() {}
  union {
    ImplicitWeakMessage _default;  // NOLINT
  };
};
#else
struct ImplicitWeakMessageDefaultType : MessageGlobalsBase {
  constexpr ImplicitWeakMessageDefaultType()
      : MessageGlobalsBase(ImplicitWeakMessage::InternalGenerateClassData_(
            _default, &implicit_weak_message_globals._table.header)),
        _default(ConstantInitialized{}),
        _table(
            ImplicitWeakMessage::InternalGenerateParseTable_(GetClassData())) {}
  ~ImplicitWeakMessageDefaultType() {}
  union {
    alignas(kMaxMessageAlignment) ImplicitWeakMessage _default;  // NOLINT
  };
  TcParseTable<0> _table;  // NOLINT
};
static_assert(PROTOBUF_FIELD_OFFSET(ImplicitWeakMessageDefaultType, _default) ==
              MessageGlobalsBase::OffsetToDefault());
#endif  // PROTOBUF_MESSAGE_GLOBALS

constexpr ImplicitWeakMessage::ImplicitWeakMessage(ConstantInitialized)
    : MessageLite(
#ifndef PROTOBUF_MESSAGE_GLOBALS
          class_data_.base()
#else
          implicit_weak_message_globals.GetClassData()
#endif  // PROTOBUF_MESSAGE_GLOBALS
              ),
      data_(nullptr) {
}

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ImplicitWeakMessageDefaultType
    implicit_weak_message_globals;

const ImplicitWeakMessage& ImplicitWeakMessage::default_instance() {
  return implicit_weak_message_globals._default;
}

#ifndef PROTOBUF_MESSAGE_GLOBALS
const TcParseTable<0> ImplicitWeakMessage::table_ =
    internal::CreateStubTcParseTable<ImplicitWeakMessage, ParseImpl>(
        class_data_.base());
constexpr ClassDataLite ImplicitWeakMessage::class_data_ =
    ImplicitWeakMessage::InternalGenerateClassData_(
        implicit_weak_message_globals._default);
#endif  // !PROTOBUF_MESSAGE_GLOBALS

const ClassData* ImplicitWeakMessage::GetClassData() const {
#ifndef PROTOBUF_MESSAGE_GLOBALS
  return class_data_.base();
#else
  return implicit_weak_message_globals.GetClassData();
#endif  // PROTOBUF_MESSAGE_GLOBALS
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
