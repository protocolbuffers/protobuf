// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_MAP_FIELD_H__
#define GOOGLE_PROTOBUF_MAP_FIELD_H__

#include <atomic>
#include <functional>
#include <type_traits>

#include "absl/synchronization/mutex.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/map.h"
#include "google/protobuf/map_entry.h"
#include "google/protobuf/map_field_lite.h"
#include "google/protobuf/map_type_handler.h"
#include "google/protobuf/message.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/unknown_field_set.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {
class DynamicMessage;
class MapIterator;

// Microsoft compiler complains about non-virtual destructor,
// even when the destructor is private.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4265)
#endif  // _MSC_VER

#define TYPE_CHECK(EXPECTEDTYPE, METHOD)                                  \
  if (type() != EXPECTEDTYPE) {                                           \
    ABSL_LOG(FATAL) << "Protocol Buffer map usage error:\n"               \
                    << METHOD << " type does not match\n"                 \
                    << "  Expected : "                                    \
                    << FieldDescriptor::CppTypeName(EXPECTEDTYPE) << "\n" \
                    << "  Actual   : "                                    \
                    << FieldDescriptor::CppTypeName(type());              \
  }

// MapKey is an union type for representing any possible
// map key.
class PROTOBUF_EXPORT MapKey {
 public:
  MapKey() : type_() {}
  MapKey(const MapKey& other) : type_() { CopyFrom(other); }

  MapKey& operator=(const MapKey& other) {
    CopyFrom(other);
    return *this;
  }

  ~MapKey() {
    if (type_ == FieldDescriptor::CPPTYPE_STRING) {
      val_.string_value_.Destruct();
    }
  }

  FieldDescriptor::CppType type() const {
    if (type_ == FieldDescriptor::CppType()) {
      ABSL_LOG(FATAL) << "Protocol Buffer map usage error:\n"
                      << "MapKey::type MapKey is not initialized. "
                      << "Call set methods to initialize MapKey.";
    }
    return type_;
  }

  void SetInt64Value(int64_t value) {
    SetType(FieldDescriptor::CPPTYPE_INT64);
    val_.int64_value_ = value;
  }
  void SetUInt64Value(uint64_t value) {
    SetType(FieldDescriptor::CPPTYPE_UINT64);
    val_.uint64_value_ = value;
  }
  void SetInt32Value(int32_t value) {
    SetType(FieldDescriptor::CPPTYPE_INT32);
    val_.int32_value_ = value;
  }
  void SetUInt32Value(uint32_t value) {
    SetType(FieldDescriptor::CPPTYPE_UINT32);
    val_.uint32_value_ = value;
  }
  void SetBoolValue(bool value) {
    SetType(FieldDescriptor::CPPTYPE_BOOL);
    val_.bool_value_ = value;
  }
  void SetStringValue(std::string val) {
    SetType(FieldDescriptor::CPPTYPE_STRING);
    *val_.string_value_.get_mutable() = std::move(val);
  }

  int64_t GetInt64Value() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_INT64, "MapKey::GetInt64Value");
    return val_.int64_value_;
  }
  uint64_t GetUInt64Value() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_UINT64, "MapKey::GetUInt64Value");
    return val_.uint64_value_;
  }
  int32_t GetInt32Value() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_INT32, "MapKey::GetInt32Value");
    return val_.int32_value_;
  }
  uint32_t GetUInt32Value() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_UINT32, "MapKey::GetUInt32Value");
    return val_.uint32_value_;
  }
  bool GetBoolValue() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_BOOL, "MapKey::GetBoolValue");
    return val_.bool_value_;
  }
  const std::string& GetStringValue() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_STRING, "MapKey::GetStringValue");
    return val_.string_value_.get();
  }

  bool operator<(const MapKey& other) const {
    if (type_ != other.type_) {
      // We could define a total order that handles this case, but
      // there currently no need.  So, for now, fail.
      ABSL_LOG(FATAL) << "Unsupported: type mismatch";
    }
    switch (type()) {
      case FieldDescriptor::CPPTYPE_DOUBLE:
      case FieldDescriptor::CPPTYPE_FLOAT:
      case FieldDescriptor::CPPTYPE_ENUM:
      case FieldDescriptor::CPPTYPE_MESSAGE:
        ABSL_LOG(FATAL) << "Unsupported";
        return false;
      case FieldDescriptor::CPPTYPE_STRING:
        return val_.string_value_.get() < other.val_.string_value_.get();
      case FieldDescriptor::CPPTYPE_INT64:
        return val_.int64_value_ < other.val_.int64_value_;
      case FieldDescriptor::CPPTYPE_INT32:
        return val_.int32_value_ < other.val_.int32_value_;
      case FieldDescriptor::CPPTYPE_UINT64:
        return val_.uint64_value_ < other.val_.uint64_value_;
      case FieldDescriptor::CPPTYPE_UINT32:
        return val_.uint32_value_ < other.val_.uint32_value_;
      case FieldDescriptor::CPPTYPE_BOOL:
        return val_.bool_value_ < other.val_.bool_value_;
    }
    return false;
  }

  bool operator==(const MapKey& other) const {
    if (type_ != other.type_) {
      // To be consistent with operator<, we don't allow this either.
      ABSL_LOG(FATAL) << "Unsupported: type mismatch";
    }
    switch (type()) {
      case FieldDescriptor::CPPTYPE_DOUBLE:
      case FieldDescriptor::CPPTYPE_FLOAT:
      case FieldDescriptor::CPPTYPE_ENUM:
      case FieldDescriptor::CPPTYPE_MESSAGE:
        ABSL_LOG(FATAL) << "Unsupported";
        break;
      case FieldDescriptor::CPPTYPE_STRING:
        return val_.string_value_.get() == other.val_.string_value_.get();
      case FieldDescriptor::CPPTYPE_INT64:
        return val_.int64_value_ == other.val_.int64_value_;
      case FieldDescriptor::CPPTYPE_INT32:
        return val_.int32_value_ == other.val_.int32_value_;
      case FieldDescriptor::CPPTYPE_UINT64:
        return val_.uint64_value_ == other.val_.uint64_value_;
      case FieldDescriptor::CPPTYPE_UINT32:
        return val_.uint32_value_ == other.val_.uint32_value_;
      case FieldDescriptor::CPPTYPE_BOOL:
        return val_.bool_value_ == other.val_.bool_value_;
    }
    ABSL_LOG(FATAL) << "Can't get here.";
    return false;
  }

  void CopyFrom(const MapKey& other) {
    SetType(other.type());
    switch (type_) {
      case FieldDescriptor::CPPTYPE_DOUBLE:
      case FieldDescriptor::CPPTYPE_FLOAT:
      case FieldDescriptor::CPPTYPE_ENUM:
      case FieldDescriptor::CPPTYPE_MESSAGE:
        ABSL_LOG(FATAL) << "Unsupported";
        break;
      case FieldDescriptor::CPPTYPE_STRING:
        *val_.string_value_.get_mutable() = other.val_.string_value_.get();
        break;
      case FieldDescriptor::CPPTYPE_INT64:
        val_.int64_value_ = other.val_.int64_value_;
        break;
      case FieldDescriptor::CPPTYPE_INT32:
        val_.int32_value_ = other.val_.int32_value_;
        break;
      case FieldDescriptor::CPPTYPE_UINT64:
        val_.uint64_value_ = other.val_.uint64_value_;
        break;
      case FieldDescriptor::CPPTYPE_UINT32:
        val_.uint32_value_ = other.val_.uint32_value_;
        break;
      case FieldDescriptor::CPPTYPE_BOOL:
        val_.bool_value_ = other.val_.bool_value_;
        break;
    }
  }

 private:
  template <typename K, typename V>
  friend class internal::TypeDefinedMapFieldBase;
  friend class internal::MapFieldBase;
  friend class MapIterator;
  friend class internal::DynamicMapField;

  union KeyValue {
    KeyValue() {}
    internal::ExplicitlyConstructed<std::string> string_value_;
    int64_t int64_value_;
    int32_t int32_value_;
    uint64_t uint64_value_;
    uint32_t uint32_value_;
    bool bool_value_;
  } val_;

  void SetType(FieldDescriptor::CppType type) {
    if (type_ == type) return;
    if (type_ == FieldDescriptor::CPPTYPE_STRING) {
      val_.string_value_.Destruct();
    }
    type_ = type;
    if (type_ == FieldDescriptor::CPPTYPE_STRING) {
      val_.string_value_.DefaultConstruct();
    }
  }

  // type_ is 0 or a valid FieldDescriptor::CppType.
  // Use "CppType()" to indicate zero.
  FieldDescriptor::CppType type_;
};

namespace internal {

template <>
struct is_internal_map_key_type<MapKey> : std::true_type {};

template <>
struct RealKeyToVariantKey<MapKey> {
  VariantKey operator()(const MapKey& value) const;
};

}  // namespace internal

}  // namespace protobuf
}  // namespace google
namespace std {
template <>
struct hash<google::protobuf::MapKey> {
  size_t operator()(const google::protobuf::MapKey& map_key) const {
    return ::google::protobuf::internal::RealKeyToVariantKey<::google::protobuf::MapKey>{}(map_key)
        .Hash();
  }
  bool operator()(const google::protobuf::MapKey& map_key1,
                  const google::protobuf::MapKey& map_key2) const {
    return map_key1 < map_key2;
  }
};
}  // namespace std

namespace google {
namespace protobuf {
namespace internal {

class ContendedMapCleanTest;
class GeneratedMessageReflection;
class MapFieldAccessor;

// This class provides access to map field using reflection, which is the same
// as those provided for RepeatedPtrField<Message>. It is used for internal
// reflection implementation only. Users should never use this directly.
class PROTOBUF_EXPORT MapFieldBase : public MapFieldBaseForParse {
 public:
  explicit constexpr MapFieldBase(const VTable* vtable)
      : MapFieldBaseForParse(vtable) {}
  explicit MapFieldBase(const VTable* vtable, Arena* arena)
      : MapFieldBaseForParse(vtable), payload_{ToTaggedPtr(arena)} {}
  MapFieldBase(const MapFieldBase&) = delete;
  MapFieldBase& operator=(const MapFieldBase&) = delete;

 protected:
  // "protected" stops users from deleting a `MapFieldBase *`
  ~MapFieldBase();

  struct VTable : MapFieldBaseForParse::VTable {
    bool (*lookup_map_value)(const MapFieldBase& map, const MapKey& map_key,
                             MapValueConstRef* val);
    bool (*delete_map_value)(MapFieldBase& map, const MapKey& map_key);
    void (*set_map_iterator_value)(MapIterator* map_iter);
    bool (*insert_or_lookup_no_sync)(MapFieldBase& map, const MapKey& map_key,
                                     MapValueRef* val);

    void (*clear_map_no_sync)(MapFieldBase& map);
    void (*merge_from)(MapFieldBase& map, const MapFieldBase& other);
    void (*swap)(MapFieldBase& lhs, MapFieldBase& rhs);
    void (*unsafe_shallow_swap)(MapFieldBase& lhs, MapFieldBase& rhs);
    size_t (*space_used_excluding_self_nolock)(const MapFieldBase& map);

    const Message* (*get_prototype)(const MapFieldBase& map);
  };
  template <typename T>
  static constexpr VTable MakeVTable() {
    VTable out{};
    out.get_map = &T::GetMapImpl;
    out.lookup_map_value = &T::LookupMapValueImpl;
    out.delete_map_value = &T::DeleteMapValueImpl;
    out.set_map_iterator_value = &T::SetMapIteratorValueImpl;
    out.insert_or_lookup_no_sync = &T::InsertOrLookupMapValueNoSyncImpl;
    out.clear_map_no_sync = &T::ClearMapNoSyncImpl;
    out.merge_from = &T::MergeFromImpl;
    out.swap = &T::SwapImpl;
    out.unsafe_shallow_swap = &T::UnsafeShallowSwapImpl;
    out.space_used_excluding_self_nolock = &T::SpaceUsedExcludingSelfNoLockImpl;
    out.get_prototype = &T::GetPrototypeImpl;
    return out;
  }

 public:
  // Returns reference to internal repeated field. Data written using
  // Map's api prior to calling this function is guarantted to be
  // included in repeated field.
  const RepeatedPtrFieldBase& GetRepeatedField() const;

  // Like above. Returns mutable pointer to the internal repeated field.
  RepeatedPtrFieldBase* MutableRepeatedField();

  const VTable* vtable() const { return static_cast<const VTable*>(vtable_); }

  bool ContainsMapKey(const MapKey& map_key) const {
    return LookupMapValue(map_key, static_cast<MapValueConstRef*>(nullptr));
  }
  bool LookupMapValue(const MapKey& map_key, MapValueConstRef* val) const {
    return vtable()->lookup_map_value(*this, map_key, val);
  }
  bool LookupMapValue(const MapKey&, MapValueRef*) const = delete;

  bool InsertOrLookupMapValue(const MapKey& map_key, MapValueRef* val);

  // Returns whether changes to the map are reflected in the repeated field.
  bool IsRepeatedFieldValid() const;
  // Insures operations after won't get executed before calling this.
  bool IsMapValid() const;
  bool DeleteMapValue(const MapKey& map_key) {
    return vtable()->delete_map_value(*this, map_key);
  }
  void MergeFrom(const MapFieldBase& other) {
    vtable()->merge_from(*this, other);
  }
  void Swap(MapFieldBase* other) { vtable()->swap(*this, *other); }
  void UnsafeShallowSwap(MapFieldBase* other) {
    vtable()->unsafe_shallow_swap(*this, *other);
  }
  // Sync Map with repeated field and returns the size of map.
  int size() const;
  void Clear();
  void SetMapIteratorValue(MapIterator* map_iter) const {
    return vtable()->set_map_iterator_value(map_iter);
  }

  void MapBegin(MapIterator* map_iter) const;
  void MapEnd(MapIterator* map_iter) const;
  bool EqualIterator(const MapIterator& a, const MapIterator& b) const;

  // Returns the number of bytes used by the repeated field, excluding
  // sizeof(*this)
  size_t SpaceUsedExcludingSelfLong() const;

  int SpaceUsedExcludingSelf() const {
    return internal::ToIntSize(SpaceUsedExcludingSelfLong());
  }

 protected:
  // Gets the size of space used by map field.
  size_t SpaceUsedExcludingSelfNoLock() const {
    return vtable()->space_used_excluding_self_nolock(*this);
  }

  const Message* GetPrototype() const { return vtable()->get_prototype(*this); }
  void ClearMapNoSync() { return vtable()->clear_map_no_sync(*this); }

  // Synchronizes the content in Map to RepeatedPtrField if there is any change
  // to Map after last synchronization.
  void SyncRepeatedFieldWithMap() const;
  void SyncRepeatedFieldWithMapNoLock();

  // Synchronizes the content in RepeatedPtrField to Map if there is any change
  // to RepeatedPtrField after last synchronization.
  void SyncMapWithRepeatedField() const;
  void SyncMapWithRepeatedFieldNoLock();

  static void SwapImpl(MapFieldBase& lhs, MapFieldBase& rhs);
  static void UnsafeShallowSwapImpl(MapFieldBase& lhs, MapFieldBase& rhs);

  // Tells MapFieldBase that there is new change to Map.
  void SetMapDirty();

  // Tells MapFieldBase that there is new change to RepeatedPtrField.
  void SetRepeatedDirty();

  // Provides derived class the access to repeated field.
  void* MutableRepeatedPtrField() const;

  bool InsertOrLookupMapValueNoSync(const MapKey& map_key, MapValueRef* val) {
    return vtable()->insert_or_lookup_no_sync(*this, map_key, val);
  }

  void InternalSwap(MapFieldBase* other);

  // Support thread sanitizer (tsan) by making const / mutable races
  // more apparent.  If one thread calls MutableAccess() while another
  // thread calls either ConstAccess() or MutableAccess(), on the same
  // MapFieldBase-derived object, and there is no synchronization going
  // on between them, tsan will alert.
#if defined(__SANITIZE_THREAD__) || defined(THREAD_SANITIZER)
  void ConstAccess() const { ABSL_CHECK_EQ(seq1_, seq2_); }
  void MutableAccess() {
    if (seq1_ & 1) {
      seq2_ = ++seq1_;
    } else {
      seq1_ = ++seq2_;
    }
  }
  unsigned int seq1_ = 0, seq2_ = 0;
#else
  void ConstAccess() const {}
  void MutableAccess() {}
#endif
  enum State {
    STATE_MODIFIED_MAP = 0,       // map has newly added data that has not been
                                  // synchronized to repeated field
    STATE_MODIFIED_REPEATED = 1,  // repeated field has newly added data that
                                  // has not been synchronized to map
    CLEAN = 2,                    // data in map and repeated field are same
  };

  struct ReflectionPayload {
    explicit ReflectionPayload(Arena* arena) : repeated_field(arena) {}
    RepeatedPtrField<Message> repeated_field;

    absl::Mutex mutex;  // The thread to synchronize map and repeated
                        // field needs to get lock first;
    std::atomic<State> state{STATE_MODIFIED_MAP};
  };

  Arena* arena() const {
    auto p = payload_.load(std::memory_order_acquire);
    if (IsPayload(p)) return ToPayload(p)->repeated_field.GetArena();
    return ToArena(p);
  }

  // Returns the reflection payload. Returns null if it does not exist yet.
  ReflectionPayload* maybe_payload() const {
    auto p = payload_.load(std::memory_order_acquire);
    return IsPayload(p) ? ToPayload(p) : nullptr;
  }
  // Returns the reflection payload, and constructs one if does not exist yet.
  ReflectionPayload& payload() const {
    auto* p = maybe_payload();
    return p != nullptr ? *p : PayloadSlow();
  }
  ReflectionPayload& PayloadSlow() const;

  State state() const {
    auto* p = maybe_payload();
    return p != nullptr ? p->state.load(std::memory_order_acquire)
                        // The default
                        : STATE_MODIFIED_MAP;
  }

  static const UntypedMapBase& GetMapImpl(const MapFieldBaseForParse& map,
                                          bool is_mutable);

 private:
  friend class ContendedMapCleanTest;
  friend class GeneratedMessageReflection;
  friend class MapFieldAccessor;
  friend class google::protobuf::Reflection;
  friend class google::protobuf::DynamicMessage;

  // See assertion in TypeDefinedMapFieldBase::TypeDefinedMapFieldBase()
  const UntypedMapBase& GetMapRaw() const {
    return *reinterpret_cast<const UntypedMapBase*>(this + 1);
  }
  UntypedMapBase& GetMapRaw() {
    return *reinterpret_cast<UntypedMapBase*>(this + 1);
  }

  // Virtual helper methods for MapIterator. MapIterator doesn't have the
  // type helper for key and value. Call these help methods to deal with
  // different types. Real helper methods are implemented in
  // TypeDefinedMapFieldBase.
  friend class google::protobuf::MapIterator;

  // Copy the map<...>::iterator from other_iterator to
  // this_iterator.
  void CopyIterator(MapIterator* this_iter, const MapIterator& that_iter) const;

  // IncreaseIterator() is called by operator++() of MapIterator only.
  // It implements the ++ operator of MapIterator.
  void IncreaseIterator(MapIterator* map_iter) const;

  enum class TaggedPtr : uintptr_t {};
  static constexpr uintptr_t kHasPayloadBit = 1;

  static ReflectionPayload* ToPayload(TaggedPtr p) {
    ABSL_DCHECK(IsPayload(p));
    auto* res = reinterpret_cast<ReflectionPayload*>(static_cast<uintptr_t>(p) -
                                                     kHasPayloadBit);
    PROTOBUF_ASSUME(res != nullptr);
    return res;
  }
  static Arena* ToArena(TaggedPtr p) {
    ABSL_DCHECK(!IsPayload(p));
    return reinterpret_cast<Arena*>(p);
  }
  static TaggedPtr ToTaggedPtr(ReflectionPayload* p) {
    return static_cast<TaggedPtr>(reinterpret_cast<uintptr_t>(p) +
                                  kHasPayloadBit);
  }
  static TaggedPtr ToTaggedPtr(Arena* p) {
    return static_cast<TaggedPtr>(reinterpret_cast<uintptr_t>(p));
  }
  static bool IsPayload(TaggedPtr p) {
    return static_cast<uintptr_t>(p) & kHasPayloadBit;
  }

  mutable std::atomic<TaggedPtr> payload_{};
};

// This class provides common Map Reflection implementations for generated
// message and dynamic message.
template <typename Key, typename T>
class TypeDefinedMapFieldBase : public MapFieldBase {
 public:
  explicit constexpr TypeDefinedMapFieldBase(const VTable* vtable)
      : MapFieldBase(vtable), map_() {
    // This invariant is required by MapFieldBase to easily access the map
    // member without paying for dynamic dispatch. It reduces code size.
    static_assert(PROTOBUF_FIELD_OFFSET(TypeDefinedMapFieldBase, map_) ==
                      sizeof(MapFieldBase),
                  "");
  }
  TypeDefinedMapFieldBase(const TypeDefinedMapFieldBase&) = delete;
  TypeDefinedMapFieldBase& operator=(const TypeDefinedMapFieldBase&) = delete;

  TypeDefinedMapFieldBase(const VTable* vtable, Arena* arena)
      : MapFieldBase(vtable, arena), map_(arena) {}

 protected:
  ~TypeDefinedMapFieldBase() { map_.~Map(); }

  // Not all overrides are marked `final` here because DynamicMapField overrides
  // them. DynamicMapField does extra memory management for the elements and
  // needs to override the functions that create or destroy elements.

 public:
  const Map<Key, T>& GetMap() const {
    SyncMapWithRepeatedField();
    return map_;
  }

  Map<Key, T>* MutableMap() {
    SyncMapWithRepeatedField();
    SetMapDirty();
    return &map_;
  }

  static void ClearMapNoSyncImpl(MapFieldBase& map) {
    static_cast<TypeDefinedMapFieldBase&>(map).map_.clear();
  }

  void InternalSwap(TypeDefinedMapFieldBase* other);

 protected:
  friend struct MapFieldTestPeer;

  using Iter = typename Map<Key, T>::const_iterator;

  static bool DeleteMapValueImpl(MapFieldBase& map, const MapKey& map_key);
  static bool LookupMapValueImpl(const MapFieldBase& self,
                                 const MapKey& map_key, MapValueConstRef* val);
  static void SetMapIteratorValueImpl(MapIterator* map_iter);
  static bool InsertOrLookupMapValueNoSyncImpl(MapFieldBase& map,
                                               const MapKey& map_key,
                                               MapValueRef* val);

  static void MergeFromImpl(MapFieldBase& base, const MapFieldBase& other);
  static void SwapImpl(MapFieldBase& lhs, MapFieldBase& rhs);
  static void UnsafeShallowSwapImpl(MapFieldBase& lhs, MapFieldBase& rhs);

  static size_t SpaceUsedExcludingSelfNoLockImpl(const MapFieldBase& map);

  // map_ is inside an anonymous union so we can explicitly control its
  // destruction
  union {
    Map<Key, T> map_;
  };
};

// This class provides access to map field using generated api. It is used for
// internal generated message implementation only. Users should never use this
// directly.
template <typename Derived, typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType_,
          WireFormatLite::FieldType kValueFieldType_>
class MapField final : public TypeDefinedMapFieldBase<Key, T> {
  // Provide utilities to parse/serialize key/value.  Provide utilities to
  // manipulate internal stored type.
  typedef MapTypeHandler<kKeyFieldType_, Key> KeyTypeHandler;
  typedef MapTypeHandler<kValueFieldType_, T> ValueTypeHandler;

  // Define message type for internal repeated field.
  typedef Derived EntryType;

 public:
  typedef Map<Key, T> MapType;
  static constexpr WireFormatLite::FieldType kKeyFieldType = kKeyFieldType_;
  static constexpr WireFormatLite::FieldType kValueFieldType = kValueFieldType_;

  constexpr MapField() : MapField::TypeDefinedMapFieldBase(&kVTable) {}
  MapField(const MapField&) = delete;
  MapField& operator=(const MapField&) = delete;
  ~MapField() {}

  explicit MapField(Arena* arena)
      : TypeDefinedMapFieldBase<Key, T>(&kVTable, arena) {}
  MapField(ArenaInitialized, Arena* arena) : MapField(arena) {}
  MapField(InternalVisibility, Arena* arena) : MapField(arena) {}
  MapField(InternalVisibility, Arena* arena, const MapField& from)
      : MapField(arena) {
    this->MergeFromImpl(*this, from);
  }

  // Used in the implementation of parsing. Caller should take the ownership iff
  // arena_ is nullptr.
  EntryType* NewEntry() const {
    return Arena::CreateMessage<EntryType>(this->arena());
  }

 private:
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;

  static const Message* GetPrototypeImpl(const MapFieldBase& map);

  static const MapFieldBase::VTable kVTable;

  friend class google::protobuf::Arena;
  friend class MapFieldBase;
  friend class MapFieldStateTest;  // For testing, it needs raw access to impl_
};

template <typename Derived, typename Key, typename T,
          WireFormatLite::FieldType kKeyFieldType_,
          WireFormatLite::FieldType kValueFieldType_>
PROTOBUF_CONSTINIT const MapFieldBase::VTable
    MapField<Derived, Key, T, kKeyFieldType_, kValueFieldType_>::kVTable =
        MapField::template MakeVTable<MapField>();

template <typename Key, typename T>
bool AllAreInitialized(const TypeDefinedMapFieldBase<Key, T>& field) {
  for (const auto& p : field.GetMap()) {
    if (!p.second.IsInitialized()) return false;
  }
  return true;
}

template <typename T, typename Key, typename Value,
          WireFormatLite::FieldType kKeyFieldType,
          WireFormatLite::FieldType kValueFieldType>
struct MapEntryToMapField<
    MapEntry<T, Key, Value, kKeyFieldType, kValueFieldType>> {
  typedef MapField<T, Key, Value, kKeyFieldType, kValueFieldType> MapFieldType;
};

class PROTOBUF_EXPORT DynamicMapField final
    : public TypeDefinedMapFieldBase<MapKey, MapValueRef> {
 public:
  explicit DynamicMapField(const Message* default_entry);
  DynamicMapField(const Message* default_entry, Arena* arena);
  DynamicMapField(const DynamicMapField&) = delete;
  DynamicMapField& operator=(const DynamicMapField&) = delete;
  ~DynamicMapField();

 private:
  friend class MapFieldBase;

  const Message* default_entry_;

  static const VTable kVTable;

  void AllocateMapValue(MapValueRef* map_val);

  static void MergeFromImpl(MapFieldBase& base, const MapFieldBase& other);
  static bool InsertOrLookupMapValueNoSyncImpl(MapFieldBase& base,
                                               const MapKey& map_key,
                                               MapValueRef* val);
  static void ClearMapNoSyncImpl(MapFieldBase& base);

  static void UnsafeShallowSwapImpl(MapFieldBase& lhs, MapFieldBase& rhs) {
    static_cast<DynamicMapField&>(lhs).Swap(
        static_cast<DynamicMapField*>(&rhs));
  }

  static size_t SpaceUsedExcludingSelfNoLockImpl(const MapFieldBase& map);

  static const Message* GetPrototypeImpl(const MapFieldBase& map);
};

}  // namespace internal

// MapValueConstRef points to a map value. Users can NOT modify
// the map value.
class PROTOBUF_EXPORT MapValueConstRef {
 public:
  MapValueConstRef() : data_(nullptr), type_() {}

  int64_t GetInt64Value() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_INT64,
               "MapValueConstRef::GetInt64Value");
    return *reinterpret_cast<int64_t*>(data_);
  }
  uint64_t GetUInt64Value() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_UINT64,
               "MapValueConstRef::GetUInt64Value");
    return *reinterpret_cast<uint64_t*>(data_);
  }
  int32_t GetInt32Value() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_INT32,
               "MapValueConstRef::GetInt32Value");
    return *reinterpret_cast<int32_t*>(data_);
  }
  uint32_t GetUInt32Value() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_UINT32,
               "MapValueConstRef::GetUInt32Value");
    return *reinterpret_cast<uint32_t*>(data_);
  }
  bool GetBoolValue() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_BOOL, "MapValueConstRef::GetBoolValue");
    return *reinterpret_cast<bool*>(data_);
  }
  int GetEnumValue() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_ENUM, "MapValueConstRef::GetEnumValue");
    return *reinterpret_cast<int*>(data_);
  }
  const std::string& GetStringValue() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_STRING,
               "MapValueConstRef::GetStringValue");
    return *reinterpret_cast<std::string*>(data_);
  }
  float GetFloatValue() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_FLOAT,
               "MapValueConstRef::GetFloatValue");
    return *reinterpret_cast<float*>(data_);
  }
  double GetDoubleValue() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_DOUBLE,
               "MapValueConstRef::GetDoubleValue");
    return *reinterpret_cast<double*>(data_);
  }

  const Message& GetMessageValue() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_MESSAGE,
               "MapValueConstRef::GetMessageValue");
    return *reinterpret_cast<Message*>(data_);
  }

 protected:
  // data_ point to a map value. MapValueConstRef does not
  // own this value.
  void* data_;
  // type_ is 0 or a valid FieldDescriptor::CppType.
  // Use "CppType()" to indicate zero.
  FieldDescriptor::CppType type_;

  FieldDescriptor::CppType type() const {
    if (type_ == FieldDescriptor::CppType() || data_ == nullptr) {
      ABSL_LOG(FATAL)
          << "Protocol Buffer map usage error:\n"
          << "MapValueConstRef::type MapValueConstRef is not initialized.";
    }
    return type_;
  }

 private:
  template <typename Derived, typename K, typename V,
            internal::WireFormatLite::FieldType key_wire_type,
            internal::WireFormatLite::FieldType value_wire_type>
  friend class internal::MapField;
  template <typename K, typename V>
  friend class internal::TypeDefinedMapFieldBase;
  friend class google::protobuf::MapIterator;
  friend class Reflection;
  friend class internal::DynamicMapField;
  friend class internal::MapFieldBase;

  void SetValueOrCopy(const void* val) { SetValue(val); }
  void SetValueOrCopy(const MapValueConstRef* val) { CopyFrom(*val); }

  void SetType(FieldDescriptor::CppType type) { type_ = type; }
  void SetValue(const void* val) { data_ = const_cast<void*>(val); }
  void CopyFrom(const MapValueConstRef& other) {
    type_ = other.type_;
    data_ = other.data_;
  }
};

// MapValueRef points to a map value. Users are able to modify
// the map value.
class PROTOBUF_EXPORT MapValueRef final : public MapValueConstRef {
 public:
  MapValueRef() {}

  void SetInt64Value(int64_t value) {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_INT64, "MapValueRef::SetInt64Value");
    *reinterpret_cast<int64_t*>(data_) = value;
  }
  void SetUInt64Value(uint64_t value) {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_UINT64, "MapValueRef::SetUInt64Value");
    *reinterpret_cast<uint64_t*>(data_) = value;
  }
  void SetInt32Value(int32_t value) {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_INT32, "MapValueRef::SetInt32Value");
    *reinterpret_cast<int32_t*>(data_) = value;
  }
  void SetUInt32Value(uint32_t value) {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_UINT32, "MapValueRef::SetUInt32Value");
    *reinterpret_cast<uint32_t*>(data_) = value;
  }
  void SetBoolValue(bool value) {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_BOOL, "MapValueRef::SetBoolValue");
    *reinterpret_cast<bool*>(data_) = value;
  }
  // TODO - Checks that enum is member.
  void SetEnumValue(int value) {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_ENUM, "MapValueRef::SetEnumValue");
    *reinterpret_cast<int*>(data_) = value;
  }
  void SetStringValue(const std::string& value) {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_STRING, "MapValueRef::SetStringValue");
    *reinterpret_cast<std::string*>(data_) = value;
  }
  void SetFloatValue(float value) {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_FLOAT, "MapValueRef::SetFloatValue");
    *reinterpret_cast<float*>(data_) = value;
  }
  void SetDoubleValue(double value) {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_DOUBLE, "MapValueRef::SetDoubleValue");
    *reinterpret_cast<double*>(data_) = value;
  }

  Message* MutableMessageValue() {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_MESSAGE,
               "MapValueRef::MutableMessageValue");
    return reinterpret_cast<Message*>(data_);
  }

 private:
  friend class internal::DynamicMapField;

  // Only used in DynamicMapField
  void DeleteData() {
    switch (type_) {
#define HANDLE_TYPE(CPPTYPE, TYPE)           \
  case FieldDescriptor::CPPTYPE_##CPPTYPE: { \
    delete reinterpret_cast<TYPE*>(data_);   \
    break;                                   \
  }
      HANDLE_TYPE(INT32, int32_t);
      HANDLE_TYPE(INT64, int64_t);
      HANDLE_TYPE(UINT32, uint32_t);
      HANDLE_TYPE(UINT64, uint64_t);
      HANDLE_TYPE(DOUBLE, double);
      HANDLE_TYPE(FLOAT, float);
      HANDLE_TYPE(BOOL, bool);
      HANDLE_TYPE(STRING, std::string);
      HANDLE_TYPE(ENUM, int32_t);
      HANDLE_TYPE(MESSAGE, Message);
#undef HANDLE_TYPE
    }
  }
};

#undef TYPE_CHECK

class PROTOBUF_EXPORT MapIterator {
 public:
  MapIterator(Message* message, const FieldDescriptor* field) {
    const Reflection* reflection = message->GetReflection();
    map_ = reflection->MutableMapData(message, field);
    key_.SetType(field->message_type()->map_key()->cpp_type());
    value_.SetType(field->message_type()->map_value()->cpp_type());
  }
  MapIterator(const MapIterator& other) { *this = other; }
  MapIterator& operator=(const MapIterator& other) {
    map_ = other.map_;
    map_->CopyIterator(this, other);
    return *this;
  }
  friend bool operator==(const MapIterator& a, const MapIterator& b) {
    return a.map_->EqualIterator(a, b);
  }
  friend bool operator!=(const MapIterator& a, const MapIterator& b) {
    return !a.map_->EqualIterator(a, b);
  }
  MapIterator& operator++() {
    map_->IncreaseIterator(this);
    return *this;
  }
  MapIterator operator++(int) {
    // iter_ is copied from Map<...>::iterator, no need to
    // copy from its self again. Use the same implementation
    // with operator++()
    map_->IncreaseIterator(this);
    return *this;
  }
  const MapKey& GetKey() { return key_; }
  const MapValueRef& GetValueRef() { return value_; }
  MapValueRef* MutableValueRef() {
    map_->SetMapDirty();
    return &value_;
  }

 private:
  template <typename Key, typename T>
  friend class internal::TypeDefinedMapFieldBase;
  friend class internal::DynamicMapField;
  template <typename Derived, typename Key, typename T,
            internal::WireFormatLite::FieldType kKeyFieldType,
            internal::WireFormatLite::FieldType kValueFieldType>
  friend class internal::MapField;
  friend class internal::MapFieldBase;

  MapIterator(internal::MapFieldBase* map, const Descriptor* descriptor) {
    map_ = map;
    key_.SetType(descriptor->map_key()->cpp_type());
    value_.SetType(descriptor->map_value()->cpp_type());
  }

  internal::UntypedMapIterator iter_;
  // Point to a MapField to call helper methods implemented in MapField.
  // MapIterator does not own this object.
  internal::MapFieldBase* map_;
  MapKey key_;
  MapValueRef value_;
};

namespace internal {
template <>
struct is_internal_map_value_type<class MapValueConstRef> : std::true_type {};
template <>
struct is_internal_map_value_type<class MapValueRef> : std::true_type {};
}  // namespace internal

}  // namespace protobuf
}  // namespace google

#ifdef _MSC_VER
#pragma warning(pop)  // restore warning C4265
#endif                // _MSC_VER

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_MAP_FIELD_H__
