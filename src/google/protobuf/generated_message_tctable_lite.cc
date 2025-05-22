// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>  // IWYU pragma: keep for operator new
#include <numeric>
#include <optional>
#include <string>
#include <type_traits>

#include "absl/base/optimization.h"
#include "absl/functional/overload.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/numeric/bits.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arenastring.h"
#include "google/protobuf/generated_enum_util.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/inlined_string_field.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/map.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/micro_string.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/serial_arena.h"
#include "google/protobuf/varint_shuffle.h"
#include "google/protobuf/wire_format_lite.h"
#include "utf8_validity.h"


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
[[noreturn]] void AlignFail(std::integral_constant<size_t, 4>,
                            std::uintptr_t address) {
  ABSL_LOG(FATAL) << "Unaligned (4) access at " << address;
}
[[noreturn]] void AlignFail(std::integral_constant<size_t, 8>,
                            std::uintptr_t address) {
  ABSL_LOG(FATAL) << "Unaligned (8) access at " << address;
}
#endif

const char* TcParser::GenericFallbackLite(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return GenericFallbackImpl<MessageLite, std::string>(
      PROTOBUF_TC_PARAM_PASS);
}

namespace {
bool ReadHas(const FieldEntry& entry, const MessageLite* msg) {
  auto has_idx = static_cast<uint32_t>(entry.has_idx);
  const auto& hasblock = TcParser::RefAt<const uint32_t>(msg, has_idx / 32 * 4);
  return (hasblock & (uint32_t{1} << (has_idx % 32))) != 0;
}
}  // namespace

void TcParser::VerifyHasBitConsistency(const MessageLite* msg,
                                       const TcParseTableBase* table) {
  namespace fl = internal::field_layout;
  if (table->has_bits_offset == 0) {
    // Nothing to check
    return;
  }

  for (const auto& entry : table->field_entries()) {
    const auto print_error = [&] {
      return absl::StrFormat("Type=%s Field=%d\n", msg->GetTypeName(),
                             FieldNumber(table, &entry));
    };
    if ((entry.type_card & fl::kFcMask) != fl::kFcOptional) return;
    const bool has_bit = ReadHas(entry, msg);
    const void* base = msg;
    const void* default_base = table->default_instance();
    if ((entry.type_card & field_layout::kSplitMask) ==
        field_layout::kSplitTrue) {
      const size_t offset = table->field_aux(kSplitOffsetAuxIdx)->offset;
      base = TcParser::RefAt<const void*>(base, offset);
      default_base = TcParser::RefAt<const void*>(default_base, offset);
    }
    switch (entry.type_card & fl::kFkMask) {
      case fl::kFkVarint:
      case fl::kFkFixed:
        // Numerics can have any value when the has bit is on.
        if (has_bit) return;
        switch (entry.type_card & fl::kRepMask) {
          case fl::kRep8Bits:
            ABSL_CHECK_EQ(RefAt<bool>(base, entry.offset),
                          RefAt<bool>(default_base, entry.offset))
                << print_error();
            break;
          case fl::kRep32Bits:
            ABSL_CHECK_EQ(RefAt<uint32_t>(base, entry.offset),
                          RefAt<uint32_t>(default_base, entry.offset))
                << print_error();
            break;
          case fl::kRep64Bits:
            ABSL_CHECK_EQ(RefAt<uint64_t>(base, entry.offset),
                          RefAt<uint64_t>(default_base, entry.offset))
                << print_error();
            break;
        }
        break;

      case fl::kFkString:
        switch (entry.type_card & fl::kRepMask) {
          case field_layout::kRepAString:
            if (has_bit) {
              // Must not point to the default.
              ABSL_CHECK(!RefAt<ArenaStringPtr>(base, entry.offset).IsDefault())
                  << print_error();
            } else {
              // We should technically check that the value matches the default
              // value of the field, but the prototype does not actually contain
              // this value. Non-empty defaults are loaded on access.
            }
            break;
          case field_layout::kRepCord:
            if (!has_bit) {
              // If the has bit is off, it must match the default.
              ABSL_CHECK_EQ(RefAt<absl::Cord>(base, entry.offset),
                            RefAt<absl::Cord>(default_base, entry.offset))
                  << print_error();
            }
            break;
          case field_layout::kRepIString:
            if (!has_bit) {
              // If the has bit is off, it must match the default.
              ABSL_CHECK_EQ(
                  RefAt<InlinedStringField>(base, entry.offset).Get(),
                  RefAt<InlinedStringField>(default_base, entry.offset).Get())
                  << print_error();
            }
            break;
          case field_layout::kRepSString:
            Unreachable();
        }
        break;
      case fl::kFkMessage:
        switch (entry.type_card & fl::kRepMask) {
          case fl::kRepMessage:
          case fl::kRepGroup:
            if (has_bit) {
              ABSL_CHECK(RefAt<const MessageLite*>(base, entry.offset) !=
                         nullptr)
                  << print_error();
            } else {
              // An off has_bit does not imply a null pointer.
              // We might have a previous instance that we cached.
            }
            break;
          default:
            Unreachable();
        }
        break;

      default:
        // All other types are not `optional`.
        Unreachable();
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// Core fast parsing implementation:
//////////////////////////////////////////////////////////////////////////////

PROTOBUF_NOINLINE const char* TcParser::ParseLoopPreserveNone(
    MessageLite* msg, const char* ptr, ParseContext* ctx,
    const TcParseTableBase* table) {
  return ParseLoop(msg, ptr, ctx, table);
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

  if (ABSL_PREDICT_TRUE(adj_fnum < 32)) {
    uint32_t skipmap = table->skipmap32;
    uint32_t skipbit = 1 << adj_fnum;
    if (ABSL_PREDICT_FALSE(skipmap & skipbit)) return nullptr;
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
    if (ABSL_PREDICT_TRUE(skip_num < num_skip_entries)) {
      // for each group of 16 fields we have:
      // a bitmap of 16 bits
      // a 16-bit field-entry offset for the first of them.
      auto* skip_data = lookup_table + (adj_fnum / 16) * (sizeof(SkipEntry16) /
                                                          sizeof(uint16_t));
      SkipEntry16 se = {skip_data[0], skip_data[1]};
      adj_fnum &= 15;
      uint32_t skipmap = se.skipmap;
      uint16_t skipbit = 1 << adj_fnum;
      if (ABSL_PREDICT_FALSE(skipmap & skipbit)) return nullptr;
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

int TcParser::FieldNumber(const TcParseTableBase* table,
                          const TcParseTableBase::FieldEntry* entry) {
  // The data structure was not designed to be queried in this direction, so
  // we have to do a linear search over the entries to see which one matches
  // while keeping track of the field number.
  // But it is fine because we are only using this for debug check messages.
  size_t need_to_skip = entry - table->field_entries_begin();
  const auto visit_bitmap = [&](uint32_t field_bitmap,
                                int base_field_number) -> std::optional<int> {
    for (; field_bitmap != 0; field_bitmap &= field_bitmap - 1) {
      if (need_to_skip == 0) {
        return absl::countr_zero(field_bitmap) + base_field_number;
      }
      --need_to_skip;
    }
    return std::nullopt;
  };
  if (auto number = visit_bitmap(~table->skipmap32, 1)) {
    return *number;
  }

  for (const uint16_t* lookup_table = table->field_lookup_begin();
       lookup_table[0] != 0xFFFF || lookup_table[1] != 0xFFFF;) {
    uint32_t fstart = lookup_table[0] | (lookup_table[1] << 16);
    lookup_table += 2;
    const uint16_t num_skip_entries = *lookup_table++;
    for (uint16_t i = 0; i < num_skip_entries; ++i) {
      // for each group of 16 fields we have: a
      // bitmap of 16 bits a 16-bit field-entry
      // offset for the first of them.
      if (auto number = visit_bitmap(static_cast<uint16_t>(~*lookup_table),
                                     fstart + 16 * i)) {
        return *number;
      }
      lookup_table += 2;
    }
  }
  Unreachable();
}

PROTOBUF_NOINLINE const char* TcParser::Error(PROTOBUF_TC_PARAM_NO_DATA_DECL) {
  (void)ctx;
  (void)ptr;
  SyncHasbits(msg, hasbits, table);
  return nullptr;
}

template <bool export_called_function>
PROTOBUF_ALWAYS_INLINE const char* TcParser::MiniParse(PROTOBUF_TC_PARAM_DECL) {
  TestMiniParseResult* test_out;
  if (export_called_function) {
    test_out = reinterpret_cast<TestMiniParseResult*>(
        static_cast<uintptr_t>(data.data));
  }

  uint32_t tag;
  ptr = ReadTagInlined(ptr, &tag);
  if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
    if (export_called_function) *test_out = {Error};
    PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }

  auto* entry = FindFieldEntry(table, tag >> 3);
  if (entry == nullptr) {
    if (export_called_function) *test_out = {table->fallback, tag};
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

  PROTOBUF_MUSTTAIL return parse_fn(PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::MiniParse(
    PROTOBUF_TC_PARAM_NO_DATA_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse<false>(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}
PROTOBUF_NOINLINE TcParser::TestMiniParseResult TcParser::TestMiniParse(
    PROTOBUF_TC_PARAM_DECL) {
  TestMiniParseResult result = {};
  data.data = reinterpret_cast<uintptr_t>(&result);
  result.ptr = MiniParse<true>(PROTOBUF_TC_PARAM_PASS);
  return result;
}

PROTOBUF_NOINLINE const char* TcParser::MpFallback(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType>
const char* TcParser::FastEndGroupImpl(PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  ctx->SetLastTag(data.decoded_tag());
  ptr += sizeof(TagType);
  PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastEndG1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return FastEndGroupImpl<uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastEndG2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return FastEndGroupImpl<uint16_t>(PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// Message fields
//////////////////////////////////////////////////////////////////////////////

PROTOBUF_ALWAYS_INLINE MessageLite* TcParser::NewMessage(
    const TcParseTableBase* table, Arena* arena) {
  return table->class_data->New(arena);
}

MessageLite* TcParser::AddMessage(const TcParseTableBase* table,
                                  RepeatedPtrFieldBase& field) {
  return field.AddFromPrototype<GenericTypeHandler<MessageLite>>(
      table->class_data->prototype);
}

template <typename TagType, bool group_coding, bool aux_is_table>
PROTOBUF_ALWAYS_INLINE const char* TcParser::SingularParseMessageAuxImpl(
    PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_PREFETCH_WITH_OFFSET(ptr, 192);
  PROTOBUF_PREFETCH_WITH_OFFSET(ptr, 256);
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  auto saved_tag = UnalignedLoad<TagType>(ptr);
  ptr += sizeof(TagType);
  hasbits |= (uint64_t{1} << data.hasbit_idx());
  SyncHasbits(msg, hasbits, table);
  auto& field = RefAt<MessageLite*>(msg, data.offset());
  const auto aux = *table->field_aux(data.aux_idx());
  const auto* inner_table =
      aux_is_table ? aux.table : aux.message_default()->GetTcParseTable();

  if (field == nullptr) {
    field = NewMessage(inner_table, msg->GetArena());
  }
  const auto inner_loop = [&](const char* ptr) {
    return ParseLoop(field, ptr, ctx, inner_table);
  };
  return group_coding
             ? ctx->ParseGroupInlined(ptr, FastDecodeTag(saved_tag), inner_loop)
             : ctx->ParseLengthDelimitedInlined(ptr, inner_loop);
}

PROTOBUF_NOINLINE const char* TcParser::FastMdS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularParseMessageAuxImpl<uint8_t, false, false>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastMdS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularParseMessageAuxImpl<uint16_t, false, false>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastGdS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularParseMessageAuxImpl<uint8_t, true, false>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastGdS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularParseMessageAuxImpl<uint16_t, true, false>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastMtS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularParseMessageAuxImpl<uint8_t, false, true>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastMtS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularParseMessageAuxImpl<uint16_t, false, true>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastGtS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularParseMessageAuxImpl<uint8_t, true, true>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastGtS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularParseMessageAuxImpl<uint16_t, true, true>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType>
const char* TcParser::LazyMessage(PROTOBUF_TC_PARAM_DECL) {
  ABSL_LOG(FATAL) << "Unimplemented";
  return nullptr;
}

PROTOBUF_NOINLINE const char* TcParser::FastMlS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return LazyMessage<uint8_t>(PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastMlS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return LazyMessage<uint16_t>(PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType, bool group_coding, bool aux_is_table>
PROTOBUF_ALWAYS_INLINE const char* TcParser::RepeatedParseMessageAuxImpl(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  PROTOBUF_PREFETCH_WITH_OFFSET(ptr, 256);
  const auto expected_tag = UnalignedLoad<TagType>(ptr);
  const auto aux = *table->field_aux(data.aux_idx());
  auto& field = RefAt<RepeatedPtrFieldBase>(msg, data.offset());
  const TcParseTableBase* inner_table =
      aux_is_table ? aux.table : aux.message_default()->GetTcParseTable();
  do {
    ptr += sizeof(TagType);
    MessageLite* submsg = AddMessage(inner_table, field);
    const auto inner_loop = [&](const char* ptr) {
      return ParseLoop(submsg, ptr, ctx, inner_table);
    };
    ptr = group_coding ? ctx->ParseGroupInlined(
                             ptr, FastDecodeTag(expected_tag), inner_loop)
                       : ctx->ParseLengthDelimitedInlined(ptr, inner_loop);
    if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
      PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
    }
    if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) {
      PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_NO_DATA_PASS);
    }
  } while (UnalignedLoad<TagType>(ptr) == expected_tag);

  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastMdR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedParseMessageAuxImpl<uint8_t, false, false>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastMdR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedParseMessageAuxImpl<uint16_t, false, false>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastGdR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedParseMessageAuxImpl<uint8_t, true, false>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastGdR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedParseMessageAuxImpl<uint16_t, true, false>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastMtR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedParseMessageAuxImpl<uint8_t, false, true>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastMtR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedParseMessageAuxImpl<uint16_t, false, true>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastGtR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedParseMessageAuxImpl<uint8_t, true, true>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastGtR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedParseMessageAuxImpl<uint16_t, true, true>(
      PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// Fixed fields
//////////////////////////////////////////////////////////////////////////////

template <typename LayoutType, typename TagType>
PROTOBUF_ALWAYS_INLINE const char* TcParser::SingularFixed(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  ptr += sizeof(TagType);  // Consume tag
  hasbits |= (uint64_t{1} << data.hasbit_idx());
  RefAt<LayoutType>(msg, data.offset()) = UnalignedLoad<LayoutType>(ptr);
  ptr += sizeof(LayoutType);
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastF32S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularFixed<uint32_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastF32S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularFixed<uint32_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastF64S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularFixed<uint64_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastF64S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularFixed<uint64_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename LayoutType, typename TagType>
PROTOBUF_ALWAYS_INLINE const char* TcParser::RepeatedFixed(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  auto& field = RefAt<RepeatedField<LayoutType>>(msg, data.offset());
  const auto tag = UnalignedLoad<TagType>(ptr);
  do {
    field.Add(UnalignedLoad<LayoutType>(ptr + sizeof(TagType)));
    ptr += sizeof(TagType) + sizeof(LayoutType);
    if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) {
      PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_NO_DATA_PASS);
    }
  } while (UnalignedLoad<TagType>(ptr) == tag);
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastF32R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedFixed<uint32_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastF32R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedFixed<uint32_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastF64R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedFixed<uint64_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastF64R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedFixed<uint64_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename LayoutType, typename TagType>
PROTOBUF_ALWAYS_INLINE const char* TcParser::PackedFixed(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  ptr += sizeof(TagType);
  // Since ctx->ReadPackedFixed does not use TailCall<> or Return<>, sync any
  // pending hasbits now:
  SyncHasbits(msg, hasbits, table);
  auto& field = RefAt<RepeatedField<LayoutType>>(msg, data.offset());
  int size = ReadSize(&ptr);
  // TODO: add a tailcalling variant of ReadPackedFixed.
  return ctx->ReadPackedFixed(ptr, size,
                              static_cast<RepeatedField<LayoutType>*>(&field));
}

PROTOBUF_NOINLINE const char* TcParser::FastF32P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedFixed<uint32_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastF32P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedFixed<uint32_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastF64P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedFixed<uint64_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastF64P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedFixed<uint64_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// Varint fields
//////////////////////////////////////////////////////////////////////////////

namespace {

template <typename Type>
PROTOBUF_ALWAYS_INLINE const char* ParseVarint(const char* p, Type* value) {
  static_assert(sizeof(Type) == 4 || sizeof(Type) == 8,
                "Only [u]int32_t and [u]int64_t please");
#ifdef __aarch64__
  // The VarintParse parser has a faster implementation on ARM.
  absl::conditional_t<sizeof(Type) == 4, uint32_t, uint64_t> tmp;
  p = VarintParse(p, &tmp);
  if (p != nullptr) {
    *value = tmp;
  }
  return p;
#endif
  int64_t res;
  p = ShiftMixParseVarint<Type>(p, res);
  *value = res;
  return p;
}

// This overload is specifically for handling bool, because bools have very
// different requirements and performance opportunities than ints.
PROTOBUF_ALWAYS_INLINE const char* ParseVarint(const char* p, bool* value) {
  unsigned char byte = static_cast<unsigned char>(*p++);
  if (ABSL_PREDICT_TRUE(byte == 0 || byte == 1)) {
    // This is the code path almost always taken,
    // so we take care to make it very efficient.
    if (sizeof(byte) == sizeof(*value)) {
      memcpy(value, &byte, 1);
    } else {
      // The C++ standard does not specify that a `bool` takes only one byte
      *value = byte;
    }
    return p;
  }
  // This part, we just care about code size.
  // Although it's almost never used, we have to support it because we guarantee
  // compatibility for users who change a field from an int32 or int64 to a bool
  if (ABSL_PREDICT_FALSE(byte & 0x80)) {
    byte = (byte - 0x80) | *p++;
    if (ABSL_PREDICT_FALSE(byte & 0x80)) {
      byte = (byte - 0x80) | *p++;
      if (ABSL_PREDICT_FALSE(byte & 0x80)) {
        byte = (byte - 0x80) | *p++;
        if (ABSL_PREDICT_FALSE(byte & 0x80)) {
          byte = (byte - 0x80) | *p++;
          if (ABSL_PREDICT_FALSE(byte & 0x80)) {
            byte = (byte - 0x80) | *p++;
            if (ABSL_PREDICT_FALSE(byte & 0x80)) {
              byte = (byte - 0x80) | *p++;
              if (ABSL_PREDICT_FALSE(byte & 0x80)) {
                byte = (byte - 0x80) | *p++;
                if (ABSL_PREDICT_FALSE(byte & 0x80)) {
                  byte = (byte - 0x80) | *p++;
                  if (ABSL_PREDICT_FALSE(byte & 0x80)) {
                    // We only care about the continuation bit and the first bit
                    // of the 10th byte.
                    byte = (byte - 0x80) | (*p++ & 0x81);
                    if (ABSL_PREDICT_FALSE(byte & 0x80)) {
                      return nullptr;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  *value = byte;
  return p;
}

template <typename FieldType, bool zigzag = false>
inline FieldType ZigZagDecodeHelper(FieldType value) {
  return static_cast<FieldType>(value);
}

template <>
inline int32_t ZigZagDecodeHelper<int32_t, true>(int32_t value) {
  return WireFormatLite::ZigZagDecode32(value);
}

template <>
inline int64_t ZigZagDecodeHelper<int64_t, true>(int64_t value) {
  return WireFormatLite::ZigZagDecode64(value);
}

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
    return aux.enum_range.first <= val && val <= aux.enum_range.last;
  }
  if (PROTOBUF_BUILTIN_CONSTANT_P(xform_val)) {
    return internal::ValidateEnumInlined(val, aux.enum_data);
  } else {
    return internal::ValidateEnum(val, aux.enum_data);
  }
}

}  // namespace

template <typename FieldType, typename TagType, bool zigzag>
PROTOBUF_ALWAYS_INLINE const char* TcParser::SingularVarint(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  ptr += sizeof(TagType);  // Consume tag
  hasbits |= (uint64_t{1} << data.hasbit_idx());

  // clang isn't smart enough to be able to only conditionally save
  // registers to the stack, so we turn the integer-greater-than-128
  // case into a separate routine.
  if (ABSL_PREDICT_FALSE(static_cast<int8_t>(*ptr) < 0)) {
    PROTOBUF_MUSTTAIL return SingularVarBigint<FieldType, TagType, zigzag>(
        PROTOBUF_TC_PARAM_PASS);
  }

  RefAt<FieldType>(msg, data.offset()) =
      ZigZagDecodeHelper<FieldType, zigzag>(static_cast<uint8_t>(*ptr++));
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

template <typename FieldType, typename TagType, bool zigzag>
PROTOBUF_NOINLINE const char* TcParser::SingularVarBigint(
    PROTOBUF_TC_PARAM_DECL) {
  uint64_t tmp;
  PROTOBUF_ASSUME(static_cast<int8_t>(*ptr) < 0);
  ptr = ParseVarint(ptr, &tmp);

  if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
    PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  RefAt<FieldType>(msg, data.offset()) =
      ZigZagDecodeHelper<FieldType, zigzag>(tmp);
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

template <typename FieldType>
PROTOBUF_ALWAYS_INLINE const char* TcParser::FastVarintS1(
    PROTOBUF_TC_PARAM_DECL) {
  using TagType = uint8_t;
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  int64_t res;
  ptr = ShiftMixParseVarint<FieldType>(ptr + sizeof(TagType), res);
  if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
    PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  hasbits |= (uint64_t{1} << data.hasbit_idx());
  RefAt<FieldType>(msg, data.offset()) = res;
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastV8S1(PROTOBUF_TC_PARAM_DECL) {
  using TagType = uint8_t;

  // Special case for a varint bool field with a tag of 1 byte:
  // The coded_tag() field will actually contain the value too and we can check
  // both at the same time.
  auto coded_tag = data.coded_tag<uint16_t>();
  if (ABSL_PREDICT_TRUE(coded_tag == 0x0000 || coded_tag == 0x0100)) {
    auto& field = RefAt<bool>(msg, data.offset());
    // Note: we use `data.data` because Clang generates suboptimal code when
    // using coded_tag.
    // In x86_64 this uses the CH register to read the second byte out of
    // `data`.
    uint8_t value = data.data >> 8;
    // The assume allows using a mov instead of test+setne.
    PROTOBUF_ASSUME(value <= 1);
    field = static_cast<bool>(value);

    ptr += sizeof(TagType) + 1;  // Consume the tag and the value.
    hasbits |= (uint64_t{1} << data.hasbit_idx());

    PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }

  // If it didn't match above either the tag is wrong, or the value is encoded
  // non-canonically.
  // Jump to MiniParse as wrong tag is the most probable reason.
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastV8S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<bool, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV32S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return FastVarintS1<uint32_t>(PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV32S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<uint32_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV64S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return FastVarintS1<uint64_t>(PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV64S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<uint64_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastZ32S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<int32_t, uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastZ32S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<int32_t, uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastZ64S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<int64_t, uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastZ64S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularVarint<int64_t, uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename FieldType, typename TagType, bool zigzag>
PROTOBUF_ALWAYS_INLINE const char* TcParser::RepeatedVarint(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  auto& field = RefAt<RepeatedField<FieldType>>(msg, data.offset());
  const auto expected_tag = UnalignedLoad<TagType>(ptr);
  do {
    ptr += sizeof(TagType);
    FieldType tmp;
    ptr = ParseVarint(ptr, &tmp);
    if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
      PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
    }
    field.Add(ZigZagDecodeHelper<FieldType, zigzag>(tmp));
    if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) {
      PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_NO_DATA_PASS);
    }
  } while (UnalignedLoad<TagType>(ptr) == expected_tag);
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastV8R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<bool, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV8R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<bool, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV32R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<uint32_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV32R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<uint32_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV64R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<uint64_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV64R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<uint64_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastZ32R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<int32_t, uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastZ32R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<int32_t, uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastZ64R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<int64_t, uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastZ64R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedVarint<int64_t, uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename FieldType, typename TagType, bool zigzag>
PROTOBUF_ALWAYS_INLINE const char* TcParser::PackedVarint(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
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

PROTOBUF_NOINLINE const char* TcParser::FastV8P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<bool, uint8_t>(PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV8P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<bool, uint16_t>(PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV32P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<uint32_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV32P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<uint32_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV64P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<uint64_t, uint8_t>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastV64P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<uint64_t, uint16_t>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastZ32P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<int32_t, uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastZ32P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<int32_t, uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastZ64P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<int64_t, uint8_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastZ64P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedVarint<int64_t, uint16_t, true>(
      PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// Enum fields
//////////////////////////////////////////////////////////////////////////////

PROTOBUF_NOINLINE const char* TcParser::FastUnknownEnumFallback(
    PROTOBUF_TC_PARAM_DECL) {
  // Skip MiniParse/fallback and insert the element directly into the unknown
  // field set. We also normalize the value into an int32 as we do for known
  // enum values.
  uint32_t tag;
  ptr = ReadTag(ptr, &tag);
  if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
    PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  uint64_t tmp;
  ptr = ParseVarint(ptr, &tmp);
  if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
    PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  AddUnknownEnum(msg, table, tag, static_cast<int32_t>(tmp));
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::MpUnknownEnumFallback(
    PROTOBUF_TC_PARAM_DECL) {
  // Like FastUnknownEnumFallback, but with the Mp ABI.
  uint32_t tag = data.tag();
  uint64_t tmp;
  ptr = ParseVarint(ptr, &tmp);
  if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
    PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  AddUnknownEnum(msg, table, tag, static_cast<int32_t>(tmp));
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

template <typename TagType, uint16_t xform_val>
PROTOBUF_ALWAYS_INLINE const char* TcParser::SingularEnum(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  const TcParseTableBase::FieldAux aux = *table->field_aux(data.aux_idx());
  PrefetchEnumData(xform_val, aux);
  const char* ptr2 = ptr;  // Save for unknown enum case
  ptr += sizeof(TagType);  // Consume tag
  uint64_t tmp;
  ptr = ParseVarint(ptr, &tmp);
  if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
    PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  if (ABSL_PREDICT_FALSE(
          !EnumIsValidAux(static_cast<int32_t>(tmp), xform_val, aux))) {
    ptr = ptr2;
    PROTOBUF_MUSTTAIL return FastUnknownEnumFallback(PROTOBUF_TC_PARAM_PASS);
  }
  hasbits |= (uint64_t{1} << data.hasbit_idx());
  RefAt<int32_t>(msg, data.offset()) = tmp;
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastErS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularEnum<uint8_t, field_layout::kTvRange>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastErS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularEnum<uint16_t, field_layout::kTvRange>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastEvS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularEnum<uint8_t, field_layout::kTvEnum>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastEvS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularEnum<uint16_t, field_layout::kTvEnum>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType, uint16_t xform_val>
PROTOBUF_ALWAYS_INLINE const char* TcParser::RepeatedEnum(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  auto& field = RefAt<RepeatedField<int32_t>>(msg, data.offset());
  const auto expected_tag = UnalignedLoad<TagType>(ptr);
  const TcParseTableBase::FieldAux aux = *table->field_aux(data.aux_idx());
  PrefetchEnumData(xform_val, aux);
  do {
    const char* ptr2 = ptr;  // save for unknown enum case
    ptr += sizeof(TagType);
    uint64_t tmp;
    ptr = ParseVarint(ptr, &tmp);
    if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
      PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
    }
    if (ABSL_PREDICT_FALSE(
            !EnumIsValidAux(static_cast<int32_t>(tmp), xform_val, aux))) {
      // We can avoid duplicate work in MiniParse by directly calling
      // table->fallback.
      ptr = ptr2;
      PROTOBUF_MUSTTAIL return FastUnknownEnumFallback(PROTOBUF_TC_PARAM_PASS);
    }
    field.Add(static_cast<int32_t>(tmp));
    if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) {
      PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_NO_DATA_PASS);
    }
  } while (UnalignedLoad<TagType>(ptr) == expected_tag);

  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

const TcParser::UnknownFieldOps& TcParser::GetUnknownFieldOps(
    const TcParseTableBase* table) {
  // Call the fallback function in a special mode to only act as a
  // way to return the ops.
  // Hiding the unknown fields vtable behind the fallback function avoids adding
  // more pointers in TcParseTableBase, and the extra runtime jumps are not
  // relevant because unknown fields are rare.
  const char* ptr = table->fallback(nullptr, nullptr, nullptr, {}, nullptr, 0);
  return *reinterpret_cast<const UnknownFieldOps*>(ptr);
}

PROTOBUF_NOINLINE void TcParser::AddUnknownEnum(MessageLite* msg,
                                                const TcParseTableBase* table,
                                                uint32_t tag,
                                                int32_t enum_value) {
  GetUnknownFieldOps(table).write_varint(msg, tag >> 3, enum_value);
}

template <typename TagType, uint16_t xform_val>
PROTOBUF_ALWAYS_INLINE const char* TcParser::PackedEnum(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  const auto saved_tag = UnalignedLoad<TagType>(ptr);
  ptr += sizeof(TagType);
  // Since ctx->ReadPackedVarint does not use TailCall or Return, sync any
  // pending hasbits now:
  SyncHasbits(msg, hasbits, table);
  auto* field = &RefAt<RepeatedField<int32_t>>(msg, data.offset());
  const TcParseTableBase::FieldAux aux = *table->field_aux(data.aux_idx());
  PrefetchEnumData(xform_val, aux);
  return ctx->ReadPackedVarint(ptr, [=](int32_t value) {
    if (!EnumIsValidAux(value, xform_val, aux)) {
      AddUnknownEnum(msg, table, FastDecodeTag(saved_tag), value);
    } else {
      field->Add(value);
    }
  });
}

PROTOBUF_NOINLINE const char* TcParser::FastErR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedEnum<uint8_t, field_layout::kTvRange>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastErR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedEnum<uint16_t, field_layout::kTvRange>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastEvR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedEnum<uint8_t, field_layout::kTvEnum>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastEvR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedEnum<uint16_t, field_layout::kTvEnum>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastErP1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedEnum<uint8_t, field_layout::kTvRange>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastErP2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedEnum<uint16_t, field_layout::kTvRange>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastEvP1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedEnum<uint8_t, field_layout::kTvEnum>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastEvP2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedEnum<uint16_t, field_layout::kTvEnum>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType, uint8_t min>
PROTOBUF_ALWAYS_INLINE const char* TcParser::SingularEnumSmallRange(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }

  uint8_t v = ptr[sizeof(TagType)];
  if (ABSL_PREDICT_FALSE(min > v || v > data.aux_idx())) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }

  RefAt<int32_t>(msg, data.offset()) = v;
  ptr += sizeof(TagType) + 1;
  hasbits |= (uint64_t{1} << data.hasbit_idx());
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastEr0S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularEnumSmallRange<uint8_t, 0>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastEr0S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularEnumSmallRange<uint16_t, 0>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastEr1S1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularEnumSmallRange<uint8_t, 1>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastEr1S2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularEnumSmallRange<uint16_t, 1>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType, uint8_t min>
PROTOBUF_ALWAYS_INLINE const char* TcParser::RepeatedEnumSmallRange(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  auto& field = RefAt<RepeatedField<int32_t>>(msg, data.offset());
  auto expected_tag = UnalignedLoad<TagType>(ptr);
  const uint8_t max = data.aux_idx();
  do {
    uint8_t v = ptr[sizeof(TagType)];
    if (ABSL_PREDICT_FALSE(min > v || v > max)) {
      PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
    }
    field.Add(static_cast<int32_t>(v));
    ptr += sizeof(TagType) + 1;
    if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) {
      PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_NO_DATA_PASS);
    }
  } while (UnalignedLoad<TagType>(ptr) == expected_tag);

  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastEr0R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedEnumSmallRange<uint8_t, 0>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastEr0R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedEnumSmallRange<uint16_t, 0>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastEr1R1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedEnumSmallRange<uint8_t, 1>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastEr1R2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedEnumSmallRange<uint16_t, 1>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType, uint8_t min>
PROTOBUF_ALWAYS_INLINE const char* TcParser::PackedEnumSmallRange(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }

  // Since ctx->ReadPackedVarint does not use TailCall or Return, sync any
  // pending hasbits now:
  SyncHasbits(msg, hasbits, table);

  const auto saved_tag = UnalignedLoad<TagType>(ptr);
  ptr += sizeof(TagType);
  auto* field = &RefAt<RepeatedField<int32_t>>(msg, data.offset());
  const uint8_t max = data.aux_idx();

  return ctx->ReadPackedVarint(
      ptr,
      [=](int32_t v) {
        if (ABSL_PREDICT_FALSE(min > v || v > max)) {
          AddUnknownEnum(msg, table, FastDecodeTag(saved_tag), v);
        } else {
          field->Add(v);
        }
      },
      /*size_callback=*/
      [=](int32_t size_bytes) {
        // For enums that fit in one varint byte, optimistically assume that all
        // the values are one byte long (i.e. no large unknown values).  If so,
        // we know exactly how many values we're going to get.
        //
        // But! size_bytes might be much larger than the total size of the
        // serialized proto (e.g. input corruption, or parsing msg1 as msg2).
        // We don't want a small serialized proto to lead to giant memory
        // allocations.
        //
        // Ideally we'd restrict size_bytes to the total size of the input, but
        // we don't know that value.  The best we can do is to restrict it to
        // the remaining bytes in the chunk, plus a "benefit of the doubt"
        // factor if we're very close to the end of the chunk.
        //
        // Do these calculations in int64 because it's possible we overflow
        // int32 (imgaine that field->size() and size_bytes are both large).
        int64_t new_size =
            int64_t{field->size()} +
            std::min(size_bytes, std::max(1024, ctx->MaximumReadSize(ptr)));
        field->Reserve(static_cast<int32_t>(
            std::min(new_size, int64_t{std::numeric_limits<int32_t>::max()})));
      });
}

PROTOBUF_NOINLINE const char* TcParser::FastEr0P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedEnumSmallRange<uint8_t, 0>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastEr0P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedEnumSmallRange<uint16_t, 0>(
      PROTOBUF_TC_PARAM_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastEr1P1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedEnumSmallRange<uint8_t, 1>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastEr1P2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return PackedEnumSmallRange<uint16_t, 1>(
      PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// String/bytes fields
//////////////////////////////////////////////////////////////////////////////

// Defined in wire_format_lite.cc
void PrintUTF8ErrorLog(absl::string_view message_name,
                       absl::string_view field_name, const char* operation_str,
                       bool emit_stacktrace);

void TcParser::ReportFastUtf8Error(uint32_t decoded_tag,
                                   const TcParseTableBase* table) {
  uint32_t field_num = decoded_tag >> 3;
  const auto* entry = FindFieldEntry(table, field_num);
  PrintUTF8ErrorLog(MessageName(table), FieldName(table, entry), "parsing",
                    false);
}

namespace {

// Here are overloads of ReadStringIntoArena, ReadStringNoArena and IsValidUTF8
// for every string class for which we provide fast-table parser support.

PROTOBUF_ALWAYS_INLINE const char* ReadStringIntoArena(
    MessageLite* /*msg*/, const char* ptr, ParseContext* ctx,
    uint32_t /*aux_idx*/, const TcParseTableBase* /*table*/,
    ArenaStringPtr& field, Arena* arena) {
  return ctx->ReadArenaString(ptr, &field, arena);
}

PROTOBUF_NOINLINE
const char* ReadStringNoArena(MessageLite* /*msg*/, const char* ptr,
                              ParseContext* ctx, uint32_t /*aux_idx*/,
                              const TcParseTableBase* /*table*/,
                              ArenaStringPtr& field) {
  int size = ReadSize(&ptr);
  if (!ptr) return nullptr;
  return ctx->ReadString(ptr, size, field.MutableNoCopy(nullptr));
}

PROTOBUF_ALWAYS_INLINE bool IsValidUTF8(ArenaStringPtr& field) {
  return utf8_range::IsStructurallyValid(field.Get());
}


PROTOBUF_ALWAYS_INLINE const char* ReadStringIntoArena(
    MessageLite* /* msg */, const char* ptr, ParseContext* ctx,
    uint32_t /* aux_idx */, const TcParseTableBase* /* table */,
    MicroString& field, Arena* arena) {
  return ctx->ReadMicroString(ptr, field, arena);
}

PROTOBUF_ALWAYS_INLINE const char* ReadStringNoArena(
    MessageLite* /* msg */, const char* ptr, ParseContext* ctx,
    uint32_t /* aux_idx */, const TcParseTableBase* /* table */,
    MicroString& field) {
  return ctx->ReadMicroString(ptr, field, nullptr);
}

PROTOBUF_ALWAYS_INLINE bool IsValidUTF8(const MicroString& field) {
  return utf8_range::IsStructurallyValid(field.Get());
}

void EnsureArenaStringIsNotDefault(const MessageLite* msg,
                                   ArenaStringPtr* field) {
  // If we failed here we might have left the string in its IsDefault state, but
  // already set the has bit which breaks the message invariants. We must make
  // it consistent again. We do that by guaranteeing the string always exists.
  if (field->IsDefault()) {
    field->Set("", msg->GetArena());
  }
}
// The rest do nothing.
[[maybe_unused]] void EnsureArenaStringIsNotDefault(const MessageLite* msg,
                                                    void*) {}

}  // namespace

template <typename TagType, typename FieldType, TcParser::Utf8Type utf8>
PROTOBUF_ALWAYS_INLINE const char* TcParser::SingularString(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  auto saved_tag = UnalignedLoad<TagType>(ptr);
  ptr += sizeof(TagType);
  hasbits |= (uint64_t{1} << data.hasbit_idx());
  auto& field = RefAt<FieldType>(msg, data.offset());
  auto arena = msg->GetArena();
  if (arena) {
    ptr =
        ReadStringIntoArena(msg, ptr, ctx, data.aux_idx(), table, field, arena);
  } else {
    ptr = ReadStringNoArena(msg, ptr, ctx, data.aux_idx(), table, field);
  }
  if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
    EnsureArenaStringIsNotDefault(msg, &field);
    PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  switch (utf8) {
    case kNoUtf8:
#ifdef NDEBUG
    case kUtf8ValidateOnly:
#endif
      break;
    default:
      if (ABSL_PREDICT_TRUE(IsValidUTF8(field))) {
        break;
      }
      ReportFastUtf8Error(FastDecodeTag(saved_tag), table);
      if (utf8 == kUtf8) {
        PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
      }
      break;
  }

  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastBS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint8_t, ArenaStringPtr, kNoUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastBS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint16_t, ArenaStringPtr, kNoUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastSS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint8_t, ArenaStringPtr,
                                          kUtf8ValidateOnly>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastSS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint16_t, ArenaStringPtr,
                                          kUtf8ValidateOnly>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastUS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint8_t, ArenaStringPtr, kUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastUS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint16_t, ArenaStringPtr, kUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}

// Inlined string variants:

const char* TcParser::FastBiS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}
const char* TcParser::FastBiS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}
const char* TcParser::FastSiS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}
const char* TcParser::FastSiS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}
const char* TcParser::FastUiS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}
const char* TcParser::FastUiS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

// Corded string variants:
const char* TcParser::FastBcS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}
const char* TcParser::FastBcS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}
const char* TcParser::FastScS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}
const char* TcParser::FastScS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}
const char* TcParser::FastUcS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}
const char* TcParser::FastUcS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

// MicroString variants:
PROTOBUF_NOINLINE const char* TcParser::FastBmS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint8_t, MicroString, kNoUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastBmS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint16_t, MicroString, kNoUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastSmS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint8_t, MicroString,
                                          kUtf8ValidateOnly>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastSmS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint16_t, MicroString,
                                          kUtf8ValidateOnly>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastUmS1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint8_t, MicroString, kUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastUmS2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return SingularString<uint16_t, MicroString, kUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}

template <typename TagType, typename FieldType, TcParser::Utf8Type utf8>
PROTOBUF_ALWAYS_INLINE const char* TcParser::RepeatedString(
    PROTOBUF_TC_PARAM_DECL) {
  if (ABSL_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  const auto expected_tag = UnalignedLoad<TagType>(ptr);
  auto& field = RefAt<FieldType>(msg, data.offset());

  const auto validate_last_string = [expected_tag, table, &field] {
    switch (utf8) {
      case kNoUtf8:
#ifdef NDEBUG
      case kUtf8ValidateOnly:
#endif
        return true;
      default:
        if (ABSL_PREDICT_TRUE(
                utf8_range::IsStructurallyValid(field[field.size() - 1]))) {
          return true;
        }
        ReportFastUtf8Error(FastDecodeTag(expected_tag), table);
        if (utf8 == kUtf8) return false;
        return true;
    }
  };

  auto* arena = field.GetArena();
  SerialArena* serial_arena;
  if (ABSL_PREDICT_TRUE(arena != nullptr &&
                        arena->impl_.GetSerialArenaFast(&serial_arena) &&
                        field.PrepareForParse())) {
    do {
      ptr += sizeof(TagType);
      ptr = ParseRepeatedStringOnce(ptr, serial_arena, ctx, field);

      if (ABSL_PREDICT_FALSE(ptr == nullptr || !validate_last_string())) {
        PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
      }
      if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
    } while (UnalignedLoad<TagType>(ptr) == expected_tag);
  } else {
    do {
      ptr += sizeof(TagType);
      std::string* str = field.Add();
      ptr = InlineGreedyStringParser(str, ptr, ctx);
      if (ABSL_PREDICT_FALSE(ptr == nullptr || !validate_last_string())) {
        PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
      }
      if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
    } while (UnalignedLoad<TagType>(ptr) == expected_tag);
  }
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
parse_loop:
  PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_NOINLINE const char* TcParser::FastBR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedString<
      uint8_t, RepeatedPtrField<std::string>, kNoUtf8>(PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastBR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedString<
      uint16_t, RepeatedPtrField<std::string>, kNoUtf8>(PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastSR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedString<
      uint8_t, RepeatedPtrField<std::string>, kUtf8ValidateOnly>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastSR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedString<
      uint16_t, RepeatedPtrField<std::string>, kUtf8ValidateOnly>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastUR1(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedString<uint8_t,
                                          RepeatedPtrField<std::string>, kUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}
PROTOBUF_NOINLINE const char* TcParser::FastUR2(PROTOBUF_TC_PARAM_DECL) {
  PROTOBUF_MUSTTAIL return RepeatedString<uint16_t,
                                          RepeatedPtrField<std::string>, kUtf8>(
      PROTOBUF_TC_PARAM_PASS);
}

//////////////////////////////////////////////////////////////////////////////
// Mini parsing
//////////////////////////////////////////////////////////////////////////////

namespace {
inline void SetHas(const FieldEntry& entry, MessageLite* msg) {
  auto has_idx = static_cast<uint32_t>(entry.has_idx);
  auto& hasblock = TcParser::RefAt<uint32_t>(msg, has_idx / 32 * 4);
  hasblock |= uint32_t{1} << (has_idx % 32);
}
}  // namespace

void TcParser::InitOneof(const TcParseTableBase* table,
                         const TcParseTableBase* inner_table,
                         const TcParseTableBase::FieldEntry& entry,
                         MessageLite* msg) {
  uint16_t kind = entry.type_card & field_layout::kFkMask;
  uint16_t rep = entry.type_card & field_layout::kRepMask;
  if (kind == field_layout::kFkString) {
    switch (rep) {
      case field_layout::kRepAString: {
        RefAt<ArenaStringPtr>(msg, entry.offset).InitDefault();
        break;
      }
      case field_layout::kRepMString: {
        RefAt<MicroString>(msg, entry.offset).InitDefault();
        break;
      }
      case field_layout::kRepCord: {
        absl::Cord* field = Arena::Create<absl::Cord>(msg->GetArena());
        RefAt<absl::Cord*>(msg, entry.offset) = field;
        break;
      }
      case field_layout::kRepSString:
      case field_layout::kRepIString:
      default:
        internal::Unreachable();
        return;
    }
  } else if (kind == field_layout::kFkMessage) {
    switch (rep) {
      case field_layout::kRepMessage:
      case field_layout::kRepGroup: {
        auto& field = RefAt<MessageLite*>(msg, entry.offset);
        field = NewMessage(inner_table, msg->GetArena());
        break;
      }
      default:
        internal::Unreachable();
        return;
    }
  }
}

// Destroys any existing oneof union member (if necessary). Initializes the
// oneof field if the caller is responsible for initializing the object, or does
// not perform initialization if the field already has the desired case.
void TcParser::ChangeOneof(const TcParseTableBase* table,
                           const TcParseTableBase* inner_table,
                           const TcParseTableBase::FieldEntry& entry,
                           uint32_t field_num, ParseContext* ctx,
                           MessageLite* msg) {
  // The _oneof_case_ value offset is stored in the has-bit index.
  uint32_t* oneof_case = &TcParser::RefAt<uint32_t>(msg, entry.has_idx);
  uint32_t current_case = *oneof_case;
  *oneof_case = field_num;

  // If the member is already active, then it should be merged. We're done.
  if (current_case == field_num) return;

  if (current_case == 0) {
    // If the member is empty, we don't have anything to clear.
    // We must create a new member object.
    InitOneof(table, inner_table, entry, msg);
    return;
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
      case field_layout::kRepMString: {
        if (msg->GetArena() == nullptr) {
          RefAt<MicroString>(msg, current_entry->offset).Destroy();
        }
        break;
      }
      case field_layout::kRepCord: {
        if (msg->GetArena() == nullptr) {
          delete RefAt<absl::Cord*>(msg, current_entry->offset);
        }
        break;
      }
      case field_layout::kRepSString:
      case field_layout::kRepIString:
      default:
        internal::Unreachable();
        return;
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
        internal::Unreachable();
        return;
    }
  }
  InitOneof(table, inner_table, entry, msg);
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
        TcParser::RefAt<void*>(table->default_instance(), split_offset);
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
PROTOBUF_NOINLINE const char* TcParser::MpFixed(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint16_t card = type_card & field_layout::kFcMask;

  // Check for repeated parsing (wiretype fallback is handled there):
  if (card == field_layout::kFcRepeated) {
    PROTOBUF_MUSTTAIL return MpRepeatedFixed<is_split>(PROTOBUF_TC_PARAM_PASS);
  }
  // Check for mismatched wiretype:
  const uint16_t rep = type_card & field_layout::kRepMask;
  const uint32_t decoded_wiretype = data.tag() & 7;
  if (rep == field_layout::kRep64Bits) {
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED64) {
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
  } else {
    ABSL_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep32Bits));
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED32) {
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
  }
  // Set the field present:
  if (card == field_layout::kFcOptional) {
    SetHas(entry, msg);
  } else if (card == field_layout::kFcOneof) {
    ChangeOneof(table, /*inner_table=*/nullptr, entry, data.tag() >> 3, ctx,
                msg);
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
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpRepeatedFixed(
    PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint32_t decoded_tag = data.tag();
  const uint32_t decoded_wiretype = decoded_tag & 7;

  // Check for packed repeated fallback:
  if (decoded_wiretype == WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return MpPackedFixed<is_split>(PROTOBUF_TC_PARAM_PASS);
  }

  void* const base = MaybeGetSplitBase(msg, is_split, table);
  const uint16_t type_card = entry.type_card;
  const uint16_t rep = type_card & field_layout::kRepMask;
  if (rep == field_layout::kRep64Bits) {
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED64) {
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
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
      if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
      ptr2 = ReadTag(ptr, &next_tag);
      if (ABSL_PREDICT_FALSE(ptr2 == nullptr)) goto error;
    } while (next_tag == decoded_tag);
  } else {
    ABSL_DCHECK_EQ(rep, static_cast<uint16_t>(field_layout::kRep32Bits));
    if (decoded_wiretype != WireFormatLite::WIRETYPE_FIXED32) {
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
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
      if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
      ptr2 = ReadTag(ptr, &next_tag);
      if (ABSL_PREDICT_FALSE(ptr2 == nullptr)) goto error;
    } while (next_tag == decoded_tag);
  }

  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
parse_loop:
  PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_NO_DATA_PASS);
error:
  PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpPackedFixed(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint32_t decoded_wiretype = data.tag() & 7;

  // Check for non-packed repeated fallback:
  if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return MpRepeatedFixed<is_split>(PROTOBUF_TC_PARAM_PASS);
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

  if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
    PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpVarint(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint16_t card = type_card & field_layout::kFcMask;

  // Check for repeated parsing:
  if (card == field_layout::kFcRepeated) {
    PROTOBUF_MUSTTAIL return MpRepeatedVarint<is_split>(PROTOBUF_TC_PARAM_PASS);
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
  if (ptr == nullptr) {
    PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
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
        PROTOBUF_MUSTTAIL return MpUnknownEnumFallback(PROTOBUF_TC_PARAM_PASS);
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
    ChangeOneof(table, /*inner_table=*/nullptr, entry, data.tag() >> 3, ctx,
                msg);
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

  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

template <bool is_split, typename FieldType, uint16_t xform_val_in>
const char* TcParser::MpRepeatedVarintT(PROTOBUF_TC_PARAM_DECL) {
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
    ptr = ParseVarint(ptr2, &tmp);
    if (ABSL_PREDICT_FALSE(ptr == nullptr)) goto error;
    if (is_validated_enum) {
      if (!EnumIsValidAux(static_cast<int32_t>(tmp), xform_val, aux)) {
        ptr = ptr2;
        PROTOBUF_MUSTTAIL return MpUnknownEnumFallback(PROTOBUF_TC_PARAM_PASS);
      }
    } else if (is_zigzag) {
      tmp = sizeof(FieldType) == 8 ? WireFormatLite::ZigZagDecode64(tmp)
                                   : WireFormatLite::ZigZagDecode32(tmp);
    }
    field.Add(static_cast<FieldType>(tmp));
    if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
    ptr2 = ReadTag(ptr, &next_tag);
    if (ABSL_PREDICT_FALSE(ptr2 == nullptr)) goto error;
  } while (next_tag == decoded_tag);

parse_loop:
  PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_NO_DATA_PASS);
error:
  PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpRepeatedVarint(
    PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const auto type_card = entry.type_card;
  const uint32_t decoded_tag = data.tag();
  const auto decoded_wiretype = decoded_tag & 7;

  // Check for packed repeated fallback:
  if (decoded_wiretype == WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return MpPackedVarint<is_split>(PROTOBUF_TC_PARAM_PASS);
  }
  // Check for wire type mismatch:
  if (decoded_wiretype != WireFormatLite::WIRETYPE_VARINT) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }
  // For split we avoid the duplicate code and have the impl reload the value.
  // Less code bloat for uncommon paths.
  const uint16_t xform_val = (type_card & field_layout::kTvMask);
  const uint16_t rep = type_card & field_layout::kRepMask;
  switch (rep >> field_layout::kRepShift) {
    case field_layout::kRep64Bits >> field_layout::kRepShift:
      if (xform_val == 0) {
        PROTOBUF_MUSTTAIL return MpRepeatedVarintT<is_split, uint64_t, 0>(
            PROTOBUF_TC_PARAM_PASS);
      } else {
        ABSL_DCHECK_EQ(xform_val, +field_layout::kTvZigZag);
        PROTOBUF_MUSTTAIL return MpRepeatedVarintT<
            is_split, uint64_t, (is_split ? 0 : field_layout::kTvZigZag)>(
            PROTOBUF_TC_PARAM_PASS);
      }
    case field_layout::kRep32Bits >> field_layout::kRepShift:
      switch (xform_val >> field_layout::kTvShift) {
        case 0:
          PROTOBUF_MUSTTAIL return MpRepeatedVarintT<is_split, uint32_t, 0>(
              PROTOBUF_TC_PARAM_PASS);
        case field_layout::kTvZigZag >> field_layout::kTvShift:
          PROTOBUF_MUSTTAIL return MpRepeatedVarintT<
              is_split, uint32_t, (is_split ? 0 : field_layout::kTvZigZag)>(
              PROTOBUF_TC_PARAM_PASS);
        case field_layout::kTvEnum >> field_layout::kTvShift:
          PROTOBUF_MUSTTAIL return MpRepeatedVarintT<
              is_split, uint32_t, (is_split ? 0 : field_layout::kTvEnum)>(
              PROTOBUF_TC_PARAM_PASS);
        case field_layout::kTvRange >> field_layout::kTvShift:
          PROTOBUF_MUSTTAIL return MpRepeatedVarintT<
              is_split, uint32_t, (is_split ? 0 : field_layout::kTvRange)>(
              PROTOBUF_TC_PARAM_PASS);
        default:
          Unreachable();
      }
    case field_layout::kRep8Bits >> field_layout::kRepShift:
      PROTOBUF_MUSTTAIL return MpRepeatedVarintT<is_split, bool, 0>(
          PROTOBUF_TC_PARAM_PASS);

    default:
      Unreachable();
      return nullptr;  // To silence -Werror=return-type in some toolchains
  }
}

template <bool is_split, typename FieldType, uint16_t xform_val_in>
const char* TcParser::MpPackedVarintT(PROTOBUF_TC_PARAM_DECL) {
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
PROTOBUF_NOINLINE const char* TcParser::MpPackedVarint(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const auto type_card = entry.type_card;
  const auto decoded_wiretype = data.tag() & 7;

  // Check for non-packed repeated fallback:
  if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return MpRepeatedVarint<is_split>(PROTOBUF_TC_PARAM_PASS);
  }

  // For split we avoid the duplicate code and have the impl reload the value.
  // Less code bloat for uncommon paths.
  const uint16_t xform_val = (type_card & field_layout::kTvMask);

  // Since ctx->ReadPackedFixed does not use TailCall<> or Return<>, sync any
  // pending hasbits now:
  SyncHasbits(msg, hasbits, table);

  const uint16_t rep = type_card & field_layout::kRepMask;

  switch (rep >> field_layout::kRepShift) {
    case field_layout::kRep64Bits >> field_layout::kRepShift:
      if (xform_val == 0) {
        PROTOBUF_MUSTTAIL return MpPackedVarintT<is_split, uint64_t, 0>(
            PROTOBUF_TC_PARAM_PASS);
      } else {
        ABSL_DCHECK_EQ(xform_val, +field_layout::kTvZigZag);
        PROTOBUF_MUSTTAIL return MpPackedVarintT<
            is_split, uint64_t, (is_split ? 0 : field_layout::kTvZigZag)>(
            PROTOBUF_TC_PARAM_PASS);
      }
    case field_layout::kRep32Bits >> field_layout::kRepShift:
      switch (xform_val >> field_layout::kTvShift) {
        case 0:
          PROTOBUF_MUSTTAIL return MpPackedVarintT<is_split, uint32_t, 0>(
              PROTOBUF_TC_PARAM_PASS);
        case field_layout::kTvZigZag >> field_layout::kTvShift:
          PROTOBUF_MUSTTAIL return MpPackedVarintT<
              is_split, uint32_t, (is_split ? 0 : field_layout::kTvZigZag)>(
              PROTOBUF_TC_PARAM_PASS);
        case field_layout::kTvEnum >> field_layout::kTvShift:
          PROTOBUF_MUSTTAIL return MpPackedVarintT<
              is_split, uint32_t, (is_split ? 0 : field_layout::kTvEnum)>(
              PROTOBUF_TC_PARAM_PASS);
        case field_layout::kTvRange >> field_layout::kTvShift:
          PROTOBUF_MUSTTAIL return MpPackedVarintT<
              is_split, uint32_t, (is_split ? 0 : field_layout::kTvRange)>(
              PROTOBUF_TC_PARAM_PASS);
        default:
          Unreachable();
      }
    case field_layout::kRep8Bits >> field_layout::kRepShift:
      PROTOBUF_MUSTTAIL return MpPackedVarintT<is_split, bool, 0>(
          PROTOBUF_TC_PARAM_PASS);

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
PROTOBUF_NOINLINE const char* TcParser::MpString(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint16_t card = type_card & field_layout::kFcMask;
  const uint32_t decoded_wiretype = data.tag() & 7;

  if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }
  if (card == field_layout::kFcRepeated) {
    PROTOBUF_MUSTTAIL return MpRepeatedString<is_split>(PROTOBUF_TC_PARAM_PASS);
  }
  const uint16_t xform_val = type_card & field_layout::kTvMask;
  const uint16_t rep = type_card & field_layout::kRepMask;

  // Mark the field as present:
  const bool is_oneof = card == field_layout::kFcOneof;
  if (card == field_layout::kFcOptional) {
    SetHas(entry, msg);
  } else if (is_oneof) {
    ChangeOneof(table, /*inner_table=*/nullptr, entry, data.tag() >> 3, ctx,
                msg);
  }

  bool is_valid = false;
  void* const base = MaybeGetSplitBase(msg, is_split, table);
  switch (rep) {
    case field_layout::kRepAString: {
      auto& field = RefAt<ArenaStringPtr>(base, entry.offset);
      Arena* arena = msg->GetArena();
      if (arena) {
        ptr = ctx->ReadArenaString(ptr, &field, arena);
      } else {
        std::string* str = field.MutableNoCopy(nullptr);
        ptr = InlineGreedyStringParser(str, ptr, ctx);
      }
      if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
        EnsureArenaStringIsNotDefault(msg, &field);
        break;
      }
      is_valid = MpVerifyUtf8(field.Get(), table, entry, xform_val);
      break;
    }

    case field_layout::kRepMString: {
      auto& field = RefAt<MicroString>(base, entry.offset);
      ptr = ctx->ReadMicroString(ptr, field, msg->GetArena());
      is_valid = MpVerifyUtf8(field.Get(), table, entry, xform_val);
      break;
    }


    case field_layout::kRepCord: {
      absl::Cord* field;
      if (is_oneof) {
        field = RefAt<absl::Cord*>(msg, entry.offset);
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

  if (ABSL_PREDICT_FALSE(ptr == nullptr || !is_valid)) {
    PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  }
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

PROTOBUF_ALWAYS_INLINE const char* TcParser::ParseRepeatedStringOnce(
    const char* ptr, SerialArena* serial_arena, ParseContext* ctx,
    RepeatedPtrField<std::string>& field) {
  int size = ReadSize(&ptr);
  if (ABSL_PREDICT_FALSE(!ptr)) return {};
  auto* str = new (serial_arena->AllocateFromStringBlock()) std::string();
  field.AddAllocatedForParse(str);
  ptr = ctx->ReadString(ptr, size, str);
  if (ABSL_PREDICT_FALSE(!ptr)) return {};
  PROTOBUF_ASSUME(ptr != nullptr);
  return ptr;
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpRepeatedString(
    PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint32_t decoded_tag = data.tag();
  const uint32_t decoded_wiretype = decoded_tag & 7;

  if (decoded_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
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
      if (ABSL_PREDICT_TRUE(arena != nullptr &&
                            arena->impl_.GetSerialArenaFast(&serial_arena) &&
                            field.PrepareForParse())) {
        do {
          ptr = ptr2;
          ptr = ParseRepeatedStringOnce(ptr, serial_arena, ctx, field);
          if (ABSL_PREDICT_FALSE(ptr == nullptr ||
                                 !MpVerifyUtf8(field[field.size() - 1], table,
                                               entry, xform_val))) {
            PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
          }
          if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
          ptr2 = ReadTag(ptr, &next_tag);
        } while (next_tag == decoded_tag);
      } else {
        do {
          ptr = ptr2;
          std::string* str = field.Add();
          ptr = InlineGreedyStringParser(str, ptr, ctx);
          if (ABSL_PREDICT_FALSE(
                  ptr == nullptr ||
                  !MpVerifyUtf8(*str, table, entry, xform_val))) {
            PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
          }
          if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
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

  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
parse_loop:
  PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}


inline const TcParseTableBase* TcParser::GetTableFromAux(
    uint16_t type_card, TcParseTableBase::FieldAux aux) {
  uint16_t tv = type_card & field_layout::kTvMask;
  if (ABSL_PREDICT_TRUE(tv == field_layout::kTvTable)) {
    return aux.table;
  }
  ABSL_DCHECK(tv == field_layout::kTvDefault || tv == field_layout::kTvWeakPtr);
  const MessageLite* prototype = tv == field_layout::kTvDefault
                                     ? aux.message_default()
                                     : aux.message_default_weak();
  return prototype->GetTcParseTable();
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpMessage(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  const uint16_t type_card = entry.type_card;
  const uint16_t card = type_card & field_layout::kFcMask;

  // Check for repeated parsing:
  if (card == field_layout::kFcRepeated) {
    const uint16_t rep = type_card & field_layout::kRepMask;
    switch (rep) {
      case field_layout::kRepMessage:
        PROTOBUF_MUSTTAIL return MpRepeatedMessageOrGroup<is_split, false>(
            PROTOBUF_TC_PARAM_PASS);
      case field_layout::kRepGroup:
        PROTOBUF_MUSTTAIL return MpRepeatedMessageOrGroup<is_split, true>(
            PROTOBUF_TC_PARAM_PASS);
      default:
        PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
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
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
  }

  const TcParseTableBase* inner_table =
      GetTableFromAux(type_card, *table->field_aux(&entry));

  const bool is_oneof = card == field_layout::kFcOneof;
  if (card == field_layout::kFcOptional) {
    SetHas(entry, msg);
  } else if (is_oneof) {
    ChangeOneof(table, inner_table, entry, data.tag() >> 3, ctx, msg);
  }

  SyncHasbits(msg, hasbits, table);

  void* const base = MaybeGetSplitBase(msg, is_split, table);
  MessageLite*& field = RefAt<MessageLite*>(base, entry.offset);
  if (field == nullptr) {
    field = NewMessage(inner_table, msg->GetArena());
  }
  const auto inner_loop = [&](const char* ptr) {
    return ParseLoopPreserveNone(field, ptr, ctx, inner_table);
  };
  return is_group ? ctx->ParseGroupInlined(ptr, decoded_tag, inner_loop)
                  : ctx->ParseLengthDelimitedInlined(ptr, inner_loop);
}

template <bool is_split, bool is_group>
const char* TcParser::MpRepeatedMessageOrGroup(PROTOBUF_TC_PARAM_DECL) {
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
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
  } else {
    ABSL_DCHECK_EQ(type_card & field_layout::kRepMask,
                   static_cast<uint16_t>(field_layout::kRepGroup));
    if (decoded_wiretype != WireFormatLite::WIRETYPE_START_GROUP) {
      PROTOBUF_MUSTTAIL return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
  }

  void* const base = MaybeGetSplitBase(msg, is_split, table);
  RepeatedPtrFieldBase& field =
      MaybeCreateRepeatedRefAt<RepeatedPtrFieldBase, is_split>(
          base, entry.offset, msg);
  const TcParseTableBase* inner_table =
      GetTableFromAux(type_card, *table->field_aux(&entry));

  const char* ptr2 = ptr;
  uint32_t next_tag;
  do {
    MessageLite* value = AddMessage(inner_table, field);
    const auto inner_loop = [&](const char* ptr) {
      return ParseLoopPreserveNone(value, ptr, ctx, inner_table);
    };
    ptr = is_group ? ctx->ParseGroupInlined(ptr2, decoded_tag, inner_loop)
                   : ctx->ParseLengthDelimitedInlined(ptr2, inner_loop);
    if (ABSL_PREDICT_FALSE(ptr == nullptr)) goto error;
    if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) goto parse_loop;
    ptr2 = ReadTag(ptr, &next_tag);
    if (ABSL_PREDICT_FALSE(ptr2 == nullptr)) goto error;
  } while (next_tag == decoded_tag);
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
parse_loop:
  PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_NO_DATA_PASS);
error:
  PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

static void SerializeMapKey(UntypedMapBase& map, NodeBase* node,
                            MapTypeCard type_card,
                            io::CodedOutputStream& coded_output) {
  switch (type_card.wiretype()) {
    case WireFormatLite::WIRETYPE_VARINT:
      map.VisitKey(node,  //
                   absl::Overload{
                       [&](const bool* v) {
                         WireFormatLite::WriteBool(1, *v, &coded_output);
                       },
                       [&](const uint32_t* v) {
                         if (type_card.is_zigzag()) {
                           WireFormatLite::WriteSInt32(1, *v, &coded_output);
                         } else if (type_card.is_signed()) {
                           WireFormatLite::WriteInt32(1, *v, &coded_output);
                         } else {
                           WireFormatLite::WriteUInt32(1, *v, &coded_output);
                         }
                       },
                       [&](const uint64_t* v) {
                         if (type_card.is_zigzag()) {
                           WireFormatLite::WriteSInt64(1, *v, &coded_output);
                         } else if (type_card.is_signed()) {
                           WireFormatLite::WriteInt64(1, *v, &coded_output);
                         } else {
                           WireFormatLite::WriteUInt64(1, *v, &coded_output);
                         }
                       },
                       [](const void*) { Unreachable(); },
                   });
      break;
    case WireFormatLite::WIRETYPE_FIXED32:
      WireFormatLite::WriteFixed32(1, *map.GetKey<uint32_t>(node),
                                   &coded_output);
      break;
    case WireFormatLite::WIRETYPE_FIXED64:
      WireFormatLite::WriteFixed64(1, *map.GetKey<uint64_t>(node),
                                   &coded_output);
      break;
    case WireFormatLite::WIRETYPE_LENGTH_DELIMITED:
      // We should never have a message here. They can only be values maps.
      WireFormatLite::WriteString(1, *map.GetKey<std::string>(node),
                                  &coded_output);
      break;
    default:
      Unreachable();
  }
}

void TcParser::WriteMapEntryAsUnknown(MessageLite* msg,
                                      const TcParseTableBase* table,
                                      UntypedMapBase& map, uint32_t tag,
                                      NodeBase* node, MapAuxInfo map_info) {
  std::string serialized;
  {
    io::StringOutputStream string_output(&serialized);
    io::CodedOutputStream coded_output(&string_output);
    SerializeMapKey(map, node, map_info.key_type_card, coded_output);
    // The mapped_type is always an enum here.
    ABSL_DCHECK(map_info.value_is_validated_enum);
    WireFormatLite::WriteInt32(2, *map.GetValue<int32_t>(node), &coded_output);
  }
  GetUnknownFieldOps(table).write_length_delimited(msg, tag >> 3, serialized);

  if (map.arena() == nullptr) {
    map.DeleteNode(node);
  }
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
    const TcParseTableBase::FieldEntry& entry, UntypedMapBase& map) {
  using WFL = WireFormatLite;

  const auto map_info = aux[0].map_info;
  const uint8_t key_tag = map_info.key_type_card.tag();
  const uint8_t value_tag = map_info.value_type_card.tag();

  while (!ctx->Done(&ptr)) {
    uint32_t inner_tag = ptr[0];

    if (ABSL_PREDICT_FALSE(inner_tag != key_tag && inner_tag != value_tag)) {
      // Do a full parse and check again in case the tag has non-canonical
      // encoding.
      ptr = ReadTag(ptr, &inner_tag);
      if (ABSL_PREDICT_FALSE(inner_tag != key_tag && inner_tag != value_tag)) {
        if (ABSL_PREDICT_FALSE(ptr == nullptr)) return nullptr;

        if (inner_tag == 0 || (inner_tag & 7) == WFL::WIRETYPE_END_GROUP) {
          ctx->SetLastTag(inner_tag);
          break;
        }

        ptr = UnknownFieldParse(inner_tag, nullptr, ptr, ctx);
        if (ABSL_PREDICT_FALSE(ptr == nullptr)) return nullptr;
        continue;
      }
    } else {
      ++ptr;
    }

    MapTypeCard type_card;
    UntypedMapBase::TypeKind type_kind;
    void* obj;
    if (inner_tag == key_tag) {
      type_card = map_info.key_type_card;
      type_kind = map.type_info().key_type_kind();
      obj = node->GetVoidKey();
    } else {
      type_card = map_info.value_type_card;
      type_kind = map.type_info().value_type_kind();
      obj = map.GetVoidValue(node);
    }

    switch (inner_tag & 7) {
      case WFL::WIRETYPE_VARINT:
        uint64_t tmp;
        ptr = ParseVarint(ptr, &tmp);
        if (ABSL_PREDICT_FALSE(ptr == nullptr)) return nullptr;
        switch (type_kind) {
          case UntypedMapBase::TypeKind::kBool:
            *reinterpret_cast<bool*>(obj) = static_cast<bool>(tmp);
            continue;
          case UntypedMapBase::TypeKind::kU32: {
            uint32_t v = static_cast<uint32_t>(tmp);
            if (type_card.is_zigzag()) v = WFL::ZigZagDecode32(v);
            memcpy(obj, &v, sizeof(v));
            continue;
          }
          case UntypedMapBase::TypeKind::kU64:
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
        if (type_kind == UntypedMapBase::TypeKind::kString) {
          const int size = ReadSize(&ptr);
          if (ABSL_PREDICT_FALSE(ptr == nullptr)) return nullptr;
          std::string* str = reinterpret_cast<std::string*>(obj);
          ptr = ctx->ReadString(ptr, size, str);
          if (ABSL_PREDICT_FALSE(ptr == nullptr)) return nullptr;
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
          ABSL_DCHECK_EQ(static_cast<int>(type_kind),
                         static_cast<int>(UntypedMapBase::TypeKind::kMessage));
          ABSL_DCHECK_EQ(inner_tag, value_tag);
          ptr = ctx->ParseLengthDelimitedInlined(ptr, [&](const char* ptr) {
            return ParseLoop(reinterpret_cast<MessageLite*>(obj), ptr, ctx,
                             aux[1].table);
          });
          if (ABSL_PREDICT_FALSE(ptr == nullptr)) return nullptr;
          continue;
        }
      default:
        Unreachable();
    }
  }
  return ptr;
}

template <bool is_split>
PROTOBUF_NOINLINE const char* TcParser::MpMap(PROTOBUF_TC_PARAM_DECL) {
  const auto& entry = RefAt<FieldEntry>(table, data.entry_offset());
  // `aux[0]` points into a MapAuxInfo.
  // If we have a message mapped_type aux[1] points into a `create_in_arena`.
  // If we have a validated enum mapped_type aux[1] point into a
  // `enum_data`.
  const auto* aux = table->field_aux(&entry);
  const auto map_info = aux[0].map_info;

  if (ABSL_PREDICT_FALSE(!map_info.is_supported ||
                         (data.tag() & 7) !=
                             WireFormatLite::WIRETYPE_LENGTH_DELIMITED)) {
    PROTOBUF_MUSTTAIL return MpFallback(PROTOBUF_TC_PARAM_PASS);
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
    NodeBase* node = map.AllocNode();
    char* const node_end =
        reinterpret_cast<char*>(node) + map.type_info().node_size;
    void* const node_key = node->GetVoidKey();

    // Due to node alignment we can guarantee that we have at least 8 writable
    // bytes from the key position to the end of the node.
    // We can initialize the first and last 8 bytes, which takes care of all the
    // scalar value types. This makes the VisitXXX calls below faster because
    // the switch is much smaller.
    // Assert this in debug mode, just in case.
    ABSL_DCHECK_GE(node_end - static_cast<char*>(node_key), sizeof(uint64_t));
    memset(node_key, 0, sizeof(uint64_t));
    memset(node_end - sizeof(uint64_t), 0, sizeof(uint64_t));

    map.VisitKey(  //
        node, absl::Overload{
                  [&](std::string* str) {
                    Arena::CreateInArenaStorage(str, map.arena());
                  },
                  // Already initialized above. Do nothing here.
                  [](void*) {},
              });

    map.VisitValue(  //
        node, absl::Overload{
                  [&](std::string* str) {
                    Arena::CreateInArenaStorage(str, map.arena());
                  },
                  [&](MessageLite* msg) {
                    aux[1].table->class_data->PlacementNew(msg, map.arena());
                  },
                  // Already initialized above. Do nothing here.
                  [](void*) {},
              });

    ptr = ctx->ParseLengthDelimitedInlined(ptr, [&](const char* ptr) {
      return ParseOneMapEntry(node, ptr, ctx, aux, table, entry, map);
    });

    if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
      // Parsing failed. Delete the node that we didn't insert.
      if (map.arena() == nullptr) map.DeleteNode(node);
      PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
    }

    if (ABSL_PREDICT_FALSE(
            map_info.value_is_validated_enum &&
            !internal::ValidateEnumInlined(*map.GetValue<int32_t>(node),
                                           aux[1].enum_data))) {
      WriteMapEntryAsUnknown(msg, table, map, saved_tag, node, map_info);
    } else {
      // Done parsing the node, insert it.
      switch (map.type_info().key_type_kind()) {
        case UntypedMapBase::TypeKind::kBool:
          static_cast<KeyMapBase<bool>&>(map).InsertOrReplaceNode(
              static_cast<KeyMapBase<bool>::KeyNode*>(node));
          break;
        case UntypedMapBase::TypeKind::kU32:
          static_cast<KeyMapBase<uint32_t>&>(map).InsertOrReplaceNode(
              static_cast<KeyMapBase<uint32_t>::KeyNode*>(node));
          break;
        case UntypedMapBase::TypeKind::kU64:
          static_cast<KeyMapBase<uint64_t>&>(map).InsertOrReplaceNode(
              static_cast<KeyMapBase<uint64_t>::KeyNode*>(node));
          break;
        case UntypedMapBase::TypeKind::kString:
          static_cast<KeyMapBase<std::string>&>(map).InsertOrReplaceNode(
              static_cast<KeyMapBase<std::string>::KeyNode*>(node));
          break;
        default:
          Unreachable();
      }
    }

    if (ABSL_PREDICT_FALSE(!ctx->DataAvailable(ptr))) {
      PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_NO_DATA_PASS);
    }

    uint32_t next_tag;
    const char* ptr2 = ReadTagInlined(ptr, &next_tag);
    if (next_tag != saved_tag) break;
    ptr = ptr2;
  }

  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_NO_DATA_PASS);
}

const char* TcParser::MessageSetWireFormatParseLoopLite(
    PROTOBUF_TC_PARAM_NO_DATA_DECL) {
  PROTOBUF_MUSTTAIL return MessageSetWireFormatParseLoopImpl<MessageLite>(
      PROTOBUF_TC_PARAM_NO_DATA_PASS);
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

      static constexpr const char* kRepNames[] = {
          "AString", "IString", "Cord", "SPiece", "SString", "MString"};
      static_assert((fl::kRepAString >> fl::kRepShift) == 0, "");
      static_assert((fl::kRepIString >> fl::kRepShift) == 1, "");
      static_assert((fl::kRepCord >> fl::kRepShift) == 2, "");
      static_assert((fl::kRepSPiece >> fl::kRepShift) == 3, "");
      static_assert((fl::kRepSString >> fl::kRepShift) == 4, "");
      static_assert((fl::kRepMString >> fl::kRepShift) == 5, "");

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

const char* TcParser::DiscardEverythingFallback(PROTOBUF_TC_PARAM_DECL) {
  SyncHasbits(msg, hasbits, table);
  uint32_t tag = data.tag();
  if ((tag & 7) == WireFormatLite::WIRETYPE_END_GROUP || tag == 0) {
    ctx->SetLastTag(tag);
    return ptr;
  }
  return UnknownFieldParse(tag, nullptr, ptr, ctx);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
