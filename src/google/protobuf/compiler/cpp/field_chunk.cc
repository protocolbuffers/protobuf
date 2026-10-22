#include "google/protobuf/compiler/cpp/field_chunk.h"

#include <cstdint>

#include "absl/log/absl_check.h"
#include "absl/numeric/bits.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/cpp/field_layout.h"
#include "google/protobuf/descriptor.h"

// must be last
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {

uint32_t GenRawChunkMask(absl::Span<const FieldDescriptor* const> fields,
                         int first_index_offset,
                         const FieldLayout& field_layout) {
  uint32_t chunk_mask = 0;
  for (auto field : fields) {
    // "bit_idx" defines where in the _has_bits_ the field appears.
    int bit_idx = field_layout.GetHasBitIndex(field).value();
    ABSL_CHECK_EQ(first_index_offset, bit_idx / 32);
    chunk_mask |= static_cast<uint32_t>(1) << (bit_idx % 32);
  }
  return chunk_mask;
}

// If the chunk covers all the fields sharing the same has_word, we return
// 0xFFFFFFFFU. This is a binary size optimization.
uint32_t SimplifyChunkMask(uint32_t chunk_mask, int has_word_index,
                           const FieldLayout& field_layout) {
  if (absl::popcount(chunk_mask) ==
      field_layout.CountFieldsSharingHasWord(has_word_index)) {
    return ~uint32_t{0};
  }
  return chunk_mask;
}

}  // namespace

// Returns a bit mask based on has_bit index of "fields" that are typically on
// the same chunk. It is used in a group presence check where _has_bits_ is
// masked to tell if any thing in "fields" is present.
uint32_t GenChunkMask(absl::Span<const FieldDescriptor* const> fields,
                      const FieldLayout& field_layout) {
  ABSL_CHECK_GT(fields.size(), 1u);

  int first_index_offset = field_layout.GetHasWordIndex(fields.front()).value();
  uint32_t chunk_mask =
      GenRawChunkMask(fields, first_index_offset, field_layout);

  return SimplifyChunkMask(chunk_mask, first_index_offset, field_layout);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
