// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// This header is logically internal, but is made public because it is used
// from protocol-compiler-generated code, which may reside in other components.

#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_REFLECTION_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_REFLECTION_H__

#include <string>
#include <vector>
#include <google/protobuf/stubs/casts.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_enum_reflection.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/port.h>
#include <google/protobuf/unknown_field_set.h>


#include <google/protobuf/port_def.inc>

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {
class MapKey;
class MapValueRef;
class MessageLayoutInspector;
class Message;
struct Metadata;
}  // namespace protobuf
}  // namespace google

namespace google {
namespace protobuf {
namespace internal {
class DefaultEmptyOneof;
// Defined in other files.
class ExtensionSet;  // extension_set.h
class WeakFieldMap;  // weak_field_map.h

// This struct describes the internal layout of the message, hence this is
// used to act on the message reflectively.
//   default_instance:  The default instance of the message.  This is only
//                  used to obtain pointers to default instances of embedded
//                  messages, which GetMessage() will return if the particular
//                  sub-message has not been initialized yet.  (Thus, all
//                  embedded message fields *must* have non-null pointers
//                  in the default instance.)
//   offsets:       An array of ints giving the byte offsets.
//                  For each oneof or weak field, the offset is relative to the
//                  default_instance. These can be computed at compile time
//                  using the
//                  PROTO2_GENERATED_DEFAULT_ONEOF_FIELD_OFFSET()
//                  macro. For each none oneof field, the offset is related to
//                  the start of the message object.  These can be computed at
//                  compile time using the
//                  PROTO2_GENERATED_MESSAGE_FIELD_OFFSET() macro.
//                  Besides offsets for all fields, this array also contains
//                  offsets for oneof unions. The offset of the i-th oneof union
//                  is offsets[descriptor->field_count() + i].
//   has_bit_indices:  Mapping from field indexes to their index in the has
//                  bit array.
//   has_bits_offset:  Offset in the message of an array of uint32s of size
//                  descriptor->field_count()/32, rounded up.  This is a
//                  bitfield where each bit indicates whether or not the
//                  corresponding field of the message has been initialized.
//                  The bit for field index i is obtained by the expression:
//                    has_bits[i / 32] & (1 << (i % 32))
//   unknown_fields_offset:  Offset in the message of the UnknownFieldSet for
//                  the message.
//   extensions_offset:  Offset in the message of the ExtensionSet for the
//                  message, or -1 if the message type has no extension
//                  ranges.
//   oneof_case_offset:  Offset in the message of an array of uint32s of
//                  size descriptor->oneof_decl_count().  Each uint32
//                  indicates what field is set for each oneof.
//   object_size:   The size of a message object of this type, as measured
//                  by sizeof().
//   arena_offset:  If a message doesn't have a unknown_field_set that stores
//                  the arena, it must have a direct pointer to the arena.
//   weak_field_map_offset: If the message proto has weak fields, this is the
//                  offset of _weak_field_map_ in the generated proto. Otherwise
//                  -1.
struct ReflectionSchema {
 public:
  // Size of a google::protobuf::Message object of this type.
  uint32 GetObjectSize() const { return static_cast<uint32>(object_size_); }

  bool InRealOneof(const FieldDescriptor* field) const {
    return field->containing_oneof() &&
           !field->containing_oneof()->is_synthetic();
  }

  // Offset of a non-oneof field.  Getting a field offset is slightly more
  // efficient when we know statically that it is not a oneof field.
  uint32 GetFieldOffsetNonOneof(const FieldDescriptor* field) const {
    GOOGLE_DCHECK(!InRealOneof(field));
    return OffsetValue(offsets_[field->index()], field->type());
  }

  // Offset of any field.
  uint32 GetFieldOffset(const FieldDescriptor* field) const {
    if (InRealOneof(field)) {
      size_t offset =
          static_cast<size_t>(field->containing_type()->field_count() +
                              field->containing_oneof()->index());
      return OffsetValue(offsets_[offset], field->type());
    } else {
      return GetFieldOffsetNonOneof(field);
    }
  }

  uint32 GetOneofCaseOffset(const OneofDescriptor* oneof_descriptor) const {
    return static_cast<uint32>(oneof_case_offset_) +
           static_cast<uint32>(static_cast<size_t>(oneof_descriptor->index()) *
                               sizeof(uint32));
  }

  bool HasHasbits() const { return has_bits_offset_ != -1; }

  // Bit index within the bit array of hasbits.  Bit order is low-to-high.
  uint32 HasBitIndex(const FieldDescriptor* field) const {
    if (has_bits_offset_ == -1) return static_cast<uint32>(-1);
    GOOGLE_DCHECK(HasHasbits());
    return has_bit_indices_[field->index()];
  }

  // Byte offset of the hasbits array.
  uint32 HasBitsOffset() const {
    GOOGLE_DCHECK(HasHasbits());
    return static_cast<uint32>(has_bits_offset_);
  }

  // The offset of the InternalMetadataWithArena member.
  // For Lite this will actually be an InternalMetadataWithArenaLite.
  // The schema doesn't contain enough information to distinguish between
  // these two cases.
  uint32 GetMetadataOffset() const {
    return static_cast<uint32>(metadata_offset_);
  }

  // Whether this message has an ExtensionSet.
  bool HasExtensionSet() const { return extensions_offset_ != -1; }

  // The offset of the ExtensionSet in this message.
  uint32 GetExtensionSetOffset() const {
    GOOGLE_DCHECK(HasExtensionSet());
    return static_cast<uint32>(extensions_offset_);
  }

  // The off set of WeakFieldMap when the message contains weak fields.
  // The default is 0 for now.
  int GetWeakFieldMapOffset() const { return weak_field_map_offset_; }

  bool IsDefaultInstance(const Message& message) const {
    return &message == default_instance_;
  }

  // Returns a pointer to the default value for this field.  The size and type
  // of the underlying data depends on the field's type.
  const void* GetFieldDefault(const FieldDescriptor* field) const {
    return reinterpret_cast<const uint8*>(default_instance_) +
           OffsetValue(offsets_[field->index()], field->type());
  }

  // Returns true if the field's accessor is called by any external code (aka,
  // non proto library code).
  bool IsFieldUsed(const FieldDescriptor* field) const {
    (void)field;
    return true;
  }

  bool IsFieldStripped(const FieldDescriptor* field) const {
    (void)field;
    return false;
  }

  bool IsMessageStripped(const Descriptor* descriptor) const {
    (void)descriptor;
    return false;
  }


  bool HasWeakFields() const { return weak_field_map_offset_ > 0; }

  // These members are intended to be private, but we cannot actually make them
  // private because this prevents us from using aggregate initialization of
  // them, ie.
  //
  //   ReflectionSchema schema = {a, b, c, d, e, ...};
  // private:
  const Message* default_instance_;
  const uint32* offsets_;
  const uint32* has_bit_indices_;
  int has_bits_offset_;
  int metadata_offset_;
  int extensions_offset_;
  int oneof_case_offset_;
  int object_size_;
  int weak_field_map_offset_;

  // We tag offset values to provide additional data about fields (such as
  // "unused").
  static uint32 OffsetValue(uint32 v, FieldDescriptor::Type /* type */) {
    return v & 0x7FFFFFFFu;
  }
};

// Structs that the code generator emits directly to describe a message.
// These should never used directly except to build a ReflectionSchema
// object.
//
// EXPERIMENTAL: these are changing rapidly, and may completely disappear
// or merge with ReflectionSchema.
struct MigrationSchema {
  int32 offsets_index;
  int32 has_bit_indices_index;
  int object_size;
};

// This struct tries to reduce unnecessary padding.
// The num_xxx might not be close to their respective pointer, but this saves
// padding.
struct PROTOBUF_EXPORT DescriptorTable {
  mutable bool is_initialized;
  bool is_eager;
  int size;  // of serialized descriptor
  const char* descriptor;
  const char* filename;
  once_flag* once;
  const DescriptorTable* const* deps;
  int num_deps;
  int num_messages;
  const MigrationSchema* schemas;
  const Message* const* default_instances;
  const uint32* offsets;
  // update the following descriptor arrays.
  Metadata* file_level_metadata;
  const EnumDescriptor** file_level_enum_descriptors;
  const ServiceDescriptor** file_level_service_descriptors;
};

enum {
  // Tag used on offsets for fields that don't have a real offset.
  // For example, weak message fields go into the WeakFieldMap and not in an
  // actual field.
  kInvalidFieldOffsetTag = 0x40000000u,
};

// AssignDescriptors() pulls the compiled FileDescriptor from the DescriptorPool
// and uses it to populate all of the global variables which store pointers to
// the descriptor objects.  It also constructs the reflection objects.  It is
// called the first time anyone calls descriptor() or GetReflection() on one of
// the types defined in the file.  AssignDescriptors() is thread-safe.
void PROTOBUF_EXPORT AssignDescriptors(const DescriptorTable* table,
                                       bool eager = false);

// Overload used to implement GetMetadataStatic in the generated code.
// See comments in compiler/cpp/internal/file.cc as to why.
// It takes a `Metadata` and returns it to allow for tail calls and reduce
// binary size.
Metadata PROTOBUF_EXPORT AssignDescriptors(const DescriptorTable* (*table)(),
                                           internal::once_flag* once,
                                           const Metadata& metadata);

// These cannot be in lite so we put them in the reflection.
PROTOBUF_EXPORT void UnknownFieldSetSerializer(const uint8* base, uint32 offset,
                                               uint32 tag, uint32 has_offset,
                                               io::CodedOutputStream* output);

struct PROTOBUF_EXPORT AddDescriptorsRunner {
  explicit AddDescriptorsRunner(const DescriptorTable* table);
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_REFLECTION_H__
