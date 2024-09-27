#include "google/protobuf/map.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/log/absl_log.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/strings.h"

namespace google {
namespace protobuf {
namespace rust {
namespace {

// LINT.IfChange(map_key_category)
enum class MapKeyCategory : uint8_t {
  kOneByte = 0,
  kFourBytes = 1,
  kEightBytes = 2,
  kStdString = 3,
};
// LINT.ThenChange(//depot/google3/third_party/protobuf/rust/cpp.rs:map_key_category)

size_t KeySize(MapKeyCategory category) {
  switch (category) {
    case MapKeyCategory::kOneByte:
      return 1;
    case MapKeyCategory::kFourBytes:
      return 4;
    case MapKeyCategory::kEightBytes:
      return 8;
    case MapKeyCategory::kStdString:
      return sizeof(std::string);
    default:
      ABSL_DLOG(FATAL) << "Unexpected value of MapKeyCategory enum";
  }
}

enum class MapValueTag : uint8_t {
  kBool,
  kU32,
  kU64,
  kString,
  kMessage,
};

struct MapValue {
  MapValueTag tag;
  union {
    bool b;
    uint32_t u32;
    uint64_t u64;
    std::string* s;
    google::protobuf::MessageLite* message;
  };
};

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
void GetSizeAndAlignment(T, uint16_t* size, uint8_t* alignment) {
  *size = sizeof(T);
  *alignment = alignof(T);
}

template <>
void GetSizeAndAlignment<MapValue>(MapValue value, uint16_t* size,
                                   uint8_t* alignment) {
  switch (value.tag) {
    case MapValueTag::kBool:
      *size = sizeof(bool);
      *alignment = alignof(bool);
      break;
    case MapValueTag::kU32:
      *size = sizeof(uint32_t);
      *alignment = alignof(uint32_t);
      break;
    case MapValueTag::kU64:
      *size = sizeof(uint64_t);
      *alignment = alignof(uint64_t);
      break;
    case MapValueTag::kString:
      *size = sizeof(std::string);
      *alignment = alignof(std::string);
      break;
    case MapValueTag::kMessage:
      internal::RustMapHelper::GetSizeAndAlignment(value.message, size,
                                                   alignment);
      break;
    default:
      ABSL_DLOG(FATAL) << "Unexpected value of MapValue";
  }
}

template <typename ValueType>
internal::MapNodeSizeInfoT GetSizeInfo(size_t key_size, ValueType value) {
  // Each map node consists of a NodeBase followed by a std::pair<Key, Value>.
  // We need to compute the offset of the value and the total size of the node.
  size_t node_and_key_size = sizeof(internal::NodeBase) + key_size;
  uint16_t value_size;
  uint8_t value_alignment;
  GetSizeAndAlignment(value, &value_size, &value_alignment);
  // Round node_and_key_size up to the nearest multiple of value_alignment.
  uint16_t offset =
      (((node_and_key_size - 1) / value_alignment) + 1) * value_alignment;

  size_t overall_alignment = std::max(alignof(internal::NodeBase),
                                      static_cast<size_t>(value_alignment));
  // Round up size to nearest multiple of overall_alignment.
  size_t overall_size =
      (((offset + value_size - 1) / overall_alignment) + 1) * overall_alignment;

  return internal::RustMapHelper::MakeSizeInfo(overall_size, offset);
}

template <typename Key, typename Value>
internal::MapNodeSizeInfoT GetSizeInfo(Value value) {
  return GetSizeInfo(sizeof(Key), value);
}

template <typename Key>
void DestroyMapNode(internal::UntypedMapBase* m, internal::NodeBase* node,
                    internal::MapNodeSizeInfoT size_info,
                    bool destroy_message) {
  if constexpr (std::is_same<Key, PtrAndLen>::value) {
    static_cast<std::string*>(node->GetVoidKey())->~basic_string();
  }
  if (destroy_message) {
    internal::RustMapHelper::DestroyMessage(
        static_cast<MessageLite*>(node->GetVoidValue(size_info)));
  }
  internal::RustMapHelper::DeallocNode(m, node, size_info);
}

void InitializeMessageValue(void* raw_ptr, MessageLite* msg) {
  MessageLite* new_msg = internal::RustMapHelper::PlacementNew(msg, raw_ptr);
  auto* full_msg = DynamicCastMessage<Message>(new_msg);

  // If we are working with a full (non-lite) proto, we reflectively swap the
  // value into place. Otherwise, we have to perform a copy.
  if (full_msg != nullptr) {
    full_msg->GetReflection()->Swap(full_msg, DynamicCastMessage<Message>(msg));
  } else {
    new_msg->CheckTypeAndMergeFrom(*msg);
  }
  delete msg;
}

template <typename Key>
bool Insert(internal::UntypedMapBase* m, Key key, MapValue value) {
  internal::MapNodeSizeInfoT size_info =
      GetSizeInfo(sizeof(typename FromViewType<Key>::type), value);
  internal::NodeBase* node = internal::RustMapHelper::AllocNode(m, size_info);
  if constexpr (std::is_same<Key, PtrAndLen>::value) {
    new (node->GetVoidKey()) std::string(key.ptr, key.len);
  } else {
    *static_cast<Key*>(node->GetVoidKey()) = key;
  }

  void* value_ptr = node->GetVoidValue(size_info);
  switch (value.tag) {
    case MapValueTag::kBool:
      *static_cast<bool*>(value_ptr) = value.b;
      break;
    case MapValueTag::kU32:
      *static_cast<uint32_t*>(value_ptr) = value.u32;
      break;
    case MapValueTag::kU64:
      *static_cast<uint64_t*>(value_ptr) = value.u64;
      break;
    case MapValueTag::kString:
      new (value_ptr) std::string(std::move(*value.s));
      delete value.s;
      break;
    case MapValueTag::kMessage:
      InitializeMessageValue(value_ptr, value.message);
      break;
    default:
      ABSL_DLOG(FATAL) << "Unexpected value of MapValue";
  }

  node = internal::RustMapHelper::InsertOrReplaceNode(
      static_cast<KeyMap<Key>*>(m), node);
  if (node == nullptr) {
    return true;
  }
  DestroyMapNode<Key>(m, node, size_info, value.tag == MapValueTag::kMessage);
  return false;
}

template <typename Map, typename Key,
          typename = typename std::enable_if<
              !std::is_same<Key, google::protobuf::rust::PtrAndLen>::value>::type>
internal::RustMapHelper::NodeAndBucket FindHelper(Map* m, Key key) {
  return internal::RustMapHelper::FindHelper(
      m, static_cast<internal::KeyForBase<Key>>(key));
}

template <typename Map>
internal::RustMapHelper::NodeAndBucket FindHelper(Map* m,
                                                  google::protobuf::rust::PtrAndLen key) {
  return internal::RustMapHelper::FindHelper(
      m, absl::string_view(key.ptr, key.len));
}

void PopulateMapValue(MapValueTag tag, void* data, MapValue& output) {
  output.tag = tag;
  switch (tag) {
    case MapValueTag::kBool:
      output.b = *static_cast<const bool*>(data);
      break;
    case MapValueTag::kU32:
      output.u32 = *static_cast<const uint32_t*>(data);
      break;
    case MapValueTag::kU64:
      output.u64 = *static_cast<const uint64_t*>(data);
      break;
    case MapValueTag::kString:
      output.s = static_cast<std::string*>(data);
      break;
    case MapValueTag::kMessage:
      output.message = static_cast<MessageLite*>(data);
      break;
    default:
      ABSL_DLOG(FATAL) << "Unexpected MapValueTag";
  }
}

template <typename Key>
bool Get(internal::UntypedMapBase* m, MapValue prototype, Key key,
         MapValue* value) {
  internal::MapNodeSizeInfoT size_info =
      GetSizeInfo<typename FromViewType<Key>::type>(prototype);
  auto* map_base = static_cast<KeyMap<Key>*>(m);
  internal::RustMapHelper::NodeAndBucket result = FindHelper(map_base, key);
  if (result.node == nullptr) {
    return false;
  }
  PopulateMapValue(prototype.tag, result.node->GetVoidValue(size_info), *value);
  return true;
}

template <typename Key>
bool Remove(internal::UntypedMapBase* m, MapValue prototype, Key key) {
  internal::MapNodeSizeInfoT size_info =
      GetSizeInfo<typename FromViewType<Key>::type>(prototype);
  auto* map_base = static_cast<KeyMap<Key>*>(m);
  internal::RustMapHelper::NodeAndBucket result = FindHelper(map_base, key);
  if (result.node == nullptr) {
    return false;
  }
  internal::RustMapHelper::EraseNoDestroy(map_base, result.bucket, result.node);
  DestroyMapNode<Key>(m, result.node, size_info,
                      prototype.tag == MapValueTag::kMessage);
  return true;
}

template <typename Key>
void IterGet(const internal::UntypedMapIterator* iter, MapValue prototype,
             Key* key, MapValue* value) {
  internal::MapNodeSizeInfoT size_info =
      GetSizeInfo<typename FromViewType<Key>::type>(prototype);
  internal::NodeBase* node = iter->node_;
  if constexpr (std::is_same<Key, PtrAndLen>::value) {
    const std::string* s = static_cast<const std::string*>(node->GetVoidKey());
    *key = PtrAndLen{s->data(), s->size()};
  } else {
    *key = *static_cast<const Key*>(node->GetVoidKey());
  }
  PopulateMapValue(prototype.tag, node->GetVoidValue(size_info), *value);
}

void ClearMap(internal::UntypedMapBase* m, MapKeyCategory category,
              bool reset_table, MapValue prototype) {
  internal::MapNodeSizeInfoT size_info =
      GetSizeInfo(KeySize(category), prototype);
  if (internal::RustMapHelper::IsGlobalEmptyTable(m)) return;
  uint8_t bits = 0;
  if (category == MapKeyCategory::kStdString) {
    bits |= internal::RustMapHelper::kKeyIsString;
  }
  if (prototype.tag == MapValueTag::kString) {
    bits |= internal::RustMapHelper::kValueIsString;
  } else if (prototype.tag == MapValueTag::kMessage) {
    bits |= internal::RustMapHelper::kValueIsProto;
  }
  internal::RustMapHelper::ClearTable(
      m, internal::RustMapHelper::ClearInput{size_info, bits, reset_table,
                                             /* destroy_node = */ nullptr});
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

google::protobuf::internal::UntypedMapBase* proto2_rust_map_new() {
  return new google::protobuf::internal::UntypedMapBase(/* arena = */ nullptr);
}

void proto2_rust_map_free(google::protobuf::internal::UntypedMapBase* m,
                          google::protobuf::rust::MapKeyCategory category,
                          google::protobuf::rust::MapValue prototype) {
  google::protobuf::rust::ClearMap(m, category, /* reset_table = */ false, prototype);
  delete m;
}

void proto2_rust_map_clear(google::protobuf::internal::UntypedMapBase* m,
                           google::protobuf::rust::MapKeyCategory category,
                           google::protobuf::rust::MapValue prototype) {
  google::protobuf::rust::ClearMap(m, category, /* reset_table = */ true, prototype);
}

size_t proto2_rust_map_size(google::protobuf::internal::UntypedMapBase* m) {
  return m->size();
}

google::protobuf::internal::UntypedMapIterator proto2_rust_map_iter(
    google::protobuf::internal::UntypedMapBase* m) {
  return m->begin();
}

#define DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(cpp_type, suffix)                 \
  bool proto2_rust_map_insert_##suffix(google::protobuf::internal::UntypedMapBase* m,  \
                                       cpp_type key,                         \
                                       google::protobuf::rust::MapValue value) {       \
    return google::protobuf::rust::Insert(m, key, value);                              \
  }                                                                          \
                                                                             \
  bool proto2_rust_map_get_##suffix(                                         \
      google::protobuf::internal::UntypedMapBase* m, google::protobuf::rust::MapValue prototype, \
      cpp_type key, google::protobuf::rust::MapValue* value) {                         \
    return google::protobuf::rust::Get(m, prototype, key, value);                      \
  }                                                                          \
                                                                             \
  bool proto2_rust_map_remove_##suffix(google::protobuf::internal::UntypedMapBase* m,  \
                                       google::protobuf::rust::MapValue prototype,     \
                                       cpp_type key) {                       \
    return google::protobuf::rust::Remove(m, prototype, key);                          \
  }                                                                          \
                                                                             \
  void proto2_rust_map_iter_get_##suffix(                                    \
      const google::protobuf::internal::UntypedMapIterator* iter,                      \
      google::protobuf::rust::MapValue prototype, cpp_type* key,                       \
      google::protobuf::rust::MapValue* value) {                                       \
    return google::protobuf::rust::IterGet(iter, prototype, key, value);               \
  }

DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(int32_t, i32)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(uint32_t, u32)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(int64_t, i64)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(uint64_t, u64)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(bool, bool)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(google::protobuf::rust::PtrAndLen, ProtoString)

}  // extern "C"
