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
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>  // To support Visual Studio 2008
#include <string>
#include <type_traits>
#include <utility>

#if !defined(GOOGLE_PROTOBUF_NO_RDTSC) && defined(__APPLE__)
#include <time.h>
#endif

#include "google/protobuf/stubs/common.h"
#include "absl/base/attributes.h"
#include "absl/container/btree_map.h"
#include "absl/hash/hash.h"
#include "absl/meta/type_traits.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/generated_enum_util.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/map_type_handler.h"
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

namespace internal {
template <typename Key, typename T>
class MapFieldLite;

template <typename Derived, typename Key, typename T,
          WireFormatLite::FieldType key_wire_type,
          WireFormatLite::FieldType value_wire_type>
class MapField;

struct MapTestPeer;
struct MapBenchmarkPeer;

template <typename Key, typename T>
class TypeDefinedMapFieldBase;

class DynamicMapField;

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

// re-implement std::allocator to use arena allocator for memory allocation.
// Used for Map implementation. Users should not use this class
// directly.
template <typename U>
class MapAllocator {
 public:
  using value_type = U;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  constexpr MapAllocator() : arena_(nullptr) {}
  explicit constexpr MapAllocator(Arena* arena) : arena_(arena) {}
  template <typename X>
  MapAllocator(const MapAllocator<X>& allocator)  // NOLINT(runtime/explicit)
      : arena_(allocator.arena()) {}

  // MapAllocator does not support alignments beyond 8. Technically we should
  // support up to std::max_align_t, but this fails with ubsan and tcmalloc
  // debug allocation logic which assume 8 as default alignment.
  static_assert(alignof(value_type) <= 8, "");

  pointer allocate(size_type n, const void* /* hint */ = nullptr) {
    // If arena is not given, malloc needs to be called which doesn't
    // construct element object.
    if (arena_ == nullptr) {
      return static_cast<pointer>(::operator new(n * sizeof(value_type)));
    } else {
      return reinterpret_cast<pointer>(
          Arena::CreateArray<uint8_t>(arena_, n * sizeof(value_type)));
    }
  }

  void deallocate(pointer p, size_type n) {
    if (arena_ == nullptr) {
      internal::SizedDelete(p, n * sizeof(value_type));
    }
  }

#if !defined(GOOGLE_PROTOBUF_OS_APPLE) && !defined(GOOGLE_PROTOBUF_OS_NACL) && \
    !defined(GOOGLE_PROTOBUF_OS_EMSCRIPTEN)
  template <class NodeType, class... Args>
  void construct(NodeType* p, Args&&... args) {
    // Clang 3.6 doesn't compile static casting to void* directly. (Issue
    // #1266) According C++ standard 5.2.9/1: "The static_cast operator shall
    // not cast away constness". So first the maybe const pointer is casted to
    // const void* and after the const void* is const casted.
    new (const_cast<void*>(static_cast<const void*>(p)))
        NodeType(std::forward<Args>(args)...);
  }

  template <class NodeType>
  void destroy(NodeType* p) {
    p->~NodeType();
  }
#else
  void construct(pointer p, const_reference t) { new (p) value_type(t); }

  void destroy(pointer p) { p->~value_type(); }
#endif

  template <typename X>
  struct rebind {
    using other = MapAllocator<X>;
  };

  template <typename X>
  bool operator==(const MapAllocator<X>& other) const {
    return arena_ == other.arena_;
  }

  template <typename X>
  bool operator!=(const MapAllocator<X>& other) const {
    return arena_ != other.arena_;
  }

  // To support Visual Studio 2008
  size_type max_size() const {
    // parentheses around (std::...:max) prevents macro warning of max()
    return (std::numeric_limits<size_type>::max)();
  }

  // To support gcc-4.4, which does not properly
  // support templated friend classes
  Arena* arena() const { return arena_; }

 private:
  using DestructorSkippable_ = void;
  Arena* arena_;
};

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
  // We hash all the scalars as uint64_t so that we can implement the same hash
  // function for VariantKey. This way we can have MapKey provide the same hash
  // as the underlying value would have.
  using hash = std::hash<
      std::conditional_t<std::is_scalar<key_type>::value, uint64_t, key_type>>;

  static bool Equals(const key_type& a, const key_type& b) { return a == b; }

  template <typename K>
  using key_arg = key_type;

  using ViewType = std::conditional_t<std::is_scalar<key_type>::value, key_type,
                                      const key_type&>;
  static ViewType ToView(const key_type& v) { return v; }
};

// We add transparent support for std::string keys. We use
// std::hash<absl::string_view> as it supports the input types we care about.
// The lookup functions accept arbitrary `K`. This will include any key type
// that is convertible to absl::string_view.
template <>
struct TransparentSupport<std::string> {
  // If the element is not convertible to absl::string_view, try to convert to
  // std::string first, and then fallback to support for converting from
  // std::string_view. The ranked overload pattern is used to specify our
  // order of preference.
  struct Rank0 {};
  struct Rank1 : Rank0 {};
  struct Rank2 : Rank1 {};
  template <typename T, typename = std::enable_if_t<
                            std::is_convertible<T, absl::string_view>::value>>
  static absl::string_view ImplicitConvertImpl(T&& str, Rank2) {
    absl::string_view ref = str;
    return ref;
  }
  template <typename T, typename = std::enable_if_t<
                            std::is_convertible<T, const std::string&>::value>>
  static absl::string_view ImplicitConvertImpl(T&& str, Rank1) {
    const std::string& ref = str;
    return ref;
  }
  template <typename T>
  static absl::string_view ImplicitConvertImpl(T&& str, Rank0) {
    return {str.data(), str.size()};
  }

  template <typename T>
  static absl::string_view ImplicitConvert(T&& str) {
    return ImplicitConvertImpl(std::forward<T>(str), Rank2{});
  }

  struct hash : public absl::Hash<absl::string_view> {
    using is_transparent = void;

    template <typename T>
    size_t operator()(T&& str) const {
      return absl::Hash<absl::string_view>::operator()(
          ImplicitConvert(std::forward<T>(str)));
    }
  };

  template <typename T, typename U>
  static bool Equals(T&& t, U&& u) {
    return ImplicitConvert(std::forward<T>(t)) ==
           ImplicitConvert(std::forward<U>(u));
  }

  template <typename K>
  using key_arg = K;

  using ViewType = absl::string_view;
  template <typename T>
  static ViewType ToView(const T& v) {
    return ImplicitConvert(v);
  }
};

enum class MapNodeSizeInfoT : uint32_t;
inline uint16_t SizeFromInfo(MapNodeSizeInfoT node_size_info) {
  return static_cast<uint16_t>(static_cast<uint32_t>(node_size_info) >> 16);
}
inline uint16_t ValueOffsetFromInfo(MapNodeSizeInfoT node_size_info) {
  return static_cast<uint16_t>(static_cast<uint32_t>(node_size_info) >> 0);
}
constexpr MapNodeSizeInfoT MakeNodeInfo(uint16_t size, uint16_t value_offset) {
  return static_cast<MapNodeSizeInfoT>((static_cast<uint32_t>(size) << 16) |
                                       value_offset);
}

struct NodeBase {
  // Align the node to allow KeyNode to predict the location of the key.
  // This way sizeof(NodeBase) contains any possible padding it was going to
  // have between NodeBase and the key.
  alignas(kMaxMessageAlignment) NodeBase* next;

  void* GetVoidKey() { return this + 1; }
  const void* GetVoidKey() const { return this + 1; }

  void* GetVoidValue(MapNodeSizeInfoT size_info) {
    return reinterpret_cast<char*>(this) + ValueOffsetFromInfo(size_info);
  }
};

inline NodeBase* EraseFromLinkedList(NodeBase* item, NodeBase* head) {
  if (head == item) {
    return head->next;
  } else {
    head->next = EraseFromLinkedList(item, head->next);
    return head;
  }
}

constexpr size_t MapTreeLengthThreshold() { return 8; }
inline bool TableEntryIsTooLong(NodeBase* node) {
  const size_t kMaxLength = MapTreeLengthThreshold();
  size_t count = 0;
  do {
    ++count;
    node = node->next;
  } while (node != nullptr);
  // Invariant: no linked list ever is more than kMaxLength in length.
  ABSL_DCHECK_LE(count, kMaxLength);
  return count >= kMaxLength;
}

// Similar to the public MapKey, but specialized for the internal
// implementation.
struct VariantKey {
  // We make this value 16 bytes to make it cheaper to pass in the ABI.
  // Can't overload string_view this way, so we unpack the fields.
  // data==nullptr means this is a number and `integral` is the value.
  // data!=nullptr means this is a string and `integral` is the size.
  const char* data;
  uint64_t integral;

  explicit VariantKey(uint64_t v) : data(nullptr), integral(v) {}
  explicit VariantKey(absl::string_view v)
      : data(v.data()), integral(v.size()) {
    // We use `data` to discriminate between the types, so make sure it is never
    // null here.
    if (data == nullptr) data = "";
  }

  size_t Hash() const {
    return data == nullptr ? std::hash<uint64_t>{}(integral)
                           : absl::Hash<absl::string_view>{}(
                                 absl::string_view(data, integral));
  }

  friend bool operator<(const VariantKey& left, const VariantKey& right) {
    ABSL_DCHECK_EQ(left.data == nullptr, right.data == nullptr);
    if (left.integral != right.integral) {
      // If they are numbers with different value, or strings with different
      // size, check the number only.
      return left.integral < right.integral;
    }
    if (left.data == nullptr) {
      // If they are numbers they have the same value, so return.
      return false;
    }
    // They are strings of the same size, so check the bytes.
    return memcmp(left.data, right.data, left.integral) < 0;
  }
};

// This is to be specialized by MapKey.
template <typename T>
struct RealKeyToVariantKey {
  VariantKey operator()(T value) const { return VariantKey(value); }
};

template <>
struct RealKeyToVariantKey<std::string> {
  template <typename T>
  VariantKey operator()(const T& value) const {
    return VariantKey(TransparentSupport<std::string>::ImplicitConvert(value));
  }
};

// We use a single kind of tree for all maps. This reduces code duplication.
using TreeForMap =
    absl::btree_map<VariantKey, NodeBase*, std::less<VariantKey>,
                    MapAllocator<std::pair<const VariantKey, NodeBase*>>>;

// Type safe tagged pointer.
// We convert to/from nodes and trees using the operations below.
// They ensure that the tags are used correctly.
// There are three states:
//  - x == 0: the entry is empty
//  - x != 0 && (x&1) == 0: the entry is a node list
//  - x != 0 && (x&1) == 1: the entry is a tree
enum class TableEntryPtr : uintptr_t;

inline bool TableEntryIsEmpty(TableEntryPtr entry) {
  return entry == TableEntryPtr{};
}
inline bool TableEntryIsTree(TableEntryPtr entry) {
  return (static_cast<uintptr_t>(entry) & 1) == 1;
}
inline bool TableEntryIsList(TableEntryPtr entry) {
  return !TableEntryIsTree(entry);
}
inline bool TableEntryIsNonEmptyList(TableEntryPtr entry) {
  return !TableEntryIsEmpty(entry) && TableEntryIsList(entry);
}
inline NodeBase* TableEntryToNode(TableEntryPtr entry) {
  ABSL_DCHECK(TableEntryIsList(entry));
  return reinterpret_cast<NodeBase*>(static_cast<uintptr_t>(entry));
}
inline TableEntryPtr NodeToTableEntry(NodeBase* node) {
  ABSL_DCHECK((reinterpret_cast<uintptr_t>(node) & 1) == 0);
  return static_cast<TableEntryPtr>(reinterpret_cast<uintptr_t>(node));
}
inline TreeForMap* TableEntryToTree(TableEntryPtr entry) {
  ABSL_DCHECK(TableEntryIsTree(entry));
  return reinterpret_cast<TreeForMap*>(static_cast<uintptr_t>(entry) - 1);
}
inline TableEntryPtr TreeToTableEntry(TreeForMap* node) {
  ABSL_DCHECK((reinterpret_cast<uintptr_t>(node) & 1) == 0);
  return static_cast<TableEntryPtr>(reinterpret_cast<uintptr_t>(node) | 1);
}

// This captures all numeric types.
inline size_t MapValueSpaceUsedExcludingSelfLong(bool) { return 0; }
inline size_t MapValueSpaceUsedExcludingSelfLong(const std::string& str) {
  return StringSpaceUsedExcludingSelfLong(str);
}
template <typename T,
          typename = decltype(std::declval<const T&>().SpaceUsedLong())>
size_t MapValueSpaceUsedExcludingSelfLong(const T& message) {
  return message.SpaceUsedLong() - sizeof(T);
}

constexpr size_t kGlobalEmptyTableSize = 1;
PROTOBUF_EXPORT extern const TableEntryPtr
    kGlobalEmptyTable[kGlobalEmptyTableSize];

template <typename Map,
          typename = typename std::enable_if<
              !std::is_scalar<typename Map::key_type>::value ||
              !std::is_scalar<typename Map::mapped_type>::value>::type>
size_t SpaceUsedInValues(const Map* map) {
  size_t size = 0;
  for (const auto& v : *map) {
    size += internal::MapValueSpaceUsedExcludingSelfLong(v.first) +
            internal::MapValueSpaceUsedExcludingSelfLong(v.second);
  }
  return size;
}

// Multiply two numbers where overflow is expected.
template <typename N>
N MultiplyWithOverflow(N a, N b) {
#if defined(PROTOBUF_HAS_BUILTIN_MUL_OVERFLOW)
  N res;
  (void)__builtin_mul_overflow(a, b, &res);
  return res;
#else
  return a * b;
#endif
}

inline size_t SpaceUsedInValues(const void*) { return 0; }

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
  UntypedMapIterator() : node_(nullptr), m_(nullptr), bucket_index_(0) {}

  explicit UntypedMapIterator(const UntypedMapBase* m);

  UntypedMapIterator(NodeBase* n, const UntypedMapBase* m, map_index_t index)
      : node_(n), m_(m), bucket_index_(index) {}

  // Advance through buckets, looking for the first that isn't empty.
  // If nothing non-empty is found then leave node_ == nullptr.
  void SearchFrom(map_index_t start_bucket);

  // The definition of operator== is handled by the derived type. If we were
  // to do it in this class it would allow comparing iterators of different
  // map types.
  bool Equals(const UntypedMapIterator& other) const {
    return node_ == other.node_;
  }

  // The definition of operator++ is handled in the derived type. We would not
  // be able to return the right type from here.
  void PlusPlus() {
    if (node_->next == nullptr) {
      SearchFrom(bucket_index_ + 1);
    } else {
      node_ = node_->next;
    }
  }

  NodeBase* node_;
  const UntypedMapBase* m_;
  map_index_t bucket_index_;
};

// Base class for all Map instantiations.
// This class holds all the data and provides the basic functionality shared
// among all instantiations.
// Having an untyped base class helps generic consumers (like the table-driven
// parser) by having non-template code that can handle all instantiations.
class PROTOBUF_EXPORT UntypedMapBase {
  using Allocator = internal::MapAllocator<void*>;
  using Tree = internal::TreeForMap;

 public:
  using size_type = size_t;

  explicit constexpr UntypedMapBase(Arena* arena)
      : num_elements_(0),
        num_buckets_(internal::kGlobalEmptyTableSize),
        seed_(0),
        index_of_first_non_null_(internal::kGlobalEmptyTableSize),
        table_(const_cast<TableEntryPtr*>(internal::kGlobalEmptyTable)),
        alloc_(arena) {}

  UntypedMapBase(const UntypedMapBase&) = delete;
  UntypedMapBase& operator=(const UntypedMapBase&) = delete;

 protected:
  enum { kMinTableSize = 8 };

 public:
  Arena* arena() const { return this->alloc_.arena(); }

  void InternalSwap(UntypedMapBase* other) {
    std::swap(num_elements_, other->num_elements_);
    std::swap(num_buckets_, other->num_buckets_);
    std::swap(seed_, other->seed_);
    std::swap(index_of_first_non_null_, other->index_of_first_non_null_);
    std::swap(table_, other->table_);
    std::swap(alloc_, other->alloc_);
  }

  static size_type max_size() {
    return std::numeric_limits<map_index_t>::max();
  }
  size_type size() const { return num_elements_; }
  bool empty() const { return size() == 0; }

  UntypedMapIterator begin() const { return UntypedMapIterator(this); }
  // We make this a static function to reduce the cost in MapField.
  // All the end iterators are singletons anyway.
  static UntypedMapIterator EndIterator() { return {}; }

 protected:
  friend class TcParser;
  friend struct MapTestPeer;
  friend struct MapBenchmarkPeer;
  friend class UntypedMapIterator;

  struct NodeAndBucket {
    NodeBase* node;
    map_index_t bucket;
  };

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
    return (reinterpret_cast<uintptr_t>(node) ^ seed_) % 13 > 6;
#endif
  }

  // Helper for InsertUnique.  Handles the case where bucket b is a
  // not-too-long linked list.
  void InsertUniqueInList(map_index_t b, NodeBase* node) {
    if (!TableEntryIsEmpty(b) && ShouldInsertAfterHead(node)) {
      auto* first = TableEntryToNode(table_[b]);
      node->next = first->next;
      first->next = node;
    } else {
      node->next = TableEntryToNode(table_[b]);
      table_[b] = NodeToTableEntry(node);
    }
  }

  bool TableEntryIsEmpty(map_index_t b) const {
    return internal::TableEntryIsEmpty(table_[b]);
  }
  bool TableEntryIsNonEmptyList(map_index_t b) const {
    return internal::TableEntryIsNonEmptyList(table_[b]);
  }
  bool TableEntryIsTree(map_index_t b) const {
    return internal::TableEntryIsTree(table_[b]);
  }
  bool TableEntryIsList(map_index_t b) const {
    return internal::TableEntryIsList(table_[b]);
  }

  // Return whether table_[b] is a linked list that seems awfully long.
  // Requires table_[b] to point to a non-empty linked list.
  bool TableEntryIsTooLong(map_index_t b) {
    return internal::TableEntryIsTooLong(TableEntryToNode(table_[b]));
  }

  // Return a power of two no less than max(kMinTableSize, n).
  // Assumes either n < kMinTableSize or n is a power of two.
  map_index_t TableSize(map_index_t n) {
    return n < static_cast<map_index_t>(kMinTableSize)
               ? static_cast<map_index_t>(kMinTableSize)
               : n;
  }

  template <typename T>
  using AllocFor = absl::allocator_traits<Allocator>::template rebind_alloc<T>;

  // Alignment of the nodes is the same as alignment of NodeBase.
  NodeBase* AllocNode(MapNodeSizeInfoT size_info) {
    return AllocNode(SizeFromInfo(size_info));
  }

  NodeBase* AllocNode(size_t node_size) {
    PROTOBUF_ASSUME(node_size % sizeof(NodeBase) == 0);
    return AllocFor<NodeBase>(alloc_).allocate(node_size / sizeof(NodeBase));
  }

  void DeallocNode(NodeBase* node, MapNodeSizeInfoT size_info) {
    DeallocNode(node, SizeFromInfo(size_info));
  }

  void DeallocNode(NodeBase* node, size_t node_size) {
    PROTOBUF_ASSUME(node_size % sizeof(NodeBase) == 0);
    AllocFor<NodeBase>(alloc_).deallocate(node, node_size / sizeof(NodeBase));
  }

  void DeleteTable(TableEntryPtr* table, map_index_t n) {
    AllocFor<TableEntryPtr>(alloc_).deallocate(table, n);
  }

  NodeBase* DestroyTree(Tree* tree);
  using GetKey = VariantKey (*)(NodeBase*);
  void InsertUniqueInTree(map_index_t b, GetKey get_key, NodeBase* node);
  void TransferTree(Tree* tree, GetKey get_key);
  TableEntryPtr ConvertToTree(NodeBase* node, GetKey get_key);
  void EraseFromTree(map_index_t b, typename Tree::iterator tree_it);

  map_index_t VariantBucketNumber(VariantKey key) const;

  map_index_t BucketNumberFromHash(uint64_t h) const {
    // We xor the hash value against the random seed so that we effectively
    // have a random hash function.
    h ^= seed_;

    // We use the multiplication method to determine the bucket number from
    // the hash value. The constant kPhi (suggested by Knuth) is roughly
    // (sqrt(5) - 1) / 2 * 2^64.
    constexpr uint64_t kPhi = uint64_t{0x9e3779b97f4a7c15};
    return (MultiplyWithOverflow(kPhi, h) >> 32) & (num_buckets_ - 1);
  }

  TableEntryPtr* CreateEmptyTable(map_index_t n) {
    ABSL_DCHECK_GE(n, map_index_t{kMinTableSize});
    ABSL_DCHECK_EQ(n & (n - 1), 0u);
    TableEntryPtr* result = AllocFor<TableEntryPtr>(alloc_).allocate(n);
    memset(result, 0, n * sizeof(result[0]));
    return result;
  }

  // Return a randomish value.
  map_index_t Seed() const {
    // We get a little bit of randomness from the address of the map. The
    // lower bits are not very random, due to alignment, so we discard them
    // and shift the higher bits into their place.
    map_index_t s = reinterpret_cast<uintptr_t>(this) >> 4;
#if !defined(GOOGLE_PROTOBUF_NO_RDTSC)
#if defined(__APPLE__)
    // Use a commpage-based fast time function on Apple environments (MacOS,
    // iOS, tvOS, watchOS, etc).
    s = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
#elif defined(__x86_64__) && defined(__GNUC__)
    uint32_t hi, lo;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    s += ((static_cast<uint64_t>(hi) << 32) | lo);
#elif defined(__aarch64__) && defined(__GNUC__)
    // There is no rdtsc on ARMv8. CNTVCT_EL0 is the virtual counter of the
    // system timer. It runs at a different frequency than the CPU's, but is
    // the best source of time-based entropy we get.
    uint64_t virtual_timer_value;
    asm volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
    s += virtual_timer_value;
#endif
#endif  // !defined(GOOGLE_PROTOBUF_NO_RDTSC)
    return s;
  }

  enum {
    kKeyIsString = 1 << 0,
    kValueIsString = 1 << 1,
    kValueIsProto = 1 << 2,
    kUseDestructFunc = 1 << 3,
  };
  template <typename Key, typename Value>
  static constexpr uint8_t MakeDestroyBits() {
    uint8_t result = 0;
    if (!std::is_trivially_destructible<Key>::value) {
      if (std::is_same<Key, std::string>::value) {
        result |= kKeyIsString;
      } else {
        return kUseDestructFunc;
      }
    }
    if (!std::is_trivially_destructible<Value>::value) {
      if (std::is_same<Value, std::string>::value) {
        result |= kValueIsString;
      } else if (std::is_base_of<MessageLite, Value>::value) {
        result |= kValueIsProto;
      } else {
        return kUseDestructFunc;
      }
    }
    return result;
  }

  struct ClearInput {
    MapNodeSizeInfoT size_info;
    uint8_t destroy_bits;
    bool reset_table;
    void (*destroy_node)(NodeBase*);
  };

  template <typename Node>
  static void DestroyNode(NodeBase* node) {
    static_cast<Node*>(node)->~Node();
  }

  template <typename Node>
  static constexpr ClearInput MakeClearInput(bool reset) {
    constexpr auto bits =
        MakeDestroyBits<typename Node::key_type, typename Node::mapped_type>();
    return ClearInput{Node::size_info(), bits, reset,
                      bits & kUseDestructFunc ? DestroyNode<Node> : nullptr};
  }

  void ClearTable(ClearInput input);

  NodeAndBucket FindFromTree(map_index_t b, VariantKey key,
                             Tree::iterator* it) const;

  // Space used for the table, trees, and nodes.
  // Does not include the indirect space used. Eg the data of a std::string.
  size_t SpaceUsedInTable(size_t sizeof_node) const;

  map_index_t num_elements_;
  map_index_t num_buckets_;
  map_index_t seed_;
  map_index_t index_of_first_non_null_;
  TableEntryPtr* table_;  // an array with num_buckets_ entries
  Allocator alloc_;
};

inline UntypedMapIterator::UntypedMapIterator(const UntypedMapBase* m) : m_(m) {
  if (m_->index_of_first_non_null_ == m_->num_buckets_) {
    bucket_index_ = 0;
    node_ = nullptr;
  } else {
    bucket_index_ = m_->index_of_first_non_null_;
    TableEntryPtr entry = m_->table_[bucket_index_];
    node_ = PROTOBUF_PREDICT_TRUE(TableEntryIsList(entry))
                ? TableEntryToNode(entry)
                : TableEntryToTree(entry)->begin()->second;
    PROTOBUF_ASSUME(node_ != nullptr);
  }
}

inline void UntypedMapIterator::SearchFrom(map_index_t start_bucket) {
  ABSL_DCHECK(m_->index_of_first_non_null_ == m_->num_buckets_ ||
              !m_->TableEntryIsEmpty(m_->index_of_first_non_null_));
  for (map_index_t i = start_bucket; i < m_->num_buckets_; ++i) {
    TableEntryPtr entry = m_->table_[i];
    if (entry == TableEntryPtr{}) continue;
    bucket_index_ = i;
    if (PROTOBUF_PREDICT_TRUE(TableEntryIsList(entry))) {
      node_ = TableEntryToNode(entry);
    } else {
      TreeForMap* tree = TableEntryToTree(entry);
      ABSL_DCHECK(!tree->empty());
      node_ = tree->begin()->second;
    }
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
    return vtable_->get_map(*this, false);
  }
  UntypedMapBase* MutableMap() {
    return &const_cast<UntypedMapBase&>(vtable_->get_map(*this, true));
  }

 protected:
  struct VTable {
    const UntypedMapBase& (*get_map)(const MapFieldBaseForParse&,
                                     bool is_mutable);
  };
  explicit constexpr MapFieldBaseForParse(const VTable* vtable)
      : vtable_(vtable) {}
  ~MapFieldBaseForParse() = default;

  const VTable* vtable_;
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

// KeyMapBase is a chaining hash map with the additional feature that some
// buckets can be converted to use an ordered container.  This ensures O(lg n)
// bounds on find, insert, and erase, while avoiding the overheads of ordered
// containers most of the time.
//
// The implementation doesn't need the full generality of unordered_map,
// and it doesn't have it.  More bells and whistles can be added as needed.
// Some implementation details:
// 1. The number of buckets is a power of two.
// 2. As is typical for hash_map and such, the Keys and Values are always
//    stored in linked list nodes.  Pointers to elements are never invalidated
//    until the element is deleted.
// 3. The trees' payload type is pointer to linked-list node.  Tree-converting
//    a bucket doesn't copy Key-Value pairs.
// 4. Once we've tree-converted a bucket, it is never converted back unless the
//    bucket is completely emptied out. Note that the items a tree contains may
//    wind up assigned to trees or lists upon a rehash.
// 5. Mutations to a map do not invalidate the map's iterators, pointers to
//    elements, or references to elements.
// 6. Except for erase(iterator), any non-const method can reorder iterators.
// 7. Uses VariantKey when using the Tree representation, which holds all
//    possible key types as a variant value.

template <typename Key>
class KeyMapBase : public UntypedMapBase {
  static_assert(!std::is_signed<Key>::value || !std::is_integral<Key>::value,
                "");

  using TS = TransparentSupport<Key>;

 public:
  using hasher = typename TS::hash;

  using UntypedMapBase::UntypedMapBase;

 protected:
  using KeyNode = internal::KeyNode<Key>;

  // Trees. The payload type is a copy of Key, so that we can query the tree
  // with Keys that are not in any particular data structure.
  // The value is a void* pointing to Node. We use void* instead of Node* to
  // avoid code bloat. That way there is only one instantiation of the tree
  // class per key type.
  using Tree = internal::TreeForMap;
  using TreeIterator = typename Tree::iterator;

 public:
  hasher hash_function() const { return {}; }

 protected:
  friend class TcParser;
  friend struct MapTestPeer;
  friend struct MapBenchmarkPeer;

  PROTOBUF_NOINLINE void erase_no_destroy(map_index_t b, KeyNode* node) {
    TreeIterator tree_it;
    const bool is_list = revalidate_if_necessary(b, node, &tree_it);
    if (is_list) {
      ABSL_DCHECK(TableEntryIsNonEmptyList(b));
      auto* head = TableEntryToNode(table_[b]);
      head = EraseFromLinkedList(node, head);
      table_[b] = NodeToTableEntry(head);
    } else {
      EraseFromTree(b, tree_it);
    }
    --num_elements_;
    if (PROTOBUF_PREDICT_FALSE(b == index_of_first_non_null_)) {
      while (index_of_first_non_null_ < num_buckets_ &&
             TableEntryIsEmpty(index_of_first_non_null_)) {
        ++index_of_first_non_null_;
      }
    }
  }

  NodeAndBucket FindHelper(typename TS::ViewType k,
                           TreeIterator* it = nullptr) const {
    map_index_t b = BucketNumber(k);
    if (TableEntryIsNonEmptyList(b)) {
      auto* node = internal::TableEntryToNode(table_[b]);
      do {
        if (TS::Equals(static_cast<KeyNode*>(node)->key(), k)) {
          return {node, b};
        } else {
          node = node->next;
        }
      } while (node != nullptr);
    } else if (TableEntryIsTree(b)) {
      return FindFromTree(b, internal::RealKeyToVariantKey<Key>{}(k), it);
    }
    return {nullptr, b};
  }

  // Insert the given node.
  // If the key is a duplicate, it inserts the new node and returns the old one.
  // Gives ownership to the caller.
  // If the key is unique, it returns `nullptr`.
  KeyNode* InsertOrReplaceNode(KeyNode* node) {
    KeyNode* to_erase = nullptr;
    auto p = this->FindHelper(node->key());
    if (p.node != nullptr) {
      erase_no_destroy(p.bucket, static_cast<KeyNode*>(p.node));
      to_erase = static_cast<KeyNode*>(p.node);
    } else if (ResizeIfLoadIsOutOfRange(num_elements_ + 1)) {
      p = FindHelper(node->key());
    }
    const map_index_t b = p.bucket;  // bucket number
    InsertUnique(b, node);
    ++num_elements_;
    return to_erase;
  }

  // Insert the given Node in bucket b.  If that would make bucket b too big,
  // and bucket b is not a tree, create a tree for buckets b.
  // Requires count(*KeyPtrFromNodePtr(node)) == 0 and that b is the correct
  // bucket.  num_elements_ is not modified.
  void InsertUnique(map_index_t b, KeyNode* node) {
    ABSL_DCHECK(index_of_first_non_null_ == num_buckets_ ||
                !TableEntryIsEmpty(index_of_first_non_null_));
    // In practice, the code that led to this point may have already
    // determined whether we are inserting into an empty list, a short list,
    // or whatever.  But it's probably cheap enough to recompute that here;
    // it's likely that we're inserting into an empty or short list.
    ABSL_DCHECK(FindHelper(node->key()).node == nullptr);
    if (TableEntryIsEmpty(b)) {
      InsertUniqueInList(b, node);
      index_of_first_non_null_ = (std::min)(index_of_first_non_null_, b);
    } else if (TableEntryIsNonEmptyList(b) && !TableEntryIsTooLong(b)) {
      InsertUniqueInList(b, node);
    } else {
      InsertUniqueInTree(b, NodeToVariantKey, node);
    }
  }

  static VariantKey NodeToVariantKey(NodeBase* node) {
    return internal::RealKeyToVariantKey<Key>{}(
        static_cast<KeyNode*>(node)->key());
  }

  // Returns whether it did resize.  Currently this is only used when
  // num_elements_ increases, though it could be used in other situations.
  // It checks for load too low as well as load too high: because any number
  // of erases can occur between inserts, the load could be as low as 0 here.
  // Resizing to a lower size is not always helpful, but failing to do so can
  // destroy the expected big-O bounds for some operations. By having the
  // policy that sometimes we resize down as well as up, clients can easily
  // keep O(size()) = O(number of buckets) if they want that.
  bool ResizeIfLoadIsOutOfRange(size_type new_size) {
    const size_type kMaxMapLoadTimes16 = 12;  // controls RAM vs CPU tradeoff
    const size_type hi_cutoff = num_buckets_ * kMaxMapLoadTimes16 / 16;
    const size_type lo_cutoff = hi_cutoff / 4;
    // We don't care how many elements are in trees.  If a lot are,
    // we may resize even though there are many empty buckets.  In
    // practice, this seems fine.
    if (PROTOBUF_PREDICT_FALSE(new_size >= hi_cutoff)) {
      if (num_buckets_ <= max_size() / 2) {
        Resize(num_buckets_ * 2);
        return true;
      }
    } else if (PROTOBUF_PREDICT_FALSE(new_size <= lo_cutoff &&
                                      num_buckets_ > kMinTableSize)) {
      size_type lg2_of_size_reduction_factor = 1;
      // It's possible we want to shrink a lot here... size() could even be 0.
      // So, estimate how much to shrink by making sure we don't shrink so
      // much that we would need to grow the table after a few inserts.
      const size_type hypothetical_size = new_size * 5 / 4 + 1;
      while ((hypothetical_size << lg2_of_size_reduction_factor) < hi_cutoff) {
        ++lg2_of_size_reduction_factor;
      }
      size_type new_num_buckets = std::max<size_type>(
          kMinTableSize, num_buckets_ >> lg2_of_size_reduction_factor);
      if (new_num_buckets != num_buckets_) {
        Resize(new_num_buckets);
        return true;
      }
    }
    return false;
  }

  // Resize to the given number of buckets.
  void Resize(map_index_t new_num_buckets) {
    if (num_buckets_ == kGlobalEmptyTableSize) {
      // This is the global empty array.
      // Just overwrite with a new one. No need to transfer or free anything.
      num_buckets_ = index_of_first_non_null_ = kMinTableSize;
      table_ = CreateEmptyTable(num_buckets_);
      seed_ = Seed();
      return;
    }

    ABSL_DCHECK_GE(new_num_buckets, kMinTableSize);
    const auto old_table = table_;
    const map_index_t old_table_size = num_buckets_;
    num_buckets_ = new_num_buckets;
    table_ = CreateEmptyTable(num_buckets_);
    const map_index_t start = index_of_first_non_null_;
    index_of_first_non_null_ = num_buckets_;
    for (map_index_t i = start; i < old_table_size; ++i) {
      if (internal::TableEntryIsNonEmptyList(old_table[i])) {
        TransferList(static_cast<KeyNode*>(TableEntryToNode(old_table[i])));
      } else if (internal::TableEntryIsTree(old_table[i])) {
        this->TransferTree(TableEntryToTree(old_table[i]), NodeToVariantKey);
      }
    }
    DeleteTable(old_table, old_table_size);
  }

  // Transfer all nodes in the list `node` into `this`.
  void TransferList(KeyNode* node) {
    do {
      auto* next = static_cast<KeyNode*>(node->next);
      InsertUnique(BucketNumber(node->key()), node);
      node = next;
    } while (node != nullptr);
  }

  map_index_t BucketNumber(typename TS::ViewType k) const {
    ABSL_DCHECK_EQ(BucketNumberFromHash(hash_function()(k)),
                   VariantBucketNumber(RealKeyToVariantKey<Key>{}(k)));
    return BucketNumberFromHash(hash_function()(k));
  }

  // Assumes node_ and m_ are correct and non-null, but other fields may be
  // stale.  Fix them as needed.  Then return true iff node_ points to a
  // Node in a list.  If false is returned then *it is modified to be
  // a valid iterator for node_.
  bool revalidate_if_necessary(map_index_t& bucket_index, KeyNode* node,
                               TreeIterator* it) const {
    // Force bucket_index to be in range.
    bucket_index &= (num_buckets_ - 1);
    // Common case: the bucket we think is relevant points to `node`.
    if (table_[bucket_index] == NodeToTableEntry(node)) return true;
    // Less common: the bucket is a linked list with node_ somewhere in it,
    // but not at the head.
    if (TableEntryIsNonEmptyList(bucket_index)) {
      auto* l = TableEntryToNode(table_[bucket_index]);
      while ((l = l->next) != nullptr) {
        if (l == node) {
          return true;
        }
      }
    }
    // Well, bucket_index_ still might be correct, but probably
    // not.  Revalidate just to be sure.  This case is rare enough that we
    // don't worry about potential optimizations, such as having a custom
    // find-like method that compares Node* instead of the key.
    auto res = FindHelper(node->key(), it);
    bucket_index = res.bucket;
    return TableEntryIsList(bucket_index);
  }
};

template <typename T, typename K>
bool InitializeMapKey(T*, K&&, Arena*) {
  return false;
}


}  // namespace internal

// This is the class for Map's internal value_type.
template <typename Key, typename T>
using MapPair = std::pair<const Key, T>;

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
class Map : private internal::KeyMapBase<internal::KeyForBase<Key>> {
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
  using hasher = typename TS::hash;

  constexpr Map() : Base(nullptr) { StaticValidityCheck(); }
  Map(const Map& other) : Map(nullptr, other) {}

  // Internal Arena constructors: do not use!
  // TODO: remove non internal ctors
  explicit Map(Arena* arena) : Base(arena) { StaticValidityCheck(); }
  Map(internal::InternalVisibility, Arena* arena) : Map(arena) {}
  Map(internal::InternalVisibility, Arena* arena, const Map& other)
      : Map(arena, other) {}

  Map(Map&& other) noexcept : Map() {
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

    if (this->num_buckets_ != internal::kGlobalEmptyTableSize) {
      this->ClearTable(this->template MakeClearInput<Node>(false));
    }
  }

 private:
  Map(Arena* arena, const Map& other) : Base(arena) {
    StaticValidityCheck();
    insert(other.begin(), other.end());
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
        absl::disjunction<internal::is_supported_integral_type<key_type>,
                          internal::is_supported_string_type<key_type>,
                          internal::is_internal_map_key_type<key_type>>::value,
        "We only support integer, string, or designated internal key "
        "types.");
    static_assert(absl::disjunction<
                      internal::is_supported_scalar_type<mapped_type>,
                      is_proto_enum<mapped_type>,
                      internal::is_supported_message_type<mapped_type>,
                      internal::is_internal_map_value_type<mapped_type>>::value,
                  "We only support scalar, Message, and designated internal "
                  "mapped types.");
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
  class const_iterator : private internal::UntypedMapIterator {
    using BaseIt = internal::UntypedMapIterator;

   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename Map::value_type;
    using difference_type = ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;

    const_iterator() {}
    const_iterator(const const_iterator&) = default;
    const_iterator& operator=(const const_iterator&) = default;

    reference operator*() const { return static_cast<Node*>(this->node_)->kv; }
    pointer operator->() const { return &(operator*()); }

    const_iterator& operator++() {
      this->PlusPlus();
      return *this;
    }
    const_iterator operator++(int) {
      auto copy = *this;
      this->PlusPlus();
      return copy;
    }

    friend bool operator==(const const_iterator& a, const const_iterator& b) {
      return a.Equals(b);
    }
    friend bool operator!=(const const_iterator& a, const const_iterator& b) {
      return !a.Equals(b);
    }

   private:
    using BaseIt::BaseIt;
    explicit const_iterator(const BaseIt& base) : BaseIt(base) {}
    friend class Map;
    friend class internal::TypeDefinedMapFieldBase<Key, T>;
  };

  class iterator : private internal::UntypedMapIterator {
    using BaseIt = internal::UntypedMapIterator;

   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename Map::value_type;
    using difference_type = ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    iterator() {}
    iterator(const iterator&) = default;
    iterator& operator=(const iterator&) = default;

    reference operator*() const { return static_cast<Node*>(this->node_)->kv; }
    pointer operator->() const { return &(operator*()); }

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
    operator const_iterator() const {  // NOLINT(runtime/explicit)
      return const_iterator(static_cast<const BaseIt&>(*this));
    }

    friend bool operator==(const iterator& a, const iterator& b) {
      return a.Equals(b);
    }
    friend bool operator!=(const iterator& a, const iterator& b) {
      return !a.Equals(b);
    }

   private:
    using BaseIt::BaseIt;
    friend class Map;
  };

  iterator begin() ABSL_ATTRIBUTE_LIFETIME_BOUND { return iterator(this); }
  iterator end() ABSL_ATTRIBUTE_LIFETIME_BOUND { return iterator(); }
  const_iterator begin() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return const_iterator(this);
  }
  const_iterator end() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return const_iterator();
  }
  const_iterator cbegin() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return begin();
  }
  const_iterator cend() const ABSL_ATTRIBUTE_LIFETIME_BOUND { return end(); }

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
    return try_emplace(std::forward<K>(key)).first->second;
  }

  template <typename K = key_type>
  const T& at(const key_arg<K>& key) const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    const_iterator it = find(key);
    ABSL_CHECK(it != end()) << "key not found: " << static_cast<Key>(key);
    return it->second;
  }

  template <typename K = key_type>
  T& at(const key_arg<K>& key) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    iterator it = find(key);
    ABSL_CHECK(it != end()) << "key not found: " << static_cast<Key>(key);
    return it->second;
  }

  // Lookup
  template <typename K = key_type>
  size_type count(const key_arg<K>& key) const {
    return find(key) == end() ? 0 : 1;
  }

  template <typename K = key_type>
  const_iterator find(const key_arg<K>& key) const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return const_cast<Map*>(this)->find(key);
  }
  template <typename K = key_type>
  iterator find(const key_arg<K>& key) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    auto res = this->FindHelper(TS::ToView(key));
    return iterator(static_cast<Node*>(res.node), this, res.bucket);
  }

  template <typename K = key_type>
  bool contains(const key_arg<K>& key) const {
    return find(key) != end();
  }

  template <typename K = key_type>
  std::pair<const_iterator, const_iterator> equal_range(
      const key_arg<K>& key) const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    const_iterator it = find(key);
    if (it == end()) {
      return std::pair<const_iterator, const_iterator>(it, it);
    } else {
      const_iterator begin = it++;
      return std::pair<const_iterator, const_iterator>(begin, it);
    }
  }

  template <typename K = key_type>
  std::pair<iterator, iterator> equal_range(const key_arg<K>& key)
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
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
    // Inserts a new element into the container if there is no element with the
    // key in the container.
    // The new element is:
    //  (1) Constructed in-place with the given args, if mapped_type is not
    //      arena constructible.
    //  (2) Constructed in-place with the arena and then assigned with a
    //      mapped_type temporary constructed with the given args, otherwise.
    return ArenaAwareTryEmplace(Arena::is_arena_constructable<mapped_type>(),
                                std::forward<K>(k),
                                std::forward<Args>(args)...);
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
    return EmplaceInternal(Rank0{}, std::forward<Args>(args)...);
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
    iterator it = find(key);
    if (it == end()) {
      return 0;
    } else {
      erase(it);
      return 1;
    }
  }

  iterator erase(iterator pos) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    auto next = std::next(pos);
    ABSL_DCHECK_EQ(pos.m_, static_cast<Base*>(this));
    auto* node = static_cast<Node*>(pos.node_);
    this->erase_no_destroy(pos.bucket_index_, node);
    DestroyNode(node);
    return next;
  }

  void erase(iterator first, iterator last) {
    while (first != last) {
      first = erase(first);
    }
  }

  void clear() {
    if (this->num_buckets_ == internal::kGlobalEmptyTableSize) return;
    this->ClearTable(this->template MakeClearInput<Node>(true));
  }

  // Assign
  Map& operator=(const Map& other) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (this != &other) {
      clear();
      insert(other.begin(), other.end());
    }
    return *this;
  }

  void swap(Map& other) {
    if (arena() == other.arena()) {
      InternalSwap(&other);
    } else {
      // TODO: optimize this. The temporary copy can be allocated
      // in the same arena as the other message, and the "other = copy" can
      // be replaced with the fast-path swap above.
      Map copy = *this;
      *this = other;
      other = copy;
    }
  }

  void InternalSwap(Map* other) {
    internal::UntypedMapBase::InternalSwap(other);
  }

  hasher hash_function() const { return {}; }

  size_t SpaceUsedExcludingSelfLong() const {
    if (empty()) return 0;
    return SpaceUsedInternal() + internal::SpaceUsedInValues(this);
  }

 private:
  struct Rank1 {};
  struct Rank0 : Rank1 {};

  // Linked-list nodes, as one would expect for a chaining hash table.
  struct Node : Base::KeyNode {
    using key_type = Key;
    using mapped_type = T;
    static constexpr internal::MapNodeSizeInfoT size_info() {
      return internal::MakeNodeInfo(sizeof(Node),
                                    PROTOBUF_FIELD_OFFSET(Node, kv.second));
    }
    value_type kv;
  };

  using Tree = internal::TreeForMap;
  using TreeIterator = typename Tree::iterator;
  using TableEntryPtr = internal::TableEntryPtr;

  static Node* NodeFromTreeIterator(TreeIterator it) {
    static_assert(
        PROTOBUF_FIELD_OFFSET(Node, kv.first) == Base::KeyNode::kOffset, "");
    static_assert(alignof(Node) == alignof(internal::NodeBase), "");
    return static_cast<Node*>(it->second);
  }

  void DestroyNode(Node* node) {
    if (this->alloc_.arena() == nullptr) {
      node->kv.first.~key_type();
      node->kv.second.~mapped_type();
      this->DeallocNode(node, sizeof(Node));
    }
  }

  size_t SpaceUsedInternal() const {
    return this->SpaceUsedInTable(sizeof(Node));
  }

  // We try to construct `init_type` from `Args` with a fall back to
  // `value_type`. The latter is less desired as it unconditionally makes a copy
  // of `value_type::first`.
  template <typename... Args>
  auto EmplaceInternal(Rank0, Args&&... args) ->
      typename std::enable_if<std::is_constructible<init_type, Args...>::value,
                              std::pair<iterator, bool>>::type {
    return insert(init_type(std::forward<Args>(args)...));
  }
  template <typename... Args>
  std::pair<iterator, bool> EmplaceInternal(Rank1, Args&&... args) {
    return insert(value_type(std::forward<Args>(args)...));
  }

  template <typename K, typename... Args>
  std::pair<iterator, bool> TryEmplaceInternal(K&& k, Args&&... args) {
    auto p = this->FindHelper(TS::ToView(k));
    // Case 1: key was already present.
    if (p.node != nullptr)
      return std::make_pair(
          iterator(static_cast<Node*>(p.node), this, p.bucket), false);
    // Case 2: insert.
    if (this->ResizeIfLoadIsOutOfRange(this->num_elements_ + 1)) {
      p = this->FindHelper(TS::ToView(k));
    }
    const auto b = p.bucket;  // bucket number
    // If K is not key_type, make the conversion to key_type explicit.
    using TypeToInit = typename std::conditional<
        std::is_same<typename std::decay<K>::type, key_type>::value, K&&,
        key_type>::type;
    Node* node = static_cast<Node*>(this->AllocNode(sizeof(Node)));

    // Even when arena is nullptr, CreateInArenaStorage is still used to
    // ensure the arena of submessage will be consistent. Otherwise,
    // submessage may have its own arena when message-owned arena is enabled.
    // Note: This only works if `Key` is not arena constructible.
    if (!internal::InitializeMapKey(const_cast<Key*>(&node->kv.first),
                                    std::forward<K>(k), this->alloc_.arena())) {
      Arena::CreateInArenaStorage(const_cast<Key*>(&node->kv.first),
                                  this->alloc_.arena(),
                                  static_cast<TypeToInit>(std::forward<K>(k)));
    }
    // Note: if `T` is arena constructible, `Args` needs to be empty.
    Arena::CreateInArenaStorage(&node->kv.second, this->alloc_.arena(),
                                std::forward<Args>(args)...);

    this->InsertUnique(b, node);
    ++this->num_elements_;
    return std::make_pair(iterator(node, this, b), true);
  }

  // A helper function to perform an assignment of `mapped_type`.
  // If the first argument is true, then it is a regular assignment.
  // Otherwise, we first create a temporary and then perform an assignment.
  template <typename V>
  static void AssignMapped(std::true_type, mapped_type& mapped, V&& v) {
    mapped = std::forward<V>(v);
  }
  template <typename... Args>
  static void AssignMapped(std::false_type, mapped_type& mapped,
                           Args&&... args) {
    mapped = mapped_type(std::forward<Args>(args)...);
  }

  // Case 1: `mapped_type` is arena constructible. A temporary object is
  // created and then (if `Args` are not empty) assigned to a mapped value
  // that was created with the arena.
  template <typename K>
  std::pair<iterator, bool> ArenaAwareTryEmplace(std::true_type, K&& k) {
    // case 1.1: "default" constructed (e.g. from arena only).
    return TryEmplaceInternal(std::forward<K>(k));
  }
  template <typename K, typename... Args>
  std::pair<iterator, bool> ArenaAwareTryEmplace(std::true_type, K&& k,
                                                 Args&&... args) {
    // case 1.2: "default" constructed + copy/move assignment
    auto p = TryEmplaceInternal(std::forward<K>(k));
    if (p.second) {
      AssignMapped(std::is_same<void(typename std::decay<Args>::type...),
                                void(mapped_type)>(),
                   p.first->second, std::forward<Args>(args)...);
    }
    return p;
  }
  // Case 2: `mapped_type` is not arena constructible. Using in-place
  // construction.
  template <typename... Args>
  std::pair<iterator, bool> ArenaAwareTryEmplace(std::false_type,
                                                 Args&&... args) {
    return TryEmplaceInternal(std::forward<Args>(args)...);
  }

  using Base::arena;

  friend class Arena;
  template <typename, typename>
  friend class internal::TypeDefinedMapFieldBase;
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;
  template <typename K, typename V>
  friend class internal::MapFieldLite;
  friend class internal::TcParser;
  friend struct internal::MapTestPeer;
  friend struct internal::MapBenchmarkPeer;
};

namespace internal {
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
