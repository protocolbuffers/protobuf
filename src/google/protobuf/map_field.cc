// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/map_field.h"

#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "google/protobuf/map.h"
#include "google/protobuf/map_field_inl.h"
#include "google/protobuf/port.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
using ::google::protobuf::internal::DownCast;

VariantKey RealKeyToVariantKey<MapKey>::operator()(const MapKey& value) const {
  switch (value.type()) {
    case FieldDescriptor::CPPTYPE_STRING:
      return VariantKey(value.GetStringValue());
    case FieldDescriptor::CPPTYPE_INT64:
      return VariantKey(value.GetInt64Value());
    case FieldDescriptor::CPPTYPE_INT32:
      return VariantKey(value.GetInt32Value());
    case FieldDescriptor::CPPTYPE_UINT64:
      return VariantKey(value.GetUInt64Value());
    case FieldDescriptor::CPPTYPE_UINT32:
      return VariantKey(value.GetUInt32Value());
    case FieldDescriptor::CPPTYPE_BOOL:
      return VariantKey(static_cast<uint64_t>(value.GetBoolValue()));
    default:
      ABSL_ASSUME(false);
      return VariantKey(uint64_t{});
  }
}

MapFieldBase::~MapFieldBase() {
  ABSL_DCHECK_EQ(arena(), nullptr);
  delete maybe_payload();
}

const UntypedMapBase& MapFieldBase::GetMapImpl(const MapFieldBaseForParse& map,
                                               bool is_mutable) {
  const auto& self = static_cast<const MapFieldBase&>(map);
  self.SyncMapWithRepeatedField();
  if (is_mutable) const_cast<MapFieldBase&>(self).SetMapDirty();
  return self.GetMapRaw();
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
  SyncRepeatedFieldWithMap();
  return reinterpret_cast<RepeatedPtrFieldBase&>(payload().repeated_field);
}

RepeatedPtrFieldBase* MapFieldBase::MutableRepeatedField() {
  MutableAccess();
  SyncRepeatedFieldWithMap();
  SetRepeatedDirty();
  return reinterpret_cast<RepeatedPtrFieldBase*>(&payload().repeated_field);
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

void MapFieldBase::SwapImpl(MapFieldBase& lhs, MapFieldBase& rhs) {
  if (lhs.arena() == rhs.arena()) {
    lhs.InternalSwap(&rhs);
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

void MapFieldBase::UnsafeShallowSwapImpl(MapFieldBase& lhs, MapFieldBase& rhs) {
  ABSL_DCHECK_EQ(lhs.arena(), rhs.arena());
  lhs.InternalSwap(&rhs);
}

void MapFieldBase::InternalSwap(MapFieldBase* other) {
  SwapRelaxed(payload_, other->payload_);
}

size_t MapFieldBase::SpaceUsedExcludingSelfLong() const {
  ConstAccess();
  size_t size = 0;
  if (auto* p = maybe_payload()) {
    {
      absl::MutexLock lock(&p->mutex);
      size = SpaceUsedExcludingSelfNoLock();
    }
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

void MapFieldBase::SetMapDirty() {
  MutableAccess();
  // These are called by (non-const) mutator functions. So by our API it's the
  // callers responsibility to have these calls properly ordered.
  payload().state.store(STATE_MODIFIED_MAP, std::memory_order_relaxed);
}

void MapFieldBase::SetRepeatedDirty() {
  MutableAccess();
  // These are called by (non-const) mutator functions. So by our API it's the
  // callers responsibility to have these calls properly ordered.
  payload().state.store(STATE_MODIFIED_REPEATED, std::memory_order_relaxed);
}

void MapFieldBase::SyncRepeatedFieldWithMap() const {
  ConstAccess();
  if (state() == STATE_MODIFIED_MAP) {
    auto& p = payload();
    {
      absl::MutexLock lock(&p.mutex);
      // Double check state, because another thread may have seen the same
      // state and done the synchronization before the current thread.
      if (p.state.load(std::memory_order_relaxed) == STATE_MODIFIED_MAP) {
        const_cast<MapFieldBase*>(this)->SyncRepeatedFieldWithMapNoLock();
        p.state.store(CLEAN, std::memory_order_release);
      }
    }
    ConstAccess();
  }
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
        reflection->SetString(new_entry, key_des, map_key.GetStringValue());
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
        PROTOBUF_ASSUME(false);
    }

    const MapValueRef& map_val = it.GetValueRef();
    switch (val_des->cpp_type()) {
      case FieldDescriptor::CPPTYPE_STRING:
        reflection->SetString(new_entry, val_des, map_val.GetStringValue());
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
    MapKey map_key;
    switch (key_des->cpp_type()) {
      case FieldDescriptor::CPPTYPE_STRING:
        map_key.SetStringValue(reflection->GetString(elem, key_des));
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
        PROTOBUF_ASSUME(false);
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

// ------------------DynamicMapField------------------
DynamicMapField::DynamicMapField(const Message* default_entry)
    : DynamicMapField::TypeDefinedMapFieldBase(&kVTable),
      default_entry_(default_entry) {}

DynamicMapField::DynamicMapField(const Message* default_entry, Arena* arena)
    : TypeDefinedMapFieldBase<MapKey, MapValueRef>(&kVTable, arena),
      default_entry_(default_entry) {}

constexpr DynamicMapField::VTable DynamicMapField::kVTable =
    MakeVTable<DynamicMapField>();

DynamicMapField::~DynamicMapField() {
  ABSL_DCHECK_EQ(arena(), nullptr);
  // DynamicMapField owns map values. Need to delete them before clearing the
  // map.
  for (auto& kv : map_) {
    kv.second.DeleteData();
  }
  map_.clear();
}

void DynamicMapField::ClearMapNoSyncImpl(MapFieldBase& base) {
  auto& self = static_cast<DynamicMapField&>(base);
  if (self.arena() == nullptr) {
    for (auto& elem : self.map_) {
      elem.second.DeleteData();
    }
  }

  self.map_.clear();
}

void DynamicMapField::AllocateMapValue(MapValueRef* map_val) {
  const FieldDescriptor* val_des = default_entry_->GetDescriptor()->map_value();
  map_val->SetType(val_des->cpp_type());
  // Allocate memory for the MapValueRef, and initialize to
  // default value.
  switch (val_des->cpp_type()) {
#define HANDLE_TYPE(CPPTYPE, TYPE)              \
  case FieldDescriptor::CPPTYPE_##CPPTYPE: {    \
    auto* value = Arena::Create<TYPE>(arena()); \
    map_val->SetValue(value);                   \
    break;                                      \
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
#undef HANDLE_TYPE
    case FieldDescriptor::CPPTYPE_MESSAGE: {
      const Message& message =
          default_entry_->GetReflection()->GetMessage(*default_entry_, val_des);
      Message* value = message.New(arena());
      map_val->SetValue(value);
      break;
    }
  }
}

bool DynamicMapField::InsertOrLookupMapValueNoSyncImpl(MapFieldBase& base,
                                                       const MapKey& map_key,
                                                       MapValueRef* val) {
  auto& self = static_cast<DynamicMapField&>(base);
  Map<MapKey, MapValueRef>::iterator iter = self.map_.find(map_key);
  if (iter == self.map_.end()) {
    MapValueRef& map_val = self.map_[map_key];
    self.AllocateMapValue(&map_val);
    val->CopyFrom(map_val);
    return true;
  }
  // map_key is already in the map. Make sure (*map)[map_key] is not called.
  // [] may reorder the map and iterators.
  val->CopyFrom(iter->second);
  return false;
}

void DynamicMapField::MergeFromImpl(MapFieldBase& base,
                                    const MapFieldBase& other) {
  auto& self = static_cast<DynamicMapField&>(base);
  ABSL_DCHECK(self.IsMapValid() && other.IsMapValid());
  Map<MapKey, MapValueRef>* map = self.MutableMap();
  const DynamicMapField& other_field =
      reinterpret_cast<const DynamicMapField&>(other);
  for (Map<MapKey, MapValueRef>::const_iterator other_it =
           other_field.map_.begin();
       other_it != other_field.map_.end(); ++other_it) {
    Map<MapKey, MapValueRef>::iterator iter = map->find(other_it->first);
    MapValueRef* map_val;
    if (iter == map->end()) {
      map_val = &self.map_[other_it->first];
      self.AllocateMapValue(map_val);
    } else {
      map_val = &iter->second;
    }

    // Copy map value
    const FieldDescriptor* field_descriptor =
        self.default_entry_->GetDescriptor()->map_value();
    switch (field_descriptor->cpp_type()) {
      case FieldDescriptor::CPPTYPE_INT32: {
        map_val->SetInt32Value(other_it->second.GetInt32Value());
        break;
      }
      case FieldDescriptor::CPPTYPE_INT64: {
        map_val->SetInt64Value(other_it->second.GetInt64Value());
        break;
      }
      case FieldDescriptor::CPPTYPE_UINT32: {
        map_val->SetUInt32Value(other_it->second.GetUInt32Value());
        break;
      }
      case FieldDescriptor::CPPTYPE_UINT64: {
        map_val->SetUInt64Value(other_it->second.GetUInt64Value());
        break;
      }
      case FieldDescriptor::CPPTYPE_FLOAT: {
        map_val->SetFloatValue(other_it->second.GetFloatValue());
        break;
      }
      case FieldDescriptor::CPPTYPE_DOUBLE: {
        map_val->SetDoubleValue(other_it->second.GetDoubleValue());
        break;
      }
      case FieldDescriptor::CPPTYPE_BOOL: {
        map_val->SetBoolValue(other_it->second.GetBoolValue());
        break;
      }
      case FieldDescriptor::CPPTYPE_STRING: {
        map_val->SetStringValue(other_it->second.GetStringValue());
        break;
      }
      case FieldDescriptor::CPPTYPE_ENUM: {
        map_val->SetEnumValue(other_it->second.GetEnumValue());
        break;
      }
      case FieldDescriptor::CPPTYPE_MESSAGE: {
        map_val->MutableMessageValue()->CopyFrom(
            other_it->second.GetMessageValue());
        break;
      }
    }
  }
}

const Message* DynamicMapField::GetPrototypeImpl(const MapFieldBase& map) {
  return static_cast<const DynamicMapField&>(map).default_entry_;
}

size_t DynamicMapField::SpaceUsedExcludingSelfNoLockImpl(
    const MapFieldBase& map) {
  auto& self = static_cast<const DynamicMapField&>(map);
  size_t size = 0;
  if (auto* p = self.maybe_payload()) {
    size += p->repeated_field.SpaceUsedExcludingSelfLong();
  }
  size_t map_size = self.map_.size();
  if (map_size) {
    Map<MapKey, MapValueRef>::const_iterator it = self.map_.begin();
    size += sizeof(it->first) * map_size;
    size += sizeof(it->second) * map_size;
    // If key is string, add the allocated space.
    if (it->first.type() == FieldDescriptor::CPPTYPE_STRING) {
      size += sizeof(std::string) * map_size;
    }
    // Add the allocated space in MapValueRef.
    switch (it->second.type()) {
#define HANDLE_TYPE(CPPTYPE, TYPE)           \
  case FieldDescriptor::CPPTYPE_##CPPTYPE: { \
    size += sizeof(TYPE) * map_size;         \
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
#undef HANDLE_TYPE
      case FieldDescriptor::CPPTYPE_MESSAGE: {
        while (it != self.map_.end()) {
          const Message& message = it->second.GetMessageValue();
          size += message.GetReflection()->SpaceUsedLong(message);
          ++it;
        }
        break;
      }
    }
  }
  return size;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
