// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdint>
#include <string>
#include <utility>

#include "absl/log/absl_check.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/strings.h"

static const google::protobuf::internal::ExtensionSet* GetExtensionSet(
    const google::protobuf::MessageLite* m) {
  const google::protobuf::internal::ExtensionSet* ext =
      google::protobuf::internal::PrivateAccess::GetExtensionSet(m);
  ABSL_DCHECK(ext != nullptr);
  return ext;
}

static google::protobuf::internal::ExtensionSet* GetExtensionSet(google::protobuf::MessageLite* m) {
  google::protobuf::internal::ExtensionSet* ext =
      google::protobuf::internal::PrivateAccess::GetExtensionSet(m);
  ABSL_DCHECK(ext != nullptr);
  return ext;
}

extern "C" {

bool proto2_rust_Message_has_extension(const google::protobuf::MessageLite* m,
                                       int32_t number) {
  return GetExtensionSet(m)->Has(number);
}

void proto2_rust_Message_clear_extension(google::protobuf::MessageLite* m,
                                         int32_t number) {
  GetExtensionSet(m)->ClearExtension(number);
}

#define DEFN_EXT_PRIMITIVE(rust_type, cpp_type)                               \
  void proto2_rust_Message_set_extension_##rust_type(                         \
      google::protobuf::MessageLite* m, int32_t number, int32_t type, cpp_type value) { \
    GetExtensionSet(m)->Set<cpp_type>(m->GetArena(), number, type, value,     \
                                      nullptr);                               \
  }                                                                           \
  cpp_type proto2_rust_Message_get_extension_##rust_type(                     \
      google::protobuf::MessageLite* m, int32_t number, cpp_type default_value) {       \
    return GetExtensionSet(m)->Get<cpp_type>(number, default_value);          \
  }

DEFN_EXT_PRIMITIVE(bool, bool)
DEFN_EXT_PRIMITIVE(float, float)
DEFN_EXT_PRIMITIVE(double, double)
DEFN_EXT_PRIMITIVE(int32, int32_t)
DEFN_EXT_PRIMITIVE(int64, int64_t)
DEFN_EXT_PRIMITIVE(uint32, uint32_t)
DEFN_EXT_PRIMITIVE(uint64, uint64_t)

#undef DEFN_EXT_PRIMITIVE

void proto2_rust_Message_set_extension_string(google::protobuf::MessageLite* m,
                                              int32_t number, int32_t type,
                                              std::string* value) {
  GetExtensionSet(m)->Set<std::string>(m->GetArena(), number, type,
                                       std::move(*value), nullptr);
  delete value;
}

google::protobuf::rust::PtrAndLen proto2_rust_Message_get_extension_string(
    google::protobuf::MessageLite* m, int32_t number,
    google::protobuf::rust::PtrAndLen default_value) {
  if (!proto2_rust_Message_has_extension(m, number)) {
    return default_value;
  }
  std::string fake_default;
  const std::string& value =
      GetExtensionSet(m)->Get<std::string>(number, fake_default);
  return google::protobuf::rust::PtrAndLen{value.data(), value.size()};
}

const google::protobuf::MessageLite* proto2_rust_Message_get_extension_message(
    const google::protobuf::MessageLite* m, int32_t number,
    const google::protobuf::MessageLite* default_instance) {
  return &GetExtensionSet(m)->GetMessage(m->GetArena(), number,
                                         *default_instance);
}

google::protobuf::MessageLite* proto2_rust_Message_mutable_extension_message(
    google::protobuf::MessageLite* m, int32_t number, int32_t type,
    const google::protobuf::MessageLite* default_instance) {
  return GetExtensionSet(m)->MutableMessage(m->GetArena(), number, type,
                                            *default_instance, nullptr);
}

}  // extern "C"
