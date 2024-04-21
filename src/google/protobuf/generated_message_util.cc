// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/generated_message_util.h"

#include <atomic>
#include <cstdint>
#include <limits>

#include "google/protobuf/arenastring.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/wire_format_lite.h"

// Must be included last
#include "google/protobuf/port_def.inc"

PROTOBUF_PRAGMA_INIT_SEG


namespace google {
namespace protobuf {
namespace internal {

void DestroyMessage(const void* message) {
  static_cast<const MessageLite*>(message)->~MessageLite();
}
void DestroyString(const void* s) {
  static_cast<const std::string*>(s)->~basic_string();
}

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 ExplicitlyConstructedArenaString
        fixed_address_empty_string{};  // NOLINT


PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT const EmptyCord empty_cord_;

#if defined(PROTOBUF_DESCRIPTOR_WEAK_MESSAGES_ALLOWED)

// We add a single dummy entry to guarantee the section is never empty.
struct DummyWeakDefault {
  const Message* m;
  WeakDescriptorDefaultTail tail;
};
DummyWeakDefault dummy_weak_default __attribute__((section("pb_defaults"))) = {
    nullptr, {&dummy_weak_default.m, sizeof(dummy_weak_default)}};

extern "C" {
// When using --descriptor_implicit_weak_messages we expect the default instance
// objects to live in the `pb_defaults` section. We load them all using the
// __start/__end symbols provided by the linker.
// Each object is its own type and size, so we use a `char` to load them
// appropriately.
extern const char __start_pb_defaults;
extern const char __stop_pb_defaults;
}
static void InitWeakDefaults() {
  // force link the dummy entry.
  StrongPointer<DummyWeakDefault*, &dummy_weak_default>();
  // We don't know the size of each object, but we know the layout of the tail.
  // It contains a WeakDescriptorDefaultTail object.
  // As such, we iterate the section backwards.
  const char* start = &__start_pb_defaults;
  const char* end = &__stop_pb_defaults;
  while (start != end) {
    auto* tail = reinterpret_cast<const WeakDescriptorDefaultTail*>(end) - 1;
    end -= tail->size;
    const Message* instance = reinterpret_cast<const Message*>(end);
    *tail->target = instance;
  }
}
#else
void InitWeakDefaults() {}
#endif

PROTOBUF_CONSTINIT std::atomic<bool> init_protobuf_defaults_state{false};
static bool InitProtobufDefaultsImpl() {
  fixed_address_empty_string.DefaultConstruct();
  OnShutdownDestroyString(fixed_address_empty_string.get_mutable());
  InitWeakDefaults();


  init_protobuf_defaults_state.store(true, std::memory_order_release);
  return true;
}

void InitProtobufDefaultsSlow() {
  static bool is_inited = InitProtobufDefaultsImpl();
  (void)is_inited;
}
// Force the initialization of the empty string.
// Normally, registration would do it, but we don't have any guarantee that
// there is any object with reflection.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 static std::true_type init_empty_string =
    (InitProtobufDefaultsSlow(), std::true_type{});

size_t StringSpaceUsedExcludingSelfLong(const std::string& str) {
  const void* start = &str;
  const void* end = &str + 1;
  if (start <= str.data() && str.data() < end) {
    // The string's data is stored inside the string object itself.
    return 0;
  } else {
    return str.capacity();
  }
}

template <typename T>
const T& Get(const void* ptr) {
  return *static_cast<const T*>(ptr);
}

// PrimitiveTypeHelper is a wrapper around the interface of WireFormatLite.
// WireFormatLite has a very inconvenient interface with respect to template
// meta-programming. This class wraps the different named functions into
// a single Serialize / SerializeToArray interface.
template <int type>
struct PrimitiveTypeHelper;

template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_BOOL> {
  typedef bool Type;
  static void Serialize(const void* ptr, io::CodedOutputStream* output) {
    WireFormatLite::WriteBoolNoTag(Get<bool>(ptr), output);
  }
  static uint8_t* SerializeToArray(const void* ptr, uint8_t* buffer) {
    return WireFormatLite::WriteBoolNoTagToArray(Get<Type>(ptr), buffer);
  }
};

template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_INT32> {
  typedef int32_t Type;
  static void Serialize(const void* ptr, io::CodedOutputStream* output) {
    WireFormatLite::WriteInt32NoTag(Get<int32_t>(ptr), output);
  }
  static uint8_t* SerializeToArray(const void* ptr, uint8_t* buffer) {
    return WireFormatLite::WriteInt32NoTagToArray(Get<Type>(ptr), buffer);
  }
};

template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_SINT32> {
  typedef int32_t Type;
  static void Serialize(const void* ptr, io::CodedOutputStream* output) {
    WireFormatLite::WriteSInt32NoTag(Get<int32_t>(ptr), output);
  }
  static uint8_t* SerializeToArray(const void* ptr, uint8_t* buffer) {
    return WireFormatLite::WriteSInt32NoTagToArray(Get<Type>(ptr), buffer);
  }
};

template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_UINT32> {
  typedef uint32_t Type;
  static void Serialize(const void* ptr, io::CodedOutputStream* output) {
    WireFormatLite::WriteUInt32NoTag(Get<uint32_t>(ptr), output);
  }
  static uint8_t* SerializeToArray(const void* ptr, uint8_t* buffer) {
    return WireFormatLite::WriteUInt32NoTagToArray(Get<Type>(ptr), buffer);
  }
};
template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_INT64> {
  typedef int64_t Type;
  static void Serialize(const void* ptr, io::CodedOutputStream* output) {
    WireFormatLite::WriteInt64NoTag(Get<int64_t>(ptr), output);
  }
  static uint8_t* SerializeToArray(const void* ptr, uint8_t* buffer) {
    return WireFormatLite::WriteInt64NoTagToArray(Get<Type>(ptr), buffer);
  }
};

template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_SINT64> {
  typedef int64_t Type;
  static void Serialize(const void* ptr, io::CodedOutputStream* output) {
    WireFormatLite::WriteSInt64NoTag(Get<int64_t>(ptr), output);
  }
  static uint8_t* SerializeToArray(const void* ptr, uint8_t* buffer) {
    return WireFormatLite::WriteSInt64NoTagToArray(Get<Type>(ptr), buffer);
  }
};
template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_UINT64> {
  typedef uint64_t Type;
  static void Serialize(const void* ptr, io::CodedOutputStream* output) {
    WireFormatLite::WriteUInt64NoTag(Get<uint64_t>(ptr), output);
  }
  static uint8_t* SerializeToArray(const void* ptr, uint8_t* buffer) {
    return WireFormatLite::WriteUInt64NoTagToArray(Get<Type>(ptr), buffer);
  }
};

template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_FIXED32> {
  typedef uint32_t Type;
  static void Serialize(const void* ptr, io::CodedOutputStream* output) {
    WireFormatLite::WriteFixed32NoTag(Get<uint32_t>(ptr), output);
  }
  static uint8_t* SerializeToArray(const void* ptr, uint8_t* buffer) {
    return WireFormatLite::WriteFixed32NoTagToArray(Get<Type>(ptr), buffer);
  }
};

template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_FIXED64> {
  typedef uint64_t Type;
  static void Serialize(const void* ptr, io::CodedOutputStream* output) {
    WireFormatLite::WriteFixed64NoTag(Get<uint64_t>(ptr), output);
  }
  static uint8_t* SerializeToArray(const void* ptr, uint8_t* buffer) {
    return WireFormatLite::WriteFixed64NoTagToArray(Get<Type>(ptr), buffer);
  }
};

template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_ENUM>
    : PrimitiveTypeHelper<WireFormatLite::TYPE_INT32> {};

template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_SFIXED32>
    : PrimitiveTypeHelper<WireFormatLite::TYPE_FIXED32> {
  typedef int32_t Type;
};
template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_SFIXED64>
    : PrimitiveTypeHelper<WireFormatLite::TYPE_FIXED64> {
  typedef int64_t Type;
};
template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_FLOAT>
    : PrimitiveTypeHelper<WireFormatLite::TYPE_FIXED32> {
  typedef float Type;
};
template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_DOUBLE>
    : PrimitiveTypeHelper<WireFormatLite::TYPE_FIXED64> {
  typedef double Type;
};

template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_STRING> {
  typedef std::string Type;
  static void Serialize(const void* ptr, io::CodedOutputStream* output) {
    const Type& value = *static_cast<const Type*>(ptr);
    output->WriteVarint32(value.size());
    output->WriteRawMaybeAliased(value.data(), value.size());
  }
  static uint8_t* SerializeToArray(const void* ptr, uint8_t* buffer) {
    const Type& value = *static_cast<const Type*>(ptr);
    return io::CodedOutputStream::WriteStringWithSizeToArray(value, buffer);
  }
};

template <>
struct PrimitiveTypeHelper<WireFormatLite::TYPE_BYTES>
    : PrimitiveTypeHelper<WireFormatLite::TYPE_STRING> {};

// We want to serialize to both CodedOutputStream and directly into byte arrays
// without duplicating the code. In fact we might want extra output channels in
// the future.
template <typename O, int type>
struct OutputHelper;

template <int type, typename O>
void SerializeTo(const void* ptr, O* output) {
  OutputHelper<O, type>::Serialize(ptr, output);
}

template <typename O>
void WriteTagTo(uint32_t tag, O* output) {
  SerializeTo<WireFormatLite::TYPE_UINT32>(&tag, output);
}

template <typename O>
void WriteLengthTo(uint32_t length, O* output) {
  SerializeTo<WireFormatLite::TYPE_UINT32>(&length, output);
}

// Specialization for coded output stream
template <int type>
struct OutputHelper<io::CodedOutputStream, type> {
  static void Serialize(const void* ptr, io::CodedOutputStream* output) {
    PrimitiveTypeHelper<type>::Serialize(ptr, output);
  }
};

// Specialization for writing into a plain array
struct ArrayOutput {
  uint8_t* ptr;
  bool is_deterministic;
};

template <int type>
struct OutputHelper<ArrayOutput, type> {
  static void Serialize(const void* ptr, ArrayOutput* output) {
    output->ptr = PrimitiveTypeHelper<type>::SerializeToArray(ptr, output->ptr);
  }
};

void SerializeMessageNoTable(const MessageLite* msg,
                             io::CodedOutputStream* output) {
  msg->SerializeWithCachedSizes(output);
}

void SerializeMessageNoTable(const MessageLite* msg, ArrayOutput* output) {
  io::ArrayOutputStream array_stream(output->ptr, INT_MAX);
  io::CodedOutputStream o(&array_stream);
  o.SetSerializationDeterministic(output->is_deterministic);
  msg->SerializeWithCachedSizes(&o);
  output->ptr += o.ByteCount();
}

// We need to use a helper class to get access to the private members
class AccessorHelper {
 public:
  static int Size(const RepeatedPtrFieldBase& x) { return x.size(); }
  static void const* Get(const RepeatedPtrFieldBase& x, int idx) {
    return x.raw_data()[idx];
  }
};

void SerializeNotImplemented(int field) {
  ABSL_LOG(FATAL) << "Not implemented field number " << field;
}

// When switching to c++11 we should make these constexpr functions
#define SERIALIZE_TABLE_OP(type, type_class) \
  ((type - 1) + static_cast<int>(type_class) * FieldMetadata::kNumTypes)

template <int type>
bool IsNull(const void* ptr) {
  return *static_cast<const typename PrimitiveTypeHelper<type>::Type*>(ptr) ==
         0;
}

template <>
bool IsNull<WireFormatLite::TYPE_STRING>(const void* ptr) {
  return static_cast<const ArenaStringPtr*>(ptr)->Get().size() == 0;
}

template <>
bool IsNull<WireFormatLite::TYPE_BYTES>(const void* ptr) {
  return static_cast<const ArenaStringPtr*>(ptr)->Get().size() == 0;
}

template <>
bool IsNull<WireFormatLite::TYPE_GROUP>(const void* ptr) {
  return Get<const MessageLite*>(ptr) == nullptr;
}

template <>
bool IsNull<WireFormatLite::TYPE_MESSAGE>(const void* ptr) {
  return Get<const MessageLite*>(ptr) == nullptr;
}

void ExtensionSerializer(const MessageLite* extendee, const uint8_t* ptr,
                         uint32_t offset, uint32_t tag, uint32_t has_offset,
                         io::CodedOutputStream* output) {
  reinterpret_cast<const ExtensionSet*>(ptr + offset)
      ->SerializeWithCachedSizes(extendee, tag, has_offset, output);
}

void UnknownFieldSerializerLite(const uint8_t* ptr, uint32_t offset,
                                uint32_t /*tag*/, uint32_t /*has_offset*/,
                                io::CodedOutputStream* output) {
  output->WriteString(
      reinterpret_cast<const InternalMetadata*>(ptr + offset)
          ->unknown_fields<std::string>(&internal::GetEmptyString));
}

MessageLite* DuplicateIfNonNullInternal(MessageLite* message) {
  if (message) {
    MessageLite* ret = message->New();
    ret->CheckTypeAndMergeFrom(*message);
    return ret;
  } else {
    return nullptr;
  }
}

void GenericSwap(MessageLite* m1, MessageLite* m2) {
  std::unique_ptr<MessageLite> tmp(m1->New());
  tmp->CheckTypeAndMergeFrom(*m1);
  m1->Clear();
  m1->CheckTypeAndMergeFrom(*m2);
  m2->Clear();
  m2->CheckTypeAndMergeFrom(*tmp);
}

// Returns a message owned by this Arena.  This may require Own()ing or
// duplicating the message.
MessageLite* GetOwnedMessageInternal(Arena* message_arena,
                                     MessageLite* submessage,
                                     Arena* submessage_arena) {
  ABSL_DCHECK(submessage->GetArena() == submessage_arena);
  ABSL_DCHECK(message_arena != submessage_arena);
  ABSL_DCHECK_EQ(submessage_arena, nullptr);
  if (message_arena != nullptr && submessage_arena == nullptr) {
    message_arena->Own(submessage);
    return submessage;
  } else {
    MessageLite* ret = submessage->New(message_arena);
    ret->CheckTypeAndMergeFrom(*submessage);
    return ret;
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
