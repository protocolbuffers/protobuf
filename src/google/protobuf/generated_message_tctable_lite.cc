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
#include <numeric>

#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_message_tctable_decl.h>
#include <google/protobuf/generated_message_tctable_impl.h>
#include <google/protobuf/inlined_string_field.h>
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

//////////////////////////////////////////////////////////////////////////////
// Template instantiations:
//////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
template void AlignFail<4>(uintptr_t);
template void AlignFail<8>(uintptr_t);
#endif

const char* TcParser::GenericFallbackLite(PROTOBUF_TC_PARAM_DECL) {
  return GenericFallbackImpl<MessageLite, std::string>(PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// Core fast parsing implementation:
//////////////////////////////////////////////////////////////////////////////

class TcParser::ScopedArenaSwap final {
 public:
  ScopedArenaSwap(MessageLite* msg, ParseContext* ctx)
      : ctx_(ctx), saved_(ctx->data().arena) {
    ctx_->data().arena = msg->GetArenaForAllocation();
  }
  ScopedArenaSwap(const ScopedArenaSwap&) = delete;
  ~ScopedArenaSwap() { ctx_->data().arena = saved_; }

 private:
  ParseContext* const ctx_;
  Arena* const saved_;
};

PROTOBUF_NOINLINE const char* TcParser::ParseLoop(
    MessageLite* msg, const char* ptr, ParseContext* ctx,
    const TcParseTableBase* table) {
  ScopedArenaSwap saved(msg, ctx);
  while (!ctx->Done(&ptr)) {
    // Unconditionally read has bits, even if we don't have has bits.
    // has_bits_offset will be 0 and we will just read something valid.
    uint64_t hasbits = ReadAt<uint32_t>(msg, table->has_bits_offset);
    ptr = TagDispatch(msg, ptr, ctx, table, hasbits, {});
    if (ptr == nullptr) break;
    if (ctx->LastTag() != 1) break;  // Ended on terminating tag
  }
  return ptr;
}

  // Dispatch to the designated parse function
inline PROTOBUF_ALWAYS_INLINE const char* TcParser::TagDispatch(
    PROTOBUF_TC_PARAM_DECL) {
  const auto coded_tag = UnalignedLoad<uint16_t>(ptr);
  const size_t idx = coded_tag & table->fast_idx_mask;
  PROTOBUF_ASSUME((idx & 7) == 0);
  auto* fast_entry = table->fast_entry(idx >> 3);
  data = fast_entry->bits;
  data.data ^= coded_tag;
  PROTOBUF_MUSTTAIL return fast_entry->target(PROTOBUF_TC_PARAM_PASS);
}

// We can only safely call from field to next field if the call is optimized
// to a proper tail call. Otherwise we blow through stack. Clang and gcc
// reliably do this optimization in opt mode, but do not perform this in debug
// mode. Luckily the structure of the algorithm is such that it's always
// possible to just return and use the enclosing parse loop as a trampoline.
inline PROTOBUF_ALWAYS_INLINE const char* TcParser::ToTagDispatch(
    PROTOBUF_TC_PARAM_DECL) {
  constexpr bool always_return = !PROTOBUF_TAILCALL;
  if (always_return || !ctx->DataAvailable(ptr)) {
    PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_PASS);
  }
  PROTOBUF_MUSTTAIL return TagDispatch(PROTOBUF_TC_PARAM_PASS);
}

inline PROTOBUF_ALWAYS_INLINE const char* TcParser::ToParseLoop(
    PROTOBUF_TC_PARAM_DECL) {
  (void)data;
  (void)ctx;
  SyncHasbits(msg, hasbits, table);
  return ptr;
}

inline PROTOBUF_ALWAYS_INLINE const char* TcParser::Error(
    PROTOBUF_TC_PARAM_DECL) {
  (void)data;
  (void)ctx;
  (void)ptr;
  SyncHasbits(msg, hasbits, table);
  return nullptr;
}

// On the fast path, a (matching) 1-byte tag already has the decoded value.
static uint32_t FastDecodeTag(uint8_t coded_tag) {
  return coded_tag;
}

// On the fast path, a (matching) 2-byte tag always needs to be decoded.
static uint32_t FastDecodeTag(uint16_t coded_tag) {
  uint32_t result = coded_tag;
  result += static_cast<int8_t>(coded_tag);
  return result >> 1;
}

//////////////////////////////////////////////////////////////////////////////
// Core mini parsing implementation:
//////////////////////////////////////////////////////////////////////////////

// Field lookup table layout:
//
// Because it consists of a series of variable-length segments, the lookuup
// table is organized within an array of uint16_t, and each element is either
// a uint16_t or a uint32_t stored little-endian as a pair of uint16_t.
//
// Its fundamental building block maps 16 contiguously ascending field numbers
// to their locations within the field entry table:

struct SkipEntry16 {
  uint16_t skipmap;
  uint16_t field_entry_offset;
};

// The skipmap is a bitfield of which of those field numbers do NOT have a
// field entry.  The lowest bit of the skipmap corresponds to the lowest of
// the 16 field numbers, so if a proto had only fields 1, 2, 3, and 7, the
// skipmap would contain 0b11111111'10111000.
//
// The field lookup table begins with a single 32-bit skipmap that maps the
// field numbers 1 through 32.  This is because the majority of proto
// messages only contain fields numbered 1 to 32.
//
// The rest of the lookup table is a repeated series of
// { 32-bit field #,  #SkipEntry16s,  {SkipEntry16...} }
// That is, the next thing is a pair of uint16_t that form the next
// lowest field number that the lookup table handles.  If this number is -1,
// that is the end of the table.  Then there is a uint16_t that is
// the number of contiguous SkipEntry16 entries that follow, and then of
// course the SkipEntry16s themselves.

// Originally developed and tested at https://godbolt.org/z/vbc7enYcf

// Returns the address of the field for `tag` in the table's field entries.
// Returns nullptr if the field was not found.
const TcParseTableBase::FieldEntry* TcParser::FindFieldEntry(
    const TcParseTableBase* table, uint32_t field_num) {
  const FieldEntry* const field_entries = table->field_entries_begin();

  uint32_t fstart = 1;
  uint32_t adj_fnum = field_num - fstart;

  if (PROTOBUF_PREDICT_TRUE(adj_fnum < 32)) {
    uint32_t skipmap = table->skipmap32;
    uint32_t skipbit = 1 << adj_fnum;
    if (PROTOBUF_PREDICT_FALSE(skipmap & skipbit)) return nullptr;
    skipmap &= skipbit - 1;
#if (__GNUC__ || __clang__) && __POPCNT__
    // Note: here and below, skipmap typically has very few set bits
    // (31 in the worst case, but usually zero) so a loop isn't that
    // bad, and a compiler-generated popcount is typically only
    // worthwhile if the processor itself has hardware popcount support.
    adj_fnum -= __builtin_popcount(skipmap);
#else
    while (skipmap) {
      --adj_fnum;
      skipmap &= skipmap - 1;
    }
#endif
    auto* entry = field_entries + adj_fnum;
    PROTOBUF_ASSUME(entry != nullptr);
    return entry;
  }
  const uint16_t* lookup_table = table->field_lookup_begin();
  for (;;) {
#ifdef PROTOBUF_LITTLE_ENDIAN
    memcpy(&fstart, lookup_table, sizeof(fstart));
#else
    fstart = lookup_table[0] | (lookup_table[1] << 16);
#endif
    lookup_table += sizeof(fstart) / sizeof(*lookup_table);
    uint32_t num_skip_entries = *lookup_table++;
    if (field_num < fstart) return nullptr;
    adj_fnum = field_num - fstart;
    uint32_t skip_num = adj_fnum / 16;
    if (PROTOBUF_PREDICT_TRUE(skip_num < num_skip_entries)) {
      // for each group of 16 fields we have:
      // a bitmap of 16 bits
      // a 16-bit field-entry offset for the first of them.
      auto* skip_data = lookup_table + (adj_fnum / 16) * (sizeof(SkipEntry16) /
                                                          sizeof(uint16_t));
      SkipEntry16 se = {skip_data[0], skip_data[1]};
      adj_fnum &= 15;
      uint32_t skipmap = se.skipmap;
      uint16_t skipbit = 1 << adj_fnum;
      if (PROTOBUF_PREDICT_FALSE(skipmap & skipbit)) return nullptr;
      skipmap &= skipbit - 1;
      adj_fnum += se.field_entry_offset;
#if (__GNUC__ || __clang__) && __POPCNT__
      adj_fnum -= __builtin_popcount(skipmap);
#else
      while (skipmap) {
        --adj_fnum;
        skipmap &= skipmap - 1;
      }
#endif
      auto* entry = field_entries + adj_fnum;
      PROTOBUF_ASSUME(entry != nullptr);
      return entry;
    }
    lookup_table +=
        num_skip_entries * (sizeof(SkipEntry16) / sizeof(*lookup_table));
  }
}

// Field names are stored in a format of:
//
// 1) A table of name sizes, one byte each, from 1 to 255 per name.
//    `entries` is the size of this first table.
// 1a) padding bytes, so the table of name sizes is a multiple of
//     eight bytes in length. They are zero.
//
// 2) All the names, concatenated, with neither separation nor termination.
//
// This is designed to be compact but not particularly fast to retrieve.
// In particular, it takes O(n) to retrieve the name of the n'th field,
// which is usually fine because most protos have fewer than 10 fields.
static StringPiece FindName(const char* name_data, size_t entries,
                                  size_t index) {
  // The compiler unrolls these... if this isn't fast enough,
  // there's an AVX version at https://godbolt.org/z/eojrjqzfr
  // ARM-compatible version at https://godbolt.org/z/n5YT5Ee85

  // The field name sizes are padded up to a multiple of 8, so we
  // must pad them here.
  size_t num_sizes = (entries + 7) & -8;
  auto* uint8s = reinterpret_cast<const uint8_t*>(name_data);
  size_t pos = std::accumulate(uint8s, uint8s + index, num_sizes);
  size_t size = name_data[index];
  auto* start = &name_data[pos];
  return {start, size};
}

StringPiece TcParser::MessageName(const TcParseTableBase* table) {
  return FindName(table->name_data(), table->num_field_entries + 1, 0);
}

StringPiece TcParser::FieldName(const TcParseTableBase* table,
                                      const FieldEntry* field_entry) {
  const FieldEntry* const field_entries = table->field_entries_begin();
  auto field_index = static_cast<size_t>(field_entry - field_entries);
  return FindName(table->name_data(), table->num_field_entries + 1,
                  field_index + 1);
}

const char* TcParser::MiniParse(PROTOBUF_TC_PARAM_DECL) {
  uint32_t tag;
  ptr = ReadTagInlined(ptr, &tag);
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

template <typename TagType, bool group_coding>
inline PROTOBUF_ALWAYS_INLINE
const char* TcParser::SingularParseMessageAuxImpl(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
  }
  auto saved_tag = UnalignedLoad<TagType>(ptr);
  ptr += sizeof(TagType);
  hasbits |= (uint64_t{1} << data.hasbit_idx());
  SyncHasbits(msg, hasbits, table);
  auto& field = RefAt<MessageLite*>(msg, data.offset());
  if (field == nullptr) {
    const MessageLite* default_instance =
        table->field_aux(data.aux_idx())->message_default;
    field = default_instance->New(ctx->data().arena);
  }
  if (group_coding) {
    return ctx->ParseGroup(field, ptr, FastDecodeTag(saved_tag));
  }
  return ctx->ParseMessage(field, ptr);
}

const char* TcParser::FastMS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularParseMessageAuxImpl<uint8_t, false>(
      PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastMS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularParseMessageAuxImpl<uint16_t, false>(
      PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastGS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularParseMessageAuxImpl<uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastGS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularParseMessageAuxImpl<uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType, bool group_coding>
inline PROTOBUF_ALWAYS_INLINE
const char* TcParser::RepeatedParseMessageAuxImpl(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
  }
  auto saved_tag = UnalignedLoad<TagType>(ptr);
  ptr += sizeof(TagType);
  SyncHasbits(msg, hasbits, table);
  const MessageLite* default_instance =
      table->field_aux(data.aux_idx())->message_default;
  auto& field = RefAt<RepeatedPtrFieldBase>(msg, data.offset());
  MessageLite* submsg =
      field.Add<GenericTypeHandler<MessageLite>>(default_instance);
  if (group_coding) {
    return ctx->ParseGroup(submsg, ptr, FastDecodeTag(saved_tag));
  }
  return ctx->ParseMessage(submsg, ptr);
}

const char* TcParser::FastMR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedParseMessageAuxImpl<uint8_t, false>(
      PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastMR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedParseMessageAuxImpl<uint16_t, false>(
      PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastGR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedParseMessageAuxImpl<uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastGR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedParseMessageAuxImpl<uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// Fixed fields
//////////////////////////////////////////////////////////////////////////////

template <typename LayoutType, typename TagType>
PROTOBUF_ALWAYS_INLINE const char* TcParser::SingularFixed(
    PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
  }
  ptr += sizeof(TagType);  // Consume tag
  hasbits |= (uint64_t{1} << data.hasbit_idx());
  RefAt<LayoutType>(msg, data.offset()) = UnalignedLoad<LayoutType>(ptr);
  ptr += sizeof(LayoutType);
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastF32S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularFixed<uint32_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF32S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularFixed<uint32_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF64S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularFixed<uint64_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF64S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularFixed<uint64_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename LayoutType, typename TagType>
PROTOBUF_ALWAYS_INLINE const char* TcParser::RepeatedFixed(
    PROTOBUF_TC_PARAM_DECL) {
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
    elem[idx++] = UnalignedLoad<LayoutType>(ptr);
    ptr += sizeof(LayoutType);
    if (idx >= space) break;
    if (!ctx->DataAvailable(ptr)) break;
  } while (UnalignedLoad<TagType>(ptr) == expected_tag);
  field.AddNAlreadyReserved(idx - 1);
  return ToParseLoop(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastF32R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedFixed<uint32_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF32R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedFixed<uint32_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF64R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedFixed<uint64_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF64R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedFixed<uint64_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}

// Note: some versions of GCC will fail with error "function not inlinable" if
// corecursive functions are both marked with PROTOBUF_ALWAYS_INLINE (Clang
// accepts this). We can still apply the attribute to one of the two functions,
// just not both (so we do mark the Repeated variant as always inlined). This
// also applies to PackedVarint, below.
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
  PROTOBUF_MUSTTAIL return PackedFixed<uint32_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF32P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedFixed<uint32_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF64P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedFixed<uint64_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastF64P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedFixed<uint64_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// Varint fields
//////////////////////////////////////////////////////////////////////////////

namespace {

// Shift "byte" left by n * 7 bits, filling vacated bits with ones.
template <int n>
inline PROTOBUF_ALWAYS_INLINE uint64_t
shift_left_fill_with_ones(uint64_t byte, uint64_t ones) {
  return (byte << (n * 7)) | (ones >> (64 - (n * 7)));
}

// Shift "byte" left by n * 7 bits, filling vacated bits with ones, and
// put the new value in res.  Return whether the result was negative.
template <int n>
inline PROTOBUF_ALWAYS_INLINE bool shift_left_fill_with_ones_was_negative(
    uint64_t byte, uint64_t ones, int64_t& res) {
#if defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
  // For the first two rounds (ptr[1] and ptr[2]), micro benchmarks show a
  // substantial improvement from capturing the sign from the condition code
  // register on x86-64.
  bool sign_bit;
  asm("shldq %3, %2, %1"
      : "=@ccs"(sign_bit), "+r"(byte)
      : "r"(ones), "i"(n * 7));
  res = byte;
  return sign_bit;
#else
  // Generic fallback:
  res = (byte << (n * 7)) | (ones >> (64 - (n * 7)));
  return static_cast<int64_t>(res) < 0;
#endif
}

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
  int64_t res2, res3;    // accumulated result chunks

  if (!shift_left_fill_with_ones_was_negative<1>(ptr[1], ones, res2))
    goto done2;
  if (!shift_left_fill_with_ones_was_negative<2>(ptr[2], ones, res3))
    goto done3;

  // For the remainder of the chunks, check the sign of the AND result.
  res1 &= shift_left_fill_with_ones<3>(ptr[3], ones);
  if (res1 >= 0) goto done4;
  res2 &= shift_left_fill_with_ones<4>(ptr[4], ones);
  if (res2 >= 0) goto done5;
  res3 &= shift_left_fill_with_ones<5>(ptr[5], ones);
  if (res3 >= 0) goto done6;
  res1 &= shift_left_fill_with_ones<6>(ptr[6], ones);
  if (res1 >= 0) goto done7;
  res2 &= shift_left_fill_with_ones<7>(ptr[7], ones);
  if (res2 >= 0) goto done8;
  res3 &= shift_left_fill_with_ones<8>(ptr[8], ones);
  if (res3 >= 0) goto done9;

  // For valid 64bit varints, the 10th byte/ptr[9] should be exactly 1. In this
  // case, the continuation bit of ptr[8] already set the top bit of res3
  // correctly, so all we have to do is check that the expected case is true.
  if (PROTOBUF_PREDICT_TRUE(ptr[9] == 1)) goto done10;

  // A value of 0, however, represents an over-serialized varint. This case
  // should not happen, but if does (say, due to a nonconforming serializer),
  // deassert the continuation bit that came from ptr[8].
  if (ptr[9] == 0) {
#if defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
    // Use a small instruction since this is an uncommon code path.
    asm("btcq $63,%0" : "+r"(res3));
#else
    res3 ^= static_cast<uint64_t>(1) << 63;
#endif
    goto done10;
  }

  // If the 10th byte/ptr[9] itself has any other value, then it is too big to
  // fit in 64 bits. If the continue bit is set, it is an unterminated varint.
  return {nullptr, 0};

done2:
  return {p + 2, res1 & res2};
done3:
  return {p + 3, res1 & res2 & res3};
done4:
  return {p + 4, res1 & res2 & res3};
done5:
  return {p + 5, res1 & res2 & res3};
done6:
  return {p + 6, res1 & res2 & res3};
done7:
  return {p + 7, res1 & res2 & res3};
done8:
  return {p + 8, res1 & res2 & res3};
done9:
  return {p + 9, res1 & res2 & res3};
done10:
  return {p + 10, res1 & res2 & res3};
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

bool EnumIsValidAux(int32_t val, uint16_t xform_val,
                    TcParseTableBase::FieldAux aux) {
  if (xform_val == field_layout::kTvRange) {
    auto lo = aux.enum_range.start;
    return lo <= val && val < (lo + aux.enum_range.length);
  }
  return aux.enum_validator(val);
}

}  // namespace

template <typename FieldType, typename TagType, bool zigzag>
PROTOBUF_ALWAYS_INLINE const char* TcParser::SingularVarint(
    PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
  }
  ptr += sizeof(TagType);  // Consume tag
  hasbits |= (uint64_t{1} << data.hasbit_idx());

  // clang isn't smart enough to be able to only conditionally save
  // registers to the stack, so we turn the integer-greater-than-128
  // case into a separate routine.
  if (PROTOBUF_PREDICT_FALSE(static_cast<int8_t>(*ptr) < 0)) {
    PROTOBUF_MUSTTAIL return SingularVarBigint<FieldType, TagType, zigzag>(
        PROTOBUF_TC_PARAM_PASS);
  }

  RefAt<FieldType>(msg, data.offset()) =
      ZigZagDecodeHelper<FieldType, zigzag>(static_cast<uint8_t>(*ptr++));
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
}

template <typename FieldType, typename TagType, bool zigzag>
PROTOBUF_NOINLINE const char* TcParser::SingularVarBigint(
    PROTOBUF_TC_PARAM_DECL) {
  // For some reason clang wants to save 5 registers to the stack here,
  // but we only need four for this code, so save the data we don't need
  // to the stack.  Happily, saving them this way uses regular store
  // instructions rather than PUSH/POP, which saves time at the cost of greater
  // code size, but for this heavily-used piece of code, that's fine.
  struct Spill {
    uint64_t field_data;
    ::google::protobuf::MessageLite* msg;
    const ::google::protobuf::internal::TcParseTableBase* table;
    uint64_t hasbits;
  };
  volatile Spill spill = {data.data, msg, table, hasbits};

  uint64_t tmp;
  PROTOBUF_ASSUME(static_cast<int8_t>(*ptr) < 0);
  ptr = ParseVarint(ptr, &tmp);

  data.data = spill.field_data;
  msg = spill.msg;
  table = spill.table;
  hasbits = spill.hasbits;

  if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) {
    return Error(PROTOBUF_TC_PARAM_PASS);
  }
  RefAt<FieldType>(msg, data.offset()) =
      ZigZagDecodeHelper<FieldType, zigzag>(tmp);
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastV8S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<bool, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV8S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<bool, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV32S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<uint32_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV32S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<uint32_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV64S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<uint64_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV64S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<uint64_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastZ32S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<int32_t, uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ32S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<int32_t, uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ64S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<int64_t, uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ64S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<int64_t, uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename FieldType, typename TagType, bool zigzag>
PROTOBUF_ALWAYS_INLINE const char* TcParser::RepeatedVarint(
    PROTOBUF_TC_PARAM_DECL) {
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
  PROTOBUF_MUSTTAIL return RepeatedVarint<bool, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV8R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<bool, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV32R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<uint32_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV32R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<uint32_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV64R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<uint64_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV64R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<uint64_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastZ32R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<int32_t, uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ32R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<int32_t, uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ64R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<int64_t, uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ64R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<int64_t, uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}

// See comment on PackedFixed for why this is not PROTOBUF_ALWAYS_INLINE.
template <typename FieldType, typename TagType, bool zigzag>
const char* TcParser::PackedVarint(PROTOBUF_TC_PARAM_DECL) {
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
  PROTOBUF_MUSTTAIL return PackedVarint<bool, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV8P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<bool, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV32P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<uint32_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV32P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<uint32_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV64P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<uint64_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastV64P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<uint64_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastZ32P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<int32_t, uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ32P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<int32_t, uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ64P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<int64_t, uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastZ64P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<int64_t, uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// Enum fields
//////////////////////////////////////////////////////////////////////////////

PROTOBUF_NOINLINE const char* TcParser::FastUnknownEnumFallback(
    PROTOBUF_TC_PARAM_DECL) {
  (void)msg;
  (void)ctx;
  (void)hasbits;

  // If we know we want to put this field directly into the unknown field set,
  // then we can skip the call to MiniParse and directly call table->fallback.
  // However, we first have to update `data` to contain the decoded tag.
  uint32_t tag;
  ptr = ReadTag(ptr, &tag);
  if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) {
    return Error(PROTOBUF_TC_PARAM_PASS);
  }
  data.data = tag;
  PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType, uint16_t xform_val>
PROTOBUF_ALWAYS_INLINE const char* TcParser::SingularEnum(
    PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
  }
  const char* ptr2 = ptr;  // Save for unknown enum case
  ptr += sizeof(TagType);  // Consume tag
  uint64_t tmp;
  ptr = ParseVarint(ptr, &tmp);
  if (ptr == nullptr) {
    return Error(PROTOBUF_TC_PARAM_PASS);
  }
  const TcParseTableBase::FieldAux aux = *table->field_aux(data.aux_idx());
  if (PROTOBUF_PREDICT_FALSE(
          !EnumIsValidAux(static_cast<int32_t>(tmp), xform_val, aux))) {
    ptr = ptr2;
    PROTOBUF_MUSTTAIL return FastUnknownEnumFallback(PROTOBUF_TC_PARAM_PASS);
  }
  hasbits |= (uint64_t{1} << data.hasbit_idx());
  RefAt<int32_t>(msg, data.offset()) = tmp;
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastErS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularEnum<uint8_t, field_layout::kTvRange>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastErS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularEnum<uint16_t, field_layout::kTvRange>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastEvS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularEnum<uint8_t, field_layout::kTvEnum>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastEvS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularEnum<uint16_t, field_layout::kTvEnum>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType, uint16_t xform_val>
PROTOBUF_ALWAYS_INLINE const char* TcParser::RepeatedEnum(
    PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    InvertPacked<WireFormatLite::WIRETYPE_VARINT>(data);
    if (data.coded_tag<TagType>() == 0) {
      // Packed parsing is handled by generated fallback.
      PROTOBUF_MUSTTAIL return FastUnknownEnumFallback(PROTOBUF_TC_PARAM_PASS);
    } else {
      PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
    }
  }
  auto& field = RefAt<RepeatedField<int32_t>>(msg, data.offset());
  auto expected_tag = UnalignedLoad<TagType>(ptr);
  const TcParseTableBase::FieldAux aux = *table->field_aux(data.aux_idx());
  do {
    const char* ptr2 = ptr;  // save for unknown enum case
    ptr += sizeof(TagType);
    uint64_t tmp;
    ptr = ParseVarint(ptr, &tmp);
    if (ptr == nullptr) {
      return Error(PROTOBUF_TC_PARAM_PASS);
    }
    if (PROTOBUF_PREDICT_FALSE(
            !EnumIsValidAux(static_cast<int32_t>(tmp), xform_val, aux))) {
      // We can avoid duplicate work in MiniParse by directly calling
      // table->fallback.
      ptr = ptr2;
      PROTOBUF_MUSTTAIL return FastUnknownEnumFallback(PROTOBUF_TC_PARAM_PASS);
    }
    field.Add(static_cast<int32_t>(tmp));
    if (!ctx->DataAvailable(ptr)) {
      break;
    }
  } while (UnalignedLoad<TagType>(ptr) == expected_tag);
  return ToParseLoop(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastErR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedEnum<uint8_t, field_layout::kTvRange>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastErR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedEnum<uint16_t, field_layout::kTvRange>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastEvR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedEnum<uint8_t, field_layout::kTvEnum>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastEvR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedEnum<uint16_t, field_layout::kTvEnum>(
      PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// String/bytes fields
//////////////////////////////////////////////////////////////////////////////

// Defined in wire_format_lite.cc
void PrintUTF8ErrorLog(StringPiece message_name,
                       StringPiece field_name, const char* operation_str,
                       bool emit_stacktrace);

void TcParser::ReportFastUtf8Error(uint32_t decoded_tag,
                                   const TcParseTableBase* table) {
  uint32_t field_num = decoded_tag >> 3;
  const auto* entry = FindFieldEntry(table, field_num);
  PrintUTF8ErrorLog(MessageName(table), FieldName(table, entry), "parsing",
                    false);
}

namespace {

PROTOBUF_NOINLINE
const char* SingularStringParserFallback(ArenaStringPtr* s, const char* ptr,
                                         EpsCopyInputStream* stream) {
  int size = ReadSize(&ptr);
  if (!ptr) return nullptr;
  return stream->ReadString(ptr, size, s->MutableNoCopy(nullptr));
}

}  // namespace

template <typename TagType, TcParser::Utf8Type utf8>
PROTOBUF_ALWAYS_INLINE const char* TcParser::SingularString(
    PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
  }
  auto saved_tag = UnalignedLoad<TagType>(ptr);
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
      ReportFastUtf8Error(FastDecodeTag(saved_tag), table);
      return utf8 == kUtf8 ? Error(PROTOBUF_TC_PARAM_PASS)
                           : ToParseLoop(PROTOBUF_TC_PARAM_PASS);
  }
}

const char* TcParser::FastBS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint8_t, kNoUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastBS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint16_t, kNoUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastSS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint8_t, kUtf8ValidateOnly>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastSS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint16_t, kUtf8ValidateOnly>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastUS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint8_t, kUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastUS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint16_t, kUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}

// Inlined string variants:

const char* TcParser::FastBiS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastBiS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastSiS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastSiS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastUiS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastUiS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType, TcParser::Utf8Type utf8>
PROTOBUF_ALWAYS_INLINE const char* TcParser::RepeatedString(
    PROTOBUF_TC_PARAM_DECL) {
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
    switch (utf8) {
      case kNoUtf8:
#ifdef NDEBUG
      case kUtf8ValidateOnly:
#endif
        break;
      default:
        if (PROTOBUF_PREDICT_TRUE(IsStructurallyValidUTF8(*str))) {
          break;
        }
        ReportFastUtf8Error(FastDecodeTag(expected_tag), table);
        if (utf8 == kUtf8) return Error(PROTOBUF_TC_PARAM_PASS);
        break;
    }
    if (!ctx->DataAvailable(ptr)) break;
  } while (UnalignedLoad<TagType>(ptr) == expected_tag);
  return ToParseLoop(PROTOBUF_TC_PARAM_PASS);
}

const char* TcParser::FastBR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedString<uint8_t, kNoUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastBR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedString<uint16_t, kNoUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastSR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedString<uint8_t, kUtf8ValidateOnly>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastSR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedString<uint16_t, kUtf8ValidateOnly>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastUR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedString<uint8_t, kUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}
const char* TcParser::FastUR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedString<uint16_t, kUtf8>(
      PROTOBUF_TC_PARAM_PASS);
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

// Destroys any existing oneof union member (if necessary). Returns true if the
// caller is responsible for initializing the object, or false if the field
// already has the desired case.
bool TcParser::ChangeOneof(const TcParseTableBase* table,
                           const TcParseTableBase::FieldEntry& entry,
                           uint32_t field_num, ParseContext* ctx,
                           MessageLite* msg) {
  // The _oneof_case_ array offset is stored in the first aux entry.
  uint32_t oneof_case_offset = table->field_aux(0u)->offset;
  // The _oneof_case_ array index is stored in the has-bit index.
  uint32_t* oneof_case =
      &TcParser::RefAt<uint32_t>(msg, oneof_case_offset) + entry.has_idx;
  uint32_t current_case = *oneof_case;
  *oneof_case = field_num;

  if (current_case == 0) {
    // If the member is empty, we don't have anything to clear. Caller is
    // responsible for creating a new member object.
    return true;
  }
  if (current_case == field_num) {
    // If the member is already active, then it should be merged. We're done.
    return false;
  }
  // Look up the value that is already stored, and dispose of it if necessary.
  const FieldEntry* current_entry = FindFieldEntry(table, current_case);
  uint16_t current_kind = current_entry->type_card & field_layout::kFkMask;
  uint16_t current_rep = current_entry->type_card & field_layout::kRepMask;
  if (current_kind == field_layout::kFkString) {
    switch (current_rep) {
      case field_layout::kRepAString: {
        auto& field = RefAt<ArenaStringPtr>(msg, current_entry->offset);
        field.Destroy();
        break;
      }
      case field_layout::kRepSString:
      case field_layout::kRepIString:
      default:
        GOOGLE_LOG(DFATAL) << "string rep not handled: "
                    << (current_rep >> field_layout::kRepShift);
        return true;
    }
  } else if (current_kind == field_layout::kFkMessage) {
    switch (current_rep) {
      case field_layout::kRepMessage:
      case field_layout::kRepGroup:
      case field_layout::kRepIWeak: {
        auto& field = RefAt<MessageLite*>(msg, current_entry->offset);
        if (!ctx->data().arena) {
          delete field;
        }
        break;
      }
      default:
        GOOGLE_LOG(DFATAL) << "message rep not handled: "
                    << (current_rep >> field_layout::kRepShift);
        break;
    }
  }
  return true;
}

const char* TcParser::MpFixed(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint16_t card = type_card & field_layout::kFcMask;

  // Check for repeated parsing (wiretype fallback is handled there):
  if (card == field_layout::kFcRepeated) {
    PROTOBUF_MUSTTAIL return MpRepeatedFixed(PROTOBUF_TC_PARAM_PASS);
  }
  // Check for mismatched wiretype:
  const uint16_t rep = type_card & field_layout::kRepMask;
  const uint32_t decoded_wiretype = data.tag() & 7;
  if (rep == field_layout::kRep64Bits) {
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED64) {
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
  } else {
    GOOGLE_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep32Bits));
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED32) {
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
  }
  // Set the field present:
  if (card == field_layout::kFcOptional) {
    SetHas(table, entry, msg, hasbits);
  } else if (card == field_layout::kFcOneof) {
    ChangeOneof(table, entry, data.tag() >> 3, ctx, msg);
  }
  // Copy the value:
  if (rep == field_layout::kRep64Bits) {
    RefAt<uint64_t>(msg, entry.offset) = UnalignedLoad<uint64_t>(ptr);
    ptr += sizeof(uint64_t);
  } else {
    RefAt<uint32_t>(msg, entry.offset) = UnalignedLoad<uint32_t>(ptr);
    ptr += sizeof(uint32_t);
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
      *field.Add() = UnalignedLoad<uint64_t>(ptr);
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
      *field.Add() = UnalignedLoad<uint32_t>(ptr);
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
  // Check for wire type mismatch:
  if ((data.tag() & 7) != WireFormatLite::WIRETYPE_VARINT) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }
  const uint16_t xform_val = type_card & field_layout::kTvMask;
  const bool is_zigzag = xform_val == field_layout::kTvZigZag;
  const bool is_validated_enum = xform_val & field_layout::kTvEnum;

  // Parse the value:
  const char* ptr2 = ptr;  // save for unknown enum case
  uint64_t tmp;
  ptr = ParseVarint(ptr, &tmp);
  if (ptr == nullptr) return Error(PROTOBUF_TC_PARAM_PASS);

  // Transform and/or validate the value
  uint16_t rep = type_card & field_layout::kRepMask;
  if (rep == field_layout::kRep64Bits) {
    if (is_zigzag) {
      tmp = WireFormatLite::ZigZagDecode64(tmp);
    }
  } else if (rep == field_layout::kRep32Bits) {
    if (is_validated_enum) {
      if (!EnumIsValidAux(tmp, xform_val, *table->field_aux(&entry))) {
        ptr = ptr2;
        PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
      }
    } else if (is_zigzag) {
      tmp = WireFormatLite::ZigZagDecode32(static_cast<uint32_t>(tmp));
    }
  }

  // Mark the field as present:
  const bool is_oneof = card == field_layout::kFcOneof;
  if (card == field_layout::kFcOptional) {
    SetHas(table, entry, msg, hasbits);
  } else if (is_oneof) {
    ChangeOneof(table, entry, data.tag() >> 3, ctx, msg);
  }

  if (rep == field_layout::kRep64Bits) {
    RefAt<uint64_t>(msg, entry.offset) = tmp;
  } else if (rep == field_layout::kRep32Bits) {
    RefAt<uint32_t>(msg, entry.offset) = static_cast<uint32_t>(tmp);
  } else {
    GOOGLE_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep8Bits));
    RefAt<bool>(msg, entry.offset) = static_cast<bool>(tmp);
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
  const bool is_zigzag = xform_val == field_layout::kTvZigZag;
  const bool is_validated_enum = xform_val & field_layout::kTvEnum;

  uint16_t rep = type_card & field_layout::kRepMask;
  if (rep == field_layout::kRep64Bits) {
    auto& field = RefAt<RepeatedField<uint64_t>>(msg, entry.offset);
    const char* ptr2 = ptr;
    uint32_t next_tag;
    do {
      uint64_t tmp;
      ptr = ParseVarint(ptr2, &tmp);
      if (ptr == nullptr) return Error(PROTOBUF_TC_PARAM_PASS);
      field.Add(is_zigzag ? WireFormatLite::ZigZagDecode64(tmp) : tmp);
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
      if (is_validated_enum) {
        if (!EnumIsValidAux(tmp, xform_val, *table->field_aux(&entry))) {
          ptr = ptr2;
          PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
        }
      } else if (is_zigzag) {
        tmp = WireFormatLite::ZigZagDecode32(tmp);
      }
      field.Add(tmp);
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
  const bool is_zigzag = xform_val == field_layout::kTvZigZag;
  const bool is_validated_enum = xform_val & field_layout::kTvEnum;
  if (is_validated_enum) {
    // TODO(b/206890171): handle enums
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }

  // Since ctx->ReadPackedFixed does not use TailCall<> or Return<>, sync any
  // pending hasbits now:
  SyncHasbits(msg, hasbits, table);

  uint16_t rep = type_card & field_layout::kRepMask;
  if (rep == field_layout::kRep64Bits) {
    auto* field = &RefAt<RepeatedField<uint64_t>>(msg, entry.offset);
    return ctx->ReadPackedVarint(ptr, [field, is_zigzag](uint64_t value) {
      field->Add(is_zigzag ? WireFormatLite::ZigZagDecode64(value) : value);
    });
  } else if (rep == field_layout::kRep32Bits) {
    auto* field = &RefAt<RepeatedField<uint32_t>>(msg, entry.offset);
    return ctx->ReadPackedVarint(ptr, [field, is_zigzag](uint64_t value) {
      field->Add(is_zigzag ? WireFormatLite::ZigZagDecode32(
                                 static_cast<uint32_t>(value))
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

bool TcParser::MpVerifyUtf8(StringPiece wire_bytes,
                            const TcParseTableBase* table,
                            const FieldEntry& entry, uint16_t xform_val) {
  if (xform_val == field_layout::kTvUtf8) {
    if (!IsStructurallyValidUTF8(wire_bytes)) {
      PrintUTF8ErrorLog(MessageName(table), FieldName(table, &entry), "parsing",
                        false);
      return false;
    }
    return true;
  }
#ifndef NDEBUG
  if (xform_val == field_layout::kTvUtf8Debug) {
    if (!IsStructurallyValidUTF8(wire_bytes)) {
      PrintUTF8ErrorLog(MessageName(table), FieldName(table, &entry), "parsing",
                        false);
    }
  }
#endif  // NDEBUG
  return true;
}

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
  const uint16_t xform_val = type_card & field_layout::kTvMask;
  const uint16_t rep = type_card & field_layout::kRepMask;
  if (rep == field_layout::kRepIString) {
    // TODO(b/198211897): support InilnedStringField.
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }

  // Mark the field as present:
  const bool is_oneof = card == field_layout::kFcOneof;
  bool need_init = false;
  if (card == field_layout::kFcOptional) {
    SetHas(table, entry, msg, hasbits);
  } else if (is_oneof) {
    need_init = ChangeOneof(table, entry, data.tag() >> 3, ctx, msg);
  }

  bool is_valid = false;
  Arena* arena = ctx->data().arena;
  switch (rep) {
    case field_layout::kRepAString: {
      auto& field = RefAt<ArenaStringPtr>(msg, entry.offset);
      if (need_init) field.InitDefault();
      if (arena) {
        ptr = ctx->ReadArenaString(ptr, &field, arena);
      } else {
        std::string* str = field.MutableNoCopy(nullptr);
        ptr = InlineGreedyStringParser(str, ptr, ctx);
      }
      if (!ptr) break;
      is_valid = MpVerifyUtf8(field.Get(), table, entry, xform_val);
      break;
    }

    case field_layout::kRepIString: {
      break;
    }
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
  switch (rep) {
    case field_layout::kRepSString: {
      auto& field = RefAt<RepeatedPtrField<std::string>>(msg, entry.offset);
      const char* ptr2 = ptr;
      uint32_t next_tag;
      do {
        ptr = ptr2;
        std::string* str = field.Add();
        ptr = InlineGreedyStringParser(str, ptr, ctx);
        if (PROTOBUF_PREDICT_FALSE(
                ptr == nullptr ||
                !MpVerifyUtf8(*str, table, entry, xform_val))) {
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

  const uint32_t decoded_tag = data.tag();
  const uint32_t decoded_wiretype = decoded_tag & 7;
  const uint16_t rep = type_card & field_layout::kRepMask;
  const bool is_group = rep == field_layout::kRepGroup;

  // Validate wiretype:
  switch (rep) {
    case field_layout::kRepMessage:
      if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
        goto fallback;
      }
      break;
    case field_layout::kRepGroup:
      if (decoded_wiretype != WireFormatLite::WIRETYPE_START_GROUP) {
        goto fallback;
      }
      break;
    default: {
    fallback:
      // Lazy and implicit weak fields are handled by generated code:
      // TODO(b/210762816): support these.
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
  }

  const bool is_oneof = card == field_layout::kFcOneof;
  bool need_init = false;
  if (card == field_layout::kFcOptional) {
    SetHas(table, entry, msg, hasbits);
  } else if (is_oneof) {
    need_init = ChangeOneof(table, entry, data.tag() >> 3, ctx, msg);
  }
  MessageLite*& field = RefAt<MessageLite*>(msg, entry.offset);
  if (need_init || field == nullptr) {
    const MessageLite* default_instance =
        table->field_aux(&entry)->message_default;
    field = default_instance->New(ctx->data().arena);
  }
  SyncHasbits(msg, hasbits, table);
  if (is_group) {
    return ctx->ParseGroup(field, ptr, decoded_tag);
  }
  return ctx->ParseMessage(field, ptr);
}

const char* TcParser::MpRepeatedMessage(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  GOOGLE_DCHECK_EQ(type_card & field_layout::kFcMask,
            static_cast<uint16_t>(field_layout::kFcRepeated));
  const uint32_t decoded_tag = data.tag();
  const uint32_t decoded_wiretype = decoded_tag & 7;
  const uint16_t rep = type_card & field_layout::kRepMask;
  const bool is_group = rep == field_layout::kRepGroup;

  // Validate wiretype:
  switch (rep) {
    case field_layout::kRepMessage:
      if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
        goto fallback;
      }
      break;
    case field_layout::kRepGroup:
      if (decoded_wiretype != WireFormatLite::WIRETYPE_START_GROUP) {
        goto fallback;
      }
      break;
    default: {
    fallback:
      // Lazy and implicit weak fields are handled by generated code:
      // TODO(b/210762816): support these.
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
  }

  SyncHasbits(msg, hasbits, table);
  const MessageLite* default_instance =
      table->field_aux(&entry)->message_default;
  auto& field = RefAt<RepeatedPtrFieldBase>(msg, entry.offset);
  MessageLite* value =
      field.Add<GenericTypeHandler<MessageLite>>(default_instance);
  if (is_group) {
    return ctx->ParseGroup(value, ptr, decoded_tag);
  }
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
