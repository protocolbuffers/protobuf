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

#include <iostream>
#include <stack>
#include <unordered_map>

#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>

#include <google/protobuf/stubs/casts.h>
#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/parse_context.h>
#include <google/protobuf/reflection_internal.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/map_field.h>
#include <google/protobuf/map_field_inl.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/stubs/strutil.h>

#include <google/protobuf/stubs/map_util.h>
#include <google/protobuf/stubs/stl_util.h>
#include <google/protobuf/stubs/hash.h>

#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {

#if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
using internal::ParseClosure;
#endif
using internal::ReflectionOps;
using internal::WireFormat;
using internal::WireFormatLite;

void Message::MergeFrom(const Message& from) {
  const Descriptor* descriptor = GetDescriptor();
  GOOGLE_CHECK_EQ(from.GetDescriptor(), descriptor)
      << ": Tried to merge from a message with a different type.  "
         "to: "
      << descriptor->full_name()
      << ", "
         "from: "
      << from.GetDescriptor()->full_name();
  ReflectionOps::Merge(from, this);
}

void Message::CheckTypeAndMergeFrom(const MessageLite& other) {
  MergeFrom(*down_cast<const Message*>(&other));
}

void Message::CopyFrom(const Message& from) {
  const Descriptor* descriptor = GetDescriptor();
  GOOGLE_CHECK_EQ(from.GetDescriptor(), descriptor)
      << ": Tried to copy from a message with a different type. "
         "to: "
      << descriptor->full_name()
      << ", "
         "from: "
      << from.GetDescriptor()->full_name();
  ReflectionOps::Copy(from, this);
}

string Message::GetTypeName() const { return GetDescriptor()->full_name(); }

void Message::Clear() { ReflectionOps::Clear(this); }

bool Message::IsInitialized() const {
  return ReflectionOps::IsInitialized(*this);
}

void Message::FindInitializationErrors(std::vector<string>* errors) const {
  return ReflectionOps::FindInitializationErrors(*this, "", errors);
}

string Message::InitializationErrorString() const {
  std::vector<string> errors;
  FindInitializationErrors(&errors);
  return Join(errors, ", ");
}

void Message::CheckInitialized() const {
  GOOGLE_CHECK(IsInitialized()) << "Message of type \"" << GetDescriptor()->full_name()
                         << "\" is missing required fields: "
                         << InitializationErrorString();
}

void Message::DiscardUnknownFields() {
  return ReflectionOps::DiscardUnknownFields(this);
}

#if !GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
bool Message::MergePartialFromCodedStream(io::CodedInputStream* input) {
  return WireFormat::ParseAndMergePartial(input, this);
}
#endif

bool Message::ParseFromFileDescriptor(int file_descriptor) {
  io::FileInputStream input(file_descriptor);
  return ParseFromZeroCopyStream(&input) && input.GetErrno() == 0;
}

bool Message::ParsePartialFromFileDescriptor(int file_descriptor) {
  io::FileInputStream input(file_descriptor);
  return ParsePartialFromZeroCopyStream(&input) && input.GetErrno() == 0;
}

bool Message::ParseFromIstream(std::istream* input) {
  io::IstreamInputStream zero_copy_input(input);
  return ParseFromZeroCopyStream(&zero_copy_input) && input->eof();
}

bool Message::ParsePartialFromIstream(std::istream* input) {
  io::IstreamInputStream zero_copy_input(input);
  return ParsePartialFromZeroCopyStream(&zero_copy_input) && input->eof();
}

#if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
namespace internal {

class ReflectionAccessor {
 public:
  static void* GetOffset(void* msg, const google::protobuf::FieldDescriptor* f,
                         const google::protobuf::Reflection* r) {
    return static_cast<char*>(msg) + CheckedCast(r)->schema_.GetFieldOffset(f);
  }

  static ExtensionSet* GetExtensionSet(void* msg, const google::protobuf::Reflection* r) {
    return reinterpret_cast<ExtensionSet*>(
        static_cast<char*>(msg) +
        CheckedCast(r)->schema_.GetExtensionSetOffset());
  }
  static InternalMetadataWithArena* GetMetadata(void* msg,
                                                const google::protobuf::Reflection* r) {
    return reinterpret_cast<InternalMetadataWithArena*>(
        static_cast<char*>(msg) + CheckedCast(r)->schema_.GetMetadataOffset());
  }
  static void* GetRepeatedEnum(const Reflection* reflection,
                               const FieldDescriptor* field, Message* msg) {
    return reflection->MutableRawRepeatedField(
        msg, field, FieldDescriptor::CPPTYPE_ENUM, 0, nullptr);
  }

 private:
  static const GeneratedMessageReflection* CheckedCast(const Reflection* r) {
    auto gr = dynamic_cast<const GeneratedMessageReflection*>(r);
    GOOGLE_CHECK(gr != nullptr);
    return gr;
  }
};

}  // namespace internal

void SetField(uint64 val, const FieldDescriptor* field, Message* msg,
              const Reflection* reflection) {
#define STORE_TYPE(CPPTYPE_METHOD)                        \
  do                                                      \
    if (field->is_repeated()) {                           \
      reflection->Add##CPPTYPE_METHOD(msg, field, value); \
    } else {                                              \
      reflection->Set##CPPTYPE_METHOD(msg, field, value); \
    }                                                     \
  while (0)

  switch (field->type()) {
#define HANDLE_TYPE(TYPE, CPPTYPE, CPPTYPE_METHOD) \
  case FieldDescriptor::TYPE_##TYPE: {             \
    CPPTYPE value = val;                           \
    STORE_TYPE(CPPTYPE_METHOD);                    \
    break;                                         \
  }

    // Varints
    HANDLE_TYPE(INT32, int32, Int32)
    HANDLE_TYPE(INT64, int64, Int64)
    HANDLE_TYPE(UINT32, uint32, UInt32)
    HANDLE_TYPE(UINT64, uint64, UInt64)
    case FieldDescriptor::TYPE_SINT32: {
      int32 value = WireFormatLite::ZigZagDecode32(val);
      STORE_TYPE(Int32);
      break;
    }
    case FieldDescriptor::TYPE_SINT64: {
      int64 value = WireFormatLite::ZigZagDecode64(val);
      STORE_TYPE(Int64);
      break;
    }
      HANDLE_TYPE(BOOL, bool, Bool)

      // Fixed
      HANDLE_TYPE(FIXED32, uint32, UInt32)
      HANDLE_TYPE(FIXED64, uint64, UInt64)
      HANDLE_TYPE(SFIXED32, int32, Int32)
      HANDLE_TYPE(SFIXED64, int64, Int64)

    case FieldDescriptor::TYPE_FLOAT: {
      float value;
      uint32 bit_rep = val;
      std::memcpy(&value, &bit_rep, sizeof(value));
      STORE_TYPE(Float);
      break;
    }
    case FieldDescriptor::TYPE_DOUBLE: {
      double value;
      uint64 bit_rep = val;
      std::memcpy(&value, &bit_rep, sizeof(value));
      STORE_TYPE(Double);
      break;
    }
    case FieldDescriptor::TYPE_ENUM: {
      int value = val;
      if (field->is_repeated()) {
        reflection->AddEnumValue(msg, field, value);
      } else {
        reflection->SetEnumValue(msg, field, value);
      }
      break;
    }
    default:
      GOOGLE_LOG(FATAL) << "Error in descriptors, primitve field with field type "
                 << field->type();
  }
#undef STORE_TYPE
#undef HANDLE_TYPE
}

bool ReflectiveValidator(const void* arg, int val) {
  auto d = static_cast<const EnumDescriptor*>(arg);
  return d->FindValueByNumber(val) != nullptr;
}

ParseClosure GetPackedField(const FieldDescriptor* field, Message* msg,
                            const Reflection* reflection,
                            internal::ParseContext* ctx) {
  switch (field->type()) {
#define HANDLE_PACKED_TYPE(TYPE, CPPTYPE, METHOD_NAME) \
  case FieldDescriptor::TYPE_##TYPE:                   \
    return {internal::Packed##METHOD_NAME##Parser,     \
            reflection->MutableRepeatedField<CPPTYPE>(msg, field)}
    HANDLE_PACKED_TYPE(INT32, int32, Int32);
    HANDLE_PACKED_TYPE(INT64, int64, Int64);
    HANDLE_PACKED_TYPE(SINT32, int32, SInt32);
    HANDLE_PACKED_TYPE(SINT64, int64, SInt64);
    HANDLE_PACKED_TYPE(UINT32, uint32, UInt32);
    HANDLE_PACKED_TYPE(UINT64, uint64, UInt64);
    HANDLE_PACKED_TYPE(BOOL, bool, Bool);
    case FieldDescriptor::TYPE_ENUM: {
      auto object =
          internal::ReflectionAccessor::GetRepeatedEnum(reflection, field, msg);
      if (field->file()->syntax() == FileDescriptor::SYNTAX_PROTO3) {
        return {internal::PackedEnumParser, object};
      } else {
        ctx->extra_parse_data().SetEnumValidatorArg(
            ReflectiveValidator, field->enum_type(),
            reflection->MutableUnknownFields(msg), field->number());
        return {internal::PackedValidEnumParserArg, object};
      }
    }
      HANDLE_PACKED_TYPE(FIXED32, uint32, Fixed32);
      HANDLE_PACKED_TYPE(FIXED64, uint64, Fixed64);
      HANDLE_PACKED_TYPE(SFIXED32, int32, SFixed32);
      HANDLE_PACKED_TYPE(SFIXED64, int64, SFixed64);
      HANDLE_PACKED_TYPE(FLOAT, float, Float);
      HANDLE_PACKED_TYPE(DOUBLE, double, Double);
#undef HANDLE_PACKED_TYPE

    default:
      GOOGLE_LOG(FATAL) << "Type is not packable " << field->type();
      return {};  // Make compiler happy
  }
}

ParseClosure GetLenDelim(int field_number, const FieldDescriptor* field,
                         Message* msg, const Reflection* reflection,
                         internal::ParseContext* ctx) {
  if (WireFormat::WireTypeForFieldType(field->type()) !=
      WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    GOOGLE_DCHECK(field->is_packable());
    return GetPackedField(field, msg, reflection, ctx);
  }
  enum { kNone = 0, kVerify, kStrict } utf8_level = kNone;
  internal::ParseFunc string_parsers[] = {internal::StringParser,
                                          internal::StringParserUTF8Verify,
                                          internal::StringParserUTF8};
  switch (field->type()) {
    case FieldDescriptor::TYPE_STRING:
      if (field->file()->syntax() == FileDescriptor::SYNTAX_PROTO3
      ) {
        ctx->extra_parse_data().SetFieldName(field->full_name().c_str());
        utf8_level = kStrict;
      } else if (
          true) {
        ctx->extra_parse_data().SetFieldName(field->full_name().c_str());
        utf8_level = kVerify;
      }
      PROTOBUF_FALLTHROUGH_INTENDED;
    case FieldDescriptor::TYPE_BYTES: {
      if (field->is_repeated()) {
        int index = reflection->FieldSize(*msg, field);
        // Add new empty value.
        reflection->AddString(msg, field, "");
        if (field->options().ctype() == FieldOptions::STRING ||
            field->is_extension()) {
          auto object = reflection->MutableRepeatedPtrField<string>(msg, field)
                            ->Mutable(index);
          return {string_parsers[utf8_level], object};
        } else {
          auto object = reflection->MutableRepeatedPtrField<string>(msg, field)
                            ->Mutable(index);
          return {string_parsers[utf8_level], object};
        }
      } else {
        // Clear value and make sure it's set.
        reflection->SetString(msg, field, "");
        if (field->options().ctype() == FieldOptions::STRING ||
            field->is_extension()) {
          // HACK around inability to get mutable_string in reflection
          string* object = &const_cast<string&>(
              reflection->GetStringReference(*msg, field, nullptr));
          return {string_parsers[utf8_level], object};
        } else {
          // HACK around inability to get mutable_string in reflection
          string* object = &const_cast<string&>(
              reflection->GetStringReference(*msg, field, nullptr));
          return {string_parsers[utf8_level], object};
        }
      }
      GOOGLE_LOG(FATAL) << "No other type than string supported";
    }
    case FieldDescriptor::TYPE_MESSAGE: {
      Message* object;
      auto factory = ctx->extra_parse_data().factory;
      if (field->is_repeated()) {
        object = reflection->AddMessage(msg, field, factory);
      } else {
        object = reflection->MutableMessage(msg, field, factory);
      }
      return {object->_ParseFunc(), object};
    }
    default:
      GOOGLE_LOG(FATAL) << "Wrong type for length delim " << field->type();
  }
  return {};  // Make compiler happy.
}

ParseClosure GetGroup(int field_number, const FieldDescriptor* field,
                      Message* msg, const Reflection* reflection) {
  Message* object;
  if (field->is_repeated()) {
    object = reflection->AddMessage(msg, field, nullptr);
  } else {
    object = reflection->MutableMessage(msg, field, nullptr);
  }
  return {object->_ParseFunc(), object};
}

const char* Message::_InternalParse(const char* begin, const char* end,
                                    void* object, internal::ParseContext* ctx) {
  class ReflectiveFieldParser {
   public:
    ReflectiveFieldParser(Message* msg, internal::ParseContext* ctx)
        : ReflectiveFieldParser(msg, ctx, false) {}

    void AddVarint(uint32 num, uint64 value) {
      if (is_item_ && num == 2) {
        if (!ctx_->extra_parse_data().payload.empty()) {
          auto field = Field(value, 2);
          if (field && field->message_type()) {
            auto child = reflection_->MutableMessage(msg_, field);
            // TODO(gerbens) signal error
            child->ParsePartialFromString(ctx_->extra_parse_data().payload);
          } else {
            MutableUnknown()->AddLengthDelimited(value)->swap(
                ctx_->extra_parse_data().payload);
          }
          return;
        }
        ctx_->extra_parse_data().field_number = value;
        return;
      }
      auto field = Field(num, 0);
      if (field) {
        SetField(value, field, msg_, reflection_);
      } else {
        MutableUnknown()->AddVarint(num, value);
      }
    }
    void AddFixed64(uint32 num, uint64 value) {
      auto field = Field(num, 1);
      if (field) {
        SetField(value, field, msg_, reflection_);
      } else {
        MutableUnknown()->AddFixed64(num, value);
      }
    }
    ParseClosure AddLengthDelimited(uint32 num, uint32) {
      if (is_item_ && num == 3) {
        int type_id = ctx_->extra_parse_data().field_number;
        if (type_id == 0) {
          return {internal::StringParser, &ctx_->extra_parse_data().payload};
        }
        ctx_->extra_parse_data().field_number = 0;
        num = type_id;
      }
      auto field = Field(num, 2);
      if (field) {
        return GetLenDelim(num, field, msg_, reflection_, ctx_);
      } else {
        return {internal::StringParser,
                MutableUnknown()->AddLengthDelimited(num)};
      }
    }
    ParseClosure StartGroup(uint32 num) {
      if (!is_item_ && descriptor_->options().message_set_wire_format() &&
          num == 1) {
        ctx_->extra_parse_data().payload.clear();
        ctx_->extra_parse_data().field_number = 0;
        return {ItemParser, msg_};
      }
      auto field = Field(num, 3);
      if (field) {
        return GetGroup(num, field, msg_, reflection_);
      } else {
        return {internal::UnknownGroupParse, MutableUnknown()->AddGroup(num)};
      }
    }
    void EndGroup(uint32 num) {}
    void AddFixed32(uint32 num, uint32 value) {
      auto field = Field(num, 5);
      if (field) {
        SetField(value, field, msg_, reflection_);
      } else {
        MutableUnknown()->AddFixed32(num, value);
      }
    }

   private:
    Message* msg_;
    const Descriptor* descriptor_;
    const Reflection* reflection_;
    internal::ParseContext* ctx_;
    UnknownFieldSet* unknown_ = nullptr;
    bool is_item_ = false;

    ReflectiveFieldParser(Message* msg, internal::ParseContext* ctx,
                          bool is_item)
        : msg_(msg),
          descriptor_(msg->GetDescriptor()),
          reflection_(msg->GetReflection()),
          ctx_(ctx),
          is_item_(is_item) {
      GOOGLE_CHECK(descriptor_) << msg->GetTypeName();
      GOOGLE_CHECK(reflection_) << msg->GetTypeName();
    }

    const FieldDescriptor* Field(int num, int wire_type) {
      auto field = descriptor_->FindFieldByNumber(num);

      // If that failed, check if the field is an extension.
      if (field == nullptr && descriptor_->IsExtensionNumber(num)) {
        const DescriptorPool* pool = ctx_->extra_parse_data().pool;
        if (pool == NULL) {
          field = reflection_->FindKnownExtensionByNumber(num);
        } else {
          field = pool->FindExtensionByNumber(descriptor_, num);
        }
      }
      if (field == nullptr) return nullptr;

      if (internal::WireFormat::WireTypeForFieldType(field->type()) !=
          wire_type) {
        if (field->is_packable()) {
          if (wire_type ==
              internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
            return field;
          }
        }
        return nullptr;
      }
      return field;
    }

    UnknownFieldSet* MutableUnknown() {
      if (unknown_) return unknown_;
      return unknown_ = reflection_->MutableUnknownFields(msg_);
    }

    static const char* ItemParser(const char* begin, const char* end,
                                  void* object, internal::ParseContext* ctx) {
      auto msg = static_cast<Message*>(object);
      ReflectiveFieldParser field_parser(msg, ctx, true);
      return internal::WireFormatParser({ItemParser, object}, field_parser,
                                        begin, end, ctx);
    }
  };

  ReflectiveFieldParser field_parser(static_cast<Message*>(object), ctx);
  return internal::WireFormatParser({_InternalParse, object}, field_parser,
                                    begin, end, ctx);
}
#endif  // GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER


void Message::SerializeWithCachedSizes(io::CodedOutputStream* output) const {
  const internal::SerializationTable* table =
      static_cast<const internal::SerializationTable*>(InternalGetTable());
  if (table == 0) {
    WireFormat::SerializeWithCachedSizes(*this, GetCachedSize(), output);
  } else {
    internal::TableSerialize(*this, table, output);
  }
}

size_t Message::ByteSizeLong() const {
  size_t size = WireFormat::ByteSize(*this);
  SetCachedSize(internal::ToCachedSize(size));
  return size;
}

void Message::SetCachedSize(int /* size */) const {
  GOOGLE_LOG(FATAL) << "Message class \"" << GetDescriptor()->full_name()
             << "\" implements neither SetCachedSize() nor ByteSize().  "
                "Must implement one or the other.";
}

size_t Message::SpaceUsedLong() const {
  return GetReflection()->SpaceUsedLong(*this);
}

bool Message::SerializeToFileDescriptor(int file_descriptor) const {
  io::FileOutputStream output(file_descriptor);
  return SerializeToZeroCopyStream(&output) && output.Flush();
}

bool Message::SerializePartialToFileDescriptor(int file_descriptor) const {
  io::FileOutputStream output(file_descriptor);
  return SerializePartialToZeroCopyStream(&output) && output.Flush();
}

bool Message::SerializeToOstream(std::ostream* output) const {
  {
    io::OstreamOutputStream zero_copy_output(output);
    if (!SerializeToZeroCopyStream(&zero_copy_output)) return false;
  }
  return output->good();
}

bool Message::SerializePartialToOstream(std::ostream* output) const {
  io::OstreamOutputStream zero_copy_output(output);
  return SerializePartialToZeroCopyStream(&zero_copy_output);
}


// =============================================================================
// Reflection and associated Template Specializations

Reflection::~Reflection() {}

void Reflection::AddAllocatedMessage(Message* /* message */,
                                     const FieldDescriptor* /*field */,
                                     Message* /* new_entry */) const {}

#define HANDLE_TYPE(TYPE, CPPTYPE, CTYPE)                               \
  template <>                                                           \
  const RepeatedField<TYPE>& Reflection::GetRepeatedField<TYPE>(        \
      const Message& message, const FieldDescriptor* field) const {     \
    return *static_cast<RepeatedField<TYPE>*>(MutableRawRepeatedField(  \
        const_cast<Message*>(&message), field, CPPTYPE, CTYPE, NULL));  \
  }                                                                     \
                                                                        \
  template <>                                                           \
  RepeatedField<TYPE>* Reflection::MutableRepeatedField<TYPE>(          \
      Message * message, const FieldDescriptor* field) const {          \
    return static_cast<RepeatedField<TYPE>*>(                           \
        MutableRawRepeatedField(message, field, CPPTYPE, CTYPE, NULL)); \
  }

HANDLE_TYPE(int32, FieldDescriptor::CPPTYPE_INT32, -1);
HANDLE_TYPE(int64, FieldDescriptor::CPPTYPE_INT64, -1);
HANDLE_TYPE(uint32, FieldDescriptor::CPPTYPE_UINT32, -1);
HANDLE_TYPE(uint64, FieldDescriptor::CPPTYPE_UINT64, -1);
HANDLE_TYPE(float, FieldDescriptor::CPPTYPE_FLOAT, -1);
HANDLE_TYPE(double, FieldDescriptor::CPPTYPE_DOUBLE, -1);
HANDLE_TYPE(bool, FieldDescriptor::CPPTYPE_BOOL, -1);


#undef HANDLE_TYPE

void* Reflection::MutableRawRepeatedString(Message* message,
                                           const FieldDescriptor* field,
                                           bool is_string) const {
  return MutableRawRepeatedField(message, field,
                                 FieldDescriptor::CPPTYPE_STRING,
                                 FieldOptions::STRING, NULL);
}


MapIterator Reflection::MapBegin(Message* message,
                                 const FieldDescriptor* field) const {
  GOOGLE_LOG(FATAL) << "Unimplemented Map Reflection API.";
  MapIterator iter(message, field);
  return iter;
}

MapIterator Reflection::MapEnd(Message* message,
                               const FieldDescriptor* field) const {
  GOOGLE_LOG(FATAL) << "Unimplemented Map Reflection API.";
  MapIterator iter(message, field);
  return iter;
}

// =============================================================================
// MessageFactory

MessageFactory::~MessageFactory() {}

namespace internal {

// TODO(gerbens) make this factorized better. This should not have to hop
// to reflection. Currently uses GeneratedMessageReflection and thus is
// defined in generated_message_reflection.cc
void RegisterFileLevelMetadata(void* assign_descriptors_table);

}  // namespace internal

namespace {

void RegisterFileLevelMetadata(void* assign_descriptors_table,
                               const string& filename) {
  internal::RegisterFileLevelMetadata(assign_descriptors_table);
}

class GeneratedMessageFactory : public MessageFactory {
 public:
  static GeneratedMessageFactory* singleton();

  struct RegistrationData {
    const Metadata* file_level_metadata;
    int size;
  };

  void RegisterFile(const char* file, void* registration_data);
  void RegisterType(const Descriptor* descriptor, const Message* prototype);

  // implements MessageFactory ---------------------------------------
  const Message* GetPrototype(const Descriptor* type) override;

 private:
  // Only written at static init time, so does not require locking.
  std::unordered_map<const char*, void*, hash<const char*>,
                     streq>
      file_map_;

  internal::WrappedMutex mutex_;
  // Initialized lazily, so requires locking.
  std::unordered_map<const Descriptor*, const Message*> type_map_;
};

GeneratedMessageFactory* GeneratedMessageFactory::singleton() {
  static auto instance =
      internal::OnShutdownDelete(new GeneratedMessageFactory);
  return instance;
}

void GeneratedMessageFactory::RegisterFile(const char* file,
                                           void* registration_data) {
  if (!InsertIfNotPresent(&file_map_, file, registration_data)) {
    GOOGLE_LOG(FATAL) << "File is already registered: " << file;
  }
}

void GeneratedMessageFactory::RegisterType(const Descriptor* descriptor,
                                           const Message* prototype) {
  GOOGLE_DCHECK_EQ(descriptor->file()->pool(), DescriptorPool::generated_pool())
      << "Tried to register a non-generated type with the generated "
         "type registry.";

  // This should only be called as a result of calling a file registration
  // function during GetPrototype(), in which case we already have locked
  // the mutex.
  mutex_.AssertHeld();
  if (!InsertIfNotPresent(&type_map_, descriptor, prototype)) {
    GOOGLE_LOG(DFATAL) << "Type is already registered: " << descriptor->full_name();
  }
}


const Message* GeneratedMessageFactory::GetPrototype(const Descriptor* type) {
  {
    ReaderMutexLock lock(&mutex_);
    const Message* result = FindPtrOrNull(type_map_, type);
    if (result != NULL) return result;
  }

  // If the type is not in the generated pool, then we can't possibly handle
  // it.
  if (type->file()->pool() != DescriptorPool::generated_pool()) return NULL;

  // Apparently the file hasn't been registered yet.  Let's do that now.
  void* registration_data =
      FindPtrOrNull(file_map_, type->file()->name().c_str());
  if (registration_data == NULL) {
    GOOGLE_LOG(DFATAL) << "File appears to be in generated pool but wasn't "
                   "registered: "
                << type->file()->name();
    return NULL;
  }

  WriterMutexLock lock(&mutex_);

  // Check if another thread preempted us.
  const Message* result = FindPtrOrNull(type_map_, type);
  if (result == NULL) {
    // Nope.  OK, register everything.
    RegisterFileLevelMetadata(registration_data, type->file()->name());
    // Should be here now.
    result = FindPtrOrNull(type_map_, type);
  }

  if (result == NULL) {
    GOOGLE_LOG(DFATAL) << "Type appears to be in generated pool but wasn't "
                << "registered: " << type->full_name();
  }

  return result;
}

}  // namespace

MessageFactory* MessageFactory::generated_factory() {
  return GeneratedMessageFactory::singleton();
}

void MessageFactory::InternalRegisterGeneratedFile(
    const char* filename, void* assign_descriptors_table) {
  GeneratedMessageFactory::singleton()->RegisterFile(filename,
                                                     assign_descriptors_table);
}

void MessageFactory::InternalRegisterGeneratedMessage(
    const Descriptor* descriptor, const Message* prototype) {
  GeneratedMessageFactory::singleton()->RegisterType(descriptor, prototype);
}


MessageFactory* Reflection::GetMessageFactory() const {
  GOOGLE_LOG(FATAL) << "Not implemented.";
  return NULL;
}

void* Reflection::RepeatedFieldData(Message* message,
                                    const FieldDescriptor* field,
                                    FieldDescriptor::CppType cpp_type,
                                    const Descriptor* message_type) const {
  GOOGLE_LOG(FATAL) << "Not implemented.";
  return NULL;
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
  GOOGLE_CHECK(field->is_repeated());
  switch (field->cpp_type()) {
#define HANDLE_PRIMITIVE_TYPE(TYPE, type) \
  case FieldDescriptor::CPPTYPE_##TYPE:   \
    return GetSingleton<internal::RepeatedFieldPrimitiveAccessor<type> >();
    HANDLE_PRIMITIVE_TYPE(INT32, int32)
    HANDLE_PRIMITIVE_TYPE(UINT32, uint32)
    HANDLE_PRIMITIVE_TYPE(INT64, int64)
    HANDLE_PRIMITIVE_TYPE(UINT64, uint64)
    HANDLE_PRIMITIVE_TYPE(FLOAT, float)
    HANDLE_PRIMITIVE_TYPE(DOUBLE, double)
    HANDLE_PRIMITIVE_TYPE(BOOL, bool)
    HANDLE_PRIMITIVE_TYPE(ENUM, int32)
#undef HANDLE_PRIMITIVE_TYPE
    case FieldDescriptor::CPPTYPE_STRING:
      switch (field->options().ctype()) {
        default:
        case FieldOptions::STRING:
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
  GOOGLE_LOG(FATAL) << "Should not reach here.";
  return NULL;
}

namespace internal {
template <>
#if defined(_MSC_VER) && (_MSC_VER >= 1800)
// Note: force noinline to workaround MSVC compiler bug with /Zc:inline, issue
// #240
PROTOBUF_NOINLINE
#endif
    Message*
    GenericTypeHandler<Message>::NewFromPrototype(const Message* prototype,
                                                  Arena* arena) {
  return prototype->New(arena);
}
template <>
#if defined(_MSC_VER) && (_MSC_VER >= 1800)
// Note: force noinline to workaround MSVC compiler bug with /Zc:inline, issue
// #240
PROTOBUF_NOINLINE
#endif
    Arena*
    GenericTypeHandler<Message>::GetArena(Message* value) {
  return value->GetArena();
}
template <>
#if defined(_MSC_VER) && (_MSC_VER >= 1800)
// Note: force noinline to workaround MSVC compiler bug with /Zc:inline, issue
// #240
PROTOBUF_NOINLINE
#endif
    void*
    GenericTypeHandler<Message>::GetMaybeArenaPointer(Message* value) {
  return value->GetMaybeArenaPointer();
}
}  // namespace internal

}  // namespace protobuf
}  // namespace google
