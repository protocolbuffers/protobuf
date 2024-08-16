// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/any.h"

#include "absl/functional/any_invocable.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/message.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

bool AnyMetadata::PackFrom(Arena* arena, const Message& message) {
  return PackFrom(arena, message, kTypeGoogleApisComPrefix);
}

bool AnyMetadata::PackFrom(Arena* arena, const Message& message,
                           absl::string_view type_url_prefix) {
  type_url_->Set(GetTypeUrl(message.GetTypeName(), type_url_prefix), arena);
  return message.SerializeToString(value_->Mutable(arena));
}

bool AnyMetadata::UnpackTo(Message* message) const {
  if (!InternalIs(message->GetTypeName())) {
    return false;
  }
  return message->ParseFromString(value_->Get());
}

bool AnyMetadata::PackFromHelper(
    const Message& message, absl::string_view type_url_prefix,
    absl::AnyInvocable<bool(const MessageLite&, absl::string_view,
                            absl::string_view)>
        internal_pack_from) {
  return internal_pack_from(message, type_url_prefix, message.GetTypeName());
}

bool AnyMetadata::UnpackToHelper(
    Message* message, absl::AnyInvocable<bool(absl::string_view, MessageLite*)>
                          internal_unpack_to) {
  return internal_unpack_to(message->GetTypeName(), message);
}

bool GetAnyFieldDescriptors(const Message& message,
                            const FieldDescriptor** type_url_field,
                            const FieldDescriptor** value_field) {
  const Descriptor* descriptor = message.GetDescriptor();
  if (descriptor->full_name() != kAnyFullTypeName) {
    return false;
  }
  *type_url_field = descriptor->FindFieldByNumber(1);
  *value_field = descriptor->FindFieldByNumber(2);
  return (*type_url_field != nullptr &&
          (*type_url_field)->type() == FieldDescriptor::TYPE_STRING &&
          *value_field != nullptr &&
          (*value_field)->type() == FieldDescriptor::TYPE_BYTES);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
