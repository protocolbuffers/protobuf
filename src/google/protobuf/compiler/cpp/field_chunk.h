#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_CHUNK_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_CHUNK_H__

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/cpp/field_layout.h"
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
  explicit FieldChunk(bool has_hasbit) : has_hasbit(has_hasbit) {}

  bool has_hasbit;

  std::vector<const FieldDescriptor*> fields;
};

inline FieldChunk ChunkWithFields(const FieldChunk& chunk,
                                  std::vector<const FieldDescriptor*> fields) {
  FieldChunk res(chunk.has_hasbit);
  res.fields = std::move(fields);
  return res;
}

inline const FieldChunk* GetPartitionableChunk(const FieldChunk& chunk) {
  return chunk.has_hasbit ? &chunk : nullptr;
}

PROTOC_EXPORT uint32_t
GenChunkMask(absl::Span<const FieldDescriptor* const> fields,
             const FieldLayout& field_layout);

struct AlwaysTruePred {
  template <typename T>
  bool operator()(const T&) const {
    return true;
  }
};

// Breaks down a single chunk of fields into a few chunks that share attributes
// projected by "proj". Returns an array of chunks.
template <typename Projector, typename Filter = AlwaysTruePred>
std::vector<FieldChunk> CollectFields(
    absl::Span<const FieldDescriptor* const> fields, const Options& options,
    const Projector proj, const Filter filter = {}) {
  std::vector<FieldChunk> chunks;
  bool force_new_chunk = true;
  for (auto field : fields) {
    if (!filter(field)) {
      // Chunks must be of consecutive fields.
      // If we skip any we have to start a new chunk.
      force_new_chunk = true;
      continue;
    }
    if (force_new_chunk || proj(chunks.back().fields.back()) != proj(field)) {
      force_new_chunk = false;
      chunks.emplace_back(HasHasbit(field, options));
    }
    chunks.back().fields.push_back(field);
  }
  return chunks;
}

namespace field_chunk_internal {

// Calculates the expected number of branch checks executed for a chunk of
// `size` fields, where `p_none` is the probability that none of the fields
// are present.
//
// For a single field (`size <= 1`), 1 branch is evaluated for its individual
// presence check. For `size > 1`, 1 branch is evaluated for the batch presence
// check, plus `size` subsequent individual branch checks evaluated when the
// batch check succeeds (with probability `p_any`).
inline double ExpectedNumberOfBranchesTaken(size_t size, double p_none) {
  if (size <= 1) return 1.0;

  double p_any = 1.0 - p_none;
  return 1.0 + size * p_any;
}

inline double GetAbsenceProbability(const FieldDescriptor* field,
                                    const Options& options) {
  // The default value of 0.0 is chosen for two reasons:
  // 1. We batch in other generated methods when the PdProto data isn't
  // available and the value of 0.0f promotes batching.
  // 2. It has shown the best performance in the protogen_micro benchmark when
  // compared do 1.0f and 0.5f.
  return 1.0 - GetPresenceProbability(field, options).value_or(0.0f);
}

}  // namespace field_chunk_internal

// Partitions the fields sharing a hasbit word into batches using dynamic
// programming to minimize the expected number of branch checks executed.
template <typename Chunk>
std::vector<Chunk> PartitionToMinimizeExpectedBranches(const Chunk& chunk,
                                                       const Options& options) {
  const auto& fields = chunk.fields;
  const int n = fields.size();
  if (n == 0) return {};

  const auto p_absent = [&](int i) {
    return field_chunk_internal::GetAbsenceProbability(fields[i], options);
  };
  // `min_cost[i]` is the minimum expected number of branch checks for the
  // suffix `fields[i..n)`.
  std::vector<double> min_cost(n + 1, 0.0);
  // `next_split[i]` is the exclusive end index `j` of the optimal first batch
  // `fields[i..j)` of the suffix `fields[i..n)`.
  std::vector<int> next_split(n, n);

  for (int i = n - 1; i >= 0; --i) {
    // Base candidate: put `fields[i]` in a single-field chunk `[i, i+1)`,
    // which always costs 1.0 branch check.
    min_cost[i] = 1.0 + min_cost[i + 1];
    next_split[i] = i + 1;
    double p_none = p_absent(i);
    // Try extending the batch starting at `i` to `[i, j)`.
    for (int j = i + 2; j <= n; ++j) {
      p_none *= p_absent(j - 1);
      double cost =
          field_chunk_internal::ExpectedNumberOfBranchesTaken(j - i, p_none) +
          min_cost[j];
      if (cost <= min_cost[i]) {
        min_cost[i] = cost;
        next_split[i] = j;
      }
    }
  }

  std::vector<Chunk> result;
  for (int i = 0; i < n; i = next_split[i]) {
    result.push_back(ChunkWithFields(
        chunk, {fields.begin() + i, fields.begin() + next_split[i]}));
  }
  return result;
}

template <typename Chunk>
std::vector<Chunk> PartitionToMinimizeExpectedBranches(
    std::vector<Chunk> chunks, const Options& options) {
  std::vector<Chunk> result;

  for (Chunk& chunk : chunks) {
    const auto* partitionable = GetPartitionableChunk(chunk);
    if (partitionable != nullptr && partitionable->fields.size() > 1) {
      absl::c_move(PartitionToMinimizeExpectedBranches(*partitionable, options),
                   std::back_inserter(result));
    } else {
      result.push_back(std::move(chunk));
    }
  }

  return result;
}
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_CHUNK_H__
