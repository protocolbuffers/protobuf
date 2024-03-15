// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/wire_format_lite.h"

#include <limits>
#include <stack>
#include <string>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "utf8_validity.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

#if !defined(_MSC_VER) || (_MSC_VER >= 1900 && _MSC_VER < 1912)
// Old version of MSVC doesn't like definitions of inline constants, GCC
// requires them.
const int WireFormatLite::kMessageSetItemStartTag;
const int WireFormatLite::kMessageSetItemEndTag;
const int WireFormatLite::kMessageSetTypeIdTag;
const int WireFormatLite::kMessageSetMessageTag;

#endif

constexpr size_t WireFormatLite::kFixed32Size;
constexpr size_t WireFormatLite::kFixed64Size;
constexpr size_t WireFormatLite::kSFixed32Size;
constexpr size_t WireFormatLite::kSFixed64Size;
constexpr size_t WireFormatLite::kFloatSize;
constexpr size_t WireFormatLite::kDoubleSize;
constexpr size_t WireFormatLite::kBoolSize;

// IBM xlC requires prefixing constants with WireFormatLite::
const size_t WireFormatLite::kMessageSetItemTagsSize =
    io::CodedOutputStream::StaticVarintSize32<
        WireFormatLite::kMessageSetItemStartTag>::value +
    io::CodedOutputStream::StaticVarintSize32<
        WireFormatLite::kMessageSetItemEndTag>::value +
    io::CodedOutputStream::StaticVarintSize32<
        WireFormatLite::kMessageSetTypeIdTag>::value +
    io::CodedOutputStream::StaticVarintSize32<
        WireFormatLite::kMessageSetMessageTag>::value;

const WireFormatLite::CppType
    WireFormatLite::kFieldTypeToCppTypeMap[MAX_FIELD_TYPE + 1] = {
        static_cast<CppType>(0),  // 0 is reserved for errors

        CPPTYPE_DOUBLE,   // TYPE_DOUBLE
        CPPTYPE_FLOAT,    // TYPE_FLOAT
        CPPTYPE_INT64,    // TYPE_INT64
        CPPTYPE_UINT64,   // TYPE_UINT64
        CPPTYPE_INT32,    // TYPE_INT32
        CPPTYPE_UINT64,   // TYPE_FIXED64
        CPPTYPE_UINT32,   // TYPE_FIXED32
        CPPTYPE_BOOL,     // TYPE_BOOL
        CPPTYPE_STRING,   // TYPE_STRING
        CPPTYPE_MESSAGE,  // TYPE_GROUP
        CPPTYPE_MESSAGE,  // TYPE_MESSAGE
        CPPTYPE_STRING,   // TYPE_BYTES
        CPPTYPE_UINT32,   // TYPE_UINT32
        CPPTYPE_ENUM,     // TYPE_ENUM
        CPPTYPE_INT32,    // TYPE_SFIXED32
        CPPTYPE_INT64,    // TYPE_SFIXED64
        CPPTYPE_INT32,    // TYPE_SINT32
        CPPTYPE_INT64,    // TYPE_SINT64
};

const WireFormatLite::WireType
    WireFormatLite::kWireTypeForFieldType[MAX_FIELD_TYPE + 1] = {
        static_cast<WireFormatLite::WireType>(-1),  // invalid
        WireFormatLite::WIRETYPE_FIXED64,           // TYPE_DOUBLE
        WireFormatLite::WIRETYPE_FIXED32,           // TYPE_FLOAT
        WireFormatLite::WIRETYPE_VARINT,            // TYPE_INT64
        WireFormatLite::WIRETYPE_VARINT,            // TYPE_UINT64
        WireFormatLite::WIRETYPE_VARINT,            // TYPE_INT32
        WireFormatLite::WIRETYPE_FIXED64,           // TYPE_FIXED64
        WireFormatLite::WIRETYPE_FIXED32,           // TYPE_FIXED32
        WireFormatLite::WIRETYPE_VARINT,            // TYPE_BOOL
        WireFormatLite::WIRETYPE_LENGTH_DELIMITED,  // TYPE_STRING
        WireFormatLite::WIRETYPE_START_GROUP,       // TYPE_GROUP
        WireFormatLite::WIRETYPE_LENGTH_DELIMITED,  // TYPE_MESSAGE
        WireFormatLite::WIRETYPE_LENGTH_DELIMITED,  // TYPE_BYTES
        WireFormatLite::WIRETYPE_VARINT,            // TYPE_UINT32
        WireFormatLite::WIRETYPE_VARINT,            // TYPE_ENUM
        WireFormatLite::WIRETYPE_FIXED32,           // TYPE_SFIXED32
        WireFormatLite::WIRETYPE_FIXED64,           // TYPE_SFIXED64
        WireFormatLite::WIRETYPE_VARINT,            // TYPE_SINT32
        WireFormatLite::WIRETYPE_VARINT,            // TYPE_SINT64
};

bool WireFormatLite::SkipField(io::CodedInputStream* input, uint32_t tag) {
  // Field number 0 is illegal.
  if (WireFormatLite::GetTagFieldNumber(tag) == 0) return false;
  switch (WireFormatLite::GetTagWireType(tag)) {
    case WireFormatLite::WIRETYPE_VARINT: {
      uint64_t value;
      if (!input->ReadVarint64(&value)) return false;
      return true;
    }
    case WireFormatLite::WIRETYPE_FIXED64: {
      uint64_t value;
      if (!input->ReadLittleEndian64(&value)) return false;
      return true;
    }
    case WireFormatLite::WIRETYPE_LENGTH_DELIMITED: {
      uint32_t length;
      if (!input->ReadVarint32(&length)) return false;
      if (!input->Skip(length)) return false;
      return true;
    }
    case WireFormatLite::WIRETYPE_START_GROUP: {
      if (!input->IncrementRecursionDepth()) return false;
      if (!SkipMessage(input)) return false;
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
      return true;
    }
    default: {
      return false;
    }
  }
}

bool WireFormatLite::SkipField(io::CodedInputStream* input, uint32_t tag,
                               io::CodedOutputStream* output) {
  // Field number 0 is illegal.
  if (WireFormatLite::GetTagFieldNumber(tag) == 0) return false;
  switch (WireFormatLite::GetTagWireType(tag)) {
    case WireFormatLite::WIRETYPE_VARINT: {
      uint64_t value;
      if (!input->ReadVarint64(&value)) return false;
      output->WriteVarint32(tag);
      output->WriteVarint64(value);
      return true;
    }
    case WireFormatLite::WIRETYPE_FIXED64: {
      uint64_t value;
      if (!input->ReadLittleEndian64(&value)) return false;
      output->WriteVarint32(tag);
      output->WriteLittleEndian64(value);
      return true;
    }
    case WireFormatLite::WIRETYPE_LENGTH_DELIMITED: {
      uint32_t length;
      if (!input->ReadVarint32(&length)) return false;
      output->WriteVarint32(tag);
      output->WriteVarint32(length);
      // TODO: Provide API to prevent extra string copying.
      std::string temp;
      if (!input->ReadString(&temp, length)) return false;
      output->WriteString(temp);
      return true;
    }
    case WireFormatLite::WIRETYPE_START_GROUP: {
      output->WriteVarint32(tag);
      if (!input->IncrementRecursionDepth()) return false;
      if (!SkipMessage(input, output)) return false;
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
      output->WriteVarint32(tag);
      output->WriteLittleEndian32(value);
      return true;
    }
    default: {
      return false;
    }
  }
}

bool WireFormatLite::SkipMessage(io::CodedInputStream* input) {
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

    if (!SkipField(input, tag)) return false;
  }
}

bool WireFormatLite::SkipMessage(io::CodedInputStream* input,
                                 io::CodedOutputStream* output) {
  while (true) {
    uint32_t tag = input->ReadTag();
    if (tag == 0) {
      // End of input.  This is a valid place to end, so return true.
      return true;
    }

    WireFormatLite::WireType wire_type = WireFormatLite::GetTagWireType(tag);

    if (wire_type == WireFormatLite::WIRETYPE_END_GROUP) {
      output->WriteVarint32(tag);
      // Must be the end of the message.
      return true;
    }

    if (!SkipField(input, tag, output)) return false;
  }
}

bool FieldSkipper::SkipField(io::CodedInputStream* input, uint32_t tag) {
  return WireFormatLite::SkipField(input, tag);
}

bool FieldSkipper::SkipMessage(io::CodedInputStream* input) {
  return WireFormatLite::SkipMessage(input);
}

void FieldSkipper::SkipUnknownEnum(int /* field_number */, int /* value */) {
  // Nothing.
}

bool CodedOutputStreamFieldSkipper::SkipField(io::CodedInputStream* input,
                                              uint32_t tag) {
  return WireFormatLite::SkipField(input, tag, unknown_fields_);
}

bool CodedOutputStreamFieldSkipper::SkipMessage(io::CodedInputStream* input) {
  return WireFormatLite::SkipMessage(input, unknown_fields_);
}

void CodedOutputStreamFieldSkipper::SkipUnknownEnum(int field_number,
                                                    int value) {
  unknown_fields_->WriteVarint32(field_number);
  unknown_fields_->WriteVarint64(value);
}

bool WireFormatLite::ReadPackedEnumPreserveUnknowns(
    io::CodedInputStream* input, int field_number, bool (*is_valid)(int),
    io::CodedOutputStream* unknown_fields_stream, RepeatedField<int>* values) {
  uint32_t length;
  if (!input->ReadVarint32(&length)) return false;
  io::CodedInputStream::Limit limit = input->PushLimit(length);
  while (input->BytesUntilLimit() > 0) {
    int value;
    if (!ReadPrimitive<int, WireFormatLite::TYPE_ENUM>(input, &value)) {
      return false;
    }
    if (is_valid == nullptr || is_valid(value)) {
      values->Add(value);
    } else {
      uint32_t tag = WireFormatLite::MakeTag(field_number,
                                             WireFormatLite::WIRETYPE_VARINT);
      unknown_fields_stream->WriteVarint32(tag);
      unknown_fields_stream->WriteVarint32(value);
    }
  }
  input->PopLimit(limit);
  return true;
}

#if !defined(ABSL_IS_LITTLE_ENDIAN)

namespace {
void EncodeFixedSizeValue(float v, uint8_t* dest) {
  WireFormatLite::WriteFloatNoTagToArray(v, dest);
}

void EncodeFixedSizeValue(double v, uint8_t* dest) {
  WireFormatLite::WriteDoubleNoTagToArray(v, dest);
}

void EncodeFixedSizeValue(uint32_t v, uint8_t* dest) {
  WireFormatLite::WriteFixed32NoTagToArray(v, dest);
}

void EncodeFixedSizeValue(uint64_t v, uint8_t* dest) {
  WireFormatLite::WriteFixed64NoTagToArray(v, dest);
}

void EncodeFixedSizeValue(int32_t v, uint8_t* dest) {
  WireFormatLite::WriteSFixed32NoTagToArray(v, dest);
}

void EncodeFixedSizeValue(int64_t v, uint8_t* dest) {
  WireFormatLite::WriteSFixed64NoTagToArray(v, dest);
}

void EncodeFixedSizeValue(bool v, uint8_t* dest) {
  WireFormatLite::WriteBoolNoTagToArray(v, dest);
}
}  // anonymous namespace

#endif  // !defined(ABSL_IS_LITTLE_ENDIAN)

template <typename CType>
static void WriteArray(const CType* a, int n, io::CodedOutputStream* output) {
#if defined(ABSL_IS_LITTLE_ENDIAN)
  output->WriteRaw(reinterpret_cast<const char*>(a), n * sizeof(a[0]));
#else
  const int kAtATime = 128;
  uint8_t buf[sizeof(CType) * kAtATime];
  for (int i = 0; i < n; i += kAtATime) {
    int to_do = std::min(kAtATime, n - i);
    uint8_t* ptr = buf;
    for (int j = 0; j < to_do; j++) {
      EncodeFixedSizeValue(a[i + j], ptr);
      ptr += sizeof(a[0]);
    }
    output->WriteRaw(buf, to_do * sizeof(a[0]));
  }
#endif
}

void WireFormatLite::WriteFloatArray(const float* a, int n,
                                     io::CodedOutputStream* output) {
  WriteArray<float>(a, n, output);
}

void WireFormatLite::WriteDoubleArray(const double* a, int n,
                                      io::CodedOutputStream* output) {
  WriteArray<double>(a, n, output);
}

void WireFormatLite::WriteFixed32Array(const uint32_t* a, int n,
                                       io::CodedOutputStream* output) {
  WriteArray<uint32_t>(a, n, output);
}

void WireFormatLite::WriteFixed64Array(const uint64_t* a, int n,
                                       io::CodedOutputStream* output) {
  WriteArray<uint64_t>(a, n, output);
}

void WireFormatLite::WriteSFixed32Array(const int32_t* a, int n,
                                        io::CodedOutputStream* output) {
  WriteArray<int32_t>(a, n, output);
}

void WireFormatLite::WriteSFixed64Array(const int64_t* a, int n,
                                        io::CodedOutputStream* output) {
  WriteArray<int64_t>(a, n, output);
}

void WireFormatLite::WriteBoolArray(const bool* a, int n,
                                    io::CodedOutputStream* output) {
  WriteArray<bool>(a, n, output);
}

void WireFormatLite::WriteInt32(int field_number, int32_t value,
                                io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_VARINT, output);
  WriteInt32NoTag(value, output);
}
void WireFormatLite::WriteInt64(int field_number, int64_t value,
                                io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_VARINT, output);
  WriteInt64NoTag(value, output);
}
void WireFormatLite::WriteUInt32(int field_number, uint32_t value,
                                 io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_VARINT, output);
  WriteUInt32NoTag(value, output);
}
void WireFormatLite::WriteUInt64(int field_number, uint64_t value,
                                 io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_VARINT, output);
  WriteUInt64NoTag(value, output);
}
void WireFormatLite::WriteSInt32(int field_number, int32_t value,
                                 io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_VARINT, output);
  WriteSInt32NoTag(value, output);
}
void WireFormatLite::WriteSInt64(int field_number, int64_t value,
                                 io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_VARINT, output);
  WriteSInt64NoTag(value, output);
}
void WireFormatLite::WriteFixed32(int field_number, uint32_t value,
                                  io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_FIXED32, output);
  WriteFixed32NoTag(value, output);
}
void WireFormatLite::WriteFixed64(int field_number, uint64_t value,
                                  io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_FIXED64, output);
  WriteFixed64NoTag(value, output);
}
void WireFormatLite::WriteSFixed32(int field_number, int32_t value,
                                   io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_FIXED32, output);
  WriteSFixed32NoTag(value, output);
}
void WireFormatLite::WriteSFixed64(int field_number, int64_t value,
                                   io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_FIXED64, output);
  WriteSFixed64NoTag(value, output);
}
void WireFormatLite::WriteFloat(int field_number, float value,
                                io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_FIXED32, output);
  WriteFloatNoTag(value, output);
}
void WireFormatLite::WriteDouble(int field_number, double value,
                                 io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_FIXED64, output);
  WriteDoubleNoTag(value, output);
}
void WireFormatLite::WriteBool(int field_number, bool value,
                               io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_VARINT, output);
  WriteBoolNoTag(value, output);
}
void WireFormatLite::WriteEnum(int field_number, int value,
                               io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_VARINT, output);
  WriteEnumNoTag(value, output);
}

constexpr size_t kInt32MaxSize = std::numeric_limits<int32_t>::max();

void WireFormatLite::WriteString(int field_number, const std::string& value,
                                 io::CodedOutputStream* output) {
  // String is for UTF-8 text only
  WriteTag(field_number, WIRETYPE_LENGTH_DELIMITED, output);
  ABSL_CHECK_LE(value.size(), kInt32MaxSize);
  output->WriteVarint32(value.size());
  output->WriteString(value);
}
void WireFormatLite::WriteStringMaybeAliased(int field_number,
                                             const std::string& value,
                                             io::CodedOutputStream* output) {
  // String is for UTF-8 text only
  WriteTag(field_number, WIRETYPE_LENGTH_DELIMITED, output);
  ABSL_CHECK_LE(value.size(), kInt32MaxSize);
  output->WriteVarint32(value.size());
  output->WriteRawMaybeAliased(value.data(), value.size());
}
void WireFormatLite::WriteBytes(int field_number, const std::string& value,
                                io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_LENGTH_DELIMITED, output);
  ABSL_CHECK_LE(value.size(), kInt32MaxSize);
  output->WriteVarint32(value.size());
  output->WriteString(value);
}
void WireFormatLite::WriteBytesMaybeAliased(int field_number,
                                            const std::string& value,
                                            io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_LENGTH_DELIMITED, output);
  ABSL_CHECK_LE(value.size(), kInt32MaxSize);
  output->WriteVarint32(value.size());
  output->WriteRawMaybeAliased(value.data(), value.size());
}


void WireFormatLite::WriteGroup(int field_number, const MessageLite& value,
                                io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_START_GROUP, output);
  value.SerializeWithCachedSizes(output);
  WriteTag(field_number, WIRETYPE_END_GROUP, output);
}

void WireFormatLite::WriteMessage(int field_number, const MessageLite& value,
                                  io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_LENGTH_DELIMITED, output);
  const int size = value.GetCachedSize();
  output->WriteVarint32(size);
  value.SerializeWithCachedSizes(output);
}

uint8_t* WireFormatLite::InternalWriteGroup(int field_number,
                                            const MessageLite& value,
                                            uint8_t* target,
                                            io::EpsCopyOutputStream* stream) {
  target = stream->EnsureSpace(target);
  target = WriteTagToArray(field_number, WIRETYPE_START_GROUP, target);
  target = value._InternalSerialize(target, stream);
  target = stream->EnsureSpace(target);
  return WriteTagToArray(field_number, WIRETYPE_END_GROUP, target);
}

uint8_t* WireFormatLite::InternalWriteMessage(int field_number,
                                              const MessageLite& value,
                                              int cached_size, uint8_t* target,
                                              io::EpsCopyOutputStream* stream) {
  target = stream->EnsureSpace(target);
  target = WriteTagToArray(field_number, WIRETYPE_LENGTH_DELIMITED, target);
  target = io::CodedOutputStream::WriteVarint32ToArray(
      static_cast<uint32_t>(cached_size), target);
  return value._InternalSerialize(target, stream);
}

void WireFormatLite::WriteSubMessageMaybeToArray(
    int /*size*/, const MessageLite& value, io::CodedOutputStream* output) {
  output->SetCur(value._InternalSerialize(output->Cur(), output->EpsCopy()));
}

void WireFormatLite::WriteGroupMaybeToArray(int field_number,
                                            const MessageLite& value,
                                            io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_START_GROUP, output);
  const int size = value.GetCachedSize();
  WriteSubMessageMaybeToArray(size, value, output);
  WriteTag(field_number, WIRETYPE_END_GROUP, output);
}

void WireFormatLite::WriteMessageMaybeToArray(int field_number,
                                              const MessageLite& value,
                                              io::CodedOutputStream* output) {
  WriteTag(field_number, WIRETYPE_LENGTH_DELIMITED, output);
  const int size = value.GetCachedSize();
  output->WriteVarint32(size);
  WriteSubMessageMaybeToArray(size, value, output);
}

PROTOBUF_NDEBUG_INLINE static bool ReadBytesToString(
    io::CodedInputStream* input, std::string* value);
inline static bool ReadBytesToString(io::CodedInputStream* input,
                                     std::string* value) {
  uint32_t length;
  return input->ReadVarint32(&length) && input->ReadString(value, length);
}

bool WireFormatLite::ReadBytes(io::CodedInputStream* input,
                               std::string* value) {
  return ReadBytesToString(input, value);
}

bool WireFormatLite::ReadBytes(io::CodedInputStream* input, std::string** p) {
  if (*p == &GetEmptyStringAlreadyInited()) {
    *p = new std::string();
  }
  return ReadBytesToString(input, *p);
}

void PrintUTF8ErrorLog(absl::string_view message_name,
                       absl::string_view field_name, const char* operation_str,
                       bool emit_stacktrace) {
  std::string stacktrace;
  (void)emit_stacktrace;  // Parameter is used by Google-internal code.
  std::string quoted_field_name = "";
  if (!field_name.empty()) {
    if (!message_name.empty()) {
      quoted_field_name =
          absl::StrCat(" '", message_name, ".", field_name, "'");
    } else {
      quoted_field_name = absl::StrCat(" '", field_name, "'");
    }
  }
  std::string error_message =
      absl::StrCat("String field", quoted_field_name,
                   " contains invalid UTF-8 data "
                   "when ",
                   operation_str,
                   " a protocol buffer. Use the 'bytes' type if you intend to "
                   "send raw bytes. ",
                   stacktrace);
  ABSL_LOG(ERROR) << error_message;
}

bool WireFormatLite::VerifyUtf8String(const char* data, int size, Operation op,
                                      const char* field_name) {
  if (!utf8_range::IsStructurallyValid({data, static_cast<size_t>(size)})) {
    const char* operation_str = nullptr;
    switch (op) {
      case PARSE:
        operation_str = "parsing";
        break;
      case SERIALIZE:
        operation_str = "serializing";
        break;
        // no default case: have the compiler warn if a case is not covered.
    }
    PrintUTF8ErrorLog("", field_name, operation_str, false);
    return false;
  }
  return true;
}

// this code is deliberately written such that clang makes it into really
// efficient SSE code.
template <bool ZigZag, bool SignExtended, typename T>
static size_t VarintSize(const T* data, const int n) {
  static_assert(sizeof(T) == 4, "This routine only works for 32 bit integers");
  // is_unsigned<T> => !ZigZag
  static_assert(
      (std::is_unsigned<T>::value ^ ZigZag) || std::is_signed<T>::value,
      "Cannot ZigZag encode unsigned types");
  // is_unsigned<T> => !SignExtended
  static_assert(
      (std::is_unsigned<T>::value ^ SignExtended) || std::is_signed<T>::value,
      "Cannot SignExtended unsigned types");
  static_assert(!(SignExtended && ZigZag),
                "Cannot SignExtended and ZigZag on the same type");
  // This approach is only faster when vectorized, and the vectorized
  // implementation only works in units of the platform's vector width, and is
  // only faster once a certain number of iterations are used. Normally the
  // compiler generates two loops - one partially unrolled vectorized loop that
  // processes big chunks, and a second "epilogue" scalar loop to finish up the
  // remainder. This is done manually here so that the faster scalar
  // implementation is used for small inputs and for the epilogue.
  int vectorN = n & -32;
  uint32_t sum = vectorN;
  uint32_t msb_sum = 0;
  int i = 0;
  for (; i < vectorN; i++) {
    uint32_t x = data[i];
    if (ZigZag) {
      x = WireFormatLite::ZigZagEncode32(x);
    } else if (SignExtended) {
      msb_sum += x >> 31;
    }
    // clang is so smart that it produces optimal SIMD sequence unrolling
    // the loop 8 ints at a time. With a sequence of 4
    // cmpres = cmpgt x, sizeclass  ( -1 or 0)
    // sum = sum - cmpres
    if (x > 0x7F) sum++;
    if (x > 0x3FFF) sum++;
    if (x > 0x1FFFFF) sum++;
    if (x > 0xFFFFFFF) sum++;
  }
// Clang is not smart enough to see that this loop doesn't run many times
// NOLINTNEXTLINE(google3-runtime-pragma-loop-hint): b/315043579
#pragma clang loop vectorize(disable) unroll(disable) interleave(disable)
  for (; i < n; i++) {
    uint32_t x = data[i];
    if (ZigZag) {
      sum += WireFormatLite::SInt32Size(x);
    } else if (SignExtended) {
      sum += WireFormatLite::Int32Size(x);
    } else {
      sum += WireFormatLite::UInt32Size(x);
    }
  }
  if (SignExtended) sum += msb_sum * 5;
  return sum;
}

template <bool ZigZag, typename T>
static size_t VarintSize64(const T* data, const int n) {
  static_assert(sizeof(T) == 8, "This routine only works for 64 bit integers");
  // is_unsigned<T> => !ZigZag
  static_assert(!ZigZag || !std::is_unsigned<T>::value,
                "Cannot ZigZag encode unsigned types");
  int vectorN = n & -32;
  uint64_t sum = vectorN;
  int i = 0;
  for (; i < vectorN; i++) {
    uint64_t x = data[i];
    if (ZigZag) {
      x = WireFormatLite::ZigZagEncode64(x);
    }
    // First step is a binary search, we can't branch in sse so we use the
    // result of the compare to adjust sum and appropriately. This code is
    // written to make clang recognize the vectorization.
    uint64_t tmp = x >= (static_cast<uint64_t>(1) << 35) ? -1 : 0;
    sum += 5 & tmp;
    x >>= 35 & tmp;
    if (x > 0x7F) sum++;
    if (x > 0x3FFF) sum++;
    if (x > 0x1FFFFF) sum++;
    if (x > 0xFFFFFFF) sum++;
  }
// Clang is not smart enough to see that this loop doesn't run many times
// NOLINTNEXTLINE(google3-runtime-pragma-loop-hint): b/315043579
#pragma clang loop vectorize(disable) unroll(disable) interleave(disable)
  for (; i < n; i++) {
    uint64_t x = data[i];
    if (ZigZag) {
      sum += WireFormatLite::SInt64Size(x);
    } else {
      sum += WireFormatLite::UInt64Size(x);
    }
  }
  return sum;
}

// On machines without a vector count-leading-zeros instruction such as SVE CLZ
// on arm or VPLZCNT on x86, SSE or AVX2 instructions can allow vectorization of
// the size calculation loop. GCC does not detect this autovectorization
// opportunity, so only enable for clang.
// When last tested, AVX512-vectorized lzcnt was slower than the SSE/AVX2
// implementation, so __AVX512CD__ is not checked.
#if defined(__SSE__) && defined(__clang__)
size_t WireFormatLite::Int32Size(const RepeatedField<int32_t>& value) {
  return VarintSize<false, true>(value.data(), value.size());
}

size_t WireFormatLite::UInt32Size(const RepeatedField<uint32_t>& value) {
  return VarintSize<false, false>(value.data(), value.size());
}

size_t WireFormatLite::SInt32Size(const RepeatedField<int32_t>& value) {
  return VarintSize<true, false>(value.data(), value.size());
}

size_t WireFormatLite::EnumSize(const RepeatedField<int>& value) {
  // On ILP64, sizeof(int) == 8, which would require a different template.
  return VarintSize<false, true>(value.data(), value.size());
}

#else  // !(defined(__SSE__) && defined(__clang__))

size_t WireFormatLite::Int32Size(const RepeatedField<int32_t>& value) {
  size_t out = 0;
  const int n = value.size();
  for (int i = 0; i < n; i++) {
    out += Int32Size(value.Get(i));
  }
  return out;
}

size_t WireFormatLite::UInt32Size(const RepeatedField<uint32_t>& value) {
  size_t out = 0;
  const int n = value.size();
  for (int i = 0; i < n; i++) {
    out += UInt32Size(value.Get(i));
  }
  return out;
}

size_t WireFormatLite::SInt32Size(const RepeatedField<int32_t>& value) {
  size_t out = 0;
  const int n = value.size();
  for (int i = 0; i < n; i++) {
    out += SInt32Size(value.Get(i));
  }
  return out;
}

size_t WireFormatLite::EnumSize(const RepeatedField<int>& value) {
  size_t out = 0;
  const int n = value.size();
  for (int i = 0; i < n; i++) {
    out += EnumSize(value.Get(i));
  }
  return out;
}

#endif

// Micro benchmarks show that the vectorizable loop only starts beating
// the normal loop when 256-bit vector registers are available.
#if defined(__AVX2__) && defined(__clang__)
size_t WireFormatLite::Int64Size(const RepeatedField<int64_t>& value) {
  return VarintSize64<false>(value.data(), value.size());
}

size_t WireFormatLite::UInt64Size(const RepeatedField<uint64_t>& value) {
  return VarintSize64<false>(value.data(), value.size());
}

size_t WireFormatLite::SInt64Size(const RepeatedField<int64_t>& value) {
  return VarintSize64<true>(value.data(), value.size());
}

#else

size_t WireFormatLite::Int64Size(const RepeatedField<int64_t>& value) {
  size_t out = 0;
  const int n = value.size();
  for (int i = 0; i < n; i++) {
    out += Int64Size(value.Get(i));
  }
  return out;
}

size_t WireFormatLite::UInt64Size(const RepeatedField<uint64_t>& value) {
  size_t out = 0;
  const int n = value.size();
  for (int i = 0; i < n; i++) {
    out += UInt64Size(value.Get(i));
  }
  return out;
}

size_t WireFormatLite::SInt64Size(const RepeatedField<int64_t>& value) {
  size_t out = 0;
  const int n = value.size();
  for (int i = 0; i < n; i++) {
    out += SInt64Size(value.Get(i));
  }
  return out;
}

#endif

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
