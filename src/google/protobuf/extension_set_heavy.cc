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
// Contains methods defined in extension_set.h which cannot be part of the
// lite library because they use descriptors or reflection.

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <variant>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/extension_set_inl.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/unknown_field_set.h"
#include "google/protobuf/wire_format_lite.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// Implementation of ExtensionFinder which finds extensions in a given
// DescriptorPool, using the given MessageFactory to construct sub-objects.
// This class is implemented in extension_set_heavy.cc.
class DescriptorPoolExtensionFinder {
 public:
  DescriptorPoolExtensionFinder(const DescriptorPool* pool,
                                MessageFactory* factory,
                                const Descriptor* extendee)
      : pool_(pool), factory_(factory), containing_type_(extendee) {}

  bool Find(int number, ExtensionInfo* output);

 private:
  const DescriptorPool* pool_;
  MessageFactory* factory_;
  const Descriptor* containing_type_;
};

void ExtensionSet::AppendToList(
    const Descriptor* extendee, const DescriptorPool* pool,
    std::vector<const FieldDescriptor*>* output) const {
  ForEach(
      [extendee, pool, &output](int number, const Extension& ext) {
        bool has = false;
        if (ext.is_repeated) {
          has = ext.GetSize() > 0;
        } else {
          has = !ext.is_cleared;
        }

        if (has) {
          // TODO: Looking up each field by number is somewhat
          // unfortunate.
          //   Is there a better way?  The problem is that descriptors are
          //   lazily-initialized, so they might not even be constructed until
          //   AppendToList() is called.

          if (ext.descriptor == nullptr) {
            output->push_back(pool->FindExtensionByNumber(extendee, number));
          } else {
            output->push_back(ext.descriptor);
          }
        }
      },
      Prefetch{});
}

inline FieldDescriptor::Type real_type(FieldType type) {
  ABSL_DCHECK(type > 0 && type <= FieldDescriptor::MAX_TYPE);
  return static_cast<FieldDescriptor::Type>(type);
}

inline FieldDescriptor::CppType cpp_type(FieldType type) {
  return FieldDescriptor::TypeToCppType(
      static_cast<FieldDescriptor::Type>(type));
}

inline WireFormatLite::FieldType field_type(FieldType type) {
  ABSL_DCHECK(type > 0 && type <= WireFormatLite::MAX_FIELD_TYPE);
  return static_cast<WireFormatLite::FieldType>(type);
}

#define ABSL_DCHECK_TYPE(EXTENSION, LABEL, CPPTYPE)                         \
  ABSL_DCHECK_EQ((EXTENSION).is_repeated ? FieldDescriptor::LABEL_REPEATED  \
                                         : FieldDescriptor::LABEL_OPTIONAL, \
                 FieldDescriptor::LABEL_##LABEL);                           \
  ABSL_DCHECK_EQ(cpp_type((EXTENSION).type), FieldDescriptor::CPPTYPE_##CPPTYPE)

const MessageLite& ExtensionSet::GetMessage(int number,
                                            const Descriptor* message_type,
                                            MessageFactory* factory) const {
  const Extension* extension = FindOrNull(number);
  if (extension == nullptr || extension->is_cleared) {
    // Not present.  Return the default value.
    return *factory->GetPrototype(message_type);
  } else {
    ABSL_DCHECK_TYPE(*extension, OPTIONAL, MESSAGE);
    if (extension->is_lazy) {
      return extension->ptr.lazymessage_value->GetMessage(
          *factory->GetPrototype(message_type), arena_);
    } else {
      return *extension->ptr.message_value;
    }
  }
}

MessageLite* ExtensionSet::MutableMessage(const FieldDescriptor* descriptor,
                                          MessageFactory* factory) {
  Extension* extension;
  if (MaybeNewExtension(descriptor->number(), descriptor, &extension)) {
    extension->type = descriptor->type();
    ABSL_DCHECK_EQ(cpp_type(extension->type), FieldDescriptor::CPPTYPE_MESSAGE);
    extension->is_repeated = false;
    extension->is_pointer = true;
    extension->is_packed = false;
    const MessageLite* prototype =
        factory->GetPrototype(descriptor->message_type());
    extension->is_lazy = false;
    extension->ptr.message_value = prototype->New(arena_);
    extension->is_cleared = false;
    return extension->ptr.message_value;
  } else {
    ABSL_DCHECK_TYPE(*extension, OPTIONAL, MESSAGE);
    extension->is_cleared = false;
    if (extension->is_lazy) {
      return extension->ptr.lazymessage_value->MutableMessage(
          *factory->GetPrototype(descriptor->message_type()), arena_);
    } else {
      return extension->ptr.message_value;
    }
  }
}

MessageLite* ExtensionSet::ReleaseMessage(const FieldDescriptor* descriptor,
                                          MessageFactory* factory) {
  Extension* extension = FindOrNull(descriptor->number());
  if (extension == nullptr) {
    // Not present.  Return nullptr.
    return nullptr;
  } else {
    ABSL_DCHECK_TYPE(*extension, OPTIONAL, MESSAGE);
    MessageLite* ret = nullptr;
    if (extension->is_lazy) {
      ret = extension->ptr.lazymessage_value->ReleaseMessage(
          *factory->GetPrototype(descriptor->message_type()), arena_);
      if (arena_ == nullptr) {
        delete extension->ptr.lazymessage_value;
      }
    } else {
      if (arena_ != nullptr) {
        ret = extension->ptr.message_value->New();
        ret->CheckTypeAndMergeFrom(*extension->ptr.message_value);
      } else {
        ret = extension->ptr.message_value;
      }
    }
    Erase(descriptor->number());
    return ret;
  }
}

MessageLite* ExtensionSet::UnsafeArenaReleaseMessage(
    const FieldDescriptor* descriptor, MessageFactory* factory) {
  Extension* extension = FindOrNull(descriptor->number());
  if (extension == nullptr) {
    // Not present.  Return nullptr.
    return nullptr;
  } else {
    ABSL_DCHECK_TYPE(*extension, OPTIONAL, MESSAGE);
    MessageLite* ret = nullptr;
    if (extension->is_lazy) {
      ret = extension->ptr.lazymessage_value->UnsafeArenaReleaseMessage(
          *factory->GetPrototype(descriptor->message_type()), arena_);
      if (arena_ == nullptr) {
        delete extension->ptr.lazymessage_value;
      }
    } else {
      ret = extension->ptr.message_value;
    }
    Erase(descriptor->number());
    return ret;
  }
}

ExtensionSet::Extension* ExtensionSet::MaybeNewRepeatedExtension(
    const FieldDescriptor* descriptor) {
  Extension* extension;
  if (MaybeNewExtension(descriptor->number(), descriptor, &extension)) {
    extension->type = descriptor->type();
    ABSL_DCHECK_EQ(cpp_type(extension->type), FieldDescriptor::CPPTYPE_MESSAGE);
    extension->is_repeated = true;
    extension->is_pointer = true;
    extension->ptr.repeated_message_value =
        Arena::Create<RepeatedPtrField<MessageLite> >(arena_);
  } else {
    ABSL_DCHECK_TYPE(*extension, REPEATED, MESSAGE);
  }
  return extension;
}

MessageLite* ExtensionSet::AddMessage(const FieldDescriptor* descriptor,
                                      MessageFactory* factory) {
  Extension* extension = MaybeNewRepeatedExtension(descriptor);

  // RepeatedPtrField<Message> does not know how to Add() since it cannot
  // allocate an abstract object, so we have to be tricky.
  MessageLite* result =
      reinterpret_cast<internal::RepeatedPtrFieldBase*>(
          extension->ptr.repeated_message_value)
          ->AddFromCleared<GenericTypeHandler<MessageLite> >();
  if (result == nullptr) {
    const MessageLite* prototype;
    if (extension->ptr.repeated_message_value->empty()) {
      prototype = factory->GetPrototype(descriptor->message_type());
      ABSL_CHECK(prototype != nullptr);
    } else {
      prototype = &extension->ptr.repeated_message_value->Get(0);
    }
    result = prototype->New(arena_);
    extension->ptr.repeated_message_value->AddAllocated(result);
  }
  return result;
}

void ExtensionSet::AddAllocatedMessage(const FieldDescriptor* descriptor,
                                       MessageLite* new_entry) {
  Extension* extension = MaybeNewRepeatedExtension(descriptor);

  extension->ptr.repeated_message_value->AddAllocated(new_entry);
}

void ExtensionSet::UnsafeArenaAddAllocatedMessage(
    const FieldDescriptor* descriptor, MessageLite* new_entry) {
  Extension* extension = MaybeNewRepeatedExtension(descriptor);

  extension->ptr.repeated_message_value->UnsafeArenaAddAllocated(new_entry);
}

static bool ValidateEnumUsingDescriptor(const void* arg, int number) {
  return reinterpret_cast<const EnumDescriptor*>(arg)->FindValueByNumber(
             number) != nullptr;
}

bool DescriptorPoolExtensionFinder::Find(int number, ExtensionInfo* output) {
  const FieldDescriptor* extension =
      pool_->FindExtensionByNumber(containing_type_, number);
  if (extension == nullptr) {
    return false;
  } else {
    output->type = extension->type();
    output->is_repeated = extension->is_repeated();
    output->is_packed = extension->is_packed();
    output->descriptor = extension;
    if (extension->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      output->message_info.prototype =
          factory_->GetPrototype(extension->message_type());
      output->message_info.tc_table =
          output->message_info.prototype->GetTcParseTable();
      ABSL_CHECK(output->message_info.prototype != nullptr)
          << "Extension factory's GetPrototype() returned nullptr; extension: "
          << extension->full_name();

    } else if (extension->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      output->enum_validity_check.func = ValidateEnumUsingDescriptor;
      output->enum_validity_check.arg = extension->enum_type();
    }

    return true;
  }
}


bool ExtensionSet::FindExtension(int wire_type, uint32_t field,
                                 const Message* extendee,
                                 const internal::ParseContext* ctx,
                                 ExtensionInfo* extension,
                                 bool* was_packed_on_wire) {
  if (ctx->data().pool == nullptr) {
    GeneratedExtensionFinder finder(extendee);
    if (!FindExtensionInfoFromFieldNumber(wire_type, field, &finder, extension,
                                          was_packed_on_wire)) {
      return false;
    }
  } else {
    DescriptorPoolExtensionFinder finder(ctx->data().pool, ctx->data().factory,
                                         extendee->GetDescriptor());
    if (!FindExtensionInfoFromFieldNumber(wire_type, field, &finder, extension,
                                          was_packed_on_wire)) {
      return false;
    }
  }
  return true;
}


const char* ExtensionSet::ParseField(uint64_t tag, const char* ptr,
                                     const Message* extendee,
                                     internal::InternalMetadata* metadata,
                                     internal::ParseContext* ctx) {
  int number = tag >> 3;
  bool was_packed_on_wire;
  ExtensionInfo extension;
  if (!FindExtension(tag & 7, number, extendee, ctx, &extension,
                     &was_packed_on_wire)) {
    return UnknownFieldParse(
        tag, metadata->mutable_unknown_fields<UnknownFieldSet>(), ptr, ctx);
  }
  return ParseFieldWithExtensionInfo<UnknownFieldSet>(
      number, was_packed_on_wire, extension, metadata, ptr, ctx);
}

const char* ExtensionSet::ParseFieldMaybeLazily(
    uint64_t tag, const char* ptr, const Message* extendee,
    internal::InternalMetadata* metadata, internal::ParseContext* ctx) {
  return ParseField(tag, ptr, extendee, metadata, ctx);
}

const char* ExtensionSet::ParseMessageSetItem(
    const char* ptr, const Message* extendee,
    internal::InternalMetadata* metadata, internal::ParseContext* ctx) {
  return ParseMessageSetItemTmpl<Message, UnknownFieldSet>(ptr, extendee,
                                                           metadata, ctx);
}

int ExtensionSet::SpaceUsedExcludingSelf() const {
  return internal::FromIntSize(SpaceUsedExcludingSelfLong());
}

size_t ExtensionSet::SpaceUsedExcludingSelfLong() const {
  size_t total_size =
      (is_large() ? map_.large->size() : flat_capacity_) * sizeof(KeyValue);
  ForEach(
      [&total_size](int /* number */, const Extension& ext) {
        total_size += ext.SpaceUsedExcludingSelfLong();
      },
      Prefetch{});
  return total_size;
}

inline size_t ExtensionSet::RepeatedMessage_SpaceUsedExcludingSelfLong(
    RepeatedPtrFieldBase* field) {
  return field->SpaceUsedExcludingSelfLong<GenericTypeHandler<Message> >();
}

size_t ExtensionSet::Extension::SpaceUsedExcludingSelfLong() const {
  size_t total_size = 0;
  if (is_repeated) {
    switch (cpp_type(type)) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)                               \
  case FieldDescriptor::CPPTYPE_##UPPERCASE:                            \
    total_size +=                                                       \
        sizeof(*ptr.repeated_##LOWERCASE##_value) +                     \
        ptr.repeated_##LOWERCASE##_value->SpaceUsedExcludingSelfLong(); \
    break

      HANDLE_TYPE(INT32, int32_t);
      HANDLE_TYPE(INT64, int64_t);
      HANDLE_TYPE(UINT32, uint32_t);
      HANDLE_TYPE(UINT64, uint64_t);
      HANDLE_TYPE(FLOAT, float);
      HANDLE_TYPE(DOUBLE, double);
      HANDLE_TYPE(BOOL, bool);
      HANDLE_TYPE(ENUM, enum);
      HANDLE_TYPE(STRING, string);
#undef HANDLE_TYPE

      case FieldDescriptor::CPPTYPE_MESSAGE:
        // repeated_message_value is actually a RepeatedPtrField<MessageLite>,
        // but MessageLite has no SpaceUsedLong(), so we must directly call
        // RepeatedPtrFieldBase::SpaceUsedExcludingSelfLong() with a different
        // type handler.
        total_size += sizeof(*ptr.repeated_message_value) +
                      RepeatedMessage_SpaceUsedExcludingSelfLong(
                          reinterpret_cast<internal::RepeatedPtrFieldBase*>(
                              ptr.repeated_message_value));
        break;
    }
  } else {
    switch (cpp_type(type)) {
      case FieldDescriptor::CPPTYPE_STRING:
        total_size += sizeof(*ptr.string_value) +
                      StringSpaceUsedExcludingSelfLong(*ptr.string_value);
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE:
        if (is_lazy) {
          total_size += ptr.lazymessage_value->SpaceUsedLong();
        } else {
          total_size +=
              DownCastMessage<Message>(ptr.message_value)->SpaceUsedLong();
        }
        break;
      default:
        // No extra storage costs for primitive types.
        break;
    }
  }
  return total_size;
}

uint8_t* ExtensionSet::SerializeMessageSetWithCachedSizesToArray(
    const MessageLite* extendee, uint8_t* target) const {
  io::EpsCopyOutputStream stream(
      target, MessageSetByteSize(),
      io::CodedOutputStream::IsDefaultSerializationDeterministic());
  return InternalSerializeMessageSetWithCachedSizesToArray(extendee, target,
                                                           &stream);
}

#if defined(PROTOBUF_DESCRIPTOR_WEAK_MESSAGES_ALLOWED)
bool ExtensionSet::ShouldRegisterAtThisTime(
    std::initializer_list<WeakPrototypeRef> messages, bool is_preregistration) {
  bool has_all = true;
  for (auto ref : messages) {
    has_all = has_all && GetPrototypeForWeakDescriptor(ref.table, ref.index,
                                                       false) != nullptr;
  }
  return has_all == is_preregistration;
}
#endif  // PROTOBUF_DESCRIPTOR_WEAK_MESSAGES_ALLOWED

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
