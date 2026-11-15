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

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <string>
#include <type_traits>

#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/repeated_field.h"
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
                                      const absl::string_view field_name) {
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

constexpr int kVarintSizeMaxChunk = 1 << 28;

#if defined(__SSE2__) && defined(__clang__) && \
    !(defined(__AVX512CD__) && defined(__AVX512VL__))
// On x86 without hardware 32-bit vector CLZ (vplzcntd), Clang auto-vectorizes
// the 4-threshold comparison loop into SSE2/AVX2 instructions much faster than
// byte-shuffle CLZ emulation. GCC does not vectorize this loop, so restrict it
// to Clang.
#define PROTOBUF_VARINT32_USE_SSE2_SIMD 1
#else
#define PROTOBUF_VARINT32_USE_SSE2_SIMD 0
#endif

#if PROTOBUF_VARINT32_USE_SSE2_SIMD
template <bool ZigZag, typename T>
static size_t VarintSize32Scalar(const T* data, const int n) {
  constexpr bool kSignExtended = std::is_signed_v<T> && !ZigZag;
  constexpr size_t kMaxBytes = kSignExtended ? 10 : 5;
  // Hoist kMaxBytes * n out of the unrolled=1 scalar loop so the loop-carried
  // dependency is a single add instead of sub + add.
  constexpr bool kHoistMaxBytes = PROTOBUF_HAS_X86_LZCNT != 0;
  auto elem_size = [](T v) {
    if constexpr (ZigZag) {
      return WireFormatLite::SInt32Size(v);
    } else if constexpr (kSignExtended) {
      return WireFormatLite::Int32Size(v);
    } else {
      return WireFormatLite::UInt32Size(static_cast<uint32_t>(v));
    }
  };
  size_t acc = 0;
  // Clang is not smart enough to see that this loop doesn't run many times.
  // NOLINTNEXTLINE(google3-runtime-pragma-loop-hint): b/315043579
#pragma clang loop vectorize(disable) unroll(disable) interleave(disable)
  for (int i = 0; i < n; i++) {
    size_t s = elem_size(data[i]);
    acc += kHoistMaxBytes ? (kMaxBytes - s) : s;
  }
  return kHoistMaxBytes ? static_cast<size_t>(n) * kMaxBytes - acc : acc;
}

// Deliberately written so Clang vectorizes 32-bit varint sizing in 32-bit SIMD
// lanes without widening accumulators to 64-bit. Requires
// n <= kVarintSizeMaxChunk so uint32_t lane accumulators cannot overflow.
template <bool ZigZag, typename T>
PROTOBUF_NOINLINE static size_t VarintSize32Chunk(const T* data, const int n) {
  constexpr bool kSignExtended = std::is_signed_v<T> && !ZigZag;
  // SSE2/AVX2 lack unsigned vpcmpgtud; biasing unsigned values into int32_t
  // emits 1 vpxor + 4 vpcmpgtd instead of 4 vpmaxud + 4 vpcmpeqd. For
  // SignExtended, negative values have cx < 0 (0 across all 4 comparisons)
  // and msb_sum adds the remaining 9 bytes (1 + 9 = 10).
  constexpr uint32_t kBias = kSignExtended ? 0 : 0x80000000u;
  int vector_n = n & -16;
  uint32_t sum = static_cast<uint32_t>(vector_n);
  uint32_t msb_sum = 0;
  int i = 0;
  // NOLINTNEXTLINE(google3-runtime-pragma-loop-hint): b/315043579
#pragma clang loop interleave_count(2) unroll(disable)
  for (; i < vector_n; i++) {
    uint32_t x = static_cast<uint32_t>(data[i]);
    if constexpr (ZigZag) {
      x = WireFormatLite::ZigZagEncode32(static_cast<int32_t>(x));
    } else if constexpr (kSignExtended) {
      msb_sum += x >> 31;
    }
    int32_t cx = static_cast<int32_t>(x ^ kBias);
    if (cx > static_cast<int32_t>(0x7Fu ^ kBias)) sum++;
    if (cx > static_cast<int32_t>(0x3FFFu ^ kBias)) sum++;
    if (cx > static_cast<int32_t>(0x1FFFFFu ^ kBias)) sum++;
    if (cx > static_cast<int32_t>(0xFFFFFFFu ^ kBias)) sum++;
  }
  size_t total = sum;
  if constexpr (kSignExtended) {
    total += static_cast<size_t>(msb_sum) * 9;
  }
  if (i < n) {
    total += VarintSize32Scalar<ZigZag>(data + i, n - i);
  }
  return total;
}
#else   // !PROTOBUF_VARINT32_USE_SSE2_SIMD
// Accumulate in 32-bit lanes (5 + 5 = 10 for negatives) so targets with
// hardware 32-bit vector CLZ (such as ARM NEON or AVX-512CD) vectorize from
// n >= 4 without 64-bit lane widening or .rodata constant loads. Requires
// n <= kVarintSizeMaxChunk so uint32_t lane accumulators cannot overflow.
template <bool ZigZag, typename T>
static size_t VarintSize32Chunk(const T* data, const int n) {
  constexpr bool kSignExtended = std::is_signed_v<T> && !ZigZag;
  uint32_t sum = 0;
  uint32_t msb_sum = 0;
  for (int i = 0; i < n; i++) {
    uint32_t u = static_cast<uint32_t>(data[i]);
    if constexpr (ZigZag) {
      u = WireFormatLite::ZigZagEncode32(static_cast<int32_t>(u));
    } else if constexpr (kSignExtended) {
      msb_sum += u >> 31;
    }
    sum += static_cast<uint32_t>(WireFormatLite::UInt32Size(u));
  }
  size_t total = sum;
  if constexpr (kSignExtended) {
    total += static_cast<size_t>(msb_sum) * 5;
  }
  return total;
}
#endif  // PROTOBUF_VARINT32_USE_SSE2_SIMD

template <bool ZigZag, typename T>
PROTOBUF_NOINLINE static size_t VarintSize32Large(const T* data, int n) {
  size_t total = 0;
#ifdef __clang__
  // NOLINTNEXTLINE(google3-runtime-pragma-loop-hint): b/315043579
#pragma clang loop unroll(disable)
#endif  // __clang__
  do {
    int chunk = n < kVarintSizeMaxChunk ? n : kVarintSizeMaxChunk;
    total += VarintSize32Chunk<ZigZag>(data, chunk);
    data += chunk;
    n -= chunk;
  } while (n > 0);
  return total;
}

template <bool ZigZag, typename T>
static PROTOBUF_ALWAYS_INLINE size_t VarintSize32Chunked(const T* data,
                                                         const int n) {
  return ABSL_PREDICT_FALSE(n > kVarintSizeMaxChunk)
             ? VarintSize32Large<ZigZag>(data, n)
             : VarintSize32Chunk<ZigZag>(data, n);
}

#if PROTOBUF_VARINT32_USE_SSE2_SIMD
// Out-of-line tail-call target so the hot n < 16 path in *WithPackedTagSize
// avoids saving tag_size and cached_size across a non-tail call.
template <bool ZigZag, typename T>
PROTOBUF_NOINLINE static size_t VarintSize32VectorizedWithPackedTagSize(
    const T* data, size_t tag_size, const internal::CachedSize& cached_size,
    const int n) {
  size_t res = VarintSize32Chunked<ZigZag>(data, n);
  cached_size.SetNonZero(ToCachedSize(res));
  return tag_size + WireFormatLite::LengthDelimitedSize(res);
}
#endif  // PROTOBUF_VARINT32_USE_SSE2_SIMD

template <bool ZigZag, typename T>
static size_t VarintSize64(const T* data, const int n) {
  size_t sum = 0;
#if defined(__clang__) && !(defined(__AVX512CD__) && defined(__AVX512VL__)) && \
    !defined(__ARM_FEATURE_SVE)
  // Without hardware 64-bit vector CLZ, Clang vectorizes SInt64 ZigZag + CLZ
  // using byte-shuffle lookup tables, which is much slower than scalar CLZ.
  // NOLINTNEXTLINE(google3-runtime-pragma-loop-hint): b/315043579
#pragma clang loop vectorize(disable)
#if PROTOBUF_HAS_X86_LZCNT
  // Unroll 4x so AFDO variance does not collapse unrolling and +10 folds.
  // NOLINTNEXTLINE(google3-runtime-pragma-loop-hint): b/315043579
#pragma clang loop unroll_count(4)
#endif  // PROTOBUF_HAS_X86_LZCNT
#endif  // __clang__ && !(__AVX512CD__ && __AVX512VL__) && !__ARM_FEATURE_SVE
  for (int i = 0; i < n; i++) {
    if constexpr (ZigZag) {
      sum += WireFormatLite::SInt64Size(data[i]);
    } else {
      sum += WireFormatLite::UInt64Size(static_cast<uint64_t>(data[i]));
    }
  }
  return sum;
}

template <bool ZigZag = false, typename T>
static size_t RepeatedVarintSize(const RepeatedField<T>& value) {
  static_assert(sizeof(T) == 4 || sizeof(T) == 8);
  static_assert(!ZigZag || std::is_signed_v<T>,
                "Cannot ZigZag encode unsigned types");
  const int n = value.size();
  if (n <= 0) return 0;
  const T* data = value.data();
  if constexpr (sizeof(T) == 4) {
#if PROTOBUF_VARINT32_USE_SSE2_SIMD
    // Most repeated fields have fewer than 16 elements; keep the short scalar
    // loop on the fallthrough path and tail-call the vectorized helper.
    if (ABSL_PREDICT_FALSE(n >= 16)) {
      return VarintSize32Chunked<ZigZag>(data, n);
    }
    return VarintSize32Scalar<ZigZag>(data, n);
#else   // !PROTOBUF_VARINT32_USE_SSE2_SIMD
    return VarintSize32Chunked<ZigZag>(data, n);
#endif  // PROTOBUF_VARINT32_USE_SSE2_SIMD
  } else {
    return VarintSize64<ZigZag>(data, n);
  }
}

template <bool ZigZag = false, typename T>
static size_t RepeatedVarintSizeWithPackedTagSize(
    const RepeatedField<T>& value, size_t tag_size,
    const internal::CachedSize& cached_size) {
  static_assert(sizeof(T) == 4 || sizeof(T) == 8);
  static_assert(!ZigZag || std::is_signed_v<T>,
                "Cannot ZigZag encode unsigned types");
  const int n = value.size();
  if (n <= 0) {
    if (ABSL_PREDICT_FALSE(cached_size.Get() != 0)) {
      cached_size.Set(0);
    }
    return 0;
  }
  const T* data = value.data();
  size_t res;
  if constexpr (sizeof(T) == 4) {
#if PROTOBUF_VARINT32_USE_SSE2_SIMD
    if (ABSL_PREDICT_FALSE(n >= 16)) {
      return VarintSize32VectorizedWithPackedTagSize<ZigZag>(data, tag_size,
                                                             cached_size, n);
    }
    res = VarintSize32Scalar<ZigZag>(data, n);
#else   // !PROTOBUF_VARINT32_USE_SSE2_SIMD
    res = VarintSize32Chunked<ZigZag>(data, n);
#endif  // PROTOBUF_VARINT32_USE_SSE2_SIMD
  } else {
    res = VarintSize64<ZigZag>(data, n);
  }
  cached_size.SetNonZero(ToCachedSize(res));
  return tag_size + WireFormatLite::LengthDelimitedSize(res);
}

#undef PROTOBUF_VARINT32_USE_SSE2_SIMD

size_t WireFormatLite::Int32Size(const RepeatedField<int32_t>& value) {
  return RepeatedVarintSize(value);
}
size_t WireFormatLite::Int64Size(const RepeatedField<int64_t>& value) {
  return RepeatedVarintSize(value);
}
size_t WireFormatLite::UInt32Size(const RepeatedField<uint32_t>& value) {
  return RepeatedVarintSize(value);
}
size_t WireFormatLite::UInt64Size(const RepeatedField<uint64_t>& value) {
  return RepeatedVarintSize(value);
}
size_t WireFormatLite::SInt32Size(const RepeatedField<int32_t>& value) {
  return RepeatedVarintSize</*ZigZag=*/true>(value);
}
size_t WireFormatLite::SInt64Size(const RepeatedField<int64_t>& value) {
  return RepeatedVarintSize</*ZigZag=*/true>(value);
}
size_t WireFormatLite::EnumSize(const RepeatedField<int>& value) {
  return RepeatedVarintSize(value);
}

size_t WireFormatLite::Int32SizeWithPackedTagSize(
    const RepeatedField<int32_t>& value, size_t tag_size,
    const internal::CachedSize& cached_size) {
  return RepeatedVarintSizeWithPackedTagSize(value, tag_size, cached_size);
}
size_t WireFormatLite::Int64SizeWithPackedTagSize(
    const RepeatedField<int64_t>& value, size_t tag_size,
    const internal::CachedSize& cached_size) {
  return RepeatedVarintSizeWithPackedTagSize(value, tag_size, cached_size);
}
size_t WireFormatLite::UInt32SizeWithPackedTagSize(
    const RepeatedField<uint32_t>& value, size_t tag_size,
    const internal::CachedSize& cached_size) {
  return RepeatedVarintSizeWithPackedTagSize(value, tag_size, cached_size);
}
size_t WireFormatLite::UInt64SizeWithPackedTagSize(
    const RepeatedField<uint64_t>& value, size_t tag_size,
    const internal::CachedSize& cached_size) {
  return RepeatedVarintSizeWithPackedTagSize(value, tag_size, cached_size);
}
size_t WireFormatLite::SInt32SizeWithPackedTagSize(
    const RepeatedField<int32_t>& value, size_t tag_size,
    const internal::CachedSize& cached_size) {
  return RepeatedVarintSizeWithPackedTagSize</*ZigZag=*/true>(value, tag_size,
                                                              cached_size);
}
size_t WireFormatLite::SInt64SizeWithPackedTagSize(
    const RepeatedField<int64_t>& value, size_t tag_size,
    const internal::CachedSize& cached_size) {
  return RepeatedVarintSizeWithPackedTagSize</*ZigZag=*/true>(value, tag_size,
                                                              cached_size);
}
size_t WireFormatLite::EnumSizeWithPackedTagSize(
    const RepeatedField<int>& value, size_t tag_size,
    const internal::CachedSize& cached_size) {
  return RepeatedVarintSizeWithPackedTagSize(value, tag_size, cached_size);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
