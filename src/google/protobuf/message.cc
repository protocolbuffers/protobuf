// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/message.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>  // IWYU pragma: keep for operator new().
#include <optional>
#include <queue>
#include <string>
#include <vector>

#include "absl/base/call_once.h"
#include "absl/base/optimization.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/hash/hash.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/map_field.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/reflection_internal.h"
#include "google/protobuf/reflection_ops.h"
#include "google/protobuf/reflection_visit_fields.h"
#include "google/protobuf/unknown_field_set.h"
#include "google/protobuf/wire_format.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// TODO make this factorized better. This should not have to hop
// to reflection. Currently uses GeneratedMessageReflection and thus is
// defined in generated_message_reflection.cc
void RegisterFileLevelMetadata(const DescriptorTable* descriptor_table);

}  // namespace internal

using internal::ReflectionOps;
using internal::WireFormat;

void Message::MergeImpl(MessageLite& to, const MessageLite& from) {
  ReflectionOps::Merge(DownCastMessage<Message>(from),
                       DownCastMessage<Message>(&to));
}

void Message::ClearImpl() {
  ReflectionOps::Clear(DownCastMessage<Message>(this));
}

size_t Message::ByteSizeLongImpl(const MessageLite& msg) {
  auto& _this = DownCastMessage<Message>(msg);
  size_t size = WireFormat::ByteSize(_this);
  _this.AccessCachedSize().Set(internal::ToCachedSize(size));
  return size;
}

uint8_t* Message::_InternalSerializeImpl(const MessageLite& msg,
                                         uint8_t* target,
                                         io::EpsCopyOutputStream* stream) {
  return WireFormat::_InternalSerialize(DownCastMessage<Message>(msg), target,
                                        stream);
}

void Message::MergeFrom(const Message& from) {
  auto* class_to = GetClassData();
  auto* class_from = from.GetClassData();
  if (class_to == nullptr || class_to != class_from) {
    ReflectionOps::Merge(from, this);
  } else {
    class_to->full().merge_to_from(*this, from);
  }
}

void Message::CopyFrom(const Message& from) {
  if (&from == this) return;

  auto* class_to = GetClassData();
  auto* class_from = from.GetClassData();

  if (class_from != nullptr && class_from == class_to) {
    // Fail if "from" is a descendant of "to" as such copy is not allowed.
    ABSL_DCHECK(!internal::IsDescendant(*this, from))
        << "Source of CopyFrom cannot be a descendant of the target.";
    Clear();
    class_to->full().merge_to_from(*this, from);
  } else {
    const Descriptor* descriptor = GetDescriptor();
    ABSL_CHECK_EQ(from.GetDescriptor(), descriptor)
        << ": Tried to copy from a message with a different type. "
           "to: "
        << descriptor->full_name()
        << ", "
           "from: "
        << from.GetDescriptor()->full_name();
    ReflectionOps::Copy(from, this);
  }
}

#if !defined(PROTOBUF_CUSTOM_VTABLE)
void Message::Clear() { ReflectionOps::Clear(this); }
#endif  // !PROTOBUF_CUSTOM_VTABLE

bool Message::IsInitializedImpl(const MessageLite& msg) {
  return ReflectionOps::IsInitialized(DownCastMessage<Message>(msg));
}

void Message::FindInitializationErrors(std::vector<std::string>* errors) const {
  return ReflectionOps::FindInitializationErrors(*this, "", errors);
}

std::string Message::InitializationErrorString() const {
  std::vector<std::string> errors;
  FindInitializationErrors(&errors);
  return absl::StrJoin(errors, ", ");
}

void Message::CheckInitialized() const {
  ABSL_CHECK(IsInitialized())
      << "Message of type \"" << GetDescriptor()->full_name()
      << "\" is missing required fields: " << InitializationErrorString();
}

void Message::DiscardUnknownFields() {
  return ReflectionOps::DiscardUnknownFields(this);
}

Metadata Message::GetMetadata() const {
  return GetMetadataImpl(GetClassData()->full());
}

Metadata Message::GetMetadataImpl(const internal::ClassDataFull& data) {
  auto* table = data.descriptor_table;
  // Only codegen types provide a table. DynamicMessage does not provide a table
  // and instead eagerly initializes the descriptor/reflection members.
  if (ABSL_PREDICT_TRUE(table != nullptr)) {
    if (ABSL_PREDICT_FALSE(data.get_metadata_tracker != nullptr)) {
      data.get_metadata_tracker();
    }
    absl::call_once(*table->once, [table] {
      internal::AssignDescriptorsOnceInnerCall(table);
    });
  }
  return {data.descriptor, data.reflection};
}

#if !defined(PROTOBUF_CUSTOM_VTABLE)
uint8_t* Message::_InternalSerialize(uint8_t* target,
                                     io::EpsCopyOutputStream* stream) const {
  return WireFormat::_InternalSerialize(*this, target, stream);
}

size_t Message::ByteSizeLong() const {
  size_t size = WireFormat::ByteSize(*this);
  AccessCachedSize().Set(internal::ToCachedSize(size));
  return size;
}
#endif  // !PROTOBUF_CUSTOM_VTABLE

size_t Message::ComputeUnknownFieldsSize(
    size_t total_size, const internal::CachedSize* cached_size) const {
  total_size += WireFormat::ComputeUnknownFieldsSize(
      _internal_metadata_.unknown_fields<UnknownFieldSet>(
          UnknownFieldSet::default_instance));
  cached_size->Set(internal::ToCachedSize(total_size));
  return total_size;
}

size_t Message::MaybeComputeUnknownFieldsSize(
    size_t total_size, const internal::CachedSize* cached_size) const {
  if (ABSL_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    return ComputeUnknownFieldsSize(total_size, cached_size);
  }
  cached_size->Set(internal::ToCachedSize(total_size));
  return total_size;
}


size_t Message::SpaceUsedLong() const {
  return GetClassData()->full().descriptor_methods->space_used_long(*this);
}

absl::string_view Message::GetTypeNameImpl(const internal::ClassData* data) {
  return GetMetadataImpl(data->full()).descriptor->full_name();
}

static std::string InitializationErrorStringImpl(const MessageLite& msg) {
  return DownCastMessage<Message>(msg).InitializationErrorString();
}

const internal::TcParseTableBase* Message::GetTcParseTableImpl(
    const MessageLite& msg) {
  return DownCastMessage<Message>(msg).GetReflection()->GetTcParseTable();
}

size_t Message::SpaceUsedLongImpl(const MessageLite& msg_lite) {
  auto& msg = DownCastMessage<Message>(msg_lite);
  return msg.GetReflection()->SpaceUsedLong(msg);
}

static std::string DebugStringImpl(const MessageLite& msg) {
  return DownCastMessage<Message>(msg).DebugString();
}

PROTOBUF_CONSTINIT const internal::DescriptorMethods
    Message::kDescriptorMethods = {
        GetTypeNameImpl,     InitializationErrorStringImpl,
        GetTcParseTableImpl, SpaceUsedLongImpl,
        DebugStringImpl,
};

namespace internal {
void* CreateSplitMessageGeneric(Arena* arena, const void* default_split,
                                size_t size, const void* message,
                                const void* default_message) {
  ABSL_DCHECK_NE(message, default_message);
  void* split =
      (arena == nullptr) ? ::operator new(size) : arena->AllocateAligned(size);
  memcpy(split, default_split, size);
  return split;
}
}  // namespace internal

// =============================================================================
// MessageFactory

MessageFactory::~MessageFactory() = default;

namespace {

class GeneratedMessageFactory final : public MessageFactory {
 public:
  static GeneratedMessageFactory* singleton();

  void RegisterFile(const google::protobuf::internal::DescriptorTable* table);
  void RegisterType(const Descriptor* descriptor, const Message* prototype);

  const Message* TryGetPrototype(const Descriptor* type);

  // implements MessageFactory ---------------------------------------
  const Message* GetPrototype(const Descriptor* type) override;

 private:
  GeneratedMessageFactory() {
    dropped_defaults_factory_.SetDelegateToGeneratedFactory(true);
  }

  std::optional<const Message*> FindInTypeMap(const Descriptor* type)
      ABSL_SHARED_LOCKS_REQUIRED(mutex_)
  {
    auto it = type_map_.find(type);
    if (it == type_map_.end()) return std::nullopt;
    return it->second.get();
  }

  const google::protobuf::internal::DescriptorTable* FindInFileMap(
      absl::string_view name) {
    auto it = files_.find(name);
    if (it == files_.end()) return nullptr;
    return *it;
  }

  struct DescriptorByNameHash {
    using is_transparent = void;
    size_t operator()(const google::protobuf::internal::DescriptorTable* t) const {
      return absl::HashOf(absl::string_view{t->filename});
    }

    size_t operator()(absl::string_view name) const {
      return absl::HashOf(name);
    }
  };
  struct DescriptorByNameEq {
    using is_transparent = void;
    bool operator()(const google::protobuf::internal::DescriptorTable* lhs,
                    const google::protobuf::internal::DescriptorTable* rhs) const {
      return lhs == rhs || (*this)(lhs->filename, rhs->filename);
    }
    bool operator()(absl::string_view lhs,
                    const google::protobuf::internal::DescriptorTable* rhs) const {
      return (*this)(lhs, rhs->filename);
    }
    bool operator()(const google::protobuf::internal::DescriptorTable* lhs,
                    absl::string_view rhs) const {
      return (*this)(lhs->filename, rhs);
    }
    bool operator()(absl::string_view lhs, absl::string_view rhs) const {
      return lhs == rhs;
    }
  };

  // Only written at static init time, so does not require locking.
  absl::flat_hash_set<const google::protobuf::internal::DescriptorTable*,
                      DescriptorByNameHash, DescriptorByNameEq>
      files_;
  DynamicMessageFactory dropped_defaults_factory_;

  absl::Mutex mutex_;
  class MessagePtr {
   public:
    MessagePtr() : value_() {}
    explicit MessagePtr(const Message* msg) : value_(msg) {}
    const Message* get() const { return value_; }
    void set(const Message* msg) { value_ = msg; }

   private:
    const Message* value_;
  };
  absl::flat_hash_map<const Descriptor*, MessagePtr> type_map_
      ABSL_GUARDED_BY(mutex_);
};

GeneratedMessageFactory* GeneratedMessageFactory::singleton() {
  static auto instance =
      internal::OnShutdownDelete(new GeneratedMessageFactory);
  return instance;
}

void GeneratedMessageFactory::RegisterFile(
    const google::protobuf::internal::DescriptorTable* table) {
  if (!files_.insert(table).second) {
    ABSL_LOG(FATAL) << "File is already registered: " << table->filename;
  }
}

void GeneratedMessageFactory::RegisterType(const Descriptor* descriptor,
                                           const Message* prototype) {
  ABSL_DCHECK_EQ(descriptor->file()->pool(), DescriptorPool::generated_pool())
      << "Tried to register a non-generated type with the generated "
         "type registry.";

  // This should only be called as a result of calling a file registration
  // function during GetPrototype(), in which case we already have locked
  // the mutex.
  mutex_.AssertHeld();
  if (!type_map_.try_emplace(descriptor, prototype).second) {
    ABSL_DLOG(FATAL) << "Type is already registered: "
                     << descriptor->full_name();
  }
}


const Message* GeneratedMessageFactory::GetPrototype(const Descriptor* type) {
  const Message* result = TryGetPrototype(type);
  if (result == nullptr &&
      type->file()->pool() == DescriptorPool::generated_pool()) {
    // We registered this descriptor with a null pointer.
    // In this case we need to create the prototype from the dynamic factory.
    // We _must_ do this outside the lock because the dynamic factory will call
    // back into the generated factory for cross linking.
    result = dropped_defaults_factory_.GetPrototype(type);

    {
      absl::WriterMutexLock lock(&mutex_);
      // And update the main map to make the next lookup faster.
      // We don't need to recheck here. Even if someone raced us here the result
      // is the same, so we can just write it.
      type_map_[type].set(result);
    }
  }

  return result;
}

const Message* GeneratedMessageFactory::TryGetPrototype(
    const Descriptor* type) {
  std::optional<const Message*> result;
  {
    absl::ReaderMutexLock lock(&mutex_);
    result = FindInTypeMap(type);
    if (result.has_value() && *result != nullptr) {
      return *result;
    }
  }

  // If the type is not in the generated pool, then we can't possibly handle
  // it.
  if (type->file()->pool() != DescriptorPool::generated_pool()) return nullptr;

  // Apparently the file hasn't been registered yet.  Let's do that now.
  const internal::DescriptorTable* registration_data =
      FindInFileMap(type->file()->name());
  if (registration_data == nullptr) {
    ABSL_DLOG(FATAL) << "File appears to be in generated pool but wasn't "
                        "registered: "
                     << type->file()->name();
    return nullptr;
  }

  {
    absl::WriterMutexLock lock(&mutex_);

    // Check if another thread preempted us.
    result = FindInTypeMap(type);
    if (!result.has_value()) {
      // Nope.  OK, register everything.
      internal::RegisterFileLevelMetadata(registration_data);
      // Should be here now.
      result = FindInTypeMap(type);
      ABSL_DCHECK(result.has_value());
    }
  }

  return *result;
}

}  // namespace

const Message* MessageFactory::TryGetGeneratedPrototype(
    const Descriptor* type) {
  return GeneratedMessageFactory::singleton()->TryGetPrototype(type);
}

MessageFactory* MessageFactory::generated_factory() {
  return GeneratedMessageFactory::singleton();
}

void MessageFactory::InternalRegisterGeneratedFile(
    const google::protobuf::internal::DescriptorTable* table) {
  GeneratedMessageFactory::singleton()->RegisterFile(table);
}

void MessageFactory::InternalRegisterGeneratedMessage(
    const Descriptor* descriptor, const Message* prototype) {
  GeneratedMessageFactory::singleton()->RegisterType(descriptor, prototype);
}


namespace {
template <typename T>
T* GetSingleton() {
  static T singleton;
  return &singleton;
}
}  // namespace

const internal::RepeatedFieldAccessor* Reflection::RepeatedFieldAccessor(
    const FieldDescriptor* field) const {
  ABSL_CHECK(field->is_repeated());
  switch (field->cpp_type()) {
#define HANDLE_PRIMITIVE_TYPE(TYPE, type) \
  case FieldDescriptor::CPPTYPE_##TYPE:   \
    return GetSingleton<internal::RepeatedFieldPrimitiveAccessor<type> >();
    HANDLE_PRIMITIVE_TYPE(INT32, int32_t)
    HANDLE_PRIMITIVE_TYPE(UINT32, uint32_t)
    HANDLE_PRIMITIVE_TYPE(INT64, int64_t)
    HANDLE_PRIMITIVE_TYPE(UINT64, uint64_t)
    HANDLE_PRIMITIVE_TYPE(FLOAT, float)
    HANDLE_PRIMITIVE_TYPE(DOUBLE, double)
    HANDLE_PRIMITIVE_TYPE(BOOL, bool)
    HANDLE_PRIMITIVE_TYPE(ENUM, int32_t)
#undef HANDLE_PRIMITIVE_TYPE
    case FieldDescriptor::CPPTYPE_STRING:
      switch (field->cpp_string_type()) {
        case FieldDescriptor::CppStringType::kCord:
          ABSL_LOG(FATAL) << "Repeated cords are not supported.";
        case FieldDescriptor::CppStringType::kView:
        case FieldDescriptor::CppStringType::kString:
          return GetSingleton<internal::RepeatedPtrFieldStringAccessor>();
      }
      break;
    case FieldDescriptor::CPPTYPE_MESSAGE:
      if (field->is_map()) {
        return GetSingleton<internal::MapFieldAccessor>();
      } else {
        return GetSingleton<internal::RepeatedPtrFieldMessageAccessor>();
      }
  }
  ABSL_LOG(FATAL) << "Should not reach here.";
  return nullptr;
}

namespace internal {
template void InternalMetadata::DoClear<UnknownFieldSet>();
template void InternalMetadata::DoMergeFrom<UnknownFieldSet>(
    const UnknownFieldSet& other);
template void InternalMetadata::DoSwap<UnknownFieldSet>(UnknownFieldSet* other);
template void InternalMetadata::DeleteOutOfLineHelper<UnknownFieldSet>();
template UnknownFieldSet*
InternalMetadata::mutable_unknown_fields_slow<UnknownFieldSet>();
}  // namespace internal


}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
