// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_MAP_FIELD_H__
#define GOOGLE_PROTOBUF_MAP_FIELD_H__

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/field_with_arena.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/map.h"
#include "google/protobuf/map_type_handler.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/unknown_field_set.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {
class DynamicMessage;
template <bool>
class MapIteratorBase;
class ConstMapIterator;
class MapIterator;

namespace internal {
class MapFieldBase;
}

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

// MapKey is an union type for representing any possible map key. For strings,
// map key does not own the underlying data. It is up to the caller to ensure
// any supplied strings outlive any instance of this class.
class PROTOBUF_EXPORT MapKey {
 public:
  MapKey() = default;
  MapKey(const MapKey&) = default;
  MapKey& operator=(const MapKey&) = default;

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
    val_.int64_value = value;
  }
  void SetUInt64Value(uint64_t value) {
    SetType(FieldDescriptor::CPPTYPE_UINT64);
    val_.uint64_value = value;
  }
  void SetInt32Value(int32_t value) {
    SetType(FieldDescriptor::CPPTYPE_INT32);
    val_.int32_value = value;
  }
  void SetUInt32Value(uint32_t value) {
    SetType(FieldDescriptor::CPPTYPE_UINT32);
    val_.uint32_value = value;
  }
  void SetBoolValue(bool value) {
    SetType(FieldDescriptor::CPPTYPE_BOOL);
    val_.bool_value = value;
  }
  void SetStringValue(absl::string_view val) {
    SetType(FieldDescriptor::CPPTYPE_STRING);
    val_.string_value = val;
  }

  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD int64_t GetInt64Value() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_INT64, "MapKey::GetInt64Value");
    return val_.int64_value;
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD uint64_t GetUInt64Value() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_UINT64, "MapKey::GetUInt64Value");
    return val_.uint64_value;
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD int32_t GetInt32Value() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_INT32, "MapKey::GetInt32Value");
    return val_.int32_value;
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD uint32_t GetUInt32Value() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_UINT32, "MapKey::GetUInt32Value");
    return val_.uint32_value;
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD bool GetBoolValue() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_BOOL, "MapKey::GetBoolValue");
    return val_.bool_value;
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD absl::string_view GetStringValue() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_STRING, "MapKey::GetStringValue");
    return val_.string_value;
  }

  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD bool operator<(
      const MapKey& other) const {
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
        return val_.string_value < other.val_.string_value;
      case FieldDescriptor::CPPTYPE_INT64:
        return val_.int64_value < other.val_.int64_value;
      case FieldDescriptor::CPPTYPE_INT32:
        return val_.int32_value < other.val_.int32_value;
      case FieldDescriptor::CPPTYPE_UINT64:
        return val_.uint64_value < other.val_.uint64_value;
      case FieldDescriptor::CPPTYPE_UINT32:
        return val_.uint32_value < other.val_.uint32_value;
      case FieldDescriptor::CPPTYPE_BOOL:
        return val_.bool_value < other.val_.bool_value;
    }
    return false;
  }

  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD bool operator==(
      const MapKey& other) const {
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
        return val_.string_value == other.val_.string_value;
      case FieldDescriptor::CPPTYPE_INT64:
        return val_.int64_value == other.val_.int64_value;
      case FieldDescriptor::CPPTYPE_INT32:
        return val_.int32_value == other.val_.int32_value;
      case FieldDescriptor::CPPTYPE_UINT64:
        return val_.uint64_value == other.val_.uint64_value;
      case FieldDescriptor::CPPTYPE_UINT32:
        return val_.uint32_value == other.val_.uint32_value;
      case FieldDescriptor::CPPTYPE_BOOL:
        return val_.bool_value == other.val_.bool_value;
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
        val_.string_value = other.val_.string_value;
        break;
      case FieldDescriptor::CPPTYPE_INT64:
        val_.int64_value = other.val_.int64_value;
        break;
      case FieldDescriptor::CPPTYPE_INT32:
        val_.int32_value = other.val_.int32_value;
        break;
      case FieldDescriptor::CPPTYPE_UINT64:
        val_.uint64_value = other.val_.uint64_value;
        break;
      case FieldDescriptor::CPPTYPE_UINT32:
        val_.uint32_value = other.val_.uint32_value;
        break;
      case FieldDescriptor::CPPTYPE_BOOL:
        val_.bool_value = other.val_.bool_value;
        break;
    }
  }

 private:
  template <typename K, typename V>
  friend class internal::TypeDefinedMapFieldBase;
  friend class internal::MapFieldBase;
  template <bool>
  friend class MapIteratorBase;

  template <typename H>
  friend auto AbslHashValue(H state, const MapKey& key) {
    switch (key.type()) {
      case FieldDescriptor::CPPTYPE_STRING:
        return H::combine(std::move(state), key.GetStringValue());
      case FieldDescriptor::CPPTYPE_INT64:
        return H::combine(std::move(state), key.GetInt64Value());
      case FieldDescriptor::CPPTYPE_INT32:
        return H::combine(std::move(state), key.GetInt32Value());
      case FieldDescriptor::CPPTYPE_UINT64:
        return H::combine(std::move(state), key.GetUInt64Value());
      case FieldDescriptor::CPPTYPE_UINT32:
        return H::combine(std::move(state), key.GetUInt32Value());
      case FieldDescriptor::CPPTYPE_BOOL:
        return H::combine(std::move(state), key.GetBoolValue());
      case FieldDescriptor::CPPTYPE_DOUBLE:
      case FieldDescriptor::CPPTYPE_FLOAT:
      case FieldDescriptor::CPPTYPE_ENUM:
      case FieldDescriptor::CPPTYPE_MESSAGE:
      default:
        internal::Unreachable();
    }
  }

  union KeyValue {
    KeyValue() {}
    absl::string_view string_value;
    int64_t int64_value;
    int32_t int32_value;
    uint64_t uint64_value;
    uint32_t uint32_value;
    bool bool_value;
  } val_;

  void SetType(FieldDescriptor::CppType type) { type_ = type; }

  // type_ is 0 or a valid FieldDescriptor::CppType.
  // Use "CppType()" to indicate zero.
  FieldDescriptor::CppType type_ = FieldDescriptor::CppType();
};

namespace internal {

template <>
struct is_internal_map_key_type<MapKey> : std::true_type {};

}  // namespace internal

namespace internal {

class ContendedMapCleanTest;
class GeneratedMessageReflection;
class MapFieldAccessor;

template <typename MessageT>
struct MapDynamicFieldInfo;
struct MapFieldTestPeer;

// Return the prototype message for a Map entry.
// REQUIRES: `default_entry` is a map entry message.
// REQUIRES: mapped_type is of type message.
inline const Message& GetMapEntryValuePrototype(const Message& default_entry) {
  return default_entry.GetReflection()->GetMessage(
      default_entry, default_entry.GetDescriptor()->map_value());
}

// This class provides access to map field using reflection, which is the same
// as those provided for RepeatedPtrField<Message>. It is used for internal
// reflection implementation only. Users should never use this directly.
class PROTOBUF_EXPORT MapFieldBase : public MapFieldBaseForParse {
 public:
  explicit constexpr MapFieldBase(const void* prototype_as_void)
      : MapFieldBaseForParse(prototype_as_void) {}
  explicit MapFieldBase(const Message* prototype)
      : MapFieldBaseForParse(prototype) {}
  MapFieldBase(const MapFieldBase&) = delete;
  MapFieldBase& operator=(const MapFieldBase&) = delete;

 protected:
  // "protected" stops users from deleting a `MapFieldBase *`
  ~MapFieldBase();

 public:
  // Returns reference to internal repeated field. Data written using
  // Map's api prior to calling this function is guarantted to be
  // included in repeated field.
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD const RepeatedPtrFieldBase&
  GetRepeatedField() const;

  // Like above. Returns mutable pointer to the internal repeated field.
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD RepeatedPtrFieldBase*
  MutableRepeatedField();

  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD bool ContainsMapKey(
      const MapKey& map_key) const {
    return LookupMapValue(map_key, static_cast<MapValueConstRef*>(nullptr));
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD bool LookupMapValue(
      const MapKey& map_key, MapValueConstRef* val) const {
    SyncMapWithRepeatedField();
    return LookupMapValueNoSync(map_key, val);
  }
  bool LookupMapValue(const MapKey&, MapValueRef*) const = delete;

  bool InsertOrLookupMapValue(const MapKey& map_key, MapValueRef* val);

  // Returns whether changes to the map are reflected in the repeated field.
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD bool IsRepeatedFieldValid() const;
  // Insures operations after won't get executed before calling this.
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD bool IsMapValid() const;
  bool DeleteMapValue(Arena* arena, const MapKey& map_key);
  void MergeFrom(Arena* arena, const MapFieldBase& other);
  void Swap(Arena* arena, MapFieldBase* other, Arena* other_arena);
  void InternalSwap(MapFieldBase* other);
  // Sync Map with repeated field and returns the size of map.
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD int size() const;
  void Clear();
  template <bool kIsMutable>
  void SetMapIteratorValue(MapIteratorBase<kIsMutable>* map_iter) const;

  void MapBegin(MapIterator* map_iter) const;
  void MapEnd(MapIterator* map_iter) const;
  void ConstMapBegin(ConstMapIterator* map_iter) const;
  void ConstMapEnd(ConstMapIterator* map_iter) const;
  template <bool kIsMutable>
  bool EqualIterator(const MapIteratorBase<kIsMutable>& a,
                     const MapIteratorBase<kIsMutable>& b) const;

  // Returns the number of bytes used by the repeated field, excluding
  // sizeof(*this)
  size_t SpaceUsedExcludingSelfLong() const;

  int SpaceUsedExcludingSelf() const {
    return internal::ToIntSize(SpaceUsedExcludingSelfLong());
  }

 protected:
  const Message* GetPrototype() const;
  void ClearMapNoSync();

  // Synchronizes the content in Map to RepeatedPtrField if there is any change
  // to Map after last synchronization.
  const RepeatedPtrFieldBase& SyncRepeatedFieldWithMap(bool for_mutation) const;
  void SyncRepeatedFieldWithMapNoLock();

  // Synchronizes the content in RepeatedPtrField to Map if there is any change
  // to RepeatedPtrField after last synchronization.
  void SyncMapWithRepeatedField() const;
  void SyncMapWithRepeatedFieldNoLock();

  static void SwapPayload(MapFieldBase& lhs, MapFieldBase& rhs);

  // Tells MapFieldBase that there is new change to Map.
  void SetMapDirty() {
    MutableAccess();
    // These are called by (non-const) mutator functions. So by our API it's the
    // callers responsibility to have these calls properly ordered.
    if (auto* p = maybe_payload()) {
      // If we don't have a payload, it is already assumed `STATE_MODIFIED_MAP`.
      p->set_state_relaxed(STATE_MODIFIED_MAP);
    }
  }

  // Tells MapFieldBase that there is new change to RepeatedPtrField.
  void SetRepeatedDirty();

  // Provides derived class the access to repeated field.
  void* MutableRepeatedPtrField() const;

  bool InsertOrLookupMapValueNoSync(const MapKey& map_key, MapValueRef* val);

  // Support thread sanitizer (tsan) by making const / mutable races
  // more apparent.  If one thread calls MutableAccess() while another
  // thread calls either ConstAccess() or MutableAccess(), on the same
  // MapFieldBase-derived object, and there is no synchronization going
  // on between them, tsan will alert.
  void ConstAccess() const { GetMapRaw().ConstAccess(); }
  void MutableAccess() { GetMapRaw().MutableAccess(); }
  enum State {
    STATE_MODIFIED_MAP = 0,       // map has newly added data that has not been
                                  // synchronized to repeated field
    STATE_MODIFIED_REPEATED = 1,  // repeated field has newly added data that
                                  // has not been synchronized to map
    CLEAN = 2,                    // data in map and repeated field are same
  };

  class ReflectionPayload {
   public:
    explicit ReflectionPayload(Arena* arena, const Message* prototype)
        : repeated_field_(Arena::Create<RepeatedPtrField<Message>>(arena)),
          prototype_(prototype) {}
    ~ReflectionPayload() {
      if (repeated_field_->GetArena() == nullptr) {
        delete repeated_field_;
      }
    }

    RepeatedPtrField<Message>& repeated_field() { return *repeated_field_; }

    const Message* prototype() const { return prototype_; }

    absl::Mutex& mutex() { return mutex_; }

    State load_state_relaxed() const {
      return state_.load(std::memory_order_relaxed);
    }
    State load_state_acquire() const {
      return state_.load(std::memory_order_acquire);
    }
    void set_state_relaxed(State state) {
      state_.store(state, std::memory_order_relaxed);
    }
    void set_state_release(State state) {
      state_.store(state, std::memory_order_release);
    }

    void Swap(ReflectionPayload& other);

   private:
    RepeatedPtrField<Message>* repeated_field_;
    const Message* prototype_;
    absl::Mutex mutex_;  // The thread to synchronize map and repeated
                         // field needs to get lock first;
    std::atomic<State> state_{STATE_MODIFIED_MAP};
  };

  Arena* arena() const { return GetMapRaw().arena(); }

  // Returns the reflection payload. Returns null if it does not exist yet.
  ReflectionPayload* maybe_payload() const {
    auto p = prototype_or_payload_.load(std::memory_order_acquire);
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
    return p != nullptr ? p->load_state_acquire()
                        // The default
                        : STATE_MODIFIED_MAP;
  }

 private:
  friend class ContendedMapCleanTest;
  friend class GeneratedMessageReflection;
  friend class MapFieldAccessor;
  friend class google::protobuf::Reflection;
  friend class google::protobuf::DynamicMessage;

  template <typename T, typename... U>
  void InitializeKeyValue(T* v, const U&... init) {
    ::new (static_cast<void*>(v)) T(init...);
    if constexpr (std::is_same_v<std::string, T>) {
      if (arena() != nullptr) {
        arena()->OwnDestructor(v);
      }
    }
  }

  void InitializeKeyValue(MessageLite* msg) {
    GetClassData(GetMapEntryValuePrototype(*GetPrototype()))
        ->PlacementNew(msg, arena());
  }

  // Virtual helper methods for MapIterator. MapIterator doesn't have the
  // type helper for key and value. Call these help methods to deal with
  // different types. Real helper methods are implemented in
  // TypeDefinedMapFieldBase.
  template <bool>
  friend class google::protobuf::MapIteratorBase;
  friend class google::protobuf::MapIterator;

  // Copy the map<...>::iterator from other_iterator to
  // this_iterator.
  template <bool kIsMutable>
  void CopyIterator(MapIteratorBase<kIsMutable>* this_iter,
                    const MapIteratorBase<kIsMutable>& that_iter) const;

  // IncreaseIterator() is called by operator++() of MapIterator only.
  // It implements the ++ operator of MapIterator.
  template <bool kIsMutable>
  void IncreaseIterator(MapIteratorBase<kIsMutable>* map_iter) const;

  bool LookupMapValueNoSync(const MapKey& map_key, MapValueConstRef* val) const;
  static ReflectionPayload* ToPayload(const void* p) {
    ABSL_DCHECK(IsPayload(p));
    auto* res = reinterpret_cast<ReflectionPayload*>(
        reinterpret_cast<uintptr_t>(p) - kHasPayloadBit);
    PROTOBUF_ASSUME(res != nullptr);
    return res;
  }
  static const void* ToTaggedPtr(ReflectionPayload* p) {
    return reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(p) +
                                         kHasPayloadBit);
  }
};

// This class provides common Map Reflection implementations for generated
// message and dynamic message.
template <typename Key, typename T>
class TypeDefinedMapFieldBase : public MapFieldBase {
 public:
  explicit constexpr TypeDefinedMapFieldBase(const void* prototype_as_void,
                                             InternalMetadataOffset offset)
      : MapFieldBase(prototype_as_void),
        map_(offset.TranslateForMember<offsetof(TypeDefinedMapFieldBase,
                                                map_)>()) {
    // This invariant is required by `GetMapRaw` to easily access the map
    // member without paying for dynamic dispatch.
    static_assert(MapFieldBaseForParse::MapOffset() ==
                  PROTOBUF_FIELD_OFFSET(TypeDefinedMapFieldBase, map_));
  }
  TypeDefinedMapFieldBase(const TypeDefinedMapFieldBase&) = delete;
  TypeDefinedMapFieldBase& operator=(const TypeDefinedMapFieldBase&) = delete;

  TypeDefinedMapFieldBase(const Message* prototype,
                          InternalMetadataOffset offset)
      : MapFieldBase(prototype),
        map_(offset.TranslateForMember<offsetof(TypeDefinedMapFieldBase,
                                                map_)>()) {}

  TypeDefinedMapFieldBase(const Message* prototype,
                          InternalMetadataOffset offset,
                          const TypeDefinedMapFieldBase& from)
      : MapFieldBase(prototype),
        map_(offset
                 .TranslateForMember<offsetof(TypeDefinedMapFieldBase, map_)>(),
             from.GetMap()) {}

 protected:
  ~TypeDefinedMapFieldBase() { map_.~Map(); }

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

  // This overload is called from codegen, so we use templates for speed.
  // If there is no codegen (eg optimize_for=CODE_SIZE), then only the
  // reflection based one above will be used.
  void MergeFrom(const TypeDefinedMapFieldBase& other) {
    internal::MapMergeFrom(*MutableMap(), other.GetMap());
  }

 protected:
  friend struct MapFieldTestPeer;

  using Iter = typename Map<Key, T>::const_iterator;

  // map_ is inside an anonymous union so we can explicitly control its
  // destruction
  union {
    Map<Key, T> map_;
  };
};

// This class provides access to map field using generated api. It is used for
// internal generated message implementation only. Users should never use this
// directly.
template <typename Derived, typename Key, typename T>
class PROTOBUF_FUTURE_ADD_EARLY_WARN_UNUSED MapField final
    : public TypeDefinedMapFieldBase<Key, T> {
 public:
  typedef Map<Key, T> MapType;

  constexpr MapField() : MapField(InternalMetadataOffset()) {}
  MapField(const MapField&) = delete;
  MapField& operator=(const MapField&) = delete;
  ~MapField() = default;

  constexpr MapField(ArenaInitialized, InternalMetadataOffset offset)
      : MapField(offset) {}
  constexpr MapField(InternalVisibility, InternalMetadataOffset offset)
      : MapField(offset) {}
  MapField(InternalVisibility, InternalMetadataOffset offset,
           const MapField& from)
      : TypeDefinedMapFieldBase<Key, T>(
            static_cast<const Message*>(Derived::internal_default_instance()),
            offset, from) {}

 private:
  explicit constexpr MapField(InternalMetadataOffset offset)
      : MapField::TypeDefinedMapFieldBase(Derived::internal_default_instance(),
                                          offset) {}

  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;

  friend class google::protobuf::Arena;
  friend class google::protobuf::internal::FieldWithArena<MapField>;
  friend class MapFieldBase;
  friend class MapFieldStateTest;  // For testing, it needs raw access to impl_
};

template <typename Key, typename T>
bool AllAreInitialized(const TypeDefinedMapFieldBase<Key, T>& field) {
  for (const auto& p : field.GetMap()) {
    if (!p.second.IsInitialized()) return false;
  }
  return true;
}

template <typename Derived, typename Key, typename T>
using MapFieldWithArena = FieldWithArena<MapField<Derived, Key, T>>;

template <typename Derived, typename Key, typename T>
struct FieldArenaRep<MapField<Derived, Key, T>> {
  using Type = MapFieldWithArena<Derived, Key, T>;

  static inline MapField<Derived, Key, T>* Get(Type* arena_rep) {
    return &arena_rep->field();
  }
};

template <typename Derived, typename Key, typename T>
struct FieldArenaRep<const MapField<Derived, Key, T>> {
  using Type = const MapFieldWithArena<Derived, Key, T>;

  static inline const MapField<Derived, Key, T>* Get(Type* arena_rep) {
    return &arena_rep->field();
  }
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
  absl::string_view GetStringValue() const {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_STRING,
               "MapValueConstRef::GetStringValue");
    return absl::string_view(*reinterpret_cast<std::string*>(data_));
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

  FieldDescriptor::CppType type() const {
    if (type_ == FieldDescriptor::CppType() || data_ == nullptr) {
      ABSL_LOG(FATAL)
          << "Protocol Buffer map usage error:\n"
          << "MapValueConstRef::type MapValueConstRef is not initialized.";
    }
    return type_;
  }

 protected:
  // data_ point to a map value. MapValueConstRef does not
  // own this value.
  void* data_;
  // type_ is 0 or a valid FieldDescriptor::CppType.
  // Use "CppType()" to indicate zero.
  FieldDescriptor::CppType type_;

 private:
  template <typename Derived, typename K, typename V>
  friend class internal::MapField;
  template <typename K, typename V>
  friend class internal::TypeDefinedMapFieldBase;
  template <bool>
  friend class google::protobuf::MapIteratorBase;
  friend class Reflection;
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
  MapValueRef() = default;

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
  void SetStringValue(absl::string_view value) {
    TYPE_CHECK(FieldDescriptor::CPPTYPE_STRING, "MapValueRef::SetStringValue");
    reinterpret_cast<std::string*>(data_)->assign(value.data(), value.size());
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
};

#undef TYPE_CHECK

template <bool kIsMutable>
class PROTOBUF_EXPORT MapIteratorBase {
  using MessageT =
      std::conditional_t<kIsMutable, google::protobuf::Message, const google::protobuf::Message>;
  using MapFieldBase = std::conditional_t<kIsMutable, internal::MapFieldBase,
                                          const internal::MapFieldBase>;
  using ValueRef =
      std::conditional_t<kIsMutable, MapValueRef, MapValueConstRef>;
  using DerivedIterator =
      std::conditional_t<kIsMutable, MapIterator, ConstMapIterator>;

 public:
  MapIteratorBase(MessageT* message, const FieldDescriptor* field);
  MapIteratorBase(const MapIteratorBase& other) { *this = other; }

  MapIteratorBase& operator=(const MapIteratorBase& other);

  bool operator==(const MapIteratorBase& other) const;
  friend bool operator!=(const MapIteratorBase& a, const MapIteratorBase& b) {
    return !(a == b);
  }

  DerivedIterator& operator++();
  DerivedIterator operator++(int);

  const MapKey& GetKey() { return key_; }
  const ValueRef& GetValueRef() { return value_; }

 protected:
  template <typename Key, typename T>
  friend class internal::TypeDefinedMapFieldBase;
  template <typename Derived, typename Key, typename T>
  friend class internal::MapField;
  friend class internal::MapFieldBase;

  MapIteratorBase(MapFieldBase* map, const Descriptor* descriptor);

  internal::UntypedMapIterator iter_;
  // Point to a MapField to call helper methods implemented in MapField.
  // MapIterator does not own this object.
  MapFieldBase* map_;
  MapKey key_;
  ValueRef value_;
};

extern template class MapIteratorBase</*kIsMutable=*/false>;
extern template class MapIteratorBase</*kIsMutable=*/true>;

class PROTOBUF_EXPORT ConstMapIterator final
    : public MapIteratorBase</*kIsMutable=*/false> {
  friend class internal::MapFieldBase;
  template <typename MessageT>
  friend struct internal::MapDynamicFieldInfo;

 public:
  ConstMapIterator(const google::protobuf::Message* message, const FieldDescriptor* field)
      : MapIteratorBase(message, field) {}

 private:
  ConstMapIterator(const internal::MapFieldBase* map,
                   const Descriptor* descriptor)
      : MapIteratorBase(map, descriptor) {}
};

class PROTOBUF_EXPORT MapIterator final
    : public MapIteratorBase</*kIsMutable=*/true> {
  friend class internal::MapFieldBase;
  template <typename MessageT>
  friend struct internal::MapDynamicFieldInfo;

 public:
  MapIterator(google::protobuf::Message* message, const FieldDescriptor* field)
      : MapIteratorBase(message, field) {}

  MapValueRef* MutableValueRef() {
    map_->SetMapDirty();
    return &value_;
  }

 private:
  MapIterator(internal::MapFieldBase* map, const Descriptor* descriptor)
      : MapIteratorBase(map, descriptor) {}
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
