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
#include <memory>
#include <new>

#include "google/protobuf/arenastring.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor_legacy.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/map_field.h"
#include "google/protobuf/map_field_inl.h"
#include "google/protobuf/map_type_handler.h"
#include "google/protobuf/reflection_ops.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/unknown_field_set.h"
#include "google/protobuf/wire_format.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

using internal::DynamicMapField;
using internal::ExtensionSet;
using internal::MapField;


using internal::ArenaStringPtr;

// ===================================================================
// Some helper tables and functions...

namespace {

bool IsMapFieldInApi(const FieldDescriptor* field) { return field->is_map(); }


inline bool InRealOneof(const FieldDescriptor* field) {
  return field->containing_oneof() &&
         !OneofDescriptorLegacy(field->containing_oneof()).is_synthetic();
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
  explicit DynamicMessage(const DynamicMessageFactory::TypeInfo* type_info);

  // This should only be used by GetPrototypeNoLock() to avoid dead lock.
  DynamicMessage(DynamicMessageFactory::TypeInfo* type_info, bool lock_factory);
  DynamicMessage(const DynamicMessage&) = delete;
  DynamicMessage& operator=(const DynamicMessage&) = delete;

  ~DynamicMessage() override;

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

  Message* New(Arena* arena) const override;

  internal::CachedSize* AccessCachedSize() const final {
    return &cached_byte_size_;
  }

  Metadata GetMetadata() const override;

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

  void* MutableRaw(int i);
  void* MutableExtensionsRaw();
  void* MutableWeakFieldMapRaw();
  void* MutableOneofCaseRaw(int i);
  void* MutableOneofFieldRaw(const FieldDescriptor* f);

  const DynamicMessageFactory::TypeInfo* type_info_;
  mutable internal::CachedSize cached_byte_size_;
};

struct DynamicMessageFactory::TypeInfo {
  int size;
  int has_bits_offset;
  int oneof_case_offset;
  int extensions_offset;

  // Not owned by the TypeInfo.
  DynamicMessageFactory* factory;  // The factory that created this object.
  const DescriptorPool* pool;      // The factory's DescriptorPool.
  const Descriptor* type;          // Type of this DynamicMessage.

  // Warning:  The order in which the following pointers are defined is
  //   important (the prototype must be deleted *before* the offsets).
  std::unique_ptr<uint32_t[]> offsets;
  std::unique_ptr<uint32_t[]> has_bits_indices;
  std::unique_ptr<const Reflection> reflection;
  // Don't use a unique_ptr to hold the prototype: the destructor for
  // DynamicMessage needs to know whether it is the prototype, and does so by
  // looking back at this field. This would assume details about the
  // implementation of unique_ptr.
  const DynamicMessage* prototype;
  int weak_field_map_offset;  // The offset for the weak_field_map;

  TypeInfo() : prototype(nullptr) {}

  ~TypeInfo() {
    delete prototype;

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

DynamicMessage::DynamicMessage(const DynamicMessageFactory::TypeInfo* type_info)
    : type_info_(type_info), cached_byte_size_(0) {
  SharedCtor(true);
}

DynamicMessage::DynamicMessage(const DynamicMessageFactory::TypeInfo* type_info,
                               Arena* arena)
    : Message(arena), type_info_(type_info), cached_byte_size_(0) {
  SharedCtor(true);
}

DynamicMessage::DynamicMessage(DynamicMessageFactory::TypeInfo* type_info,
                               bool lock_factory)
    : type_info_(type_info), cached_byte_size_(0) {
  // The prototype in type_info has to be set before creating the prototype
  // instance on memory. e.g., message Foo { map<int32_t, Foo> a = 1; }. When
  // creating prototype for Foo, prototype of the map entry will also be
  // created, which needs the address of the prototype of Foo (the value in
  // map). To break the cyclic dependency, we have to assign the address of
  // prototype into type_info first.
  type_info->prototype = this;
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
  return OffsetToPointer(type_info_->offsets[type_info_->type->field_count() +
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

  const Descriptor* descriptor = type_info_->type;
  Arena* arena = GetArena();
  // Initialize oneof cases.
  int oneof_count = 0;
  for (int i = 0; i < descriptor->oneof_decl_count(); ++i) {
    if (OneofDescriptorLegacy(descriptor->oneof_decl(i)).is_synthetic())
      continue;
    new (MutableOneofCaseRaw(oneof_count++)) uint32_t{0};
  }

  if (type_info_->extensions_offset != -1) {
    new (MutableExtensionsRaw()) ExtensionSet(GetArena());
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
      new (field_ptr) RepeatedField<TYPE>(GetArena());     \
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
          new (field_ptr) RepeatedField<int>(GetArena());
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
              new (field_ptr) RepeatedPtrField<std::string>(GetArena());
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
              if (GetArena() != nullptr) {
                new (field_ptr) DynamicMapField(
                    type_info_->factory->GetPrototype(field->message_type()),
                    GetArena());
              } else {
                new (field_ptr) DynamicMapField(
                    type_info_->factory->GetPrototype(field->message_type()));
              }
            } else {
              if (GetArena() != nullptr) {
                new (field_ptr)
                    DynamicMapField(type_info_->factory->GetPrototypeNoLock(
                                        field->message_type()),
                                    GetArena());
              } else {
                new (field_ptr)
                    DynamicMapField(type_info_->factory->GetPrototypeNoLock(
                        field->message_type()));
              }
            }
          } else {
            new (field_ptr) RepeatedPtrField<Message>(GetArena());
          }
        }
        break;
      }
    }
  }
}

bool DynamicMessage::is_prototype() const {
  return type_info_->prototype == this ||
         // If type_info_->prototype is nullptr, then we must be constructing
         // the prototype now, which means we must be the prototype.
         type_info_->prototype == nullptr;
}

#if defined(__cpp_lib_destroying_delete) && defined(__cpp_sized_deallocation)
void DynamicMessage::operator delete(DynamicMessage* msg,
                                     std::destroying_delete_t) {
  const size_t size = msg->type_info_->size;
  msg->~DynamicMessage();
  ::operator delete(msg, size);
}
#endif

DynamicMessage::~DynamicMessage() {
  const Descriptor* descriptor = type_info_->type;

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

void DynamicMessage::CrossLinkPrototypes() {
  // This should only be called on the prototype message.
  ABSL_CHECK(is_prototype());

  DynamicMessageFactory* factory = type_info_->factory;
  const Descriptor* descriptor = type_info_->type;

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

Message* DynamicMessage::New(Arena* arena) const {
  if (arena != nullptr) {
    void* new_base = Arena::CreateArray<char>(arena, type_info_->size);
    memset(new_base, 0, type_info_->size);
    return new (new_base) DynamicMessage(type_info_, arena);
  } else {
    void* new_base = operator new(type_info_->size);
    memset(new_base, 0, type_info_->size);
    return new (new_base) DynamicMessage(type_info_);
  }
}

Metadata DynamicMessage::GetMetadata() const {
  Metadata metadata;
  metadata.descriptor = type_info_->type;
  metadata.reflection = type_info_->reflection.get();
  return metadata;
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
  absl::MutexLock lock(&prototypes_mutex_);
  return GetPrototypeNoLock(type);
}

const Message* DynamicMessageFactory::GetPrototypeNoLock(
    const Descriptor* type) {
  if (delegate_to_generated_factory_ &&
      type->file()->pool() == DescriptorPool::generated_pool()) {
    return MessageFactory::generated_factory()->GetPrototype(type);
  }

  const TypeInfo** target = &prototypes_[type];
  if (*target != nullptr) {
    // Already exists.
    return (*target)->prototype;
  }

  TypeInfo* type_info = new TypeInfo;
  *target = type_info;

  type_info->type = type;
  type_info->pool = (pool_ == nullptr) ? type->file()->pool() : pool_;
  type_info->factory = this;

  // We need to construct all the structures passed to Reflection's constructor.
  // This includes:
  // - A block of memory that contains space for all the message's fields.
  // - An array of integers indicating the byte offset of each field within
  //   this block.
  // - A big bitfield containing a bit for each field indicating whether
  //   or not that field is set.
  int real_oneof_count = 0;
  for (int i = 0; i < type->oneof_decl_count(); i++) {
    if (!OneofDescriptorLegacy(type->oneof_decl(i)).is_synthetic()) {
      real_oneof_count++;
    }
  }

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
    if (internal::cpp::HasHasbit(type->field(i))) {
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
  for (int i = 0; i < type->oneof_decl_count(); i++) {
    if (!OneofDescriptorLegacy(type->oneof_decl(i)).is_synthetic()) {
      size = AlignTo(size, kSafeAlignment);
      offsets[type->field_count() + i] = size;
      size += kMaxOneofUnionSize;
    }
  }

  type_info->weak_field_map_offset = -1;

  // Align the final size to make sure no clever allocators think that
  // alignment is not necessary.
  type_info->size = size;

  // Construct the reflection object.

  // Compute the size of default oneof instance and offsets of default
  // oneof fields.
  for (int i = 0; i < type->oneof_decl_count(); i++) {
    if (OneofDescriptorLegacy(type->oneof_decl(i)).is_synthetic()) continue;
    for (int j = 0; j < type->oneof_decl(i)->field_count(); j++) {
      const FieldDescriptor* field = type->oneof_decl(i)->field(j);
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
      type_info->prototype,
      type_info->offsets.get(),
      type_info->has_bits_indices.get(),
      type_info->has_bits_offset,
      PROTOBUF_FIELD_OFFSET(DynamicMessage, _internal_metadata_),
      type_info->extensions_offset,
      type_info->oneof_case_offset,
      type_info->size,
      type_info->weak_field_map_offset,
      nullptr,  // inlined_string_indices_
      0,        // inlined_string_donated_offset_
      -1,       // split_offset_
      -1,       // sizeof_split_
  };

  type_info->reflection.reset(
      new Reflection(type_info->type, schema, type_info->pool, this));

  // Cross link prototypes.
  prototype->CrossLinkPrototypes();

  return prototype;
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"  // NOLINT
