// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// DynamicMessage is implemented by constructing a data structure which
// has roughly the same memory layout as a generated message would have.
// Then, we use Reflection to implement our reflection interface.  All
// the other operations we need to implement (e.g.  parsing, copying,
// etc.) are already implemented in terms of Reflection, so the rest is
// easy.
//
// The up side of this strategy is that it's very efficient.  We don't
// need to use hash_maps or generic representations of fields.  The
// down side is that this is a low-level memory management hack which
// can be tricky to get right.
//
// As mentioned in the header, we only expose a DynamicMessageFactory
// publicly, not the DynamicMessage class itself.  This is because
// GenericMessageReflection wants to have a pointer to a "default"
// copy of the class, with all fields initialized to their default
// values.  We only want to construct one of these per message type,
// so DynamicMessageFactory stores a cache of default messages for
// each type it sees (each unique Descriptor pointer).  The code
// refers to the "default" copy of the class as the "prototype".
//
// Note on memory allocation:  This module often calls "operator new()"
// to allocate untyped memory, rather than calling something like
// "new uint8_t[]".  This is because "operator new()" means "Give me some
// space which I can use as I please." while "new uint8_t[]" means "Give
// me an array of 8-bit integers.".  In practice, the later may return
// a pointer that is not aligned correctly for general use.  I believe
// Item 8 of "More Effective C++" discusses this in more detail, though
// I don't have the book on me right now so I'm not sure.

#include "google/protobuf/dynamic_message.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <type_traits>

#include "absl/base/attributes.h"
#include "absl/hash/hash.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/types/variant.h"
#include "absl/utility/utility.h"
#include "google/protobuf/arenastring.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/map.h"
#include "google/protobuf/map_field.h"
#include "google/protobuf/map_field_inl.h"  // IWYU pragma: keep
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/unknown_field_set.h"
#include "google/protobuf/wire_format.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
using internal::ExtensionSet;


using internal::ArenaStringPtr;

// ===================================================================
// Some helper tables and functions...

namespace internal {

// Used by DynamicMapField for it's key type.
//
// This is a lite wrapper around `absl::variant`. We do not use the variant
// directly to prevent accidental hashing or equality of the implicitly provided
// operators.
class DynamicMapKey {
 public:
  DynamicMapKey() = default;
  DynamicMapKey(const DynamicMapKey&) = default;
  DynamicMapKey(DynamicMapKey&&) = default;
  DynamicMapKey& operator=(const DynamicMapKey&) = default;
  DynamicMapKey& operator=(DynamicMapKey&&) = default;

  explicit DynamicMapKey(google::protobuf::MapKey map_key)
      : variant_(FromMapKey(map_key)) {}

  google::protobuf::MapKey ToMapKey() const ABSL_ATTRIBUTE_LIFETIME_BOUND;

  bool IsString() const {
    return absl::holds_alternative<std::string>(variant_);
  }

  friend void swap(DynamicMapKey& lhs, DynamicMapKey& rhs) noexcept {
    using std::swap;
    swap(lhs.variant_, rhs.variant_);
  }

 private:
  using Variant =
      absl::variant<bool, int32_t, int64_t, uint32_t, uint64_t, std::string>;

  static Variant FromMapKey(google::protobuf::MapKey map_key);

  Variant variant_;
};

// The other overloads for SetMapKey are located in map_field_inl.h
inline void SetMapKey(MapKey* map_key, const DynamicMapKey& value) {
  *map_key = value.ToMapKey();
}

template <>
struct is_internal_map_key_type<DynamicMapKey> : std::true_type {};

template <>
struct TransparentSupport<DynamicMapKey> {
  template <typename K>
  using key_arg = K;

  using ViewType = google::protobuf::MapKey;

  static ViewType ToView(ViewType v) { return v; }

  static ViewType ToView(const DynamicMapKey& v ABSL_ATTRIBUTE_LIFETIME_BOUND) {
    return v.ToMapKey();
  }
};

DynamicMapKey::Variant DynamicMapKey::FromMapKey(google::protobuf::MapKey map_key) {
  switch (map_key.type()) {
    case FieldDescriptor::CPPTYPE_STRING:
      return DynamicMapKey::Variant(absl::in_place_type<std::string>,
                                    map_key.GetStringValue());
    case FieldDescriptor::CPPTYPE_INT64:
      return DynamicMapKey::Variant(map_key.GetInt64Value());
    case FieldDescriptor::CPPTYPE_INT32:
      return DynamicMapKey::Variant(map_key.GetInt32Value());
    case FieldDescriptor::CPPTYPE_UINT64:
      return DynamicMapKey::Variant(map_key.GetUInt64Value());
    case FieldDescriptor::CPPTYPE_UINT32:
      return DynamicMapKey::Variant(map_key.GetUInt32Value());
    case FieldDescriptor::CPPTYPE_BOOL:
      return DynamicMapKey::Variant(map_key.GetBoolValue());
    default:
      internal::Unreachable();
  }
}

namespace {

struct DynamicMapKeyToMapKey {
  google::protobuf::MapKey* map_key;

  void operator()(bool value) const { map_key->SetBoolValue(value); }

  void operator()(int32_t value) const { map_key->SetInt32Value(value); }

  void operator()(int64_t value) const { map_key->SetInt64Value(value); }

  void operator()(uint32_t value) const { map_key->SetUInt32Value(value); }

  void operator()(uint64_t value) const { map_key->SetUInt64Value(value); }

  void operator()(const std::string& value) const {
    map_key->SetStringValue(value);
  }
};

}  // namespace

google::protobuf::MapKey DynamicMapKey::ToMapKey() const {
  google::protobuf::MapKey result;
  absl::visit(DynamicMapKeyToMapKey{&result}, variant_);
  return result;
}

class DynamicMapField final
    : public TypeDefinedMapFieldBase<DynamicMapKey, MapValueRef> {
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

DynamicMapField::DynamicMapField(const Message* default_entry)
    : DynamicMapField::TypeDefinedMapFieldBase(&kVTable),
      default_entry_(default_entry) {}

DynamicMapField::DynamicMapField(const Message* default_entry, Arena* arena)
    : TypeDefinedMapFieldBase<DynamicMapKey, MapValueRef>(&kVTable, arena),
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
  auto iter = self.map_.find(map_key);
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
  Map<DynamicMapKey, MapValueRef>* map = self.MutableMap();
  const DynamicMapField& other_field =
      reinterpret_cast<const DynamicMapField&>(other);
  for (auto other_it = other_field.map_.begin();
       other_it != other_field.map_.end(); ++other_it) {
    auto iter = map->find(other_it->first);
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
    auto it = self.map_.begin();
    size += sizeof(it->first) * map_size;
    size += sizeof(it->second) * map_size;
    // If key is string, add the allocated space.
    if (it->first.IsString()) {
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

using internal::DynamicMapField;

namespace {

bool IsMapFieldInApi(const FieldDescriptor* field) { return field->is_map(); }

bool IsMapEntryField(const FieldDescriptor* field) {
  return (field->containing_type() != nullptr &&
          field->containing_type()->options().map_entry());
}


inline bool InRealOneof(const FieldDescriptor* field) {
  return field->real_containing_oneof() != nullptr;
}

// Compute the byte size of the in-memory representation of the field.
int FieldSpaceUsed(const FieldDescriptor* field) {
  typedef FieldDescriptor FD;  // avoid line wrapping
  if (field->label() == FD::LABEL_REPEATED) {
    switch (field->cpp_type()) {
      case FD::CPPTYPE_INT32:
        return sizeof(RepeatedField<int32_t>);
      case FD::CPPTYPE_INT64:
        return sizeof(RepeatedField<int64_t>);
      case FD::CPPTYPE_UINT32:
        return sizeof(RepeatedField<uint32_t>);
      case FD::CPPTYPE_UINT64:
        return sizeof(RepeatedField<uint64_t>);
      case FD::CPPTYPE_DOUBLE:
        return sizeof(RepeatedField<double>);
      case FD::CPPTYPE_FLOAT:
        return sizeof(RepeatedField<float>);
      case FD::CPPTYPE_BOOL:
        return sizeof(RepeatedField<bool>);
      case FD::CPPTYPE_ENUM:
        return sizeof(RepeatedField<int>);
      case FD::CPPTYPE_MESSAGE:
        if (IsMapFieldInApi(field)) {
          return sizeof(DynamicMapField);
        } else {
          return sizeof(RepeatedPtrField<Message>);
        }

      case FD::CPPTYPE_STRING:
        switch (field->cpp_string_type()) {
          case FieldDescriptor::CppStringType::kCord:
            return sizeof(RepeatedField<absl::Cord>);
          case FieldDescriptor::CppStringType::kView:
          case FieldDescriptor::CppStringType::kString:
            return sizeof(RepeatedPtrField<std::string>);
        }
        break;
    }
  } else {
    switch (field->cpp_type()) {
      case FD::CPPTYPE_INT32:
        return sizeof(int32_t);
      case FD::CPPTYPE_INT64:
        return sizeof(int64_t);
      case FD::CPPTYPE_UINT32:
        return sizeof(uint32_t);
      case FD::CPPTYPE_UINT64:
        return sizeof(uint64_t);
      case FD::CPPTYPE_DOUBLE:
        return sizeof(double);
      case FD::CPPTYPE_FLOAT:
        return sizeof(float);
      case FD::CPPTYPE_BOOL:
        return sizeof(bool);
      case FD::CPPTYPE_ENUM:
        return sizeof(int);

      case FD::CPPTYPE_MESSAGE:
        return sizeof(Message*);

      case FD::CPPTYPE_STRING:
        switch (field->cpp_string_type()) {
          case FieldDescriptor::CppStringType::kCord:
            return sizeof(absl::Cord);
          case FieldDescriptor::CppStringType::kView:
          case FieldDescriptor::CppStringType::kString:
            return sizeof(ArenaStringPtr);
        }
        break;
    }
  }

  ABSL_DLOG(FATAL) << "Can't get here.";
  return 0;
}

inline int DivideRoundingUp(int i, int j) { return (i + (j - 1)) / j; }

static const int kSafeAlignment = sizeof(uint64_t);
static const int kMaxOneofUnionSize = sizeof(uint64_t);

inline int AlignTo(int offset, int alignment) {
  return DivideRoundingUp(offset, alignment) * alignment;
}

// Rounds the given byte offset up to the next offset aligned such that any
// type may be stored at it.
inline int AlignOffset(int offset) { return AlignTo(offset, kSafeAlignment); }

#define bitsizeof(T) (sizeof(T) * 8)

}  // namespace

// ===================================================================

class DynamicMessage final : public Message {
 public:
  // This should only be used by GetPrototypeNoLock() to avoid dead lock.
  DynamicMessage(DynamicMessageFactory::TypeInfo* type_info, bool lock_factory);
  DynamicMessage(const DynamicMessage&) = delete;
  DynamicMessage& operator=(const DynamicMessage&) = delete;

  ~DynamicMessage() PROTOBUF_FINAL;

  // Called on the prototype after construction to initialize message fields.
  // Cross linking the default instances allows for fast reflection access of
  // unset message fields. Without it we would have to go to the MessageFactory
  // to get the prototype, which is a much more expensive operation.
  //
  // Generated messages do not cross-link to avoid dynamic initialization of the
  // global instances.
  // Instead, they keep the default instances in the FieldDescriptor objects.
  void CrossLinkPrototypes();

  // implements Message ----------------------------------------------

  const internal::ClassData* GetClassData() const PROTOBUF_FINAL;

#if defined(__cpp_lib_destroying_delete) && defined(__cpp_sized_deallocation)
  static void operator delete(DynamicMessage* msg, std::destroying_delete_t);
#else
  // We actually allocate more memory than sizeof(*this) when this
  // class's memory is allocated via the global operator new. Thus, we need to
  // manually call the global operator delete. Calling the destructor is taken
  // care of for us. This makes DynamicMessage compatible with -fsized-delete.
  // It doesn't work for MSVC though.
#ifndef _MSC_VER
  static void operator delete(void* ptr) { ::operator delete(ptr); }
#endif  // !_MSC_VER
#endif

 private:
  DynamicMessage(const DynamicMessageFactory::TypeInfo* type_info,
                 Arena* arena);

  void SharedCtor(bool lock_factory);

  // Needed to get the offset of the internal metadata member.
  friend class DynamicMessageFactory;

  bool is_prototype() const;

  inline void* OffsetToPointer(int offset) {
    return reinterpret_cast<uint8_t*>(this) + offset;
  }
  inline const void* OffsetToPointer(int offset) const {
    return reinterpret_cast<const uint8_t*>(this) + offset;
  }

  static void* NewImpl(const void* prototype, void* mem, Arena* arena);
  static void DestroyImpl(MessageLite& ptr);

  void* MutableRaw(int i);
  void* MutableExtensionsRaw();
  void* MutableWeakFieldMapRaw();
  void* MutableOneofCaseRaw(int i);
  void* MutableOneofFieldRaw(const FieldDescriptor* f);

  const DynamicMessageFactory::TypeInfo* type_info_;
  internal::CachedSize cached_byte_size_;
};

struct DynamicMessageFactory::TypeInfo {
  int has_bits_offset;
  int oneof_case_offset;
  int extensions_offset;

  // Not owned by the TypeInfo.
  DynamicMessageFactory* factory;  // The factory that created this object.
  const DescriptorPool* pool;      // The factory's DescriptorPool.

  // Warning:  The order in which the following pointers are defined is
  //   important (the prototype must be deleted *before* the offsets).
  std::unique_ptr<uint32_t[]> offsets;
  std::unique_ptr<uint32_t[]> has_bits_indices;
  int weak_field_map_offset;  // The offset for the weak_field_map;

  internal::ClassDataFull class_data = {
      internal::ClassData{
          nullptr,  // default_instance
          nullptr,  // tc_table
          nullptr,  // on_demand_register_arena_dtor
          &DynamicMessage::IsInitializedImpl,
          &DynamicMessage::MergeImpl,
          internal::MessageCreator(),  // to be filled later
          &DynamicMessage::DestroyImpl,
          static_cast<void (MessageLite::*)()>(&DynamicMessage::ClearImpl),
          DynamicMessage::ByteSizeLongImpl,
          DynamicMessage::_InternalSerializeImpl,
          PROTOBUF_FIELD_OFFSET(DynamicMessage, cached_byte_size_),
          false,
      },
      &DynamicMessage::kDescriptorMethods,
      nullptr,  // descriptor_table
      nullptr,  // get_metadata_tracker
  };

  TypeInfo() = default;

  ~TypeInfo() {
    delete class_data.prototype;
    delete class_data.reflection;

    auto* type = class_data.descriptor;

    // Scribble the payload to prevent unsanitized opt builds from silently
    // allowing use-after-free bugs where the factory is destroyed but the
    // DynamicMessage instances are still used.
    // This is a common bug with DynamicMessageFactory.
    // NOTE: This must happen after deleting the prototype.
    if (offsets != nullptr) {
      std::fill_n(offsets.get(), type->field_count(), 0xCDCDCDCDu);
    }
    if (has_bits_indices != nullptr) {
      std::fill_n(has_bits_indices.get(), type->field_count(), 0xCDCDCDCDu);
    }
  }
};

DynamicMessage::DynamicMessage(const DynamicMessageFactory::TypeInfo* type_info,
                               Arena* arena)
    : Message(arena, type_info->class_data.base()),
      type_info_(type_info),
      cached_byte_size_(0) {
  SharedCtor(true);
}

DynamicMessage::DynamicMessage(DynamicMessageFactory::TypeInfo* type_info,
                               bool lock_factory)
    : Message(type_info->class_data.base()),
      type_info_(type_info),
      cached_byte_size_(0) {
  // The prototype in type_info has to be set before creating the prototype
  // instance on memory. e.g., message Foo { map<int32_t, Foo> a = 1; }. When
  // creating prototype for Foo, prototype of the map entry will also be
  // created, which needs the address of the prototype of Foo (the value in
  // map). To break the cyclic dependency, we have to assign the address of
  // prototype into type_info first.
  type_info->class_data.prototype = this;
  SharedCtor(lock_factory);
}

inline void* DynamicMessage::MutableRaw(int i) {
  return OffsetToPointer(type_info_->offsets[i]);
}
inline void* DynamicMessage::MutableExtensionsRaw() {
  return OffsetToPointer(type_info_->extensions_offset);
}
inline void* DynamicMessage::MutableWeakFieldMapRaw() {
  return OffsetToPointer(type_info_->weak_field_map_offset);
}
inline void* DynamicMessage::MutableOneofCaseRaw(int i) {
  return OffsetToPointer(type_info_->oneof_case_offset + sizeof(uint32_t) * i);
}
inline void* DynamicMessage::MutableOneofFieldRaw(const FieldDescriptor* f) {
  return OffsetToPointer(
      type_info_->offsets[type_info_->class_data.descriptor->field_count() +
                          f->containing_oneof()->index()]);
}

void DynamicMessage::SharedCtor(bool lock_factory) {
  // We need to call constructors for various fields manually and set
  // default values where appropriate.  We use placement new to call
  // constructors.  If you haven't heard of placement new, I suggest Googling
  // it now.  We use placement new even for primitive types that don't have
  // constructors for consistency.  (In theory, placement new should be used
  // any time you are trying to convert untyped memory to typed memory, though
  // in practice that's not strictly necessary for types that don't have a
  // constructor.)

  const Descriptor* descriptor = type_info_->class_data.descriptor;
  Arena* arena = GetArena();
  // Initialize oneof cases.
  int oneof_count = 0;
  for (int i = 0; i < descriptor->real_oneof_decl_count(); ++i) {
    new (MutableOneofCaseRaw(oneof_count++)) uint32_t{0};
  }

  if (type_info_->extensions_offset != -1) {
    new (MutableExtensionsRaw()) ExtensionSet(arena);
  }
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    void* field_ptr = MutableRaw(i);
    if (InRealOneof(field)) {
      continue;
    }
    switch (field->cpp_type()) {
#define HANDLE_TYPE(CPPTYPE, TYPE)                         \
  case FieldDescriptor::CPPTYPE_##CPPTYPE:                 \
    if (!field->is_repeated()) {                           \
      new (field_ptr) TYPE(field->default_value_##TYPE()); \
    } else {                                               \
      new (field_ptr) RepeatedField<TYPE>(arena);          \
    }                                                      \
    break;

      HANDLE_TYPE(INT32, int32_t);
      HANDLE_TYPE(INT64, int64_t);
      HANDLE_TYPE(UINT32, uint32_t);
      HANDLE_TYPE(UINT64, uint64_t);
      HANDLE_TYPE(DOUBLE, double);
      HANDLE_TYPE(FLOAT, float);
      HANDLE_TYPE(BOOL, bool);
#undef HANDLE_TYPE

      case FieldDescriptor::CPPTYPE_ENUM:
        if (!field->is_repeated()) {
          new (field_ptr) int{field->default_value_enum()->number()};
        } else {
          new (field_ptr) RepeatedField<int>(arena);
        }
        break;

      case FieldDescriptor::CPPTYPE_STRING:
        switch (field->cpp_string_type()) {
          case FieldDescriptor::CppStringType::kCord:
            if (!field->is_repeated()) {
              if (field->has_default_value()) {
                new (field_ptr) absl::Cord(field->default_value_string());
              } else {
                new (field_ptr) absl::Cord;
              }
              if (arena != nullptr) {
                // Cord does not support arena so here we need to notify arena
                // to remove the data it allocated on the heap by calling its
                // destructor.
                arena->OwnDestructor(static_cast<absl::Cord*>(field_ptr));
              }
            } else {
              new (field_ptr) RepeatedField<absl::Cord>(arena);
              if (arena != nullptr) {
                // Needs to destroy Cord elements.
                arena->OwnDestructor(
                    static_cast<RepeatedField<absl::Cord>*>(field_ptr));
              }
            }
            break;
          case FieldDescriptor::CppStringType::kView:
          case FieldDescriptor::CppStringType::kString:
            if (!field->is_repeated()) {
              ArenaStringPtr* asp = new (field_ptr) ArenaStringPtr();
              asp->InitDefault();
            } else {
              new (field_ptr) RepeatedPtrField<std::string>(arena);
            }
            break;
        }
        break;

      case FieldDescriptor::CPPTYPE_MESSAGE: {
        if (!field->is_repeated()) {
          new (field_ptr) Message*(nullptr);
        } else {
          if (IsMapFieldInApi(field)) {
            // We need to lock in most cases to avoid data racing. Only not lock
            // when the constructor is called inside GetPrototype(), in which
            // case we have already locked the factory.
            if (lock_factory) {
              if (arena != nullptr) {
                new (field_ptr) DynamicMapField(
                    type_info_->factory->GetPrototype(field->message_type()),
                    arena);
              } else {
                new (field_ptr) DynamicMapField(
                    type_info_->factory->GetPrototype(field->message_type()));
              }
            } else {
              if (arena != nullptr) {
                new (field_ptr)
                    DynamicMapField(type_info_->factory->GetPrototypeNoLock(
                                        field->message_type()),
                                    arena);
              } else {
                new (field_ptr)
                    DynamicMapField(type_info_->factory->GetPrototypeNoLock(
                        field->message_type()));
              }
            }
          } else {
            new (field_ptr) RepeatedPtrField<Message>(arena);
          }
        }
        break;
      }
    }
  }
}

bool DynamicMessage::is_prototype() const {
  return type_info_->class_data.prototype == this ||
         // If type_info_->prototype is nullptr, then we must be constructing
         // the prototype now, which means we must be the prototype.
         type_info_->class_data.prototype == nullptr;
}

#if defined(__cpp_lib_destroying_delete) && defined(__cpp_sized_deallocation)
void DynamicMessage::operator delete(DynamicMessage* msg,
                                     std::destroying_delete_t) {
  const size_t size = msg->type_info_->class_data.allocation_size();
  msg->~DynamicMessage();
  ::operator delete(msg, size);
}
#endif

DynamicMessage::~DynamicMessage() {
  const Descriptor* descriptor = type_info_->class_data.descriptor;

  _internal_metadata_.Delete<UnknownFieldSet>();

  if (type_info_->extensions_offset != -1) {
    reinterpret_cast<ExtensionSet*>(MutableExtensionsRaw())->~ExtensionSet();
  }

  // We need to manually run the destructors for repeated fields and strings,
  // just as we ran their constructors in the DynamicMessage constructor.
  // We also need to manually delete oneof fields if it is set and is string
  // or message.
  // Additionally, if any singular embedded messages have been allocated, we
  // need to delete them, UNLESS we are the prototype message of this type,
  // in which case any embedded messages are other prototypes and shouldn't
  // be touched.
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    if (InRealOneof(field)) {
      void* field_ptr = MutableOneofCaseRaw(field->containing_oneof()->index());
      if (*(reinterpret_cast<const int32_t*>(field_ptr)) == field->number()) {
        field_ptr = MutableOneofFieldRaw(field);
        if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
          switch (field->cpp_string_type()) {
            case FieldDescriptor::CppStringType::kCord:
              delete *reinterpret_cast<absl::Cord**>(field_ptr);
              break;
            case FieldDescriptor::CppStringType::kView:
            case FieldDescriptor::CppStringType::kString: {
              reinterpret_cast<ArenaStringPtr*>(field_ptr)->Destroy();
              break;
            }
          }
        } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
            delete *reinterpret_cast<Message**>(field_ptr);
        }
      }
      continue;
    }
    void* field_ptr = MutableRaw(i);

    if (field->is_repeated()) {
      switch (field->cpp_type()) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)                  \
  case FieldDescriptor::CPPTYPE_##UPPERCASE:               \
    reinterpret_cast<RepeatedField<LOWERCASE>*>(field_ptr) \
        ->~RepeatedField<LOWERCASE>();                     \
    break

        HANDLE_TYPE(INT32, int32_t);
        HANDLE_TYPE(INT64, int64_t);
        HANDLE_TYPE(UINT32, uint32_t);
        HANDLE_TYPE(UINT64, uint64_t);
        HANDLE_TYPE(DOUBLE, double);
        HANDLE_TYPE(FLOAT, float);
        HANDLE_TYPE(BOOL, bool);
        HANDLE_TYPE(ENUM, int);
#undef HANDLE_TYPE

        case FieldDescriptor::CPPTYPE_STRING:
          switch (field->cpp_string_type()) {
            case FieldDescriptor::CppStringType::kCord:
              reinterpret_cast<RepeatedField<absl::Cord>*>(field_ptr)
                  ->~RepeatedField<absl::Cord>();
              break;
            case FieldDescriptor::CppStringType::kView:
            case FieldDescriptor::CppStringType::kString:
              reinterpret_cast<RepeatedPtrField<std::string>*>(field_ptr)
                  ->~RepeatedPtrField<std::string>();
              break;
          }
          break;

        case FieldDescriptor::CPPTYPE_MESSAGE:
          if (IsMapFieldInApi(field)) {
            reinterpret_cast<DynamicMapField*>(field_ptr)->~DynamicMapField();
          } else {
            reinterpret_cast<RepeatedPtrField<Message>*>(field_ptr)
                ->~RepeatedPtrField<Message>();
          }
          break;
      }

    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
      switch (field->cpp_string_type()) {
        case FieldDescriptor::CppStringType::kCord:
          reinterpret_cast<absl::Cord*>(field_ptr)->~Cord();
          break;
        case FieldDescriptor::CppStringType::kView:
        case FieldDescriptor::CppStringType::kString: {
          reinterpret_cast<ArenaStringPtr*>(field_ptr)->Destroy();
          break;
        }
      }
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
          if (!is_prototype()) {
        Message* message = *reinterpret_cast<Message**>(field_ptr);
        if (message != nullptr) {
          delete message;
        }
      }
    }
  }
}

void* DynamicMessage::NewImpl(const void* prototype, void* mem, Arena* arena) {
  const auto* type_info =
      static_cast<const DynamicMessage*>(prototype)->type_info_;
  memset(mem, 0, type_info->class_data.allocation_size());
  return new (mem) DynamicMessage(type_info, arena);
}

void DynamicMessage::DestroyImpl(MessageLite& msg) {
  static_cast<DynamicMessage&>(msg).~DynamicMessage();
}

void DynamicMessage::CrossLinkPrototypes() {
  // This should only be called on the prototype message.
  ABSL_CHECK(is_prototype());

  DynamicMessageFactory* factory = type_info_->factory;
  const Descriptor* descriptor = type_info_->class_data.descriptor;

  // Cross-link default messages.
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
        !field->options().weak() && !InRealOneof(field) &&
        !field->is_repeated()) {
      void* field_ptr = MutableRaw(i);
      // For fields with message types, we need to cross-link with the
      // prototype for the field's type.
      // For singular fields, the field is just a pointer which should
      // point to the prototype.
      *reinterpret_cast<const Message**>(field_ptr) =
          factory->GetPrototypeNoLock(field->message_type());
    }
  }
}

const internal::ClassData* DynamicMessage::GetClassData() const {
  return type_info_->class_data.base();
}

// ===================================================================

DynamicMessageFactory::DynamicMessageFactory()
    : pool_(nullptr), delegate_to_generated_factory_(false) {}

DynamicMessageFactory::DynamicMessageFactory(const DescriptorPool* pool)
    : pool_(pool), delegate_to_generated_factory_(false) {}

DynamicMessageFactory::~DynamicMessageFactory() {
  for (auto iter = prototypes_.begin(); iter != prototypes_.end(); ++iter) {
    delete iter->second;
  }
}

const Message* DynamicMessageFactory::GetPrototype(const Descriptor* type) {
  ABSL_CHECK(type != nullptr);
  absl::MutexLock lock(&prototypes_mutex_);
  return GetPrototypeNoLock(type);
}

const Message* DynamicMessageFactory::GetPrototypeNoLock(
    const Descriptor* type) {
  if (delegate_to_generated_factory_ &&
      type->file()->pool() == DescriptorPool::generated_pool()) {
    const Message* result = MessageFactory::TryGetGeneratedPrototype(type);
    if (result != nullptr) return result;
    // Otherwise, we will create it dynamically so keep going.
  }

  const TypeInfo** target = &prototypes_[type];
  if (*target != nullptr) {
    // Already exists.
    return static_cast<const Message*>((*target)->class_data.prototype);
  }

  TypeInfo* type_info = new TypeInfo;
  *target = type_info;

  type_info->class_data.descriptor = type;
  type_info->class_data.is_dynamic = true;
  type_info->pool = (pool_ == nullptr) ? type->file()->pool() : pool_;
  type_info->factory = this;

  // We need to construct all the structures passed to Reflection's constructor.
  // This includes:
  // - A block of memory that contains space for all the message's fields.
  // - An array of integers indicating the byte offset of each field within
  //   this block.
  // - A big bitfield containing a bit for each field indicating whether
  //   or not that field is set.
  int real_oneof_count = type->real_oneof_decl_count();

  // Compute size and offsets.
  uint32_t* offsets = new uint32_t[type->field_count() + real_oneof_count];
  type_info->offsets.reset(offsets);

  // Decide all field offsets by packing in order.
  // We place the DynamicMessage object itself at the beginning of the allocated
  // space.
  int size = sizeof(DynamicMessage);
  size = AlignOffset(size);

  // Next the has_bits, which is an array of uint32s.
  type_info->has_bits_offset = -1;
  int max_hasbit = 0;
  for (int i = 0; i < type->field_count(); i++) {
    const FieldDescriptor* field = type->field(i);

    // If a field has hasbits, it could be either an explicit-presence or
    // implicit-presence field. Explicit presence fields will have "true
    // hasbits" where hasbit is set iff field is present. Implicit presence
    // fields will have "hint hasbits" where
    // - if hasbit is unset, field is not present.
    // - if hasbit is set, field is present if it is also nonempty.
    if (internal::cpp::HasHasbit(field)) {
      // TODO: b/112602698 - during Python textproto serialization, MapEntry
      // messages may be generated from DynamicMessage on the fly. C++
      // implementations of MapEntry messages always have hasbits, but
      // has_presence return values might be different depending on how field
      // presence is set. For MapEntrys, has_presence returns true for
      // explicit-presence (proto2) messages and returns false for
      // implicit-presence (proto3) messages.
      //
      // In the case of implicit presence, there is a potential inconsistency in
      // code behavior between C++ and Python:
      // - If C++ implementation is linked, hasbits are always generated for
      //   MapEntry messages, and MapEntry messages will behave like explicit
      //   presence.
      // - If C++ implementation is not linked, Python defaults to the
      //   DynamicMessage implementation for MapEntrys which traditionally does
      //   not assume the presence of hasbits, so the default Python behavior
      //   for MapEntry messages (by default C++ implementations are not linked)
      //   will fall back to the DynamicMessage implementation and behave like
      //   implicit presence.
      // This is an inconsistency and this if-condition preserves it.
      //
      // Longer term, we want to get rid of this additional if-check of
      // IsMapEntryField. It might take one or more breaking changes and more
      // consensus gathering & clarification though.
      if (!field->has_presence() && IsMapEntryField(field)) {
        continue;
      }

      if (type_info->has_bits_offset == -1) {
        // At least one field in the message requires a hasbit, so allocate
        // hasbits.
        type_info->has_bits_offset = size;
        uint32_t* has_bits_indices = new uint32_t[type->field_count()];
        for (int j = 0; j < type->field_count(); j++) {
          // Initialize to -1, fields that need a hasbit will overwrite.
          has_bits_indices[j] = static_cast<uint32_t>(-1);
        }
        type_info->has_bits_indices.reset(has_bits_indices);
      }
      type_info->has_bits_indices[i] = max_hasbit++;
    }
  }

  if (max_hasbit > 0) {
    int has_bits_array_size = DivideRoundingUp(max_hasbit, bitsizeof(uint32_t));
    size += has_bits_array_size * sizeof(uint32_t);
    size = AlignOffset(size);
  }

  // The oneof_case, if any. It is an array of uint32s.
  if (real_oneof_count > 0) {
    type_info->oneof_case_offset = size;
    size += real_oneof_count * sizeof(uint32_t);
    size = AlignOffset(size);
  }

  // The ExtensionSet, if any.
  if (type->extension_range_count() > 0) {
    type_info->extensions_offset = size;
    size += sizeof(ExtensionSet);
    size = AlignOffset(size);
  } else {
    // No extensions.
    type_info->extensions_offset = -1;
  }

  // All the fields.
  //
  // TODO:  Optimize the order of fields to minimize padding.
  for (int i = 0; i < type->field_count(); i++) {
    // Make sure field is aligned to avoid bus errors.
    // Oneof fields do not use any space.
    if (!InRealOneof(type->field(i))) {
      int field_size = FieldSpaceUsed(type->field(i));
      size = AlignTo(size, std::min(kSafeAlignment, field_size));
      offsets[i] = size;
      size += field_size;
    }
  }

  // The oneofs.
  for (int i = 0; i < type->real_oneof_decl_count(); i++) {
    size = AlignTo(size, kSafeAlignment);
    offsets[type->field_count() + i] = size;
    size += kMaxOneofUnionSize;
  }

  type_info->weak_field_map_offset = -1;

  type_info->class_data.message_creator =
      internal::MessageCreator(DynamicMessage::NewImpl, size, kSafeAlignment);

  // Construct the reflection object.

  // Compute the size of default oneof instance and offsets of default
  // oneof fields.
  for (int i = 0; i < type->real_oneof_decl_count(); i++) {
    for (int j = 0; j < type->real_oneof_decl(i)->field_count(); j++) {
      const FieldDescriptor* field = type->real_oneof_decl(i)->field(j);
      // oneof fields are not accessed through offsets, but we still have the
      // entry from a legacy implementation. This should be removed at some
      // point.
      // Mark the field to prevent unintentional access through reflection.
      // Don't use the top bit because that is for unused fields.
      offsets[field->index()] = internal::kInvalidFieldOffsetTag;
    }
  }

  // Allocate the prototype fields.
  void* base = operator new(size);
  memset(base, 0, size);

  // We have already locked the factory so we should not lock in the constructor
  // of dynamic message to avoid dead lock.
  DynamicMessage* prototype = new (base) DynamicMessage(type_info, false);

  internal::ReflectionSchema schema = {
      static_cast<const Message*>(type_info->class_data.prototype),
      type_info->offsets.get(),
      type_info->has_bits_indices.get(),
      type_info->has_bits_offset,
      PROTOBUF_FIELD_OFFSET(DynamicMessage, _internal_metadata_),
      type_info->extensions_offset,
      type_info->oneof_case_offset,
      static_cast<int>(type_info->class_data.allocation_size()),
      type_info->weak_field_map_offset,
      nullptr,  // inlined_string_indices_
      0,        // inlined_string_donated_offset_
      -1,       // split_offset_
      -1,       // sizeof_split_
  };

  type_info->class_data.reflection = new Reflection(
      type_info->class_data.descriptor, schema, type_info->pool, this);

  // Cross link prototypes.
  prototype->CrossLinkPrototypes();

  return prototype;
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"  // NOLINT
