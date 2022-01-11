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

#include <cstdint>

#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_message_tctable_decl.h>
#include <google/protobuf/generated_message_tctable_impl.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/parse_context.h>
#include <google/protobuf/wire_format_lite.h>

// clang-format off
#include <google/protobuf/port_def.inc>
// clang-format on

namespace google {
namespace protobuf {
namespace internal {

using FieldEntry = TcParseTableBase::FieldEntry;

#ifndef NDEBUG
template void AlignFail<4>(uintptr_t);
template void AlignFail<8>(uintptr_t);
#endif

const uint32_t TcParser::kMtSmallScanSize;

const char* TcParser::GenericFallbackLite(PROTOBUF_TC_PARAM_DECL) {
  return GenericFallbackImpl<MessageLite, std::string>(PROTOBUF_TC_PARAM_PASS);
}

// Returns the address of the field for `tag` in the table's field entries.
// Returns nullptr if the field was not found.
const TcParseTableBase::FieldEntry* TcParser::FindFieldEntry(
    const TcParseTableBase* table, uint32_t field_num) {
  const FieldEntry* const field_entries = table->field_entries_begin();

  // Most messages have fields numbered sequentially. If the decoded tag is
  // within that range, we can look up the field by index.
  const uint32_t sequential_start = table->sequential_fields_start;
  uint32_t adjusted_field_num = field_num - sequential_start;
  const uint32_t num_sequential = table->num_sequential_fields;
  if (PROTOBUF_PREDICT_TRUE(adjusted_field_num < num_sequential)) {
    return field_entries + adjusted_field_num;
  }

  // Check if this field is larger than the max in the table. This is often an
  // extension.
  if (field_num > table->max_field_number) {
    return nullptr;
  }

  // Otherwise, scan the next few field numbers, skipping the first
  // `num_sequential` entries.
  const uint32_t* const field_num_begin = table->field_numbers_begin();
  const uint32_t small_scan_limit =
      std::min(num_sequential + kMtSmallScanSize,
               static_cast<uint32_t>(table->num_field_entries));
  for (uint32_t i = num_sequential; i < small_scan_limit; ++i) {
    if (field_num <= field_num_begin[i]) {
      if (PROTOBUF_PREDICT_FALSE(field_num != field_num_begin[i])) {
        // Field number mismatch.
        return nullptr;
      }
      return field_entries + i;
    }
  }

  // Finally, look up with binary search.
  const uint32_t* const field_num_end = table->field_numbers_end();
  auto it = std::lower_bound(field_num_begin + small_scan_limit, field_num_end,
                             field_num);
  if (it == field_num_end) {
    // The only reason for binary search failing is if there was nothing to
    // search.
    GOOGLE_DCHECK_EQ(field_num_begin + small_scan_limit, field_num_end) << field_num;
    return nullptr;
  }
  if (PROTOBUF_PREDICT_FALSE(*it != field_num)) {
    // Field number mismatch.
    return nullptr;
  }
  return field_entries + (it - field_num_begin);
}

const char* TcParser::MiniParse(PROTOBUF_TC_PARAM_DECL) {
  uint32_t tag;
  ptr = ReadTag(ptr, &tag);
  if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) return nullptr;

  auto* entry = FindFieldEntry(table, tag >> 3);
  if (entry == nullptr) {
    data.data = tag;
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }

  // The handler may need the tag and the entry to resolve fallback logic. Both
  // of these are 32 bits, so pack them into (the 64-bit) `data`. Since we can't
  // pack the entry pointer itself, just pack its offset from `table`.
  uint64_t entry_offset = reinterpret_cast<const char*>(entry) -
                          reinterpret_cast<const char*>(table);
  data.data = entry_offset << 32 | tag;

  using field_layout::FieldKind;
  auto field_type = entry->type_card & FieldKind::kFkMask;
  switch (field_type) {
    case FieldKind::kFkNone:
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    case FieldKind::kFkVarint:
      PROTOBUF_MUSTTAIL return MpVarint(PROTOBUF_TC_PARAM_PASS);
    case FieldKind::kFkPackedVarint:
      PROTOBUF_MUSTTAIL return MpPackedVarint(PROTOBUF_TC_PARAM_PASS);
    case FieldKind::kFkFixed:
      PROTOBUF_MUSTTAIL return MpFixed(PROTOBUF_TC_PARAM_PASS);
    case FieldKind::kFkPackedFixed:
      PROTOBUF_MUSTTAIL return MpPackedFixed(PROTOBUF_TC_PARAM_PASS);
    case FieldKind::kFkString:
      PROTOBUF_MUSTTAIL return MpString(PROTOBUF_TC_PARAM_PASS);
    case FieldKind::kFkMessage:
      PROTOBUF_MUSTTAIL return MpMessage(PROTOBUF_TC_PARAM_PASS);
    case FieldKind::kFkMap:
      PROTOBUF_MUSTTAIL return MpMap(PROTOBUF_TC_PARAM_PASS);
    default:
      return Error(PROTOBUF_TC_PARAM_PASS);
  }
}

namespace {

// Offset returns the address `offset` bytes after `base`.
inline void* Offset(void* base, uint32_t offset) {
  return static_cast<uint8_t*>(base) + offset;
}

// InvertPacked changes tag bits from the given wire type to length
// delimited. This is the difference expected between packed and non-packed
// repeated fields.
template <WireFormatLite::WireType Wt>
inline PROTOBUF_ALWAYS_INLINE void InvertPacked(TcFieldData& data) {
  data.data ^= Wt ^ WireFormatLite::WIRETYPE_LENGTH_DELIMITED;
}

}  // namespace

//////////////////////////////////////////////////////////////////////////////
// Message fields
//////////////////////////////////////////////////////////////////////////////

template <typename TagType>
inline PROTOBUF_ALWAYS_INLINE
const char* TcParser::SingularParseMessageAuxImpl(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
  }
  ptr += sizeof(TagType);
  hasbits |= (uint64_t{1} << data.hasbit_idx());
  auto& field = RefAt<MessageLite*>(msg, data.offset());
  if (field == nullptr) {
    const MessageLite* default_instance =
        table->field_aux(data.aux_idx())->message_default;
    field = default_instance->New(ctx->data().arena);
  }
  SyncHasbits(msg, hasbits, table);
  return ctx->ParseMessage(field, ptr);
}

const char* TcParser::SingularParseMessageAux1(PROTOBUF_TC_PARAM_DECL) {
  return SingularParseMessageAuxImpl<uint8_t>(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::SingularParseMessageAux2(PROTOBUF_TC_PARAM_DECL) {
  return SingularParseMessageAuxImpl<uint16_t>(PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType>
inline PROTOBUF_ALWAYS_INLINE
const char* TcParser::RepeatedParseMessageAuxImpl(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
  }
  ptr += sizeof(TagType);
  SyncHasbits(msg, hasbits, table);
  const MessageLite* default_instance =
      table->field_aux(data.aux_idx())->message_default;
  auto& field = RefAt<RepeatedPtrFieldBase>(msg, data.offset());
  MessageLite* submsg =
      field.Add<GenericTypeHandler<MessageLite>>(default_instance);
  return ctx->ParseMessage(submsg, ptr);
}

const char* TcParser::RepeatedParseMessageAux1(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedParseMessageAuxImpl<uint8_t>(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::RepeatedParseMessageAux2(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedParseMessageAuxImpl<uint16_t>(PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// Fixed fields
//////////////////////////////////////////////////////////////////////////////

template <typename LayoutType, typename TagType>
const char* TcParser::SingularFixed(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
  }
  ptr += sizeof(TagType);  // Consume tag
  hasbits |= (uint64_t{1} << data.hasbit_idx());
  std::memcpy(Offset(msg, data.offset()), ptr, sizeof(LayoutType));
  ptr += sizeof(LayoutType);
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastF32S1(PROTOBUF_TC_PARAM_DECL) {
  return SingularFixed<uint32_t, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF32S2(PROTOBUF_TC_PARAM_DECL) {
  return SingularFixed<uint32_t, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF64S1(PROTOBUF_TC_PARAM_DECL) {
  return SingularFixed<uint64_t, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF64S2(PROTOBUF_TC_PARAM_DECL) {
  return SingularFixed<uint64_t, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}

template <typename LayoutType, typename TagType>
const char* TcParser::RepeatedFixed(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    // Check if the field can be parsed as packed repeated:
    constexpr WireFormatLite::WireType fallback_wt =
        sizeof(LayoutType) == 4 ? WireFormatLite::WIRETYPE_FIXED32
                                : WireFormatLite::WIRETYPE_FIXED64;
    InvertPacked<fallback_wt>(data);
    if (data.coded_tag<TagType>() == 0) {
      return PackedFixed<LayoutType, TagType>(PROTOBUF_TC_PARAM_PASS);
    } else {
      PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
    }
  }
  auto& field = RefAt<RepeatedField<LayoutType>>(msg, data.offset());
  int idx = field.size();
  auto elem = field.Add();
  int space = field.Capacity() - idx;
  idx = 0;
  auto expected_tag = UnalignedLoad<TagType>(ptr);
  do {
    ptr += sizeof(TagType);
    std::memcpy(elem + (idx++), ptr, sizeof(LayoutType));
    ptr += sizeof(LayoutType);
    if (idx >= space) break;
    if (!ctx->DataAvailable(ptr)) break;
  } while (UnalignedLoad<TagType>(ptr) == expected_tag);
  field.AddNAlreadyReserved(idx - 1);
  return ToParseLoop(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastF32R1(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedFixed<uint32_t, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF32R2(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedFixed<uint32_t, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF64R1(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedFixed<uint64_t, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF64R2(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedFixed<uint64_t, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}

template <typename LayoutType, typename TagType>
const char* TcParser::PackedFixed(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    // Try parsing as non-packed repeated:
    constexpr WireFormatLite::WireType fallback_wt =
        sizeof(LayoutType) == 4 ? WireFormatLite::WIRETYPE_FIXED32
                                : WireFormatLite::WIRETYPE_FIXED64;
    InvertPacked<fallback_wt>(data);
    if (data.coded_tag<TagType>() == 0) {
      return RepeatedFixed<LayoutType, TagType>(PROTOBUF_TC_PARAM_PASS);
    } else {
      PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
    }
  }
  ptr += sizeof(TagType);
  // Since ctx->ReadPackedFixed does not use TailCall<> or Return<>, sync any
  // pending hasbits now:
  SyncHasbits(msg, hasbits, table);
  auto& field = RefAt<RepeatedField<LayoutType>>(msg, data.offset());
  int size = ReadSize(&ptr);
  // TODO(dlj): add a tailcalling variant of ReadPackedFixed.
  return ctx->ReadPackedFixed(ptr, size,
                              static_cast<RepeatedField<LayoutType>*>(&field));
}

const char* TcParser::FastF32P1(PROTOBUF_TC_PARAM_DECL) {
  return PackedFixed<uint32_t, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF32P2(PROTOBUF_TC_PARAM_DECL) {
  return PackedFixed<uint32_t, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF64P1(PROTOBUF_TC_PARAM_DECL) {
  return PackedFixed<uint64_t, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF64P2(PROTOBUF_TC_PARAM_DECL) {
  return PackedFixed<uint64_t, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// Varint fields
//////////////////////////////////////////////////////////////////////////////

namespace {

inline PROTOBUF_ALWAYS_INLINE std::pair<const char*, uint64_t>
Parse64FallbackPair(const char* p, int64_t res1) {
  auto ptr = reinterpret_cast<const int8_t*>(p);

  // The algorithm relies on sign extension for each byte to set all high bits
  // when the varint continues. It also relies on asserting all of the lower
  // bits for each successive byte read. This allows the result to be aggregated
  // using a bitwise AND. For example:
  //
  //          8       1          64     57 ... 24     17  16      9  8       1
  // ptr[0] = 1aaa aaaa ; res1 = 1111 1111 ... 1111 1111  1111 1111  1aaa aaaa
  // ptr[1] = 1bbb bbbb ; res2 = 1111 1111 ... 1111 1111  11bb bbbb  b111 1111
  // ptr[2] = 1ccc cccc ; res3 = 0000 0000 ... 000c cccc  cc11 1111  1111 1111
  //                             ---------------------------------------------
  //        res1 & res2 & res3 = 0000 0000 ... 000c cccc  ccbb bbbb  baaa aaaa
  //
  // On x86-64, a shld from a single register filled with enough 1s in the high
  // bits can accomplish all this in one instruction. It so happens that res1
  // has 57 high bits of ones, which is enough for the largest shift done.
  GOOGLE_DCHECK_EQ(res1 >> 7, -1);
  uint64_t ones = res1;  // save the high 1 bits from res1 (input to SHLD)
  uint64_t byte;         // the "next" 7-bit chunk, shifted (result from SHLD)
  int64_t res2, res3;    // accumulated result chunks
#define SHLD(n) byte = ((byte << (n * 7)) | (ones >> (64 - (n * 7))))

  int sign_bit;
#if defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
  // For the first two rounds (ptr[1] and ptr[2]), micro benchmarks show a
  // substantial improvement from capturing the sign from the condition code
  // register on x86-64.
#define SHLD_SIGN(n)                  \
  asm("shldq %3, %2, %1"              \
      : "=@ccs"(sign_bit), "+r"(byte) \
      : "r"(ones), "i"(n * 7))
#else
  // Generic fallback:
#define SHLD_SIGN(n)                           \
  do {                                         \
    SHLD(n);                                   \
    sign_bit = static_cast<int64_t>(byte) < 0; \
  } while (0)
#endif

  byte = ptr[1];
  SHLD_SIGN(1);
  res2 = byte;
  if (!sign_bit) goto done2;
  byte = ptr[2];
  SHLD_SIGN(2);
  res3 = byte;
  if (!sign_bit) goto done3;

#undef SHLD_SIGN

  // For the remainder of the chunks, check the sign of the AND result.
  byte = ptr[3];
  SHLD(3);
  res1 &= byte;
  if (res1 >= 0) goto done4;
  byte = ptr[4];
  SHLD(4);
  res2 &= byte;
  if (res2 >= 0) goto done5;
  byte = ptr[5];
  SHLD(5);
  res3 &= byte;
  if (res3 >= 0) goto done6;
  byte = ptr[6];
  SHLD(6);
  res1 &= byte;
  if (res1 >= 0) goto done7;
  byte = ptr[7];
  SHLD(7);
  res2 &= byte;
  if (res2 >= 0) goto done8;
  byte = ptr[8];
  SHLD(8);
  res3 &= byte;
  if (res3 >= 0) goto done9;

#undef SHLD

  // For valid 64bit varints, the 10th byte/ptr[9] should be exactly 1. In this
  // case, the continuation bit of ptr[8] already set the top bit of res3
  // correctly, so all we have to do is check that the expected case is true.
  byte = ptr[9];
  if (PROTOBUF_PREDICT_TRUE(byte == 1)) goto done10;

  // A value of 0, however, represents an over-serialized varint. This case
  // should not happen, but if does (say, due to a nonconforming serializer),
  // deassert the continuation bit that came from ptr[8].
  if (byte == 0) {
    res3 ^= static_cast<uint64_t>(1) << 63;
    goto done10;
  }

  // If the 10th byte/ptr[9] itself has any other value, then it is too big to
  // fit in 64 bits. If the continue bit is set, it is an unterminated varint.
  return {nullptr, 0};

#define DONE(n) done##n : return {p + n, res1 & res2 & res3};
done2:
  return {p + 2, res1 & res2};
  DONE(3)
  DONE(4)
  DONE(5)
  DONE(6)
  DONE(7)
  DONE(8)
  DONE(9)
  DONE(10)
#undef DONE
}

inline PROTOBUF_ALWAYS_INLINE const char* ParseVarint(const char* p,
                                                      uint64_t* value) {
  int64_t byte = static_cast<int8_t>(*p);
  if (PROTOBUF_PREDICT_TRUE(byte >= 0)) {
    *value = byte;
    return p + 1;
  } else {
    auto tmp = Parse64FallbackPair(p, byte);
    if (PROTOBUF_PREDICT_TRUE(tmp.first)) *value = tmp.second;
    return tmp.first;
  }
}

template <typename FieldType, bool zigzag = false>
inline FieldType ZigZagDecodeHelper(uint64_t value) {
  return static_cast<FieldType>(value);
}

template <>
inline int32_t ZigZagDecodeHelper<int32_t, true>(uint64_t value) {
  return WireFormatLite::ZigZagDecode32(value);
}

template <>
inline int64_t ZigZagDecodeHelper<int64_t, true>(uint64_t value) {
  return WireFormatLite::ZigZagDecode64(value);
}

}  // namespace

template <typename FieldType, typename TagType, bool zigzag>
const char* TcParser::SingularVarint(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
  }
  ptr += sizeof(TagType);  // Consume tag
  hasbits |= (uint64_t{1} << data.hasbit_idx());
  uint64_t tmp;
  ptr = ParseVarint(ptr, &tmp);
  if (ptr == nullptr) {
    return Error(PROTOBUF_TC_PARAM_PASS);
  }
  RefAt<FieldType>(msg, data.offset()) =
      ZigZagDecodeHelper<FieldType, zigzag>(tmp);
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastV8S1(PROTOBUF_TC_PARAM_DECL) {
  return SingularVarint<bool, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV8S2(PROTOBUF_TC_PARAM_DECL) {
  return SingularVarint<bool, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV32S1(PROTOBUF_TC_PARAM_DECL) {
  return SingularVarint<uint32_t, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV32S2(PROTOBUF_TC_PARAM_DECL) {
  return SingularVarint<uint32_t, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV64S1(PROTOBUF_TC_PARAM_DECL) {
  return SingularVarint<uint64_t, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV64S2(PROTOBUF_TC_PARAM_DECL) {
  return SingularVarint<uint64_t, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastZ32S1(PROTOBUF_TC_PARAM_DECL) {
  return SingularVarint<int32_t, uint8_t, true>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ32S2(PROTOBUF_TC_PARAM_DECL) {
  return SingularVarint<int32_t, uint16_t, true>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ64S1(PROTOBUF_TC_PARAM_DECL) {
  return SingularVarint<int64_t, uint8_t, true>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ64S2(PROTOBUF_TC_PARAM_DECL) {
  return SingularVarint<int64_t, uint16_t, true>(PROTOBUF_TC_PARAM_PASS);
}

template <typename FieldType, typename TagType, bool zigzag>
PROTOBUF_NOINLINE const char* TcParser::RepeatedVarint(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    // Try parsing as non-packed repeated:
    InvertPacked<WireFormatLite::WIRETYPE_VARINT>(data);
    if (data.coded_tag<TagType>() == 0) {
      return PackedVarint<FieldType, TagType, zigzag>(PROTOBUF_TC_PARAM_PASS);
    } else {
      PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
    }
  }
  auto& field = RefAt<RepeatedField<FieldType>>(msg, data.offset());
  auto expected_tag = UnalignedLoad<TagType>(ptr);
  do {
    ptr += sizeof(TagType);
    uint64_t tmp;
    ptr = ParseVarint(ptr, &tmp);
    if (ptr == nullptr) {
      return Error(PROTOBUF_TC_PARAM_PASS);
    }
    field.Add(ZigZagDecodeHelper<FieldType, zigzag>(tmp));
    if (!ctx->DataAvailable(ptr)) {
      break;
    }
  } while (UnalignedLoad<TagType>(ptr) == expected_tag);
  return ToParseLoop(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastV8R1(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedVarint<bool, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV8R2(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedVarint<bool, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV32R1(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedVarint<uint32_t, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV32R2(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedVarint<uint32_t, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV64R1(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedVarint<uint64_t, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV64R2(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedVarint<uint64_t, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastZ32R1(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedVarint<int32_t, uint8_t, true>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ32R2(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedVarint<int32_t, uint16_t, true>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ64R1(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedVarint<int64_t, uint8_t, true>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ64R2(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedVarint<int64_t, uint16_t, true>(PROTOBUF_TC_PARAM_PASS);
}

template <typename FieldType, typename TagType, bool zigzag>
PROTOBUF_NOINLINE const char* TcParser::PackedVarint(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    InvertPacked<WireFormatLite::WIRETYPE_VARINT>(data);
    if (data.coded_tag<TagType>() == 0) {
      return RepeatedVarint<FieldType, TagType, zigzag>(PROTOBUF_TC_PARAM_PASS);
    } else {
      PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
    }
  }
  ptr += sizeof(TagType);
  // Since ctx->ReadPackedVarint does not use TailCall or Return, sync any
  // pending hasbits now:
  SyncHasbits(msg, hasbits, table);
  auto* field = &RefAt<RepeatedField<FieldType>>(msg, data.offset());
  return ctx->ReadPackedVarint(ptr, [field](uint64_t varint) {
    FieldType val;
    if (zigzag) {
      if (sizeof(FieldType) == 8) {
        val = WireFormatLite::ZigZagDecode64(varint);
      } else {
        val = WireFormatLite::ZigZagDecode32(varint);
      }
    } else {
      val = varint;
    }
    field->Add(val);
  });
}

const char* TcParser::FastV8P1(PROTOBUF_TC_PARAM_DECL) {
  return PackedVarint<bool, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV8P2(PROTOBUF_TC_PARAM_DECL) {
  return PackedVarint<bool, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV32P1(PROTOBUF_TC_PARAM_DECL) {
  return PackedVarint<uint32_t, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV32P2(PROTOBUF_TC_PARAM_DECL) {
  return PackedVarint<uint32_t, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV64P1(PROTOBUF_TC_PARAM_DECL) {
  return PackedVarint<uint64_t, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV64P2(PROTOBUF_TC_PARAM_DECL) {
  return PackedVarint<uint64_t, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastZ32P1(PROTOBUF_TC_PARAM_DECL) {
  return PackedVarint<int32_t, uint8_t, true>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ32P2(PROTOBUF_TC_PARAM_DECL) {
  return PackedVarint<int32_t, uint16_t, true>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ64P1(PROTOBUF_TC_PARAM_DECL) {
  return PackedVarint<int64_t, uint8_t, true>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ64P2(PROTOBUF_TC_PARAM_DECL) {
  return PackedVarint<int64_t, uint16_t, true>(PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// String/bytes fields
//////////////////////////////////////////////////////////////////////////////

// Defined in wire_format_lite.cc
void PrintUTF8ErrorLog(const char* field_name, const char* operation_str,
                       bool emit_stacktrace);

namespace {

PROTOBUF_NOINLINE
const char* SingularStringParserFallback(ArenaStringPtr* s, const char* ptr,
                                         EpsCopyInputStream* stream) {
  int size = ReadSize(&ptr);
  if (!ptr) return nullptr;
  return stream->ReadString(
      ptr, size, s->MutableNoArenaNoDefault(&GetEmptyStringAlreadyInited()));
}

}  // namespace

template <typename TagType, TcParser::Utf8Type utf8>
const char* TcParser::SingularString(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
  }
  ptr += sizeof(TagType);
  hasbits |= (uint64_t{1} << data.hasbit_idx());
  auto& field = RefAt<ArenaStringPtr>(msg, data.offset());
  auto arena = ctx->data().arena;
  if (arena) {
    ptr = ctx->ReadArenaString(ptr, &field, arena);
  } else {
    ptr = SingularStringParserFallback(&field, ptr, ctx);
  }
  if (ptr == nullptr) return Error(PROTOBUF_TC_PARAM_PASS);
  switch (utf8) {
    case kNoUtf8:
#ifdef NDEBUG
    case kUtf8ValidateOnly:
#endif
      return ToParseLoop(PROTOBUF_TC_PARAM_PASS);
    default:
      if (PROTOBUF_PREDICT_TRUE(IsStructurallyValidUTF8(field.Get()))) {
        return ToParseLoop(PROTOBUF_TC_PARAM_PASS);
      }
      PrintUTF8ErrorLog("unknown", "parsing", false);
      return utf8 == kUtf8 ? Error(PROTOBUF_TC_PARAM_PASS)
                           : ToParseLoop(PROTOBUF_TC_PARAM_PASS);
  }
}

const char* TcParser::FastBS1(PROTOBUF_TC_PARAM_DECL) {
  return SingularString<uint8_t, kNoUtf8>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastBS2(PROTOBUF_TC_PARAM_DECL) {
  return SingularString<uint16_t, kNoUtf8>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastSS1(PROTOBUF_TC_PARAM_DECL) {
  return SingularString<uint8_t, kUtf8ValidateOnly>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastSS2(PROTOBUF_TC_PARAM_DECL) {
  return SingularString<uint16_t, kUtf8ValidateOnly>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastUS1(PROTOBUF_TC_PARAM_DECL) {
  return SingularString<uint8_t, kUtf8>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastUS2(PROTOBUF_TC_PARAM_DECL) {
  return SingularString<uint16_t, kUtf8>(PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType, TcParser::Utf8Type utf8>
const char* TcParser::RepeatedString(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
  }
  auto expected_tag = UnalignedLoad<TagType>(ptr);
  auto& field = RefAt<RepeatedPtrField<std::string>>(msg, data.offset());
  do {
    ptr += sizeof(TagType);
    std::string* str = field.Add();
    ptr = InlineGreedyStringParser(str, ptr, ctx);
    if (ptr == nullptr) {
      return Error(PROTOBUF_TC_PARAM_PASS);
    }
    if (utf8 != kNoUtf8) {
      if (PROTOBUF_PREDICT_FALSE(!IsStructurallyValidUTF8(*str))) {
        PrintUTF8ErrorLog("unknown", "parsing", false);
        if (utf8 == kUtf8) return Error(PROTOBUF_TC_PARAM_PASS);
      }
    }
    if (!ctx->DataAvailable(ptr)) break;
  } while (UnalignedLoad<TagType>(ptr) == expected_tag);
  return ToParseLoop(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastBR1(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedString<uint8_t, kNoUtf8>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastBR2(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedString<uint16_t, kNoUtf8>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastSR1(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedString<uint8_t, kUtf8ValidateOnly>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastSR2(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedString<uint16_t, kUtf8ValidateOnly>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastUR1(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedString<uint8_t, kUtf8>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastUR2(PROTOBUF_TC_PARAM_DECL) {
  return RepeatedString<uint16_t, kUtf8>(PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// Mini parsing
//////////////////////////////////////////////////////////////////////////////

namespace {
inline void SetHas(const TcParseTableBase* table, const FieldEntry& entry,
                   MessageLite* msg, uint64_t& hasbits) {
  int32_t has_idx = entry.has_idx;
  if (has_idx < 32) {
    hasbits |= uint64_t{1} << has_idx;
  } else {
    auto* hasblocks = &TcParser::RefAt<uint32_t>(msg, table->has_bits_offset);
#if defined(__x86_64__) && defined(__GNUC__)
    asm("bts %1, %0\n" : "+m"(*hasblocks) : "r"(has_idx));
#else
    auto& hasblock = hasblocks[has_idx / 32];
    hasblock |= uint32_t{1} << (has_idx % 32);
#endif
  }
}
}  // namespace

const char* TcParser::MpFixed(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint16_t card = type_card & field_layout::kFcMask;

  // Check for repeated parsing (wiretype fallback is handled there):
  if (card == field_layout::kFcRepeated) {
    PROTOBUF_MUSTTAIL return MpRepeatedFixed(PROTOBUF_TC_PARAM_PASS);
  }
  if (card == field_layout::kFcOneof) {
    // TODO(b/206520218): support oneofs
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }

  // Check the wiretype, copy the value, and set the has bit if necessary.
  const uint16_t rep = type_card & field_layout::kRepMask;
  const uint32_t decoded_wiretype = data.tag() & 7;
  if (rep == field_layout::kRep64Bits) {
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED64) {
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
    std::memcpy(Offset(msg, entry.offset), ptr, sizeof(uint64_t));
    ptr += sizeof(uint64_t);
  } else {
    GOOGLE_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep32Bits));
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED32) {
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
    std::memcpy(Offset(msg, entry.offset), ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
  }
  if (card == field_layout::kFcOptional) {
    SetHas(table, entry, msg, hasbits);
  }
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::MpRepeatedFixed(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint32_t decoded_tag = data.tag();
  const uint32_t decoded_wiretype = decoded_tag & 7;

  // Check for packed repeated fallback:
  if (decoded_wiretype == WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return MpPackedFixed(PROTOBUF_TC_PARAM_PASS);
  }

  const uint16_t type_card = entry.type_card;
  const uint16_t rep = type_card & field_layout::kRepMask;
  if (rep == field_layout::kRep64Bits) {
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED64) {
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
    auto& field = RefAt<RepeatedField<uint64_t>>(msg, entry.offset);
    constexpr auto size = sizeof(uint64_t);
    const char* ptr2 = ptr;
    uint32_t next_tag;
    do {
      ptr = ptr2;
      std::memcpy(field.Add(), ptr, size);
      ptr += size;
      if (!ctx->DataAvailable(ptr)) break;
      ptr2 = ReadTag(ptr, &next_tag);
    } while (next_tag == decoded_tag);
  } else {
    GOOGLE_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep32Bits));
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED32) {
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
    auto& field = RefAt<RepeatedField<uint32_t>>(msg, entry.offset);
    constexpr auto size = sizeof(uint32_t);
    const char* ptr2 = ptr;
    uint32_t next_tag;
    do {
      ptr = ptr2;
      std::memcpy(field.Add(), ptr, size);
      ptr += size;
      if (!ctx->DataAvailable(ptr)) break;
      ptr2 = ReadTag(ptr, &next_tag);
    } while (next_tag == decoded_tag);
  }

  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::MpPackedFixed(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint32_t decoded_wiretype = data.tag() & 7;

  // Check for non-packed repeated fallback:
  if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return MpRepeatedFixed(PROTOBUF_TC_PARAM_PASS);
  }

  // Since ctx->ReadPackedFixed does not use TailCall<> or Return<>, sync any
  // pending hasbits now:
  SyncHasbits(msg, hasbits, table);

  int size = ReadSize(&ptr);
  uint16_t rep = type_card & field_layout::kRepMask;
  if (rep == field_layout::kRep64Bits) {
    auto& field = RefAt<RepeatedField<uint64_t>>(msg, entry.offset);
    ptr = ctx->ReadPackedFixed(ptr, size, &field);
  } else {
    GOOGLE_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep32Bits));
    auto& field = RefAt<RepeatedField<uint32_t>>(msg, entry.offset);
    ptr = ctx->ReadPackedFixed(ptr, size, &field);
  }

  if (ptr == nullptr) {
    return Error(PROTOBUF_TC_PARAM_PASS);
  }
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::MpVarint(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint16_t card = type_card & field_layout::kFcMask;

  // Check for repeated parsing:
  if (card == field_layout::kFcRepeated) {
    PROTOBUF_MUSTTAIL return MpRepeatedVarint(PROTOBUF_TC_PARAM_PASS);
  }
  if (card == field_layout::kFcOneof) {
    // TODO(b/206520218): handle oneofs
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }
  // Check for wire type mismatch:
  if ((data.tag() & 7) != WireFormatLite::WIRETYPE_VARINT) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }
  const uint16_t xform_val = type_card & field_layout::kTvMask;
  const bool is_zigzag = xform_val == field_layout::kTvZigZag;
  const bool is_enum = xform_val == field_layout::kTvEnum;
  if (is_enum) {
    // TODO(b/206890171): handle enums
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }

  // Parse the value:
  uint64_t tmp;
  ptr = ParseVarint(ptr, &tmp);
  if (ptr == nullptr) return Error(PROTOBUF_TC_PARAM_PASS);

  // Transform, validate, and store the value based on the field size:
  uint16_t rep = type_card & field_layout::kRepMask;
  if (rep == field_layout::kRep64Bits) {
    if (is_zigzag) {
      tmp = WireFormatLite::ZigZagDecode64(tmp);
    }
    RefAt<uint64_t>(msg, entry.offset) = tmp;
  } else if (rep == field_layout::kRep32Bits) {
    if (is_zigzag) {
      tmp = WireFormatLite::ZigZagDecode32(static_cast<uint32_t>(tmp));
    }
    RefAt<uint32_t>(msg, entry.offset) = static_cast<uint32_t>(tmp);
  } else {
    GOOGLE_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep8Bits));
    RefAt<bool>(msg, entry.offset) = static_cast<bool>(tmp);
  }

  // Mark the field as present:
  if (card == field_layout::kFcOptional) {
    SetHas(table, entry, msg, hasbits);
  }

  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::MpRepeatedVarint(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  auto type_card = entry.type_card;
  const uint32_t decoded_tag = data.tag();
  auto decoded_wiretype = decoded_tag & 7;

  // Check for packed repeated fallback:
  if (decoded_wiretype == WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return MpPackedVarint(PROTOBUF_TC_PARAM_PASS);
  }
  // Check for wire type mismatch:
  if (decoded_wiretype != WireFormatLite::WIRETYPE_VARINT) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }
  uint16_t xform_val = (type_card & field_layout::kTvMask);
  const bool zigzag = xform_val == field_layout::kTvZigZag;
  const bool is_enum = xform_val == field_layout::kTvEnum;
  if (is_enum) {
    // TODO(b/206890171): handle enums
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }

  uint16_t rep = type_card & field_layout::kRepMask;
  if (rep == field_layout::kRep64Bits) {
    auto& field = RefAt<RepeatedField<uint64_t>>(msg, entry.offset);
    const char* ptr2 = ptr;
    uint32_t next_tag;
    do {
      uint64_t tmp;
      ptr = ParseVarint(ptr2, &tmp);
      if (ptr == nullptr) return Error(PROTOBUF_TC_PARAM_PASS);
      field.Add(zigzag ? WireFormatLite::ZigZagDecode64(tmp) : tmp);
      if (!ctx->DataAvailable(ptr)) break;
      ptr2 = ReadTag(ptr, &next_tag);
    } while (next_tag == decoded_tag);
  } else if (rep == field_layout::kRep32Bits) {
    auto& field = RefAt<RepeatedField<uint32_t>>(msg, entry.offset);
    const char* ptr2 = ptr;
    uint32_t next_tag;
    do {
      uint64_t tmp;
      ptr = ParseVarint(ptr2, &tmp);
      if (ptr == nullptr) return Error(PROTOBUF_TC_PARAM_PASS);
      field.Add(zigzag ? WireFormatLite::ZigZagDecode32(tmp) : tmp);
      if (!ctx->DataAvailable(ptr)) break;
      ptr2 = ReadTag(ptr, &next_tag);
    } while (next_tag == decoded_tag);
  } else {
    GOOGLE_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep8Bits));
    auto& field = RefAt<RepeatedField<bool>>(msg, entry.offset);
    const char* ptr2 = ptr;
    uint32_t next_tag;
    do {
      uint64_t tmp;
      ptr = ParseVarint(ptr2, &tmp);
      if (ptr == nullptr) return Error(PROTOBUF_TC_PARAM_PASS);
      field.Add(static_cast<bool>(tmp));
      if (!ctx->DataAvailable(ptr)) break;
      ptr2 = ReadTag(ptr, &next_tag);
    } while (next_tag == decoded_tag);
  }

  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::MpPackedVarint(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  auto type_card = entry.type_card;
  auto decoded_wiretype = data.tag() & 7;

  // Check for non-packed repeated fallback:
  if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return MpRepeatedVarint(PROTOBUF_TC_PARAM_PASS);
  }
  uint16_t xform_val = (type_card & field_layout::kTvMask);
  const bool zigzag = xform_val == field_layout::kTvZigZag;
  const bool is_enum = xform_val == field_layout::kTvEnum;
  if (is_enum) {
    // TODO(b/206890171): handle enums
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }

  // Since ctx->ReadPackedFixed does not use TailCall<> or Return<>, sync any
  // pending hasbits now:
  SyncHasbits(msg, hasbits, table);

  uint16_t rep = type_card & field_layout::kRepMask;
  if (rep == field_layout::kRep64Bits) {
    auto* field = &RefAt<RepeatedField<uint64_t>>(msg, entry.offset);
    return ctx->ReadPackedVarint(ptr, [field, zigzag](uint64_t value) {
      field->Add(zigzag ? WireFormatLite::ZigZagDecode64(value) : value);
    });
  } else if (rep == field_layout::kRep32Bits) {
    auto* field = &RefAt<RepeatedField<uint32_t>>(msg, entry.offset);
    return ctx->ReadPackedVarint(ptr, [field, zigzag](uint64_t value) {
      field->Add(
          zigzag ? WireFormatLite::ZigZagDecode32(static_cast<uint32_t>(value))
                 : value);
    });
  } else {
    GOOGLE_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep8Bits));
    auto* field = &RefAt<RepeatedField<bool>>(msg, entry.offset);
    return ctx->ReadPackedVarint(
        ptr, [field](uint64_t value) { field->Add(value); });
  }

  return Error(PROTOBUF_TC_PARAM_PASS);
}

namespace {

inline bool MpVerifyUtf8(StringPiece wire_bytes, const FieldEntry& entry,
                         uint16_t xform_val) {
  if (xform_val == field_layout::kTvUtf8) {
    return VerifyUTF8(wire_bytes, "unknown");
  }
#ifndef NDEBUG
  if (xform_val == field_layout::kTvUtf8Debug) {
    VerifyUTF8(wire_bytes, "unknown");
  }
#endif  // NDEBUG
  return true;
}

}  // namespace

const char* TcParser::MpString(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint16_t card = type_card & field_layout::kFcMask;
  const uint32_t decoded_wiretype = data.tag() & 7;

  if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }
  if (card == field_layout::kFcRepeated) {
    PROTOBUF_MUSTTAIL return MpRepeatedString(PROTOBUF_TC_PARAM_PASS);
  }
  if (card == field_layout::kFcOneof) {
    // TODO(b/206520218): handle oneofs
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }
  const uint16_t xform_val = type_card & field_layout::kTvMask;

  // TODO(b/209516305): handle UTF-8 fields once field names are available.
  if (
#ifdef NDEBUG
      xform_val == field_layout::kTvUtf8
#else
      xform_val != 0
#endif
  ) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }

  const uint16_t rep = type_card & field_layout::kRepMask;
  if (rep == field_layout::kRepIString) {
    // TODO(b/198211897): support InilnedStringField.
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }

  // Mark the field as present:
  if (card == field_layout::kFcOptional) {
    SetHas(table, entry, msg, hasbits);
  }

  bool is_valid = false;
  switch (rep) {
    case field_layout::kRepAString: {
      const std::string* default_value =
          RefAt<ArenaStringPtr>(table->default_instance, entry.offset)
              .tagged_ptr_.Get();
      auto* arena = ctx->data().arena;
      auto& field = RefAt<ArenaStringPtr>(msg, entry.offset);
      if (arena) {
        ptr = ctx->ReadArenaString(ptr, &field, arena);
      } else {
        std::string* str = field.MutableNoCopy(default_value, nullptr);
        ptr = InlineGreedyStringParser(str, ptr, ctx);
      }
      is_valid = MpVerifyUtf8(field.Get(), entry, xform_val);
      break;
    }

    case field_layout::kRepIString:
      break;  // note: skipped above
  }

  if (ptr == nullptr || !is_valid) {
    return Error(PROTOBUF_TC_PARAM_PASS);
  }
  return ToParseLoop(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::MpRepeatedString(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint32_t decoded_tag = data.tag();
  const uint32_t decoded_wiretype = decoded_tag & 7;

  if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }

  const uint16_t rep = type_card & field_layout::kRepMask;
  const uint16_t xform_val = type_card & field_layout::kTvMask;

  // TODO(b/209516305): handle UTF-8 fields once field names are available.
  if (
#ifdef NDEBUG
      xform_val == field_layout::kTvUtf8
#else
      xform_val != 0
#endif
  ) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }

  switch (rep) {
    case field_layout::kRepSString: {
      auto& field = RefAt<RepeatedPtrField<std::string>>(msg, entry.offset);
      const char* ptr2 = ptr;
      uint32_t next_tag;
      do {
        ptr = ptr2;
        std::string* str = field.Add();
        ptr = InlineGreedyStringParser(str, ptr, ctx);
        if (PROTOBUF_PREDICT_FALSE(ptr == nullptr ||
                                   !MpVerifyUtf8(*str, entry, xform_val))) {
          return Error(PROTOBUF_TC_PARAM_PASS);
        }
        if (!ctx->DataAvailable(ptr)) break;
        ptr2 = ReadTag(ptr, &next_tag);
      } while (next_tag == decoded_tag);
      break;
    }

#ifndef NDEBUG
    default:
      GOOGLE_LOG(FATAL) << "Unsupported repeated string rep: " << rep;
      break;
#endif
  }

  return ToParseLoop(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::MpMessage(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint16_t card = type_card & field_layout::kFcMask;

  // Check for repeated parsing:
  if (card == field_layout::kFcRepeated) {
    PROTOBUF_MUSTTAIL return MpRepeatedMessage(PROTOBUF_TC_PARAM_PASS);
  }
  if (card == field_layout::kFcOneof) {
    // TODO(b/206520218): handle oneofs
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }
  // Check for wire type mismatch:
  // TODO(b/210762816): support groups.
  if ((data.tag() & 7) != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }
  // Lazy and implicit weak fields are handled by generated code:
  // TODO(b/210762816): support these.
  if ((type_card & field_layout::kRepMask) != field_layout::kRepMessage) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }

  MessageLite*& field = RefAt<MessageLite*>(msg, entry.offset);
  if (field == nullptr) {
    const MessageLite* default_instance =
        table->field_aux(&entry)->message_default;
    field = default_instance->New(ctx->data().arena);
  }
  if (card == field_layout::kFcOptional) {
    SetHas(table, entry, msg, hasbits);
  }
  SyncHasbits(msg, hasbits, table);
  return ctx->ParseMessage(field, ptr);
}

const char* TcParser::MpRepeatedMessage(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  GOOGLE_DCHECK_EQ(type_card & field_layout::kFcMask,
            static_cast<uint16_t>(field_layout::kFcRepeated));

  // Check for wire type mismatch:
  // TODO(b/210762816): support groups.
  if ((data.tag() & 7) != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }
  // Implicit weak fields are handled by generated code:
  // TODO(b/210762816): support these.
  if ((type_card & field_layout::kRepMask) != field_layout::kRepMessage) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }

  SyncHasbits(msg, hasbits, table);
  const MessageLite* default_instance =
      table->field_aux(&entry)->message_default;
  auto& field = RefAt<RepeatedPtrFieldBase>(msg, entry.offset);
  MessageLite* value =
      field.Add<GenericTypeHandler<MessageLite>>(default_instance);
  return ctx->ParseMessage(value, ptr);
}

const char* TcParser::MpMap(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  (void)entry;
  PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
