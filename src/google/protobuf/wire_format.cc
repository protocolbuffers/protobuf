// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/wire_format.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/cord.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/map_field.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/unknown_field_set.h"
#include "google/protobuf/wire_format_lite.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

const size_t kMapEntryTagByteSize = 2;

namespace google {
namespace protobuf {
namespace internal {

// Forward declare static functions
static size_t MapValueRefDataOnlyByteSize(const FieldDescriptor* field,
                                          const MapValueConstRef& value);

// ===================================================================

bool UnknownFieldSetFieldSkipper::SkipField(io::CodedInputStream* input,
                                            uint32_t tag) {
  return WireFormat::SkipField(input, tag, unknown_fields_);
}

bool UnknownFieldSetFieldSkipper::SkipMessage(io::CodedInputStream* input) {
  return WireFormat::SkipMessage(input, unknown_fields_);
}

void UnknownFieldSetFieldSkipper::SkipUnknownEnum(int field_number, int value) {
  unknown_fields_->AddVarint(field_number, value);
}

bool WireFormat::SkipField(io::CodedInputStream* input, uint32_t tag,
                           UnknownFieldSet* unknown_fields) {
  int number = WireFormatLite::GetTagFieldNumber(tag);
  // Field number 0 is illegal.
  if (number == 0) return false;

  switch (WireFormatLite::GetTagWireType(tag)) {
    case WireFormatLite::WIRETYPE_VARINT: {
      uint64_t value;
      if (!input->ReadVarint64(&value)) return false;
      if (unknown_fields != nullptr) unknown_fields->AddVarint(number, value);
      return true;
    }
    case WireFormatLite::WIRETYPE_FIXED64: {
      uint64_t value;
      if (!input->ReadLittleEndian64(&value)) return false;
      if (unknown_fields != nullptr) unknown_fields->AddFixed64(number, value);
      return true;
    }
    case WireFormatLite::WIRETYPE_LENGTH_DELIMITED: {
      uint32_t length;
      if (!input->ReadVarint32(&length)) return false;
      if (unknown_fields == nullptr) {
        if (!input->Skip(length)) return false;
      } else {
        if (!input->ReadString(unknown_fields->AddLengthDelimited(number),
                               length)) {
          return false;
        }
      }
      return true;
    }
    case WireFormatLite::WIRETYPE_START_GROUP: {
      if (!input->IncrementRecursionDepth()) return false;
      if (!SkipMessage(input, (unknown_fields == nullptr)
                                  ? nullptr
                                  : unknown_fields->AddGroup(number))) {
        return false;
      }
      input->DecrementRecursionDepth();
      // Check that the ending tag matched the starting tag.
      if (!input->LastTagWas(
              WireFormatLite::MakeTag(WireFormatLite::GetTagFieldNumber(tag),
                                      WireFormatLite::WIRETYPE_END_GROUP))) {
        return false;
      }
      return true;
    }
    case WireFormatLite::WIRETYPE_END_GROUP: {
      return false;
    }
    case WireFormatLite::WIRETYPE_FIXED32: {
      uint32_t value;
      if (!input->ReadLittleEndian32(&value)) return false;
      if (unknown_fields != nullptr) unknown_fields->AddFixed32(number, value);
      return true;
    }
    default: {
      return false;
    }
  }
}

bool WireFormat::SkipMessage(io::CodedInputStream* input,
                             UnknownFieldSet* unknown_fields) {
  while (true) {
    uint32_t tag = input->ReadTag();
    if (tag == 0) {
      // End of input.  This is a valid place to end, so return true.
      return true;
    }

    WireFormatLite::WireType wire_type = WireFormatLite::GetTagWireType(tag);

    if (wire_type == WireFormatLite::WIRETYPE_END_GROUP) {
      // Must be the end of the message.
      return true;
    }

    if (!SkipField(input, tag, unknown_fields)) return false;
  }
}

bool WireFormat::ReadPackedEnumPreserveUnknowns(io::CodedInputStream* input,
                                                uint32_t field_number,
                                                bool (*is_valid)(int),
                                                UnknownFieldSet* unknown_fields,
                                                RepeatedField<int>* values) {
  uint32_t length;
  if (!input->ReadVarint32(&length)) return false;
  io::CodedInputStream::Limit limit = input->PushLimit(length);
  while (input->BytesUntilLimit() > 0) {
    int value;
    if (!WireFormatLite::ReadPrimitive<int, WireFormatLite::TYPE_ENUM>(
            input, &value)) {
      return false;
    }
    if (is_valid == nullptr || is_valid(value)) {
      values->Add(value);
    } else {
      unknown_fields->AddVarint(field_number, value);
    }
  }
  input->PopLimit(limit);
  return true;
}

uint8_t* WireFormat::InternalSerializeUnknownFieldsToArray(
    const UnknownFieldSet& unknown_fields, uint8_t* target,
    io::EpsCopyOutputStream* stream) {
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);

    target = stream->EnsureSpace(target);
    switch (field.type()) {
      case UnknownField::TYPE_VARINT:
        target = WireFormatLite::WriteUInt64ToArray(field.number(),
                                                    field.varint(), target);
        break;
      case UnknownField::TYPE_FIXED32:
        target = WireFormatLite::WriteFixed32ToArray(field.number(),
                                                     field.fixed32(), target);
        break;
      case UnknownField::TYPE_FIXED64:
        target = WireFormatLite::WriteFixed64ToArray(field.number(),
                                                     field.fixed64(), target);
        break;
      case UnknownField::TYPE_LENGTH_DELIMITED:
        target = stream->WriteString(field.number(), field.length_delimited(),
                                     target);
        break;
      case UnknownField::TYPE_GROUP:
        target = WireFormatLite::WriteTagToArray(
            field.number(), WireFormatLite::WIRETYPE_START_GROUP, target);
        target = InternalSerializeUnknownFieldsToArray(field.group(), target,
                                                       stream);
        target = stream->EnsureSpace(target);
        target = WireFormatLite::WriteTagToArray(
            field.number(), WireFormatLite::WIRETYPE_END_GROUP, target);
        break;
    }
  }
  return target;
}

uint8_t* WireFormat::InternalSerializeUnknownMessageSetItemsToArray(
    const UnknownFieldSet& unknown_fields, uint8_t* target,
    io::EpsCopyOutputStream* stream) {
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);

    // The only unknown fields that are allowed to exist in a MessageSet are
    // messages, which are length-delimited.
    if (field.type() == UnknownField::TYPE_LENGTH_DELIMITED) {
      target = stream->EnsureSpace(target);
      // Start group.
      target = io::CodedOutputStream::WriteTagToArray(
          WireFormatLite::kMessageSetItemStartTag, target);

      // Write type ID.
      target = io::CodedOutputStream::WriteTagToArray(
          WireFormatLite::kMessageSetTypeIdTag, target);
      target =
          io::CodedOutputStream::WriteVarint32ToArray(field.number(), target);

      // Write message.
      target = io::CodedOutputStream::WriteTagToArray(
          WireFormatLite::kMessageSetMessageTag, target);

      target = field.InternalSerializeLengthDelimitedNoTag(target, stream);

      target = stream->EnsureSpace(target);
      // End group.
      target = io::CodedOutputStream::WriteTagToArray(
          WireFormatLite::kMessageSetItemEndTag, target);
    }
  }

  return target;
}

size_t WireFormat::ComputeUnknownFieldsSize(
    const UnknownFieldSet& unknown_fields) {
  size_t size = 0;
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);

    switch (field.type()) {
      case UnknownField::TYPE_VARINT:
        size += io::CodedOutputStream::VarintSize32(WireFormatLite::MakeTag(
            field.number(), WireFormatLite::WIRETYPE_VARINT));
        size += io::CodedOutputStream::VarintSize64(field.varint());
        break;
      case UnknownField::TYPE_FIXED32:
        size += io::CodedOutputStream::VarintSize32(WireFormatLite::MakeTag(
            field.number(), WireFormatLite::WIRETYPE_FIXED32));
        size += sizeof(int32_t);
        break;
      case UnknownField::TYPE_FIXED64:
        size += io::CodedOutputStream::VarintSize32(WireFormatLite::MakeTag(
            field.number(), WireFormatLite::WIRETYPE_FIXED64));
        size += sizeof(int64_t);
        break;
      case UnknownField::TYPE_LENGTH_DELIMITED:
        size += io::CodedOutputStream::VarintSize32(WireFormatLite::MakeTag(
            field.number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED));
        size += io::CodedOutputStream::VarintSize32(
            field.length_delimited().size());
        size += field.length_delimited().size();
        break;
      case UnknownField::TYPE_GROUP:
        size += io::CodedOutputStream::VarintSize32(WireFormatLite::MakeTag(
            field.number(), WireFormatLite::WIRETYPE_START_GROUP));
        size += ComputeUnknownFieldsSize(field.group());
        size += io::CodedOutputStream::VarintSize32(WireFormatLite::MakeTag(
            field.number(), WireFormatLite::WIRETYPE_END_GROUP));
        break;
    }
  }

  return size;
}

size_t WireFormat::ComputeUnknownMessageSetItemsSize(
    const UnknownFieldSet& unknown_fields) {
  size_t size = 0;
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);

    // The only unknown fields that are allowed to exist in a MessageSet are
    // messages, which are length-delimited.
    if (field.type() == UnknownField::TYPE_LENGTH_DELIMITED) {
      size += WireFormatLite::kMessageSetItemTagsSize;
      size += io::CodedOutputStream::VarintSize32(field.number());

      int field_size = field.GetLengthDelimitedSize();
      size += io::CodedOutputStream::VarintSize32(field_size);
      size += field_size;
    }
  }

  return size;
}

// ===================================================================

bool WireFormat::ParseAndMergePartial(io::CodedInputStream* input,
                                      Message* message) {
  const Descriptor* descriptor = message->GetDescriptor();
  const Reflection* message_reflection = message->GetReflection();

  while (true) {
    uint32_t tag = input->ReadTag();
    if (tag == 0) {
      // End of input.  This is a valid place to end, so return true.
      return true;
    }

    if (WireFormatLite::GetTagWireType(tag) ==
        WireFormatLite::WIRETYPE_END_GROUP) {
      // Must be the end of the message.
      return true;
    }

    const FieldDescriptor* field = nullptr;

    if (descriptor != nullptr) {
      int field_number = WireFormatLite::GetTagFieldNumber(tag);
      field = descriptor->FindFieldByNumber(field_number);

      // If that failed, check if the field is an extension.
      if (field == nullptr && descriptor->IsExtensionNumber(field_number)) {
        if (input->GetExtensionPool() == nullptr) {
          field = message_reflection->FindKnownExtensionByNumber(field_number);
        } else {
          field = input->GetExtensionPool()->FindExtensionByNumber(
              descriptor, field_number);
        }
      }

      // If that failed, but we're a MessageSet, and this is the tag for a
      // MessageSet item, then parse that.
      if (field == nullptr && descriptor->options().message_set_wire_format() &&
          tag == WireFormatLite::kMessageSetItemStartTag) {
        if (!ParseAndMergeMessageSetItem(input, message)) {
          return false;
        }
        continue;  // Skip ParseAndMergeField(); already taken care of.
      }
    }

    if (!ParseAndMergeField(tag, field, message, input)) {
      return false;
    }
  }
}

bool WireFormat::SkipMessageSetField(io::CodedInputStream* input,
                                     uint32_t field_number,
                                     UnknownFieldSet* unknown_fields) {
  uint32_t length;
  if (!input->ReadVarint32(&length)) return false;
  return input->ReadString(unknown_fields->AddLengthDelimited(field_number),
                           length);
}

bool WireFormat::ParseAndMergeMessageSetField(uint32_t field_number,
                                              const FieldDescriptor* field,
                                              Message* message,
                                              io::CodedInputStream* input) {
  const Reflection* message_reflection = message->GetReflection();
  if (field == nullptr) {
    // We store unknown MessageSet extensions as groups.
    return SkipMessageSetField(
        input, field_number, message_reflection->MutableUnknownFields(message));
  } else if (field->is_repeated() ||
             field->type() != FieldDescriptor::TYPE_MESSAGE) {
    // This shouldn't happen as we only allow optional message extensions to
    // MessageSet.
    ABSL_LOG(ERROR) << "Extensions of MessageSets must be optional messages.";
    return false;
  } else {
    Message* sub_message = message_reflection->MutableMessage(
        message, field, input->GetExtensionFactory());
    return WireFormatLite::ReadMessage(input, sub_message);
  }
}

bool WireFormat::ParseAndMergeField(
    uint32_t tag,
    const FieldDescriptor* field,  // May be nullptr for unknown
    Message* message, io::CodedInputStream* input) {
  const Reflection* message_reflection = message->GetReflection();

  enum { UNKNOWN, NORMAL_FORMAT, PACKED_FORMAT } value_format;

  if (field == nullptr) {
    value_format = UNKNOWN;
  } else if (WireFormatLite::GetTagWireType(tag) ==
             WireTypeForFieldType(field->type())) {
    value_format = NORMAL_FORMAT;
  } else if (field->is_packable() &&
             WireFormatLite::GetTagWireType(tag) ==
                 WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    value_format = PACKED_FORMAT;
  } else {
    // We don't recognize this field. Either the field number is unknown
    // or the wire type doesn't match. Put it in our unknown field set.
    value_format = UNKNOWN;
  }

  if (value_format == UNKNOWN) {
    return SkipField(input, tag,
                     message_reflection->MutableUnknownFields(message));
  } else if (value_format == PACKED_FORMAT) {
    uint32_t length;
    if (!input->ReadVarint32(&length)) return false;
    io::CodedInputStream::Limit limit = input->PushLimit(length);

    switch (field->type()) {
#define HANDLE_PACKED_TYPE(TYPE, CPPTYPE, CPPTYPE_METHOD)                      \
  case FieldDescriptor::TYPE_##TYPE: {                                         \
    while (input->BytesUntilLimit() > 0) {                                     \
      CPPTYPE value;                                                           \
      if (!WireFormatLite::ReadPrimitive<CPPTYPE,                              \
                                         WireFormatLite::TYPE_##TYPE>(input,   \
                                                                      &value)) \
        return false;                                                          \
      message_reflection->Add##CPPTYPE_METHOD(message, field, value);          \
    }                                                                          \
    break;                                                                     \
  }

      HANDLE_PACKED_TYPE(INT32, int32_t, Int32)
      HANDLE_PACKED_TYPE(INT64, int64_t, Int64)
      HANDLE_PACKED_TYPE(SINT32, int32_t, Int32)
      HANDLE_PACKED_TYPE(SINT64, int64_t, Int64)
      HANDLE_PACKED_TYPE(UINT32, uint32_t, UInt32)
      HANDLE_PACKED_TYPE(UINT64, uint64_t, UInt64)

      HANDLE_PACKED_TYPE(FIXED32, uint32_t, UInt32)
      HANDLE_PACKED_TYPE(FIXED64, uint64_t, UInt64)
      HANDLE_PACKED_TYPE(SFIXED32, int32_t, Int32)
      HANDLE_PACKED_TYPE(SFIXED64, int64_t, Int64)

      HANDLE_PACKED_TYPE(FLOAT, float, Float)
      HANDLE_PACKED_TYPE(DOUBLE, double, Double)

      HANDLE_PACKED_TYPE(BOOL, bool, Bool)
#undef HANDLE_PACKED_TYPE

      case FieldDescriptor::TYPE_ENUM: {
        while (input->BytesUntilLimit() > 0) {
          int value;
          if (!WireFormatLite::ReadPrimitive<int, WireFormatLite::TYPE_ENUM>(
                  input, &value))
            return false;
          if (!field->legacy_enum_field_treated_as_closed()) {
            message_reflection->AddEnumValue(message, field, value);
          } else {
            const EnumValueDescriptor* enum_value =
                field->enum_type()->FindValueByNumber(value);
            if (enum_value != nullptr) {
              message_reflection->AddEnum(message, field, enum_value);
            } else {
              // The enum value is not one of the known values.  Add it to the
              // UnknownFieldSet.
              int64_t sign_extended_value = static_cast<int64_t>(value);
              message_reflection->MutableUnknownFields(message)->AddVarint(
                  WireFormatLite::GetTagFieldNumber(tag), sign_extended_value);
            }
          }
        }

        break;
      }

      case FieldDescriptor::TYPE_STRING:
      case FieldDescriptor::TYPE_GROUP:
      case FieldDescriptor::TYPE_MESSAGE:
      case FieldDescriptor::TYPE_BYTES:
        // Can't have packed fields of these types: these should be caught by
        // the protocol compiler.
        return false;
        break;
    }

    input->PopLimit(limit);
  } else {
    // Non-packed value (value_format == NORMAL_FORMAT)
    switch (field->type()) {
#define HANDLE_TYPE(TYPE, CPPTYPE, CPPTYPE_METHOD)                            \
  case FieldDescriptor::TYPE_##TYPE: {                                        \
    CPPTYPE value;                                                            \
    if (!WireFormatLite::ReadPrimitive<CPPTYPE, WireFormatLite::TYPE_##TYPE>( \
            input, &value))                                                   \
      return false;                                                           \
    if (field->is_repeated()) {                                               \
      message_reflection->Add##CPPTYPE_METHOD(message, field, value);         \
    } else {                                                                  \
      message_reflection->Set##CPPTYPE_METHOD(message, field, value);         \
    }                                                                         \
    break;                                                                    \
  }

      HANDLE_TYPE(INT32, int32_t, Int32)
      HANDLE_TYPE(INT64, int64_t, Int64)
      HANDLE_TYPE(SINT32, int32_t, Int32)
      HANDLE_TYPE(SINT64, int64_t, Int64)
      HANDLE_TYPE(UINT32, uint32_t, UInt32)
      HANDLE_TYPE(UINT64, uint64_t, UInt64)

      HANDLE_TYPE(FIXED32, uint32_t, UInt32)
      HANDLE_TYPE(FIXED64, uint64_t, UInt64)
      HANDLE_TYPE(SFIXED32, int32_t, Int32)
      HANDLE_TYPE(SFIXED64, int64_t, Int64)

      HANDLE_TYPE(FLOAT, float, Float)
      HANDLE_TYPE(DOUBLE, double, Double)

      HANDLE_TYPE(BOOL, bool, Bool)
#undef HANDLE_TYPE

      case FieldDescriptor::TYPE_ENUM: {
        int value;
        if (!WireFormatLite::ReadPrimitive<int, WireFormatLite::TYPE_ENUM>(
                input, &value))
          return false;
        if (field->is_repeated()) {
          message_reflection->AddEnumValue(message, field, value);
        } else {
          message_reflection->SetEnumValue(message, field, value);
        }
        break;
      }

      // Handle strings separately so that we can optimize the ctype=CORD case.
      case FieldDescriptor::TYPE_STRING: {
        bool strict_utf8_check = field->requires_utf8_validation();
        std::string value;
        if (!WireFormatLite::ReadString(input, &value)) return false;
        if (strict_utf8_check) {
          if (!WireFormatLite::VerifyUtf8String(value.data(), value.length(),
                                                WireFormatLite::PARSE,
                                                field->full_name())) {
            return false;
          }
        } else {
          VerifyUTF8StringNamedField(value.data(), value.length(), PARSE,
                                     field->full_name());
        }
        if (field->is_repeated()) {
          message_reflection->AddString(message, field, value);
        } else {
          message_reflection->SetString(message, field, value);
        }
        break;
      }

      case FieldDescriptor::TYPE_BYTES: {
        if (field->cpp_string_type() == FieldDescriptor::CppStringType::kCord) {
          absl::Cord value;
          if (!WireFormatLite::ReadBytes(input, &value)) return false;
          message_reflection->SetString(message, field, value);
          break;
        }
        std::string value;
        if (!WireFormatLite::ReadBytes(input, &value)) return false;
        if (field->is_repeated()) {
          message_reflection->AddString(message, field, value);
        } else {
          message_reflection->SetString(message, field, value);
        }
        break;
      }

      case FieldDescriptor::TYPE_GROUP: {
        Message* sub_message;
        if (field->is_repeated()) {
          sub_message = message_reflection->AddMessage(
              message, field, input->GetExtensionFactory());
        } else {
          sub_message = message_reflection->MutableMessage(
              message, field, input->GetExtensionFactory());
        }

        if (!WireFormatLite::ReadGroup(WireFormatLite::GetTagFieldNumber(tag),
                                       input, sub_message))
          return false;
        break;
      }

      case FieldDescriptor::TYPE_MESSAGE: {
        Message* sub_message;
        if (field->is_repeated()) {
          sub_message = message_reflection->AddMessage(
              message, field, input->GetExtensionFactory());
        } else {
          sub_message = message_reflection->MutableMessage(
              message, field, input->GetExtensionFactory());
        }

        if (!WireFormatLite::ReadMessage(input, sub_message)) return false;
        break;
      }
    }
  }

  return true;
}

bool WireFormat::ParseAndMergeMessageSetItem(io::CodedInputStream* input,
                                             Message* message) {
  struct MSReflective {
    bool ParseField(int type_id, io::CodedInputStream* input) {
      const FieldDescriptor* field =
          message_reflection->FindKnownExtensionByNumber(type_id);
      return ParseAndMergeMessageSetField(type_id, field, message, input);
    }

    bool SkipField(uint32_t tag, io::CodedInputStream* input) {
      return WireFormat::SkipField(input, tag, nullptr);
    }

    const Reflection* message_reflection;
    Message* message;
  };

  return ParseMessageSetItemImpl(
      input, MSReflective{message->GetReflection(), message});
}

struct WireFormat::MessageSetParser {
  const char* ParseElement(const char* ptr, internal::ParseContext* ctx) {
    // Parse a MessageSetItem
    auto metadata = reflection->MutableInternalMetadata(msg);
    enum class State { kNoTag, kHasType, kHasPayload, kDone };
    State state = State::kNoTag;

    std::string payload;
    uint32_t type_id = 0;
    while (!ctx->Done(&ptr)) {
      // We use 64 bit tags in order to allow typeid's that span the whole
      // range of 32 bit numbers.
      uint32_t tag = static_cast<uint8_t>(*ptr++);
      if (tag == WireFormatLite::kMessageSetTypeIdTag) {
        uint64_t tmp;
        ptr = ParseBigVarint(ptr, &tmp);
        // We should fail parsing if type id is 0 after cast to uint32.
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr != nullptr &&
                                       static_cast<uint32_t>(tmp) != 0);
        if (state == State::kNoTag) {
          type_id = static_cast<uint32_t>(tmp);
          state = State::kHasType;
        } else if (state == State::kHasPayload) {
          type_id = static_cast<uint32_t>(tmp);
          const FieldDescriptor* field;
          if (ctx->data().pool == nullptr) {
            field = reflection->FindKnownExtensionByNumber(type_id);
          } else {
            field =
                ctx->data().pool->FindExtensionByNumber(descriptor, type_id);
          }
          if (field == nullptr || field->message_type() == nullptr) {
            WriteLengthDelimited(
                type_id, payload,
                metadata->mutable_unknown_fields<UnknownFieldSet>());
          } else {
            Message* value =
                field->is_repeated()
                    ? reflection->AddMessage(msg, field, ctx->data().factory)
                    : reflection->MutableMessage(msg, field,
                                                 ctx->data().factory);
            const char* p;
            // We can't use regular parse from string as we have to track
            // proper recursion depth and descriptor pools. Spawn a new
            // ParseContext inheriting those attributes.
            ParseContext tmp_ctx(ParseContext::kSpawn, *ctx, &p, payload);
            GOOGLE_PROTOBUF_PARSER_ASSERT(value->_InternalParse(p, &tmp_ctx) &&
                                           tmp_ctx.EndedAtLimit());
          }
          state = State::kDone;
        }
        continue;
      } else if (tag == WireFormatLite::kMessageSetMessageTag) {
        if (state == State::kNoTag) {
          int32_t size = ReadSize(&ptr);
          GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
          ptr = ctx->ReadString(ptr, size, &payload);
          GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
          state = State::kHasPayload;
        } else if (state == State::kHasType) {
          // We're now parsing the payload
          const FieldDescriptor* field = nullptr;
          if (descriptor->IsExtensionNumber(type_id)) {
            if (ctx->data().pool == nullptr) {
              field = reflection->FindKnownExtensionByNumber(type_id);
            } else {
              field =
                  ctx->data().pool->FindExtensionByNumber(descriptor, type_id);
            }
          }
          ptr = WireFormat::_InternalParseAndMergeField(
              msg, ptr, ctx, static_cast<uint64_t>(type_id) * 8 + 2, reflection,
              field);
          state = State::kDone;
        } else {
          int32_t size = ReadSize(&ptr);
          GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
          ptr = ctx->Skip(ptr, size);
          GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        }
      } else {
        // An unknown field in MessageSetItem.
        ptr = ReadTag(ptr - 1, &tag);
        if (tag == 0 || (tag & 7) == WireFormatLite::WIRETYPE_END_GROUP) {
          ctx->SetLastTag(tag);
          return ptr;
        }
        // Skip field.
        ptr = internal::UnknownFieldParse(
            tag, static_cast<std::string*>(nullptr), ptr, ctx);
      }
      GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
    }
    return ptr;
  }

  const char* ParseMessageSet(const char* ptr, internal::ParseContext* ctx) {
    while (!ctx->Done(&ptr)) {
      uint32_t tag;
      ptr = ReadTag(ptr, &tag);
      if (ABSL_PREDICT_FALSE(ptr == nullptr)) return nullptr;
      if (tag == 0 || (tag & 7) == WireFormatLite::WIRETYPE_END_GROUP) {
        ctx->SetLastTag(tag);
        break;
      }
      if (tag == WireFormatLite::kMessageSetItemStartTag) {
        // A message set item starts
        ptr = ctx->ParseGroupInlined(
            ptr, tag, [&](const char* ptr) { return ParseElement(ptr, ctx); });
      } else {
        // Parse other fields as normal extensions.
        int field_number = WireFormatLite::GetTagFieldNumber(tag);
        const FieldDescriptor* field = nullptr;
        if (descriptor->IsExtensionNumber(field_number)) {
          if (ctx->data().pool == nullptr) {
            field = reflection->FindKnownExtensionByNumber(field_number);
          } else {
            field = ctx->data().pool->FindExtensionByNumber(descriptor,
                                                            field_number);
          }
        }
        ptr = WireFormat::_InternalParseAndMergeField(msg, ptr, ctx, tag,
                                                      reflection, field);
      }
      if (ABSL_PREDICT_FALSE(ptr == nullptr)) return nullptr;
    }
    return ptr;
  }

  Message* msg;
  const Descriptor* descriptor;
  const Reflection* reflection;
};

static const char* HandleMessage(Message* msg, const char* ptr,
                                 internal::ParseContext* ctx, uint64_t tag,
                                 const Reflection* reflection,
                                 const FieldDescriptor* field) {
  Message* sub_message;
  if (field->is_repeated()) {
    sub_message = reflection->AddMessage(msg, field, ctx->data().factory);
  } else {
    sub_message = reflection->MutableMessage(msg, field, ctx->data().factory);
  }

  if (WireFormatLite::GetTagWireType(tag) ==
      WireFormatLite::WIRETYPE_START_GROUP) {
    return ctx->ParseGroup(sub_message, ptr, tag);
  } else {
    ABSL_DCHECK(WireFormatLite::GetTagWireType(tag) ==
                WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
  }

  ptr = ctx->ParseMessage(sub_message, ptr);

  // For map entries, if the value is an unknown enum we have to push it
  // into the unknown field set and remove it from the list.
  if (ptr != nullptr && field->is_map()) {
    auto* value_field = field->message_type()->map_value();
    auto* enum_type = value_field->enum_type();
    if (enum_type != nullptr &&
        !internal::cpp::HasPreservingUnknownEnumSemantics(value_field) &&
        enum_type->FindValueByNumber(sub_message->GetReflection()->GetEnumValue(
            *sub_message, value_field)) == nullptr) {
      reflection->MutableUnknownFields(msg)->AddLengthDelimited(
          field->number(), sub_message->SerializeAsString());
      reflection->RemoveLast(msg, field);
    }
  }
  return ptr;
}

const char* WireFormat::_InternalParse(Message* msg, const char* ptr,
                                       internal::ParseContext* ctx) {
  const Descriptor* descriptor = msg->GetDescriptor();
  const Reflection* reflection = msg->GetReflection();
  ABSL_DCHECK(descriptor);
  ABSL_DCHECK(reflection);
  if (descriptor->options().message_set_wire_format()) {
    MessageSetParser message_set{msg, descriptor, reflection};
    return message_set.ParseMessageSet(ptr, ctx);
  }
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ReadTag(ptr, &tag);
    if (ABSL_PREDICT_FALSE(ptr == nullptr)) return nullptr;
    if (tag == 0 || (tag & 7) == WireFormatLite::WIRETYPE_END_GROUP) {
      ctx->SetLastTag(tag);
      break;
    }
    const FieldDescriptor* field = nullptr;

    int field_number = WireFormatLite::GetTagFieldNumber(tag);
    field = descriptor->FindFieldByNumber(field_number);

    // If that failed, check if the field is an extension.
    if (field == nullptr && descriptor->IsExtensionNumber(field_number)) {
      if (ctx->data().pool == nullptr) {
        field = reflection->FindKnownExtensionByNumber(field_number);
      } else {
        field =
            ctx->data().pool->FindExtensionByNumber(descriptor, field_number);
      }
    }

    ptr = _InternalParseAndMergeField(msg, ptr, ctx, tag, reflection, field);
    if (ABSL_PREDICT_FALSE(ptr == nullptr)) return nullptr;
  }
  return ptr;
}

const char* WireFormat::_InternalParseAndMergeField(
    Message* msg, const char* ptr, internal::ParseContext* ctx, uint64_t tag,
    const Reflection* reflection, const FieldDescriptor* field) {
  if (field == nullptr) {
    // unknown field set parser takes 64bit tags, because message set type ids
    // span the full 32 bit range making the tag span [0, 2^35) range.
    return internal::UnknownFieldParse(
        tag, reflection->MutableUnknownFields(msg), ptr, ctx);
  }
  if (WireFormatLite::GetTagWireType(tag) !=
      WireTypeForFieldType(field->type())) {
    if (field->is_packable() && WireFormatLite::GetTagWireType(tag) ==
                                    WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
      switch (field->type()) {
#define HANDLE_PACKED_TYPE(TYPE, CPPTYPE, CPPTYPE_METHOD)                   \
  case FieldDescriptor::TYPE_##TYPE: {                                      \
    ptr = internal::Packed##CPPTYPE_METHOD##Parser(                         \
        reflection->MutableRepeatedFieldInternal<CPPTYPE>(msg, field), ptr, \
        ctx);                                                               \
    return ptr;                                                             \
  }

        HANDLE_PACKED_TYPE(INT32, int32_t, Int32)
        HANDLE_PACKED_TYPE(INT64, int64_t, Int64)
        HANDLE_PACKED_TYPE(SINT32, int32_t, SInt32)
        HANDLE_PACKED_TYPE(SINT64, int64_t, SInt64)
        HANDLE_PACKED_TYPE(UINT32, uint32_t, UInt32)
        HANDLE_PACKED_TYPE(UINT64, uint64_t, UInt64)

        HANDLE_PACKED_TYPE(FIXED32, uint32_t, Fixed32)
        HANDLE_PACKED_TYPE(FIXED64, uint64_t, Fixed64)
        HANDLE_PACKED_TYPE(SFIXED32, int32_t, SFixed32)
        HANDLE_PACKED_TYPE(SFIXED64, int64_t, SFixed64)

        HANDLE_PACKED_TYPE(FLOAT, float, Float)
        HANDLE_PACKED_TYPE(DOUBLE, double, Double)

        HANDLE_PACKED_TYPE(BOOL, bool, Bool)
#undef HANDLE_PACKED_TYPE

        case FieldDescriptor::TYPE_ENUM: {
          auto rep_enum =
              reflection->MutableRepeatedFieldInternal<int>(msg, field);
          if (!field->legacy_enum_field_treated_as_closed()) {
            ptr = internal::PackedEnumParser(rep_enum, ptr, ctx);
          } else {
            return ctx->ReadPackedVarint(
                ptr, [rep_enum, field, reflection, msg](int32_t val) {
                  if (field->enum_type()->FindValueByNumber(val) != nullptr) {
                    rep_enum->Add(val);
                  } else {
                    WriteVarint(field->number(), val,
                                reflection->MutableUnknownFields(msg));
                  }
                });
          }
          return ptr;
        }

        case FieldDescriptor::TYPE_STRING:
        case FieldDescriptor::TYPE_GROUP:
        case FieldDescriptor::TYPE_MESSAGE:
        case FieldDescriptor::TYPE_BYTES:
          ABSL_LOG(FATAL) << "Can't reach";
          return nullptr;
      }
    } else {
      // mismatched wiretype;
      return internal::UnknownFieldParse(
          tag, reflection->MutableUnknownFields(msg), ptr, ctx);
    }
  }

  // Non-packed value
  bool utf8_check = false;
  bool strict_utf8_check = false;
  switch (field->type()) {
#define HANDLE_TYPE(TYPE, CPPTYPE, CPPTYPE_METHOD)        \
  case FieldDescriptor::TYPE_##TYPE: {                    \
    CPPTYPE value;                                        \
    ptr = VarintParse(ptr, &value);                       \
    if (ptr == nullptr) return nullptr;                   \
    if (field->is_repeated()) {                           \
      reflection->Add##CPPTYPE_METHOD(msg, field, value); \
    } else {                                              \
      reflection->Set##CPPTYPE_METHOD(msg, field, value); \
    }                                                     \
    return ptr;                                           \
  }

    HANDLE_TYPE(BOOL, uint64_t, Bool)
    HANDLE_TYPE(INT32, uint32_t, Int32)
    HANDLE_TYPE(INT64, uint64_t, Int64)
    HANDLE_TYPE(UINT32, uint32_t, UInt32)
    HANDLE_TYPE(UINT64, uint64_t, UInt64)

    case FieldDescriptor::TYPE_SINT32: {
      int32_t value = ReadVarintZigZag32(&ptr);
      if (ptr == nullptr) return nullptr;
      if (field->is_repeated()) {
        reflection->AddInt32(msg, field, value);
      } else {
        reflection->SetInt32(msg, field, value);
      }
      return ptr;
    }
    case FieldDescriptor::TYPE_SINT64: {
      int64_t value = ReadVarintZigZag64(&ptr);
      if (ptr == nullptr) return nullptr;
      if (field->is_repeated()) {
        reflection->AddInt64(msg, field, value);
      } else {
        reflection->SetInt64(msg, field, value);
      }
      return ptr;
    }
#undef HANDLE_TYPE
#define HANDLE_TYPE(TYPE, CPPTYPE, CPPTYPE_METHOD)        \
  case FieldDescriptor::TYPE_##TYPE: {                    \
    CPPTYPE value;                                        \
    value = UnalignedLoad<CPPTYPE>(ptr);                  \
    ptr += sizeof(CPPTYPE);                               \
    if (field->is_repeated()) {                           \
      reflection->Add##CPPTYPE_METHOD(msg, field, value); \
    } else {                                              \
      reflection->Set##CPPTYPE_METHOD(msg, field, value); \
    }                                                     \
    return ptr;                                           \
  }

      HANDLE_TYPE(FIXED32, uint32_t, UInt32)
      HANDLE_TYPE(FIXED64, uint64_t, UInt64)
      HANDLE_TYPE(SFIXED32, int32_t, Int32)
      HANDLE_TYPE(SFIXED64, int64_t, Int64)

      HANDLE_TYPE(FLOAT, float, Float)
      HANDLE_TYPE(DOUBLE, double, Double)

#undef HANDLE_TYPE

    case FieldDescriptor::TYPE_ENUM: {
      uint32_t value;
      ptr = VarintParse(ptr, &value);
      if (ptr == nullptr) return nullptr;
      if (field->is_repeated()) {
        reflection->AddEnumValue(msg, field, value);
      } else {
        reflection->SetEnumValue(msg, field, value);
      }
      return ptr;
    }

    // Handle strings separately so that we can optimize the ctype=CORD case.
    case FieldDescriptor::TYPE_STRING:
      utf8_check = true;
      strict_utf8_check = field->requires_utf8_validation();
      ABSL_FALLTHROUGH_INTENDED;
    case FieldDescriptor::TYPE_BYTES: {
      int size = ReadSize(&ptr);
      if (ptr == nullptr) return nullptr;
      if (field->cpp_string_type() == FieldDescriptor::CppStringType::kCord) {
        absl::Cord value;
        ptr = ctx->ReadCord(ptr, size, &value);
        if (ptr == nullptr) return nullptr;
        reflection->SetString(msg, field, value);
        return ptr;
      }
      std::string value;
      ptr = ctx->ReadString(ptr, size, &value);
      if (ptr == nullptr) return nullptr;
      if (utf8_check) {
        if (strict_utf8_check) {
          if (!WireFormatLite::VerifyUtf8String(value.data(), value.length(),
                                                WireFormatLite::PARSE,
                                                field->full_name())) {
            return nullptr;
          }
        } else {
          VerifyUTF8StringNamedField(value.data(), value.length(), PARSE,
                                     field->full_name());
        }
      }
      if (field->is_repeated()) {
        reflection->AddString(msg, field, std::move(value));
      } else {
        reflection->SetString(msg, field, std::move(value));
      }
      return ptr;
    }

    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_GROUP:
      return HandleMessage(msg, ptr, ctx, tag, reflection, field);
  }

  // GCC 8 complains about control reaching end of non-void function here.
  // Let's keep it happy by returning a nullptr.
  return nullptr;
}

// ===================================================================

uint8_t* WireFormat::_InternalSerialize(const Message& message, uint8_t* target,
                                        io::EpsCopyOutputStream* stream) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* message_reflection = message.GetReflection();

  std::vector<const FieldDescriptor*> fields;

  // Fields of map entry should always be serialized.
  if (descriptor->options().map_entry()) {
    for (int i = 0; i < descriptor->field_count(); i++) {
      fields.push_back(descriptor->field(i));
    }
  } else {
    message_reflection->ListFields(message, &fields);
  }

  for (auto field : fields) {
    target = InternalSerializeField(field, message, target, stream);
  }

  if (descriptor->options().message_set_wire_format()) {
    return InternalSerializeUnknownMessageSetItemsToArray(
        message_reflection->GetUnknownFields(message), target, stream);
  } else {
    return InternalSerializeUnknownFieldsToArray(
        message_reflection->GetUnknownFields(message), target, stream);
  }
}

uint8_t* SerializeMapKeyWithCachedSizes(const FieldDescriptor* field,
                                        const MapKey& value, uint8_t* target,
                                        io::EpsCopyOutputStream* stream) {
  target = stream->EnsureSpace(target);
  switch (field->type()) {
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_ENUM:
      ABSL_LOG(FATAL) << "Unsupported";
      break;
#define CASE_TYPE(FieldType, CamelFieldType, CamelCppType)   \
  case FieldDescriptor::TYPE_##FieldType:                    \
    target = WireFormatLite::Write##CamelFieldType##ToArray( \
        1, value.Get##CamelCppType##Value(), target);        \
    break;
      CASE_TYPE(INT64, Int64, Int64)
      CASE_TYPE(UINT64, UInt64, UInt64)
      CASE_TYPE(INT32, Int32, Int32)
      CASE_TYPE(FIXED64, Fixed64, UInt64)
      CASE_TYPE(FIXED32, Fixed32, UInt32)
      CASE_TYPE(BOOL, Bool, Bool)
      CASE_TYPE(UINT32, UInt32, UInt32)
      CASE_TYPE(SFIXED32, SFixed32, Int32)
      CASE_TYPE(SFIXED64, SFixed64, Int64)
      CASE_TYPE(SINT32, SInt32, Int32)
      CASE_TYPE(SINT64, SInt64, Int64)
#undef CASE_TYPE
    case FieldDescriptor::TYPE_STRING:
      target = stream->WriteString(1, value.GetStringValue(), target);
      break;
  }
  return target;
}

static uint8_t* SerializeMapValueRefWithCachedSizes(
    const FieldDescriptor* field, const MapValueConstRef& value,
    uint8_t* target, io::EpsCopyOutputStream* stream) {
  target = stream->EnsureSpace(target);
  switch (field->type()) {
#define CASE_TYPE(FieldType, CamelFieldType, CamelCppType)   \
  case FieldDescriptor::TYPE_##FieldType:                    \
    target = WireFormatLite::Write##CamelFieldType##ToArray( \
        2, value.Get##CamelCppType##Value(), target);        \
    break;
    CASE_TYPE(INT64, Int64, Int64)
    CASE_TYPE(UINT64, UInt64, UInt64)
    CASE_TYPE(INT32, Int32, Int32)
    CASE_TYPE(FIXED64, Fixed64, UInt64)
    CASE_TYPE(FIXED32, Fixed32, UInt32)
    CASE_TYPE(BOOL, Bool, Bool)
    CASE_TYPE(UINT32, UInt32, UInt32)
    CASE_TYPE(SFIXED32, SFixed32, Int32)
    CASE_TYPE(SFIXED64, SFixed64, Int64)
    CASE_TYPE(SINT32, SInt32, Int32)
    CASE_TYPE(SINT64, SInt64, Int64)
    CASE_TYPE(ENUM, Enum, Enum)
    CASE_TYPE(DOUBLE, Double, Double)
    CASE_TYPE(FLOAT, Float, Float)
#undef CASE_TYPE
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      target = stream->WriteString(2, value.GetStringValue(), target);
      break;
    case FieldDescriptor::TYPE_MESSAGE: {
      auto& msg = value.GetMessageValue();
      target = WireFormatLite::InternalWriteMessage(2, msg, msg.GetCachedSize(),
                                                    target, stream);
    } break;
    case FieldDescriptor::TYPE_GROUP:
      target = WireFormatLite::InternalWriteGroup(2, value.GetMessageValue(),
                                                  target, stream);
      break;
  }
  return target;
}

class MapKeySorter {
 public:
  static std::vector<MapKey> SortKey(const Message& message,
                                     const Reflection* reflection,
                                     const FieldDescriptor* field) {
    std::vector<MapKey> sorted_key_list;
    for (MapIterator it =
             reflection->MapBegin(const_cast<Message*>(&message), field);
         it != reflection->MapEnd(const_cast<Message*>(&message), field);
         ++it) {
      sorted_key_list.push_back(it.GetKey());
    }
    MapKeyComparator comparator;
    std::sort(sorted_key_list.begin(), sorted_key_list.end(), comparator);
    return sorted_key_list;
  }

 private:
  class MapKeyComparator {
   public:
    bool operator()(const MapKey& a, const MapKey& b) const {
      ABSL_DCHECK(a.type() == b.type());
      switch (a.type()) {
#define CASE_TYPE(CppType, CamelCppType)                                \
  case FieldDescriptor::CPPTYPE_##CppType: {                            \
    return a.Get##CamelCppType##Value() < b.Get##CamelCppType##Value(); \
  }
        CASE_TYPE(STRING, String)
        CASE_TYPE(INT64, Int64)
        CASE_TYPE(INT32, Int32)
        CASE_TYPE(UINT64, UInt64)
        CASE_TYPE(UINT32, UInt32)
        CASE_TYPE(BOOL, Bool)
#undef CASE_TYPE

        default:
          ABSL_DLOG(FATAL) << "Invalid key for map field.";
          return true;
      }
    }
  };
};

static uint8_t* InternalSerializeMapEntry(const FieldDescriptor* field,
                                          const MapKey& key,
                                          const MapValueConstRef& value,
                                          uint8_t* target,
                                          io::EpsCopyOutputStream* stream) {
  const FieldDescriptor* key_field = field->message_type()->field(0);
  const FieldDescriptor* value_field = field->message_type()->field(1);

  size_t size = kMapEntryTagByteSize;
  size += MapKeyDataOnlyByteSize(key_field, key);
  size += MapValueRefDataOnlyByteSize(value_field, value);
  target = stream->EnsureSpace(target);
  target = WireFormatLite::WriteTagToArray(
      field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED, target);
  target = io::CodedOutputStream::WriteVarint32ToArray(size, target);
  target = SerializeMapKeyWithCachedSizes(key_field, key, target, stream);
  target =
      SerializeMapValueRefWithCachedSizes(value_field, value, target, stream);
  return target;
}

uint8_t* WireFormat::InternalSerializeField(const FieldDescriptor* field,
                                            const Message& message,
                                            uint8_t* target,
                                            io::EpsCopyOutputStream* stream) {
  const Reflection* message_reflection = message.GetReflection();

  if (field->is_extension() &&
      field->containing_type()->options().message_set_wire_format() &&
      field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
      !field->is_repeated()) {
    return InternalSerializeMessageSetItem(field, message, target, stream);
  }


  // For map fields, we can use either repeated field reflection or map
  // reflection.  Our choice has some subtle effects.  If we use repeated field
  // reflection here, then the repeated field representation becomes
  // authoritative for this field: any existing references that came from map
  // reflection remain valid for reading, but mutations to them are lost and
  // will be overwritten next time we call map reflection!
  //
  // So far this mainly affects Python, which keeps long-term references to map
  // values around, and always uses map reflection.  See: b/35918691
  //
  // Here we choose to use map reflection API as long as the internal
  // map is valid. In this way, the serialization doesn't change map field's
  // internal state and existing references that came from map reflection remain
  // valid for both reading and writing.
  if (field->is_map()) {
    const MapFieldBase* map_field =
        message_reflection->GetMapData(message, field);
    if (map_field->IsMapValid()) {
      if (stream->IsSerializationDeterministic()) {
        std::vector<MapKey> sorted_key_list =
            MapKeySorter::SortKey(message, message_reflection, field);
        for (std::vector<MapKey>::iterator it = sorted_key_list.begin();
             it != sorted_key_list.end(); ++it) {
          MapValueConstRef map_value;
          message_reflection->LookupMapValue(message, field, *it, &map_value);
          target =
              InternalSerializeMapEntry(field, *it, map_value, target, stream);
        }
      } else {
        for (MapIterator it = message_reflection->MapBegin(
                 const_cast<Message*>(&message), field);
             it !=
             message_reflection->MapEnd(const_cast<Message*>(&message), field);
             ++it) {
          target = InternalSerializeMapEntry(field, it.GetKey(),
                                             it.GetValueRef(), target, stream);
        }
      }

      return target;
    }
  }
  int count = 0;

  if (field->is_repeated()) {
    count = message_reflection->FieldSize(message, field);
  } else if (field->containing_type()->options().map_entry()) {
    // Map entry fields always need to be serialized.
    count = 1;
  } else if (message_reflection->HasField(message, field)) {
    count = 1;
  }

  // map_entries is for maps that'll be deterministically serialized.
  std::vector<const Message*> map_entries;
  if (count > 1 && field->is_map() && stream->IsSerializationDeterministic()) {
    map_entries =
        DynamicMapSorter::Sort(message, count, message_reflection, field);
  }

  if (field->is_packed()) {
    if (count == 0) return target;
    target = stream->EnsureSpace(target);
    switch (field->type()) {
#define HANDLE_PRIMITIVE_TYPE(TYPE, CPPTYPE, TYPE_METHOD, CPPTYPE_METHOD)      \
  case FieldDescriptor::TYPE_##TYPE: {                                         \
    auto r =                                                                   \
        message_reflection->GetRepeatedFieldInternal<CPPTYPE>(message, field); \
    target = stream->Write##TYPE_METHOD##Packed(                               \
        field->number(), r, FieldDataOnlyByteSize(field, message), target);    \
    break;                                                                     \
  }

      HANDLE_PRIMITIVE_TYPE(INT32, int32_t, Int32, Int32)
      HANDLE_PRIMITIVE_TYPE(INT64, int64_t, Int64, Int64)
      HANDLE_PRIMITIVE_TYPE(SINT32, int32_t, SInt32, Int32)
      HANDLE_PRIMITIVE_TYPE(SINT64, int64_t, SInt64, Int64)
      HANDLE_PRIMITIVE_TYPE(UINT32, uint32_t, UInt32, UInt32)
      HANDLE_PRIMITIVE_TYPE(UINT64, uint64_t, UInt64, UInt64)
      HANDLE_PRIMITIVE_TYPE(ENUM, int, Enum, Enum)

#undef HANDLE_PRIMITIVE_TYPE
#define HANDLE_PRIMITIVE_TYPE(TYPE, CPPTYPE, TYPE_METHOD, CPPTYPE_METHOD)      \
  case FieldDescriptor::TYPE_##TYPE: {                                         \
    auto r =                                                                   \
        message_reflection->GetRepeatedFieldInternal<CPPTYPE>(message, field); \
    target = stream->WriteFixedPacked(field->number(), r, target);             \
    break;                                                                     \
  }

      HANDLE_PRIMITIVE_TYPE(FIXED32, uint32_t, Fixed32, UInt32)
      HANDLE_PRIMITIVE_TYPE(FIXED64, uint64_t, Fixed64, UInt64)
      HANDLE_PRIMITIVE_TYPE(SFIXED32, int32_t, SFixed32, Int32)
      HANDLE_PRIMITIVE_TYPE(SFIXED64, int64_t, SFixed64, Int64)

      HANDLE_PRIMITIVE_TYPE(FLOAT, float, Float, Float)
      HANDLE_PRIMITIVE_TYPE(DOUBLE, double, Double, Double)

      HANDLE_PRIMITIVE_TYPE(BOOL, bool, Bool, Bool)
#undef HANDLE_PRIMITIVE_TYPE
      default:
        ABSL_LOG(FATAL) << "Invalid descriptor";
    }
    return target;
  }

  auto get_message_from_field = [&message, &map_entries, message_reflection](
                                    const FieldDescriptor* field, int j) {
    if (!field->is_repeated()) {
      return &message_reflection->GetMessage(message, field);
    }
    if (!map_entries.empty()) {
      return map_entries[j];
    }
    return &message_reflection->GetRepeatedMessage(message, field, j);
  };
  for (int j = 0; j < count; j++) {
    target = stream->EnsureSpace(target);
    switch (field->type()) {
#define HANDLE_PRIMITIVE_TYPE(TYPE, CPPTYPE, TYPE_METHOD, CPPTYPE_METHOD)     \
  case FieldDescriptor::TYPE_##TYPE: {                                        \
    const CPPTYPE value =                                                     \
        field->is_repeated()                                                  \
            ? message_reflection->GetRepeated##CPPTYPE_METHOD(message, field, \
                                                              j)              \
            : message_reflection->Get##CPPTYPE_METHOD(message, field);        \
    target = WireFormatLite::Write##TYPE_METHOD##ToArray(field->number(),     \
                                                         value, target);      \
    break;                                                                    \
  }

      HANDLE_PRIMITIVE_TYPE(INT32, int32_t, Int32, Int32)
      HANDLE_PRIMITIVE_TYPE(INT64, int64_t, Int64, Int64)
      HANDLE_PRIMITIVE_TYPE(SINT32, int32_t, SInt32, Int32)
      HANDLE_PRIMITIVE_TYPE(SINT64, int64_t, SInt64, Int64)
      HANDLE_PRIMITIVE_TYPE(UINT32, uint32_t, UInt32, UInt32)
      HANDLE_PRIMITIVE_TYPE(UINT64, uint64_t, UInt64, UInt64)

      HANDLE_PRIMITIVE_TYPE(FIXED32, uint32_t, Fixed32, UInt32)
      HANDLE_PRIMITIVE_TYPE(FIXED64, uint64_t, Fixed64, UInt64)
      HANDLE_PRIMITIVE_TYPE(SFIXED32, int32_t, SFixed32, Int32)
      HANDLE_PRIMITIVE_TYPE(SFIXED64, int64_t, SFixed64, Int64)

      HANDLE_PRIMITIVE_TYPE(FLOAT, float, Float, Float)
      HANDLE_PRIMITIVE_TYPE(DOUBLE, double, Double, Double)

      HANDLE_PRIMITIVE_TYPE(BOOL, bool, Bool, Bool)
#undef HANDLE_PRIMITIVE_TYPE

      case FieldDescriptor::TYPE_GROUP: {
        auto* msg = get_message_from_field(field, j);
        target = WireFormatLite::InternalWriteGroup(field->number(), *msg,
                                                    target, stream);
      } break;

      case FieldDescriptor::TYPE_MESSAGE: {
        auto* msg = get_message_from_field(field, j);
        target = WireFormatLite::InternalWriteMessage(
            field->number(), *msg, msg->GetCachedSize(), target, stream);
      } break;

      case FieldDescriptor::TYPE_ENUM: {
        const EnumValueDescriptor* value =
            field->is_repeated()
                ? message_reflection->GetRepeatedEnum(message, field, j)
                : message_reflection->GetEnum(message, field);
        target = WireFormatLite::WriteEnumToArray(field->number(),
                                                  value->number(), target);
        break;
      }

      // Handle strings separately so that we can get string references
      // instead of copying.
      case FieldDescriptor::TYPE_STRING: {
        bool strict_utf8_check = field->requires_utf8_validation();
        std::string scratch;
        const std::string& value =
            field->is_repeated()
                ? message_reflection->GetRepeatedStringReference(message, field,
                                                                 j, &scratch)
                : message_reflection->GetStringReference(message, field,
                                                         &scratch);
        if (strict_utf8_check) {
          WireFormatLite::VerifyUtf8String(value.data(), value.length(),
                                           WireFormatLite::SERIALIZE,
                                           field->full_name());
        } else {
          VerifyUTF8StringNamedField(value.data(), value.length(), SERIALIZE,
                                     field->full_name());
        }
        target = stream->WriteString(field->number(), value, target);
        break;
      }

      case FieldDescriptor::TYPE_BYTES: {
        if (field->cpp_string_type() == FieldDescriptor::CppStringType::kCord) {
          absl::Cord value = message_reflection->GetCord(message, field);
          target = stream->WriteString(field->number(), value, target);
          break;
        }
        std::string scratch;
        const std::string& value =
            field->is_repeated()
                ? message_reflection->GetRepeatedStringReference(message, field,
                                                                 j, &scratch)
                : message_reflection->GetStringReference(message, field,
                                                         &scratch);
        target = stream->WriteString(field->number(), value, target);
        break;
      }
    }
  }
  return target;
}

uint8_t* WireFormat::InternalSerializeMessageSetItem(
    const FieldDescriptor* field, const Message& message, uint8_t* target,
    io::EpsCopyOutputStream* stream) {
  const Reflection* message_reflection = message.GetReflection();

  target = stream->EnsureSpace(target);
  // Start group.
  target = io::CodedOutputStream::WriteTagToArray(
      WireFormatLite::kMessageSetItemStartTag, target);
  // Write type ID.
  target = WireFormatLite::WriteUInt32ToArray(
      WireFormatLite::kMessageSetTypeIdNumber, field->number(), target);
    // Write message.
    auto& msg = message_reflection->GetMessage(message, field);
    target = WireFormatLite::InternalWriteMessage(
        WireFormatLite::kMessageSetMessageNumber, msg, msg.GetCachedSize(),
        target, stream);
  // End group.
  target = stream->EnsureSpace(target);
  target = io::CodedOutputStream::WriteTagToArray(
      WireFormatLite::kMessageSetItemEndTag, target);
  return target;
}

// ===================================================================

size_t WireFormat::ByteSize(const Message& message) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* message_reflection = message.GetReflection();

  size_t our_size = 0;

  std::vector<const FieldDescriptor*> fields;

  // Fields of map entry should always be serialized.
  if (descriptor->options().map_entry()) {
    for (int i = 0; i < descriptor->field_count(); i++) {
      fields.push_back(descriptor->field(i));
    }
  } else {
    message_reflection->ListFields(message, &fields);
  }

  for (const FieldDescriptor* field : fields) {
    our_size += FieldByteSize(field, message);
  }

  if (descriptor->options().message_set_wire_format()) {
    our_size += ComputeUnknownMessageSetItemsSize(
        message_reflection->GetUnknownFields(message));
  } else {
    our_size +=
        ComputeUnknownFieldsSize(message_reflection->GetUnknownFields(message));
  }

  return our_size;
}

size_t WireFormat::FieldByteSize(const FieldDescriptor* field,
                                 const Message& message) {
  const Reflection* message_reflection = message.GetReflection();

  if (field->is_extension() &&
      field->containing_type()->options().message_set_wire_format() &&
      field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
      !field->is_repeated()) {
    return MessageSetItemByteSize(field, message);
  }

  size_t count = 0;
  if (field->is_repeated()) {
    if (field->is_map()) {
      const MapFieldBase* map_field =
          message_reflection->GetMapData(message, field);
      if (map_field->IsMapValid()) {
        count = FromIntSize(map_field->size());
      } else {
        count = FromIntSize(message_reflection->FieldSize(message, field));
      }
    } else {
      count = FromIntSize(message_reflection->FieldSize(message, field));
    }
  } else if (field->containing_type()->options().map_entry()) {
    // Map entry fields always need to be serialized.
    count = 1;
  } else if (message_reflection->HasField(message, field)) {
    count = 1;
  }

  const size_t data_size = FieldDataOnlyByteSize(field, message);
  size_t our_size = data_size;
  if (field->is_packed()) {
    if (data_size > 0) {
      // Packed fields get serialized like a string, not their native type.
      // Technically this doesn't really matter; the size only changes if it's
      // a GROUP
      our_size += TagSize(field->number(), FieldDescriptor::TYPE_STRING);
      our_size += io::CodedOutputStream::VarintSize32(data_size);
    }
  } else {
    our_size += count * TagSize(field->number(), field->type());
  }
  return our_size;
}

size_t MapKeyDataOnlyByteSize(const FieldDescriptor* field,
                              const MapKey& value) {
  ABSL_DCHECK_EQ(FieldDescriptor::TypeToCppType(field->type()), value.type());
  switch (field->type()) {
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_ENUM:
      ABSL_LOG(FATAL) << "Unsupported";
      return 0;
#define CASE_TYPE(FieldType, CamelFieldType, CamelCppType) \
  case FieldDescriptor::TYPE_##FieldType:                  \
    return WireFormatLite::CamelFieldType##Size(           \
        value.Get##CamelCppType##Value());

#define FIXED_CASE_TYPE(FieldType, CamelFieldType) \
  case FieldDescriptor::TYPE_##FieldType:          \
    return WireFormatLite::k##CamelFieldType##Size;

      CASE_TYPE(INT32, Int32, Int32);
      CASE_TYPE(INT64, Int64, Int64);
      CASE_TYPE(UINT32, UInt32, UInt32);
      CASE_TYPE(UINT64, UInt64, UInt64);
      CASE_TYPE(SINT32, SInt32, Int32);
      CASE_TYPE(SINT64, SInt64, Int64);
      CASE_TYPE(STRING, String, String);
      FIXED_CASE_TYPE(FIXED32, Fixed32);
      FIXED_CASE_TYPE(FIXED64, Fixed64);
      FIXED_CASE_TYPE(SFIXED32, SFixed32);
      FIXED_CASE_TYPE(SFIXED64, SFixed64);
      FIXED_CASE_TYPE(BOOL, Bool);

#undef CASE_TYPE
#undef FIXED_CASE_TYPE
  }
  ABSL_LOG(FATAL) << "Cannot get here";
  return 0;
}

static size_t MapValueRefDataOnlyByteSize(const FieldDescriptor* field,
                                          const MapValueConstRef& value) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_GROUP:
      ABSL_LOG(FATAL) << "Unsupported";
      return 0;
#define CASE_TYPE(FieldType, CamelFieldType, CamelCppType) \
  case FieldDescriptor::TYPE_##FieldType:                  \
    return WireFormatLite::CamelFieldType##Size(           \
        value.Get##CamelCppType##Value());

#define FIXED_CASE_TYPE(FieldType, CamelFieldType) \
  case FieldDescriptor::TYPE_##FieldType:          \
    return WireFormatLite::k##CamelFieldType##Size;

      CASE_TYPE(INT32, Int32, Int32);
      CASE_TYPE(INT64, Int64, Int64);
      CASE_TYPE(UINT32, UInt32, UInt32);
      CASE_TYPE(UINT64, UInt64, UInt64);
      CASE_TYPE(SINT32, SInt32, Int32);
      CASE_TYPE(SINT64, SInt64, Int64);
      CASE_TYPE(STRING, String, String);
      CASE_TYPE(BYTES, Bytes, String);
      CASE_TYPE(ENUM, Enum, Enum);
      CASE_TYPE(MESSAGE, Message, Message);
      FIXED_CASE_TYPE(FIXED32, Fixed32);
      FIXED_CASE_TYPE(FIXED64, Fixed64);
      FIXED_CASE_TYPE(SFIXED32, SFixed32);
      FIXED_CASE_TYPE(SFIXED64, SFixed64);
      FIXED_CASE_TYPE(DOUBLE, Double);
      FIXED_CASE_TYPE(FLOAT, Float);
      FIXED_CASE_TYPE(BOOL, Bool);

#undef CASE_TYPE
#undef FIXED_CASE_TYPE
  }
  ABSL_LOG(FATAL) << "Cannot get here";
  return 0;
}

size_t WireFormat::FieldDataOnlyByteSize(const FieldDescriptor* field,
                                         const Message& message) {
  const Reflection* message_reflection = message.GetReflection();

  size_t data_size = 0;

  if (field->is_map()) {
    const MapFieldBase* map_field =
        message_reflection->GetMapData(message, field);
    if (map_field->IsMapValid()) {
      MapIterator iter(const_cast<Message*>(&message), field);
      MapIterator end(const_cast<Message*>(&message), field);
      const FieldDescriptor* key_field = field->message_type()->field(0);
      const FieldDescriptor* value_field = field->message_type()->field(1);
      for (map_field->MapBegin(&iter), map_field->MapEnd(&end); iter != end;
           ++iter) {
        size_t size = kMapEntryTagByteSize;
        size += MapKeyDataOnlyByteSize(key_field, iter.GetKey());
        size += MapValueRefDataOnlyByteSize(value_field, iter.GetValueRef());
        data_size += WireFormatLite::LengthDelimitedSize(size);
      }
      return data_size;
    }
  }

  size_t count = 0;
  if (field->is_repeated()) {
    count =
        internal::FromIntSize(message_reflection->FieldSize(message, field));
  } else if (field->containing_type()->options().map_entry()) {
    // Map entry fields always need to be serialized.
    count = 1;
  } else if (message_reflection->HasField(message, field)) {
    count = 1;
  }

  switch (field->type()) {
#define HANDLE_TYPE(TYPE, TYPE_METHOD, CPPTYPE_METHOD)                      \
  case FieldDescriptor::TYPE_##TYPE:                                        \
    if (field->is_repeated()) {                                             \
      for (size_t j = 0; j < count; j++) {                                  \
        data_size += WireFormatLite::TYPE_METHOD##Size(                     \
            message_reflection->GetRepeated##CPPTYPE_METHOD(message, field, \
                                                            j));            \
      }                                                                     \
    } else {                                                                \
      data_size += WireFormatLite::TYPE_METHOD##Size(                       \
          message_reflection->Get##CPPTYPE_METHOD(message, field));         \
    }                                                                       \
    break;

#define HANDLE_FIXED_TYPE(TYPE, TYPE_METHOD)                   \
  case FieldDescriptor::TYPE_##TYPE:                           \
    data_size += count * WireFormatLite::k##TYPE_METHOD##Size; \
    break;

    HANDLE_TYPE(INT32, Int32, Int32)
    HANDLE_TYPE(INT64, Int64, Int64)
    HANDLE_TYPE(SINT32, SInt32, Int32)
    HANDLE_TYPE(SINT64, SInt64, Int64)
    HANDLE_TYPE(UINT32, UInt32, UInt32)
    HANDLE_TYPE(UINT64, UInt64, UInt64)

    HANDLE_FIXED_TYPE(FIXED32, Fixed32)
    HANDLE_FIXED_TYPE(FIXED64, Fixed64)
    HANDLE_FIXED_TYPE(SFIXED32, SFixed32)
    HANDLE_FIXED_TYPE(SFIXED64, SFixed64)

    HANDLE_FIXED_TYPE(FLOAT, Float)
    HANDLE_FIXED_TYPE(DOUBLE, Double)

    HANDLE_FIXED_TYPE(BOOL, Bool)

    HANDLE_TYPE(GROUP, Group, Message)

    case FieldDescriptor::TYPE_MESSAGE: {
      if (field->is_repeated()) {
        for (size_t j = 0; j < count; ++j) {
          data_size += WireFormatLite::MessageSize(
              message_reflection->GetRepeatedMessage(message, field, j));
        }
        break;
      }
      if (field->is_extension()) {
        data_size += WireFormatLite::LengthDelimitedSize(
            message_reflection->GetExtensionSet(message).GetMessageByteSizeLong(
                field->number()));
        break;
      }
      data_size += WireFormatLite::MessageSize(
          message_reflection->GetMessage(message, field));
      break;
    }

#undef HANDLE_TYPE
#undef HANDLE_FIXED_TYPE

    case FieldDescriptor::TYPE_ENUM: {
      if (field->is_repeated()) {
        for (size_t j = 0; j < count; j++) {
          data_size += WireFormatLite::EnumSize(
              message_reflection->GetRepeatedEnum(message, field, j)->number());
        }
      } else {
        data_size += WireFormatLite::EnumSize(
            message_reflection->GetEnum(message, field)->number());
      }
      break;
    }

    // Handle strings separately so that we can get string references
    // instead of copying.
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES: {
      if (field->cpp_string_type() == FieldDescriptor::CppStringType::kCord) {
        for (size_t j = 0; j < count; j++) {
          absl::Cord value = message_reflection->GetCord(message, field);
          data_size += WireFormatLite::StringSize(value);
        }
        break;
      }
      for (size_t j = 0; j < count; j++) {
        std::string scratch;
        const std::string& value =
            field->is_repeated()
                ? message_reflection->GetRepeatedStringReference(message, field,
                                                                 j, &scratch)
                : message_reflection->GetStringReference(message, field,
                                                         &scratch);
        data_size += WireFormatLite::StringSize(value);
      }
      break;
    }
  }
  return data_size;
}

size_t WireFormat::MessageSetItemByteSize(const FieldDescriptor* field,
                                          const Message& message) {
  const Reflection* message_reflection = message.GetReflection();

  size_t our_size = WireFormatLite::kMessageSetItemTagsSize;

  // type_id
  our_size += io::CodedOutputStream::VarintSize32(field->number());

  // message
  size_t message_size;
    const Message& sub_message = message_reflection->GetMessage(message, field);
    message_size = sub_message.ByteSizeLong();

  our_size += io::CodedOutputStream::VarintSize32(message_size);
  our_size += message_size;

  return our_size;
}

// Compute the size of the UnknownFieldSet on the wire.
size_t ComputeUnknownFieldsSize(const InternalMetadata& metadata,
                                size_t total_size, CachedSize* cached_size) {
  total_size += WireFormat::ComputeUnknownFieldsSize(
      metadata.unknown_fields<UnknownFieldSet>(
          UnknownFieldSet::default_instance));
  cached_size->Set(ToCachedSize(total_size));
  return total_size;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
