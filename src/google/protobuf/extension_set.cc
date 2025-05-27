// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/extension_set.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "absl/base/attributes.h"
#include "absl/base/optimization.h"
#include "absl/container/flat_hash_set.h"
#include "absl/functional/overload.h"
#include "absl/hash/hash.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/numeric/bits.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/extension_set_inl.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/wire_format_lite.h"

// must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace {

inline WireFormatLite::FieldType real_type(FieldType type) {
  ABSL_DCHECK(type > 0 && type <= WireFormatLite::MAX_FIELD_TYPE);
  return static_cast<WireFormatLite::FieldType>(type);
}

inline WireFormatLite::CppType cpp_type(FieldType type) {
  return WireFormatLite::FieldTypeToCppType(real_type(type));
}

// Registry stuff.

struct ExtensionInfoKey {
  const MessageLite* message;
  int number;
};

struct ExtensionEq {
  using is_transparent = void;
  bool operator()(const ExtensionInfo& lhs, const ExtensionInfo& rhs) const {
    return lhs.message == rhs.message && lhs.number == rhs.number;
  }
  bool operator()(const ExtensionInfo& lhs, const ExtensionInfoKey& rhs) const {
    return lhs.message == rhs.message && lhs.number == rhs.number;
  }
  bool operator()(const ExtensionInfoKey& lhs, const ExtensionInfo& rhs) const {
    return lhs.message == rhs.message && lhs.number == rhs.number;
  }
};

struct ExtensionHasher {
  using is_transparent = void;
  std::size_t operator()(const ExtensionInfo& info) const {
    return absl::HashOf(info.message, info.number);
  }
  std::size_t operator()(const ExtensionInfoKey& info) const {
    return absl::HashOf(info.message, info.number);
  }
};

using ExtensionRegistry =
    absl::flat_hash_set<ExtensionInfo, ExtensionHasher, ExtensionEq>;

static const ExtensionRegistry* global_registry = nullptr;

// This function is only called at startup, so there is no need for thread-
// safety.
void Register(const ExtensionInfo& info) {
  static auto local_static_registry = OnShutdownDelete(new ExtensionRegistry);
  global_registry = local_static_registry;
  if (!local_static_registry->insert(info).second) {
    ABSL_LOG(FATAL) << "Multiple extension registrations for type \""
                    << info.message->GetTypeName() << "\", field number "
                    << info.number << ".";
  }
}

const ExtensionInfo* FindRegisteredExtension(const MessageLite* extendee,
                                             int number) {
  if (!global_registry) return nullptr;

  ExtensionInfoKey info;
  info.message = extendee;
  info.number = number;

  auto it = global_registry->find(info);
  if (it == global_registry->end()) {
    return nullptr;
  } else {
    return &*it;
  }
}

}  // namespace

bool GeneratedExtensionFinder::Find(int number, ExtensionInfo* output) {
  const ExtensionInfo* extension = FindRegisteredExtension(extendee_, number);
  if (extension == nullptr) {
    return false;
  } else {
    *output = *extension;
    return true;
  }
}

void ExtensionSet::RegisterExtension(const MessageLite* extendee, int number,
                                     FieldType type, bool is_repeated,
                                     bool is_packed) {
  ABSL_CHECK_NE(type, WireFormatLite::TYPE_ENUM);
  ABSL_CHECK_NE(type, WireFormatLite::TYPE_MESSAGE);
  ABSL_CHECK_NE(type, WireFormatLite::TYPE_GROUP);
  ExtensionInfo info(extendee, number, type, is_repeated, is_packed);
  Register(info);
}

void ExtensionSet::RegisterEnumExtension(const MessageLite* extendee,
                                         int number, FieldType type,
                                         bool is_repeated, bool is_packed,
                                         const uint32_t* validation_data) {
  ABSL_CHECK_EQ(type, WireFormatLite::TYPE_ENUM);
  ExtensionInfo info(extendee, number, type, is_repeated, is_packed);
  info.enum_validity_check.func = nullptr;
  info.enum_validity_check.arg = validation_data;
  Register(info);
}

void ExtensionSet::RegisterMessageExtension(const MessageLite* extendee,
                                            int number, FieldType type,
                                            bool is_repeated, bool is_packed,
                                            const MessageLite* prototype,
                                            LazyEagerVerifyFnType verify_func,
                                            LazyAnnotation is_lazy) {
  ABSL_CHECK(type == WireFormatLite::TYPE_MESSAGE ||
             type == WireFormatLite::TYPE_GROUP);
  ExtensionInfo info(extendee, number, type, is_repeated, is_packed,
                     verify_func, is_lazy);
  info.message_info = {prototype,
#if defined(PROTOBUF_CONSTINIT_DEFAULT_INSTANCES)
                       prototype->GetTcParseTable()
#else
                       nullptr
#endif
  };
  Register(info);
}

// ===================================================================
// Constructors and basic methods.

ExtensionSet::~ExtensionSet() {
  // Deletes all allocated extensions.
  if (arena_ == nullptr) {
    ForEach([](int /* number */, Extension& ext) { ext.Free(); },
            PrefetchNta{});
    if (ABSL_PREDICT_FALSE(is_large())) {
      delete map_.large;
    } else {
      DeleteFlatMap(map_.flat, flat_capacity_);
    }
  }
}

ExtensionSet::KeyValue* ExtensionSet::AllocateFlatMap(
    Arena* arena, uint16_t powerof2_flat_capacity) {
  // It is important to allocate power-of-2 bytes in order to reuse
  // allocated blocks in arena for ExtensionSet and RepeatedFields.
  // ReturnArrayMemory is also more efficient with power-of-2 bytes, and
  // sizeof(KeyValue) is a power-of-2 on 64-bit platforms.
  static_assert(absl::has_single_bit(sizeof(KeyValue)) || sizeof(void*) != 8,
                "sizeof(KeyValue) must be a power of 2");
  ABSL_DCHECK(absl::has_single_bit(powerof2_flat_capacity));
  return Arena::CreateArray<ExtensionSet::KeyValue>(arena,
                                                    powerof2_flat_capacity);
}

void ExtensionSet::DeleteFlatMap(const ExtensionSet::KeyValue* flat,
                                 uint16_t flat_capacity) {
  // Arena::CreateArray already requires a trivially destructible type, but
  // ensure this constraint is not violated in the future.
  static_assert(std::is_trivially_destructible<KeyValue>::value,
                "CreateArray requires a trivially destructible type");
  // A const-cast is needed, but this is safe as we are about to deallocate the
  // array.
  internal::SizedArrayDelete(const_cast<KeyValue*>(flat),
                             sizeof(*flat) * flat_capacity);
}

// Defined in extension_set_heavy.cc.
// void ExtensionSet::AppendToList(const Descriptor* extendee,
//                                 const DescriptorPool* pool,
//                                 vector<const FieldDescriptor*>* output) const

bool ExtensionSet::Has(int number) const {
  const Extension* ext = FindOrNull(number);
  if (ext == nullptr) return false;
  ABSL_DCHECK(!ext->is_repeated);
  return !ext->is_cleared;
}

bool ExtensionSet::HasLazy(int number) const {
  return Has(number) && FindOrNull(number)->is_lazy;
}

int ExtensionSet::NumExtensions() const {
  int result = 0;
  ForEachNoPrefetch([&result](int /* number */, const Extension& ext) {
    if (!ext.is_cleared) {
      ++result;
    }
  });
  return result;
}

int ExtensionSet::ExtensionSize(int number) const {
  const Extension* ext = FindOrNull(number);
  return ext == nullptr ? 0 : ext->GetSize();
}

FieldType ExtensionSet::ExtensionType(int number) const {
  const Extension* ext = FindOrNull(number);
  if (ext == nullptr) {
    ABSL_DLOG(FATAL)
        << "Don't lookup extension types if they aren't present (1). ";
    return 0;
  }
  if (ext->is_cleared) {
    ABSL_DLOG(FATAL)
        << "Don't lookup extension types if they aren't present (2). ";
  }
  return ext->type;
}

void ExtensionSet::ClearExtension(int number) {
  Extension* ext = FindOrNull(number);
  if (ext == nullptr) return;
  ext->Clear();
}

// ===================================================================
// Field accessors

namespace {

enum { REPEATED_FIELD, OPTIONAL_FIELD };

}  // namespace

#define ABSL_DCHECK_TYPE(EXTENSION, LABEL, CPPTYPE)                         \
  ABSL_DCHECK_EQ((EXTENSION).is_repeated ? REPEATED_FIELD : OPTIONAL_FIELD, \
                 LABEL);                                                    \
  ABSL_DCHECK_EQ(cpp_type((EXTENSION).type), WireFormatLite::CPPTYPE_##CPPTYPE)

// -------------------------------------------------------------------

const void* ExtensionSet::GetRawRepeatedField(int number,
                                              const void* default_value) const {
  const Extension* extension = FindOrNull(number);
  if (extension == nullptr) {
    return default_value;
  }
  return extension->raw_ptr();
}

void* ExtensionSet::MutableRawRepeatedField(int number, FieldType field_type,
                                            bool packed,
                                            const FieldDescriptor* desc) {
  Extension* extension;

  // We instantiate an empty Repeated{,Ptr}Field if one doesn't exist for this
  // extension.
  if (MaybeNewExtension(number, desc, &extension)) {
    extension->is_repeated = true;
    extension->is_pointer = true;
    extension->type = field_type;
    extension->is_packed = packed;
    ABSL_DCHECK(!extension->is_cleared);

    switch (WireFormatLite::FieldTypeToCppType(
        static_cast<WireFormatLite::FieldType>(field_type))) {
      case WireFormatLite::CPPTYPE_INT32:
        extension->ptr.repeated_int32_t_value =
            Arena::Create<RepeatedField<int32_t>>(arena_);
        break;
      case WireFormatLite::CPPTYPE_INT64:
        extension->ptr.repeated_int64_t_value =
            Arena::Create<RepeatedField<int64_t>>(arena_);
        break;
      case WireFormatLite::CPPTYPE_UINT32:
        extension->ptr.repeated_uint32_t_value =
            Arena::Create<RepeatedField<uint32_t>>(arena_);
        break;
      case WireFormatLite::CPPTYPE_UINT64:
        extension->ptr.repeated_uint64_t_value =
            Arena::Create<RepeatedField<uint64_t>>(arena_);
        break;
      case WireFormatLite::CPPTYPE_DOUBLE:
        extension->ptr.repeated_double_value =
            Arena::Create<RepeatedField<double>>(arena_);
        break;
      case WireFormatLite::CPPTYPE_FLOAT:
        extension->ptr.repeated_float_value =
            Arena::Create<RepeatedField<float>>(arena_);
        break;
      case WireFormatLite::CPPTYPE_BOOL:
        extension->ptr.repeated_bool_value =
            Arena::Create<RepeatedField<bool>>(arena_);
        break;
      case WireFormatLite::CPPTYPE_ENUM:
        extension->ptr.repeated_int32_t_value =
            Arena::Create<RepeatedField<int>>(arena_);
        break;
      case WireFormatLite::CPPTYPE_STRING:
        extension->ptr.repeated_string_value =
            Arena::Create<RepeatedPtrField<std::string>>(arena_);
        break;
      case WireFormatLite::CPPTYPE_MESSAGE:
        extension->ptr.repeated_message_value =
            Arena::Create<RepeatedPtrField<MessageLite>>(arena_);
        break;
    }
  }

  return extension->raw_ptr();
}

// Compatible version using old call signature. Does not create extensions when
// the don't already exist; instead, just ABSL_CHECK-fails.
void* ExtensionSet::MutableRawRepeatedField(int number) {
  Extension* extension = FindOrNull(number);
  ABSL_CHECK(extension != nullptr) << "Extension not found.";
  return extension->raw_ptr();
}

// -------------------------------------------------------------------
// Enums

size_t ExtensionSet::GetMessageByteSizeLong(int number) const {
  const Extension* extension = FindOrNull(number);
  ABSL_CHECK(extension != nullptr) << "not present";
  ABSL_DCHECK_TYPE(*extension, OPTIONAL_FIELD, MESSAGE);
  return extension->is_lazy ? extension->ptr.lazymessage_value->ByteSizeLong()
                            : extension->ptr.message_value->ByteSizeLong();
}

uint8_t* ExtensionSet::InternalSerializeMessage(
    int number, const MessageLite* prototype, uint8_t* target,
    io::EpsCopyOutputStream* stream) const {
  const Extension* extension = FindOrNull(number);
  ABSL_CHECK(extension != nullptr) << "not present";
  ABSL_DCHECK_TYPE(*extension, OPTIONAL_FIELD, MESSAGE);

  if (extension->is_lazy) {
    return extension->ptr.lazymessage_value->WriteMessageToArray(
        prototype, number, target, stream);
  }

  const auto* msg = extension->ptr.message_value;
  return WireFormatLite::InternalWriteMessage(
      number, *msg, msg->GetCachedSize(), target, stream);
}

// -------------------------------------------------------------------
// Strings

std::string* ExtensionSet::MutableString(int number, FieldType type,
                                         const FieldDescriptor* descriptor) {
  Extension* extension;
  if (MaybeNewExtension(number, descriptor, &extension)) {
    extension->type = type;
    ABSL_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_STRING);
    extension->is_repeated = false;
    extension->is_pointer = true;
    extension->ptr.string_value = Arena::Create<std::string>(arena_);
  } else {
    ABSL_DCHECK_TYPE(*extension, OPTIONAL_FIELD, STRING);
  }
  extension->is_cleared = false;
  return extension->ptr.string_value;
}

std::string* ExtensionSet::MutableRepeatedString(int number, int index) {
  Extension* extension = FindOrNull(number);
  ABSL_CHECK(extension != nullptr) << "Index out-of-bounds (field is empty).";
  ABSL_DCHECK_TYPE(*extension, REPEATED_FIELD, STRING);
  return extension->ptr.repeated_string_value->Mutable(index);
}

std::string* ExtensionSet::AddString(int number, FieldType type,
                                     const FieldDescriptor* descriptor) {
  Extension* extension;
  if (MaybeNewExtension(number, descriptor, &extension)) {
    extension->type = type;
    ABSL_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_STRING);
    extension->is_repeated = true;
    extension->is_pointer = true;
    extension->is_packed = false;
    extension->ptr.repeated_string_value =
        Arena::Create<RepeatedPtrField<std::string>>(arena_);
  } else {
    ABSL_DCHECK_TYPE(*extension, REPEATED_FIELD, STRING);
  }
  return extension->ptr.repeated_string_value->Add();
}

// -------------------------------------------------------------------
// Messages

const MessageLite& ExtensionSet::GetMessage(
    int number, const MessageLite& default_value) const {
  const Extension* extension = FindOrNull(number);
  if (extension == nullptr) {
    // Not present.  Return the default value.
    return default_value;
  } else {
    ABSL_DCHECK_TYPE(*extension, OPTIONAL_FIELD, MESSAGE);
    if (extension->is_lazy) {
      return extension->ptr.lazymessage_value->GetMessage(default_value,
                                                          arena_);
    } else {
      return *extension->ptr.message_value;
    }
  }
}

// Defined in extension_set_heavy.cc.
// const MessageLite& ExtensionSet::GetMessage(int number,
//                                             const Descriptor* message_type,
//                                             MessageFactory* factory) const

MessageLite* ExtensionSet::MutableMessage(int number, FieldType type,
                                          const MessageLite& prototype,
                                          const FieldDescriptor* descriptor) {
  Extension* extension;
  if (MaybeNewExtension(number, descriptor, &extension)) {
    extension->type = type;
    ABSL_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_MESSAGE);
    extension->is_repeated = false;
    extension->is_pointer = true;
    extension->is_lazy = false;
    extension->ptr.message_value = prototype.New(arena_);
    extension->is_cleared = false;
    return extension->ptr.message_value;
  } else {
    ABSL_DCHECK_TYPE(*extension, OPTIONAL_FIELD, MESSAGE);
    extension->is_cleared = false;
    if (extension->is_lazy) {
      return extension->ptr.lazymessage_value->MutableMessage(prototype,
                                                              arena_);
    } else {
      return extension->ptr.message_value;
    }
  }
}

// Defined in extension_set_heavy.cc.
// MessageLite* ExtensionSet::MutableMessage(int number, FieldType type,
//                                           const Descriptor* message_type,
//                                           MessageFactory* factory)

void ExtensionSet::SetAllocatedMessage(int number, FieldType type,
                                       const FieldDescriptor* descriptor,
                                       MessageLite* message) {
  if (message == nullptr) {
    ClearExtension(number);
    return;
  }
  Arena* const arena = arena_;
  Arena* const message_arena = message->GetArena();
  ABSL_DCHECK(message_arena == nullptr || message_arena == arena);

  Extension* extension;
  if (MaybeNewExtension(number, descriptor, &extension)) {
    extension->type = type;
    ABSL_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_MESSAGE);
    extension->is_repeated = false;
    extension->is_pointer = true;
    extension->is_lazy = false;
    if (message_arena == arena) {
      extension->ptr.message_value = message;
    } else if (message_arena == nullptr) {
      extension->ptr.message_value = message;
      arena->Own(message);  // not nullptr because not equal to message_arena
    } else {
      extension->ptr.message_value = message->New(arena);
      extension->ptr.message_value->CheckTypeAndMergeFrom(*message);
    }
  } else {
    ABSL_DCHECK_TYPE(*extension, OPTIONAL_FIELD, MESSAGE);
    if (extension->is_lazy) {
      extension->ptr.lazymessage_value->SetAllocatedMessage(message, arena);
    } else {
      if (arena == nullptr) {
        delete extension->ptr.message_value;
      }
      if (message_arena == arena) {
        extension->ptr.message_value = message;
      } else if (message_arena == nullptr) {
        extension->ptr.message_value = message;
        arena->Own(message);  // not nullptr because not equal to message_arena
      } else {
        extension->ptr.message_value = message->New(arena);
        extension->ptr.message_value->CheckTypeAndMergeFrom(*message);
      }
    }
  }
  extension->is_cleared = false;
}

void ExtensionSet::UnsafeArenaSetAllocatedMessage(
    int number, FieldType type, const FieldDescriptor* descriptor,
    MessageLite* message) {
  if (message == nullptr) {
    ClearExtension(number);
    return;
  }
  Extension* extension;
  if (MaybeNewExtension(number, descriptor, &extension)) {
    extension->type = type;
    ABSL_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_MESSAGE);
    extension->is_repeated = false;
    extension->is_pointer = true;
    extension->is_lazy = false;
    extension->ptr.message_value = message;
  } else {
    ABSL_DCHECK_TYPE(*extension, OPTIONAL_FIELD, MESSAGE);
    if (extension->is_lazy) {
      extension->ptr.lazymessage_value->UnsafeArenaSetAllocatedMessage(message,
                                                                       arena_);
    } else {
      if (arena_ == nullptr) {
        delete extension->ptr.message_value;
      }
      extension->ptr.message_value = message;
    }
  }
  extension->is_cleared = false;
}

MessageLite* ExtensionSet::ReleaseMessage(int number,
                                          const MessageLite& prototype) {
  Extension* extension = FindOrNull(number);
  if (extension == nullptr) {
    // Not present.  Return nullptr.
    return nullptr;
  } else {
    ABSL_DCHECK_TYPE(*extension, OPTIONAL_FIELD, MESSAGE);
    MessageLite* ret = nullptr;
    if (extension->is_lazy) {
      Arena* const arena = arena_;
      ret = extension->ptr.lazymessage_value->ReleaseMessage(prototype, arena);
      if (arena == nullptr) {
        delete extension->ptr.lazymessage_value;
      }
    } else {
      if (arena_ == nullptr) {
        ret = extension->ptr.message_value;
      } else {
        // ReleaseMessage() always returns a heap-allocated message, and we are
        // on an arena, so we need to make a copy of this message to return.
        ret = extension->ptr.message_value->New();
        ret->CheckTypeAndMergeFrom(*extension->ptr.message_value);
      }
    }
    Erase(number);
    return ret;
  }
}

MessageLite* ExtensionSet::UnsafeArenaReleaseMessage(
    int number, const MessageLite& prototype) {
  Extension* extension = FindOrNull(number);
  if (extension == nullptr) {
    // Not present.  Return nullptr.
    return nullptr;
  } else {
    ABSL_DCHECK_TYPE(*extension, OPTIONAL_FIELD, MESSAGE);
    MessageLite* ret = nullptr;
    if (extension->is_lazy) {
      Arena* const arena = arena_;
      ret = extension->ptr.lazymessage_value->UnsafeArenaReleaseMessage(
          prototype, arena);
      if (arena == nullptr) {
        delete extension->ptr.lazymessage_value;
      }
    } else {
      ret = extension->ptr.message_value;
    }
    Erase(number);
    return ret;
  }
}

// Defined in extension_set_heavy.cc.
// MessageLite* ExtensionSet::ReleaseMessage(const FieldDescriptor* descriptor,
//                                           MessageFactory* factory);

const MessageLite& ExtensionSet::GetRepeatedMessage(int number,
                                                    int index) const {
  const Extension* extension = FindOrNull(number);
  ABSL_CHECK(extension != nullptr) << "Index out-of-bounds (field is empty).";
  ABSL_DCHECK_TYPE(*extension, REPEATED_FIELD, MESSAGE);
  return extension->ptr.repeated_message_value->Get(index);
}

MessageLite* ExtensionSet::MutableRepeatedMessage(int number, int index) {
  Extension* extension = FindOrNull(number);
  ABSL_CHECK(extension != nullptr) << "Index out-of-bounds (field is empty).";
  ABSL_DCHECK_TYPE(*extension, REPEATED_FIELD, MESSAGE);
  return extension->ptr.repeated_message_value->Mutable(index);
}

MessageLite* ExtensionSet::AddMessage(int number, FieldType type,
                                      const MessageLite& prototype,
                                      const FieldDescriptor* descriptor) {
  Extension* extension;
  if (MaybeNewExtension(number, descriptor, &extension)) {
    extension->type = type;
    ABSL_DCHECK_EQ(cpp_type(extension->type), WireFormatLite::CPPTYPE_MESSAGE);
    extension->is_repeated = true;
    extension->is_pointer = true;
    extension->ptr.repeated_message_value =
        Arena::Create<RepeatedPtrField<MessageLite>>(arena_);
  } else {
    ABSL_DCHECK_TYPE(*extension, REPEATED_FIELD, MESSAGE);
  }

  return reinterpret_cast<internal::RepeatedPtrFieldBase*>(
             extension->ptr.repeated_message_value)
      ->AddFromPrototype<GenericTypeHandler<MessageLite>>(&prototype);
}

// Defined in extension_set_heavy.cc.
// MessageLite* ExtensionSet::AddMessage(int number, FieldType type,
//                                       const Descriptor* message_type,
//                                       MessageFactory* factory)

#undef ABSL_DCHECK_TYPE

void ExtensionSet::RemoveLast(int number) {
  Extension* extension = FindOrNull(number);
  ABSL_CHECK(extension != nullptr) << "Index out-of-bounds (field is empty).";
  ABSL_DCHECK(extension->is_repeated);

  switch (cpp_type(extension->type)) {
    case WireFormatLite::CPPTYPE_INT32:
      extension->ptr.repeated_int32_t_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_INT64:
      extension->ptr.repeated_int64_t_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_UINT32:
      extension->ptr.repeated_uint32_t_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_UINT64:
      extension->ptr.repeated_uint64_t_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_FLOAT:
      extension->ptr.repeated_float_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_DOUBLE:
      extension->ptr.repeated_double_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_BOOL:
      extension->ptr.repeated_bool_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_ENUM:
      extension->ptr.repeated_int32_t_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_STRING:
      extension->ptr.repeated_string_value->RemoveLast();
      break;
    case WireFormatLite::CPPTYPE_MESSAGE:
      extension->ptr.repeated_message_value->RemoveLast();
      break;
  }
}

MessageLite* ExtensionSet::ReleaseLast(int number) {
  Extension* extension = FindOrNull(number);
  ABSL_CHECK(extension != nullptr) << "Index out-of-bounds (field is empty).";
  ABSL_DCHECK(extension->is_repeated);
  ABSL_DCHECK(cpp_type(extension->type) == WireFormatLite::CPPTYPE_MESSAGE);
  return extension->ptr.repeated_message_value->ReleaseLast();
}

MessageLite* ExtensionSet::UnsafeArenaReleaseLast(int number) {
  Extension* extension = FindOrNull(number);
  ABSL_CHECK(extension != nullptr) << "Index out-of-bounds (field is empty).";
  ABSL_DCHECK(extension->is_repeated);
  ABSL_DCHECK(cpp_type(extension->type) == WireFormatLite::CPPTYPE_MESSAGE);
  return extension->ptr.repeated_message_value->UnsafeArenaReleaseLast();
}

void ExtensionSet::SwapElements(int number, int index1, int index2) {
  Extension* extension = FindOrNull(number);
  ABSL_CHECK(extension != nullptr) << "Index out-of-bounds (field is empty).";
  ABSL_DCHECK(extension->is_repeated);

  switch (cpp_type(extension->type)) {
    case WireFormatLite::CPPTYPE_INT32:
      extension->ptr.repeated_int32_t_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_INT64:
      extension->ptr.repeated_int64_t_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_UINT32:
      extension->ptr.repeated_uint32_t_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_UINT64:
      extension->ptr.repeated_uint64_t_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_FLOAT:
      extension->ptr.repeated_float_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_DOUBLE:
      extension->ptr.repeated_double_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_BOOL:
      extension->ptr.repeated_bool_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_ENUM:
      extension->ptr.repeated_int32_t_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_STRING:
      extension->ptr.repeated_string_value->SwapElements(index1, index2);
      break;
    case WireFormatLite::CPPTYPE_MESSAGE:
      extension->ptr.repeated_message_value->SwapElements(index1, index2);
      break;
  }
}

// ===================================================================

void ExtensionSet::Clear() {
  ForEach([](int /* number */, Extension& ext) { ext.Clear(); }, Prefetch{});
}

namespace {
// Computes the size of an ExtensionSet union without actually constructing the
// union. Note that we do not count cleared extensions from the source to be
// part of the total, because there is no need to allocate space for those. We
// do include cleared extensions in the destination, though, because those are
// already allocated and will not be going away.
template <typename ItX, typename ItY>
size_t SizeOfUnion(ItX it_dest, ItX end_dest, ItY it_source, ItY end_source) {
  size_t result = std::distance(it_dest, end_dest);
  for (; it_source != end_source; ++it_source) {
    while (it_dest != end_dest && it_dest->first < it_source->first) {
      ++it_dest;
    }
    result += (it_dest == end_dest || it_dest->first > it_source->first) &&
              !it_source->second.is_cleared;
  }
  return result;
}
}  // namespace

void ExtensionSet::MergeFrom(const MessageLite* extendee,
                             const ExtensionSet& other) {
  Prefetch5LinesFrom1Line(&other);
  if (ABSL_PREDICT_TRUE(IsCompletelyEmpty() && !other.is_large())) {
    InternalMergeFromSmallToEmpty(extendee, other);
    return;
  }
  InternalMergeFromSlow(extendee, other);
}

void ExtensionSet::InternalMergeFromSmallToEmpty(const MessageLite* extendee,
                                                 const ExtensionSet& other) {
  ABSL_DCHECK(!other.is_large());
  // Compiler is complaining on potential side effects for `!other.is_large()`.
  ABSL_ASSUME(static_cast<int16_t>(flat_size_) >= 0);
  ABSL_DCHECK(IsCompletelyEmpty());

  size_t count = other.NumExtensions();
  if (count == 0) {
    return;
  }

  InternalReserveSmallCapacityFromEmpty(count);
  flat_size_ = static_cast<uint16_t>(count);
  auto dst_it = map_.flat;
  other.ForEach(
      [extendee, this, &dst_it, &other](int number, const Extension& ext) {
        if (ext.is_cleared) {
          return;
        }
        dst_it->first = number;
        this->InternalExtensionMergeFromIntoUninitializedExtension(
            dst_it->second, extendee, number, ext, other.arena_);
        ++dst_it;
      },
      Prefetch{});
}

void ExtensionSet::InternalMergeFromSlow(const MessageLite* extendee,
                                         const ExtensionSet& other) {
  if (ABSL_PREDICT_TRUE(!is_large())) {
    if (ABSL_PREDICT_TRUE(!other.is_large())) {
      GrowCapacity(SizeOfUnion(flat_begin(), flat_end(), other.flat_begin(),
                               other.flat_end()));
    } else {
      GrowCapacity(SizeOfUnion(flat_begin(), flat_end(),
                               other.map_.large->begin(),
                               other.map_.large->end()));
    }
  }
  other.ForEach(
      [extendee, this, &other](int number, const Extension& ext) {
        this->InternalExtensionMergeFrom(extendee, number, ext, other.arena_);
      },
      Prefetch{});
}

void ExtensionSet::InternalExtensionMergeFromIntoUninitializedExtension(
    Extension& dst_extension, const MessageLite* extendee, int number,
    const Extension& other_extension, Arena* other_arena) {
  // Copy and initialize all the fields.
  // We fix up incorrect pointers later.
  // Primitive values are copied here.
  dst_extension = other_extension;

  if (other_extension.is_repeated) {
    switch (cpp_type(other_extension.type)) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE, REPEATED_TYPE)       \
  case WireFormatLite::CPPTYPE_##UPPERCASE:                    \
    dst_extension.ptr.repeated_##LOWERCASE##_value =           \
        Arena::Create<REPEATED_TYPE>(arena_);                  \
    dst_extension.ptr.repeated_##LOWERCASE##_value->MergeFrom( \
        *other_extension.ptr.repeated_##LOWERCASE##_value);    \
    break;

      HANDLE_TYPE(INT32, int32_t, RepeatedField<int32_t>);
      HANDLE_TYPE(INT64, int64_t, RepeatedField<int64_t>);
      HANDLE_TYPE(UINT32, uint32_t, RepeatedField<uint32_t>);
      HANDLE_TYPE(UINT64, uint64_t, RepeatedField<uint64_t>);
      HANDLE_TYPE(FLOAT, float, RepeatedField<float>);
      HANDLE_TYPE(DOUBLE, double, RepeatedField<double>);
      HANDLE_TYPE(BOOL, bool, RepeatedField<bool>);
      HANDLE_TYPE(ENUM, int32_t, RepeatedField<int>);
      HANDLE_TYPE(STRING, string, RepeatedPtrField<std::string>);
      HANDLE_TYPE(MESSAGE, message, RepeatedPtrField<MessageLite>);
#undef HANDLE_TYPE
    }
    return;
  }

  // Non-repeated extension
  switch (cpp_type(other_extension.type)) {
    case WireFormatLite::CPPTYPE_INT32:
    case WireFormatLite::CPPTYPE_INT64:
    case WireFormatLite::CPPTYPE_UINT32:
    case WireFormatLite::CPPTYPE_UINT64:
    case WireFormatLite::CPPTYPE_FLOAT:
    case WireFormatLite::CPPTYPE_DOUBLE:
    case WireFormatLite::CPPTYPE_BOOL:
    case WireFormatLite::CPPTYPE_ENUM:
      break;  // Do nothing.
    case WireFormatLite::CPPTYPE_STRING:
      dst_extension.ptr.string_value =
          Arena::Create<std::string>(arena_, *other_extension.ptr.string_value);
      break;
    case WireFormatLite::CPPTYPE_MESSAGE: {
      if (other_extension.is_lazy) {
        dst_extension.ptr.lazymessage_value =
            other_extension.ptr.lazymessage_value->Clone(
                arena_, *other_extension.ptr.lazymessage_value, other_arena);
      } else {
        dst_extension.ptr.message_value =
            other_extension.ptr.message_value->New(arena_);
        dst_extension.ptr.message_value->CheckTypeAndMergeFrom(
            *other_extension.ptr.message_value);
      }
      break;
    }
  }
}

void ExtensionSet::InternalExtensionMergeFrom(const MessageLite* extendee,
                                              int number,
                                              const Extension& other_extension,
                                              Arena* other_arena) {
  Extension* dst_extension;
  bool is_new =
      MaybeNewExtension(number, other_extension.descriptor, &dst_extension);
  if (is_new) {
    InternalExtensionMergeFromIntoUninitializedExtension(
        *dst_extension, extendee, number, other_extension, other_arena);
    return;
  }
  if (other_extension.is_repeated) {
    ABSL_DCHECK_EQ(dst_extension->type, other_extension.type);
    ABSL_DCHECK_EQ(dst_extension->is_packed, other_extension.is_packed);
    ABSL_DCHECK(dst_extension->is_repeated);

    switch (cpp_type(other_extension.type)) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)                       \
  case WireFormatLite::CPPTYPE_##UPPERCASE:                     \
    dst_extension->ptr.repeated_##LOWERCASE##_value->MergeFrom( \
        *other_extension.ptr.repeated_##LOWERCASE##_value);     \
    break;

      HANDLE_TYPE(INT32, int32_t);
      HANDLE_TYPE(INT64, int64_t);
      HANDLE_TYPE(UINT32, uint32_t);
      HANDLE_TYPE(UINT64, uint64_t);
      HANDLE_TYPE(FLOAT, float);
      HANDLE_TYPE(DOUBLE, double);
      HANDLE_TYPE(BOOL, bool);
      HANDLE_TYPE(ENUM, int32_t);
      HANDLE_TYPE(STRING, string);
      HANDLE_TYPE(MESSAGE, message);
#undef HANDLE_TYPE
    }
    return;
  }

  if (other_extension.is_cleared) {
    return;
  }
  dst_extension->is_cleared = false;
  switch (cpp_type(other_extension.type)) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)                                 \
  case WireFormatLite::CPPTYPE_##UPPERCASE:                               \
    dst_extension->LOWERCASE##_value = other_extension.LOWERCASE##_value; \
    break;

    HANDLE_TYPE(INT32, int32_t);
    HANDLE_TYPE(INT64, int64_t);
    HANDLE_TYPE(UINT32, uint32_t);
    HANDLE_TYPE(UINT64, uint64_t);
    HANDLE_TYPE(FLOAT, float);
    HANDLE_TYPE(DOUBLE, double);
    HANDLE_TYPE(BOOL, bool);
    HANDLE_TYPE(ENUM, int32_t);
#undef HANDLE_TYPE
    case WireFormatLite::CPPTYPE_STRING:
      dst_extension->ptr.string_value->assign(
          *other_extension.ptr.string_value);
      break;
    case WireFormatLite::CPPTYPE_MESSAGE: {
      ABSL_DCHECK_EQ(dst_extension->type, other_extension.type);
      ABSL_DCHECK_EQ(dst_extension->is_packed, other_extension.is_packed);
      ABSL_DCHECK(!dst_extension->is_repeated);
      if (other_extension.is_lazy) {
        if (dst_extension->is_lazy) {
          dst_extension->ptr.lazymessage_value->MergeFrom(
              GetPrototypeForLazyMessage(extendee, number),
              *other_extension.ptr.lazymessage_value, arena_, other_arena);
        } else {
          dst_extension->ptr.message_value->CheckTypeAndMergeFrom(
              other_extension.ptr.lazymessage_value->GetMessage(
                  *dst_extension->ptr.message_value, other_arena));
        }
      } else {
        if (dst_extension->is_lazy) {
          dst_extension->ptr.lazymessage_value
              ->MutableMessage(*other_extension.ptr.message_value, arena_)
              ->CheckTypeAndMergeFrom(*other_extension.ptr.message_value);
        } else {
          dst_extension->ptr.message_value->CheckTypeAndMergeFrom(
              *other_extension.ptr.message_value);
        }
      }
      break;
    }
  }
}

void ExtensionSet::Swap(const MessageLite* extendee, ExtensionSet* other) {
  if (internal::CanUseInternalSwap(arena_, other->arena_)) {
    InternalSwap(other);
  } else {
    // TODO: We maybe able to optimize a case where we are
    // swapping from heap to arena-allocated extension set, by just Own()'ing
    // the extensions.
    ExtensionSet extension_set;
    extension_set.MergeFrom(extendee, *other);
    other->Clear();
    other->MergeFrom(extendee, *this);
    Clear();
    MergeFrom(extendee, extension_set);
  }
}

void ExtensionSet::InternalSwap(ExtensionSet* other) {
  using std::swap;
  swap(arena_, other->arena_);
  swap(flat_capacity_, other->flat_capacity_);
  swap(flat_size_, other->flat_size_);
  swap(map_, other->map_);
}

void ExtensionSet::SwapExtension(const MessageLite* extendee,
                                 ExtensionSet* other, int number) {
  if (this == other) return;

  Arena* const arena = arena_;
  Arena* const other_arena = other->arena_;
  if (arena == other_arena) {
    UnsafeShallowSwapExtension(other, number);
    return;
  }

  Extension* this_ext = FindOrNull(number);
  Extension* other_ext = other->FindOrNull(number);

  if (this_ext == other_ext) return;

  if (this_ext != nullptr && other_ext != nullptr) {
    // TODO: We could further optimize these cases,
    // especially avoid creation of ExtensionSet, and move MergeFrom logic
    // into Extensions itself (which takes arena as an argument).
    // We do it this way to reuse the copy-across-arenas logic already
    // implemented in ExtensionSet's MergeFrom.
    ExtensionSet temp;
    temp.InternalExtensionMergeFrom(extendee, number, *other_ext, other_arena);
    Extension* temp_ext = temp.FindOrNull(number);

    other_ext->Clear();
    other->InternalExtensionMergeFrom(extendee, number, *this_ext, arena);
    this_ext->Clear();
    InternalExtensionMergeFrom(extendee, number, *temp_ext, temp.GetArena());
  } else if (this_ext == nullptr) {
    InternalExtensionMergeFrom(extendee, number, *other_ext, other_arena);
    if (other_arena == nullptr) other_ext->Free();
    other->Erase(number);
  } else {
    other->InternalExtensionMergeFrom(extendee, number, *this_ext, arena);
    if (arena == nullptr) this_ext->Free();
    Erase(number);
  }
}

void ExtensionSet::UnsafeShallowSwapExtension(ExtensionSet* other, int number) {
  if (this == other) return;

  Extension* this_ext = FindOrNull(number);
  Extension* other_ext = other->FindOrNull(number);

  if (this_ext == other_ext) return;

  ABSL_DCHECK_EQ(arena_, other->arena_);

  if (this_ext != nullptr && other_ext != nullptr) {
    std::swap(*this_ext, *other_ext);
  } else if (this_ext == nullptr) {
    *Insert(number).first = *other_ext;
    other->Erase(number);
  } else {
    *other->Insert(number).first = *this_ext;
    Erase(number);
  }
}

bool ExtensionSet::IsInitialized(const MessageLite* extendee) const {
  // Extensions are never required.  However, we need to check that all
  // embedded messages are initialized.
  Arena* const arena = arena_;
  if (ABSL_PREDICT_FALSE(is_large())) {
    for (const auto& kv : *map_.large) {
      if (!kv.second.IsInitialized(this, extendee, kv.first, arena)) {
        return false;
      }
    }
    return true;
  }
  for (const KeyValue* it = flat_begin(); it != flat_end(); ++it) {
    if (!it->second.IsInitialized(this, extendee, it->first, arena)) {
      return false;
    }
  }
  return true;
}

const char* ExtensionSet::ParseField(uint64_t tag, const char* ptr,
                                     const MessageLite* extendee,
                                     internal::InternalMetadata* metadata,
                                     internal::ParseContext* ctx) {
  GeneratedExtensionFinder finder(extendee);
  int number = tag >> 3;
  bool was_packed_on_wire;
  ExtensionInfo extension;
  if (!FindExtensionInfoFromFieldNumber(tag & 7, number, &finder, &extension,
                                        &was_packed_on_wire)) {
    return UnknownFieldParse(
        tag, metadata->mutable_unknown_fields<std::string>(), ptr, ctx);
  }
  return ParseFieldWithExtensionInfo<std::string>(
      number, was_packed_on_wire, extension, metadata, ptr, ctx);
}

const char* ExtensionSet::ParseMessageSetItem(
    const char* ptr, const MessageLite* extendee,
    internal::InternalMetadata* metadata, internal::ParseContext* ctx) {
  return ParseMessageSetItemTmpl<MessageLite, std::string>(ptr, extendee,
                                                           metadata, ctx);
}

bool ExtensionSet::FieldTypeIsPointer(FieldType type) {
  return type == WireFormatLite::TYPE_STRING ||
         type == WireFormatLite::TYPE_BYTES ||
         type == WireFormatLite::TYPE_GROUP ||
         type == WireFormatLite::TYPE_MESSAGE;
}

uint8_t* ExtensionSet::_InternalSerializeImpl(
    const MessageLite* extendee, int start_field_number, int end_field_number,
    uint8_t* target, io::EpsCopyOutputStream* stream) const {
  if (ABSL_PREDICT_FALSE(is_large())) {
    return _InternalSerializeImplLarge(extendee, start_field_number,
                                       end_field_number, target, stream);
  }
  const KeyValue* end = flat_end();
  const KeyValue* it = flat_begin();
  while (it != end && it->first < start_field_number) ++it;
  for (; it != end && it->first < end_field_number; ++it) {
    target = it->second.InternalSerializeFieldWithCachedSizesToArray(
        extendee, this, it->first, target, stream);
  }
  return target;
}

uint8_t* ExtensionSet::_InternalSerializeAllImpl(
    const MessageLite* extendee, uint8_t* target,
    io::EpsCopyOutputStream* stream) const {
  ForEach(
      [&target, extendee, stream, this](int number, const Extension& ext) {
        target = ext.InternalSerializeFieldWithCachedSizesToArray(
            extendee, this, number, target, stream);
      },
      Prefetch{});
  return target;
}

uint8_t* ExtensionSet::_InternalSerializeImplLarge(
    const MessageLite* extendee, int start_field_number, int end_field_number,
    uint8_t* target, io::EpsCopyOutputStream* stream) const {
  assert(is_large());
  const auto& end = map_.large->end();
  for (auto it = map_.large->lower_bound(start_field_number);
       it != end && it->first < end_field_number; ++it) {
    target = it->second.InternalSerializeFieldWithCachedSizesToArray(
        extendee, this, it->first, target, stream);
  }
  return target;
}

uint8_t* ExtensionSet::InternalSerializeMessageSetWithCachedSizesToArray(
    const MessageLite* extendee, uint8_t* target,
    io::EpsCopyOutputStream* stream) const {
  const ExtensionSet* extension_set = this;
  ForEach(
      [&target, extendee, stream, extension_set](int number,
                                                 const Extension& ext) {
        target = ext.InternalSerializeMessageSetItemWithCachedSizesToArray(
            extendee, extension_set, number, target, stream);
      },
      Prefetch{});
  return target;
}

size_t ExtensionSet::ByteSize() const {
  size_t total_size = 0;
  ForEach(
      [&total_size](int number, const Extension& ext) {
        total_size += ext.ByteSize(number);
      },
      Prefetch{});
  return total_size;
}


// Defined in extension_set_heavy.cc.
// int ExtensionSet::SpaceUsedExcludingSelf() const

bool ExtensionSet::MaybeNewExtension(int number,
                                     const FieldDescriptor* descriptor,
                                     Extension** result) {
  bool extension_is_new = false;
  std::tie(*result, extension_is_new) = Insert(number);
  (*result)->descriptor = descriptor;
  return extension_is_new;
}

ExtensionSet::Extension& ExtensionSet::FindOrCreate(
    int number, FieldType type, bool repeated, bool packed,
    const FieldDescriptor* descriptor,
    Extension& (*pointer_creator)(Extension& ext, Arena* arena)) {
  Extension* extension;
  if (MaybeNewExtension(number, descriptor, &extension)) {
    extension->type = type;
    extension->is_repeated = repeated;
    extension->is_packed = packed;
    extension->is_pointer = pointer_creator != nullptr;
    if (pointer_creator != nullptr) {
      return pointer_creator(*extension, arena_);
    }
  } else {
    extension->is_cleared = false;
  }
  return *extension;
}

// ===================================================================
// Methods of ExtensionSet::Extension

void ExtensionSet::Extension::Clear() {
  if (is_repeated) {
    switch (cpp_type(type)) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)      \
  case WireFormatLite::CPPTYPE_##UPPERCASE:    \
    ptr.repeated_##LOWERCASE##_value->Clear(); \
    break

      HANDLE_TYPE(INT32, int32_t);
      HANDLE_TYPE(INT64, int64_t);
      HANDLE_TYPE(UINT32, uint32_t);
      HANDLE_TYPE(UINT64, uint64_t);
      HANDLE_TYPE(FLOAT, float);
      HANDLE_TYPE(DOUBLE, double);
      HANDLE_TYPE(BOOL, bool);
      HANDLE_TYPE(ENUM, int32_t);
      HANDLE_TYPE(STRING, string);
      HANDLE_TYPE(MESSAGE, message);
#undef HANDLE_TYPE
    }
  } else {
    if (!is_cleared) {
      switch (cpp_type(type)) {
        case WireFormatLite::CPPTYPE_STRING:
          ptr.string_value->clear();
          break;
        case WireFormatLite::CPPTYPE_MESSAGE:
          if (is_lazy) {
            ptr.lazymessage_value->Clear();
          } else {
            ptr.message_value->Clear();
          }
          break;
        default:
          // No need to do anything.  Get*() will return the default value
          // as long as is_cleared is true and Set*() will overwrite the
          // previous value.
          break;
      }

      is_cleared = true;
    }
  }
}

size_t ExtensionSet::Extension::ByteSize(int number) const {
  size_t result = 0;

  if (is_repeated) {
    if (is_packed) {
      switch (real_type(type)) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                     \
  case WireFormatLite::TYPE_##UPPERCASE:                                 \
    for (int i = 0; i < ptr.repeated_##LOWERCASE##_value->size(); i++) { \
      result += WireFormatLite::CAMELCASE##Size(                         \
          ptr.repeated_##LOWERCASE##_value->Get(i));                     \
    }                                                                    \
    break

        HANDLE_TYPE(INT32, Int32, int32_t);
        HANDLE_TYPE(INT64, Int64, int64_t);
        HANDLE_TYPE(UINT32, UInt32, uint32_t);
        HANDLE_TYPE(UINT64, UInt64, uint64_t);
        HANDLE_TYPE(SINT32, SInt32, int32_t);
        HANDLE_TYPE(SINT64, SInt64, int64_t);
        HANDLE_TYPE(ENUM, Enum, int32_t);
#undef HANDLE_TYPE

        // Stuff with fixed size.
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                 \
  case WireFormatLite::TYPE_##UPPERCASE:                             \
    result += WireFormatLite::k##CAMELCASE##Size *                   \
              FromIntSize(ptr.repeated_##LOWERCASE##_value->size()); \
    break
        HANDLE_TYPE(FIXED32, Fixed32, uint32_t);
        HANDLE_TYPE(FIXED64, Fixed64, uint64_t);
        HANDLE_TYPE(SFIXED32, SFixed32, int32_t);
        HANDLE_TYPE(SFIXED64, SFixed64, int64_t);
        HANDLE_TYPE(FLOAT, Float, float);
        HANDLE_TYPE(DOUBLE, Double, double);
        HANDLE_TYPE(BOOL, Bool, bool);
#undef HANDLE_TYPE

        case WireFormatLite::TYPE_STRING:
        case WireFormatLite::TYPE_BYTES:
        case WireFormatLite::TYPE_GROUP:
        case WireFormatLite::TYPE_MESSAGE:
          ABSL_LOG(FATAL) << "Non-primitive types can't be packed.";
          break;
      }

      cached_size.set(ToCachedSize(result));
      if (result > 0) {
        result += io::CodedOutputStream::VarintSize32(result);
        result += io::CodedOutputStream::VarintSize32(WireFormatLite::MakeTag(
            number, WireFormatLite::WIRETYPE_LENGTH_DELIMITED));
      }
    } else {
      size_t tag_size = WireFormatLite::TagSize(number, real_type(type));

      switch (real_type(type)) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                      \
  case WireFormatLite::TYPE_##UPPERCASE:                                  \
    result +=                                                             \
        tag_size * FromIntSize(ptr.repeated_##LOWERCASE##_value->size()); \
    for (int i = 0; i < ptr.repeated_##LOWERCASE##_value->size(); i++) {  \
      result += WireFormatLite::CAMELCASE##Size(                          \
          ptr.repeated_##LOWERCASE##_value->Get(i));                      \
    }                                                                     \
    break

        HANDLE_TYPE(INT32, Int32, int32_t);
        HANDLE_TYPE(INT64, Int64, int64_t);
        HANDLE_TYPE(UINT32, UInt32, uint32_t);
        HANDLE_TYPE(UINT64, UInt64, uint64_t);
        HANDLE_TYPE(SINT32, SInt32, int32_t);
        HANDLE_TYPE(SINT64, SInt64, int64_t);
        HANDLE_TYPE(STRING, String, string);
        HANDLE_TYPE(BYTES, Bytes, string);
        HANDLE_TYPE(ENUM, Enum, int32_t);
        HANDLE_TYPE(GROUP, Group, message);
        HANDLE_TYPE(MESSAGE, Message, message);
#undef HANDLE_TYPE

        // Stuff with fixed size.
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                 \
  case WireFormatLite::TYPE_##UPPERCASE:                             \
    result += (tag_size + WireFormatLite::k##CAMELCASE##Size) *      \
              FromIntSize(ptr.repeated_##LOWERCASE##_value->size()); \
    break
        HANDLE_TYPE(FIXED32, Fixed32, uint32_t);
        HANDLE_TYPE(FIXED64, Fixed64, uint64_t);
        HANDLE_TYPE(SFIXED32, SFixed32, int32_t);
        HANDLE_TYPE(SFIXED64, SFixed64, int64_t);
        HANDLE_TYPE(FLOAT, Float, float);
        HANDLE_TYPE(DOUBLE, Double, double);
        HANDLE_TYPE(BOOL, Bool, bool);
#undef HANDLE_TYPE
      }
    }
  } else if (!is_cleared) {
    result += WireFormatLite::TagSize(number, real_type(type));
    switch (real_type(type)) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)      \
  case WireFormatLite::TYPE_##UPPERCASE:                  \
    result += WireFormatLite::CAMELCASE##Size(LOWERCASE); \
    break

      HANDLE_TYPE(INT32, Int32, int32_t_value);
      HANDLE_TYPE(INT64, Int64, int64_t_value);
      HANDLE_TYPE(UINT32, UInt32, uint32_t_value);
      HANDLE_TYPE(UINT64, UInt64, uint64_t_value);
      HANDLE_TYPE(SINT32, SInt32, int32_t_value);
      HANDLE_TYPE(SINT64, SInt64, int64_t_value);
      HANDLE_TYPE(STRING, String, *ptr.string_value);
      HANDLE_TYPE(BYTES, Bytes, *ptr.string_value);
      HANDLE_TYPE(ENUM, Enum, int32_t_value);
      HANDLE_TYPE(GROUP, Group, *ptr.message_value);
#undef HANDLE_TYPE
      case WireFormatLite::TYPE_MESSAGE: {
        result += WireFormatLite::LengthDelimitedSize(
            is_lazy ? ptr.lazymessage_value->ByteSizeLong()
                    : ptr.message_value->ByteSizeLong());
        break;
      }

      // Stuff with fixed size.
#define HANDLE_TYPE(UPPERCASE, CAMELCASE)         \
  case WireFormatLite::TYPE_##UPPERCASE:          \
    result += WireFormatLite::k##CAMELCASE##Size; \
    break
        HANDLE_TYPE(FIXED32, Fixed32);
        HANDLE_TYPE(FIXED64, Fixed64);
        HANDLE_TYPE(SFIXED32, SFixed32);
        HANDLE_TYPE(SFIXED64, SFixed64);
        HANDLE_TYPE(FLOAT, Float);
        HANDLE_TYPE(DOUBLE, Double);
        HANDLE_TYPE(BOOL, Bool);
#undef HANDLE_TYPE
    }
  }

  return result;
}

int ExtensionSet::Extension::GetSize() const {
  ABSL_DCHECK(is_repeated);
  switch (cpp_type(type)) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)   \
  case WireFormatLite::CPPTYPE_##UPPERCASE: \
    return ptr.repeated_##LOWERCASE##_value->size()

    HANDLE_TYPE(INT32, int32_t);
    HANDLE_TYPE(INT64, int64_t);
    HANDLE_TYPE(UINT32, uint32_t);
    HANDLE_TYPE(UINT64, uint64_t);
    HANDLE_TYPE(FLOAT, float);
    HANDLE_TYPE(DOUBLE, double);
    HANDLE_TYPE(BOOL, bool);
    HANDLE_TYPE(ENUM, int32_t);
    HANDLE_TYPE(STRING, string);
    HANDLE_TYPE(MESSAGE, message);
#undef HANDLE_TYPE
  }

  ABSL_LOG(FATAL) << "Can't get here.";
  return 0;
}

// This function deletes all allocated objects. This function should be only
// called if the Extension was created without an arena.
void ExtensionSet::Extension::Free() {
  if (is_repeated) {
    switch (cpp_type(type)) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)    \
  case WireFormatLite::CPPTYPE_##UPPERCASE:  \
    delete ptr.repeated_##LOWERCASE##_value; \
    break

      HANDLE_TYPE(INT32, int32_t);
      HANDLE_TYPE(INT64, int64_t);
      HANDLE_TYPE(UINT32, uint32_t);
      HANDLE_TYPE(UINT64, uint64_t);
      HANDLE_TYPE(FLOAT, float);
      HANDLE_TYPE(DOUBLE, double);
      HANDLE_TYPE(BOOL, bool);
      HANDLE_TYPE(ENUM, int32_t);
      HANDLE_TYPE(STRING, string);
      HANDLE_TYPE(MESSAGE, message);
#undef HANDLE_TYPE
    }
  } else {
    switch (cpp_type(type)) {
      case WireFormatLite::CPPTYPE_STRING:
        delete ptr.string_value;
        break;
      case WireFormatLite::CPPTYPE_MESSAGE:
        if (is_lazy) {
          delete ptr.lazymessage_value;
        } else {
          delete ptr.message_value;
        }
        break;
      default:
        break;
    }
  }
}

// Defined in extension_set_heavy.cc.
// int ExtensionSet::Extension::SpaceUsedExcludingSelf() const

bool ExtensionSet::Extension::IsInitialized(const ExtensionSet* ext_set,
                                            const MessageLite* extendee,
                                            int number, Arena* arena) const {
  if (cpp_type(type) != WireFormatLite::CPPTYPE_MESSAGE) return true;

  if (is_repeated) {
    for (int i = 0; i < ptr.repeated_message_value->size(); i++) {
      if (!ptr.repeated_message_value->Get(i).IsInitialized()) {
        return false;
      }
    }
    return true;
  }

  if (is_cleared) return true;

  if (!is_lazy) return ptr.message_value->IsInitialized();

  const MessageLite* prototype =
      ext_set->GetPrototypeForLazyMessage(extendee, number);
  ABSL_DCHECK_NE(prototype, nullptr)
      << "extendee: " << extendee->GetTypeName() << "; number: " << number;
  return ptr.lazymessage_value->IsInitialized(prototype, arena);
}

// Dummy key method to avoid weak vtable.
void ExtensionSet::LazyMessageExtension::UnusedKeyMethod() {}

const ExtensionSet::Extension* ExtensionSet::FindOrNull(int key) const {
  if (flat_size_ == 0) {
    return nullptr;
  } else if (ABSL_PREDICT_TRUE(!is_large())) {
    for (auto it = flat_begin(), end = flat_end();
         it != end && it->first <= key; ++it) {
      if (it->first == key) return &it->second;
    }
    return nullptr;
  } else {
    return FindOrNullInLargeMap(key);
  }
}

const ExtensionSet::Extension* ExtensionSet::FindOrNullInLargeMap(
    int key) const {
  assert(is_large());
  LargeMap::const_iterator it = map_.large->find(key);
  if (it != map_.large->end()) {
    return &it->second;
  }
  return nullptr;
}

ExtensionSet::Extension* ExtensionSet::FindOrNull(int key) {
  const auto* const_this = this;
  return const_cast<ExtensionSet::Extension*>(const_this->FindOrNull(key));
}

ExtensionSet::Extension* ExtensionSet::FindOrNullInLargeMap(int key) {
  const auto* const_this = this;
  return const_cast<ExtensionSet::Extension*>(
      const_this->FindOrNullInLargeMap(key));
}

ABSL_ATTRIBUTE_NOINLINE
std::pair<ExtensionSet::Extension*, bool>
ExtensionSet::InternalInsertIntoLargeMap(int key) {
  ABSL_DCHECK(is_large());
  auto maybe = map_.large->insert({key, Extension()});
  return {&maybe.first->second, maybe.second};
}

std::pair<ExtensionSet::Extension*, bool> ExtensionSet::Insert(int key) {
  if (ABSL_PREDICT_FALSE(is_large())) {
    return InternalInsertIntoLargeMap(key);
  }
  uint16_t i = flat_size_;
  KeyValue* flat = map_.flat;
  // Iterating from the back to benefit the case where the keys are inserted in
  // increasing order.
  for (; i > 0; --i) {
    int map_key = flat[i - 1].first;
    if (map_key == key) {
      return {&flat[i - 1].second, false};
    }
    if (map_key < key) {
      break;
    }
  }
  if (flat_size_ == flat_capacity_) {
    GrowCapacity(flat_size_ + 1);
    if (ABSL_PREDICT_FALSE(is_large())) {
      return InternalInsertIntoLargeMap(key);
    }
    flat = map_.flat;  // Reload flat pointer after GrowCapacity.
  }

  std::copy_backward(flat + i, flat + flat_size_, flat + flat_size_ + 1);
  ++flat_size_;
  flat[i].first = key;
  flat[i].second = Extension();
  return {&flat[i].second, true};
}

void ExtensionSet::GrowCapacity(size_t minimum_new_capacity) {
  if (ABSL_PREDICT_FALSE(is_large())) {
    return;  // LargeMap does not have a "reserve" method.
  }
  if (flat_capacity_ >= minimum_new_capacity) {
    return;
  }

  auto new_flat_capacity = flat_capacity_;
  do {
    new_flat_capacity = new_flat_capacity == 0 ? 1 : new_flat_capacity * 4;
  } while (new_flat_capacity < minimum_new_capacity);

  KeyValue* begin = flat_begin();
  KeyValue* end = flat_end();
  AllocatedData new_map;
  Arena* const arena = arena_;
  if (new_flat_capacity > kMaximumFlatCapacity) {
    new_map.large = Arena::Create<LargeMap>(arena);
    LargeMap::iterator hint = new_map.large->begin();
    for (const KeyValue* it = begin; it != end; ++it) {
      hint = new_map.large->insert(hint, {it->first, it->second});
    }
    flat_size_ = static_cast<uint16_t>(-1);
    ABSL_DCHECK(is_large());
  } else {
    new_map.flat = AllocateFlatMap(arena, new_flat_capacity);
    std::copy(begin, end, new_map.flat);
  }

  if (flat_capacity_ > 0) {
    if (arena == nullptr) {
      DeleteFlatMap(begin, flat_capacity_);
    } else {
      arena->ReturnArrayMemory(begin, sizeof(KeyValue) * flat_capacity_);
    }
  }
  flat_capacity_ = new_flat_capacity;
  map_ = new_map;
}

void ExtensionSet::InternalReserveSmallCapacityFromEmpty(
    size_t minimum_new_capacity) {
  ABSL_DCHECK(flat_capacity_ == 0);
  ABSL_DCHECK(minimum_new_capacity <= kMaximumFlatCapacity);
  ABSL_DCHECK(minimum_new_capacity > 0);
  const size_t new_flat_capacity = absl::bit_ceil(minimum_new_capacity);
  flat_capacity_ = new_flat_capacity;
  map_.flat = AllocateFlatMap(arena_, new_flat_capacity);
}

#if (__cplusplus < 201703) && \
    (!defined(_MSC_VER) || (_MSC_VER >= 1900 && _MSC_VER < 1912))
// static
constexpr uint16_t ExtensionSet::kMaximumFlatCapacity;
#endif  //  (__cplusplus < 201703) && (!defined(_MSC_VER) || (_MSC_VER >= 1900
        //  && _MSC_VER < 1912))

void ExtensionSet::Erase(int key) {
  if (ABSL_PREDICT_FALSE(is_large())) {
    map_.large->erase(key);
    return;
  }
  KeyValue* end = flat_end();
  for (KeyValue* it = flat_begin(); it != end && it->first <= key; ++it) {
    if (it->first == key) {
      std::copy(it + 1, end, it);
      --flat_size_;
      return;
    }
  }
}

// ==================================================================
// Default repeated field instances for iterator-compatible accessors

const RepeatedPrimitiveDefaults* RepeatedPrimitiveDefaults::default_instance() {
  static auto instance = OnShutdownDelete(new RepeatedPrimitiveDefaults);
  return instance;
}

const RepeatedStringTypeTraits::RepeatedFieldType*
RepeatedStringTypeTraits::GetDefaultRepeatedField() {
  static auto instance = OnShutdownDelete(new RepeatedFieldType);
  return instance;
}

uint8_t* ExtensionSet::Extension::InternalSerializeFieldWithCachedSizesToArray(
    const MessageLite* extendee, const ExtensionSet* extension_set, int number,
    uint8_t* target, io::EpsCopyOutputStream* stream) const {
  if (is_repeated) {
    if (is_packed) {
      if (cached_size() == 0) return target;

      target = stream->EnsureSpace(target);
      target = WireFormatLite::WriteTagToArray(
          number, WireFormatLite::WIRETYPE_LENGTH_DELIMITED, target);
      target = WireFormatLite::WriteInt32NoTagToArray(cached_size(), target);

      switch (real_type(type)) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                     \
  case WireFormatLite::TYPE_##UPPERCASE:                                 \
    for (int i = 0; i < ptr.repeated_##LOWERCASE##_value->size(); i++) { \
      target = stream->EnsureSpace(target);                              \
      target = WireFormatLite::Write##CAMELCASE##NoTagToArray(           \
          ptr.repeated_##LOWERCASE##_value->Get(i), target);             \
    }                                                                    \
    break

        HANDLE_TYPE(INT32, Int32, int32_t);
        HANDLE_TYPE(INT64, Int64, int64_t);
        HANDLE_TYPE(UINT32, UInt32, uint32_t);
        HANDLE_TYPE(UINT64, UInt64, uint64_t);
        HANDLE_TYPE(SINT32, SInt32, int32_t);
        HANDLE_TYPE(SINT64, SInt64, int64_t);
        HANDLE_TYPE(FIXED32, Fixed32, uint32_t);
        HANDLE_TYPE(FIXED64, Fixed64, uint64_t);
        HANDLE_TYPE(SFIXED32, SFixed32, int32_t);
        HANDLE_TYPE(SFIXED64, SFixed64, int64_t);
        HANDLE_TYPE(FLOAT, Float, float);
        HANDLE_TYPE(DOUBLE, Double, double);
        HANDLE_TYPE(BOOL, Bool, bool);
        HANDLE_TYPE(ENUM, Enum, int32_t);
#undef HANDLE_TYPE

        case WireFormatLite::TYPE_STRING:
        case WireFormatLite::TYPE_BYTES:
        case WireFormatLite::TYPE_GROUP:
        case WireFormatLite::TYPE_MESSAGE:
          ABSL_LOG(FATAL) << "Non-primitive types can't be packed.";
          break;
      }
    } else {
      switch (real_type(type)) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                     \
  case WireFormatLite::TYPE_##UPPERCASE:                                 \
    for (int i = 0; i < ptr.repeated_##LOWERCASE##_value->size(); i++) { \
      target = stream->EnsureSpace(target);                              \
      target = WireFormatLite::Write##CAMELCASE##ToArray(                \
          number, ptr.repeated_##LOWERCASE##_value->Get(i), target);     \
    }                                                                    \
    break

        HANDLE_TYPE(INT32, Int32, int32_t);
        HANDLE_TYPE(INT64, Int64, int64_t);
        HANDLE_TYPE(UINT32, UInt32, uint32_t);
        HANDLE_TYPE(UINT64, UInt64, uint64_t);
        HANDLE_TYPE(SINT32, SInt32, int32_t);
        HANDLE_TYPE(SINT64, SInt64, int64_t);
        HANDLE_TYPE(FIXED32, Fixed32, uint32_t);
        HANDLE_TYPE(FIXED64, Fixed64, uint64_t);
        HANDLE_TYPE(SFIXED32, SFixed32, int32_t);
        HANDLE_TYPE(SFIXED64, SFixed64, int64_t);
        HANDLE_TYPE(FLOAT, Float, float);
        HANDLE_TYPE(DOUBLE, Double, double);
        HANDLE_TYPE(BOOL, Bool, bool);
        HANDLE_TYPE(ENUM, Enum, int32_t);
#undef HANDLE_TYPE
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, LOWERCASE)                     \
  case WireFormatLite::TYPE_##UPPERCASE:                                 \
    for (int i = 0; i < ptr.repeated_##LOWERCASE##_value->size(); i++) { \
      target = stream->EnsureSpace(target);                              \
      target = stream->WriteString(                                      \
          number, ptr.repeated_##LOWERCASE##_value->Get(i), target);     \
    }                                                                    \
    break
        HANDLE_TYPE(STRING, String, string);
        HANDLE_TYPE(BYTES, Bytes, string);
#undef HANDLE_TYPE
        case WireFormatLite::TYPE_GROUP:
          for (int i = 0; i < ptr.repeated_message_value->size(); i++) {
            target = stream->EnsureSpace(target);
            target = WireFormatLite::InternalWriteGroup(
                number, ptr.repeated_message_value->Get(i), target, stream);
          }
          break;
        case WireFormatLite::TYPE_MESSAGE:
          for (int i = 0; i < ptr.repeated_message_value->size(); i++) {
            auto& msg = ptr.repeated_message_value->Get(i);
            target = WireFormatLite::InternalWriteMessage(
                number, msg, msg.GetCachedSize(), target, stream);
          }
          break;
      }
    }
  } else if (!is_cleared) {
    switch (real_type(type)) {
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, VALUE)                               \
  case WireFormatLite::TYPE_##UPPERCASE:                                       \
    target = stream->EnsureSpace(target);                                      \
    target = WireFormatLite::Write##CAMELCASE##ToArray(number, VALUE, target); \
    break

      HANDLE_TYPE(INT32, Int32, int32_t_value);
      HANDLE_TYPE(INT64, Int64, int64_t_value);
      HANDLE_TYPE(UINT32, UInt32, uint32_t_value);
      HANDLE_TYPE(UINT64, UInt64, uint64_t_value);
      HANDLE_TYPE(SINT32, SInt32, int32_t_value);
      HANDLE_TYPE(SINT64, SInt64, int64_t_value);
      HANDLE_TYPE(FIXED32, Fixed32, uint32_t_value);
      HANDLE_TYPE(FIXED64, Fixed64, uint64_t_value);
      HANDLE_TYPE(SFIXED32, SFixed32, int32_t_value);
      HANDLE_TYPE(SFIXED64, SFixed64, int64_t_value);
      HANDLE_TYPE(FLOAT, Float, float_value);
      HANDLE_TYPE(DOUBLE, Double, double_value);
      HANDLE_TYPE(BOOL, Bool, bool_value);
      HANDLE_TYPE(ENUM, Enum, int32_t_value);
#undef HANDLE_TYPE
#define HANDLE_TYPE(UPPERCASE, CAMELCASE, VALUE)         \
  case WireFormatLite::TYPE_##UPPERCASE:                 \
    target = stream->EnsureSpace(target);                \
    target = stream->WriteString(number, VALUE, target); \
    break
      HANDLE_TYPE(STRING, String, *ptr.string_value);
      HANDLE_TYPE(BYTES, Bytes, *ptr.string_value);
#undef HANDLE_TYPE
      case WireFormatLite::TYPE_GROUP:
        target = stream->EnsureSpace(target);
        target = WireFormatLite::InternalWriteGroup(number, *ptr.message_value,
                                                    target, stream);
        break;
      case WireFormatLite::TYPE_MESSAGE:
        if (is_lazy) {
          const auto* prototype =
              extension_set->GetPrototypeForLazyMessage(extendee, number);
          target = ptr.lazymessage_value->WriteMessageToArray(prototype, number,
                                                              target, stream);
        } else {
          target = WireFormatLite::InternalWriteMessage(
              number, *ptr.message_value, ptr.message_value->GetCachedSize(),
              target, stream);
        }
        break;
    }
  }
  return target;
}

const MessageLite* ExtensionSet::GetPrototypeForLazyMessage(
    const MessageLite* extendee, int number) const {
  GeneratedExtensionFinder finder(extendee);
  bool was_packed_on_wire = false;
  ExtensionInfo extension_info;
  if (!FindExtensionInfoFromFieldNumber(
          WireFormatLite::WireType::WIRETYPE_LENGTH_DELIMITED, number, &finder,
          &extension_info, &was_packed_on_wire)) {
    return nullptr;
  }
  return extension_info.message_info.prototype;
}

uint8_t*
ExtensionSet::Extension::InternalSerializeMessageSetItemWithCachedSizesToArray(
    const MessageLite* extendee, const ExtensionSet* extension_set, int number,
    uint8_t* target, io::EpsCopyOutputStream* stream) const {
  if (type != WireFormatLite::TYPE_MESSAGE || is_repeated) {
    // Not a valid MessageSet extension, but serialize it the normal way.
    ABSL_LOG(WARNING) << "Invalid message set extension.";
    return InternalSerializeFieldWithCachedSizesToArray(extendee, extension_set,
                                                        number, target, stream);
  }

  if (is_cleared) return target;

  target = stream->EnsureSpace(target);
  // Start group.
  target = io::CodedOutputStream::WriteTagToArray(
      WireFormatLite::kMessageSetItemStartTag, target);
  // Write type ID.
  target = WireFormatLite::WriteUInt32ToArray(
      WireFormatLite::kMessageSetTypeIdNumber, number, target);
  // Write message.
  if (is_lazy) {
    const auto* prototype =
        extension_set->GetPrototypeForLazyMessage(extendee, number);
    target = ptr.lazymessage_value->WriteMessageToArray(
        prototype, WireFormatLite::kMessageSetMessageNumber, target, stream);
  } else {
    target = WireFormatLite::InternalWriteMessage(
        WireFormatLite::kMessageSetMessageNumber, *ptr.message_value,
        ptr.message_value->GetCachedSize(), target, stream);
  }
  // End group.
  target = stream->EnsureSpace(target);
  target = io::CodedOutputStream::WriteTagToArray(
      WireFormatLite::kMessageSetItemEndTag, target);
  return target;
}

size_t ExtensionSet::Extension::MessageSetItemByteSize(int number) const {
  if (type != WireFormatLite::TYPE_MESSAGE || is_repeated) {
    // Not a valid MessageSet extension, but compute the byte size for it the
    // normal way.
    return ByteSize(number);
  }

  if (is_cleared) return 0;

  size_t our_size = WireFormatLite::kMessageSetItemTagsSize;

  // type_id
  our_size += io::CodedOutputStream::VarintSize32(number);

  // message
  our_size += WireFormatLite::LengthDelimitedSize(
      is_lazy ? ptr.lazymessage_value->ByteSizeLong()
              : ptr.message_value->ByteSizeLong());

  return our_size;
}

size_t ExtensionSet::MessageSetByteSize() const {
  size_t total_size = 0;
  ForEach(
      [&total_size](int number, const Extension& ext) {
        total_size += ext.MessageSetItemByteSize(number);
      },
      Prefetch{});
  return total_size;
}

LazyEagerVerifyFnType FindExtensionLazyEagerVerifyFn(
    const MessageLite* extendee, int number) {
  const ExtensionInfo* registered = FindRegisteredExtension(extendee, number);
  if (registered != nullptr) {
    return registered->lazy_eager_verify_func;
  }
  return nullptr;
}

std::atomic<ExtensionSet::LazyMessageExtension* (*)(Arena* arena)>
    ExtensionSet::maybe_create_lazy_extension_;

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
