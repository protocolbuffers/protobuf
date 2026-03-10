// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdlib>
#include <string>

#include "absl/strings/cord.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/any.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/message_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

using UrlType = std::string;
using ValueType = std::string;

const char kAnyFullTypeName[] = "google.protobuf.Any";
const char kTypeGoogleApisComPrefix[] = "type.googleapis.com/";
const char kTypeGoogleProdComPrefix[] = "type.googleprod.com/";

std::string GetTypeUrl(absl::string_view message_name,
                       absl::string_view type_url_prefix) {
  if (!type_url_prefix.empty() &&
      type_url_prefix[type_url_prefix.size() - 1] == '/') {
    return absl::StrCat(type_url_prefix, message_name);
  } else {
    return absl::StrCat(type_url_prefix, "/", message_name);
  }
}

bool EndsWithTypeName(absl::string_view type_url, absl::string_view type_name) {
  return type_url.size() > type_name.size() &&
         type_url[type_url.size() - type_name.size() - 1] == '/' &&
         absl::EndsWith(type_url, type_name);
}

bool InternalPackFromLite(const MessageLite& message,
                          absl::string_view type_url_prefix,
                          absl::string_view type_name,
                          UrlType* PROTOBUF_NONNULL dst_url,
                          ValueType* PROTOBUF_NONNULL dst_value) {
  *dst_url = GetTypeUrl(type_name, type_url_prefix);
  return message.SerializeToString(dst_value);
}

bool InternalUnpackToLite(absl::string_view type_name,
                          absl::string_view type_url, const ValueType& value,
                          MessageLite* PROTOBUF_NONNULL dst_message) {
  if (!InternalIsLite(type_name, type_url)) {
    return false;
  }
  return dst_message->ParseFromString(value);
}

bool InternalIsLite(absl::string_view type_name, absl::string_view type_url) {
  return EndsWithTypeName(type_url, type_name);
}

bool ParseAnyTypeUrl(absl::string_view type_url,
                     std::string* PROTOBUF_NULLABLE url_prefix,
                     std::string* PROTOBUF_NONNULL full_type_name) {
  size_t pos = type_url.find_last_of('/');
  if (pos == std::string::npos || pos + 1 == type_url.size()) {
    return false;
  }
  if (url_prefix) {
    *url_prefix = std::string(type_url.substr(0, pos + 1));
  }
  *full_type_name = std::string(type_url.substr(pos + 1));
  return true;
}

bool ParseAnyTypeUrl(absl::string_view type_url,
                     std::string* PROTOBUF_NONNULL full_type_name) {
  return ParseAnyTypeUrl(type_url, nullptr, full_type_name);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
