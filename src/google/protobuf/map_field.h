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

namespace internal {
class MapConstIterator;
class MapConstIteratorEntry;
class MapIterator;
class MapIteratorEntry;
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
  friend internal::MapConstIterator;
  friend internal::MapConstIteratorEntry;

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
  explicit constexpr MapFieldBase(const void* globals_as_void)
      : MapFieldBaseForParse(globals_as_void) {}
  explicit MapFieldBase(const Message* prototype)
      : MapFieldBaseForParse(prototype) {}
  MapFieldBase(const MapFieldBase&) = delete;
  MapFieldBase& operator=(const MapFieldBase&) = delete;

 protected:
  // "protected" stops users from deleting a `MapFieldBase *`
  ~MapFieldBase();

 public:
  // Same as the base class, but without the dynamic dispatch.
  const UntypedMapBase& GetMap() const {
    SyncMapWithRepeatedField();
    return GetMapRaw();
  }
  UntypedMapBase* MutableMap() {
    SyncMapWithRepeatedField();
    SetMapDirty();
    return &GetMapRaw();
  }

  static const MapFieldBase* From(const UntypedMapBase* m) {
    return reinterpret_cast<const MapFieldBase*>(
        reinterpret_cast<const char*>(m) - MapOffset());
  }

  // Returns reference to internal repeated field. Data written using
  // Map's api prior to calling this function is guarantted to be
  // included in repeated field.
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD const RepeatedPtrFieldBase&
  GetRepeatedField() const;

  // Like above. Returns mutable pointer to the internal repeated field.
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD RepeatedPtrFieldBase*
  MutableRepeatedField();

  // Returns whether changes to the map are reflected in the repeated field.
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD bool IsRepeatedFieldValid() const;
  // Insures operations after won't get executed before calling this.
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD bool IsMapValid() const;
  void MergeFrom(Arena* arena, const MapFieldBase& other);
  void Swap(Arena* arena, MapFieldBase* other, Arena* other_arena);
  void InternalSwap(MapFieldBase* other);
  void Clear();

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
    auto p = globals_or_payload_.load(std::memory_order_acquire);
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
  friend GenericMapRef;

  friend class google::protobuf::MapValueRef;

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
                          InternalMetadataOffset offset, Arena* arena,
                          const TypeDefinedMapFieldBase& from)
      : MapFieldBase(prototype),
        map_(offset
                 .TranslateForMember<offsetof(TypeDefinedMapFieldBase, map_)>(),
             arena, from.GetMap()) {}

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
template <auto* kGlobals, typename Key, typename T>
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
  MapField(InternalVisibility, InternalMetadataOffset offset, Arena* arena,
           const MapField& from)
      : TypeDefinedMapFieldBase<Key, T>(
            MessageGlobalsBase::ToDefaultInstance<Message>(kGlobals), offset,
            arena, from) {}

 private:
  explicit constexpr MapField(InternalMetadataOffset offset)
      : MapField::TypeDefinedMapFieldBase(kGlobals, offset) {}

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

template <auto* kGlobals, typename Key, typename T>
using MapFieldWithArena = FieldWithArena<MapField<kGlobals, Key, T>>;

// We don't use `auto* globals` here because GCC fails to deduce the
// specialization.
template <typename Globals, const Globals* kGlobals, typename Key, typename T>
struct FieldArenaRep<MapField<kGlobals, Key, T>> {
  using Type = MapFieldWithArena<kGlobals, Key, T>;

  static MapField<kGlobals, Key, T>* Get(Type* arena_rep) {
    return &arena_rep->field();
  }
};

template <typename Globals, const Globals* kGlobals, typename Key, typename T>
struct FieldArenaRep<const MapField<kGlobals, Key, T>> {
  using Type = const MapFieldWithArena<kGlobals, Key, T>;

  static const MapField<kGlobals, Key, T>* Get(Type* arena_rep) {
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
  friend class Reflection;
  friend class internal::MapFieldBase;
  friend internal::MapConstIterator;
  friend internal::MapConstIteratorEntry;
  friend internal::MapIterator;
  friend internal::MapIteratorEntry;

  void SetType(FieldDescriptor::CppType type) { type_ = type; }
  void SetValue(const void* val) {
    ABSL_DCHECK_NE(static_cast<int>(type_), 0);
    data_ = const_cast<void*>(val);
  }
  void CopyFrom(const MapValueConstRef& other) {
    type_ = other.type_;
    data_ = other.data_;
  }

  // data_ point to a map value. MapValueConstRef does not
  // own this value.
  void* data_;
  // type_ is 0 or a valid FieldDescriptor::CppType.
  // Use "CppType()" to indicate zero.
  FieldDescriptor::CppType type_{};
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

 private:
  friend internal::MapFieldBase;
  friend internal::MapIterator;
  friend internal::MapIteratorEntry;
};

#undef TYPE_CHECK

namespace internal {

class MapIteratorEntry final {
 public:
  using key_type = MapKey;
  using mapped_type = MapValueRef;

  MapIteratorEntry() = default;
  MapIteratorEntry(const MapIteratorEntry&) = default;
  MapIteratorEntry& operator=(const MapIteratorEntry&) = default;

  MapKey key() const;
  MapValueRef value() const;

 private:
  friend MapIterator;
  friend MapConstIteratorEntry;

  uint8_t key_type_;
  uint8_t value_type_;
  internal::UntypedMapIterator iter_;
};

class MapIterator final {
 public:
  using value_type = MapIteratorEntry;
  using key_type = value_type::key_type;
  using mapped_type = value_type::mapped_type;

  MapIterator() = default;
  MapIterator(const MapIterator&) = default;
  MapIterator& operator=(const MapIterator&) = default;

  value_type operator*() const {
    ABSL_CHECK(!iter_.Equals(internal::UntypedMapBase::EndIterator()))
        << "Can't deref the end iterator";

    value_type res;
    res.key_type_ = key_type_;
    res.value_type_ = value_type_;
    res.iter_ = iter_;
    return res;
  }

  auto operator->() const {
    struct ArrowProxy {
      value_type value;
      const value_type* operator->() const { return &value; }
    };
    return ArrowProxy{**this};
  }

  MapIterator& operator++() {
    iter_.PlusPlus();
    return *this;
  }
  MapIterator operator++(int) {
    auto copy = *this;
    iter_.PlusPlus();
    return copy;
  }

  friend bool operator==(MapIterator a, MapIterator b) {
    return a.iter_.Equals(b.iter_);
  }
  friend bool operator!=(MapIterator a, MapIterator b) { return !(a == b); }

 private:
  friend GenericMapRef;
  friend MapConstIterator;

  uint8_t key_type_{};
  uint8_t value_type_{};
  internal::UntypedMapIterator iter_{};
};

class MapConstIteratorEntry final {
 public:
  using key_type = MapKey;
  using mapped_type = MapValueConstRef;

  MapConstIteratorEntry() = default;
  MapConstIteratorEntry(const MapConstIteratorEntry&) = default;
  MapConstIteratorEntry& operator=(const MapConstIteratorEntry&) = default;
  MapConstIteratorEntry(const MapIteratorEntry& entry)
      : key_type_(entry.key_type_),
        value_type_(entry.value_type_),
        iter_(entry.iter_) {}

  MapKey key() const;
  MapValueConstRef value() const;

 private:
  friend MapConstIterator;

  uint8_t key_type_{};
  uint8_t value_type_{};
  internal::UntypedMapIterator iter_{};
};

class MapConstIterator final {
 public:
  using value_type = MapConstIteratorEntry;
  using key_type = value_type::key_type;
  using mapped_type = value_type::mapped_type;

  MapConstIterator() = default;
  MapConstIterator(const MapConstIterator&) = default;
  MapConstIterator& operator=(const MapConstIterator&) = default;
  MapConstIterator(MapIterator it)
      : key_type_(it.key_type_), value_type_(it.value_type_), iter_(it.iter_) {}

  value_type operator*() const {
    ABSL_CHECK(!iter_.Equals(internal::UntypedMapBase::EndIterator()))
        << "Can't deref the end iterator";

    value_type res;
    res.key_type_ = key_type_;
    res.value_type_ = value_type_;
    res.iter_ = iter_;
    return res;
  }

  auto operator->() const {
    struct ArrowProxy {
      value_type value;
      const value_type* operator->() const { return &value; }
    };
    return ArrowProxy{**this};
  }

  MapConstIterator& operator++() {
    iter_.PlusPlus();
    return *this;
  }
  MapConstIterator operator++(int) {
    auto copy = *this;
    iter_.PlusPlus();
    return copy;
  }

  friend bool operator==(MapConstIterator a, MapConstIterator b) {
    return a.iter_.Equals(b.iter_);
  }
  friend bool operator!=(MapConstIterator a, MapConstIterator b) {
    return !(a == b);
  }

 private:
  friend GenericConstMapRef;
  friend GenericMapRef;
  friend internal::MapFieldBase;

  uint8_t key_type_;
  uint8_t value_type_;
  internal::UntypedMapIterator iter_;
};

}  // namespace internal

class PROTOBUF_EXPORT GenericMapRef final {
 public:
  using iterator = internal::MapIterator;
  using value_type = iterator::value_type;
  using key_type = iterator::key_type;
  using mapped_type = iterator::mapped_type;

  GenericMapRef() = default;
  GenericMapRef(const GenericMapRef&) = default;

  // Deleted to prevent confusion as to whether the assignment is shallow or
  // deep.
  GenericMapRef& operator=(const GenericMapRef&) = delete;

  iterator begin() const;
  iterator end() const;
  size_t size() const;
  bool empty() const;
  bool contains(const MapKey& key) const;
  iterator find(const MapKey& key) const;
  mapped_type at(const MapKey& key) const;

  void clear() const;
  bool erase(const MapKey& key) const;
  bool erase(iterator it) const;

  // These perform the operation on the underlying instances.
  // They do not alter the handles themselves.
  // REQUIRES: Both GenericMapRef point to the same static Map<> type.
  // Otherwise, behavior is undefined and might not be diagnosed.
  void assign(const GenericMapRef& other) const;
  void merge(const GenericMapRef& other) const;
  void swap(const GenericMapRef& other) const;

  // These perform the operation on the handle itself.
  // They do not modify the underlying containers.
  // REQUIRES: Both GenericMapRef point to the same static Map<> type.
  // Otherwise, behavior is undefined and might not be diagnosed.
  void shallow_assign(const GenericMapRef& other);
  void shallow_swap(GenericMapRef&);

  std::pair<iterator, bool> try_emplace(const MapKey& key) const;
  mapped_type operator[](const MapKey& key) const {
    return try_emplace(key).first->value();
  }

 private:
  friend Reflection;
  friend internal::MapFieldBase;
  friend GenericConstMapRef;

  template <typename T, typename... U>
  void InitializeKeyValue(T* v, const U&... init) const {
    ::new (static_cast<void*>(v)) T(init...);
    if constexpr (std::is_same_v<std::string, T>) {
      if (auto* a = map_->arena()) {
        a->OwnDestructor(v);
      }
    }
  }

  void InitializeKeyValue(MessageLite* msg) const {
    value_class_data_->PlacementNew(msg, map_->arena());
  }

  uint8_t key_type_;
  uint8_t value_type_;
  internal::UntypedMapBase* map_ = nullptr;
  const internal::ClassData* value_class_data_ = nullptr;
};

class PROTOBUF_EXPORT GenericConstMapRef final {
 public:
  using iterator = internal::MapConstIterator;
  using const_iterator = iterator;
  using value_type = iterator::value_type;
  using key_type = iterator::key_type;
  using mapped_type = iterator::mapped_type;

  GenericConstMapRef() = default;
  GenericConstMapRef(const GenericConstMapRef&) = default;
  GenericConstMapRef& operator=(const GenericConstMapRef&) = default;
  GenericConstMapRef(const GenericMapRef& mut)
      : key_type_(mut.key_type_),
        value_type_(mut.value_type_),
        map_(mut.map_) {}

  iterator begin() const;
  iterator end() const;
  size_t size() const;
  bool empty() const;
  bool contains(const MapKey& key) const;
  iterator find(const MapKey& key) const;
  mapped_type at(const MapKey& key) const;

 private:
  friend Reflection;

  uint8_t key_type_;
  uint8_t value_type_;
  const internal::UntypedMapBase* map_ = nullptr;
};

// This class was never part of any public API (even though it was mistakenly
// put in the public namespace).
// We are replacing the old internal map reflection API with the new public one
// and this class is going away. It is here for now to support the migration of
// a friend project that is using the internal API.
class [[deprecated(
    "Legacy internal class. Use Reflection::GetMap instead.")]] PROTOBUF_EXPORT
    ConstMapIterator final {
 public:
  MapKey GetKey() const { return it_->key(); }
  MapValueConstRef GetValueRef() const { return it_->value(); }

  ConstMapIterator& operator++() {
    ++it_;
    return *this;
  }

  friend bool operator==(ConstMapIterator a, ConstMapIterator b) {
    return a.it_ == b.it_;
  }
  friend bool operator!=(ConstMapIterator a, ConstMapIterator b) {
    return a.it_ != b.it_;
  }

 private:
  friend Reflection;
  ConstMapIterator(internal::MapConstIterator it) : it_(it) {}
  internal::MapConstIterator it_;
};

// Expose this feature macro to users to let them know that the map reflection
// API exists.
#define PROTOBUF_HAS_MAP_REFLECTION_APIS 1

}  // namespace protobuf
}  // namespace google

#ifdef _MSC_VER
#pragma warning(pop)  // restore warning C4265
#endif                // _MSC_VER

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_MAP_FIELD_H__
