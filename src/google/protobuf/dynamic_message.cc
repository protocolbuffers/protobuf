// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// DynamicMessage is implemented by constructing a data structure which
// has roughly the same memory layout as a generated message would have.
// Then, we use GeneratedMessageReflection to implement our reflection
// interface.  All the other operations we need to implement (e.g.
// parsing, copying, etc.) are already implemented in terms of
// Reflection, so the rest is easy.
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
// "new uint8[]".  This is because "operator new()" means "Give me some
// space which I can use as I please." while "new uint8[]" means "Give
// me an array of 8-bit integers.".  In practice, the later may return
// a pointer that is not aligned correctly for general use.  I believe
// Item 8 of "More Effective C++" discusses this in more detail, though
// I don't have the book on me right now so I'm not sure.

#include <algorithm>
#include <google/protobuf/stubs/hash.h>

#include <google/protobuf/stubs/common.h>

#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format.h>

namespace google {
namespace protobuf {

using internal::WireFormat;
using internal::ExtensionSet;
using internal::GeneratedMessageReflection;
using internal::GenericRepeatedField;


// ===================================================================
// Some helper tables and functions...

namespace {

// Compute the byte size of the in-memory representation of the field.
int FieldSpaceUsed(const FieldDescriptor* field) {
  typedef FieldDescriptor FD;  // avoid line wrapping
  if (field->label() == FD::LABEL_REPEATED) {
    switch (field->cpp_type()) {
      case FD::CPPTYPE_INT32  : return sizeof(RepeatedField<int32   >);
      case FD::CPPTYPE_INT64  : return sizeof(RepeatedField<int64   >);
      case FD::CPPTYPE_UINT32 : return sizeof(RepeatedField<uint32  >);
      case FD::CPPTYPE_UINT64 : return sizeof(RepeatedField<uint64  >);
      case FD::CPPTYPE_DOUBLE : return sizeof(RepeatedField<double  >);
      case FD::CPPTYPE_FLOAT  : return sizeof(RepeatedField<float   >);
      case FD::CPPTYPE_BOOL   : return sizeof(RepeatedField<bool    >);
      case FD::CPPTYPE_ENUM   : return sizeof(RepeatedField<int     >);
      case FD::CPPTYPE_MESSAGE: return sizeof(RepeatedPtrField<Message>);

      case FD::CPPTYPE_STRING:
          return sizeof(RepeatedPtrField<string>);
        break;
    }
  } else {
    switch (field->cpp_type()) {
      case FD::CPPTYPE_INT32  : return sizeof(int32   );
      case FD::CPPTYPE_INT64  : return sizeof(int64   );
      case FD::CPPTYPE_UINT32 : return sizeof(uint32  );
      case FD::CPPTYPE_UINT64 : return sizeof(uint64  );
      case FD::CPPTYPE_DOUBLE : return sizeof(double  );
      case FD::CPPTYPE_FLOAT  : return sizeof(float   );
      case FD::CPPTYPE_BOOL   : return sizeof(bool    );
      case FD::CPPTYPE_ENUM   : return sizeof(int     );
      case FD::CPPTYPE_MESSAGE: return sizeof(Message*);

      case FD::CPPTYPE_STRING:
          return sizeof(string*);
        break;
    }
  }

  GOOGLE_LOG(DFATAL) << "Can't get here.";
  return 0;
}

struct DescendingFieldSizeOrder {
  inline bool operator()(const FieldDescriptor* a,
                         const FieldDescriptor* b) {
    // All repeated fields come first.
    if (a->is_repeated()) {
      if (b->is_repeated()) {
        // Repeated fields and are not ordered with respect to each other.
        return false;
      } else {
        return true;
      }
    } else if (b->is_repeated()) {
      return false;
    } else {
      // Remaining fields in descending order by size.
      return FieldSpaceUsed(a) > FieldSpaceUsed(b);
    }
  }
};

inline int DivideRoundingUp(int i, int j) {
  return (i + (j - 1)) / j;
}

static const int kSafeAlignment = sizeof(uint64);

// Rounds the given byte offset up to the next offset aligned such that any
// type may be stored at it.
inline int AlignOffset(int offset) {
  return DivideRoundingUp(offset, kSafeAlignment) * kSafeAlignment;
}

#define bitsizeof(T) (sizeof(T) * 8)

}  // namespace

// ===================================================================

class DynamicMessage : public Message {
 public:
  struct TypeInfo {
    int size;
    int has_bits_offset;
    int unknown_fields_offset;
    int extensions_offset;

    // Not owned by the TypeInfo.
    DynamicMessageFactory* factory;  // The factory that created this object.
    const DescriptorPool* pool;      // The factory's DescriptorPool.
    const Descriptor* type;          // Type of this DynamicMessage.

    // Warning:  The order in which the following pointers are defined is
    //   important (the prototype must be deleted *before* the offsets).
    scoped_array<int> offsets;
    scoped_ptr<const GeneratedMessageReflection> reflection;
    scoped_ptr<const DynamicMessage> prototype;
  };

  DynamicMessage(const TypeInfo* type_info);
  ~DynamicMessage();

  // Called on the prototype after construction to initialize message fields.
  void CrossLinkPrototypes();

  // implements Message ----------------------------------------------

  Message* New() const;

  int GetCachedSize() const;
  void SetCachedSize(int size) const;

  const Descriptor* GetDescriptor() const;
  const Reflection* GetReflection() const;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(DynamicMessage);

  inline bool is_prototype() const {
    return type_info_->prototype == this ||
           // If type_info_->prototype is NULL, then we must be constructing
           // the prototype now, which means we must be the prototype.
           type_info_->prototype == NULL;
  }

  inline void* OffsetToPointer(int offset) {
    return reinterpret_cast<uint8*>(this) + offset;
  }
  inline const void* OffsetToPointer(int offset) const {
    return reinterpret_cast<const uint8*>(this) + offset;
  }

  const TypeInfo* type_info_;

  // TODO(kenton):  Make this an atomic<int> when C++ supports it.
  mutable int cached_byte_size_;
};

DynamicMessage::DynamicMessage(const TypeInfo* type_info)
  : type_info_(type_info),
    cached_byte_size_(0) {
  // We need to call constructors for various fields manually and set
  // default values where appropriate.  We use placement new to call
  // constructors.  If you haven't heard of placement new, I suggest Googling
  // it now.  We use placement new even for primitive types that don't have
  // constructors for consistency.  (In theory, placement new should be used
  // any time you are trying to convert untyped memory to typed memory, though
  // in practice that's not strictly necessary for types that don't have a
  // constructor.)

  const Descriptor* descriptor = type_info_->type;

  new(OffsetToPointer(type_info_->unknown_fields_offset)) UnknownFieldSet;

  if (type_info_->extensions_offset != -1) {
    new(OffsetToPointer(type_info_->extensions_offset))
      ExtensionSet(&type_info_->type, type_info_->pool, type_info_->factory);
  }

  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    void* field_ptr = OffsetToPointer(type_info_->offsets[i]);
    switch (field->cpp_type()) {
#define HANDLE_TYPE(CPPTYPE, TYPE)                                           \
      case FieldDescriptor::CPPTYPE_##CPPTYPE:                               \
        if (!field->is_repeated()) {                                         \
          new(field_ptr) TYPE(field->default_value_##TYPE());                \
        } else {                                                             \
          new(field_ptr) RepeatedField<TYPE>();                              \
        }                                                                    \
        break;

      HANDLE_TYPE(INT32 , int32 );
      HANDLE_TYPE(INT64 , int64 );
      HANDLE_TYPE(UINT32, uint32);
      HANDLE_TYPE(UINT64, uint64);
      HANDLE_TYPE(DOUBLE, double);
      HANDLE_TYPE(FLOAT , float );
      HANDLE_TYPE(BOOL  , bool  );
#undef HANDLE_TYPE

      case FieldDescriptor::CPPTYPE_ENUM:
        if (!field->is_repeated()) {
          new(field_ptr) int(field->default_value_enum()->number());
        } else {
          new(field_ptr) RepeatedField<int>();
        }
        break;

      case FieldDescriptor::CPPTYPE_STRING:
          if (!field->is_repeated()) {
            if (is_prototype()) {
              new(field_ptr) const string*(&field->default_value_string());
            } else {
              string* default_value =
                *reinterpret_cast<string* const*>(
                  type_info_->prototype->OffsetToPointer(
                    type_info_->offsets[i]));
              new(field_ptr) string*(default_value);
            }
          } else {
            new(field_ptr) RepeatedPtrField<string>();
          }
        break;

      case FieldDescriptor::CPPTYPE_MESSAGE: {
        // If this object is the prototype, its CPPTYPE_MESSAGE fields
        // must be initialized later, in CrossLinkPrototypes(), so we don't
        // initialize them here.
        if (!is_prototype()) {
          if (!field->is_repeated()) {
            new(field_ptr) Message*(NULL);
          } else {
            const RepeatedPtrField<Message>* prototype_field =
              reinterpret_cast<const RepeatedPtrField<Message>*>(
                type_info_->prototype->OffsetToPointer(
                  type_info_->offsets[i]));
            new(field_ptr) RepeatedPtrField<Message>(
              prototype_field->prototype());
          }
        }
        break;
      }
    }
  }
}

DynamicMessage::~DynamicMessage() {
  const Descriptor* descriptor = type_info_->type;

  reinterpret_cast<UnknownFieldSet*>(
    OffsetToPointer(type_info_->unknown_fields_offset))->~UnknownFieldSet();

  if (type_info_->extensions_offset != -1) {
    reinterpret_cast<ExtensionSet*>(
      OffsetToPointer(type_info_->extensions_offset))->~ExtensionSet();
  }

  // We need to manually run the destructors for repeated fields and strings,
  // just as we ran their constructors in the the DynamicMessage constructor.
  // Additionally, if any singular embedded messages have been allocated, we
  // need to delete them, UNLESS we are the prototype message of this type,
  // in which case any embedded messages are other prototypes and shouldn't
  // be touched.
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    void* field_ptr = OffsetToPointer(type_info_->offsets[i]);

    if (field->is_repeated()) {
      GenericRepeatedField* field =
        reinterpret_cast<GenericRepeatedField*>(field_ptr);
      field->~GenericRepeatedField();

    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
        string* ptr = *reinterpret_cast<string**>(field_ptr);
        if (ptr != &field->default_value_string()) {
          delete ptr;
        }
    } else if ((field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) &&
               !is_prototype()) {
      Message* message = *reinterpret_cast<Message**>(field_ptr);
      if (message != NULL) {
        delete message;
      }
    }
  }
}

void DynamicMessage::CrossLinkPrototypes() {
  // This should only be called on the prototype message.
  GOOGLE_CHECK(is_prototype());

  DynamicMessageFactory* factory = type_info_->factory;
  const Descriptor* descriptor = type_info_->type;

  // Cross-link default messages.
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    void* field_ptr = OffsetToPointer(type_info_->offsets[i]);

    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      // For fields with message types, we need to cross-link with the
      // prototype for the field's type.
      const Message* field_prototype =
        factory->GetPrototype(field->message_type());

      if (field->is_repeated()) {
        // For repeated fields, we actually construct the RepeatedPtrField
        // here, but only for fields with message types.  All other repeated
        // fields are constructed in DynamicMessage's constructor.
        new(field_ptr) RepeatedPtrField<Message>(field_prototype);
      } else {
        // For singular fields, the field is just a pointer which should
        // point to the prototype.  (OK to const_cast here because the
        // prototype itself will only be available const to the outside
        // world.)
        new(field_ptr) Message*(const_cast<Message*>(field_prototype));
      }
    }
  }
}

Message* DynamicMessage::New() const {
  void* new_base = reinterpret_cast<uint8*>(operator new(type_info_->size));
  memset(new_base, 0, type_info_->size);
  return new(new_base) DynamicMessage(type_info_);
}

int DynamicMessage::GetCachedSize() const {
  return cached_byte_size_;
}

void DynamicMessage::SetCachedSize(int size) const {
  // This is theoretically not thread-compatible, but in practice it works
  // because if multiple threads write this simultaneously, they will be
  // writing the exact same value.
  cached_byte_size_ = size;
}

const Descriptor* DynamicMessage::GetDescriptor() const {
  return type_info_->type;
}

const Reflection* DynamicMessage::GetReflection() const {
  return type_info_->reflection.get();
}

// ===================================================================

struct DynamicMessageFactory::PrototypeMap {
  typedef hash_map<const Descriptor*, const DynamicMessage::TypeInfo*> Map;
  Map map_;
};

DynamicMessageFactory::DynamicMessageFactory()
  : pool_(NULL), prototypes_(new PrototypeMap) {
}

DynamicMessageFactory::DynamicMessageFactory(const DescriptorPool* pool)
  : pool_(pool), prototypes_(new PrototypeMap) {
}

DynamicMessageFactory::~DynamicMessageFactory() {
  for (PrototypeMap::Map::iterator iter = prototypes_->map_.begin();
       iter != prototypes_->map_.end(); ++iter) {
    delete iter->second;
  }
}


const Message* DynamicMessageFactory::GetPrototype(const Descriptor* type) {
  const DynamicMessage::TypeInfo** target = &prototypes_->map_[type];
  if (*target != NULL) {
    // Already exists.
    return (*target)->prototype.get();
  }

  DynamicMessage::TypeInfo* type_info = new DynamicMessage::TypeInfo;
  *target = type_info;

  type_info->type = type;
  type_info->pool = (pool_ == NULL) ? type->file()->pool() : pool_;
  type_info->factory = this;

  // We need to construct all the structures passed to
  // GeneratedMessageReflection's constructor.  This includes:
  // - A block of memory that contains space for all the message's fields.
  // - An array of integers indicating the byte offset of each field within
  //   this block.
  // - A big bitfield containing a bit for each field indicating whether
  //   or not that field is set.

  // Compute size and offsets.
  int* offsets = new int[type->field_count()];
  type_info->offsets.reset(offsets);

  // Sort the fields of this message in descending order by size.  We
  // assume that if we then pack the fields tightly in this order, all fields
  // will end up properly-aligned, since all field sizes are powers of two or
  // are multiples of the system word size.
  scoped_array<const FieldDescriptor*> ordered_fields(
    new const FieldDescriptor*[type->field_count()]);
  for (int i = 0; i < type->field_count(); i++) {
    ordered_fields[i] = type->field(i);
  }
  stable_sort(&ordered_fields[0], &ordered_fields[type->field_count()],
              DescendingFieldSizeOrder());

  // Decide all field offsets by packing in order.
  // We place the DynamicMessage object itself at the beginning of the allocated
  // space.
  int size = sizeof(DynamicMessage);
  size = AlignOffset(size);

  // Next the has_bits, which is an array of uint32s.
  type_info->has_bits_offset = size;
  int has_bits_array_size =
    DivideRoundingUp(type->field_count(), bitsizeof(uint32));
  size += has_bits_array_size * sizeof(uint32);
  size = AlignOffset(size);

  // The ExtensionSet, if any.
  if (type->extension_range_count() > 0) {
    type_info->extensions_offset = size;
    size += sizeof(ExtensionSet);
    size = AlignOffset(size);
  } else {
    // No extensions.
    type_info->extensions_offset = -1;
  }

  // All the fields.  We don't need to align after each field because they are
  // sorted in descending size order, and the size of a type is always a
  // multiple of its alignment.
  for (int i = 0; i < type->field_count(); i++) {
    offsets[ordered_fields[i]->index()] = size;
    size += FieldSpaceUsed(ordered_fields[i]);
  }

  // Add the UnknownFieldSet to the end.
  size = AlignOffset(size);
  type_info->unknown_fields_offset = size;
  size += sizeof(UnknownFieldSet);

  // Align the final size to make sure no clever allocators think that
  // alignment is not necessary.
  size = AlignOffset(size);
  type_info->size = size;

  // Allocate the prototype.
  void* base = operator new(size);
  memset(base, 0, size);
  DynamicMessage* prototype = new(base) DynamicMessage(type_info);
  type_info->prototype.reset(prototype);

  // Construct the reflection object.
  type_info->reflection.reset(
    new GeneratedMessageReflection(
      type_info->type,
      type_info->prototype.get(),
      type_info->offsets.get(),
      type_info->has_bits_offset,
      type_info->unknown_fields_offset,
      type_info->extensions_offset,
      type_info->pool));

  // Cross link prototypes.
  prototype->CrossLinkPrototypes();

  return prototype;
}

}  // namespace protobuf
}  // namespace google
