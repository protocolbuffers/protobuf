#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_CHUNK_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_CHUNK_H__

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "absl/types/span.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"

// must be last
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

struct FieldChunk {
  FieldChunk(bool has_hasbit, bool is_rarely_present, bool should_split)
      : has_hasbit(has_hasbit),
        is_rarely_present(is_rarely_present),
        should_split(should_split) {}

  bool has_hasbit;
  bool is_rarely_present;
  bool should_split;

  std::vector<const FieldDescriptor*> fields;
};

using ChunkIterator = std::vector<FieldChunk>::iterator;

PROTOC_EXPORT uint32_t
GenChunkMask(absl::Span<const FieldDescriptor* const> fields,
             absl::Span<const int> has_bit_indices);

PROTOC_EXPORT uint32_t
GenChunkMask(const std::vector<const FieldDescriptor*>& fields,
             const std::vector<int>& has_bit_indices);

PROTOC_EXPORT uint32_t GenChunkMask(ChunkIterator it, ChunkIterator end,
                                    const std::vector<int>& has_bit_indices);


// Breaks down a single chunk of fields into a few chunks that share attributes
// controlled by "equivalent" predicate. Returns an array of chunks.
template <typename Predicate>
std::vector<FieldChunk> CollectFields(
    const std::vector<const FieldDescriptor*>& fields, const Options& options,
    const Predicate& equivalent) {
  std::vector<FieldChunk> chunks;
  for (auto field : fields) {
    if (chunks.empty() || !equivalent(chunks.back().fields.back(), field)) {
      chunks.emplace_back(internal::cpp::HasHasbit(field),
                          IsRarelyPresent(field, options),
                          ShouldSplit(field, options));
    }
    chunks.back().fields.push_back(field);
  }
  return chunks;
}

template <typename Predicate>
ChunkIterator FindNextUnequalChunk(ChunkIterator start, ChunkIterator end,
                                   const Predicate& equal) {
  auto it = start;
  while (++it != end) {
    if (!equal(*start, *it)) {
      return it;
    }
  }
  return end;
}

// Returns an array of field descriptors that meet "predicate" in [it, end). The
// matched fields are removed from the original range.
template <typename Predicate>
std::vector<const FieldDescriptor*> ExtractFields(ChunkIterator it,
                                                  ChunkIterator end,
                                                  const Predicate& predicate) {
  std::vector<const FieldDescriptor*> res;
  for (; it != end; ++it) {
    auto& source = it->fields;
    if (source.empty()) continue;

    for (const auto* f : source) {
      if (predicate(f)) {
        res.push_back(f);
      }
    }
    source.erase(std::remove_if(source.begin(), source.end(), predicate),
                 source.end());
  }
  return res;
}
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_CHUNK_H__
