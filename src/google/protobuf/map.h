// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file defines the map container and its helpers to support protobuf maps.
//
// The Map and MapIterator types are provided by this header file.
// Please avoid using other types defined here, unless they are public
// types within Map or MapIterator, such as Map::value_type.

#ifndef GOOGLE_PROTOBUF_MAP_H__
#define GOOGLE_PROTOBUF_MAP_H__

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>  // To support Visual Studio 2008
#include <new>     // IWYU pragma: keep for ::operator new.
#include <string>
#include <type_traits>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/base/optimization.h"
#include "absl/base/prefetch.h"
#include "absl/container/btree_map.h"
#include "absl/hash/hash.h"
#include "absl/log/absl_check.h"
#include "absl/numeric/bits.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/field_with_arena.h"
#include "google/protobuf/generated_enum_util.h"
#include "google/protobuf/internal_metadata_locator.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"
#include "google/protobuf/wire_format_lite.h"


#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

template <typename Key, typename T>
class Map;

class MapIterator;

template <typename Enum>
struct is_proto_enum;

namespace rust {
struct PtrAndLen;
}  // namespace rust

namespace internal {
namespace v2 {
class TableDrivenMessage;
}  // namespace v2

template <typename Key, typename T>
class MapFieldLite;
class MapFieldBase;

template <typename Derived, typename Key, typename T>
class MapField;

struct MapTestPeer;
struct MapBenchmarkPeer;

template <typename Key, typename T>
class TypeDefinedMapFieldBase;

class GeneratedMessageReflection;

// The largest valid serialization for a message is INT_MAX, so we can't have
// more than 32-bits worth of elements.
using map_index_t = uint32_t;

// Internal type traits that can be used to define custom key/value types. These
// are only be specialized by protobuf internals, and never by users.
template <typename T, typename VoidT = void>
struct is_internal_map_key_type : std::false_type {};

template <typename T, typename VoidT = void>
struct is_internal_map_value_type : std::false_type {};

// To save on binary size and simplify generic uses of the map types we collapse
// signed/unsigned versions of the same sized integer to the unsigned version.
template <typename T, typename = void>
struct KeyForBaseImpl {
  using type = T;
};
template <typename T>
struct KeyForBaseImpl<T, std::enable_if_t<std::is_integral<T>::value &&
                                          std::is_signed<T>::value>> {
  using type = std::make_unsigned_t<T>;
};
template <typename T>
using KeyForBase = typename KeyForBaseImpl<T>::type;

// Default case: Not transparent.
// We use std::hash<key_type>/std::less<key_type> and all the lookup functions
// only accept `key_type`.
template <typename key_type>
struct TransparentSupport {
  static_assert(std::is_scalar<key_type>::value,
                "Should only be used for ints.");

  template <typename K>
  using key_arg = key_type;

  using ViewType = key_type;

  static key_type ToView(key_type v) { return v; }
};

// We add transparent support for std::string keys. We use
// absl::Hash<absl::string_view> as it supports the input types we care about.
// The lookup functions accept arbitrary `K`. This will include any key type
// that is convertible to absl::string_view.
template <>
struct TransparentSupport<std::string> {
  template <typename T>
  static absl::string_view ImplicitConvert(T&& str) {
    if constexpr (std::is_convertible<T, absl::string_view>::value) {
      absl::string_view res = str;
      return res;
    } else if constexpr (std::is_convertible<T, const std::string&>::value) {
      const std::string& ref = str;
      return ref;
    } else {
      return {str.data(), str.size()};
    }
  }

  template <typename K>
  using key_arg = K;

  using ViewType = absl::string_view;
  template <typename T>
  static ViewType ToView(const T& v) {
    return ImplicitConvert(v);
  }
};

struct NodeBase {
  // Align the node to allow KeyNode to predict the location of the key.
  // This way sizeof(NodeBase) contains any possible padding it was going to
  // have between NodeBase and the key.
  alignas(kMaxMessageAlignment) NodeBase* next;

  void* GetVoidKey() { return this + 1; }
  const void* GetVoidKey() const { return this + 1; }
};

constexpr size_t kGlobalEmptyTableSize = 1;
PROTOBUF_EXPORT extern NodeBase* const kGlobalEmptyTable[kGlobalEmptyTableSize];

class UntypedMapBase;

class UntypedMapIterator {
 public:
  // Invariants:
  // node_ is always correct. This is handy because the most common
  // operations are operator* and operator-> and they only use node_.
  // When node_ is set to a non-null value, all the other non-const fields
  // are updated to be correct also, but those fields can become stale
  // if the underlying map is modified.  When those fields are needed they
  // are rechecked, and updated if necessary.

  // We do not provide any constructors for this type. We need it to be a
  // trivial type to ensure that we can safely share it with Rust.

  // The definition of operator== is handled by the derived type. If we were
  // to do it in this class it would allow comparing iterators of different
  // map types.
  bool Equals(const UntypedMapIterator& other) const {
    return node_ == other.node_;
  }

  // The definition of operator++ is handled in the derived type. We would not
  // be able to return the right type from here.
  void PlusPlus();

  // Conversion to and from a typed iterator child class is used by FFI.
  template <class Iter>
  static UntypedMapIterator FromTyped(Iter it) {
    static_assert(
#if defined(__cpp_lib_is_layout_compatible) && \
    __cpp_lib_is_layout_compatible >= 201907L
        std::is_layout_compatible_v<Iter, UntypedMapIterator>,
#else
        sizeof(it) == sizeof(UntypedMapIterator),
#endif
        "Map iterator must not have extra state that the base class"
        "does not define.");
    return static_cast<UntypedMapIterator>(it);
  }

  template <class Iter>
  Iter ToTyped() const {
    return Iter(*this);
  }
  NodeBase* node_;
  const UntypedMapBase* m_;
  map_index_t bucket_index_;
};

// These properties are depended upon by Rust FFI.
static_assert(
    std::is_trivially_default_constructible<UntypedMapIterator>::value,
    "UntypedMapIterator must be trivially default-constructible.");
static_assert(std::is_trivially_copyable<UntypedMapIterator>::value,
              "UntypedMapIterator must be trivially copyable.");
static_assert(std::is_trivially_destructible<UntypedMapIterator>::value,
              "UntypedMapIterator must be trivially destructible.");
static_assert(std::is_standard_layout<UntypedMapIterator>::value,
              "UntypedMapIterator must be standard layout.");
static_assert(offsetof(UntypedMapIterator, node_) == 0,
              "node_ must be the first field of UntypedMapIterator.");
static_assert(sizeof(UntypedMapIterator) ==
                  sizeof(void*) * 2 +
                      std::max(sizeof(uint32_t), alignof(void*)),
              "UntypedMapIterator does not have the expected size for FFI");
static_assert(
    alignof(UntypedMapIterator) == std::max(alignof(void*), alignof(uint32_t)),
    "UntypedMapIterator does not have the expected alignment for FFI");

// Base class for all Map instantiations.
// This class holds all the data and provides the basic functionality shared
// among all instantiations.
// Having an untyped base class helps generic consumers (like the table-driven
// parser) by having non-template code that can handle all instantiations.
class PROTOBUF_EXPORT UntypedMapBase {
 public:
  using size_type = size_t;

  // Possible types that a key/value can take.
  // LINT.IfChange(map_ffi)
  enum class TypeKind : uint8_t {
    kBool,     // bool
    kU32,      // int32_t, uint32_t, enums
    kU64,      // int64_t, uint64_t
    kFloat,    // float
    kDouble,   // double
    kString,   // std::string
    kMessage,  // Derived from MessageLite
  };
  // LINT.ThenChange(//depot/google3/third_party/protobuf/rust/cpp.rs:map_ffi)

  template <typename T>
  static constexpr TypeKind StaticTypeKind() {
    if constexpr (std::is_same_v<T, bool>) {
      return TypeKind::kBool;
    } else if constexpr (std::is_same_v<T, int32_t> ||
                         std::is_same_v<T, uint32_t> || std::is_enum_v<T>) {
      static_assert(sizeof(T) == 4,
                    "Only enums with the right underlying type are supported.");
      return TypeKind::kU32;
    } else if constexpr (std::is_same_v<T, int64_t> ||
                         std::is_same_v<T, uint64_t>) {
      return TypeKind::kU64;
    } else if constexpr (std::is_same_v<T, float>) {
      return TypeKind::kFloat;
    } else if constexpr (std::is_same_v<T, double>) {
      return TypeKind::kDouble;
    } else if constexpr (std::is_same_v<T, std::string>) {
      return TypeKind::kString;
    } else if constexpr (std::is_base_of_v<MessageLite, T>) {
      return TypeKind::kMessage;
    } else {
      static_assert(false && sizeof(T));
    }
  }

  struct TypeInfo {
    // Equivalent to `sizeof(Node)` in the derived type.
    uint16_t node_size;
    // Equivalent to `offsetof(Node, kv.second)` in the derived type.
    uint8_t value_offset;
    uint8_t key_type : 4;
    uint8_t value_type : 4;

    TypeKind key_type_kind() const { return static_cast<TypeKind>(key_type); }
    TypeKind value_type_kind() const {
      return static_cast<TypeKind>(value_type);
    }
  };
  static_assert(sizeof(TypeInfo) == 4);

  static TypeInfo GetTypeInfoDynamic(
      TypeKind key_type, TypeKind value_type,
      const MessageLite* value_prototype_if_message);

  constexpr UntypedMapBase(InternalMetadataOffset offset, TypeInfo type_info)
      : num_elements_(0),
        num_buckets_(internal::kGlobalEmptyTableSize),
        resolver_(offset),
        type_info_(type_info),
        table_(const_cast<NodeBase**>(internal::kGlobalEmptyTable)) {}
  explicit constexpr UntypedMapBase(TypeInfo type_info)
      : UntypedMapBase(InternalMetadataOffset(), type_info) {}

  UntypedMapBase(const UntypedMapBase&) = delete;
  UntypedMapBase& operator=(const UntypedMapBase&) = delete;

  template <typename T>
  T* GetKey(NodeBase* node) const {
    // Debug check that `T` matches what we expect from the type info.
    ABSL_DCHECK_EQ(static_cast<int>(StaticTypeKind<T>()),
                   static_cast<int>(type_info_.key_type));
    return reinterpret_cast<T*>(node->GetVoidKey());
  }

  void* GetVoidValue(NodeBase* node) const {
    return reinterpret_cast<char*>(node) + type_info_.value_offset;
  }

  template <typename T>
  T* GetValue(NodeBase* node) const {
    // Debug check that `T` matches what we expect from the type info.
    ABSL_DCHECK_EQ(static_cast<int>(StaticTypeKind<T>()),
                   static_cast<int>(type_info_.value_type));
    return reinterpret_cast<T*>(GetVoidValue(node));
  }

  void ClearTable(Arena* arena, bool reset) {
    if (num_buckets_ == internal::kGlobalEmptyTableSize) return;
    ClearTableImpl(arena, reset);
  }

  // Space used for the table and nodes.
  size_t SpaceUsedExcludingSelfLong() const;

  TypeInfo type_info() const { return type_info_; }

#if defined(ABSL_HAVE_THREAD_SANITIZER)
  // Using type_info_ as an arbitrary member that we can read/write.
  void ConstAccess() const {
    auto t = type_info_;
    asm volatile("" : "+r"(t));
  }
  void MutableAccess() {
    auto t = type_info_;
    asm volatile("" : "+r"(t));
    type_info_ = t;
  }
#else
  void ConstAccess() const {}
  void MutableAccess() {}
#endif

 protected:
  // 16 bytes is the minimum useful size for the array cache in the arena.
  static constexpr map_index_t kMinTableSize = 16 / sizeof(void*);
  static constexpr map_index_t kMaxTableSize = map_index_t{1} << 31;

 public:
  Arena* arena() const {
    return ResolveArena<&UntypedMapBase::resolver_>(this);
  }

  void InternalSwap(UntypedMapBase* other) {
    std::swap(num_elements_, other->num_elements_);
    std::swap(num_buckets_, other->num_buckets_);
    std::swap(type_info_, other->type_info_);
    std::swap(table_, other->table_);
  }

  void UntypedMergeFrom(Arena* arena, const UntypedMapBase& other);
  void UntypedSwap(Arena* arena, UntypedMapBase& other, Arena* other_arena);

  static size_type max_size() {
    return std::numeric_limits<map_index_t>::max();
  }
  size_type size() const { return num_elements_; }
  bool empty() const { return size() == 0; }
  UntypedMapIterator begin() const;

  // We make this a static function to reduce the cost in MapField.
  // All the end iterators are singletons anyway.
  static UntypedMapIterator EndIterator() { return {nullptr, nullptr, 0}; }

  // Calls `f(k)` with the key of the node, where `k` is the appropriate type
  // according to the stored TypeInfo.
  template <typename F>
  auto VisitKey(NodeBase* node, F f) const;

  // Calls `f(v)` with the value of the node, where `v` is the appropriate type
  // according to the stored TypeInfo.
  // Messages are visited as `MessageLite`, and enums are visited as int32.
  template <typename F>
  auto VisitValue(NodeBase* node, F f) const;

  // As above, but calls `f(k, v)` for every node in the map.
  template <typename F>
  void VisitAllNodes(F f) const;

 protected:
  friend class MapFieldBase;
  friend class TcParser;
  friend struct MapTestPeer;
  friend struct MapBenchmarkPeer;
  friend class UntypedMapIterator;
  friend class RustMapHelper;

  // Calls `f(type_t)` where `type_t` is an unspecified type that has a `::type`
  // typedef in it representing the dynamic type of key/value of the node.
  template <typename F>
  auto VisitKeyType(F f) const;
  template <typename F>
  auto VisitValueType(F f) const;

  struct NodeAndBucket {
    NodeBase* node;
    map_index_t bucket;
  };

  void ClearTableImpl(Arena* arena, bool reset);

  // Returns whether we should insert after the head of the list. For
  // non-optimized builds, we randomly decide whether to insert right at the
  // head of the list or just after the head. This helps add a little bit of
  // non-determinism to the map ordering.
  bool ShouldInsertAfterHead(void* node) {
#ifdef NDEBUG
    (void)node;
    return false;
#else
    // Doing modulo with a prime mixes the bits more.
    return absl::HashOf(node, table_) % 13 > 6;
#endif
  }

  // Insert all the nodes in the list. `count` must be the number of nodes in
  // the list.
  // Last-one-wins when there are duplicate keys.
  void InsertOrReplaceNodes(Arena* arena, NodeBase* list, map_index_t count);

  // Alignment of the nodes is the same as alignment of NodeBase.
  NodeBase* AllocNode(Arena* arena) {
    return AllocNode(arena, type_info_.node_size);
  }

  NodeBase* AllocNode(Arena* arena, size_t node_size) {
    ABSL_DCHECK_EQ(arena, this->arena());
    return static_cast<NodeBase*>(arena == nullptr
                                      ? Allocate(node_size)
                                      : arena->AllocateAligned(node_size));
  }

  void DeallocNode(NodeBase* node) { DeallocNode(node, type_info_.node_size); }

  void DeallocNode(NodeBase* node, size_t node_size) {
    ABSL_DCHECK(arena() == nullptr);
    internal::SizedDelete(node, node_size);
  }

  void DeleteTable(Arena* arena, NodeBase** table, map_index_t n) {
    ABSL_DCHECK_EQ(arena, this->arena());
    if (arena != nullptr) {
      arena->ReturnArrayMemory(table, n * sizeof(NodeBase*));
    } else {
      internal::SizedDelete(table, n * sizeof(NodeBase*));
    }
  }

  NodeBase** CreateEmptyTable(Arena* arena, map_index_t n) {
    ABSL_DCHECK_GE(n, kMinTableSize);
    ABSL_DCHECK_EQ(n & (n - 1), 0u);
    ABSL_DCHECK_EQ(arena, this->arena());
    NodeBase** result =
        arena == nullptr
            ? static_cast<NodeBase**>(Allocate(n * sizeof(NodeBase*)))
            : Arena::CreateArray<NodeBase*>(arena, n);
    memset(result, 0, n * sizeof(result[0]));
    return result;
  }

  void DeleteNode(NodeBase* node);
  void DeleteList(NodeBase* list);

  map_index_t num_elements_;
  map_index_t num_buckets_;
  InternalMetadataResolver resolver_;
  TypeInfo type_info_;
  NodeBase** table_;  // an array with num_buckets_ entries
};

template <typename F>
auto UntypedMapBase::VisitKeyType(F f) const {
  switch (type_info_.key_type_kind()) {
    case TypeKind::kBool:
      return f(std::enable_if<true, bool>{});
    case TypeKind::kU32:
      return f(std::enable_if<true, uint32_t>{});
    case TypeKind::kU64:
      return f(std::enable_if<true, uint64_t>{});
    case TypeKind::kString:
      return f(std::enable_if<true, std::string>{});

    case TypeKind::kFloat:
    case TypeKind::kDouble:
    case TypeKind::kMessage:
    default:
      Unreachable();
  }
}

template <typename F>
auto UntypedMapBase::VisitValueType(F f) const {
  switch (type_info_.value_type_kind()) {
    case TypeKind::kBool:
      return f(std::enable_if<true, bool>{});
    case TypeKind::kU32:
      return f(std::enable_if<true, uint32_t>{});
    case TypeKind::kU64:
      return f(std::enable_if<true, uint64_t>{});
    case TypeKind::kFloat:
      return f(std::enable_if<true, float>{});
    case TypeKind::kDouble:
      return f(std::enable_if<true, double>{});
    case TypeKind::kString:
      return f(std::enable_if<true, std::string>{});
    case TypeKind::kMessage:
      return f(std::enable_if<true, MessageLite>{});

    default:
      Unreachable();
  }
}

template <typename F>
void UntypedMapBase::VisitAllNodes(F f) const {
  VisitKeyType([&](auto key_type) {
    VisitValueType([&](auto value_type) {
      for (auto it = begin(); !it.Equals(EndIterator()); it.PlusPlus()) {
        f(GetKey<typename decltype(key_type)::type>(it.node_),
          GetValue<typename decltype(value_type)::type>(it.node_));
      }
    });
  });
}

template <typename F>
auto UntypedMapBase::VisitKey(NodeBase* node, F f) const {
  return VisitKeyType([&](auto key_type) {
    return f(GetKey<typename decltype(key_type)::type>(node));
  });
}

template <typename F>
auto UntypedMapBase::VisitValue(NodeBase* node, F f) const {
  return VisitValueType([&](auto value_type) {
    return f(GetValue<typename decltype(value_type)::type>(node));
  });
}

inline UntypedMapIterator UntypedMapBase::begin() const {
  map_index_t bucket_index;
  NodeBase* node;
  map_index_t index_of_first_non_null = 0;
  while (index_of_first_non_null != num_buckets_) {
    if (table_[index_of_first_non_null] == nullptr) {
      ++index_of_first_non_null;
    } else {
      break;
    }
  }

  if (index_of_first_non_null == num_buckets_) {
    bucket_index = 0;
    node = nullptr;
  } else {
    bucket_index = index_of_first_non_null;
    node = table_[bucket_index];
    PROTOBUF_ASSUME(node != nullptr);
  }
  return UntypedMapIterator{node, this, bucket_index};
}

inline void UntypedMapIterator::PlusPlus() {
  if (node_->next != nullptr) {
    node_ = node_->next;
    return;
  }

  for (map_index_t i = bucket_index_ + 1; i < m_->num_buckets_; ++i) {
    NodeBase* node = m_->table_[i];
    if (node == nullptr) continue;
    node_ = node;
    bucket_index_ = i;
    return;
  }

  node_ = nullptr;
  bucket_index_ = 0;
}

// Base class used by TcParser to extract the map object from a map field.
// We keep it here to avoid a dependency into map_field.h from the main TcParser
// code, since that would bring in Message too.
class MapFieldBaseForParse {
 public:
  const UntypedMapBase& GetMap() const {
    const void* p = prototype_or_payload_.load(std::memory_order_acquire);
    // If this instance has a payload, then it might need sync'n.
    if (ABSL_PREDICT_FALSE(IsPayload(p))) {
      sync_map_with_repeated.load(std::memory_order_relaxed)(*this, false);
    }
    return GetMapRaw();
  }

  UntypedMapBase* MutableMap() {
    const void* p = prototype_or_payload_.load(std::memory_order_acquire);
    // If this instance has a payload, then it might need sync'n.
    if (ABSL_PREDICT_FALSE(IsPayload(p))) {
      sync_map_with_repeated.load(std::memory_order_relaxed)(*this, true);
    }
    return &GetMapRaw();
  }

 protected:
  static constexpr size_t MapOffset() { return sizeof(MapFieldBaseForParse); }

  // See assertion in TypeDefinedMapFieldBase::TypeDefinedMapFieldBase()
  const UntypedMapBase& GetMapRaw() const {
    return *reinterpret_cast<const UntypedMapBase*>(
        reinterpret_cast<const char*>(this) + MapOffset());
  }
  UntypedMapBase& GetMapRaw() {
    return *reinterpret_cast<UntypedMapBase*>(reinterpret_cast<char*>(this) +
                                              MapOffset());
  }

  // Injected from map_field.cc once we need to use it.
  // We can't have a strong dep on it because it would cause protobuf_lite to
  // depend on reflection.
  using SyncFunc = void (*)(const MapFieldBaseForParse&, bool is_mutable);
  static std::atomic<SyncFunc> sync_map_with_repeated;

  // The prototype is a `Message`, but due to restrictions on constexpr in the
  // codegen we are receiving it as `void` during constant evaluation.
  explicit constexpr MapFieldBaseForParse(const void* prototype_as_void)
      : prototype_or_payload_(prototype_as_void) {}

  explicit MapFieldBaseForParse(const Message* prototype)
      : prototype_or_payload_(prototype) {}

  ~MapFieldBaseForParse() = default;

  static constexpr uintptr_t kHasPayloadBit = 1;

  static bool IsPayload(const void* p) {
    return reinterpret_cast<uintptr_t>(p) & kHasPayloadBit;
  }

  mutable std::atomic<const void*> prototype_or_payload_;
};

// The value might be of different signedness, so use memcpy to extract it.
template <typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
T ReadKey(const void* ptr) {
  T out;
  memcpy(&out, ptr, sizeof(T));
  return out;
}

template <typename T, std::enable_if_t<!std::is_integral<T>::value, int> = 0>
const T& ReadKey(const void* ptr) {
  return *reinterpret_cast<const T*>(ptr);
}

template <typename Key>
struct KeyNode : NodeBase {
  static constexpr size_t kOffset = sizeof(NodeBase);
  decltype(auto) key() const { return ReadKey<Key>(GetVoidKey()); }
};

inline map_index_t Hash(absl::string_view k, void* salt) {
  // Note: we could potentially also use CRC32-based hashing here.
  return absl::HashOf(k, salt);
}
inline map_index_t Hash(uint64_t k, void* salt) {
  if constexpr (!HasCrc32()) return absl::HashOf(k, salt);
  uintptr_t salt_int = reinterpret_cast<uintptr_t>(salt);
  // Note: Crc32(salt_int, k) causes the random iteration order test to fail so
  // we also rotate.
  return Crc32(salt_int, absl::rotr(k, salt_int & 0x3f));
}

// KeyMapBase is a chaining hash map.
// The implementation doesn't need the full generality of unordered_map,
// and it doesn't have it.  More bells and whistles can be added as needed.
// Some implementation details:
// 1. The number of buckets is a power of two.
// 2. As is typical for hash_map and such, the Keys and Values are always
//    stored in linked list nodes.  Pointers to elements are never invalidated
//    until the element is deleted.
// 3. Mutations to a map do not invalidate the map's iterators, pointers to
//    elements, or references to elements.
// 4. Except for erase(iterator), any non-const method can reorder iterators.

template <typename Key>
class KeyMapBase : public UntypedMapBase {
  static_assert(!std::is_signed<Key>::value || !std::is_integral<Key>::value,
                "");

  using TS = TransparentSupport<Key>;

 public:
  using UntypedMapBase::UntypedMapBase;

 protected:
  using KeyNode = internal::KeyNode<Key>;

 protected:
  friend UntypedMapBase;
  friend class MapFieldBase;
  friend class TcParser;
  friend struct MapTestPeer;
  friend struct MapBenchmarkPeer;
  friend class RustMapHelper;

  Key* GetKey(NodeBase* node) const {
    return UntypedMapBase::GetKey<Key>(node);
  }

  PROTOBUF_NOINLINE size_type EraseImpl(Arena* arena, map_index_t b,
                                        KeyNode* node, bool do_destroy) {
    ABSL_DCHECK_EQ(arena, this->arena());

    // Force bucket_index to be in range.
    b &= (num_buckets_ - 1);

    const auto find_prev = [&] {
      NodeBase** prev = table_ + b;
      for (; *prev != nullptr && *prev != node; prev = &(*prev)->next) {
      }
      return prev;
    };

    NodeBase** prev = find_prev();
    if (*prev == nullptr) {
      // The bucket index is wrong. The table was modified since the iterator
      // was made, so let's find the new bucket.
      b = FindHelper(TS::ToView(node->key())).bucket;
      prev = find_prev();
    }
    ABSL_DCHECK_EQ(*prev, node);
    *prev = (*prev)->next;

    --num_elements_;

    if (arena == nullptr && do_destroy) {
      DeleteNode(node);
    }

    // To allow for the other overload of EraseImpl to do a tail call.
    return 1;
  }

  PROTOBUF_NOINLINE size_type EraseImpl(Arena* arena, typename TS::ViewType k) {
    if (auto result = FindHelper(k); result.node != nullptr) {
      return EraseImpl(arena, result.bucket, static_cast<KeyNode*>(result.node),
                       /*do_destroy=*/true);
    }
    return 0;
  }

  NodeAndBucket FindHelper(typename TS::ViewType k) const {
    AssertLoadFactor();
    map_index_t b = BucketNumber(k);
    for (auto* node = table_[b]; node != nullptr; node = node->next) {
      if (TS::ToView(static_cast<KeyNode*>(node)->key()) == k) {
        return {node, b};
      }
    }
    return {nullptr, b};
  }

  // Insert the given node.
  // If the key is a duplicate, it inserts the new node and deletes the old one.
  bool InsertOrReplaceNode(Arena* arena, KeyNode* node) {
    ABSL_DCHECK_EQ(arena, this->arena());

    bool is_new = true;
    auto p = this->FindHelper(node->key());
    map_index_t b = p.bucket;
    if (ABSL_PREDICT_FALSE(p.node != nullptr)) {
      EraseImpl(arena, p.bucket, static_cast<KeyNode*>(p.node),
                /*do_destroy=*/true);
      is_new = false;
    } else if (ResizeIfLoadIsOutOfRange(arena, num_elements_ + 1)) {
      b = BucketNumber(node->key());  // bucket_number
    }
    InsertUnique(b, node);
    ++num_elements_;
    return is_new;
  }

  // Insert the given nodes.
  // On duplicates we discard the previous values.
  void InsertOrReplaceNodes(Arena* arena, KeyNode* list, map_index_t count) {
    ResizeIfLoadIsOutOfRangeForMultiInsert(arena, num_elements_ + count);

    map_index_t new_size = num_elements_;

    Inserter inserter(this, table_, num_buckets_);
    NodeBase* list_to_delete = nullptr;

    for (map_index_t i = 0; i < count; ++i) {
      ABSL_DCHECK_NE(list, nullptr);
      auto* node_to_insert = list;
      list = static_cast<KeyNode*>(list->next);

      const map_index_t b = inserter.BucketNumber(node_to_insert);
      for (NodeBase** node_prev = &table_[b];;
           node_prev = &(*node_prev)->next) {
        KeyNode* n = static_cast<KeyNode*>(*node_prev);

        if (n == nullptr) {
          // Reached the end without finding anything. Insert it.
          inserter.InsertUnique(node_to_insert, b);
          ++new_size;
          break;
        }

        if (ABSL_PREDICT_FALSE(n->key() == node_to_insert->key())) {
          // Let's just replace the node right there.
          *node_prev = node_to_insert;
          node_to_insert->next = n->next;
          // And append this node to a list to delete later.
          n->next = list_to_delete;
          list_to_delete = n;
          break;
        }
      }
    }

    num_elements_ = new_size;

    if (ABSL_PREDICT_FALSE(arena == nullptr && list_to_delete != nullptr)) {
      DeleteList(list_to_delete);
    }
  }

  // Insert the given Node in bucket b.  If that would make bucket b too big,
  // and bucket b is not a tree, create a tree for buckets b.
  // Requires count(*KeyPtrFromNodePtr(node)) == 0 and that b is the correct
  // bucket.  num_elements_ is not modified.
  void InsertUnique(map_index_t b, KeyNode* node) {
    // In practice, the code that led to this point may have already
    // determined whether we are inserting into an empty list, a short list,
    // or whatever.  But it's probably cheap enough to recompute that here;
    // it's likely that we're inserting into an empty or short list.
    ABSL_DCHECK(FindHelper(TS::ToView(node->key())).node == nullptr);
    AssertLoadFactor();
    auto*& head = table_[b];
    if (head == nullptr) {
      head = node;
      node->next = nullptr;
    } else if (ShouldInsertAfterHead(node)) {
      node->next = head->next;
      head->next = node;
    } else {
      node->next = head;
      head = node;
    }
  }

  // Have it a separate function for testing.
  static size_type CalculateHiCutoff(size_type num_buckets) {
    // We want the high cutoff to follow this rules:
    //  - When num_buckets_ == kGlobalEmptyTableSize, then make it 0 to force an
    //    allocation.
    //  - When num_buckets_ < 8, then make it num_buckets_ to avoid
    //    a reallocation. A large load factor is not that important on small
    //    tables and saves memory.
    //  - Otherwise, make it 75% of num_buckets_.
    return num_buckets - num_buckets / 16 * 4 - num_buckets % 2;
  }

  // For a particular size, calculate the lowest capacity `cap` where
  // `size <= CalculateHiCutoff(cap)`.
  static size_type CalculateCapacityForSize(size_type size) {
    ABSL_DCHECK_NE(size, 0u);

    if (size > kMaxTableSize / 2) {
      return kMaxTableSize;
    }

    size_t capacity = size_type{1} << (std::numeric_limits<size_type>::digits -
                                       absl::countl_zero(size - 1));

    if (size > CalculateHiCutoff(capacity)) {
      capacity *= 2;
    }

    return std::max<size_type>(capacity, kMinTableSize);
  }

  void AssertLoadFactor() const {
    ABSL_DCHECK_LE(num_elements_, CalculateHiCutoff(num_buckets_));
  }

  // Returns whether it did resize.  Currently this is only used when
  // num_elements_ increases, though it could be used in other situations.
  // It checks for load too low as well as load too high: because any number
  // of erases can occur between inserts, the load could be as low as 0 here.
  // Resizing to a lower size is not always helpful, but failing to do so can
  // destroy the expected big-O bounds for some operations. By having the
  // policy that sometimes we resize down as well as up, clients can easily
  // keep O(size()) = O(number of buckets) if they want that.
  bool ResizeIfLoadIsOutOfRange(Arena* arena, size_type new_size) {
    ABSL_DCHECK_EQ(arena, this->arena());

    const size_type hi_cutoff = CalculateHiCutoff(num_buckets_);
    const size_type lo_cutoff = hi_cutoff / 4;
    // We don't care how many elements are in trees.  If a lot are,
    // we may resize even though there are many empty buckets.  In
    // practice, this seems fine.
    if (ABSL_PREDICT_FALSE(new_size > hi_cutoff)) {
      if (num_buckets_ <= max_size() / 2) {
        Resize(arena, kMinTableSize > kGlobalEmptyTableSize * 2
                          ? std::max(kMinTableSize, num_buckets_ * 2)
                          : num_buckets_ * 2);
        return true;
      }
    } else if (ABSL_PREDICT_FALSE(new_size <= lo_cutoff &&
                                  num_buckets_ > kMinTableSize)) {
      size_type lg2_of_size_reduction_factor = 1;
      // It's possible we want to shrink a lot here... size() could even be 0.
      // So, estimate how much to shrink by making sure we don't shrink so
      // much that we would need to grow the table after a few inserts.
      const size_type hypothetical_size = new_size * 5 / 4 + 1;
      while ((hypothetical_size << (1 + lg2_of_size_reduction_factor)) <
             hi_cutoff) {
        ++lg2_of_size_reduction_factor;
      }
      size_type new_num_buckets = std::max<size_type>(
          kMinTableSize, num_buckets_ >> lg2_of_size_reduction_factor);
      if (new_num_buckets != num_buckets_) {
        Resize(arena, new_num_buckets);
        return true;
      }
    }
    return false;
  }

  void ResizeIfLoadIsOutOfRangeForMultiInsert(Arena* arena,
                                              size_type new_size) {
    if (const map_index_t needed_capacity = CalculateCapacityForSize(new_size);
        needed_capacity != this->num_buckets_) {
      Resize(arena, std::max(kMinTableSize, needed_capacity));
    }
  }

  // Interpret `head` as a linked list and insert all the nodes into `this`.
  // REQUIRES: this->empty()
  // REQUIRES: the input nodes have unique keys
  PROTOBUF_NOINLINE void MergeIntoEmpty(Arena* arena, NodeBase* head,
                                        size_t num_nodes) {
    ABSL_DCHECK_EQ(size(), size_t{0});
    ABSL_DCHECK_NE(num_nodes, size_t{0});
    ABSL_DCHECK_EQ(arena, this->arena());

    ResizeIfLoadIsOutOfRangeForMultiInsert(arena, num_nodes);
    num_elements_ = num_nodes;
    AssertLoadFactor();
    Inserter inserter(this, table_, num_buckets_);
    for (size_t i = 0; i < num_nodes; ++i) {
      KeyNode* node = static_cast<KeyNode*>(head);
      ABSL_DCHECK_NE(node, nullptr);
      head = head->next;
      absl::PrefetchToLocalCacheNta(head);
      inserter.InsertUnique(node);
    }
  }

  // Resize to the given number of buckets.
  void Resize(Arena* arena, map_index_t new_num_buckets) {
    ABSL_DCHECK_GE(new_num_buckets, kMinTableSize);
    ABSL_DCHECK(absl::has_single_bit(new_num_buckets));
    ABSL_DCHECK_EQ(arena, this->arena());

    if (num_buckets_ == kGlobalEmptyTableSize) {
      // This is the global empty array.
      // Just overwrite with a new one. No need to transfer or free anything.
      num_buckets_ = new_num_buckets;
      table_ = CreateEmptyTable(arena, num_buckets_);
      return;
    }

    ABSL_DCHECK_GE(new_num_buckets, kMinTableSize);
    const auto old_table = table_;
    const map_index_t old_table_size = num_buckets_;
    num_buckets_ = new_num_buckets;
    NodeBase** table = table_ = CreateEmptyTable(arena, num_buckets_);
    Inserter inserter(this, table, new_num_buckets);
    for (map_index_t i = 0; i < old_table_size; ++i) {
      for (KeyNode* node = static_cast<KeyNode*>(old_table[i]);
           node != nullptr;) {
        auto* next = static_cast<KeyNode*>(node->next);
        inserter.InsertUnique(node);
        node = next;
      }
    }
    DeleteTable(arena, old_table, old_table_size);
    AssertLoadFactor();
  }

  // Caches all the relevant values of `UntypedMapBase` to hold a copy on the
  // stack and avoid reloads after every write.
  // It allows inserting multiple nodes in a row with reduced cost.
  class Inserter {
   public:
    explicit Inserter(KeyMapBase* map, NodeBase** table,
                      map_index_t num_buckets)
        : table_(table), mask_(num_buckets - 1), map_(map) {}

    map_index_t BucketNumber(KeyNode* node) const {
      return Hash(node->key(), table_) & mask_;
    }

    void InsertUnique(KeyNode* node, map_index_t bucket) {
      ABSL_DCHECK_EQ(bucket, BucketNumber(node));
      auto*& head = table_[bucket];
      if (head != nullptr && map_->ShouldInsertAfterHead(node)) {
        node->next = head->next;
        head->next = node;
      } else {
        node->next = head;
        head = node;
      }
    }

    void InsertUnique(KeyNode* node) { InsertUnique(node, BucketNumber(node)); }

   private:
    NodeBase** const table_;
    const map_index_t mask_;
    KeyMapBase* const map_;
  };

  map_index_t BucketNumber(typename TS::ViewType k) const {
    return Hash(k, table_) & (num_buckets_ - 1);
  }
};

template <typename T, typename K>
bool InitializeMapKey(T*, K&&, Arena*) {
  return false;
}


// The purpose of this class is to give the Rust implementation visibility into
// some of the internals of C++ proto maps. We need access to these internals
// to be able to implement Rust map operations without duplicating the same
// functionality for every message type.
class RustMapHelper {
 public:
  using NodeAndBucket = UntypedMapBase::NodeAndBucket;

  static NodeBase* AllocNode(UntypedMapBase* m) {
    return m->AllocNode(m->arena());
  }

  static void DeleteNode(UntypedMapBase* m, NodeBase* node) {
    return m->DeleteNode(node);
  }

  template <typename Map, typename Key>
  static NodeAndBucket FindHelper(Map* m, Key key) {
    return m->FindHelper(key);
  }

  template <typename Map>
  static bool InsertOrReplaceNode(Map* m, NodeBase* node) {
    return m->InsertOrReplaceNode(m->arena(),
                                  static_cast<typename Map::KeyNode*>(node));
  }

  template <typename Map, typename Key>
  static bool EraseImpl(Map* m, const Key& key) {
    return m->EraseImpl(m->arena(), key);
  }

  static google::protobuf::MessageLite* PlacementNew(const MessageLite* prototype,
                                           void* mem) {
    return prototype->GetClassData()->PlacementNew(mem, /* arena = */ nullptr);
  }
};

}  // namespace internal

// This is the class for Map's internal value_type.
template <typename Key, typename T>
using MapPair = std::pair<const Key, T>;

// Like C++20's std::erase_if, for Map
template <typename Key, typename T, typename Pred>
size_t erase_if(Map<Key, T>& map, Pred pred) {
  return map.EraseIfImpl(std::move(pred));
}

// Map is an associative container type used to store protobuf map
// fields.  Each Map instance may or may not use a different hash function, a
// different iteration order, and so on.  E.g., please don't examine
// implementation details to decide if the following would work:
//  Map<int, int> m0, m1;
//  m0[0] = m1[0] = m0[1] = m1[1] = 0;
//  assert(m0.begin()->first == m1.begin()->first);  // Bug!
//
// Map's interface is similar to std::unordered_map, except that Map is not
// designed to play well with exceptions.
template <typename Key, typename T>
class PROTOBUF_FUTURE_ADD_EARLY_WARN_UNUSED Map
    : private internal::KeyMapBase<internal::KeyForBase<Key>> {
  using Base = typename Map::KeyMapBase;

  using TS = internal::TransparentSupport<Key>;

 public:
  using key_type = Key;
  using mapped_type = T;
  using init_type = std::pair<Key, T>;
  using value_type = MapPair<Key, T>;

  using pointer = value_type*;
  using const_pointer = const value_type*;
  using reference = value_type&;
  using const_reference = const value_type&;

  using size_type = size_t;
  using hasher = absl::Hash<typename TS::ViewType>;

  constexpr Map() : Map(internal::InternalMetadataOffset()) {}
  Map(const Map& other) : Map(internal::InternalMetadataOffset(), other) {}

  Map(internal::InternalVisibility, internal::InternalMetadataOffset offset)
      : Map(offset) {}
  Map(internal::InternalVisibility, internal::InternalMetadataOffset offset,
      const Map& other)
      : Map(offset, other) {}

  Map(Map&& other) noexcept : Map(internal::InternalMetadataOffset()) {
    if (other.arena() != nullptr) {
      *this = other;
    } else {
      swap(other);
    }
  }

  Map& operator=(Map&& other) noexcept ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (this != &other) {
      if (arena() != other.arena()) {
        *this = other;
      } else {
        swap(other);
      }
    }
    return *this;
  }

  template <class InputIt>
  Map(const InputIt& first, const InputIt& last) : Map() {
    insert(first, last);
  }

  ~Map() {
    // Fail-safe in case we miss calling this in a constructor.  Note: this one
    // won't trigger for leaked maps that never get destructed.
    StaticValidityCheck();

    this->AssertLoadFactor();
    this->ClearTable(this->arena(), /*reset=*/false);
  }

 private:
  explicit constexpr Map(internal::InternalMetadataOffset offset)
      : Base(offset, GetTypeInfo()) {
    StaticValidityCheck();
  }

  Map(internal::InternalMetadataOffset offset, const Map& other) : Map(offset) {
    StaticValidityCheck();
    CopyFromImpl(arena(), other);
  }

  static_assert(!std::is_const<mapped_type>::value &&
                    !std::is_const<key_type>::value,
                "We do not support const types.");
  static_assert(!std::is_volatile<mapped_type>::value &&
                    !std::is_volatile<key_type>::value,
                "We do not support volatile types.");
  static_assert(!std::is_pointer<mapped_type>::value &&
                    !std::is_pointer<key_type>::value,
                "We do not support pointer types.");
  static_assert(!std::is_reference<mapped_type>::value &&
                    !std::is_reference<key_type>::value,
                "We do not support reference types.");
  static constexpr PROTOBUF_ALWAYS_INLINE void StaticValidityCheck() {
    static_assert(alignof(internal::NodeBase) >= alignof(mapped_type),
                  "Alignment of mapped type is too high.");
    static_assert(
        std::disjunction<internal::is_supported_integral_type<key_type>,
                         internal::is_supported_string_type<key_type>,
                         internal::is_internal_map_key_type<key_type>>::value,
        "We only support integer, string, or designated internal key "
        "types.");
    static_assert(std::disjunction<
                      internal::is_supported_scalar_type<mapped_type>,
                      is_proto_enum<mapped_type>,
                      internal::is_supported_message_type<mapped_type>,
                      internal::is_internal_map_value_type<mapped_type>>::value,
                  "We only support scalar, Message, and designated internal "
                  "mapped types.");
    // The Rust implementation that wraps C++ protos relies on the ability to
    // create an UntypedMapBase and cast a pointer of it to google::protobuf::Map*.
    static_assert(
        sizeof(Map) == sizeof(internal::UntypedMapBase),
        "Map must not have any data members beyond what is in UntypedMapBase.");

    // Check for MpMap optimizations.
    if constexpr (std::is_scalar_v<key_type>) {
      static_assert(sizeof(key_type) <= sizeof(uint64_t),
                    "Scalar must be <= than uint64_t");
    }
    if constexpr (std::is_scalar_v<mapped_type>) {
      static_assert(sizeof(mapped_type) <= sizeof(uint64_t),
                    "Scalar must be <= than uint64_t");
    }
    static_assert(internal::kMaxMessageAlignment >= sizeof(uint64_t));
    static_assert(sizeof(Node) - sizeof(internal::NodeBase) >= sizeof(uint64_t),
                  "We must have at least this bytes for MpMap initialization");
  }

  template <typename P>
  struct SameAsElementReference
      : std::is_same<typename std::remove_cv<
                         typename std::remove_reference<reference>::type>::type,
                     typename std::remove_cv<
                         typename std::remove_reference<P>::type>::type> {};

  template <class P>
  using RequiresInsertable =
      typename std::enable_if<std::is_convertible<P, init_type>::value ||
                                  SameAsElementReference<P>::value,
                              int>::type;
  template <class P>
  using RequiresNotInit =
      typename std::enable_if<!std::is_same<P, init_type>::value, int>::type;

  template <typename LookupKey>
  using key_arg = typename TS::template key_arg<LookupKey>;

 public:
  // Iterators
  class PROTOBUF_FUTURE_ADD_EARLY_WARN_UNUSED ABSL_ATTRIBUTE_VIEW const_iterator
      : private internal::UntypedMapIterator {
    using BaseIt = internal::UntypedMapIterator;

   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename Map::value_type;
    using difference_type = ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;

    const_iterator() : BaseIt{nullptr, nullptr, 0} {}
    const_iterator(const const_iterator&) = default;
    const_iterator& operator=(const const_iterator&) = default;
    explicit const_iterator(BaseIt it) : BaseIt(it) {}

    PROTOBUF_FUTURE_ADD_EARLY_NODISCARD reference operator*() const {
      return static_cast<Node*>(this->node_)->kv;
    }
    PROTOBUF_FUTURE_ADD_EARLY_NODISCARD pointer operator->() const {
      return &(operator*());
    }

    const_iterator& operator++() {
      this->PlusPlus();
      return *this;
    }
    const_iterator operator++(int) {
      auto copy = *this;
      this->PlusPlus();
      return copy;
    }

    PROTOBUF_FUTURE_ADD_EARLY_NODISCARD friend bool operator==(
        const const_iterator& a, const const_iterator& b) {
      return a.Equals(b);
    }
    PROTOBUF_FUTURE_ADD_EARLY_NODISCARD friend bool operator!=(
        const const_iterator& a, const const_iterator& b) {
      return !a.Equals(b);
    }

   private:
    using BaseIt::BaseIt;
    friend class Map;
    friend class internal::UntypedMapIterator;
    friend class internal::TypeDefinedMapFieldBase<Key, T>;
  };

  class PROTOBUF_FUTURE_ADD_EARLY_WARN_UNUSED ABSL_ATTRIBUTE_VIEW iterator
      : private internal::UntypedMapIterator {
    using BaseIt = internal::UntypedMapIterator;

   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename Map::value_type;
    using difference_type = ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    iterator() : BaseIt{nullptr, nullptr, 0} {}
    iterator(const iterator&) = default;
    iterator& operator=(const iterator&) = default;
    explicit iterator(BaseIt it) : BaseIt(it) {}

    PROTOBUF_FUTURE_ADD_EARLY_NODISCARD reference operator*() const {
      return static_cast<Node*>(this->node_)->kv;
    }
    PROTOBUF_FUTURE_ADD_EARLY_NODISCARD pointer operator->() const {
      return &(operator*());
    }

    iterator& operator++() {
      this->PlusPlus();
      return *this;
    }
    iterator operator++(int) {
      auto copy = *this;
      this->PlusPlus();
      return copy;
    }

    // Allow implicit conversion to const_iterator.
    PROTOBUF_FUTURE_ADD_EARLY_NODISCARD operator const_iterator()
        const {  // NOLINT(google-explicit-constructor)
      return const_iterator(static_cast<const BaseIt&>(*this));
    }

    PROTOBUF_FUTURE_ADD_EARLY_NODISCARD friend bool operator==(
        const iterator& a, const iterator& b) {
      return a.Equals(b);
    }
    PROTOBUF_FUTURE_ADD_EARLY_NODISCARD friend bool operator!=(
        const iterator& a, const iterator& b) {
      return !a.Equals(b);
    }

   private:
    using BaseIt::BaseIt;
    friend class Map;
  };

  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD iterator begin()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return iterator(Base::begin());
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD iterator end()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return iterator();
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD const_iterator
  begin() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return const_iterator(Base::begin());
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD const_iterator
  end() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return const_iterator();
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD const_iterator
  cbegin() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return begin();
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD const_iterator
  cend() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return end();
  }

  using Base::empty;
  using Base::size;

  // Element access
  template <typename K = key_type>
  T& operator[](const key_arg<K>& key) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return try_emplace(key).first->second;
  }
  template <
      typename K = key_type,
      // Disable for integral types to reduce code bloat.
      typename = typename std::enable_if<!std::is_integral<K>::value>::type>
  T& operator[](key_arg<K>&& key) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return try_emplace(std::forward<key_arg<K>>(key)).first->second;
  }

  template <typename K = key_type>
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD const T& at(const key_arg<K>& key) const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    const_iterator it = find(key);
    ABSL_CHECK(it != end()) << "key not found: " << static_cast<Key>(key);
    return it->second;
  }

  template <typename K = key_type>
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD T& at(const key_arg<K>& key)
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    iterator it = find(key);
    ABSL_CHECK(it != end()) << "key not found: " << static_cast<Key>(key);
    return it->second;
  }

  // Lookup
  template <typename K = key_type>
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD size_type
  count(const key_arg<K>& key) const {
    return find(key) == end() ? 0 : 1;
  }

  template <typename K = key_type>
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD const_iterator
  find(const key_arg<K>& key) const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return const_cast<Map*>(this)->find(key);
  }
  template <typename K = key_type>
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD iterator find(const key_arg<K>& key)
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    auto res = this->FindHelper(TS::ToView(key));
    return iterator(internal::UntypedMapIterator{static_cast<Node*>(res.node),
                                                 this, res.bucket});
  }

  template <typename K = key_type>
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD bool contains(
      const key_arg<K>& key) const {
    return find(key) != end();
  }

  template <typename K = key_type>
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD std::pair<const_iterator, const_iterator>
  equal_range(const key_arg<K>& key) const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    const_iterator it = find(key);
    if (it == end()) {
      return std::pair<const_iterator, const_iterator>(it, it);
    } else {
      const_iterator begin = it++;
      return std::pair<const_iterator, const_iterator>(begin, it);
    }
  }

  template <typename K = key_type>
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD std::pair<iterator, iterator> equal_range(
      const key_arg<K>& key) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    iterator it = find(key);
    if (it == end()) {
      return std::pair<iterator, iterator>(it, it);
    } else {
      iterator begin = it++;
      return std::pair<iterator, iterator>(begin, it);
    }
  }

  // insert
  template <typename K, typename... Args>
  std::pair<iterator, bool> try_emplace(K&& k, Args&&... args)
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    // Case 1: `mapped_type` is arena constructible. A temporary object is
    // created and then (if `Args` are not empty) assigned to a mapped value
    // that was created with the arena.
    if constexpr (Arena::is_arena_constructable<mapped_type>::value) {
      if constexpr (sizeof...(Args) == 0) {
        // case 1.1: "default" constructed (e.g. from arena only).
        return TryEmplaceInternal(std::forward<K>(k));
      } else {
        // case 1.2: "default" constructed + copy/move assignment
        auto p = TryEmplaceInternal(std::forward<K>(k));
        if (p.second) {
          if constexpr (std::is_same<void(typename std::decay<Args>::type...),
                                     void(mapped_type)>::value) {
            // Avoid the temporary when the input is the right type.
            p.first->second = (std::forward<Args>(args), ...);
          } else {
            p.first->second = mapped_type(std::forward<Args>(args)...);
          }
        }
        return p;
      }
    } else {
      // Case 2: `mapped_type` is not arena constructible. Using in-place
      // construction.
      return TryEmplaceInternal(std::forward<K>(k),
                                std::forward<Args>(args)...);
    }
  }
  std::pair<iterator, bool> insert(init_type&& value)
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return try_emplace(std::move(value.first), std::move(value.second));
  }
  template <typename P, RequiresInsertable<P> = 0>
  std::pair<iterator, bool> insert(P&& value) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return try_emplace(std::forward<P>(value).first,
                       std::forward<P>(value).second);
  }
  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args)
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    // We try to construct `init_type` from `Args` with a fall back to
    // `value_type`. The latter is less desired as it unconditionally makes a
    // copy of `value_type::first`.
    if constexpr (std::is_constructible<init_type, Args...>::value) {
      return insert(init_type(std::forward<Args>(args)...));
    } else {
      return insert(value_type(std::forward<Args>(args)...));
    }
  }
  template <class InputIt>
  void insert(InputIt first, InputIt last) {
    for (; first != last; ++first) {
      auto&& pair = *first;
      try_emplace(pair.first, pair.second);
    }
  }
  void insert(std::initializer_list<init_type> values) {
    insert(values.begin(), values.end());
  }
  template <typename P, RequiresNotInit<P> = 0,
            RequiresInsertable<const P&> = 0>
  void insert(std::initializer_list<P> values) {
    insert(values.begin(), values.end());
  }

  // Erase and clear
  template <typename K = key_type>
  size_type erase(const key_arg<K>& key) {
    return this->EraseImpl(arena(), TS::ToView(key));
  }

  iterator erase(iterator pos) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    auto next = std::next(pos);
    ABSL_DCHECK_EQ(pos.m_, static_cast<Base*>(this));
    this->EraseImpl(arena(), pos.bucket_index_, static_cast<Node*>(pos.node_),
                    /*do_destroy=*/true);
    return next;
  }

  void erase(iterator first, iterator last) {
    while (first != last) {
      first = erase(first);
    }
  }

  void clear() { this->ClearTable(this->arena(), /*reset=*/true); }

  // Assign
  Map& operator=(const Map& other) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (this != &other) {
      clear();
      CopyFromImpl(this->arena(), other);
    }
    return *this;
  }

  void swap(Map& other) {
    Arena* arena = this->arena();
    if (arena == other.arena()) {
      this->InternalSwap(&other);
    } else {
      size_t other_size = other.size();
      Node* other_copy = this->CloneFromOther(arena, other);
      other = *this;
      this->clear();
      if (other_size != 0) {
        this->MergeIntoEmpty(arena, other_copy, other_size);
      }
    }
  }

  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD hasher hash_function() const {
    return {};
  }

  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD size_t
  SpaceUsedExcludingSelfLong() const {
    if (empty()) return 0;
    return internal::UntypedMapBase::SpaceUsedExcludingSelfLong();
  }

 private:
  // Linked-list nodes, as one would expect for a chaining hash table.
  struct Node : Base::KeyNode {
    using key_type = Key;
    using mapped_type = T;
    value_type kv;
  };

  static constexpr auto GetTypeInfo() {
    return internal::UntypedMapBase::TypeInfo{
        sizeof(Node),
        PROTOBUF_FIELD_OFFSET(Node, kv.second),
        static_cast<uint8_t>(internal::UntypedMapBase::StaticTypeKind<Key>()),
        static_cast<uint8_t>(internal::UntypedMapBase::StaticTypeKind<T>()),
    };
  }

  void DeleteNode(Arena* arena, Node* node) {
    ABSL_DCHECK_EQ(arena, this->arena());
    if (arena == nullptr) {
      node->kv.first.~key_type();
      node->kv.second.~mapped_type();
      this->DeallocNode(node, sizeof(Node));
    }
  }

  template <typename K, typename... Args>
  PROTOBUF_ALWAYS_INLINE Node* CreateNode(Arena* arena, K&& k, Args&&... args) {
    // If K is not key_type, make the conversion to key_type explicit.
    using TypeToInit = typename std::conditional<
        std::is_same<typename std::decay<K>::type, key_type>::value, K&&,
        key_type>::type;
    ABSL_DCHECK_EQ(arena, this->arena());
    Node* node = static_cast<Node*>(this->AllocNode(arena, sizeof(Node)));

    // Even when arena is nullptr, CreateInArenaStorage is still used to
    // ensure the arena of submessage will be consistent. Otherwise,
    // submessage may have its own arena when message-owned arena is enabled.
    // Note: This only works if `Key` is not arena constructible.
    if (!internal::InitializeMapKey(const_cast<Key*>(&node->kv.first),
                                    std::forward<K>(k), arena)) {
      Arena::CreateInArenaStorage(const_cast<Key*>(&node->kv.first), arena,
                                  static_cast<TypeToInit>(std::forward<K>(k)));
    }
    // Note: if `T` is arena constructible, `Args` needs to be empty.
    Arena::CreateInArenaStorage(&node->kv.second, arena,
                                std::forward<Args>(args)...);
    return node;
  }

  // Copy all elements from `other`, using the arena from `this`.
  // Return them as a linked list, using the `next` pointer in the node.
  PROTOBUF_NOINLINE Node* CloneFromOther(Arena* arena, const Map& other) {
    ABSL_DCHECK_EQ(arena, this->arena());

    Node* head = nullptr;
    for (const auto& [key, value] : other) {
      Node* new_node = CreateNode(arena, key, value);
      new_node->next = head;
      head = new_node;
    }
    return head;
  }

  void CopyFromImpl(Arena* arena, const Map& other) {
    if (other.empty()) return;
    // We split the logic in two: first we clone the data which requires
    // Key/Value types, then we insert them all which only requires Key.
    // That way we reduce code duplication.
    this->MergeIntoEmpty(arena, CloneFromOther(arena, other), other.size());
  }

  template <typename K, typename... Args>
  std::pair<iterator, bool> TryEmplaceInternal(K&& k, Args&&... args) {
    auto p = this->FindHelper(TS::ToView(k));
    internal::map_index_t b = p.bucket;
    // Case 1: key was already present.
    if (p.node != nullptr) {
      return std::make_pair(iterator(internal::UntypedMapIterator{
                                static_cast<Node*>(p.node), this, p.bucket}),
                            false);
    }
    // Case 2: insert.
    Arena* arena = this->arena();
    if (this->ResizeIfLoadIsOutOfRange(arena, this->num_elements_ + 1)) {
      b = this->BucketNumber(TS::ToView(k));
    }
    auto* node =
        CreateNode(arena, std::forward<K>(k), std::forward<Args>(args)...);
    this->InsertUnique(b, node);
    ++this->num_elements_;
    return std::make_pair(iterator(internal::UntypedMapIterator{node, this, b}),
                          true);
  }

  template <typename Pred>
  size_t EraseIfImpl(Pred pred) {
    size_t n = 0;
    auto* arena = this->arena();
    for (internal::NodeBase **bucket = this->table_,
                            **end = this->table_ + this->num_buckets_;
         bucket != end; ++bucket) {
      for (internal::NodeBase** prev = bucket; *prev != nullptr;) {
        Node* node = static_cast<Node*>(*prev);
        if (pred(std::as_const(node->kv))) {
          *prev = node->next;
          DeleteNode(arena, node);
          ++n;
        } else {
          prev = &node->next;
        }
      }
    }
    this->num_elements_ -= n;
    return n;
  }

  using Base::arena;

  friend class Arena;
  template <typename, typename>
  friend class internal::TypeDefinedMapFieldBase;

  template <typename Key_, typename T_, typename Pred>
  friend size_t google::protobuf::erase_if(Map<Key_, T_>& map, Pred pred);

  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;

  friend class internal::FieldWithArena<Map<Key, T>>;
  template <typename K, typename V>
  friend class internal::MapFieldLite;
  friend class internal::TcParser;
  friend struct internal::MapTestPeer;
  friend struct internal::MapBenchmarkPeer;
  friend class internal::RustMapHelper;
};

namespace internal {

template <typename Key, typename T>
using MapWithArena = FieldWithArena<Map<Key, T>>;

template <typename Key, typename T>
struct FieldArenaRep<Map<Key, T>> {
  using Type = MapWithArena<Key, T>;

  static inline Map<Key, T>* Get(Type* arena_rep) {
    return &arena_rep->field();
  }
};

template <typename Key, typename T>
struct FieldArenaRep<const Map<Key, T>> {
  using Type = const MapWithArena<Key, T>;

  static inline const Map<Key, T>* Get(Type* arena_rep) {
    return &arena_rep->field();
  }
};

template <typename... T>
PROTOBUF_NOINLINE void MapMergeFrom(Map<T...>& dest, const Map<T...>& src) {
  for (const auto& elem : src) {
    dest[elem.first] = elem.second;
  }
}
}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_MAP_H__
