#include "google/protobuf/compiler/cpp/field_chunk.h"

#include <cstdint>
#include <vector>

#include "absl/log/absl_check.h"
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

// Returns the (common) hasbit index offset. Callers must guarantee "fields" is
// not empty. Debug-check fails if it sees different hasbit index offset in
// "fields".
int GetCommonHasbitIndexOffset(absl::Span<const FieldDescriptor*> fields,
                               const FieldLayout& field_layout) {
  ABSL_CHECK(!fields.empty());

  int word_idx = field_layout.GetHasWordIndex(fields.front()).value();
  for (auto* field : fields) {
    ABSL_DCHECK_EQ(word_idx, field_layout.GetHasWordIndex(field).value());
  }
  return word_idx;
}

// Checks if chunks in [it, end) share hasbit index offset.
void CheckSameHasbitIndexOffset(ChunkIterator it, ChunkIterator end,
                                const FieldLayout& field_layout) {
  ABSL_CHECK(it != end);
  int prev_offset = -1;
  for (; it != end; ++it) {
    // Skip empty chunks (likely due to extraction).
    if (it->fields.empty()) continue;

    int offset =
        GetCommonHasbitIndexOffset(absl::MakeSpan(it->fields), field_layout);
    ABSL_CHECK(prev_offset == -1 || prev_offset == offset);
    prev_offset = offset;
  }
}

}  // namespace

// Returns a bit mask based on has_bit index of "fields" that are typically on
// the same chunk. It is used in a group presence check where _has_bits_ is
// masked to tell if any thing in "fields" is present.
uint32_t GenChunkMask(absl::Span<const FieldDescriptor* const> fields,
                      const FieldLayout& field_layout) {
  if (fields.empty()) return 0u;

  int first_index_offset = field_layout.GetHasWordIndex(fields.front()).value();
  uint32_t chunk_mask = 0;
  for (auto field : fields) {
    // "bit_idx" defines where in the _has_bits_ the field appears.
    int bit_idx = field_layout.GetHasBitIndex(field).value();
    ABSL_CHECK_EQ(first_index_offset, bit_idx / 32);
    chunk_mask |= static_cast<uint32_t>(1) << (bit_idx % 32);
  }
  ABSL_CHECK_NE(0u, chunk_mask);
  return chunk_mask;
}

// Returns a bit mask based on has_bit index of "fields" in chunks in [it, end).
// Assumes that all chunks share the same hasbit word.
uint32_t GenChunkMask(ChunkIterator it, ChunkIterator end,
                      const FieldLayout& field_layout) {
  ABSL_CHECK(it != end);

  CheckSameHasbitIndexOffset(it, end, field_layout);

  uint32_t chunk_mask = 0u;
  for (; it != end; ++it) {
    if (!it->fields.empty()) {
      chunk_mask |= GenChunkMask(it->fields, field_layout);
    }
  }
  return chunk_mask;
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
