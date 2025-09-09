// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/any.h"

#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/message.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

using UrlType = std::string;
using ValueType = std::string;

bool InternalPackFrom(const Message& message, UrlType* PROTOBUF_NONNULL dst_url,
                      ValueType* PROTOBUF_NONNULL dst_value) {
  return InternalPackFromLite(message, kTypeGoogleApisComPrefix,
                              message.GetTypeName(), dst_url, dst_value);
}

bool InternalPackFrom(const Message& message, absl::string_view type_url_prefix,
                      UrlType* PROTOBUF_NONNULL dst_url,
                      ValueType* PROTOBUF_NONNULL dst_value) {
  return InternalPackFromLite(message, type_url_prefix, message.GetTypeName(),
                              dst_url, dst_value);
}

bool InternalUnpackTo(absl::string_view type_url, const ValueType& value,
                      Message* PROTOBUF_NONNULL message) {
  return InternalUnpackToLite(message->GetTypeName(), type_url, value, message);
}

bool GetAnyFieldDescriptors(
    const Message& message,
    const FieldDescriptor* PROTOBUF_NULLABLE* PROTOBUF_NONNULL type_url_field,
    const FieldDescriptor* PROTOBUF_NULLABLE* PROTOBUF_NONNULL value_field) {
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
