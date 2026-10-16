#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_CHUNK_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_CHUNK_H__

#include <cstdint>
#include <vector>

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
// controlled by "equivalent" predicate. Returns an array of chunks.
template <typename Predicate, typename Filter = AlwaysTruePred>
std::vector<FieldChunk> CollectFields(
    absl::Span<const FieldDescriptor* const> fields, const Options& options,
    const Predicate& equivalent, const Filter filter = {}) {
  std::vector<FieldChunk> chunks;
  bool force_new_chunk = true;
  for (auto field : fields) {
    if (!filter(field)) {
      // Chunks must be of consecutive fields.
      // If we skip any we have to start a new chunk.
      force_new_chunk = true;
      continue;
    }
    if (force_new_chunk || !equivalent(chunks.back().fields.back(), field)) {
      force_new_chunk = false;
      chunks.emplace_back(HasHasbit(field, options));
    }
    chunks.back().fields.push_back(field);
  }
  return chunks;
}
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_CHUNK_H__
