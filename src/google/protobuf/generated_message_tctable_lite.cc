// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/numeric/bits.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/inlined_string_field.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/map.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/varint_shuffle.h"
#include "google/protobuf/wire_format_lite.h"
#include "utf8_validity.h"

#ifdef __x86_64__
#include "x86intrin.h"
#endif

// clang-format off
#include "google/protobuf/port_def.inc"
// clang-format on

namespace google {
namespace protobuf {
namespace internal {

using FieldEntry = TcParseTableBase::FieldEntry;

//////////////////////////////////////////////////////////////////////////////
// Template instantiations:
//////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
void AlignFail(std::integral_constant<size_t, 4>, std::uintptr_t address) {
  ABSL_LOG(FATAL) << "Unaligned (4) access at " << address;

  // Explicit abort to let compilers know this function does not return
  abort();
}
void AlignFail(std::integral_constant<size_t, 8>, std::uintptr_t address) {
  ABSL_LOG(FATAL) << "Unaligned (8) access at " << address;

  // Explicit abort to let compilers know this function does not return
  abort();
}
#endif

const char* TcParser::GenericFallbackLite(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  return GenericFallbackImpl<MessageLite, std::string>(
      msg, ptr, ctx, table, data);
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
    adj_fnum -= absl::popcount(skipmap);
    auto* entry = field_entries + adj_fnum;
    PROTOBUF_ASSUME(entry != nullptr);
    return entry;
  }
  const uint16_t* lookup_table = table->field_lookup_begin();
  for (;;) {
#ifdef ABSL_IS_LITTLE_ENDIAN
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
      adj_fnum -= absl::popcount(skipmap);
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
static absl::string_view FindName(const char* name_data, size_t entries,
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

absl::string_view TcParser::MessageName(const TcParseTableBase* table) {
  return FindName(table->name_data(), table->num_field_entries + 1, 0);
}

absl::string_view TcParser::FieldName(const TcParseTableBase* table,
                                      const FieldEntry* field_entry) {
  const FieldEntry* const field_entries = table->field_entries_begin();
  auto field_index = static_cast<size_t>(field_entry - field_entries);
  return FindName(table->name_data(), table->num_field_entries + 1,
                  field_index + 1);
}

PROTOBUF_NOINLINE const char* Error(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  (void)ctx;
  (void)ptr;
  return nullptr;
}

template <bool export_called_function>
inline PROTOBUF_ALWAYS_INLINE const char* TcParser::MiniParse(
    MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  TestMiniParseResult* test_out;
  if (export_called_function) {
    test_out = reinterpret_cast<TestMiniParseResult*>(
        static_cast<uintptr_t>(data.data));
  }

  uint32_t tag;
  ptr = ReadTagInlined(ptr, &tag);
  if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) {
    if (export_called_function) *test_out = {Error};
    return nullptr;
  }

  auto* entry = FindFieldEntry(table, tag >> 3);
  if (entry == nullptr) {
    if (export_called_function) *test_out = {table->fallback, tag};
    data.data = tag;
    return table->fallback(msg, ptr, ctx, table, data);
  }

  // The handler may need the tag and the entry to resolve fallback logic. Both
  // of these are 32 bits, so pack them into (the 64-bit) `data`. Since we can't
  // pack the entry pointer itself, just pack its offset from `table`.
  uint64_t entry_offset = reinterpret_cast<const char*>(entry) -
                          reinterpret_cast<const char*>(table);
  data.data = entry_offset << 32 | tag;

  using field_layout::FieldKind;
  auto field_type =
      entry->type_card & (+field_layout::kSplitMask | FieldKind::kFkMask);

  static constexpr TailCallParseFunc kMiniParseTable[] = {
      &MpFallback,             // FieldKind::kFkNone
      &MpVarint<false>,        // FieldKind::kFkVarint
      &MpPackedVarint<false>,  // FieldKind::kFkPackedVarint
      &MpFixed<false>,         // FieldKind::kFkFixed
      &MpPackedFixed<false>,   // FieldKind::kFkPackedFixed
      &MpString<false>,        // FieldKind::kFkString
      &MpMessage<false>,       // FieldKind::kFkMessage
      &MpMap<false>,           // FieldKind::kFkMap
      &Error,                  // kSplitMask | FieldKind::kFkNone
      &MpVarint<true>,         // kSplitMask | FieldKind::kFkVarint
      &MpPackedVarint<true>,   // kSplitMask | FieldKind::kFkPackedVarint
      &MpFixed<true>,          // kSplitMask | FieldKind::kFkFixed
      &MpPackedFixed<true>,    // kSplitMask | FieldKind::kFkPackedFixed
      &MpString<true>,         // kSplitMask | FieldKind::kFkString
      &MpMessage<true>,        // kSplitMask | FieldKind::kFkMessage
      &MpMap<true>,            // kSplitMask | FieldKind::kFkMap
  };
  // Just to be sure we got the order right, above.
  static_assert(0 == FieldKind::kFkNone, "Invalid table order");
  static_assert(1 == FieldKind::kFkVarint, "Invalid table order");
  static_assert(2 == FieldKind::kFkPackedVarint, "Invalid table order");
  static_assert(3 == FieldKind::kFkFixed, "Invalid table order");
  static_assert(4 == FieldKind::kFkPackedFixed, "Invalid table order");
  static_assert(5 == FieldKind::kFkString, "Invalid table order");
  static_assert(6 == FieldKind::kFkMessage, "Invalid table order");
  static_assert(7 == FieldKind::kFkMap, "Invalid table order");

  static_assert(8 == (+field_layout::kSplitMask | FieldKind::kFkNone),
    "Invalid table order");
  static_assert(9 == (+field_layout::kSplitMask | FieldKind::kFkVarint),
    "Invalid table order");
  static_assert(10 == (+field_layout::kSplitMask | FieldKind::kFkPackedVarint),
    "Invalid table order");
  static_assert(11 == (+field_layout::kSplitMask | FieldKind::kFkFixed),
    "Invalid table order");
  static_assert(12 == (+field_layout::kSplitMask | FieldKind::kFkPackedFixed),
    "Invalid table order");
  static_assert(13 == (+field_layout::kSplitMask | FieldKind::kFkString),
    "Invalid table order");
  static_assert(14 == (+field_layout::kSplitMask | FieldKind::kFkMessage),
    "Invalid table order");
  static_assert(15 == (+field_layout::kSplitMask | FieldKind::kFkMap),
    "Invalid table order");

  TailCallParseFunc parse_fn = kMiniParseTable[field_type];
  if (export_called_function) *test_out = {parse_fn, tag, entry};

  return parse_fn(msg, ptr, ctx, table, data);
}

PROTOBUF_NOINLINE const char* TcParser::MiniParse(
    MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  return MiniParse<false>(msg, ptr, ctx, table, data);
}
PROTOBUF_NOINLINE TcParser::TestMiniParseResult TcParser::TestMiniParse(
    MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  TestMiniParseResult result = {};
  data.data = reinterpret_cast<uintptr_t>(&result);
  result.ptr = MiniParse<true>(msg, ptr, ctx, table, data);
  return result;
}

PROTOBUF_NOINLINE const char* TcParser::MpFallback(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  return table->fallback(msg, ptr, ctx, table, data);
}

//////////////////////////////////////////////////////////////////////////////
// Varint fields
//////////////////////////////////////////////////////////////////////////////

namespace {

// Prefetch the enum data, if necessary.
// We can issue the prefetch before we start parsing the ints.
PROTOBUF_ALWAYS_INLINE void PrefetchEnumData(uint16_t xform_val,
                                             TcParseTableBase::FieldAux aux) {
}

// When `xform_val` is a constant, we want to inline `ValidateEnum` because it
// is either dropped when not a kTvEnum, or useful when it is.
//
// When it is not a constant, we do not inline `ValidateEnum` because it bloats
// the code around it and pessimizes the non-enum and kTvRange cases which are
// way more common than the kTvEnum cases. It is also called from places that
// already have out-of-line functions (like MpVarint) so an extra out-of-line
// call to `ValidateEnum` does not affect much.
PROTOBUF_ALWAYS_INLINE bool EnumIsValidAux(int32_t val, uint16_t xform_val,
                                           TcParseTableBase::FieldAux aux) {
  if (xform_val == field_layout::kTvRange) {
    auto lo = aux.enum_range.start;
    return lo <= val && val < (lo + aux.enum_range.length);
  }
  if (PROTOBUF_BUILTIN_CONSTANT_P(xform_val)) {
    return internal::ValidateEnumInlined(val, aux.enum_data);
  } else {
    return internal::ValidateEnum(val, aux.enum_data);
  }
}

}  // namespace


PROTOBUF_NOINLINE const char* TcParser::MpUnknownEnumFallback(
    MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  // Like FastUnknownEnumFallback, but with the Mp ABI.
  uint32_t tag = data.tag();
  uint64_t tmp;
  ptr = VarintParse(ptr, &tmp);
  if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) {
    return nullptr;
  }
  AddUnknownEnum(msg, table, tag, static_cast<int32_t>(tmp));
  return ptr;
}

const TcParser::UnknownFieldOps& TcParser::GetUnknownFieldOps(
    const TcParseTableBase* table) {
  // Call the fallback function in a special mode to only act as a
  // way to return the ops.
  // Hiding the unknown fields vtable behind the fallback function avoids adding
  // more pointers in TcParseTableBase, and the extra runtime jumps are not
  // relevant because unknown fields are rare.
  const char* ptr = table->fallback(nullptr, nullptr, nullptr, nullptr, {});
  return *reinterpret_cast<const UnknownFieldOps*>(ptr);
}

PROTOBUF_NOINLINE void TcParser::AddUnknownEnum(MessageLite* msg,
                                                const TcParseTableBase* table,
                                                uint32_t tag,
                                                int32_t enum_value) {
  GetUnknownFieldOps(table).write_varint(msg, tag >> 3, enum_value);
}

//////////////////////////////////////////////////////////////////////////////
// String/bytes fields
//////////////////////////////////////////////////////////////////////////////

// Defined in wire_format_lite.cc
void PrintUTF8ErrorLog(absl::string_view message_name,
                       absl::string_view field_name, const char* operation_str,
                       bool emit_stacktrace);

ABSL_ATTRIBUTE_NOINLINE
void TcParser::ReportFastUtf8Error(uint32_t decoded_tag,
                                   const TcParseTableBase* table) {
  uint32_t field_num = decoded_tag >> 3;
  const auto* entry = FindFieldEntry(table, field_num);
  PrintUTF8ErrorLog(MessageName(table), FieldName(table, entry), "parsing",
                    false);
}


namespace {
inline void SetHas(const FieldEntry& entry, MessageLite* msg) {
  auto has_idx = static_cast<uint32_t>(entry.has_idx);
#if defined(__x86_64__) && defined(__GNUC__)
  asm("bts %1, %0\n" : "+m"(*msg) : "r"(has_idx));
#else
  auto& hasblock = TcParser::RefAt<uint32_t>(msg, has_idx / 32 * 4);
  hasblock |= uint32_t{1} << (has_idx % 32);
#endif
}
}  // namespace

// Destroys any existing oneof union member (if necessary). Returns true if the
// caller is responsible for initializing the object, or false if the field
// already has the desired case.
bool TcParser::ChangeOneof(const TcParseTableBase* table,
                           const TcParseTableBase::FieldEntry entry,
                           uint32_t field_num, ParseContext* ctx,
                           MessageLite* msg) {
  // The _oneof_case_ value offset is stored in the has-bit index.
  uint32_t* oneof_case = &TcParser::RefAt<uint32_t>(msg, entry.has_idx);
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
        ABSL_DLOG(FATAL) << "string rep not handled: "
                         << (current_rep >> field_layout::kRepShift);
        return true;
    }
  } else if (current_kind == field_layout::kFkMessage) {
    switch (current_rep) {
      case field_layout::kRepMessage:
      case field_layout::kRepGroup: {
        auto& field = RefAt<MessageLite*>(msg, current_entry->offset);
        if (!msg->GetArena()) {
          delete field;
        }
        break;
      }
      default:
        ABSL_DLOG(FATAL) << "message rep not handled: "
                         << (current_rep >> field_layout::kRepShift);
        break;
    }
  }
  return true;
}

namespace {
uint32_t GetSplitOffset(const TcParseTableBase* table) {
  return table->field_aux(kSplitOffsetAuxIdx)->offset;
}

uint32_t GetSizeofSplit(const TcParseTableBase* table) {
  return table->field_aux(kSplitSizeAuxIdx)->offset;
}
}  // namespace

void* TcParser::MaybeGetSplitBase(MessageLite* msg, const bool is_split,
                                  const TcParseTableBase* table) {
  void* out = msg;
  if (is_split) {
    const uint32_t split_offset = GetSplitOffset(table);
    void* default_split =
        TcParser::RefAt<void*>(table->default_instance, split_offset);
    void*& split = TcParser::RefAt<void*>(msg, split_offset);
    if (split == default_split) {
      // Allocate split instance when needed.
      uint32_t size = GetSizeofSplit(table);
      Arena* arena = msg->GetArena();
      split = (arena == nullptr) ? ::operator new(size)
                                 : arena->AllocateAligned(size);
      memcpy(split, default_split, size);
    }
    out = split;
  }
  return out;
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpFixed(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint16_t card = type_card & field_layout::kFcMask;

  // Check for repeated parsing (wiretype fallback is handled there):
  if (card == field_layout::kFcRepeated) {
    return MpRepeatedFixed<is_split>(msg, ptr, ctx, table, data);
  }
  // Check for mismatched wiretype:
  const uint16_t rep = type_card & field_layout::kRepMask;
  const uint32_t decoded_wiretype = data.tag() & 7;
  if (rep == field_layout::kRep64Bits) {
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED64) {
      return table->fallback(msg, ptr, ctx, table, data);
    }
  } else {
    ABSL_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep32Bits));
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED32) {
      return table->fallback(msg, ptr, ctx, table, data);
    }
  }
  // Set the field present:
  if (card == field_layout::kFcOptional) {
    SetHas(entry, msg);
  } else if (card == field_layout::kFcOneof) {
    ChangeOneof(table, entry, data.tag() >> 3, ctx, msg);
  }
  void* const base = MaybeGetSplitBase(msg, is_split, table);
  // Copy the value:
  if (rep == field_layout::kRep64Bits) {
    RefAt<uint64_t>(base, entry.offset) = UnalignedLoad<uint64_t>(ptr);
    ptr += sizeof(uint64_t);
  } else {
    RefAt<uint32_t>(base, entry.offset) = UnalignedLoad<uint32_t>(ptr);
    ptr += sizeof(uint32_t);
  }
  return ptr;
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpRepeatedFixed(
    MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint32_t decoded_tag = data.tag();
  const uint32_t decoded_wiretype = decoded_tag & 7;

  // Check for packed repeated fallback:
  if (decoded_wiretype == WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    return MpPackedFixed<is_split>(msg, ptr, ctx, table, data);
  }

  void* const base = MaybeGetSplitBase(msg, is_split, table);
  const uint16_t type_card = entry.type_card;
  const uint16_t rep = type_card & field_layout::kRepMask;
  if (rep == field_layout::kRep64Bits) {
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED64) {
      return table->fallback(msg, ptr, ctx, table, data);
    }
    auto& field = MaybeCreateRepeatedFieldRefAt<uint64_t, is_split>(
        base, entry.offset, msg);
    constexpr auto size = sizeof(uint64_t);
    const char* ptr2 = ptr;
    uint32_t next_tag;
    do {
      ptr = ptr2;
      *field.Add() = UnalignedLoad<uint64_t>(ptr);
      ptr += size;
      if (PROTOBUF_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
      ptr2 = ReadTag(ptr, &next_tag);
      if (PROTOBUF_PREDICT_FALSE(ptr2 == nullptr)) goto error;
    } while (next_tag == decoded_tag);
  } else {
    ABSL_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep32Bits));
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED32) {
      return table->fallback(msg, ptr, ctx, table, data);
    }
    auto& field = MaybeCreateRepeatedFieldRefAt<uint32_t, is_split>(
        base, entry.offset, msg);
    constexpr auto size = sizeof(uint32_t);
    const char* ptr2 = ptr;
    uint32_t next_tag;
    do {
      ptr = ptr2;
      *field.Add() = UnalignedLoad<uint32_t>(ptr);
      ptr += size;
      if (PROTOBUF_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
      ptr2 = ReadTag(ptr, &next_tag);
      if (PROTOBUF_PREDICT_FALSE(ptr2 == nullptr)) goto error;
    } while (next_tag == decoded_tag);
  }

  return ptr;
parse_loop:
  return ptr;
error:
  return nullptr;
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpPackedFixed(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint32_t decoded_wiretype = data.tag() & 7;

  // Check for non-packed repeated fallback:
  if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    return MpRepeatedFixed<is_split>(msg, ptr, ctx, table, data);
  }

  void* const base = MaybeGetSplitBase(msg, is_split, table);
  int size = ReadSize(&ptr);
  uint16_t rep = type_card & field_layout::kRepMask;
  if (rep == field_layout::kRep64Bits) {
    auto& field = MaybeCreateRepeatedFieldRefAt<uint64_t, is_split>(
        base, entry.offset, msg);
    ptr = ctx->ReadPackedFixed(ptr, size, &field);
  } else {
    ABSL_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep32Bits));
    auto& field = MaybeCreateRepeatedFieldRefAt<uint32_t, is_split>(
        base, entry.offset, msg);
    ptr = ctx->ReadPackedFixed(ptr, size, &field);
  }

  if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) {
    return nullptr;
  }
  return ptr;
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpVarint(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint16_t card = type_card & field_layout::kFcMask;

  // Check for repeated parsing:
  if (card == field_layout::kFcRepeated) {
    return MpRepeatedVarint<is_split>(msg, ptr, ctx, table, data);
  }
  // Check for wire type mismatch:
  if ((data.tag() & 7) != WireFormatLite::WIRETYPE_VARINT) {
    return table->fallback(msg, ptr, ctx, table, data);
  }
  const uint16_t xform_val = type_card & field_layout::kTvMask;
  const bool is_zigzag = xform_val == field_layout::kTvZigZag;
  const bool is_validated_enum = xform_val & field_layout::kTvEnum;

  // Parse the value:
  const char* ptr2 = ptr;  // save for unknown enum case
  uint64_t tmp;
  ptr = VarintParse(ptr, &tmp);
  if (ptr == nullptr) {
    return nullptr;
  }

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
        return MpUnknownEnumFallback(msg, ptr, ctx, table, data);
      }
    } else if (is_zigzag) {
      tmp = WireFormatLite::ZigZagDecode32(static_cast<uint32_t>(tmp));
    }
  }

  // Mark the field as present:
  const bool is_oneof = card == field_layout::kFcOneof;
  if (card == field_layout::kFcOptional) {
    SetHas(entry, msg);
  } else if (is_oneof) {
    ChangeOneof(table, entry, data.tag() >> 3, ctx, msg);
  }

  void* const base = MaybeGetSplitBase(msg, is_split, table);
  if (rep == field_layout::kRep64Bits) {
    RefAt<uint64_t>(base, entry.offset) = tmp;
  } else if (rep == field_layout::kRep32Bits) {
    RefAt<uint32_t>(base, entry.offset) = static_cast<uint32_t>(tmp);
  } else {
    ABSL_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep8Bits));
    RefAt<bool>(base, entry.offset) = static_cast<bool>(tmp);
  }

  return ptr;
}

template <bool is_split, typename FieldType, uint16_t xform_val_in>
const char* TcParser::MpRepeatedVarintT(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint32_t decoded_tag = data.tag();
  // For is_split we ignore the incoming xform_val and read it from entry to
  // reduce duplication for the uncommon paths.
  const uint16_t xform_val =
      is_split ? (entry.type_card & field_layout::kTvMask) : xform_val_in;
  const bool is_zigzag = xform_val == field_layout::kTvZigZag;
  const bool is_validated_enum = xform_val & field_layout::kTvEnum;

  const char* ptr2 = ptr;
  uint32_t next_tag;
  void* const base = MaybeGetSplitBase(msg, is_split, table);
  auto& field = MaybeCreateRepeatedFieldRefAt<FieldType, is_split>(
      base, entry.offset, msg);

  TcParseTableBase::FieldAux aux;
  if (is_validated_enum) {
    aux = *table->field_aux(&entry);
    PrefetchEnumData(xform_val, aux);
  }

  do {
    uint64_t tmp;
    ptr = VarintParse(ptr2, &tmp);
    if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) goto error;
    if (is_validated_enum) {
      if (!EnumIsValidAux(static_cast<int32_t>(tmp), xform_val, aux)) {
        ptr = ptr2;
        return MpUnknownEnumFallback(msg, ptr, ctx, table, data);
      }
    } else if (is_zigzag) {
      tmp = sizeof(FieldType) == 8 ? WireFormatLite::ZigZagDecode64(tmp)
                                   : WireFormatLite::ZigZagDecode32(tmp);
    }
    field.Add(static_cast<FieldType>(tmp));
    if (PROTOBUF_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
    ptr2 = ReadTag(ptr, &next_tag);
    if (PROTOBUF_PREDICT_FALSE(ptr2 == nullptr)) goto error;
  } while (next_tag == decoded_tag);

parse_loop:
  return ptr;
error:
  return nullptr;
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpRepeatedVarint(
    MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const auto type_card = entry.type_card;
  const uint32_t decoded_tag = data.tag();
  const auto decoded_wiretype = decoded_tag & 7;

  // Check for packed repeated fallback:
  if (decoded_wiretype == WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    return MpPackedVarint<is_split>(msg, ptr, ctx, table, data);
  }
  // Check for wire type mismatch:
  if (decoded_wiretype != WireFormatLite::WIRETYPE_VARINT) {
    return table->fallback(msg, ptr, ctx, table, data);
  }
  // For split we avoid the duplicate code and have the impl reload the value.
  // Less code bloat for uncommon paths.
  const uint16_t xform_val = (type_card & field_layout::kTvMask);
  const uint16_t rep = type_card & field_layout::kRepMask;
  switch (rep >> field_layout::kRepShift) {
    case field_layout::kRep64Bits >> field_layout::kRepShift:
      if (xform_val == 0) {
        return MpRepeatedVarintT<is_split, uint64_t, 0>(
            msg, ptr, ctx, table, data);
      } else {
        ABSL_DCHECK_EQ(xform_val, +field_layout::kTvZigZag);
        return MpRepeatedVarintT<
            is_split, uint64_t, (is_split ? 0 : field_layout::kTvZigZag)>(
            msg, ptr, ctx, table, data);
      }
    case field_layout::kRep32Bits >> field_layout::kRepShift:
      switch (xform_val >> field_layout::kTvShift) {
        case 0:
          return MpRepeatedVarintT<is_split, uint32_t, 0>(
              msg, ptr, ctx, table, data);
        case field_layout::kTvZigZag >> field_layout::kTvShift:
          return MpRepeatedVarintT<
              is_split, uint32_t, (is_split ? 0 : field_layout::kTvZigZag)>(
              msg, ptr, ctx, table, data);
        case field_layout::kTvEnum >> field_layout::kTvShift:
          return MpRepeatedVarintT<
              is_split, uint32_t, (is_split ? 0 : field_layout::kTvEnum)>(
              msg, ptr, ctx, table, data);
        case field_layout::kTvRange >> field_layout::kTvShift:
          return MpRepeatedVarintT<
              is_split, uint32_t, (is_split ? 0 : field_layout::kTvRange)>(
              msg, ptr, ctx, table, data);
        default:
          Unreachable();
      }
    case field_layout::kRep8Bits >> field_layout::kRepShift:
      return MpRepeatedVarintT<is_split, bool, 0>(
          msg, ptr, ctx, table, data);

    default:
      Unreachable();
      return nullptr;  // To silence -Werror=return-type in some toolchains
  }
}

template <bool is_split, typename FieldType, uint16_t xform_val_in>
const char* TcParser::MpPackedVarintT(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  // For is_split we ignore the incoming xform_val and read it from entry to
  // reduce duplication for the uncommon paths.
  const uint16_t xform_val =
      is_split ? (entry.type_card & field_layout::kTvMask) : xform_val_in;
  const bool is_zigzag = xform_val == field_layout::kTvZigZag;
  const bool is_validated_enum = xform_val & field_layout::kTvEnum;

  void* const base = MaybeGetSplitBase(msg, is_split, table);
  auto* field = &MaybeCreateRepeatedFieldRefAt<FieldType, is_split>(
      base, entry.offset, msg);

  if (is_validated_enum) {
    const TcParseTableBase::FieldAux aux = *table->field_aux(entry.aux_idx);
    PrefetchEnumData(xform_val, aux);
    return ctx->ReadPackedVarint(ptr, [=](int32_t value) {
      if (!EnumIsValidAux(value, xform_val, aux)) {
        AddUnknownEnum(msg, table, data.tag(), value);
      } else {
        field->Add(value);
      }
    });
  } else {
    return ctx->ReadPackedVarint(ptr, [=](uint64_t value) {
      field->Add(is_zigzag ? (sizeof(FieldType) == 8
                                  ? WireFormatLite::ZigZagDecode64(value)
                                  : WireFormatLite::ZigZagDecode32(
                                        static_cast<uint32_t>(value)))
                           : value);
    });
  }
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpPackedVarint(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const auto type_card = entry.type_card;
  const auto decoded_wiretype = data.tag() & 7;

  // Check for non-packed repeated fallback:
  if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    return MpRepeatedVarint<is_split>(msg, ptr, ctx, table, data);
  }

  // For split we avoid the duplicate code and have the impl reload the value.
  // Less code bloat for uncommon paths.
  const uint16_t xform_val = (type_card & field_layout::kTvMask);

  const uint16_t rep = type_card & field_layout::kRepMask;

  switch (rep >> field_layout::kRepShift) {
    case field_layout::kRep64Bits >> field_layout::kRepShift:
      if (xform_val == 0) {
        return MpPackedVarintT<is_split, uint64_t, 0>(
            msg, ptr, ctx, table, data);
      } else {
        ABSL_DCHECK_EQ(xform_val, +field_layout::kTvZigZag);
        return MpPackedVarintT<
            is_split, uint64_t, (is_split ? 0 : field_layout::kTvZigZag)>(
            msg, ptr, ctx, table, data);
      }
    case field_layout::kRep32Bits >> field_layout::kRepShift:
      switch (xform_val >> field_layout::kTvShift) {
        case 0:
          return MpPackedVarintT<is_split, uint32_t, 0>(
              msg, ptr, ctx, table, data);
        case field_layout::kTvZigZag >> field_layout::kTvShift:
          return MpPackedVarintT<
              is_split, uint32_t, (is_split ? 0 : field_layout::kTvZigZag)>(
              msg, ptr, ctx, table, data);
        case field_layout::kTvEnum >> field_layout::kTvShift:
          return MpPackedVarintT<
              is_split, uint32_t, (is_split ? 0 : field_layout::kTvEnum)>(
              msg, ptr, ctx, table, data);
        case field_layout::kTvRange >> field_layout::kTvShift:
          return MpPackedVarintT<
              is_split, uint32_t, (is_split ? 0 : field_layout::kTvRange)>(
              msg, ptr, ctx, table, data);
        default:
          Unreachable();
      }
    case field_layout::kRep8Bits >> field_layout::kRepShift:
      return MpPackedVarintT<is_split, bool, 0>(
          msg, ptr, ctx, table, data);

    default:
      Unreachable();
      return nullptr;  // To silence -Werror=return-type in some toolchains
  }
}

bool TcParser::MpVerifyUtf8(absl::string_view wire_bytes,
                            const TcParseTableBase* table,
                            const FieldEntry& entry, uint16_t xform_val) {
  if (xform_val == field_layout::kTvUtf8) {
    if (!utf8_range::IsStructurallyValid(wire_bytes)) {
      PrintUTF8ErrorLog(MessageName(table), FieldName(table, &entry), "parsing",
                        false);
      return false;
    }
    return true;
  }
#ifndef NDEBUG
  if (xform_val == field_layout::kTvUtf8Debug) {
    if (!utf8_range::IsStructurallyValid(wire_bytes)) {
      PrintUTF8ErrorLog(MessageName(table), FieldName(table, &entry), "parsing",
                        false);
    }
  }
#endif  // NDEBUG
  return true;
}
bool TcParser::MpVerifyUtf8(const absl::Cord& wire_bytes,
                            const TcParseTableBase* table,
                            const FieldEntry& entry, uint16_t xform_val) {
  switch (xform_val) {
    default:
      ABSL_DCHECK_EQ(xform_val, 0);
      return true;
  }
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpString(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint16_t card = type_card & field_layout::kFcMask;
  const uint32_t decoded_wiretype = data.tag() & 7;

  if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    return table->fallback(msg, ptr, ctx, table, data);
  }
  if (card == field_layout::kFcRepeated) {
    return MpRepeatedString<is_split>(msg, ptr, ctx, table, data);
  }
  const uint16_t xform_val = type_card & field_layout::kTvMask;
  const uint16_t rep = type_card & field_layout::kRepMask;

  // Mark the field as present:
  const bool is_oneof = card == field_layout::kFcOneof;
  bool need_init = false;
  if (card == field_layout::kFcOptional) {
    SetHas(entry, msg);
  } else if (is_oneof) {
    need_init = ChangeOneof(table, entry, data.tag() >> 3, ctx, msg);
  }

  bool is_valid = false;
  void* const base = MaybeGetSplitBase(msg, is_split, table);
  switch (rep) {
    case field_layout::kRepAString: {
      auto& field = RefAt<ArenaStringPtr>(base, entry.offset);
      if (need_init) field.InitDefault();
      Arena* arena = msg->GetArena();
      if (arena) {
        int sz = ReadSize(&ptr);
        if (ptr == nullptr) return ptr;
        ptr = ctx->ReadArenaString(ptr, sz, &field, arena);
      } else {
        std::string* str = field.MutableNoCopy(nullptr);
        ptr = InlineGreedyStringParser(str, ptr, ctx);
      }
      if (!ptr) break;
      is_valid = MpVerifyUtf8(field.Get(), table, entry, xform_val);
      break;
    }


    case field_layout::kRepCord: {
      absl::Cord* field;
      if (is_oneof) {
        if (need_init) {
          field = Arena::Create<absl::Cord>(msg->GetArena());
          RefAt<absl::Cord*>(msg, entry.offset) = field;
        } else {
          field = RefAt<absl::Cord*>(msg, entry.offset);
        }
      } else {
        field = &RefAt<absl::Cord>(base, entry.offset);
      }
      ptr = InlineCordParser(field, ptr, ctx);
      if (!ptr) break;
      is_valid = MpVerifyUtf8(*field, table, entry, xform_val);
      break;
    }

    default:
      Unreachable();
  }

  if (PROTOBUF_PREDICT_FALSE(ptr == nullptr || !is_valid)) {
    return nullptr;
  }
  return ptr;
}

PROTOBUF_ALWAYS_INLINE const char* TcParser::ParseRepeatedStringOnce(
    const char* ptr, SerialArena* serial_arena, ParseContext* ctx,
    RepeatedPtrField<std::string>& field) {
  int size = ReadSize(&ptr);
  if (PROTOBUF_PREDICT_FALSE(!ptr)) return {};
  auto* str = new (serial_arena->AllocateFromStringBlock()) std::string();
  field.AddAllocatedForParse(str);
  ptr = ctx->ReadString(ptr, size, str);
  if (PROTOBUF_PREDICT_FALSE(!ptr)) return {};
  PROTOBUF_ASSUME(ptr != nullptr);
  return ptr;
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpRepeatedString(
    MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint32_t decoded_tag = data.tag();
  const uint32_t decoded_wiretype = decoded_tag & 7;

  if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    return table->fallback(msg, ptr, ctx, table, data);
  }

  const uint16_t rep = type_card & field_layout::kRepMask;
  const uint16_t xform_val = type_card & field_layout::kTvMask;
  void* const base = MaybeGetSplitBase(msg, is_split, table);
  switch (rep) {
    case field_layout::kRepSString: {
      auto& field = MaybeCreateRepeatedPtrFieldRefAt<std::string, is_split>(
          base, entry.offset, msg);
      const char* ptr2 = ptr;
      uint32_t next_tag;

      auto* arena = field.GetArena();
      SerialArena* serial_arena;
      if (PROTOBUF_PREDICT_TRUE(
              arena != nullptr &&
              arena->impl_.GetSerialArenaFast(&serial_arena) &&
              field.PrepareForParse())) {
        do {
          ptr = ptr2;
          ptr = ParseRepeatedStringOnce(ptr, serial_arena, ctx, field);
          if (PROTOBUF_PREDICT_FALSE(ptr == nullptr ||
                                     !MpVerifyUtf8(field[field.size() - 1],
                                                   table, entry, xform_val))) {
            return nullptr;
          }
          if (PROTOBUF_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
          ptr2 = ReadTag(ptr, &next_tag);
        } while (next_tag == decoded_tag);
      } else {
        do {
          ptr = ptr2;
          std::string* str = field.Add();
          ptr = InlineGreedyStringParser(str, ptr, ctx);
          if (PROTOBUF_PREDICT_FALSE(
                  ptr == nullptr ||
                  !MpVerifyUtf8(*str, table, entry, xform_val))) {
            return nullptr;
          }
          if (PROTOBUF_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
          ptr2 = ReadTag(ptr, &next_tag);
        } while (next_tag == decoded_tag);
      }

      break;
    }

#ifndef NDEBUG
    default:
      ABSL_LOG(FATAL) << "Unsupported repeated string rep: " << rep;
      break;
#endif
  }

  return ptr;
parse_loop:
  return ptr;
}


template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpMessage(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint16_t card = type_card & field_layout::kFcMask;

  // Check for repeated parsing:
  if (card == field_layout::kFcRepeated) {
    const uint16_t rep = type_card & field_layout::kRepMask;
    switch (rep) {
      case field_layout::kRepMessage:
        return MpRepeatedMessageOrGroup<is_split, false>(
            msg, ptr, ctx, table, data);
      case field_layout::kRepGroup:
        return MpRepeatedMessageOrGroup<is_split, true>(
            msg, ptr, ctx, table, data);
      default:
        return table->fallback(msg, ptr, ctx, table, data);
    }
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
      return table->fallback(msg, ptr, ctx, table, data);
    }
  }

  const bool is_oneof = card == field_layout::kFcOneof;
  bool need_init = false;
  if (card == field_layout::kFcOptional) {
    SetHas(entry, msg);
  } else if (is_oneof) {
    need_init = ChangeOneof(table, entry, data.tag() >> 3, ctx, msg);
  }

  void* const base = MaybeGetSplitBase(msg, is_split, table);
  MessageLite*& field = RefAt<MessageLite*>(base, entry.offset);
  if ((type_card & field_layout::kTvMask) == field_layout::kTvTable) {
    auto* inner_table = table->field_aux(&entry)->table;
    if (need_init || field == nullptr) {
      field = inner_table->default_instance->New(msg->GetArena());
    }
    if (is_group) {
      return ctx->ParseGroup<TcParser>(field, ptr, decoded_tag, inner_table);
    }
    return ctx->ParseMessage<TcParser>(field, ptr, inner_table);
  } else {
    if (need_init || field == nullptr) {
      const MessageLite* def;
      if ((type_card & field_layout::kTvMask) == field_layout::kTvDefault) {
        def = table->field_aux(&entry)->message_default();
      } else {
        ABSL_DCHECK_EQ(type_card & field_layout::kTvMask,
                       +field_layout::kTvWeakPtr);
        def = table->field_aux(&entry)->message_default_weak();
      }
      field = def->New(msg->GetArena());
    }
    if (is_group) {
      return ctx->ParseGroup(field, ptr, decoded_tag);
    }
    return ctx->ParseMessage(field, ptr);
  }
}

template <bool is_split, bool is_group>
const char* TcParser::MpRepeatedMessageOrGroup(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  ABSL_DCHECK_EQ(type_card & field_layout::kFcMask,
                 static_cast<uint16_t>(field_layout::kFcRepeated));
  const uint32_t decoded_tag = data.tag();
  const uint32_t decoded_wiretype = decoded_tag & 7;

  // Validate wiretype:
  if (!is_group) {
    ABSL_DCHECK_EQ(type_card & field_layout::kRepMask,
                   static_cast<uint16_t>(field_layout::kRepMessage));
    if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
      return table->fallback(msg, ptr, ctx, table, data);
    }
  } else {
    ABSL_DCHECK_EQ(type_card & field_layout::kRepMask,
                   static_cast<uint16_t>(field_layout::kRepGroup));
    if (decoded_wiretype != WireFormatLite::WIRETYPE_START_GROUP) {
      return table->fallback(msg, ptr, ctx, table, data);
    }
  }

  void* const base = MaybeGetSplitBase(msg, is_split, table);
  RepeatedPtrFieldBase& field =
      MaybeCreateRepeatedRefAt<RepeatedPtrFieldBase, is_split>(
          base, entry.offset, msg);
  const auto aux = *table->field_aux(&entry);
  if ((type_card & field_layout::kTvMask) == field_layout::kTvTable) {
    auto* inner_table = aux.table;
    const MessageLite* default_instance = inner_table->default_instance;
    const char* ptr2 = ptr;
    uint32_t next_tag;
    do {
      MessageLite* value = field.AddMessage(default_instance);
      ptr = is_group ? ctx->ParseGroup<TcParser>(value, ptr2, decoded_tag,
                                                 inner_table)
                     : ctx->ParseMessage<TcParser>(value, ptr2, inner_table);
      if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) goto error;
      if (PROTOBUF_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
      ptr2 = ReadTag(ptr, &next_tag);
      if (PROTOBUF_PREDICT_FALSE(ptr2 == nullptr)) goto error;
    } while (next_tag == decoded_tag);
  } else {
    const MessageLite* default_instance;
    if ((type_card & field_layout::kTvMask) == field_layout::kTvDefault) {
      default_instance = aux.message_default();
    } else {
      ABSL_DCHECK_EQ(type_card & field_layout::kTvMask,
                     +field_layout::kTvWeakPtr);
      default_instance = aux.message_default_weak();
    }
    const char* ptr2 = ptr;
    uint32_t next_tag;
    do {
      MessageLite* value = field.AddMessage(default_instance);
      ptr = is_group ? ctx->ParseGroup(value, ptr2, decoded_tag)
                     : ctx->ParseMessage(value, ptr2);
      if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) goto error;
      if (PROTOBUF_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
      ptr2 = ReadTag(ptr, &next_tag);
      if (PROTOBUF_PREDICT_FALSE(ptr2 == nullptr)) goto error;
    } while (next_tag == decoded_tag);
  }
  return ptr;
parse_loop:
  return ptr;
error:
  return nullptr;
}

static void SerializeMapKey(const NodeBase* node, MapTypeCard type_card,
                            io::CodedOutputStream& coded_output) {
  switch (type_card.wiretype()) {
    case WireFormatLite::WIRETYPE_VARINT:
      switch (type_card.cpp_type()) {
        case MapTypeCard::kBool:
          WireFormatLite::WriteBool(
              1, static_cast<const KeyNode<bool>*>(node)->key(), &coded_output);
          break;
        case MapTypeCard::k32:
          if (type_card.is_zigzag()) {
            WireFormatLite::WriteSInt32(
                1, static_cast<const KeyNode<uint32_t>*>(node)->key(),
                &coded_output);
          } else if (type_card.is_signed()) {
            WireFormatLite::WriteInt32(
                1, static_cast<const KeyNode<uint32_t>*>(node)->key(),
                &coded_output);
          } else {
            WireFormatLite::WriteUInt32(
                1, static_cast<const KeyNode<uint32_t>*>(node)->key(),
                &coded_output);
          }
          break;
        case MapTypeCard::k64:
          if (type_card.is_zigzag()) {
            WireFormatLite::WriteSInt64(
                1, static_cast<const KeyNode<uint64_t>*>(node)->key(),
                &coded_output);
          } else if (type_card.is_signed()) {
            WireFormatLite::WriteInt64(
                1, static_cast<const KeyNode<uint64_t>*>(node)->key(),
                &coded_output);
          } else {
            WireFormatLite::WriteUInt64(
                1, static_cast<const KeyNode<uint64_t>*>(node)->key(),
                &coded_output);
          }
          break;
        default:
          Unreachable();
      }
      break;
    case WireFormatLite::WIRETYPE_FIXED32:
      WireFormatLite::WriteFixed32(
          1, static_cast<const KeyNode<uint32_t>*>(node)->key(), &coded_output);
      break;
    case WireFormatLite::WIRETYPE_FIXED64:
      WireFormatLite::WriteFixed64(
          1, static_cast<const KeyNode<uint64_t>*>(node)->key(), &coded_output);
      break;
    case WireFormatLite::WIRETYPE_LENGTH_DELIMITED:
      // We should never have a message here. They can only be values maps.
      ABSL_DCHECK_EQ(+type_card.cpp_type(), +MapTypeCard::kString);
      WireFormatLite::WriteString(
          1, static_cast<const KeyNode<std::string>*>(node)->key(),
          &coded_output);
      break;
    default:
      Unreachable();
  }
}

void TcParser::WriteMapEntryAsUnknown(MessageLite* msg,
                                      const TcParseTableBase* table,
                                      uint32_t tag, NodeBase* node,
                                      MapAuxInfo map_info) {
  std::string serialized;
  {
    io::StringOutputStream string_output(&serialized);
    io::CodedOutputStream coded_output(&string_output);
    SerializeMapKey(node, map_info.key_type_card, coded_output);
    // The mapped_type is always an enum here.
    ABSL_DCHECK(map_info.value_is_validated_enum);
    WireFormatLite::WriteInt32(2,
                               *reinterpret_cast<int32_t*>(
                                   node->GetVoidValue(map_info.node_size_info)),
                               &coded_output);
  }
  GetUnknownFieldOps(table).write_length_delimited(msg, tag >> 3, serialized);
}

PROTOBUF_ALWAYS_INLINE inline void TcParser::InitializeMapNodeEntry(
    void* obj, MapTypeCard type_card, UntypedMapBase& map,
    const TcParseTableBase::FieldAux* aux, bool is_key) {
  (void)is_key;
  switch (type_card.cpp_type()) {
    case MapTypeCard::kBool:
      memset(obj, 0, sizeof(bool));
      break;
    case MapTypeCard::k32:
      memset(obj, 0, sizeof(uint32_t));
      break;
    case MapTypeCard::k64:
      memset(obj, 0, sizeof(uint64_t));
      break;
    case MapTypeCard::kString:
      Arena::CreateInArenaStorage(reinterpret_cast<std::string*>(obj),
                                  map.arena());
      break;
    case MapTypeCard::kMessage:
      aux[1].create_in_arena(map.arena(), reinterpret_cast<MessageLite*>(obj));
      break;
    default:
      Unreachable();
  }
}

PROTOBUF_NOINLINE void TcParser::DestroyMapNode(NodeBase* node,
                                                MapAuxInfo map_info,
                                                UntypedMapBase& map) {
  if (map_info.key_type_card.cpp_type() == MapTypeCard::kString) {
    static_cast<std::string*>(node->GetVoidKey())->~basic_string();
  }
  if (map_info.value_type_card.cpp_type() == MapTypeCard::kString) {
    static_cast<std::string*>(node->GetVoidValue(map_info.node_size_info))
        ->~basic_string();
  } else if (map_info.value_type_card.cpp_type() == MapTypeCard::kMessage) {
    static_cast<MessageLite*>(node->GetVoidValue(map_info.node_size_info))
        ->~MessageLite();
  }
  map.DeallocNode(node, map_info.node_size_info);
}

template <typename T>
const char* ReadFixed(void* obj, const char* ptr) {
  auto v = UnalignedLoad<T>(ptr);
  ptr += sizeof(v);
  memcpy(obj, &v, sizeof(v));
  return ptr;
}

const char* TcParser::ParseOneMapEntry(
    NodeBase* node, const char* ptr, ParseContext* ctx,
    const TcParseTableBase::FieldAux* aux, const TcParseTableBase* table,
    const TcParseTableBase::FieldEntry& entry, Arena* arena) {
  using WFL = WireFormatLite;

  const auto map_info = aux[0].map_info;
  const uint8_t key_tag = WFL::MakeTag(1, map_info.key_type_card.wiretype());
  const uint8_t value_tag =
      WFL::MakeTag(2, map_info.value_type_card.wiretype());

  while (!ctx->Done(&ptr)) {
    uint32_t inner_tag = ptr[0];

    if (PROTOBUF_PREDICT_FALSE(inner_tag != key_tag &&
                               inner_tag != value_tag)) {
      // Do a full parse and check again in case the tag has non-canonical
      // encoding.
      ptr = ReadTag(ptr, &inner_tag);
      if (PROTOBUF_PREDICT_FALSE(inner_tag != key_tag &&
                                 inner_tag != value_tag)) {
        if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) return nullptr;

        if (inner_tag == 0 || (inner_tag & 7) == WFL::WIRETYPE_END_GROUP) {
          ctx->SetLastTag(inner_tag);
          break;
        }

        ptr = UnknownFieldParse(inner_tag, nullptr, ptr, ctx);
        if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) return nullptr;
        continue;
      }
    } else {
      ++ptr;
    }

    MapTypeCard type_card;
    void* obj;
    if (inner_tag == key_tag) {
      type_card = map_info.key_type_card;
      obj = node->GetVoidKey();
    } else {
      type_card = map_info.value_type_card;
      obj = node->GetVoidValue(map_info.node_size_info);
    }

    switch (type_card.wiretype()) {
      case WFL::WIRETYPE_VARINT:
        uint64_t tmp;
        ptr = VarintParse(ptr, &tmp);
        if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) return nullptr;
        switch (type_card.cpp_type()) {
          case MapTypeCard::kBool:
            *reinterpret_cast<bool*>(obj) = static_cast<bool>(tmp);
            continue;
          case MapTypeCard::k32: {
            uint32_t v = static_cast<uint32_t>(tmp);
            if (type_card.is_zigzag()) v = WFL::ZigZagDecode32(v);
            memcpy(obj, &v, sizeof(v));
            continue;
          }
          case MapTypeCard::k64:
            if (type_card.is_zigzag()) tmp = WFL::ZigZagDecode64(tmp);
            memcpy(obj, &tmp, sizeof(tmp));
            continue;
          default:
            Unreachable();
        }
      case WFL::WIRETYPE_FIXED32:
        ptr = ReadFixed<uint32_t>(obj, ptr);
        continue;
      case WFL::WIRETYPE_FIXED64:
        ptr = ReadFixed<uint64_t>(obj, ptr);
        continue;
      case WFL::WIRETYPE_LENGTH_DELIMITED:
        if (type_card.cpp_type() == MapTypeCard::kString) {
          const int size = ReadSize(&ptr);
          if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) return nullptr;
          std::string* str = reinterpret_cast<std::string*>(obj);
          ptr = ctx->ReadString(ptr, size, str);
          if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) return nullptr;
          bool do_utf8_check = map_info.fail_on_utf8_failure;
#ifndef NDEBUG
          do_utf8_check |= map_info.log_debug_utf8_failure;
#endif
          if (type_card.is_utf8() && do_utf8_check &&
              !utf8_range::IsStructurallyValid(*str)) {
            PrintUTF8ErrorLog(MessageName(table), FieldName(table, &entry),
                              "parsing", false);
            if (map_info.fail_on_utf8_failure) {
              return nullptr;
            }
          }
          continue;
        } else {
          ABSL_DCHECK_EQ(+type_card.cpp_type(), +MapTypeCard::kMessage);
          ABSL_DCHECK_EQ(inner_tag, value_tag);
          ptr = ctx->ParseMessage(reinterpret_cast<MessageLite*>(obj), ptr);
          if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) return nullptr;
          continue;
        }
      default:
        Unreachable();
    }
  }
  return ptr;
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpMap(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  // `aux[0]` points into a MapAuxInfo.
  // If we have a message mapped_type aux[1] points into a `create_in_arena`.
  // If we have a validated enum mapped_type aux[1] point into a
  // `enum_data`.
  const auto* aux = table->field_aux(&entry);
  const auto map_info = aux[0].map_info;

  if (PROTOBUF_PREDICT_FALSE(!map_info.is_supported ||
                             (data.tag() & 7) !=
                                 WireFormatLite::WIRETYPE_LENGTH_DELIMITED)) {
    return MpFallback(msg, ptr, ctx, table, data);
  }

  // When using LITE, the offset points directly into the Map<> object.
  // Otherwise, it points into a MapField and we must synchronize with
  // reflection. It is done by calling the MutableMap() virtual function on the
  // field's base class.
  void* const base = MaybeGetSplitBase(msg, is_split, table);
  UntypedMapBase& map =
      map_info.use_lite
          ? RefAt<UntypedMapBase>(base, entry.offset)
          : *RefAt<MapFieldBaseForParse>(base, entry.offset).MutableMap();

  const uint32_t saved_tag = data.tag();

  while (true) {
    NodeBase* node = map.AllocNode(map_info.node_size_info);

    InitializeMapNodeEntry(node->GetVoidKey(), map_info.key_type_card, map, aux,
                           true);
    InitializeMapNodeEntry(node->GetVoidValue(map_info.node_size_info),
                           map_info.value_type_card, map, aux, false);

    ptr = ctx->ParseLengthDelimitedInlined(ptr, [&](const char* ptr) {
      return ParseOneMapEntry(node, ptr, ctx, aux, table, entry, map.arena());
    });

    if (PROTOBUF_PREDICT_TRUE(ptr != nullptr)) {
      if (PROTOBUF_PREDICT_FALSE(map_info.value_is_validated_enum &&
                                 !internal::ValidateEnumInlined(
                                     *static_cast<int32_t*>(node->GetVoidValue(
                                         map_info.node_size_info)),
                                     aux[1].enum_data))) {
        WriteMapEntryAsUnknown(msg, table, saved_tag, node, map_info);
      } else {
        // Done parsing the node, try to insert it.
        // If it overwrites something we get old node back to destroy it.
        switch (map_info.key_type_card.cpp_type()) {
          case MapTypeCard::kBool:
            node = static_cast<KeyMapBase<bool>&>(map).InsertOrReplaceNode(
                static_cast<KeyMapBase<bool>::KeyNode*>(node));
            break;
          case MapTypeCard::k32:
            node = static_cast<KeyMapBase<uint32_t>&>(map).InsertOrReplaceNode(
                static_cast<KeyMapBase<uint32_t>::KeyNode*>(node));
            break;
          case MapTypeCard::k64:
            node = static_cast<KeyMapBase<uint64_t>&>(map).InsertOrReplaceNode(
                static_cast<KeyMapBase<uint64_t>::KeyNode*>(node));
            break;
          case MapTypeCard::kString:
            node =
                static_cast<KeyMapBase<std::string>&>(map).InsertOrReplaceNode(
                    static_cast<KeyMapBase<std::string>::KeyNode*>(node));
            break;
          default:
            Unreachable();
        }
      }
    }

    // Destroy the node if we have it.
    // It could be because we failed to parse, or because insertion returned
    // an overwritten node.
    if (PROTOBUF_PREDICT_FALSE(node != nullptr && map.arena() == nullptr)) {
      DestroyMapNode(node, map_info, map);
    }

    if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) {
      return nullptr;
    }

    if (PROTOBUF_PREDICT_FALSE(!ctx->DataAvailable(ptr))) {
      return ptr;
    }

    uint32_t next_tag;
    const char* ptr2 = ReadTagInlined(ptr, &next_tag);
    if (next_tag != saved_tag) break;
    ptr = ptr2;
  }

  return ptr;
}

inline void SetHasBit(void* x, uint32_t fd, void* dummy) {
  using FFE = TcParseTableBase::FastFieldEntry;
  auto idx = (fd >> FFE::kHasBitShift) & FFE::kHasBitMask;
  ABSL_DCHECK(((fd & FFE::kCardMask) != FFE::kSingular) || idx == 0);
  x = (fd & FFE::kCardMask) == FFE::kSingular ? dummy : x;
  static_cast<char*>(x)[idx / 8] |= 1 << (idx & 7);
}

inline void Store(uint64_t value, void* out, uint32_t fd, void* dummy) {
    using FFE = TcParseTableBase::FastFieldEntry;
    unsigned offset = fd >> FFE::kOffsetShift;
    out = static_cast<char*>(out) + offset;
    *static_cast<bool*>(out) = value;
    auto dst = (fd & FFE::kRep32Bit) ? out : dummy;
    asm volatile (""::"r"(dst));
    *static_cast<uint32_t*>(dst) = value;
    dst = (fd & FFE::kRep64Bit) ? out : dummy;
    asm volatile (""::"r"(dst));
    *static_cast<uint64_t*>(dst) = value;
}

template <typename FieldType>
const char* MplRepeatedVarint(const char* ptr, const char* end, RepeatedField<FieldType>& field, bool zigzag, 
        uint32_t expected_tag, uint64_t value) {
#if defined(__x86_64__)
    uint8_t buffer[8] = {};
    unsigned sz = io::CodedOutputStream::WriteVarint32ToArray(expected_tag, buffer) - buffer;
    auto image = UnalignedLoad<uint64_t>((void*)buffer);
    unsigned tagbits = sz * 8;
    auto mask = (1ull << tagbits) - 1; 
    auto mask2 = mask | 0x7f7f7f7f7f7f7f7f;
    auto pext_mask = 0x7f7f7f7f7f7f7f7f & ~mask;
    field.Add(value);
    while (PROTOBUF_PREDICT_TRUE(ptr < end)) {
        uint64_t v1 = UnalignedLoad<uint64_t>(ptr);
        uint64_t v2 = UnalignedLoad<uint64_t>(ptr + 8);
        unsigned bits = 0;
        for (int i = 0; i < 2; i++) {
          if ((v1 & mask) != image) {
            return ptr + (bits / 8);
          }
          auto cbits = v1 | mask2;
          if (cbits + 1 == 0) { 
            ptr += bits / 8 + sz;
            value = ReadVarint64(&ptr);
            if (ptr == nullptr) return nullptr;
            if (zigzag) value = WireFormatLite::ZigZagDecode64(value);
            field.Add(value);
            goto next;            
          }
          auto y = cbits ^ (cbits + 1);
          value = _pext_u64(v1 & y, pext_mask);
          auto nbits = __builtin_popcountll(y);
          v1 = (nbits == 64 ? 0 : (v1 >> nbits)) | (v2 << (64 - nbits));
          // v2 = (nbits == 64 ? 0 : (v2 >> nbits)) | (v3 << (64 - nbits));
          v2 >>= nbits;
          bits += nbits;
          if (zigzag) value = WireFormatLite::ZigZagDecode64(value);
          field.Add(value);
        }
        ptr += bits / 8;
next:;
    }
    return ptr;
#else
  uint8_t buffer[8] = {};
  unsigned sz = io::CodedOutputStream::WriteVarint32ToArray(expected_tag, buffer) - buffer;
  auto image = UnalignedLoad<uint64_t>((void*)buffer);
  auto mask = (1ull << (sz * 8)) - 1; 
  field.Add(value);
  while (PROTOBUF_PREDICT_TRUE(ptr < end)) {
    uint64_t tag = UnalignedLoad<uint64_t>(ptr);
    if ((tag & mask) != image) break;
    ptr += sz;
    ptr = VarintParse(ptr, &value);
    if (ptr == nullptr) return nullptr;
    if (zigzag) value = WireFormatLite::ZigZagDecode64(value);
    field.Add(value);
  }
  return ptr;
#endif
}

template <typename FieldType>
const char* MplRepeatedFixed(const char* ptr, const char* end, RepeatedField<FieldType>& field,
        uint32_t expected_tag, uint64_t value) {
  uint8_t buffer[8] = {};
  unsigned sz = io::CodedOutputStream::WriteVarint32ToArray(expected_tag, buffer) - buffer;
  auto image = UnalignedLoad<uint64_t>((void*)buffer);
  auto mask = (1ull << (sz * 8)) - 1; 
  field.Add(value);
  while (PROTOBUF_PREDICT_TRUE(ptr < end)) {
    uint64_t tag = UnalignedLoad<uint64_t>(ptr);
    if ((tag & mask) != image) break;
    ptr += sz;
    value = UnalignedLoad<FieldType>(ptr);
    ptr += sizeof(FieldType);
    field.Add(value);
  }
  return ptr;
}

inline const char* ParseScalarBranchless(const char* ptr, uint32_t wt, uint64_t& data) {
#if defined(__x86_64__)
    static const uint64_t size_mask[] = {
        0xFF,
        0xFFFF,
        0xFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFFFF,
        0xFFFFFFFFFFFF,
        0xFFFFFFFFFFFFFF,
        0xFFFFFFFFFFFFFFFF
    };
    uint64_t mask = 0x7f7f7f7f7f7f7f7f;
    auto x = data | mask;
    auto z = x + 1;
    if (ABSL_PREDICT_FALSE(z == 0) && wt == 0) {
      data = ReadVarint64(&ptr);
      return ptr;
    }
    auto y = x ^ z;
    auto varintsize = __builtin_popcountll(y) / 8;
    auto fixedsize = wt & 4 ? 4 : 8;
    if (wt & 1) mask = -1;
    auto size = (wt & 1 ? fixedsize : varintsize);
    data &= size_mask[size - 1];
    data = _pext_u64(data, mask);
    return ptr + size;
#else
  if (ABSL_PREDICT_FALSE((data & (0x80 << wt)) & 0x80)) {
    ptr = VarintParse(ptr, &data);
  } else {
    ptr += wt & 1 ? (wt & 4 ? 4 : 8) : 1;
    data &= wt & 1 ? -1ull : 0xFF;
  }
  return ptr;
#endif
}

ABSL_ATTRIBUTE_NOINLINE
const char* TcParser::MiniParseFallback(MessageLite* msg, const char* ptr, ParseContext* ctx, const TcParseTableBase* table, const void* entry, uint32_t tag) {
  TcFieldData data;

  if (entry == nullptr) {
    entry = FindFieldEntry(table, tag >> 3);
    if (entry == nullptr) {
      data.data = tag;
      return table->fallback(msg, ptr, ctx, table, data);
    }
  }

  // The handler may need the tag and the entry to resolve fallback logic. Both
  // of these are 32 bits, so pack them into (the 64-bit) `data`. Since we can't
  // pack the entry pointer itself, just pack its offset from `table`.
  uint64_t entry_offset = reinterpret_cast<const char*>(entry) -
                          reinterpret_cast<const char*>(table);
  data.data = entry_offset << 32 | tag;

  using field_layout::FieldKind;
  auto field_type =
      static_cast<const FieldEntry*>(entry)->type_card & (+field_layout::kSplitMask | FieldKind::kFkMask);

  static constexpr TailCallParseFunc kMiniParseTable[] = {
      &MpFallback,             // FieldKind::kFkNone
      &MpVarint<false>,        // FieldKind::kFkVarint
      &MpPackedVarint<false>,  // FieldKind::kFkPackedVarint
      &MpFixed<false>,         // FieldKind::kFkFixed
      &MpPackedFixed<false>,   // FieldKind::kFkPackedFixed
      &MpString<false>,        // FieldKind::kFkString
      &MpMessage<false>,       // FieldKind::kFkMessage
      &MpMap<false>,           // FieldKind::kFkMap
      &Error,                  // kSplitMask | FieldKind::kFkNone
      &MpVarint<true>,         // kSplitMask | FieldKind::kFkVarint
      &MpPackedVarint<true>,   // kSplitMask | FieldKind::kFkPackedVarint
      &MpFixed<true>,          // kSplitMask | FieldKind::kFkFixed
      &MpPackedFixed<true>,    // kSplitMask | FieldKind::kFkPackedFixed
      &MpString<true>,         // kSplitMask | FieldKind::kFkString
      &MpMessage<true>,        // kSplitMask | FieldKind::kFkMessage
      &MpMap<true>,            // kSplitMask | FieldKind::kFkMap
  };

  TailCallParseFunc parse_fn = kMiniParseTable[field_type];

  return parse_fn(msg, ptr, ctx, table, data);  
}

// On the fast path, a (matching) 2-byte tag always needs to be decoded.
static uint32_t FastDecodeTag(const char** pptr, uint64_t* value) {
  const char*& ptr = *pptr;
  uint32_t res = UnalignedLoad<uint16_t>(ptr);
  if (ABSL_PREDICT_FALSE((res & 0x8080) == 0x8080)) {
    res = static_cast<uint8_t>(ptr[0]);
    uint32_t second = static_cast<uint8_t>(ptr[1]);
    res += (second - 1) << 7;
    auto tmp = ReadTagFallback(ptr, res);
    ptr = tmp.first;
    if (ptr == nullptr) return tmp.second;
    *value = UnalignedLoad<uint64_t>(ptr);
    return tmp.second;
  }
  const char* p1 = ptr + 1;
  const char* p2 = ptr + 2;
  uint64_t v1 = UnalignedLoad<uint64_t>(p1);
  uint64_t v2 = UnalignedLoad<uint64_t>(p2);
  asm(""::"r"(v1), "r"(v2));
  ptr = res & 0x80 ? p2 : p1;
  // Preload value to keep load of critical chain
  *value = res & 0x80 ? v2 : v1;
  uint32_t mask = static_cast<int8_t>(res);
  res = (res & mask) + mask;
  return res >> 1;
}

ABSL_ATTRIBUTE_ALWAYS_INLINE
static uint32_t FastReadSize(const char** pptr, uint64_t value) {
  const char*& ptr = *pptr;
  uint32_t res = value & 0xFFFF;
  if (ABSL_PREDICT_FALSE((res & 0x8080) == 0x8080)) {
    return ReadSize(pptr);
  }
  const char* p1 = ptr + 1;
  const char* p2 = ptr + 2;
  ptr = res & 0x80 ? p2 : p1;
  uint32_t mask = static_cast<int8_t>(res);
  res = (res & mask) + mask;
  return res >> 1;
}

bool FastFieldLookup(const TcParseTableBase* table, uint32_t tag, uint32_t* fd) {
  auto idx = (tag >> 3) - 1;
  if (ABSL_PREDICT_FALSE(idx >= table->num_fast_fields)) return false;
  *fd = table->fast_entry(idx)->bits;
  return true;
}

const char* TcParser::MiniParseLoop(MessageLite* const msg, const char* ptr, ParseContext* const ctx, 
        const TcParseTableBase* const table, int64_t const delta_or_group) {
  using FFE = TcParseTableBase::FastFieldEntry;
  // TODO move into ParseContext
  char dummy[8] = {};
  char has_dummy[8] = {};
  Arena* arena = msg->GetArena();

  uint32_t wt;
  uint64_t value;
  uint32_t tag;
  uint32_t fd;

  // Scalars
  while (!ctx->Done(&ptr)) {
    wt = UnalignedLoad<uint16_t>(ptr) & 7;
    tag = FastDecodeTag(&ptr, &value);
    if (ptr == nullptr) return nullptr;
    if (ABSL_PREDICT_FALSE(wt == 4)) goto endgroup;

    const TcParseTableBase::FieldEntry* entry;
    if (!FastFieldLookup(table, tag, &fd)) {
fallback:
      if (tag == 0) goto unusual_end;
unusual:
      entry = nullptr;
with_entry:
      ptr = MiniParseFallback(msg, ptr, ctx, table, entry, tag);
      if (ptr == nullptr) return nullptr;
      continue;
    }
    if (ABSL_PREDICT_FALSE((fd & FFE::kCardMask) == FFE::kFallback)) {
fast_fallback:
      if (ABSL_PREDICT_FALSE(fd == 0x1F)) goto unusual;
      entry = table->field_entries_begin() + (fd >> 5); 
      goto with_entry;
    }
    if (wt != (fd & 7)) goto unusual;
    if (ABSL_PREDICT_FALSE((fd & (FFE::kRepMask | 7)) == (FFE::kRepMessage | 2))) goto parse_len_delim_submessage;
    if (ABSL_PREDICT_FALSE(wt == 2)) goto parse_string;
parse_scalar:
    if (ABSL_PREDICT_FALSE(wt == 3)) {
      ABSL_DCHECK((fd & FFE::kRepMask) == FFE::kRepMessage);
      value = ~static_cast<uint64_t>(tag + 1);
      ctx->IncGroupDepth();
      goto parse_submessage;
    }
    ptr = ParseScalarBranchless(ptr, wt, value);
    if (ptr == nullptr) return ptr;
    if (fd & FFE::kZigZag) value = WireFormatLite::ZigZagDecode64(value);
    if (ABSL_PREDICT_TRUE((fd & FFE::kCardMask) <= FFE::kOptional)) {
      SetHasBit(msg, fd, has_dummy);
      Store(value, msg, fd, dummy);
    } else {
      const char* end = ctx->limit_end_;
      if ((fd & FFE::kRepMask) == FFE::kRepBool) {
        auto& field = RefAt<RepeatedField<bool>>(msg, fd >> FFE::kOffsetShift);
        ptr = MplRepeatedVarint(ptr, end, field, false, tag, value);
      } else if ((fd & FFE::kRepMask) == FFE::kRep32Bit) {
        auto& field = RefAt<RepeatedField<uint32_t>>(msg, fd >> FFE::kOffsetShift);
        if ((tag & 1) == 0) {
          bool zigzag = fd & FFE::kZigZag;
          ptr = MplRepeatedVarint(ptr, end, field, zigzag, tag, value);
        } else {
          ptr = MplRepeatedFixed(ptr, end, field, tag, value);
        }
      } else {
        auto& field = RefAt<RepeatedField<uint64_t>>(msg, fd >> FFE::kOffsetShift);
        if ((tag & 1) == 0) {
          bool zigzag = fd & FFE::kZigZag;
          ptr = MplRepeatedVarint(ptr, end, field, zigzag, tag, value);
        } else {
          ptr = MplRepeatedFixed(ptr, end, field, tag, value);
        }
      }
      if (ptr == nullptr) return nullptr;
    }
  }  // while scalars
  goto end;

  // strings
  while (!ctx->Done(&ptr)) {
    wt = UnalignedLoad<uint16_t>(ptr) & 7;
    tag = FastDecodeTag(&ptr, &value);
    if (ptr == nullptr) return nullptr;
    if (ABSL_PREDICT_FALSE(wt == 4)) goto endgroup;

    if (!FastFieldLookup(table, tag, &fd)) goto fallback;
    if (ABSL_PREDICT_FALSE((fd & FFE::kCardMask) == FFE::kFallback)) goto fast_fallback;
    if (wt != (fd & 7)) goto unusual;
    if (ABSL_PREDICT_FALSE(wt != 2)) goto parse_scalar;
parse_string:
    if (ABSL_PREDICT_FALSE((fd & FFE::kRepMask) != FFE::kRepBytes)) goto parse_len_delim_submessage;
    absl::string_view sv;
    if (ABSL_PREDICT_TRUE((fd & FFE::kCardMask) <= FFE::kOptional)) {
      SetHasBit(msg, fd, has_dummy);
      auto& field = RefAt<ArenaStringPtr>(msg, fd >> FFE::kOffsetShift);
      int sz;
      if (ABSL_PREDICT_FALSE(value & 0x80)) {
        sz = ReadSize(&ptr);
      } else {
        sz = value & 0xFF;
        ptr++;
      }

      if (ptr == nullptr) return nullptr;
      if (arena) {
        ptr = ctx->ReadArenaString(ptr, sz, &field, arena);
      } else {
        ptr = ctx->ReadString(ptr, sz, field.MutableNoCopy(nullptr));
      }
      sv = field.Get();
    } else {
      auto& field = RefAt<RepeatedPtrField<std::string>>(msg, fd >> FFE::kOffsetShift);
      auto sz = ReadSize(&ptr);
      if (ptr == nullptr) return nullptr;
      auto s = field.Add();
      ptr = ctx->ReadString(ptr, sz, s);
      sv = *s;
    }
    if (ptr == nullptr) return nullptr;
#ifdef NDEBUG
    constexpr bool kUtf8Debug = false;
#else
    constexpr bool kUtf8Debug = true;
#endif
    if (((fd & FFE::kTransformMask) != FFE::kBytes && kUtf8Debug) ||
        ((fd & FFE::kTransformMask) == FFE::kUtf8 && !kUtf8Debug)) { 
      if (ABSL_PREDICT_FALSE(!utf8_range::IsStructurallyValid(sv))) {
        ReportFastUtf8Error(tag, table);
        if ((fd & FFE::kTransformMask) == FFE::kUtf8) return nullptr;
      }
    }
  } // while strings
  goto end;

  // messages
  while (!ctx->Done(&ptr)) {
    wt = UnalignedLoad<uint16_t>(ptr) & 7;
    tag = FastDecodeTag(&ptr, &value);
    if (ptr == nullptr) return nullptr;
    if (ABSL_PREDICT_FALSE(wt == 4)) goto endgroup;

    if (!FastFieldLookup(table, tag, &fd)) goto fallback;
    if (ABSL_PREDICT_FALSE((fd & FFE::kCardMask) == FFE::kFallback)) goto fast_fallback;
    if (wt != (fd & 7)) goto unusual;
    if (ABSL_PREDICT_FALSE(wt != 2)) {
      goto parse_scalar;
    }
parse_len_delim_submessage:
    switch (__builtin_expect(fd & FFE::kRepMask, FFE::kRepMessage)) {
      case FFE::kRepBytes:
        goto parse_string;
      case FFE::kRepMessage: {
        break;
      }
      case FFE::kRepPackedFixed: {
        goto unusual;
      }
      case FFE::kRepPackedVarint: {
        goto unusual;
      }
      default:
        break;
    }
    {
      auto sz = FastReadSize(&ptr, value);
      if (ptr == nullptr) return nullptr;
      value = ctx->PushLimit(ptr, sz).token();
    }
    // TODO: this check is necessary to prevent negative size to immitate a group end
    // A test is failing because it expects presence of a submsg after a failed parse.
    // if (static_cast<int64_t>(value) < 0) return nullptr;
parse_submessage:
    auto entry_idx = fd >> FFE::kOffsetShift;
    auto entry = table->field_entries_begin() + entry_idx;
    auto child_table = table->field_aux(entry->aux_idx)->table;
    auto offset = entry->offset;
    if (!ctx->IncDepth()) return nullptr;
    if (ABSL_PREDICT_TRUE((fd & FFE::kCardMask) <= FFE::kOptional)) {
      SetHasBit(msg, fd, has_dummy);
      auto& field = RefAt<MessageLite*>(msg, offset);
      if (field == nullptr) {
        field = child_table->default_instance->New(arena);
      }
      auto child = field;
      ptr = MiniParseLoop(child, ptr, ctx, child_table, value);
      if (ptr == nullptr) return nullptr;
    } else {
      auto& field = RefAt<RepeatedPtrFieldBase>(msg, offset);
      while (true) {
        auto child = field.AddMessage(child_table->default_instance);
        ptr = MiniParseLoop(child, ptr, ctx, child_table, value);
        if (ptr == nullptr) return nullptr;
        if (!ctx->DataAvailable(ptr)) break;
        uint32_t nexttag;
        auto newptr = ReadTagInlined(ptr, &nexttag);
        if (nexttag != tag) break;
        ptr = newptr;
        if ((tag & 7) == 3) {
          ctx->IncGroupDepth();
        } else {
          auto sz = FastReadSize(&ptr, UnalignedLoad<uint16_t>(ptr));
          if (ptr == nullptr) return nullptr;
          value = ctx->PushLimit(ptr, sz).token();
          // TODO check value
        }
      }
    }
    ctx->DecDepth();
  }  // while
end:
  if (ptr == nullptr) return nullptr;
  if (delta_or_group >= 0) {
    (void)ctx->PopLimit(EpsCopyInputStream::LimitToken(delta_or_group));
    // if (!ctx->PopLimit(EpsCopyInputStream::LimitToken(delta_or_group))) return nullptr;
    return ptr;
  } else if (delta_or_group == -1) {
    return ptr;
  } else {
    return nullptr;
  } 

endgroup:
  if (delta_or_group != ~static_cast<uint64_t>(tag)) {
unusual_end:
    if (delta_or_group == -1) {
      ctx->SetLastTag(tag);
      return ptr;
    }
    return nullptr;
  }
  ctx->DecGroupDepth();
  return ptr;
}

std::string TypeCardToString(uint16_t type_card) {
  // In here we convert the runtime value of entry.type_card back into a
  // sequence of literal enum labels. We use the mnenonic labels for nicer
  // codegen.
  namespace fl = internal::field_layout;
  const int rep_index = (type_card & fl::kRepMask) >> fl::kRepShift;
  const int tv_index = (type_card & fl::kTvMask) >> fl::kTvShift;

  static constexpr const char* kFieldCardNames[] = {"Singular", "Optional",
                                                    "Repeated", "Oneof"};
  static_assert((fl::kFcSingular >> fl::kFcShift) == 0, "");
  static_assert((fl::kFcOptional >> fl::kFcShift) == 1, "");
  static_assert((fl::kFcRepeated >> fl::kFcShift) == 2, "");
  static_assert((fl::kFcOneof >> fl::kFcShift) == 3, "");

  std::string out;

  absl::StrAppend(&out, "::_fl::kFc",
                  kFieldCardNames[(type_card & fl::kFcMask) >> fl::kFcShift]);

#define PROTOBUF_INTERNAL_TYPE_CARD_CASE(x)  \
  case fl::k##x:                             \
    absl::StrAppend(&out, " | ::_fl::k" #x); \
    break

  switch (type_card & fl::kFkMask) {
    case fl::kFkString: {
      switch (type_card & ~fl::kFcMask & ~fl::kRepMask & ~fl::kSplitMask) {
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Bytes);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(RawString);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Utf8String);
        default:
          ABSL_LOG(FATAL) << "Unknown type_card: 0x" << type_card;
      }

      static constexpr const char* kRepNames[] = {"AString", "IString", "Cord",
                                                  "SPiece", "SString"};
      static_assert((fl::kRepAString >> fl::kRepShift) == 0, "");
      static_assert((fl::kRepIString >> fl::kRepShift) == 1, "");
      static_assert((fl::kRepCord >> fl::kRepShift) == 2, "");
      static_assert((fl::kRepSPiece >> fl::kRepShift) == 3, "");
      static_assert((fl::kRepSString >> fl::kRepShift) == 4, "");

      absl::StrAppend(&out, " | ::_fl::kRep", kRepNames[rep_index]);
      break;
    }

    case fl::kFkMessage: {
      absl::StrAppend(&out, " | ::_fl::kMessage");

      static constexpr const char* kRepNames[] = {nullptr, "Group", "Lazy"};
      static_assert((fl::kRepGroup >> fl::kRepShift) == 1, "");
      static_assert((fl::kRepLazy >> fl::kRepShift) == 2, "");

      if (auto* rep = kRepNames[rep_index]) {
        absl::StrAppend(&out, " | ::_fl::kRep", rep);
      }

      static constexpr const char* kXFormNames[2][4] = {
          {nullptr, "Default", "Table", "WeakPtr"}, {nullptr, "Eager", "Lazy"}};

      static_assert((fl::kTvDefault >> fl::kTvShift) == 1, "");
      static_assert((fl::kTvTable >> fl::kTvShift) == 2, "");
      static_assert((fl::kTvWeakPtr >> fl::kTvShift) == 3, "");
      static_assert((fl::kTvEager >> fl::kTvShift) == 1, "");
      static_assert((fl::kTvLazy >> fl::kTvShift) == 2, "");

      if (auto* xform = kXFormNames[rep_index == 2][tv_index]) {
        absl::StrAppend(&out, " | ::_fl::kTv", xform);
      }
      break;
    }

    case fl::kFkMap:
      absl::StrAppend(&out, " | ::_fl::kMap");
      break;

    case fl::kFkNone:
      break;

    case fl::kFkVarint:
    case fl::kFkPackedVarint:
    case fl::kFkFixed:
    case fl::kFkPackedFixed: {
      switch (type_card & ~fl::kFcMask & ~fl::kSplitMask) {
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Bool);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Fixed32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(UInt32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(SFixed32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Int32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(SInt32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Float);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Enum);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(EnumRange);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(OpenEnum);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Fixed64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(UInt64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(SFixed64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Int64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(SInt64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Double);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedBool);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedFixed32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedUInt32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedSFixed32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedInt32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedSInt32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedFloat);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedEnum);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedEnumRange);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedOpenEnum);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedFixed64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedUInt64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedSFixed64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedInt64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedSInt64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedDouble);
        default:
          ABSL_LOG(FATAL) << "Unknown type_card: 0x" << type_card;
      }
    }
  }

  if (type_card & fl::kSplitMask) {
    absl::StrAppend(&out, " | ::_fl::kSplitTrue");
  }

#undef PROTOBUF_INTERNAL_TYPE_CARD_CASE

  return out;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
