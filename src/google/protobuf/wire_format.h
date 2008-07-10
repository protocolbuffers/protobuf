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
//         atenasio@google.com (Chris Atenasio) (ZigZag transform)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// This header is logically internal, but is made public because it is used
// from protocol-compiler-generated code, which may reside in other components.

#ifndef GOOGLE_PROTOBUF_WIRE_FORMAT_H__
#define GOOGLE_PROTOBUF_WIRE_FORMAT_H__

#include <string>
#include <google/protobuf/message.h>  // Message::Reflection
#include <google/protobuf/descriptor.h>

namespace google {

namespace protobuf {
  namespace io {
    class CodedInputStream;      // coded_stream.h
    class CodedOutputStream;     // coded_stream.h
  }
  class UnknownFieldSet;       // unknown_field_set.h
}

namespace protobuf {
namespace internal {

// This class is for internal use by the protocol buffer library and by
// protocol-complier-generated message classes.  It must not be called
// directly by clients.
//
// This class contains helpers for implementing the binary protocol buffer
// wire format.  These helpers are called primarily by generated code.  The
// class also contains reflection-based implementations of the wire format.
//
// This class is really a namespace that contains only static methods.
class LIBPROTOBUF_EXPORT WireFormat {
 public:
  // These procedures can be used to implement the methods of Message which
  // handle parsing and serialization of the protocol buffer wire format
  // using only the Message::Reflection interface.  When you ask the protocol
  // compiler to optimize for code size rather than speed, it will implement
  // those methods in terms of these procedures.  Of course, these are much
  // slower than the specialized implementations which the protocol compiler
  // generates when told to optimize for speed.

  // Read a message in protocol buffer wire format.
  //
  // This procedure reads either to the end of the input stream or through
  // a WIRETYPE_END_GROUP tag ending the message, whichever comes first.
  // It returns false if the input is invalid.
  //
  // Required fields are NOT checked by this method.  You must call
  // IsInitialized() on the resulting message yourself.
  static bool ParseAndMergePartial(const Descriptor* descriptor,
                                   io::CodedInputStream* input,
                                   Message::Reflection* message_reflection);

  // Serialize a message in protocol buffer wire format.
  //
  // Any embedded messages within the message must have their correct sizes
  // cached.  However, the top-level message need not; its size is passed as
  // a parameter to this procedure.
  //
  // These return false iff the underlying stream returns a write error.
  static bool SerializeWithCachedSizes(
      const Descriptor* descriptor,
      const Message::Reflection* message_reflection,
      int size, io::CodedOutputStream* output);

  // Implements Message::ByteSize() via reflection.  WARNING:  The result
  // of this method is *not* cached anywhere.  However, all embedded messages
  // will have their ByteSize() methods called, so their sizes will be cached.
  // Therefore, calling this method is sufficient to allow you to call
  // WireFormat::SerializeWithCachedSizes() on the same object.
  static int ByteSize(const Descriptor* descriptor,
                      const Message::Reflection* message_reflection);

  // -----------------------------------------------------------------
  // Helpers for dealing with unknown fields

  // Skips a field value of the given WireType.  The input should start
  // positioned immediately after the tag.  If unknown_fields is non-NULL,
  // the contents of the field will be added to it.
  static bool SkipField(io::CodedInputStream* input, uint32 tag,
                        UnknownFieldSet* unknown_fields);

  // Reads and ignores a message from the input.  If unknown_fields is non-NULL,
  // the contents will be added to it.
  static bool SkipMessage(io::CodedInputStream* input,
                          UnknownFieldSet* unknown_fields);

  // Write the contents of an UnknownFieldSet to the output.
  static bool SerializeUnknownFields(const UnknownFieldSet& unknown_fields,
                                     io::CodedOutputStream* output);

  // Same thing except for messages that have the message_set_wire_format
  // option.
  static bool SerializeUnknownMessageSetItems(
      const UnknownFieldSet& unknown_fields,
      io::CodedOutputStream* output);

  // Compute the size of the UnknownFieldSet on the wire.
  static int ComputeUnknownFieldsSize(const UnknownFieldSet& unknown_fields);

  // Same thing except for messages that have the message_set_wire_format
  // option.
  static int ComputeUnknownMessageSetItemsSize(
      const UnknownFieldSet& unknown_fields);

  // -----------------------------------------------------------------
  // Helper constants and functions related to the format.  These are
  // mostly meant for internal and generated code to use.

  // The wire format is composed of a sequence of tag/value pairs, each
  // of which contains the value of one field (or one element of a repeated
  // field).  Each tag is encoded as a varint.  The lower bits of the tag
  // identify its wire type, which specifies the format of the data to follow.
  // The rest of the bits contain the field number.  Each type of field (as
  // declared by FieldDescriptor::Type, in descriptor.h) maps to one of
  // these wire types.  Immediately following each tag is the field's value,
  // encoded in the format specified by the wire type.  Because the tag
  // identifies the encoding of this data, it is possible to skip
  // unrecognized fields for forwards compatibility.

  enum WireType {
    WIRETYPE_VARINT           = 0,
    WIRETYPE_FIXED64          = 1,
    WIRETYPE_LENGTH_DELIMITED = 2,
    WIRETYPE_START_GROUP      = 3,
    WIRETYPE_END_GROUP        = 4,
    WIRETYPE_FIXED32          = 5,
  };

  static inline WireType WireTypeForFieldType(FieldDescriptor::Type type) {
    return kWireTypeForFieldType[type];
  }

  // Number of bits in a tag which identify the wire type.
  static const int kTagTypeBits = 3;
  // Mask for those bits.
  static const uint32 kTagTypeMask = (1 << kTagTypeBits) - 1;

  // Helper functions for encoding and decoding tags.  (Inlined below.)
  static uint32 MakeTag(const FieldDescriptor* field);
  static uint32 MakeTag(int field_number, WireType type);
  static WireType GetTagWireType(uint32 tag);
  static int GetTagFieldNumber(uint32 tag);

  // Helper functions for converting between floats/doubles and IEEE-754
  // uint32s/uint64s so that they can be written.  (Assumes your platform
  // uses IEEE-754 floats.)
  static uint32 EncodeFloat(float value);
  static float DecodeFloat(uint32 value);
  static uint64 EncodeDouble(double value);
  static double DecodeDouble(uint64 value);

  // Helper functions for mapping signed integers to unsigned integers in
  // such a way that numbers with small magnitudes will encode to smaller
  // varints.  If you simply static_cast a negative number to an unsigned
  // number and varint-encode it, it will always take 10 bytes, defeating
  // the purpose of varint.  So, for the "sint32" and "sint64" field types,
  // we ZigZag-encode the values.
  static uint32 ZigZagEncode32(int32 n);
  static int32  ZigZagDecode32(uint32 n);
  static uint64 ZigZagEncode64(int64 n);
  static int64  ZigZagDecode64(uint64 n);

  // Parse a single field.  The input should start out positioned immidately
  // after the tag.
  static bool ParseAndMergeField(
      uint32 tag,
      const FieldDescriptor* field,        // May be NULL for unknown
      Message::Reflection* message_reflection,
      io::CodedInputStream* input);

  // Serialize a single field.
  static bool SerializeFieldWithCachedSizes(
      const FieldDescriptor* field,        // Cannot be NULL
      const Message::Reflection* message_reflection,
      io::CodedOutputStream* output);

  // Compute size of a single field.  If the field is a message type, this
  // will call ByteSize() for the embedded message, insuring that it caches
  // its size.
  static int FieldByteSize(
      const FieldDescriptor* field,        // Cannot be NULL
      const Message::Reflection* message_reflection);

  // =================================================================
  // Methods for reading/writing individual field.  The implementations
  // of these methods are defined in wire_format_inl.h; you must #include
  // that file to use these.

// Avoid ugly line wrapping
#define input  io::CodedInputStream*  input
#define output io::CodedOutputStream* output
#define field_number int field_number
#define INL GOOGLE_ATTRIBUTE_ALWAYS_INLINE

  // Read fields, not including tags.  The assumption is that you already
  // read the tag to determine what field to read.
  static inline bool ReadInt32   (input,  int32* value);
  static inline bool ReadInt64   (input,  int64* value);
  static inline bool ReadUInt32  (input, uint32* value);
  static inline bool ReadUInt64  (input, uint64* value);
  static inline bool ReadSInt32  (input,  int32* value);
  static inline bool ReadSInt64  (input,  int64* value);
  static inline bool ReadFixed32 (input, uint32* value);
  static inline bool ReadFixed64 (input, uint64* value);
  static inline bool ReadSFixed32(input,  int32* value);
  static inline bool ReadSFixed64(input,  int64* value);
  static inline bool ReadFloat   (input,  float* value);
  static inline bool ReadDouble  (input, double* value);
  static inline bool ReadBool    (input,   bool* value);
  static inline bool ReadEnum    (input,    int* value);

  static inline bool ReadString(input, string* value);
  static inline bool ReadBytes (input, string* value);

  static inline bool ReadGroup  (field_number, input, Message* value);
  static inline bool ReadMessage(input, Message* value);

  // Like above, but de-virtualize the call to MergePartialFromCodedStream().
  // The pointer must point at an instance of MessageType, *not* a subclass (or
  // the subclass must not override MergePartialFromCodedStream()).
  template<typename MessageType>
  static inline bool ReadGroupNoVirtual(field_number, input,
                                        MessageType* value);
  template<typename MessageType>
  static inline bool ReadMessageNoVirtual(input, MessageType* value);

  // Write a tag.  The Write*() functions automatically include the tag, so
  // normally there's no need to call this.
  static inline bool WriteTag(field_number, WireType type, output) INL;

  // Write fields, including tags.
  static inline bool WriteInt32   (field_number,  int32 value, output) INL;
  static inline bool WriteInt64   (field_number,  int64 value, output) INL;
  static inline bool WriteUInt32  (field_number, uint32 value, output) INL;
  static inline bool WriteUInt64  (field_number, uint64 value, output) INL;
  static inline bool WriteSInt32  (field_number,  int32 value, output) INL;
  static inline bool WriteSInt64  (field_number,  int64 value, output) INL;
  static inline bool WriteFixed32 (field_number, uint32 value, output) INL;
  static inline bool WriteFixed64 (field_number, uint64 value, output) INL;
  static inline bool WriteSFixed32(field_number,  int32 value, output) INL;
  static inline bool WriteSFixed64(field_number,  int64 value, output) INL;
  static inline bool WriteFloat   (field_number,  float value, output) INL;
  static inline bool WriteDouble  (field_number, double value, output) INL;
  static inline bool WriteBool    (field_number,   bool value, output) INL;
  static inline bool WriteEnum    (field_number,    int value, output) INL;

  static inline bool WriteString(field_number, const string& value, output) INL;
  static inline bool WriteBytes (field_number, const string& value, output) INL;

  static inline bool WriteGroup(field_number, const Message& value, output) INL;
  static inline bool WriteMessage(
    field_number, const Message& value, output) INL;

  // Like above, but de-virtualize the call to SerializeWithCachedSizes().  The
  // pointer must point at an instance of MessageType, *not* a subclass (or
  // the subclass must not override SerializeWithCachedSizes()).
  template<typename MessageType>
  static inline bool WriteGroupNoVirtual(
    field_number, const MessageType& value, output) INL;
  template<typename MessageType>
  static inline bool WriteMessageNoVirtual(
    field_number, const MessageType& value, output) INL;

  // Compute the byte size of a tag.  For groups, this includes both the start
  // and end tags.
  static inline int TagSize(field_number, FieldDescriptor::Type type);

  // Compute the byte size of a field.  The XxSize() functions do NOT include
  // the tag, so you must also call TagSize().  (This is because, for repeated
  // fields, you should only call TagSize() once and multiply it by the element
  // count, but you may have to call XxSize() for each individual element.)
  static inline int Int32Size   ( int32 value);
  static inline int Int64Size   ( int64 value);
  static inline int UInt32Size  (uint32 value);
  static inline int UInt64Size  (uint64 value);
  static inline int SInt32Size  ( int32 value);
  static inline int SInt64Size  ( int64 value);
  static inline int EnumSize    (   int value);

  // These types always have the same size.
  static const int kFixed32Size  = 4;
  static const int kFixed64Size  = 8;
  static const int kSFixed32Size = 4;
  static const int kSFixed64Size = 8;
  static const int kFloatSize    = 4;
  static const int kDoubleSize   = 8;
  static const int kBoolSize     = 1;

  static inline int StringSize(const string& value);
  static inline int BytesSize (const string& value);

  static inline int GroupSize  (const Message& value);
  static inline int MessageSize(const Message& value);

  // Like above, but de-virtualize the call to ByteSize().  The
  // pointer must point at an instance of MessageType, *not* a subclass (or
  // the subclass must not override ByteSize()).
  template<typename MessageType>
  static inline int GroupSizeNoVirtual  (const MessageType& value);
  template<typename MessageType>
  static inline int MessageSizeNoVirtual(const MessageType& value);

#undef input
#undef output
#undef field_number
#undef INL

 private:
  static const WireType kWireTypeForFieldType[];

  // Parse/serialize a MessageSet::Item group.  Used with messages that use
  // opion message_set_wire_format = true.
  static bool ParseAndMergeMessageSetItem(
      io::CodedInputStream* input,
      Message::Reflection* message_reflection);
  static bool SerializeMessageSetItemWithCachedSizes(
      const FieldDescriptor* field,
      const Message::Reflection* message_reflection,
      io::CodedOutputStream* output);
  static int MessageSetItemByteSize(
      const FieldDescriptor* field,
      const Message::Reflection* message_reflection);

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(WireFormat);
};

// inline methods ====================================================

// This macro does the same thing as WireFormat::MakeTag(), but the
// result is usable as a compile-time constant, which makes it usable
// as a switch case or a template input.  WireFormat::MakeTag() is more
// type-safe, though, so prefer it if possible.
#define GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(FIELD_NUMBER, TYPE)             \
  static_cast<uint32>(                                              \
    ((FIELD_NUMBER) << ::google::protobuf::internal::WireFormat::kTagTypeBits) | (TYPE))

inline uint32 WireFormat::MakeTag(const FieldDescriptor* field) {
  return MakeTag(field->number(), WireTypeForFieldType(field->type()));
}

inline uint32 WireFormat::MakeTag(int field_number, WireType type) {
  return GOOGLE_PROTOBUF_WIRE_FORMAT_MAKE_TAG(field_number, type);
}

inline WireFormat::WireType WireFormat::GetTagWireType(uint32 tag) {
  return static_cast<WireType>(tag & kTagTypeMask);
}

inline int WireFormat::GetTagFieldNumber(uint32 tag) {
  return static_cast<int>(tag >> kTagTypeBits);
}

inline uint32 WireFormat::EncodeFloat(float value) {
  union {float f; uint32 i;};
  f = value;
  return i;
}

inline float WireFormat::DecodeFloat(uint32 value) {
  union {float f; uint32 i;};
  i = value;
  return f;
}

inline uint64 WireFormat::EncodeDouble(double value) {
  union {double f; uint64 i;};
  f = value;
  return i;
}

inline double WireFormat::DecodeDouble(uint64 value) {
  union {double f; uint64 i;};
  i = value;
  return f;
}

// ZigZag Transform:  Encodes signed integers so that they can be
// effectively used with varint encoding.
//
// varint operates on unsigned integers, encoding smaller numbers into
// fewer bytes.  If you try to use it on a signed integer, it will treat
// this number as a very large unsigned integer, which means that even
// small signed numbers like -1 will take the maximum number of bytes
// (10) to encode.  ZigZagEncode() maps signed integers to unsigned
// in such a way that those with a small absolute value will have smaller
// encoded values, making them appropriate for encoding using varint.
//
//       int32 ->     uint32
// -------------------------
//           0 ->          0
//          -1 ->          1
//           1 ->          2
//          -2 ->          3
//         ... ->        ...
//  2147483647 -> 4294967294
// -2147483648 -> 4294967295
//
//        >> encode >>
//        << decode <<

inline uint32 WireFormat::ZigZagEncode32(int32 n) {
  // Note:  the right-shift must be arithmetic
  return (n << 1) ^ (n >> 31);
}

inline int32 WireFormat::ZigZagDecode32(uint32 n) {
  return (n >> 1) ^ -static_cast<int32>(n & 1);
}

inline uint64 WireFormat::ZigZagEncode64(int64 n) {
  // Note:  the right-shift must be arithmetic
  return (n << 1) ^ (n >> 63);
}

inline int64 WireFormat::ZigZagDecode64(uint64 n) {
  return (n >> 1) ^ -static_cast<int64>(n & 1);
}

}  // namespace internal
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_WIRE_FORMAT_H__
