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

#include "absl/functional/overload.h"
#include "absl/log/absl_check.h"
#include "absl/synchronization/mutex.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/map.h"
#include "google/protobuf/port.h"
#include "google/protobuf/raw_ptr.h"
#include "google/protobuf/repeated_ptr_field.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

MapFieldBase::~MapFieldBase() {
  ABSL_DCHECK_EQ(arena(), nullptr);
  delete maybe_payload();
}

void MapFieldBase::MergeFrom(const MapFieldBase& other) {
  MutableMap()->UntypedMergeFrom(other.GetMap());
}

void MapFieldBase::Swap(MapFieldBase* other) {
  if (arena() == other->arena()) {
    InternalSwap(other);
    return;
  }
  MapFieldBase::SwapPayload(*this, *other);
  GetMapRaw().UntypedSwap(other->GetMapRaw());
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

bool MapFieldBase::InsertOrLookupMapValueNoSync(const MapKey& map_key,
                                                MapValueRef* val) {
  if (LookupMapValueNoSync(map_key, static_cast<MapValueConstRef*>(val))) {
    return false;
  }

  auto& map = GetMapRaw();

  NodeBase* node = map.AllocNode();
  map.VisitValue(node, [&](auto* v) { InitializeKeyValue(v); });
  val->SetValue(map.GetVoidValue(node));

  return VisitMapKey(map_key, map, [&](auto& map, const auto& key) {
    InitializeKeyValue(map.GetKey(node), key);
    map.InsertOrReplaceNode(
        static_cast<typename std::decay_t<decltype(map)>::KeyNode*>(node));
    return true;
  });
}

bool MapFieldBase::DeleteMapValue(const MapKey& map_key) {
  return VisitMapKey(map_key, *MutableMap(), [](auto& map, const auto& key) {
    return map.EraseImpl(key);
  });
}

void MapFieldBase::ClearMapNoSync() { GetMapRaw().ClearTable(true); }

void MapFieldBase::SetMapIteratorValue(MapIterator* map_iter) const {
  if (map_iter->iter_.Equals(UntypedMapBase::EndIterator())) return;

  const UntypedMapBase& map = *map_iter->iter_.m_;
  NodeBase* node = map_iter->iter_.node_;
  auto& key = map_iter->key_;
  map.VisitKey(node,
               absl::Overload{
                   [&](const std::string* v) { key.val_.string_value = *v; },
                   [&](const auto* v) {
                     // Memcpy the scalar into the union.
                     memcpy(static_cast<void*>(&key.val_), v, sizeof(*v));
                   },
               });
  map_iter->value_.SetValue(map.GetVoidValue(node));
}

bool MapFieldBase::LookupMapValueNoSync(const MapKey& map_key,
                                        MapValueConstRef* val) const {
  auto& map = GetMapRaw();
  if (map.empty()) return false;

  return VisitMapKey(map_key, map, [&](auto& map, const auto& key) {
    auto res = map.FindHelper(key);
    if (res.node == nullptr) {
      return false;
    }
    if (val != nullptr) {
      val->SetValue(map.GetVoidValue(res.node));
    }
    return true;
  });
}

void MapFieldBase::MapBegin(MapIterator* map_iter) const {
  map_iter->iter_ = GetMap().begin();
  SetMapIteratorValue(map_iter);
}

void MapFieldBase::MapEnd(MapIterator* map_iter) const {
  map_iter->iter_ = UntypedMapBase::EndIterator();
}

bool MapFieldBase::EqualIterator(const MapIterator& a,
                                 const MapIterator& b) const {
  return a.iter_.Equals(b.iter_);
}

void MapFieldBase::IncreaseIterator(MapIterator* map_iter) const {
  map_iter->iter_.PlusPlus();
  SetMapIteratorValue(map_iter);
}

void MapFieldBase::CopyIterator(MapIterator* this_iter,
                                const MapIterator& that_iter) const {
  this_iter->iter_ = that_iter.iter_;
  this_iter->key_.SetType(that_iter.key_.type());
  // MapValueRef::type() fails when containing data is null. However, if
  // this_iter points to MapEnd, data can be null.
  this_iter->value_.SetType(
      static_cast<FieldDescriptor::CppType>(that_iter.value_.type_));
  SetMapIteratorValue(this_iter);
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
  auto p = payload_.load(std::memory_order_acquire);
  if (!IsPayload(p)) {
    // Inject the sync callback.
    sync_map_with_repeated.store(
        [](auto& map, bool is_mutable) {
          const auto& self = static_cast<const MapFieldBase&>(map);
          self.SyncMapWithRepeatedField();
          if (is_mutable) const_cast<MapFieldBase&>(self).SetMapDirty();
        },
        std::memory_order_relaxed);

    auto* arena = ToArena(p);
    auto* payload = Arena::Create<ReflectionPayload>(arena, arena);

    auto new_p = ToTaggedPtr(payload);
    if (payload_.compare_exchange_strong(p, new_p, std::memory_order_acq_rel)) {
      // We were able to store it.
      p = new_p;
    } else {
      // Someone beat us to it. Throw away the one we made. `p` already contains
      // the one we want.
      if (arena == nullptr) delete payload;
    }
  }
  return *ToPayload(p);
}

void MapFieldBase::SwapPayload(MapFieldBase& lhs, MapFieldBase& rhs) {
  if (lhs.arena() == rhs.arena()) {
    SwapRelaxed(lhs.payload_, rhs.payload_);
    return;
  }
  auto* p1 = lhs.maybe_payload();
  auto* p2 = rhs.maybe_payload();
  if (p1 == nullptr && p2 == nullptr) return;

  if (p1 == nullptr) p1 = &lhs.payload();
  if (p2 == nullptr) p2 = &rhs.payload();
  p1->repeated_field.Swap(&p2->repeated_field);
  SwapRelaxed(p1->state, p2->state);
}

void MapFieldBase::InternalSwap(MapFieldBase* other) {
  GetMapRaw().InternalSwap(&other->GetMapRaw());
  SwapPayload(*this, *other);
}

size_t MapFieldBase::SpaceUsedExcludingSelfLong() const {
  ConstAccess();
  size_t size = 0;
  if (auto* p = maybe_payload()) {
    absl::MutexLock lock(&p->mutex);
    // Measure the map under the lock, because there could be some repeated
    // field data that might be sync'd back into the map.
    size = GetMapRaw().SpaceUsedExcludingSelfLong();
    size += p->repeated_field.SpaceUsedExcludingSelfLong();
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
  payload().state.store(STATE_MODIFIED_REPEATED, std::memory_order_relaxed);
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
      absl::MutexLock lock(&p->mutex);
      // Double check state, because another thread may have seen the same
      // state and done the synchronization before the current thread.
      if (p->state.load(std::memory_order_relaxed) == STATE_MODIFIED_MAP) {
        const_cast<MapFieldBase*>(this)->SyncRepeatedFieldWithMapNoLock();
        p->state.store(CLEAN, std::memory_order_release);
      }
    }
    ConstAccess();
    return reinterpret_cast<const RepeatedPtrFieldBase&>(p->repeated_field);
  }
  return reinterpret_cast<const RepeatedPtrFieldBase&>(
      payload().repeated_field);
}

void MapFieldBase::SyncRepeatedFieldWithMapNoLock() {
  const Message* prototype = GetPrototype();
  const Reflection* reflection = prototype->GetReflection();
  const Descriptor* descriptor = prototype->GetDescriptor();
  const FieldDescriptor* key_des = descriptor->map_key();
  const FieldDescriptor* val_des = descriptor->map_value();

  RepeatedPtrField<Message>& rep = payload().repeated_field;
  rep.Clear();

  MapIterator it(this, descriptor);
  MapIterator end(this, descriptor);

  it.iter_ = GetMapRaw().begin();
  SetMapIteratorValue(&it);
  end.iter_ = UntypedMapBase::EndIterator();

  for (; !EqualIterator(it, end); IncreaseIterator(&it)) {
    Message* new_entry = prototype->New(arena());
    rep.AddAllocated(new_entry);
    const MapKey& map_key = it.GetKey();
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

    const MapValueRef& map_val = it.GetValueRef();
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
      absl::MutexLock lock(&p.mutex);
      // Double check state, because another thread may have seen the same state
      // and done the synchronization before the current thread.
      if (p.state.load(std::memory_order_relaxed) == STATE_MODIFIED_REPEATED) {
        const_cast<MapFieldBase*>(this)->SyncMapWithRepeatedFieldNoLock();
        p.state.store(CLEAN, std::memory_order_release);
      }
    }
    ConstAccess();
  }
}

void MapFieldBase::SyncMapWithRepeatedFieldNoLock() {
  ClearMapNoSync();

  RepeatedPtrField<Message>& rep = payload().repeated_field;

  if (rep.empty()) return;

  const Message* prototype = &rep[0];
  const Reflection* reflection = prototype->GetReflection();
  const Descriptor* descriptor = prototype->GetDescriptor();
  const FieldDescriptor* key_des = descriptor->map_key();
  const FieldDescriptor* val_des = descriptor->map_value();

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

    MapValueRef map_val;
    map_val.SetType(val_des->cpp_type());
    InsertOrLookupMapValueNoSync(map_key, &map_val);

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
    p->repeated_field.Clear();
  }

  ClearMapNoSync();
  // Data in map and repeated field are both empty, but we can't set status
  // CLEAN. Because clear is a generated API, we cannot invalidate previous
  // reference to map.
  SetMapDirty();
}

int MapFieldBase::size() const { return GetMap().size(); }

bool MapFieldBase::InsertOrLookupMapValue(const MapKey& map_key,
                                          MapValueRef* val) {
  SyncMapWithRepeatedField();
  SetMapDirty();
  return InsertOrLookupMapValueNoSync(map_key, val);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
