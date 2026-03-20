
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "absl/log/absl_check.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/strings.h"

static size_t GetExtensionSetOffset(const google::protobuf::MessageLite* m) {
  size_t offset =
      google::protobuf::internal::GetClassData(*m)->tc_table->extension_offset;
  ABSL_DCHECK_NE(offset, 0);
  return offset;
}

template <typename T>
static inline const T* RefAt(const void* ptr, size_t offset) {
  return reinterpret_cast<const T*>(static_cast<const char*>(ptr) + offset);
}

template <typename T>
static inline T* RefAt(void* ptr, size_t offset) {
  return reinterpret_cast<T*>(static_cast<char*>(ptr) + offset);
}

static const google::protobuf::internal::ExtensionSet* GetExtensionSet(
    const google::protobuf::MessageLite* m) {
  return RefAt<google::protobuf::internal::ExtensionSet>(m, GetExtensionSetOffset(m));
}

static google::protobuf::internal::ExtensionSet* GetExtensionSet(google::protobuf::MessageLite* m) {
  return RefAt<google::protobuf::internal::ExtensionSet>(m, GetExtensionSetOffset(m));
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
  return google::protobuf::rust::PtrAndLen{
      .ptr = value.data(),
      .len = value.size(),
  };
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
