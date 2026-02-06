#include "google/protobuf/map.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/functional/overload.h"
#include "absl/memory/memory.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/strings.h"

namespace google {
namespace protobuf {
namespace rust {
namespace {

using FfiMapValueTag = internal::UntypedMapBase::TypeKind;

// LINT.IfChange(map_ffi)
struct FfiMapValue {
  FfiMapValueTag tag;
  union {
    bool b;
    uint32_t u32;
    uint64_t u64;
    float f32;
    double f64;
    std::string* s;
    google::protobuf::MessageLite* message;
  };
};
// LINT.ThenChange(//depot/google3/third_party/protobuf/rust/cpp.rs:map_ffi)

template <typename T>
struct FromViewType {
  using type = T;
};

template <>
struct FromViewType<PtrAndLen> {
  using type = std::string;
};

template <typename Key>
using KeyMap = internal::KeyMapBase<
    internal::KeyForBase<typename FromViewType<Key>::type>>;

template <typename T>
T AsViewType(T t) {
  return t;
}

absl::string_view AsViewType(PtrAndLen key) { return key.AsStringView(); }

constexpr bool kHasFullRuntime = true;

void InitializeMessageValue(void* raw_ptr,
                            std::unique_ptr<MessageLite> prototype) {
  MessageLite* new_msg =
      internal::RustMapHelper::PlacementNew(prototype.get(), raw_ptr);
  // If we are working with a full (non-lite) proto, we reflectively swap the
  // value into place. Otherwise, we have to perform a copy.
  if constexpr (kHasFullRuntime) {
    auto* full_msg = DynamicCastMessage<Message>(new_msg);
    if (full_msg != nullptr) {
      full_msg->GetReflection()->Swap(
          full_msg, DynamicCastMessage<Message>(prototype.get()));
    } else {
      new_msg->CheckTypeAndMergeFrom(*prototype);
    }
  } else {
    new_msg->CheckTypeAndMergeFrom(*prototype);
  }
}

template <typename Key>
bool Insert(internal::UntypedMapBase* m, Key key, FfiMapValue value) {
  internal::NodeBase* node = internal::RustMapHelper::AllocNode(m);
  if constexpr (std::is_same<Key, PtrAndLen>::value) {
    key.PlacementNewString(node->GetVoidKey());
  } else {
    *static_cast<Key*>(node->GetVoidKey()) = key;
  }

  m->VisitValue(node, absl::Overload{
                          [&](bool* v) { *v = value.b; },
                          [&](uint32_t* v) { *v = value.u32; },
                          [&](uint64_t* v) { *v = value.u64; },
                          [&](float* v) { *v = value.f32; },
                          [&](double* v) { *v = value.f64; },
                          [&](std::string* str) {
                            new (str) std::string(std::move(*value.s));
                            delete value.s;
                          },
                          [&](MessageLite* msg) {
                            InitializeMessageValue(
                                msg, absl::WrapUnique(value.message));
                          },
                      });

  return internal::RustMapHelper::InsertOrReplaceNode(
      static_cast<KeyMap<Key>*>(m), node);
}

void PopulateMapValue(const internal::UntypedMapBase& map,
                      internal::NodeBase* node, FfiMapValue& output) {
  map.VisitValue(node, absl::Overload{
                           [&](const bool* v) {
                             output.tag = FfiMapValueTag::kBool;
                             output.b = *v;
                           },
                           [&](const uint32_t* v) {
                             output.tag = FfiMapValueTag::kU32;
                             output.u32 = *v;
                           },
                           [&](const uint64_t* v) {
                             output.tag = FfiMapValueTag::kU64;
                             output.u64 = *v;
                           },
                           [&](const float* v) {
                             output.tag = FfiMapValueTag::kFloat;
                             output.f32 = *v;
                           },
                           [&](const double* v) {
                             output.tag = FfiMapValueTag::kDouble;
                             output.f64 = *v;
                           },
                           [&](std::string* str) {
                             output.tag = FfiMapValueTag::kString;
                             output.s = str;
                           },
                           [&](MessageLite* msg) {
                             output.tag = FfiMapValueTag::kMessage;
                             output.message = msg;
                           },
                       });
}

template <typename Key>
bool Get(internal::UntypedMapBase* m, Key key, FfiMapValue* value) {
  auto* map_base = static_cast<KeyMap<Key>*>(m);
  auto result = internal::RustMapHelper::FindHelper(map_base, AsViewType(key));
  if (result.node == nullptr) {
    return false;
  }
  PopulateMapValue(*m, result.node, *value);
  return true;
}

template <typename Key>
bool Remove(internal::UntypedMapBase* m, Key key) {
  auto* map_base = static_cast<KeyMap<Key>*>(m);
  return internal::RustMapHelper::EraseImpl(map_base, AsViewType(key));
}

template <typename Key>
void IterGet(const internal::UntypedMapIterator* iter, Key* key,
             FfiMapValue* value) {
  internal::NodeBase* node = iter->node_;
  if constexpr (std::is_same<Key, PtrAndLen>::value) {
    const std::string* s = iter->m_->GetKey<std::string>(node);
    *key = PtrAndLen{s->data(), s->size()};
  } else {
    *key = *iter->m_->GetKey<Key>(node);
  }
  PopulateMapValue(*iter->m_, node, *value);
}

}  // namespace
}  // namespace rust
}  // namespace protobuf
}  // namespace google

extern "C" {

void proto2_rust_thunk_UntypedMapIterator_increment(
    google::protobuf::internal::UntypedMapIterator* iter) {
  iter->PlusPlus();
}

google::protobuf::internal::UntypedMapBase* proto2_rust_map_new(
    google::protobuf::rust::FfiMapValue key_prototype,
    google::protobuf::rust::FfiMapValue value_prototype) {
  return new google::protobuf::internal::UntypedMapBase(
      google::protobuf::internal::UntypedMapBase::GetTypeInfoDynamic(
          key_prototype.tag, value_prototype.tag,
          value_prototype.tag == google::protobuf::rust::FfiMapValueTag::kMessage
              ? value_prototype.message
              : nullptr));
}

size_t proto2_rust_map_size(google::protobuf::internal::UntypedMapBase* m) {
  return m->size();
}

google::protobuf::internal::UntypedMapIterator proto2_rust_map_iter(
    google::protobuf::internal::UntypedMapBase* m) {
  return m->begin();
}

void proto2_rust_map_free(google::protobuf::internal::UntypedMapBase* m) {
  m->ClearTable(m->arena(), /*reset=*/false);
  delete m;
}

void proto2_rust_map_clear(google::protobuf::internal::UntypedMapBase* m) {
  m->ClearTable(m->arena(), /*reset=*/true);
}

#define DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(cpp_type, suffix)                \
  bool proto2_rust_map_insert_##suffix(google::protobuf::internal::UntypedMapBase* m, \
                                       cpp_type key,                        \
                                       google::protobuf::rust::FfiMapValue value) {   \
    return google::protobuf::rust::Insert(m, key, value);                             \
  }                                                                         \
                                                                            \
  bool proto2_rust_map_get_##suffix(google::protobuf::internal::UntypedMapBase* m,    \
                                    cpp_type key,                           \
                                    google::protobuf::rust::FfiMapValue* value) {     \
    return google::protobuf::rust::Get(m, key, value);                                \
  }                                                                         \
                                                                            \
  bool proto2_rust_map_remove_##suffix(google::protobuf::internal::UntypedMapBase* m, \
                                       cpp_type key) {                      \
    return google::protobuf::rust::Remove(m, key);                                    \
  }                                                                         \
                                                                            \
  void proto2_rust_map_iter_get_##suffix(                                   \
      const google::protobuf::internal::UntypedMapIterator* iter, cpp_type* key,      \
      google::protobuf::rust::FfiMapValue* value) {                                   \
    return google::protobuf::rust::IterGet(iter, key, value);                         \
  }

DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(int32_t, i32)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(uint32_t, u32)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(int64_t, i64)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(uint64_t, u64)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(bool, bool)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(google::protobuf::rust::PtrAndLen, ProtoString)

}  // extern "C"
