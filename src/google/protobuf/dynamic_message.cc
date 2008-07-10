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
// Message::Reflection, so the rest is easy.
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

#define bitsizeof(T) (sizeof(T) * 8)

}  // namespace

// ===================================================================

class DynamicMessage : public Message {
 public:
  DynamicMessage(const Descriptor* descriptor,
                 uint8* base, const uint8* prototype_base,
                 int size, const int offsets[],
                 const DescriptorPool* pool, DynamicMessageFactory* factory);
  ~DynamicMessage();

  // Called on the prototype after construction to initialize message fields.
  void CrossLinkPrototypes(DynamicMessageFactory* factory);

  // implements Message ----------------------------------------------

  Message* New() const;

  int GetCachedSize() const;
  void SetCachedSize(int size) const;

  const Descriptor* GetDescriptor() const;
  const Reflection* GetReflection() const;
  Reflection* GetReflection();

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(DynamicMessage);

  inline bool is_prototype() { return base_ == prototype_base_; }

  const Descriptor* descriptor_;
  const DescriptorPool* descriptor_pool_;
  DynamicMessageFactory* factory_;
  scoped_ptr<ExtensionSet> extensions_;
  GeneratedMessageReflection reflection_;
  uint8* base_;
  const uint8* prototype_base_;
  const int* offsets_;
  int size_;

  // TODO(kenton):  Make this an atomic<int> when C++ supports it.
  mutable int cached_byte_size_;
};

DynamicMessage::DynamicMessage(const Descriptor* descriptor,
                               uint8* base, const uint8* prototype_base,
                               int size, const int offsets[],
                               const DescriptorPool* pool,
                               DynamicMessageFactory* factory)
  : descriptor_(descriptor),
    descriptor_pool_((pool == NULL) ? descriptor->file()->pool() : pool),
    factory_(factory),
    extensions_(descriptor->extension_range_count() > 0 ?
                new ExtensionSet(descriptor, descriptor_pool_, factory_) :
                NULL),
    reflection_(descriptor, base, prototype_base, offsets,
                // has_bits
                reinterpret_cast<uint32*>(base + size) -
                DivideRoundingUp(descriptor->field_count(), bitsizeof(uint32)),
                extensions_.get()),
    base_(base),
    prototype_base_(prototype_base),
    offsets_(offsets),
    size_(size),
    cached_byte_size_(0) {
  // We need to call constructors for various fields manually and set
  // default values where appropriate.  We use placement new to call
  // constructors.  If you haven't heard of placement new, I suggest Googling
  // it now.  We use placement new even for primitive types that don't have
  // constructors for consistency.  (In theory, placement new should be used
  // any time you are trying to convert untyped memory to typed memory, though
  // in practice that's not strictly necessary for types that don't have a
  // constructor.)
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    void* field_ptr = base + offsets[i];
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
                  prototype_base + offsets[i]);
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
                prototype_base + offsets[i]);
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
  // We need to manually run the destructors for repeated fields and strings,
  // just as we ran their constructors in the the DynamicMessage constructor.
  // Additionally, if any singular embedded messages have been allocated, we
  // need to delete them, UNLESS we are the prototype message of this type,
  // in which case any embedded messages are other prototypes and shouldn't
  // be touched.
  const Descriptor* descriptor = GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    void* field_ptr = base_ + offsets_[i];

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

  // OK, now we can delete our base pointer.
  operator delete(base_);

  // When the prototype is deleted, we also want to free the offsets table.
  // (The prototype is only deleted when the factory that created it is
  // deleted.)
  if (is_prototype()) {
    delete [] offsets_;
  }
}

void DynamicMessage::CrossLinkPrototypes(DynamicMessageFactory* factory) {
  // This should only be called on the prototype message.
  GOOGLE_CHECK(is_prototype());

  // Cross-link default messages.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    void* field_ptr = base_ + offsets_[i];

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
  uint8* new_base = reinterpret_cast<uint8*>(operator new(size_));
  memset(new_base, 0, size_);

  return new DynamicMessage(GetDescriptor(), new_base, prototype_base_,
                            size_, offsets_, descriptor_pool_, factory_);
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
  return descriptor_;
}

const Message::Reflection* DynamicMessage::GetReflection() const {
  return &reflection_;
}

Message::Reflection* DynamicMessage::GetReflection() {
  return &reflection_;
}

// ===================================================================

struct DynamicMessageFactory::PrototypeMap {
  typedef hash_map<const Descriptor*, const Message*> Map;
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
  const Message** target = &prototypes_->map_[type];
  if (*target != NULL) {
    // Already exists.
    return *target;
  }

  // We need to construct all the structures passed to
  // GeneratedMessageReflection's constructor.  This includes:
  // - A block of memory that contains space for all the message's fields.
  // - An array of integers indicating the byte offset of each field within
  //   this block.
  // - A big bitfield containing a bit for each field indicating whether
  //   or not that field is set.

  // Compute size and offsets.
  int* offsets = new int[type->field_count()];

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
  int current_offset = 0;

  for (int i = 0; i < type->field_count(); i++) {
    offsets[ordered_fields[i]->index()] = current_offset;
    current_offset += FieldSpaceUsed(ordered_fields[i]);
  }

  // Allocate space for all fields plus has_bits.  We'll stick has_bits on
  // the end.
  int size = current_offset +
    DivideRoundingUp(type->field_count(), bitsizeof(uint32)) * sizeof(uint32);

  // Round size up to the nearest 64-bit boundary just to make sure no
  // clever allocators think that alignment is not necessary.  This also
  // insures that has_bits is properly-aligned, since we'll always align
  // has_bits with the end of the structure.
  size = DivideRoundingUp(size, sizeof(uint64)) * sizeof(uint64);
  uint8* base = reinterpret_cast<uint8*>(operator new(size));
  memset(base, 0, size);

  // Construct message.
  DynamicMessage* result =
    new DynamicMessage(type, base, base, size, offsets, pool_, this);
  *target = result;
  result->CrossLinkPrototypes(this);

  return result;
}

}  // namespace protobuf

}  // namespace google
