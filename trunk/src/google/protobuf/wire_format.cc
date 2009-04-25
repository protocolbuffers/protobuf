// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

#include <stack>
#include <string>
#include <vector>

#include <google/protobuf/wire_format_inl.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/unknown_field_set.h>

namespace google {
namespace protobuf {
namespace internal {

namespace {

// This function turns out to be convenient when using some macros later.
inline int GetEnumNumber(const EnumValueDescriptor* descriptor) {
  return descriptor->number();
}

// These are the tags for the old MessageSet format, which was defined as:
//   message MessageSet {
//     repeated group Item = 1 {
//       required int32 type_id = 2;
//       required string message = 3;
//     }
//   }
const int kMessageSetItemStartTag =
  GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(1, WireFormat::WIRETYPE_START_GROUP);
const int kMessageSetItemEndTag =
  GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(1, WireFormat::WIRETYPE_END_GROUP);
const int kMessageSetTypeIdTag =
  GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(2, WireFormat::WIRETYPE_VARINT);
const int kMessageSetMessageTag =
  GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(3, WireFormat::WIRETYPE_LENGTH_DELIMITED);

// Byte size of all tags of a MessageSet::Item combined.
static const int kMessageSetItemTagsSize =
  io::CodedOutputStream::VarintSize32(kMessageSetItemStartTag) +
  io::CodedOutputStream::VarintSize32(kMessageSetItemEndTag) +
  io::CodedOutputStream::VarintSize32(kMessageSetTypeIdTag) +
  io::CodedOutputStream::VarintSize32(kMessageSetMessageTag);

}  // anonymous namespace

const WireFormat::WireType
WireFormat::kWireTypeForFieldType[FieldDescriptor::MAX_TYPE + 1] = {
  static_cast<WireFormat::WireType>(-1),  // invalid
  WIRETYPE_FIXED64,           // TYPE_DOUBLE
  WIRETYPE_FIXED32,           // TYPE_FLOAT
  WIRETYPE_VARINT,            // TYPE_INT64
  WIRETYPE_VARINT,            // TYPE_UINT64
  WIRETYPE_VARINT,            // TYPE_INT32
  WIRETYPE_FIXED64,           // TYPE_FIXED64
  WIRETYPE_FIXED32,           // TYPE_FIXED32
  WIRETYPE_VARINT,            // TYPE_BOOL
  WIRETYPE_LENGTH_DELIMITED,  // TYPE_STRING
  WIRETYPE_START_GROUP,       // TYPE_GROUP
  WIRETYPE_LENGTH_DELIMITED,  // TYPE_MESSAGE
  WIRETYPE_LENGTH_DELIMITED,  // TYPE_BYTES
  WIRETYPE_VARINT,            // TYPE_UINT32
  WIRETYPE_VARINT,            // TYPE_ENUM
  WIRETYPE_FIXED32,           // TYPE_SFIXED32
  WIRETYPE_FIXED64,           // TYPE_SFIXED64
  WIRETYPE_VARINT,            // TYPE_SINT32
  WIRETYPE_VARINT,            // TYPE_SINT64
};

// ===================================================================

bool WireFormat::SkipField(io::CodedInputStream* input, uint32 tag,
                           UnknownFieldSet* unknown_fields) {
  int number = GetTagFieldNumber(tag);

  switch (GetTagWireType(tag)) {
    case WIRETYPE_VARINT: {
      uint64 value;
      if (!input->ReadVarint64(&value)) return false;
      if (unknown_fields != NULL) unknown_fields->AddVarint(number, value);
      return true;
    }
    case WIRETYPE_FIXED64: {
      uint64 value;
      if (!input->ReadLittleEndian64(&value)) return false;
      if (unknown_fields != NULL) unknown_fields->AddFixed64(number, value);
      return true;
    }
    case WIRETYPE_LENGTH_DELIMITED: {
      uint32 length;
      if (!input->ReadVarint32(&length)) return false;
      if (unknown_fields == NULL) {
        if (!input->Skip(length)) return false;
      } else {
        if (!input->ReadString(unknown_fields->AddLengthDelimited(number),
                               length)) {
          return false;
        }
      }
      return true;
    }
    case WIRETYPE_START_GROUP: {
      if (!input->IncrementRecursionDepth()) return false;
      if (!SkipMessage(input, (unknown_fields == NULL) ?
                              NULL : unknown_fields->AddGroup(number))) {
        return false;
      }
      input->DecrementRecursionDepth();
      // Check that the ending tag matched the starting tag.
      if (!input->LastTagWas(
          MakeTag(GetTagFieldNumber(tag), WIRETYPE_END_GROUP))) {
        return false;
      }
      return true;
    }
    case WIRETYPE_END_GROUP: {
      return false;
    }
    case WIRETYPE_FIXED32: {
      uint32 value;
      if (!input->ReadLittleEndian32(&value)) return false;
      if (unknown_fields != NULL) unknown_fields->AddFixed32(number, value);
      return true;
    }
    default: {
      return false;
    }
  }
}

bool WireFormat::SkipMessage(io::CodedInputStream* input,
                             UnknownFieldSet* unknown_fields) {
  while(true) {
    uint32 tag = input->ReadTag();
    if (tag == 0) {
      // End of input.  This is a valid place to end, so return true.
      return true;
    }

    WireType wire_type = GetTagWireType(tag);

    if (wire_type == WIRETYPE_END_GROUP) {
      // Must be the end of the message.
      return true;
    }

    if (!SkipField(input, tag, unknown_fields)) return false;
  }
}

void WireFormat::SerializeUnknownFields(const UnknownFieldSet& unknown_fields,
                                        io::CodedOutputStream* output) {
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);
    switch (field.type()) {
      case UnknownField::TYPE_VARINT:
        output->WriteVarint32(MakeTag(field.number(), WIRETYPE_VARINT));
        output->WriteVarint64(field.varint());
        break;
      case UnknownField::TYPE_FIXED32:
        output->WriteVarint32(MakeTag(field.number(), WIRETYPE_FIXED32));
        output->WriteLittleEndian32(field.fixed32());
        break;
      case UnknownField::TYPE_FIXED64:
        output->WriteVarint32(MakeTag(field.number(), WIRETYPE_FIXED64));
        output->WriteLittleEndian64(field.fixed64());
        break;
      case UnknownField::TYPE_LENGTH_DELIMITED:
        output->WriteVarint32(
            MakeTag(field.number(), WIRETYPE_LENGTH_DELIMITED));
        output->WriteVarint32(field.length_delimited().size());
        output->WriteString(field.length_delimited());
        break;
      case UnknownField::TYPE_GROUP:
        output->WriteVarint32(MakeTag(field.number(),WIRETYPE_START_GROUP));
        SerializeUnknownFields(field.group(), output);
        output->WriteVarint32(MakeTag(field.number(), WIRETYPE_END_GROUP));
        break;
    }
  }
}

uint8* WireFormat::SerializeUnknownFieldsToArray(
    const UnknownFieldSet& unknown_fields,
    uint8* target) {
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);

    switch (field.type()) {
      case UnknownField::TYPE_VARINT:
        target = WriteInt64ToArray(field.number(), field.varint(), target);
        break;
      case UnknownField::TYPE_FIXED32:
        target = WriteFixed32ToArray(field.number(), field.fixed32(), target);
        break;
      case UnknownField::TYPE_FIXED64:
        target = WriteFixed64ToArray(field.number(), field.fixed64(), target);
        break;
      case UnknownField::TYPE_LENGTH_DELIMITED:
        target =
          WriteBytesToArray(field.number(), field.length_delimited(), target);
        break;
      case UnknownField::TYPE_GROUP:
        target = WriteTagToArray(field.number(), WIRETYPE_START_GROUP, target);
        target = SerializeUnknownFieldsToArray(field.group(), target);
        target = WriteTagToArray(field.number(), WIRETYPE_END_GROUP, target);
        break;
    }
  }
  return target;
}

void WireFormat::SerializeUnknownMessageSetItems(
    const UnknownFieldSet& unknown_fields,
    io::CodedOutputStream* output) {
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);
    // The only unknown fields that are allowed to exist in a MessageSet are
    // messages, which are length-delimited.
    if (field.type() == UnknownField::TYPE_LENGTH_DELIMITED) {
      const string& data = field.length_delimited();

      // Start group.
      output->WriteVarint32(kMessageSetItemStartTag);

      // Write type ID.
      output->WriteVarint32(kMessageSetTypeIdTag);
      output->WriteVarint32(field.number());

      // Write message.
      output->WriteVarint32(kMessageSetMessageTag);
      output->WriteVarint32(data.size());
      output->WriteString(data);

      // End group.
      output->WriteVarint32(kMessageSetItemEndTag);
    }
  }
}

uint8* WireFormat::SerializeUnknownMessageSetItemsToArray(
    const UnknownFieldSet& unknown_fields,
    uint8* target) {
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);

    // The only unknown fields that are allowed to exist in a MessageSet are
    // messages, which are length-delimited.
    if (field.type() == UnknownField::TYPE_LENGTH_DELIMITED) {
      const string& data = field.length_delimited();

      // Start group.
      target =
        io::CodedOutputStream::WriteTagToArray(kMessageSetItemStartTag, target);

      // Write type ID.
      target =
        io::CodedOutputStream::WriteTagToArray(kMessageSetTypeIdTag, target);
      target =
        io::CodedOutputStream::WriteVarint32ToArray(field.number(), target);

      // Write message.
      target =
        io::CodedOutputStream::WriteTagToArray(kMessageSetMessageTag, target);
      target = io::CodedOutputStream::WriteVarint32ToArray(data.size(), target);
      target = io::CodedOutputStream::WriteStringToArray(data, target);

      // End group.
      target =
        io::CodedOutputStream::WriteTagToArray(kMessageSetItemEndTag, target);
    }
  }

  return target;
}

int WireFormat::ComputeUnknownFieldsSize(
    const UnknownFieldSet& unknown_fields) {
  int size = 0;
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);

    switch (field.type()) {
      case UnknownField::TYPE_VARINT:
        size += io::CodedOutputStream::VarintSize32(
          MakeTag(field.number(), WIRETYPE_VARINT));
        size += io::CodedOutputStream::VarintSize64(field.varint());
        break;
      case UnknownField::TYPE_FIXED32:
        size += io::CodedOutputStream::VarintSize32(
          MakeTag(field.number(), WIRETYPE_FIXED32));
        size += sizeof(int32);
        break;
      case UnknownField::TYPE_FIXED64:
        size += io::CodedOutputStream::VarintSize32(
          MakeTag(field.number(), WIRETYPE_FIXED64));
        size += sizeof(int64);
        break;
      case UnknownField::TYPE_LENGTH_DELIMITED:
        size += io::CodedOutputStream::VarintSize32(
          MakeTag(field.number(), WIRETYPE_LENGTH_DELIMITED));
        size += io::CodedOutputStream::VarintSize32(
          field.length_delimited().size());
        size += field.length_delimited().size();
        break;
      case UnknownField::TYPE_GROUP:
        size += io::CodedOutputStream::VarintSize32(
          MakeTag(field.number(), WIRETYPE_START_GROUP));
        size += ComputeUnknownFieldsSize(field.group());
        size += io::CodedOutputStream::VarintSize32(
          MakeTag(field.number(), WIRETYPE_END_GROUP));
        break;
    }
  }

  return size;
}

int WireFormat::ComputeUnknownMessageSetItemsSize(
    const UnknownFieldSet& unknown_fields) {
  int size = 0;
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);

    // The only unknown fields that are allowed to exist in a MessageSet are
    // messages, which are length-delimited.
    if (field.type() == UnknownField::TYPE_LENGTH_DELIMITED) {
      size += kMessageSetItemTagsSize;
      size += io::CodedOutputStream::VarintSize32(field.number());
      size += io::CodedOutputStream::VarintSize32(
        field.length_delimited().size());
      size += field.length_delimited().size();
    }
  }

  return size;
}

// ===================================================================

bool WireFormat::ParseAndMergePartial(io::CodedInputStream* input,
                                      Message* message) {
  const Descriptor* descriptor = message->GetDescriptor();
  const Reflection* message_reflection = message->GetReflection();

  while(true) {
    uint32 tag = input->ReadTag();
    if (tag == 0) {
      // End of input.  This is a valid place to end, so return true.
      return true;
    }

    if (GetTagWireType(tag) == WIRETYPE_END_GROUP) {
      // Must be the end of the message.
      return true;
    }

    const FieldDescriptor* field = NULL;

    if (descriptor != NULL) {
      int field_number = GetTagFieldNumber(tag);
      field = descriptor->FindFieldByNumber(field_number);

      // If that failed, check if the field is an extension.
      if (field == NULL && descriptor->IsExtensionNumber(field_number)) {
        field = message_reflection->FindKnownExtensionByNumber(field_number);
      }

      // If that failed, but we're a MessageSet, and this is the tag for a
      // MessageSet item, then parse that.
      if (field == NULL &&
          descriptor->options().message_set_wire_format() &&
          tag == kMessageSetItemStartTag) {
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

bool WireFormat::ParseAndMergeField(
    uint32 tag,
    const FieldDescriptor* field,        // May be NULL for unknown
    Message* message,
    io::CodedInputStream* input) {
  const Reflection* message_reflection = message->GetReflection();

  if (field == NULL ||
      GetTagWireType(tag) != WireTypeForField(field)) {
    // We don't recognize this field.  Either the field number is unknown
    // or the wire type doesn't match.  Put it in our unknown field set.
    return SkipField(input, tag,
                     message_reflection->MutableUnknownFields(message));
  }

  if (field->options().packed()) {
    uint32 length;
    if (!input->ReadVarint32(&length)) return false;
    io::CodedInputStream::Limit limit = input->PushLimit(length);

    switch (field->type()) {
#define HANDLE_PACKED_TYPE(TYPE, TYPE_METHOD, CPPTYPE, CPPTYPE_METHOD)    \
      case FieldDescriptor::TYPE_##TYPE: {                                \
        while (input->BytesUntilLimit() > 0) {                            \
          CPPTYPE value;                                                  \
          if (!Read##TYPE_METHOD(input, &value)) return false;            \
          message_reflection->Add##CPPTYPE_METHOD(message, field, value); \
        }                                                                 \
        break;                                                            \
      }

      HANDLE_PACKED_TYPE( INT32,  Int32,  int32,  Int32)
      HANDLE_PACKED_TYPE( INT64,  Int64,  int64,  Int64)
      HANDLE_PACKED_TYPE(SINT32, SInt32,  int32,  Int32)
      HANDLE_PACKED_TYPE(SINT64, SInt64,  int64,  Int64)
      HANDLE_PACKED_TYPE(UINT32, UInt32, uint32, UInt32)
      HANDLE_PACKED_TYPE(UINT64, UInt64, uint64, UInt64)

      HANDLE_PACKED_TYPE( FIXED32,  Fixed32, uint32, UInt32)
      HANDLE_PACKED_TYPE( FIXED64,  Fixed64, uint64, UInt64)
      HANDLE_PACKED_TYPE(SFIXED32, SFixed32,  int32,  Int32)
      HANDLE_PACKED_TYPE(SFIXED64, SFixed64,  int64,  Int64)

      HANDLE_PACKED_TYPE(FLOAT , Float , float , Float )
      HANDLE_PACKED_TYPE(DOUBLE, Double, double, Double)

      HANDLE_PACKED_TYPE(BOOL, Bool, bool, Bool)
#undef HANDLE_PACKED_TYPE

      case FieldDescriptor::TYPE_ENUM: {
        while (input->BytesUntilLimit() > 0) {
          int value;
          if (!ReadEnum(input, &value)) return false;
          const EnumValueDescriptor* enum_value =
              field->enum_type()->FindValueByNumber(value);
          if (enum_value != NULL) {
            message_reflection->AddEnum(message, field, enum_value);
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
    switch (field->type()) {
#define HANDLE_TYPE(TYPE, TYPE_METHOD, CPPTYPE, CPPTYPE_METHOD)           \
      case FieldDescriptor::TYPE_##TYPE: {                                \
        CPPTYPE value;                                                    \
        if (!Read##TYPE_METHOD(input, &value)) return false;              \
        if (field->is_repeated()) {                                       \
          message_reflection->Add##CPPTYPE_METHOD(message, field, value); \
        } else {                                                          \
          message_reflection->Set##CPPTYPE_METHOD(message, field, value); \
        }                                                                 \
        break;                                                            \
      }

      HANDLE_TYPE( INT32,  Int32,  int32,  Int32)
      HANDLE_TYPE( INT64,  Int64,  int64,  Int64)
      HANDLE_TYPE(SINT32, SInt32,  int32,  Int32)
      HANDLE_TYPE(SINT64, SInt64,  int64,  Int64)
      HANDLE_TYPE(UINT32, UInt32, uint32, UInt32)
      HANDLE_TYPE(UINT64, UInt64, uint64, UInt64)

      HANDLE_TYPE( FIXED32,  Fixed32, uint32, UInt32)
      HANDLE_TYPE( FIXED64,  Fixed64, uint64, UInt64)
      HANDLE_TYPE(SFIXED32, SFixed32,  int32,  Int32)
      HANDLE_TYPE(SFIXED64, SFixed64,  int64,  Int64)

      HANDLE_TYPE(FLOAT , Float , float , Float )
      HANDLE_TYPE(DOUBLE, Double, double, Double)

      HANDLE_TYPE(BOOL, Bool, bool, Bool)

      HANDLE_TYPE(STRING, String, string, String)
      HANDLE_TYPE(BYTES, Bytes, string, String)

#undef HANDLE_TYPE

      case FieldDescriptor::TYPE_ENUM: {
        int value;
        if (!ReadEnum(input, &value)) return false;
        const EnumValueDescriptor* enum_value =
          field->enum_type()->FindValueByNumber(value);
        if (enum_value != NULL) {
          if (field->is_repeated()) {
            message_reflection->AddEnum(message, field, enum_value);
          } else {
            message_reflection->SetEnum(message, field, enum_value);
          }
        } else {
          // The enum value is not one of the known values.  Add it to the
          // UnknownFieldSet.
          int64 sign_extended_value = static_cast<int64>(value);
          message_reflection->MutableUnknownFields(message)
                            ->AddVarint(GetTagFieldNumber(tag),
                                        sign_extended_value);
        }
        break;
      }


      case FieldDescriptor::TYPE_GROUP: {
        Message* sub_message;
        if (field->is_repeated()) {
          sub_message = message_reflection->AddMessage(message, field);
        } else {
          sub_message = message_reflection->MutableMessage(message, field);
        }

        if (!ReadGroup(GetTagFieldNumber(tag), input, sub_message))
          return false;
        break;
      }

      case FieldDescriptor::TYPE_MESSAGE: {
        Message* sub_message;
        if (field->is_repeated()) {
          sub_message = message_reflection->AddMessage(message, field);
        } else {
          sub_message = message_reflection->MutableMessage(message, field);
        }

        if (!ReadMessage(input, sub_message)) return false;
        break;
      }
    }
  }

  return true;
}

bool WireFormat::ParseAndMergeMessageSetItem(
    io::CodedInputStream* input,
    Message* message) {
  const Reflection* message_reflection = message->GetReflection();

  // This method parses a group which should contain two fields:
  //   required int32 type_id = 2;
  //   required data message = 3;

  // Once we see a type_id, we'll construct a fake tag for this extension
  // which is the tag it would have had under the proto2 extensions wire
  // format.
  uint32 fake_tag = 0;

  // Once we see a type_id, we'll look up the FieldDescriptor for the
  // extension.
  const FieldDescriptor* field = NULL;

  // If we see message data before the type_id, we'll append it to this so
  // we can parse it later.  This will probably never happen in practice,
  // as no MessageSet encoder I know of writes the message before the type ID.
  // But, it's technically valid so we should allow it.
  // TODO(kenton):  Use a Cord instead?  Do I care?
  string message_data;

  while (true) {
    uint32 tag = input->ReadTag();
    if (tag == 0) return false;

    switch (tag) {
      case kMessageSetTypeIdTag: {
        uint32 type_id;
        if (!input->ReadVarint32(&type_id)) return false;
        fake_tag = MakeTag(type_id, WIRETYPE_LENGTH_DELIMITED);
        field = message_reflection->FindKnownExtensionByNumber(type_id);

        if (!message_data.empty()) {
          // We saw some message data before the type_id.  Have to parse it
          // now.
          io::ArrayInputStream raw_input(message_data.data(),
                                         message_data.size());
          io::CodedInputStream sub_input(&raw_input);
          if (!ParseAndMergeField(fake_tag, field, message,
                                  &sub_input)) {
            return false;
          }
          message_data.clear();
        }

        break;
      }

      case kMessageSetMessageTag: {
        if (fake_tag == 0) {
          // We haven't seen a type_id yet.  Append this data to message_data.
          string temp;
          uint32 length;
          if (!input->ReadVarint32(&length)) return false;
          if (!input->ReadString(&temp, length)) return false;
          message_data.append(temp);
        } else {
          // Already saw type_id, so we can parse this directly.
          if (!ParseAndMergeField(fake_tag, field, message, input)) {
            return false;
          }
        }

        break;
      }

      case kMessageSetItemEndTag: {
        return true;
      }

      default: {
        if (!SkipField(input, tag, NULL)) return false;
      }
    }
  }
}

// ===================================================================

void WireFormat::SerializeWithCachedSizes(
    const Message& message,
    int size, io::CodedOutputStream* output) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* message_reflection = message.GetReflection();
  int expected_endpoint = output->ByteCount() + size;

  vector<const FieldDescriptor*> fields;
  message_reflection->ListFields(message, &fields);
  for (int i = 0; i < fields.size(); i++) {
    SerializeFieldWithCachedSizes(fields[i], message, output);
  }

  if (descriptor->options().message_set_wire_format()) {
    SerializeUnknownMessageSetItems(
        message_reflection->GetUnknownFields(message), output);
  } else {
    SerializeUnknownFields(
        message_reflection->GetUnknownFields(message), output);
  }

  GOOGLE_CHECK_EQ(output->ByteCount(), expected_endpoint)
    << ": Protocol message serialized to a size different from what was "
       "originally expected.  Perhaps it was modified by another thread "
       "during serialization?";
}

void WireFormat::SerializeFieldWithCachedSizes(
    const FieldDescriptor* field,
    const Message& message,
    io::CodedOutputStream* output) {
  const Reflection* message_reflection = message.GetReflection();

  if (field->is_extension() &&
      field->containing_type()->options().message_set_wire_format() &&
      field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
      !field->is_repeated()) {
    SerializeMessageSetItemWithCachedSizes(field, message, output);
    return;
  }

  int count = 0;

  if (field->is_repeated()) {
    count = message_reflection->FieldSize(message, field);
  } else if (message_reflection->HasField(message, field)) {
    count = 1;
  }

  const bool is_packed = field->options().packed();
  if (is_packed && count > 0) {
    WriteTag(field->number(), WIRETYPE_LENGTH_DELIMITED, output);
    const int data_size = FieldDataOnlyByteSize(field, message);
    output->WriteVarint32(data_size);
  }

  for (int j = 0; j < count; j++) {
    switch (field->type()) {
#define HANDLE_PRIMITIVE_TYPE(TYPE, CPPTYPE, TYPE_METHOD, CPPTYPE_METHOD)      \
      case FieldDescriptor::TYPE_##TYPE: {                                     \
        const CPPTYPE value = field->is_repeated() ?                           \
                              message_reflection->GetRepeated##CPPTYPE_METHOD( \
                                message, field, j) :                           \
                              message_reflection->Get##CPPTYPE_METHOD(         \
                                message, field);                               \
        if (is_packed) {                                                       \
          Write##TYPE_METHOD##NoTag(value, output);                            \
        } else {                                                               \
          Write##TYPE_METHOD(field->number(), value, output);                  \
        }                                                                      \
        break;                                                                 \
      }

      HANDLE_PRIMITIVE_TYPE( INT32,  int32,  Int32,  Int32)
      HANDLE_PRIMITIVE_TYPE( INT64,  int64,  Int64,  Int64)
      HANDLE_PRIMITIVE_TYPE(SINT32,  int32, SInt32,  Int32)
      HANDLE_PRIMITIVE_TYPE(SINT64,  int64, SInt64,  Int64)
      HANDLE_PRIMITIVE_TYPE(UINT32, uint32, UInt32, UInt32)
      HANDLE_PRIMITIVE_TYPE(UINT64, uint64, UInt64, UInt64)

      HANDLE_PRIMITIVE_TYPE( FIXED32, uint32,  Fixed32, UInt32)
      HANDLE_PRIMITIVE_TYPE( FIXED64, uint64,  Fixed64, UInt64)
      HANDLE_PRIMITIVE_TYPE(SFIXED32,  int32, SFixed32,  Int32)
      HANDLE_PRIMITIVE_TYPE(SFIXED64,  int64, SFixed64,  Int64)

      HANDLE_PRIMITIVE_TYPE(FLOAT , float , Float , Float )
      HANDLE_PRIMITIVE_TYPE(DOUBLE, double, Double, Double)

      HANDLE_PRIMITIVE_TYPE(BOOL, bool, Bool, Bool)
#undef HANDLE_PRIMITIVE_TYPE

#define HANDLE_TYPE(TYPE, TYPE_METHOD, CPPTYPE_METHOD)                       \
      case FieldDescriptor::TYPE_##TYPE:                                     \
        Write##TYPE_METHOD(                                                  \
              field->number(),                                               \
              field->is_repeated() ?                                         \
                message_reflection->GetRepeated##CPPTYPE_METHOD(             \
                  message, field, j) :                                       \
                message_reflection->Get##CPPTYPE_METHOD(message, field),     \
              output);                                                       \
        break;

      HANDLE_TYPE(GROUP  , Group  , Message)
      HANDLE_TYPE(MESSAGE, Message, Message)
#undef HANDLE_TYPE

      case FieldDescriptor::TYPE_ENUM: {
        const EnumValueDescriptor* value = field->is_repeated() ?
          message_reflection->GetRepeatedEnum(message, field, j) :
          message_reflection->GetEnum(message, field);
        if (is_packed) {
          WriteEnumNoTag(value->number(), output);
        } else {
          WriteEnum(field->number(), value->number(), output);
        }
        break;
      }

      // Handle strings separately so that we can get string references
      // instead of copying.
      case FieldDescriptor::TYPE_STRING: {
          string scratch;
          const string& value = field->is_repeated() ?
            message_reflection->GetRepeatedStringReference(
              message, field, j, &scratch) :
            message_reflection->GetStringReference(message, field, &scratch);
          WriteString(field->number(), value, output);
        break;
      }

      case FieldDescriptor::TYPE_BYTES: {
          string scratch;
          const string& value = field->is_repeated() ?
            message_reflection->GetRepeatedStringReference(
              message, field, j, &scratch) :
            message_reflection->GetStringReference(message, field, &scratch);
          WriteBytes(field->number(), value, output);
        break;
      }
    }
  }
}

void WireFormat::SerializeMessageSetItemWithCachedSizes(
    const FieldDescriptor* field,
    const Message& message,
    io::CodedOutputStream* output) {
  const Reflection* message_reflection = message.GetReflection();

  // Start group.
  output->WriteVarint32(kMessageSetItemStartTag);

  // Write type ID.
  output->WriteVarint32(kMessageSetTypeIdTag);
  output->WriteVarint32(field->number());

  // Write message.
  output->WriteVarint32(kMessageSetMessageTag);

  const Message& sub_message = message_reflection->GetMessage(message, field);
  output->WriteVarint32(sub_message.GetCachedSize());
  sub_message.SerializeWithCachedSizes(output);

  // End group.
  output->WriteVarint32(kMessageSetItemEndTag);
}

// ===================================================================

int WireFormat::ByteSize(const Message& message) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* message_reflection = message.GetReflection();

  int our_size = 0;

  vector<const FieldDescriptor*> fields;
  message_reflection->ListFields(message, &fields);
  for (int i = 0; i < fields.size(); i++) {
    our_size += FieldByteSize(fields[i], message);
  }

  if (descriptor->options().message_set_wire_format()) {
    our_size += ComputeUnknownMessageSetItemsSize(
      message_reflection->GetUnknownFields(message));
  } else {
    our_size += ComputeUnknownFieldsSize(
      message_reflection->GetUnknownFields(message));
  }

  return our_size;
}

int WireFormat::FieldByteSize(
    const FieldDescriptor* field,
    const Message& message) {
  const Reflection* message_reflection = message.GetReflection();

  if (field->is_extension() &&
      field->containing_type()->options().message_set_wire_format() &&
      field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
      !field->is_repeated()) {
    return MessageSetItemByteSize(field, message);
  }

  int count = 0;
  if (field->is_repeated()) {
    count = message_reflection->FieldSize(message, field);
  } else if (message_reflection->HasField(message, field)) {
    count = 1;
  }

  const int data_size = FieldDataOnlyByteSize(field, message);
  int our_size = data_size;
  if (field->options().packed()) {
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

int WireFormat::FieldDataOnlyByteSize(
    const FieldDescriptor* field,
    const Message& message) {
  const Reflection* message_reflection = message.GetReflection();

  int count = 0;
  if (field->is_repeated()) {
    count = message_reflection->FieldSize(message, field);
  } else if (message_reflection->HasField(message, field)) {
    count = 1;
  }

  int data_size = 0;
  switch (field->type()) {
#define HANDLE_TYPE(TYPE, TYPE_METHOD, CPPTYPE_METHOD)                     \
    case FieldDescriptor::TYPE_##TYPE:                                     \
      if (field->is_repeated()) {                                          \
        for (int j = 0; j < count; j++) {                                  \
          data_size += TYPE_METHOD##Size(                                  \
            message_reflection->GetRepeated##CPPTYPE_METHOD(               \
              message, field, j));                                         \
        }                                                                  \
      } else {                                                             \
        data_size += TYPE_METHOD##Size(                                    \
          message_reflection->Get##CPPTYPE_METHOD(message, field));        \
      }                                                                    \
      break;

#define HANDLE_FIXED_TYPE(TYPE, TYPE_METHOD)                               \
    case FieldDescriptor::TYPE_##TYPE:                                     \
      data_size += count * k##TYPE_METHOD##Size;                           \
      break;

    HANDLE_TYPE( INT32,  Int32,  Int32)
    HANDLE_TYPE( INT64,  Int64,  Int64)
    HANDLE_TYPE(SINT32, SInt32,  Int32)
    HANDLE_TYPE(SINT64, SInt64,  Int64)
    HANDLE_TYPE(UINT32, UInt32, UInt32)
    HANDLE_TYPE(UINT64, UInt64, UInt64)

    HANDLE_FIXED_TYPE( FIXED32,  Fixed32)
    HANDLE_FIXED_TYPE( FIXED64,  Fixed64)
    HANDLE_FIXED_TYPE(SFIXED32, SFixed32)
    HANDLE_FIXED_TYPE(SFIXED64, SFixed64)

    HANDLE_FIXED_TYPE(FLOAT , Float )
    HANDLE_FIXED_TYPE(DOUBLE, Double)

    HANDLE_FIXED_TYPE(BOOL, Bool)

    HANDLE_TYPE(GROUP  , Group  , Message)
    HANDLE_TYPE(MESSAGE, Message, Message)
#undef HANDLE_TYPE
#undef HANDLE_FIXED_TYPE

    case FieldDescriptor::TYPE_ENUM: {
      if (field->is_repeated()) {
        for (int j = 0; j < count; j++) {
          data_size += EnumSize(
            message_reflection->GetRepeatedEnum(message, field, j)->number());
        }
      } else {
        data_size += EnumSize(
          message_reflection->GetEnum(message, field)->number());
      }
      break;
    }

    // Handle strings separately so that we can get string references
    // instead of copying.
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES: {
        for (int j = 0; j < count; j++) {
          string scratch;
          const string& value = field->is_repeated() ?
            message_reflection->GetRepeatedStringReference(
              message, field, j, &scratch) :
            message_reflection->GetStringReference(message, field, &scratch);
          data_size += StringSize(value);
        }
      break;
    }
  }
  return data_size;
}

int WireFormat::MessageSetItemByteSize(
    const FieldDescriptor* field,
    const Message& message) {
  const Reflection* message_reflection = message.GetReflection();

  int our_size = kMessageSetItemTagsSize;

  // type_id
  our_size += io::CodedOutputStream::VarintSize32(field->number());

  // message
  const Message& sub_message = message_reflection->GetMessage(message, field);
  int message_size = sub_message.ByteSize();

  our_size += io::CodedOutputStream::VarintSize32(message_size);
  our_size += message_size;

  return our_size;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
