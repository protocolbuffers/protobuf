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

#ifndef GOOGLE_PROTOBUF_WIRE_FORMAT_INL_H__
#define GOOGLE_PROTOBUF_WIRE_FORMAT_INL_H__

#include <string>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/io/coded_stream.h>


namespace google {
namespace protobuf {
namespace internal {

inline bool WireFormat::ReadInt32(io::CodedInputStream* input, int32* value) {
  uint32 temp;
  if (!input->ReadVarint32(&temp)) return false;
  *value = static_cast<int32>(temp);
  return true;
}
inline bool WireFormat::ReadInt64(io::CodedInputStream* input, int64* value) {
  uint64 temp;
  if (!input->ReadVarint64(&temp)) return false;
  *value = static_cast<int64>(temp);
  return true;
}
inline bool WireFormat::ReadUInt32(io::CodedInputStream* input, uint32* value) {
  return input->ReadVarint32(value);
}
inline bool WireFormat::ReadUInt64(io::CodedInputStream* input, uint64* value) {
  return input->ReadVarint64(value);
}
inline bool WireFormat::ReadSInt32(io::CodedInputStream* input, int32* value) {
  uint32 temp;
  if (!input->ReadVarint32(&temp)) return false;
  *value = ZigZagDecode32(temp);
  return true;
}
inline bool WireFormat::ReadSInt64(io::CodedInputStream* input, int64* value) {
  uint64 temp;
  if (!input->ReadVarint64(&temp)) return false;
  *value = ZigZagDecode64(temp);
  return true;
}
inline bool WireFormat::ReadFixed32(io::CodedInputStream* input,
                                    uint32* value) {
  return input->ReadLittleEndian32(value);
}
inline bool WireFormat::ReadFixed64(io::CodedInputStream* input,
                                    uint64* value) {
  return input->ReadLittleEndian64(value);
}
inline bool WireFormat::ReadSFixed32(io::CodedInputStream* input,
                                     int32* value) {
  uint32 temp;
  if (!input->ReadLittleEndian32(&temp)) return false;
  *value = static_cast<int32>(temp);
  return true;
}
inline bool WireFormat::ReadSFixed64(io::CodedInputStream* input,
                                     int64* value) {
  uint64 temp;
  if (!input->ReadLittleEndian64(&temp)) return false;
  *value = static_cast<int64>(temp);
  return true;
}
inline bool WireFormat::ReadFloat(io::CodedInputStream* input, float* value) {
  uint32 temp;
  if (!input->ReadLittleEndian32(&temp)) return false;
  *value = DecodeFloat(temp);
  return true;
}
inline bool WireFormat::ReadDouble(io::CodedInputStream* input, double* value) {
  uint64 temp;
  if (!input->ReadLittleEndian64(&temp)) return false;
  *value = DecodeDouble(temp);
  return true;
}
inline bool WireFormat::ReadBool(io::CodedInputStream* input, bool* value) {
  uint32 temp;
  if (!input->ReadVarint32(&temp)) return false;
  *value = temp != 0;
  return true;
}
inline bool WireFormat::ReadEnum(io::CodedInputStream* input, int* value) {
  uint32 temp;
  if (!input->ReadVarint32(&temp)) return false;
  *value = static_cast<int>(temp);
  return true;
}

inline bool WireFormat::ReadString(io::CodedInputStream* input, string* value) {
  // WARNING:  In wire_format.cc, both strings and bytes are handled by
  //   ReadString() to avoid code duplication.  If the implementations become
  //   different, you will need to update that usage.
  uint32 length;
  if (!input->ReadVarint32(&length)) return false;
  return input->ReadString(value, length);
}
inline bool WireFormat::ReadBytes(io::CodedInputStream* input, string* value) {
  uint32 length;
  if (!input->ReadVarint32(&length)) return false;
  return input->ReadString(value, length);
}


inline bool WireFormat::ReadGroup(int field_number, io::CodedInputStream* input,
                                  Message* value) {
  if (!input->IncrementRecursionDepth()) return false;
  if (!value->MergePartialFromCodedStream(input)) return false;
  input->DecrementRecursionDepth();
  // Make sure the last thing read was an end tag for this group.
  if (!input->LastTagWas(MakeTag(field_number, WIRETYPE_END_GROUP))) {
    return false;
  }
  return true;
}
inline bool WireFormat::ReadMessage(io::CodedInputStream* input, Message* value) {
  uint32 length;
  if (!input->ReadVarint32(&length)) return false;
  if (!input->IncrementRecursionDepth()) return false;
  io::CodedInputStream::Limit limit = input->PushLimit(length);
  if (!value->MergePartialFromCodedStream(input)) return false;
  // Make sure that parsing stopped when the limit was hit, not at an endgroup
  // tag.
  if (!input->ConsumedEntireMessage()) return false;
  input->PopLimit(limit);
  input->DecrementRecursionDepth();
  return true;
}

template<typename MessageType>
inline bool WireFormat::ReadGroupNoVirtual(int field_number,
                                           io::CodedInputStream* input,
                                           MessageType* value) {
  if (!input->IncrementRecursionDepth()) return false;
  if (!value->MessageType::MergePartialFromCodedStream(input)) return false;
  input->DecrementRecursionDepth();
  // Make sure the last thing read was an end tag for this group.
  if (!input->LastTagWas(MakeTag(field_number, WIRETYPE_END_GROUP))) {
    return false;
  }
  return true;
}
template<typename MessageType>
inline bool WireFormat::ReadMessageNoVirtual(io::CodedInputStream* input,
                                             MessageType* value) {
  uint32 length;
  if (!input->ReadVarint32(&length)) return false;
  if (!input->IncrementRecursionDepth()) return false;
  io::CodedInputStream::Limit limit = input->PushLimit(length);
  if (!value->MessageType::MergePartialFromCodedStream(input)) return false;
  // Make sure that parsing stopped when the limit was hit, not at an endgroup
  // tag.
  if (!input->ConsumedEntireMessage()) return false;
  input->PopLimit(limit);
  input->DecrementRecursionDepth();
  return true;
}

// ===================================================================

inline bool WireFormat::WriteTag(int field_number, WireType type,
                                 io::CodedOutputStream* output) {
  return output->WriteTag(MakeTag(field_number, type));
}

inline bool WireFormat::WriteInt32(int field_number, int32 value,
                                   io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_VARINT, output) &&
         output->WriteVarint32SignExtended(value);
}
inline bool WireFormat::WriteInt64(int field_number, int64 value,
                                   io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_VARINT, output) &&
         output->WriteVarint64(static_cast<uint64>(value));
}
inline bool WireFormat::WriteUInt32(int field_number, uint32 value,
                                    io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_VARINT, output) &&
         output->WriteVarint32(value);
}
inline bool WireFormat::WriteUInt64(int field_number, uint64 value,
                                    io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_VARINT, output) &&
         output->WriteVarint64(value);
}
inline bool WireFormat::WriteSInt32(int field_number, int32 value,
                                    io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_VARINT, output) &&
         output->WriteVarint32(ZigZagEncode32(value));
}
inline bool WireFormat::WriteSInt64(int field_number, int64 value,
                                    io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_VARINT, output) &&
         output->WriteVarint64(ZigZagEncode64(value));
}
inline bool WireFormat::WriteFixed32(int field_number, uint32 value,
                                     io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_FIXED32, output) &&
         output->WriteLittleEndian32(value);
}
inline bool WireFormat::WriteFixed64(int field_number, uint64 value,
                                     io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_FIXED64, output) &&
         output->WriteLittleEndian64(value);
}
inline bool WireFormat::WriteSFixed32(int field_number, int32 value,
                                      io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_FIXED32, output) &&
         output->WriteLittleEndian32(static_cast<uint32>(value));
}
inline bool WireFormat::WriteSFixed64(int field_number, int64 value,
                                      io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_FIXED64, output) &&
         output->WriteLittleEndian64(static_cast<uint64>(value));
}
inline bool WireFormat::WriteFloat(int field_number, float value,
                                   io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_FIXED32, output) &&
         output->WriteLittleEndian32(EncodeFloat(value));
}
inline bool WireFormat::WriteDouble(int field_number, double value,
                                    io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_FIXED64, output) &&
         output->WriteLittleEndian64(EncodeDouble(value));
}
inline bool WireFormat::WriteBool(int field_number, bool value,
                                  io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_VARINT, output) &&
         output->WriteVarint32(value ? 1 : 0);
}
inline bool WireFormat::WriteEnum(int field_number, int value,
                                  io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_VARINT, output) &&
         output->WriteVarint32SignExtended(value);
}

inline bool WireFormat::WriteString(int field_number, const string& value,
                                    io::CodedOutputStream* output) {
  // WARNING:  In wire_format.cc, both strings and bytes are handled by
  //   WriteString() to avoid code duplication.  If the implementations become
  //   different, you will need to update that usage.
  return WriteTag(field_number, WIRETYPE_LENGTH_DELIMITED, output) &&
         output->WriteVarint32(value.size()) &&
         output->WriteString(value);
}
inline bool WireFormat::WriteBytes(int field_number, const string& value,
                                   io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_LENGTH_DELIMITED, output) &&
         output->WriteVarint32(value.size()) &&
         output->WriteString(value);
}


inline bool WireFormat::WriteGroup(int field_number, const Message& value,
                                   io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_START_GROUP, output) &&
         value.SerializeWithCachedSizes(output) &&
         WriteTag(field_number, WIRETYPE_END_GROUP, output);
}
inline bool WireFormat::WriteMessage(int field_number, const Message& value,
                                     io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_LENGTH_DELIMITED, output) &&
         output->WriteVarint32(value.GetCachedSize()) &&
         value.SerializeWithCachedSizes(output);
}

template<typename MessageType>
inline bool WireFormat::WriteGroupNoVirtual(
    int field_number, const MessageType& value,
    io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_START_GROUP, output) &&
         value.MessageType::SerializeWithCachedSizes(output) &&
         WriteTag(field_number, WIRETYPE_END_GROUP, output);
}
template<typename MessageType>
inline bool WireFormat::WriteMessageNoVirtual(
    int field_number, const MessageType& value,
    io::CodedOutputStream* output) {
  return WriteTag(field_number, WIRETYPE_LENGTH_DELIMITED, output) &&
         output->WriteVarint32(value.MessageType::GetCachedSize()) &&
         value.MessageType::SerializeWithCachedSizes(output);
}

// ===================================================================

inline int WireFormat::TagSize(int field_number, FieldDescriptor::Type type) {
  int result = io::CodedOutputStream::VarintSize32(
    field_number << kTagTypeBits);
  if (type == FieldDescriptor::TYPE_GROUP) {
    // Groups have both a start and an end tag.
    return result * 2;
  } else {
    return result;
  }
}

inline int WireFormat::Int32Size(int32 value) {
  return io::CodedOutputStream::VarintSize32SignExtended(value);
}
inline int WireFormat::Int64Size(int64 value) {
  return io::CodedOutputStream::VarintSize64(static_cast<uint64>(value));
}
inline int WireFormat::UInt32Size(uint32 value) {
  return io::CodedOutputStream::VarintSize32(value);
}
inline int WireFormat::UInt64Size(uint64 value) {
  return io::CodedOutputStream::VarintSize64(value);
}
inline int WireFormat::SInt32Size(int32 value) {
  return io::CodedOutputStream::VarintSize32(ZigZagEncode32(value));
}
inline int WireFormat::SInt64Size(int64 value) {
  return io::CodedOutputStream::VarintSize64(ZigZagEncode64(value));
}
inline int WireFormat::EnumSize(int value) {
  return io::CodedOutputStream::VarintSize32SignExtended(value);
}

inline int WireFormat::StringSize(const string& value) {
  return io::CodedOutputStream::VarintSize32(value.size()) +
         value.size();
}
inline int WireFormat::BytesSize(const string& value) {
  return io::CodedOutputStream::VarintSize32(value.size()) +
         value.size();
}


inline int WireFormat::GroupSize(const Message& value) {
  return value.ByteSize();
}
inline int WireFormat::MessageSize(const Message& value) {
  int size = value.ByteSize();
  return io::CodedOutputStream::VarintSize32(size) + size;
}

template<typename MessageType>
inline int WireFormat::GroupSizeNoVirtual(const MessageType& value) {
  return value.MessageType::ByteSize();
}
template<typename MessageType>
inline int WireFormat::MessageSizeNoVirtual(const MessageType& value) {
  int size = value.MessageType::ByteSize();
  return io::CodedOutputStream::VarintSize32(size) + size;
}

}  // namespace internal
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_WIRE_FORMAT_INL_H__
