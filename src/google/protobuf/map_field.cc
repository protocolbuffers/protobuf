// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/map_field.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/base/optimization.h"
// #include "absl/base/throw_delegate.h"
#include "absl/functional/overload.h"
#include "absl/log/absl_check.h"
#include "absl/synchronization/mutex.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/map.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/message_traits.h"
#include "google/protobuf/port.h"
#include "google/protobuf/raw_ptr.h"
#include "google/protobuf/repeated_ptr_field.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

MapFieldBase::~MapFieldBase() { delete maybe_payload(); }

void MapFieldBase::MergeFrom(Arena* arena, const MapFieldBase& other) {
  MutableMap()->UntypedMergeFrom(arena, other.GetMap());
}

void MapFieldBase::Swap(Arena* arena, MapFieldBase* other, Arena* other_arena) {
  ABSL_DCHECK_EQ(arena, this->arena());
  ABSL_DCHECK_EQ(other_arena, other->arena());

  if (arena == other_arena) {
    InternalSwap(other);
    return;
  }
  MapFieldBase::SwapPayload(*this, *other);
  GetMapRaw().UntypedSwap(arena, other->GetMapRaw(), other_arena);
}

const Message* MapFieldBase::GetPrototype() const {
  const void* p = globals_or_payload_.load(std::memory_order_acquire);
  if (IsPayload(p)) {
    return ToPayload(p)->prototype();
  }
  return MessageGlobalsBase::ToDefaultInstance<Message>(p);
}

template <typename Map, typename F>
auto VisitMapKey(const MapKey& map_key, Map& map, F f) {
  switch (map_key.type()) {
#define HANDLE_TYPE(CPPTYPE, Type, KeyBaseType)                               \
  case FieldDescriptor::CPPTYPE_##CPPTYPE: {                                  \
    using KMB = KeyMapBase<KeyBaseType>;                                      \
    return f(                                                                 \
        static_cast<                                                          \
            std::conditional_t<std::is_const_v<Map>, const KMB&, KMB&>>(map), \
        TransparentSupport<KeyBaseType>::ToView(map_key.Get##Type##Value())); \
  }
    HANDLE_TYPE(INT32, Int32, uint32_t);
    HANDLE_TYPE(UINT32, UInt32, uint32_t);
    HANDLE_TYPE(INT64, Int64, uint64_t);
    HANDLE_TYPE(UINT64, UInt64, uint64_t);
    HANDLE_TYPE(BOOL, Bool, bool);
    HANDLE_TYPE(STRING, String, std::string);
#undef HANDLE_TYPE
    default:
      Unreachable();
  }
}

void MapFieldBase::ClearMapNoSync() {
  GetMapRaw().ClearTable(arena(), /*reset=*/true);
}

const RepeatedPtrFieldBase& MapFieldBase::GetRepeatedField() const {
  ConstAccess();
  return SyncRepeatedFieldWithMap(false);
}

RepeatedPtrFieldBase* MapFieldBase::MutableRepeatedField() {
  MutableAccess();
  auto& res = SyncRepeatedFieldWithMap(true);
  SetRepeatedDirty();
  return const_cast<RepeatedPtrFieldBase*>(&res);
}

template <typename T>
static void SwapRelaxed(std::atomic<T>& a, std::atomic<T>& b) {
  auto value_b = b.load(std::memory_order_relaxed);
  auto value_a = a.load(std::memory_order_relaxed);
  b.store(value_a, std::memory_order_relaxed);
  a.store(value_b, std::memory_order_relaxed);
}

MapFieldBase::ReflectionPayload& MapFieldBase::PayloadSlow() const {
  const void* p = globals_or_payload_.load(std::memory_order_acquire);
  if (!IsPayload(p)) {
    // Inject the sync callback.
    sync_map_with_repeated.store(
        [](auto& map, bool is_mutable) {
          const auto& self = static_cast<const MapFieldBase&>(map);
          self.SyncMapWithRepeatedField();
          if (is_mutable) const_cast<MapFieldBase&>(self).SetMapDirty();
        },
        std::memory_order_relaxed);

    const auto* prototype = MessageGlobalsBase::ToDefaultInstance<Message>(p);
    auto* payload =
        Arena::Create<ReflectionPayload>(arena(), arena(), prototype);

    auto new_p = ToTaggedPtr(payload);
    if (globals_or_payload_.compare_exchange_strong(
            p, new_p, std::memory_order_acq_rel)) {
      // We were able to store it.
      p = new_p;
    } else {
      // Someone beat us to it. Throw away the one we made. `p` already contains
      // the one we want.
      if (arena() == nullptr) delete payload;
    }
  }
  return *ToPayload(p);
}

void MapFieldBase::SwapPayload(MapFieldBase& lhs, MapFieldBase& rhs) {
  if (lhs.arena() == rhs.arena()) {
    SwapRelaxed(lhs.globals_or_payload_, rhs.globals_or_payload_);
    return;
  }
  auto* p1 = lhs.maybe_payload();
  auto* p2 = rhs.maybe_payload();
  if (p1 == nullptr && p2 == nullptr) return;

  if (p1 == nullptr) p1 = &lhs.payload();
  if (p2 == nullptr) p2 = &rhs.payload();
  p1->Swap(*p2);
}

void MapFieldBase::InternalSwap(MapFieldBase* other) {
  GetMapRaw().InternalSwap(&other->GetMapRaw());
  SwapPayload(*this, *other);
}

size_t MapFieldBase::SpaceUsedExcludingSelfLong() const {
  ConstAccess();
  size_t size = 0;
  if (auto* p = maybe_payload()) {
    absl::MutexLock lock(&p->mutex());
    // Measure the map under the lock, because there could be some repeated
    // field data that might be sync'd back into the map.
    size = GetMapRaw().SpaceUsedExcludingSelfLong();
    size += p->repeated_field().SpaceUsedExcludingSelfLong();
    ConstAccess();
  } else {
    // Only measure the map without the repeated field, because it is not there.
    size = GetMapRaw().SpaceUsedExcludingSelfLong();
    ConstAccess();
  }
  return size;
}

bool MapFieldBase::IsMapValid() const {
  ConstAccess();
  // "Acquire" insures the operation after SyncRepeatedFieldWithMap won't get
  // executed before state_ is checked.
  return state() != STATE_MODIFIED_REPEATED;
}

bool MapFieldBase::IsRepeatedFieldValid() const {
  ConstAccess();
  return state() != STATE_MODIFIED_MAP;
}

void MapFieldBase::SetRepeatedDirty() {
  MutableAccess();
  // These are called by (non-const) mutator functions. So by our API it's the
  // callers responsibility to have these calls properly ordered.
  payload().set_state_relaxed(STATE_MODIFIED_REPEATED);
}

const RepeatedPtrFieldBase& MapFieldBase::SyncRepeatedFieldWithMap(
    bool for_mutation) const {
  ConstAccess();
  if (state() == STATE_MODIFIED_MAP) {
    auto* p = maybe_payload();
    if (p == nullptr) {
      // If we have no payload, and we do not want to mutate the object, and the
      // map is empty, then do nothing.
      // This prevents modifying global default instances which might be in ro
      // memory.
      if (!for_mutation && GetMapRaw().empty()) {
        return *RawPtr<const RepeatedPtrFieldBase>();
      }
      p = &payload();
    }

    {
      absl::MutexLock lock(&p->mutex());
      // Double check state, because another thread may have seen the same
      // state and done the synchronization before the current thread.
      if (p->load_state_relaxed() == STATE_MODIFIED_MAP) {
        const_cast<MapFieldBase*>(this)->SyncRepeatedFieldWithMapNoLock();
        p->set_state_release(CLEAN);
      }
    }
    ConstAccess();
    return static_cast<const RepeatedPtrFieldBase&>(p->repeated_field());
  }
  return static_cast<const RepeatedPtrFieldBase&>(payload().repeated_field());
}

void MapFieldBase::SyncRepeatedFieldWithMapNoLock() {
  const Message* prototype = GetPrototype();
  const Reflection* reflection = prototype->GetReflection();
  const Descriptor* descriptor = prototype->GetDescriptor();
  const FieldDescriptor* key_des = descriptor->map_key();
  const FieldDescriptor* val_des = descriptor->map_value();

  RepeatedPtrField<Message>& rep = payload().repeated_field();
  rep.Clear();

  GenericConstMapRef::iterator it;
  it.key_type_ = descriptor->map_key()->cpp_type();
  it.value_type_ = descriptor->map_value()->cpp_type();
  it.iter_ = GetMapRaw().begin();

  GenericConstMapRef::iterator end;
  end.iter_ = UntypedMapBase::EndIterator();

  Arena* arena = this->arena();
  for (; it != end; ++it) {
    Message* new_entry = reinterpret_cast<Message*>(
        rep.AddInternal(arena, [prototype](Arena* arena, void*& ptr) {
          ptr = prototype->New(arena);
        }));

    MapKey map_key = it->key();
    switch (key_des->cpp_type()) {
      case FieldDescriptor::CPPTYPE_STRING:
        reflection->SetString(new_entry, key_des,
                              std::string(map_key.GetStringValue()));
        break;
      case FieldDescriptor::CPPTYPE_INT64:
        reflection->SetInt64(new_entry, key_des, map_key.GetInt64Value());
        break;
      case FieldDescriptor::CPPTYPE_INT32:
        reflection->SetInt32(new_entry, key_des, map_key.GetInt32Value());
        break;
      case FieldDescriptor::CPPTYPE_UINT64:
        reflection->SetUInt64(new_entry, key_des, map_key.GetUInt64Value());
        break;
      case FieldDescriptor::CPPTYPE_UINT32:
        reflection->SetUInt32(new_entry, key_des, map_key.GetUInt32Value());
        break;
      case FieldDescriptor::CPPTYPE_BOOL:
        reflection->SetBool(new_entry, key_des, map_key.GetBoolValue());
        break;
      default:
        Unreachable();
    }

    MapValueConstRef map_val = it->value();
    switch (val_des->cpp_type()) {
      case FieldDescriptor::CPPTYPE_STRING:
        reflection->SetString(new_entry, val_des,
                              std::string(map_val.GetStringValue()));
        break;
      case FieldDescriptor::CPPTYPE_INT64:
        reflection->SetInt64(new_entry, val_des, map_val.GetInt64Value());
        break;
      case FieldDescriptor::CPPTYPE_INT32:
        reflection->SetInt32(new_entry, val_des, map_val.GetInt32Value());
        break;
      case FieldDescriptor::CPPTYPE_UINT64:
        reflection->SetUInt64(new_entry, val_des, map_val.GetUInt64Value());
        break;
      case FieldDescriptor::CPPTYPE_UINT32:
        reflection->SetUInt32(new_entry, val_des, map_val.GetUInt32Value());
        break;
      case FieldDescriptor::CPPTYPE_BOOL:
        reflection->SetBool(new_entry, val_des, map_val.GetBoolValue());
        break;
      case FieldDescriptor::CPPTYPE_DOUBLE:
        reflection->SetDouble(new_entry, val_des, map_val.GetDoubleValue());
        break;
      case FieldDescriptor::CPPTYPE_FLOAT:
        reflection->SetFloat(new_entry, val_des, map_val.GetFloatValue());
        break;
      case FieldDescriptor::CPPTYPE_ENUM:
        reflection->SetEnumValue(new_entry, val_des, map_val.GetEnumValue());
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE: {
        const Message& message = map_val.GetMessageValue();
        reflection->MutableMessage(new_entry, val_des)->CopyFrom(message);
        break;
      }
    }
  }
}

void MapFieldBase::SyncMapWithRepeatedField() const {
  ConstAccess();
  // acquire here matches with release below to ensure that we can only see a
  // value of CLEAN after all previous changes have been synced.
  if (state() == STATE_MODIFIED_REPEATED) {
    auto& p = payload();
    {
      absl::MutexLock lock(&p.mutex());
      // Double check state, because another thread may have seen the same state
      // and done the synchronization before the current thread.
      if (p.load_state_relaxed() == STATE_MODIFIED_REPEATED) {
        const_cast<MapFieldBase*>(this)->SyncMapWithRepeatedFieldNoLock();
        p.set_state_release(CLEAN);
      }
    }
    ConstAccess();
  }
}

void MapFieldBase::SyncMapWithRepeatedFieldNoLock() {
  ClearMapNoSync();

  RepeatedPtrField<Message>& rep = payload().repeated_field();

  if (rep.empty()) return;

  const Message* prototype = &rep[0];
  const Reflection* reflection = prototype->GetReflection();
  const Descriptor* descriptor = prototype->GetDescriptor();
  const FieldDescriptor* key_des = descriptor->map_key();
  const FieldDescriptor* val_des = descriptor->map_value();

  GenericMapRef map_ref;
  map_ref.key_type_ = descriptor->map_key()->cpp_type();
  map_ref.value_type_ = descriptor->map_value()->cpp_type();
  map_ref.map_ = &GetMapRaw();
  if (descriptor->map_value()->message_type()) {
    map_ref.value_class_data_ =
        GetClassData(GetMapEntryValuePrototype(*prototype));
  }

  for (const Message& elem : rep) {
    // MapKey type will be set later.
    Reflection::ScratchSpace map_key_scratch_space;
    MapKey map_key;
    switch (key_des->cpp_type()) {
      case FieldDescriptor::CPPTYPE_STRING:
        map_key.SetStringValue(
            reflection->GetStringView(elem, key_des, map_key_scratch_space));
        break;
      case FieldDescriptor::CPPTYPE_INT64:
        map_key.SetInt64Value(reflection->GetInt64(elem, key_des));
        break;
      case FieldDescriptor::CPPTYPE_INT32:
        map_key.SetInt32Value(reflection->GetInt32(elem, key_des));
        break;
      case FieldDescriptor::CPPTYPE_UINT64:
        map_key.SetUInt64Value(reflection->GetUInt64(elem, key_des));
        break;
      case FieldDescriptor::CPPTYPE_UINT32:
        map_key.SetUInt32Value(reflection->GetUInt32(elem, key_des));
        break;
      case FieldDescriptor::CPPTYPE_BOOL:
        map_key.SetBoolValue(reflection->GetBool(elem, key_des));
        break;
      default:
        Unreachable();
    }

    MapValueRef map_val = map_ref.try_emplace(map_key).first->value();

    switch (val_des->cpp_type()) {
#define HANDLE_TYPE(CPPTYPE, METHOD)                                    \
  case FieldDescriptor::CPPTYPE_##CPPTYPE:                              \
    map_val.Set##METHOD##Value(reflection->Get##METHOD(elem, val_des)); \
    break;
      HANDLE_TYPE(INT32, Int32);
      HANDLE_TYPE(INT64, Int64);
      HANDLE_TYPE(UINT32, UInt32);
      HANDLE_TYPE(UINT64, UInt64);
      HANDLE_TYPE(DOUBLE, Double);
      HANDLE_TYPE(FLOAT, Float);
      HANDLE_TYPE(BOOL, Bool);
      HANDLE_TYPE(STRING, String);
#undef HANDLE_TYPE
      case FieldDescriptor::CPPTYPE_ENUM:
        map_val.SetEnumValue(reflection->GetEnumValue(elem, val_des));
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE: {
        map_val.MutableMessageValue()->CopyFrom(
            reflection->GetMessage(elem, val_des));
        break;
      }
    }
  }
}

void MapFieldBase::Clear() {
  if (ReflectionPayload* p = maybe_payload()) {
    p->repeated_field().Clear();
  }

  ClearMapNoSync();
  // Data in map and repeated field are both empty, but we can't set status
  // CLEAN. Because clear is a generated API, we cannot invalidate previous
  // reference to map.
  SetMapDirty();
}

void MapFieldBase::ReflectionPayload::Swap(ReflectionPayload& other) {
  repeated_field().Swap(&other.repeated_field());
  SwapRelaxed(state_, other.state_);
}

MapKey MapIteratorEntry::key() const {
  return MapConstIteratorEntry(*this).key();
}

MapValueRef MapIteratorEntry::value() const {
  MapValueRef value;
  value.SetType(static_cast<FieldDescriptor::CppType>(value_type_));
  value.SetValue(iter_.m_->GetVoidValue(iter_.node_));
  return value;
}

MapKey MapConstIteratorEntry::key() const {
  MapKey key;
  key.SetType(static_cast<FieldDescriptor::CppType>(key_type_));
  iter_.m_->VisitKey(
      iter_.node_,
      absl::Overload{
          [&](const std::string* v) { key.val_.string_value = *v; },
          [&](const auto* v) {
            // Memcpy the scalar into the union.
            memcpy(static_cast<void*>(&key.val_), v, sizeof(*v));
          },
      });
  return key;
}

MapValueConstRef MapConstIteratorEntry::value() const {
  MapValueConstRef value;
  value.SetType(static_cast<FieldDescriptor::CppType>(value_type_));
  value.SetValue(iter_.m_->GetVoidValue(iter_.node_));
  return value;
}

}  // namespace internal

GenericMapRef::iterator GenericMapRef::begin() const {
  iterator iter;
  iter.key_type_ = key_type_;
  iter.value_type_ = value_type_;
  iter.iter_ = map_->begin();
  return iter;
}

GenericMapRef::iterator GenericMapRef::end() const {
  iterator iter;
  iter.iter_ = internal::UntypedMapBase::EndIterator();
  return iter;
}

size_t GenericMapRef::size() const { return map_->size(); }

bool GenericMapRef::empty() const { return size() == 0; }

bool GenericMapRef::contains(const MapKey& key) const {
  return find(key) != end();
}

GenericMapRef::iterator GenericMapRef::find(const MapKey& key) const {
  auto it = GenericConstMapRef(*this).find(key);
  iterator res;
  res.key_type_ = key_type_;
  res.value_type_ = value_type_;
  res.iter_ = it.iter_;
  return res;
}

GenericMapRef::mapped_type GenericMapRef::at(const MapKey& key) const {
  iterator it = find(key);
  if (ABSL_PREDICT_FALSE(it == end())) {
    // DO NOT SUBMIT
    // absl::ThrowStdOutOfRange("Key not found.");
    ABSL_LOG(FATAL);
  }
  return it->value();
}

void GenericMapRef::assign(const GenericMapRef& other) const {
  map_->AssertSameType(*other.map_);
  if (map_ == other.map_) return;
  clear();
  map_->UntypedMergeFrom(map_->arena(), *other.map_);
}

void GenericMapRef::swap(const GenericMapRef& other) const {
  map_->AssertSameType(*other.map_);
  map_->UntypedSwap(map_->arena(), *other.map_, other.map_->arena());
}

void GenericMapRef::shallow_assign(const GenericMapRef& other) {
  map_->AssertSameType(*other.map_);
  map_ = other.map_;
}

void GenericMapRef::shallow_swap(GenericMapRef& other) {
  map_->AssertSameType(*other.map_);
  std::swap(map_, other.map_);
}

void GenericMapRef::clear() const {
  map_->ClearTable(map_->arena(), /*reset=*/true);
}

bool GenericMapRef::erase(const MapKey& key) const {
  return internal::VisitMapKey(key, *map_, [](auto& map, const auto& key) {
    return map.EraseImpl(map.arena(), key);
  });
}

bool GenericMapRef::erase(iterator it) const { return erase(it->key()); }

std::pair<GenericMapRef::iterator, bool> GenericMapRef::try_emplace(
    const MapKey& key) const {
  std::pair<iterator, bool> res;
  res.first.key_type_ = key_type_;
  res.first.value_type_ = value_type_;
  res.first.iter_.m_ = map_;

  if (!map_->empty()) {
    auto find_res = internal::VisitMapKey(
        key, *map_,
        [&](auto& map, const auto& key) { return map.FindHelper(key); });
    if (find_res.node != nullptr) {
      res.first.iter_.node_ = find_res.node;
      res.first.iter_.bucket_index_ = find_res.bucket;
      res.second = false;
      return res;
    }
  }

  Arena* arena = map_->arena();
  auto* node = map_->AllocNode(arena);
  map_->VisitValue(node, [&](auto* v) { InitializeKeyValue(v); });
  res.second = true;
  res.first.iter_.node_ = node;
  res.first.iter_.bucket_index_ = 0;

  internal::VisitMapKey(key, *map_, [&](auto& map, const auto& key) {
    InitializeKeyValue(map.GetKey(node), key);
    map.InsertOrReplaceNode(
        arena,
        static_cast<typename std::decay_t<decltype(map)>::KeyNode*>(node));
  });

  return res;
}

GenericConstMapRef::iterator GenericConstMapRef::begin() const {
  iterator iter;
  iter.key_type_ = key_type_;
  iter.value_type_ = value_type_;
  iter.iter_ = map_->begin();
  return iter;
}

GenericConstMapRef::iterator GenericConstMapRef::end() const {
  iterator iter;
  iter.iter_ = internal::UntypedMapBase::EndIterator();
  return iter;
}

size_t GenericConstMapRef::size() const { return map_->size(); }

bool GenericConstMapRef::empty() const { return size() == 0; }

bool GenericConstMapRef::contains(const MapKey& key) const {
  return find(key) != end();
}

GenericConstMapRef::iterator GenericConstMapRef::find(
    const MapKey& map_key) const {
  iterator res;
  res.key_type_ = key_type_;
  res.value_type_ = value_type_;

  if (map_->empty()) {
    res.iter_ = internal::UntypedMapBase::EndIterator();
    return res;
  }

  res.iter_ =
      internal::VisitMapKey(map_key, *map_, [&](auto& map, const auto& key) {
        auto res = map.FindHelper(key);
        if (res.node == nullptr) {
          return internal::UntypedMapBase::EndIterator();
        }
        internal::UntypedMapIterator iter;
        iter.node_ = res.node;
        iter.bucket_index_ = res.bucket;
        iter.m_ = &map;
        return iter;
      });

  return res;
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
