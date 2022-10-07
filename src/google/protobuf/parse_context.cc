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

#include "google/protobuf/parse_context.h"

#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/arenastring.h"
#include "google/protobuf/endian.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/repeated_field.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/wire_format_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

namespace {

// Only call if at start of tag.
bool ParseEndsInSlopRegion(const char* begin, int overrun, int depth) {
  constexpr int kSlopBytes = EpsCopyInputStream::kSlopBytes;
  GOOGLE_DCHECK_GE(overrun, 0);
  GOOGLE_DCHECK_LE(overrun, kSlopBytes);
  auto ptr = begin + overrun;
  auto end = begin + kSlopBytes;
  while (ptr < end) {
    uint32_t tag;
    ptr = ReadTag(ptr, &tag);
    if (ptr == nullptr || ptr > end) return false;
    // ending on 0 tag is allowed and is the major reason for the necessity of
    // this function.
    if (tag == 0) return true;
    switch (tag & 7) {
      case 0: {  // Varint
        uint64_t val;
        ptr = VarintParse(ptr, &val);
        if (ptr == nullptr) return false;
        break;
      }
      case 1: {  // fixed64
        ptr += 8;
        break;
      }
      case 2: {  // len delim
        int32_t size = ReadSize(&ptr);
        if (ptr == nullptr || size > end - ptr) return false;
        ptr += size;
        break;
      }
      case 3: {  // start group
        depth++;
        break;
      }
      case 4: {                    // end group
        if (--depth < 0) return true;  // We exit early
        break;
      }
      case 5: {  // fixed32
        ptr += 4;
        break;
      }
      default:
        return false;  // Unknown wireformat
    }
  }
  return false;
}

}  // namespace

const char* EpsCopyInputStream::NextBuffer(int overrun, int depth) {
  if (next_chunk_ == nullptr) return nullptr;  // We've reached end of stream.
  if (next_chunk_ != buffer_) {
    GOOGLE_DCHECK(size_ > kSlopBytes);
    // The chunk is large enough to be used directly
    buffer_end_ = next_chunk_ + size_ - kSlopBytes;
    auto res = next_chunk_;
    next_chunk_ = buffer_;
    if (aliasing_ == kOnPatch) aliasing_ = kNoDelta;
    return res;
  }
  // Move the slop bytes of previous buffer to start of the patch buffer.
  // Note we must use memmove because the previous buffer could be part of
  // buffer_.
  std::memmove(buffer_, buffer_end_, kSlopBytes);
  if (overall_limit_ > 0 &&
      (depth < 0 || !ParseEndsInSlopRegion(buffer_, overrun, depth))) {
    const void* data;
    // ZeroCopyInputStream indicates Next may return 0 size buffers. Hence
    // we loop.
    while (StreamNext(&data)) {
      if (size_ > kSlopBytes) {
        // We got a large chunk
        std::memcpy(buffer_ + kSlopBytes, data, kSlopBytes);
        next_chunk_ = static_cast<const char*>(data);
        buffer_end_ = buffer_ + kSlopBytes;
        if (aliasing_ >= kNoDelta) aliasing_ = kOnPatch;
        return buffer_;
      } else if (size_ > 0) {
        std::memcpy(buffer_ + kSlopBytes, data, size_);
        next_chunk_ = buffer_;
        buffer_end_ = buffer_ + size_;
        if (aliasing_ >= kNoDelta) aliasing_ = kOnPatch;
        return buffer_;
      }
      GOOGLE_DCHECK(size_ == 0) << size_;
    }
    overall_limit_ = 0;  // Next failed, no more needs for next
  }
  // End of stream or array
  if (aliasing_ == kNoDelta) {
    // If there is no more block and aliasing is true, the previous block
    // is still valid and we can alias. We have users relying on string_view's
    // obtained from protos to outlive the proto, when the parse was from an
    // array. This guarantees string_view's are always aliased if parsed from
    // an array.
    aliasing_ = reinterpret_cast<std::uintptr_t>(buffer_end_) -
                reinterpret_cast<std::uintptr_t>(buffer_);
  }
  next_chunk_ = nullptr;
  buffer_end_ = buffer_ + kSlopBytes;
  size_ = 0;
  return buffer_;
}

const char* EpsCopyInputStream::Next() {
  GOOGLE_DCHECK(limit_ > kSlopBytes);
  auto p = NextBuffer(0 /* immaterial */, -1);
  if (p == nullptr) {
    limit_end_ = buffer_end_;
    // Distinguish ending on a pushed limit or ending on end-of-stream.
    SetEndOfStream();
    return nullptr;
  }
  limit_ -= buffer_end_ - p;  // Adjust limit_ relative to new anchor
  limit_end_ = buffer_end_ + std::min(0, limit_);
  return p;
}

std::pair<const char*, bool> EpsCopyInputStream::DoneFallback(int overrun,
                                                              int depth) {
  // Did we exceeded the limit (parse error).
  if (PROTOBUF_PREDICT_FALSE(overrun > limit_)) return {nullptr, true};
  GOOGLE_DCHECK(overrun != limit_);  // Guaranteed by caller.
  GOOGLE_DCHECK(overrun < limit_);   // Follows from above
  // TODO(gerbens) Instead of this dcheck we could just assign, and remove
  // updating the limit_end from PopLimit, ie.
  // limit_end_ = buffer_end_ + (std::min)(0, limit_);
  // if (ptr < limit_end_) return {ptr, false};
  GOOGLE_DCHECK(limit_end_ == buffer_end_ + (std::min)(0, limit_));
  // At this point we know the following assertion holds.
  GOOGLE_DCHECK_GT(limit_, 0);
  GOOGLE_DCHECK(limit_end_ == buffer_end_);  // because limit_ > 0
  const char* p;
  do {
    // We are past the end of buffer_end_, in the slop region.
    GOOGLE_DCHECK_GE(overrun, 0);
    p = NextBuffer(overrun, depth);
    if (p == nullptr) {
      // We are at the end of the stream
      if (PROTOBUF_PREDICT_FALSE(overrun != 0)) return {nullptr, true};
      GOOGLE_DCHECK_GT(limit_, 0);
      limit_end_ = buffer_end_;
      // Distinguish ending on a pushed limit or ending on end-of-stream.
      SetEndOfStream();
      return {buffer_end_, true};
    }
    limit_ -= buffer_end_ - p;  // Adjust limit_ relative to new anchor
    p += overrun;
    overrun = p - buffer_end_;
  } while (overrun >= 0);
  limit_end_ = buffer_end_ + std::min(0, limit_);
  return {p, false};
}

const char* EpsCopyInputStream::SkipFallback(const char* ptr, int size) {
  return AppendSize(ptr, size, [](const char* /*p*/, int /*s*/) {});
}

const char* EpsCopyInputStream::ReadStringFallback(const char* ptr, int size,
                                                   std::string* str) {
  str->clear();
  if (PROTOBUF_PREDICT_TRUE(size <= buffer_end_ - ptr + limit_)) {
    // Reserve the string up to a static safe size. If strings are bigger than
    // this we proceed by growing the string as needed. This protects against
    // malicious payloads making protobuf hold on to a lot of memory.
    str->reserve(str->size() + std::min<int>(size, kSafeStringSize));
  }
  return AppendSize(ptr, size,
                    [str](const char* p, int s) { str->append(p, s); });
}

const char* EpsCopyInputStream::AppendStringFallback(const char* ptr, int size,
                                                     std::string* str) {
  if (PROTOBUF_PREDICT_TRUE(size <= buffer_end_ - ptr + limit_)) {
    // Reserve the string up to a static safe size. If strings are bigger than
    // this we proceed by growing the string as needed. This protects against
    // malicious payloads making protobuf hold on to a lot of memory.
    str->reserve(str->size() + std::min<int>(size, kSafeStringSize));
  }
  return AppendSize(ptr, size,
                    [str](const char* p, int s) { str->append(p, s); });
}


const char* EpsCopyInputStream::InitFrom(io::ZeroCopyInputStream* zcis) {
  zcis_ = zcis;
  const void* data;
  int size;
  limit_ = INT_MAX;
  if (zcis->Next(&data, &size)) {
    overall_limit_ -= size;
    if (size > kSlopBytes) {
      auto ptr = static_cast<const char*>(data);
      limit_ -= size - kSlopBytes;
      limit_end_ = buffer_end_ = ptr + size - kSlopBytes;
      next_chunk_ = buffer_;
      if (aliasing_ == kOnPatch) aliasing_ = kNoDelta;
      return ptr;
    } else {
      limit_end_ = buffer_end_ = buffer_ + kSlopBytes;
      next_chunk_ = buffer_;
      auto ptr = buffer_ + 2 * kSlopBytes - size;
      std::memcpy(ptr, data, size);
      return ptr;
    }
  }
  overall_limit_ = 0;
  next_chunk_ = nullptr;
  size_ = 0;
  limit_end_ = buffer_end_ = buffer_;
  return buffer_;
}

const char* ParseContext::ReadSizeAndPushLimitAndDepth(const char* ptr,
                                                       int* old_limit) {
  int size = ReadSize(&ptr);
  if (PROTOBUF_PREDICT_FALSE(!ptr)) {
    *old_limit = 0;  // Make sure this isn't uninitialized even on error return
    return nullptr;
  }
  *old_limit = PushLimit(ptr, size);
  if (--depth_ < 0) return nullptr;
  return ptr;
}

const char* ParseContext::ParseMessage(MessageLite* msg, const char* ptr) {
  int old;
  ptr = ReadSizeAndPushLimitAndDepth(ptr, &old);
  if (ptr == nullptr) return ptr;
  ptr = msg->_InternalParse(ptr, this);
  depth_++;
  if (!PopLimit(old)) return nullptr;
  return ptr;
}

inline void WriteVarint(uint64_t val, std::string* s) {
  while (val >= 128) {
    uint8_t c = val | 0x80;
    s->push_back(c);
    val >>= 7;
  }
  s->push_back(val);
}

void WriteVarint(uint32_t num, uint64_t val, std::string* s) {
  WriteVarint(num << 3, s);
  WriteVarint(val, s);
}

void WriteLengthDelimited(uint32_t num, absl::string_view val, std::string* s) {
  WriteVarint((num << 3) + 2, s);
  WriteVarint(val.size(), s);
  s->append(val.data(), val.size());
}

std::pair<const char*, uint32_t> VarintParseSlow32(const char* p,
                                                   uint32_t res) {
  for (std::uint32_t i = 1; i < 5; i++) {
    uint32_t byte = static_cast<uint8_t>(p[i]);
    res += (byte - 1) << (7 * i);
    if (PROTOBUF_PREDICT_TRUE(byte < 128)) {
      return {p + i + 1, res};
    }
  }
  // Accept >5 bytes
  for (std::uint32_t i = 5; i < 10; i++) {
    uint32_t byte = static_cast<uint8_t>(p[i]);
    if (PROTOBUF_PREDICT_TRUE(byte < 128)) {
      return {p + i + 1, res};
    }
  }
  return {nullptr, 0};
}

std::pair<const char*, uint64_t> VarintParseSlow64(const char* p,
                                                   uint32_t res32) {
  uint64_t res = res32;
  for (std::uint32_t i = 1; i < 10; i++) {
    uint64_t byte = static_cast<uint8_t>(p[i]);
    res += (byte - 1) << (7 * i);
    if (PROTOBUF_PREDICT_TRUE(byte < 128)) {
      return {p + i + 1, res};
    }
  }
  return {nullptr, 0};
}

std::pair<const char*, uint32_t> ReadTagFallback(const char* p, uint32_t res) {
  for (std::uint32_t i = 2; i < 5; i++) {
    uint32_t byte = static_cast<uint8_t>(p[i]);
    res += (byte - 1) << (7 * i);
    if (PROTOBUF_PREDICT_TRUE(byte < 128)) {
      return {p + i + 1, res};
    }
  }
  return {nullptr, 0};
}

std::pair<const char*, int32_t> ReadSizeFallback(const char* p, uint32_t res) {
  for (std::uint32_t i = 1; i < 4; i++) {
    uint32_t byte = static_cast<uint8_t>(p[i]);
    res += (byte - 1) << (7 * i);
    if (PROTOBUF_PREDICT_TRUE(byte < 128)) {
      return {p + i + 1, res};
    }
  }
  std::uint32_t byte = static_cast<uint8_t>(p[4]);
  if (PROTOBUF_PREDICT_FALSE(byte >= 8)) return {nullptr, 0};  // size >= 2gb
  res += (byte - 1) << 28;
  // Protect against sign integer overflow in PushLimit. Limits are relative
  // to buffer ends and ptr could potential be kSlopBytes beyond a buffer end.
  // To protect against overflow we reject limits absurdly close to INT_MAX.
  if (PROTOBUF_PREDICT_FALSE(res > INT_MAX - ParseContext::kSlopBytes)) {
    return {nullptr, 0};
  }
  return {p + 5, res};
}

const char* StringParser(const char* begin, const char* end, void* object,
                         ParseContext*) {
  auto str = static_cast<std::string*>(object);
  str->append(begin, end - begin);
  return end;
}

// Defined in wire_format_lite.cc
void PrintUTF8ErrorLog(absl::string_view message_name,
                       absl::string_view field_name, const char* operation_str,
                       bool emit_stacktrace);

bool VerifyUTF8(absl::string_view str, const char* field_name) {
  if (!::google::protobuf::internal::IsStructurallyValidUTF8(str)) {
    PrintUTF8ErrorLog("", field_name, "parsing", false);
    return false;
  }
  return true;
}

const char* InlineGreedyStringParser(std::string* s, const char* ptr,
                                     ParseContext* ctx) {
  int size = ReadSize(&ptr);
  if (!ptr) return nullptr;
  return ctx->ReadString(ptr, size, s);
}


template <typename T, bool sign>
const char* VarintParser(void* object, const char* ptr, ParseContext* ctx) {
  return ctx->ReadPackedVarint(ptr, [object](uint64_t varint) {
    T val;
    if (sign) {
      if (sizeof(T) == 8) {
        val = WireFormatLite::ZigZagDecode64(varint);
      } else {
        val = WireFormatLite::ZigZagDecode32(varint);
      }
    } else {
      val = varint;
    }
    static_cast<RepeatedField<T>*>(object)->Add(val);
  });
}

const char* PackedInt32Parser(void* object, const char* ptr,
                              ParseContext* ctx) {
  return VarintParser<int32_t, false>(object, ptr, ctx);
}
const char* PackedUInt32Parser(void* object, const char* ptr,
                               ParseContext* ctx) {
  return VarintParser<uint32_t, false>(object, ptr, ctx);
}
const char* PackedInt64Parser(void* object, const char* ptr,
                              ParseContext* ctx) {
  return VarintParser<int64_t, false>(object, ptr, ctx);
}
const char* PackedUInt64Parser(void* object, const char* ptr,
                               ParseContext* ctx) {
  return VarintParser<uint64_t, false>(object, ptr, ctx);
}
const char* PackedSInt32Parser(void* object, const char* ptr,
                               ParseContext* ctx) {
  return VarintParser<int32_t, true>(object, ptr, ctx);
}
const char* PackedSInt64Parser(void* object, const char* ptr,
                               ParseContext* ctx) {
  return VarintParser<int64_t, true>(object, ptr, ctx);
}

const char* PackedEnumParser(void* object, const char* ptr, ParseContext* ctx) {
  return VarintParser<int, false>(object, ptr, ctx);
}

const char* PackedBoolParser(void* object, const char* ptr, ParseContext* ctx) {
  return VarintParser<bool, false>(object, ptr, ctx);
}

template <typename T>
const char* FixedParser(void* object, const char* ptr, ParseContext* ctx) {
  int size = ReadSize(&ptr);
  return ctx->ReadPackedFixed(ptr, size,
                              static_cast<RepeatedField<T>*>(object));
}

const char* PackedFixed32Parser(void* object, const char* ptr,
                                ParseContext* ctx) {
  return FixedParser<uint32_t>(object, ptr, ctx);
}
const char* PackedSFixed32Parser(void* object, const char* ptr,
                                 ParseContext* ctx) {
  return FixedParser<int32_t>(object, ptr, ctx);
}
const char* PackedFixed64Parser(void* object, const char* ptr,
                                ParseContext* ctx) {
  return FixedParser<uint64_t>(object, ptr, ctx);
}
const char* PackedSFixed64Parser(void* object, const char* ptr,
                                 ParseContext* ctx) {
  return FixedParser<int64_t>(object, ptr, ctx);
}
const char* PackedFloatParser(void* object, const char* ptr,
                              ParseContext* ctx) {
  return FixedParser<float>(object, ptr, ctx);
}
const char* PackedDoubleParser(void* object, const char* ptr,
                               ParseContext* ctx) {
  return FixedParser<double>(object, ptr, ctx);
}

class UnknownFieldLiteParserHelper {
 public:
  explicit UnknownFieldLiteParserHelper(std::string* unknown)
      : unknown_(unknown) {}

  void AddVarint(uint32_t num, uint64_t value) {
    if (unknown_ == nullptr) return;
    WriteVarint(num * 8, unknown_);
    WriteVarint(value, unknown_);
  }
  void AddFixed64(uint32_t num, uint64_t value) {
    if (unknown_ == nullptr) return;
    WriteVarint(num * 8 + 1, unknown_);
    char buffer[8];
    io::CodedOutputStream::WriteLittleEndian64ToArray(
        value, reinterpret_cast<uint8_t*>(buffer));
    unknown_->append(buffer, 8);
  }
  const char* ParseLengthDelimited(uint32_t num, const char* ptr,
                                   ParseContext* ctx) {
    int size = ReadSize(&ptr);
    GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
    if (unknown_ == nullptr) return ctx->Skip(ptr, size);
    WriteVarint(num * 8 + 2, unknown_);
    WriteVarint(size, unknown_);
    return ctx->AppendString(ptr, size, unknown_);
  }
  const char* ParseGroup(uint32_t num, const char* ptr, ParseContext* ctx) {
    if (unknown_) WriteVarint(num * 8 + 3, unknown_);
    ptr = ctx->ParseGroup(this, ptr, num * 8 + 3);
    GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
    if (unknown_) WriteVarint(num * 8 + 4, unknown_);
    return ptr;
  }
  void AddFixed32(uint32_t num, uint32_t value) {
    if (unknown_ == nullptr) return;
    WriteVarint(num * 8 + 5, unknown_);
    char buffer[4];
    io::CodedOutputStream::WriteLittleEndian32ToArray(
        value, reinterpret_cast<uint8_t*>(buffer));
    unknown_->append(buffer, 4);
  }

  const char* _InternalParse(const char* ptr, ParseContext* ctx) {
    return WireFormatParser(*this, ptr, ctx);
  }

 private:
  std::string* unknown_;
};

const char* UnknownGroupLiteParse(std::string* unknown, const char* ptr,
                                  ParseContext* ctx) {
  UnknownFieldLiteParserHelper field_parser(unknown);
  return WireFormatParser(field_parser, ptr, ctx);
}

const char* UnknownFieldParse(uint32_t tag, std::string* unknown,
                              const char* ptr, ParseContext* ctx) {
  UnknownFieldLiteParserHelper field_parser(unknown);
  return FieldParser(tag, field_parser, ptr, ctx);
}

#ifdef __aarch64__
// Generally, speaking, the ARM-optimized Varint decode algorithm is to extract
// and concatenate all potentially valid data bits, compute the actual length
// of the Varint, and mask off the data bits which are not actually part of the
// result.  More detail on the two main parts is shown below.
//
// 1) Extract and concatenate all potentially valid data bits.
//    Two ARM-specific features help significantly:
//    a) Efficient and non-destructive bit extraction (UBFX)
//    b) A single instruction can perform both an OR with a shifted
//       second operand in one cycle.  E.g., the following two lines do the same
//       thing
//       ```result = operand_1 | (operand2 << 7);```
//       ```ORR %[result], %[operand_1], %[operand_2], LSL #7```
//    The figure below shows the implementation for handling four chunks.
//
// Bits   32    31-24    23   22-16    15    14-8      7     6-0
//      +----+---------+----+---------+----+---------+----+---------+
//      |CB 3| Chunk 3 |CB 2| Chunk 2 |CB 1| Chunk 1 |CB 0| Chunk 0 |
//      +----+---------+----+---------+----+---------+----+---------+
//                |              |              |              |
//               UBFX           UBFX           UBFX           UBFX    -- cycle 1
//                |              |              |              |
//                V              V              V              V
//               Combined LSL #7 and ORR     Combined LSL #7 and ORR  -- cycle 2
//                                 |             |
//                                 V             V
//                            Combined LSL #14 and ORR                -- cycle 3
//                                       |
//                                       V
//                                Parsed bits 0-27
//
//
// 2) Calculate the index of the cleared continuation bit in order to determine
//    where the encoded Varint ends and the size of the decoded value.  The
//    easiest way to do this is mask off all data bits, leaving just the
//    continuation bits.  We actually need to do the masking on an inverted
//    copy of the data, which leaves a 1 in all continuation bits which were
//    originally clear.  The number of trailing zeroes in this value indicates
//    the size of the Varint.
//
//  AND  0x80    0x80    0x80    0x80    0x80    0x80    0x80    0x80
//
// Bits   63      55      47      39      31      23      15       7
//      +----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+
// ~    |CB 7|  |CB 6|  |CB 5|  |CB 4|  |CB 3|  |CB 2|  |CB 1|  |CB 0|  |
//      +----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+
//         |       |       |       |       |       |       |       |
//         V       V       V       V       V       V       V       V
// Bits   63      55      47      39      31      23      15       7
//      +----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+
//      |~CB 7|0|~CB 6|0|~CB 5|0|~CB 4|0|~CB 3|0|~CB 2|0|~CB 1|0|~CB 0|0|
//      +----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+
//                                      |
//                                     CTZ
//                                      V
//                     Index of first cleared continuation bit
//
//
// While this is implemented in C++ significant care has been taken to ensure
// the compiler emits the best instruction sequence.  In some cases we use the
// following two functions to manipulate the compiler's scheduling decisions.
//
// Controls compiler scheduling by telling it that the first value is modified
// by the second value the callsite.  This is useful if non-critical path
// instructions are too aggressively scheduled, resulting in a slowdown of the
// actual critical path due to opportunity costs.  An example usage is shown
// where a false dependence of num_bits on result is added to prevent checking
// for a very unlikely error until all critical path instructions have been
// fetched.
//
// ```
// num_bits = <multiple operations to calculate new num_bits value>
// result = <multiple operations to calculate result>
// num_bits = ValueBarrier(num_bits, result);
// if (num_bits == 63) {
//   GOOGLE_LOG(FATAL) << "Invalid num_bits value";
// }
// ```
template <typename V1Type, typename V2Type>
PROTOBUF_ALWAYS_INLINE inline V1Type ValueBarrier(V1Type value1,
                                                  V2Type value2) {
  asm("" : "+r"(value1) : "r"(value2));
  return value1;
}

// Falsely indicate that the specific value is modified at this location.  This
// prevents code which depends on this value from being scheduled earlier.
template <typename V1Type>
PROTOBUF_ALWAYS_INLINE inline V1Type ValueBarrier(V1Type value1) {
  asm("" : "+r"(value1));
  return value1;
}

PROTOBUF_ALWAYS_INLINE inline uint64_t ExtractAndMergeTwoChunks(
    uint64_t data, uint64_t first_byte) {
  GOOGLE_DCHECK(first_byte <= 6);
  uint64_t first = Ubfx7(data, first_byte * 8);
  uint64_t second = Ubfx7(data, (first_byte + 1) * 8);
  return ForceToRegister(first | (second << 7));
}

struct SlowPathEncodedInfo {
  const char* p;
  uint64_t last8;
  uint64_t valid_bits;
  uint64_t valid_chunk_bits;
  uint64_t masked_cont_bits;
};

// Performs multiple actions which are identical between 32 and 64 bit Varints
// in order to compute the length of the encoded Varint and compute the new
// of p.
PROTOBUF_ALWAYS_INLINE inline SlowPathEncodedInfo ComputeLengthAndUpdateP(
    const char* p) {
  SlowPathEncodedInfo result;
  // Load the last two bytes of the encoded Varint.
  std::memcpy(&result.last8, p + 2, sizeof(result.last8));
  uint64_t mask = ForceToRegister(0x8080808080808080);
  // Only set continuation bits remain
  result.masked_cont_bits = ForceToRegister(mask & (~result.last8));
  // The first cleared continuation bit is the most significant 1 in the
  // reversed value.  Result is undefined for an input of 0 and we handle that
  // case below.
  result.valid_bits = absl::countr_zero(result.masked_cont_bits);
  // Calculates the number of chunks in the encoded Varint.  This value is low
  // by three as neither the cleared continuation chunk nor the first two chunks
  // are counted.
  uint64_t set_continuation_bits = result.valid_bits >> 3;
  // Update p to point past the encoded Varint.
  result.p = p + set_continuation_bits + 3;
  // Calculate number of valid data bits in the decoded value so invalid bits
  // can be masked off.  Value is too low by 14 but we account for that when
  // calculating the mask.
  result.valid_chunk_bits = result.valid_bits - set_continuation_bits;
  return result;
}

constexpr uint64_t kResultMaskUnshifted = 0xffffffffffffc000ULL;
constexpr uint64_t kFirstResultBitChunk1 = 1 * 7;
constexpr uint64_t kFirstResultBitChunk2 = 2 * 7;
constexpr uint64_t kFirstResultBitChunk3 = 3 * 7;
constexpr uint64_t kFirstResultBitChunk4 = 4 * 7;
constexpr uint64_t kFirstResultBitChunk6 = 6 * 7;
constexpr uint64_t kFirstResultBitChunk8 = 8 * 7;
constexpr uint64_t kValidBitsForInvalidVarint = 0x60;

PROTOBUF_NOINLINE const char* VarintParseSlowArm64(const char* p, uint64_t* out,
                                                   uint64_t first8) {
  SlowPathEncodedInfo info = ComputeLengthAndUpdateP(p);
  // Extract data bits from the low six chunks.  This includes chunks zero and
  // one which we already know are valid.
  uint64_t merged_01 = ExtractAndMergeTwoChunks(first8, /*first_chunk=*/0);
  uint64_t merged_23 = ExtractAndMergeTwoChunks(first8, /*first_chunk=*/2);
  uint64_t merged_45 = ExtractAndMergeTwoChunks(first8, /*first_chunk=*/4);
  // Low 42 bits of decoded value.
  uint64_t result = merged_01 | merged_23 << kFirstResultBitChunk2 |
                    merged_45 << kFirstResultBitChunk4;
  // This immediate ends in 14 zeroes since valid_chunk_bits is too low by 14.
  uint64_t result_mask = kResultMaskUnshifted << info.valid_chunk_bits;
  // masked_cont_bits is 0 iff the Varint is invalid.  In that case
  info.valid_bits =
      info.masked_cont_bits ? info.valid_bits : kValidBitsForInvalidVarint;
  // Test for early exit if Varint does not exceed 6 chunks.  Branching on one
  // bit is faster on ARM than via a compare and branch.
  if (PROTOBUF_PREDICT_FALSE((info.valid_bits & 0x20) != 0)) {
    // Extract data bits from high four chunks.
    uint64_t merged_67 = ExtractAndMergeTwoChunks(first8, /*first_chunk=*/6);
    // Last two chunks come from last two bytes of info.last8.
    uint64_t merged_89 =
        ExtractAndMergeTwoChunks(info.last8, /*first_chunk=*/6);
    result |= merged_67 << kFirstResultBitChunk6;
    result |= merged_89 << kFirstResultBitChunk8;
    // Handle an invalid Varint with all 10 continuation bits set.
    info.masked_cont_bits = ValueBarrier(info.masked_cont_bits);
    if (PROTOBUF_PREDICT_FALSE(info.masked_cont_bits == 0)) {
      *out = 0;
      return nullptr;
    }
  }
  // Mask off invalid data bytes.
  result &= ~result_mask;
  *out = result;
  return info.p;
}

// See comments in VarintParseSlowArm64 for a description of the algorithm.
// Differences in the 32 bit version are noted below.
PROTOBUF_NOINLINE const char* VarintParseSlowArm32(const char* p, uint32_t* out,
                                                   uint64_t first8) {
  // This also skips the slop bytes.
  SlowPathEncodedInfo info = ComputeLengthAndUpdateP(p);
  // Extract data bits from chunks 1-4.  Chunk zero is merged in below.
  uint64_t merged_12 = ExtractAndMergeTwoChunks(first8, /*first_chunk=*/1);
  uint64_t merged_34 = ExtractAndMergeTwoChunks(first8, /*first_chunk=*/3);
  first8 = ValueBarrier(first8, p);
  uint64_t result = Ubfx7(first8, /*start=*/0);
  result = ForceToRegister(result | merged_12 << kFirstResultBitChunk1);
  result = ForceToRegister(result | merged_34 << kFirstResultBitChunk3);
  uint64_t result_mask = kResultMaskUnshifted << info.valid_chunk_bits;
  result &= ~result_mask;
  // It is extremely unlikely that a Varint is invalid so checking that
  // condition isn't on the critical path. Here we make sure that we don't do so
  // until result has been computed.
  info.masked_cont_bits = ValueBarrier(info.masked_cont_bits, result);
  if (PROTOBUF_PREDICT_FALSE(info.masked_cont_bits == 0)) {
    // Makes the compiler think out was modified here.  This ensures it won't
    // predicate this extremely predictable branch.
    out = ValueBarrier(out);
    *out = 0;
    return nullptr;
  }
  *out = result;
  return info.p;
}

#endif  // __aarch64__

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
