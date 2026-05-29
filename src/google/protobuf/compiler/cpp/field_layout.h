#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_LAYOUT_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_LAYOUT_H__

#include <cstddef>
#include <vector>

#include "google/protobuf/compiler/split_map.h"
#include "absl/base/nullability.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

class FieldLayout {
 public:
  static FieldLayout BuildOptimizedLayout(
      const Descriptor* absl_nonnull descriptor, const Options& options,
      int& num_weak_fields);

  static FieldLayout BuildForTesting(std::vector<const FieldDescriptor*> fields,
                                     std::vector<int> has_bit_indices);

  FieldLayout(const FieldLayout&) = delete;
  FieldLayout& operator=(const FieldLayout&) = delete;

  // If true, this message needs a _has_bits_ field.
  bool HasHasbits() const;

  // Returns the number of 32-bit words needed for the _has_bits_ field.
  int HasBitsSize() const;

  // Returns the index of the hasbit for the given field, or absl::nullopt if
  // the field does not have a hasbit.
  absl::optional<int> GetHasBitIndex(const FieldDescriptor* field) const;

  absl::optional<int> GetHasByteIndex(const FieldDescriptor* field) const;

  // Returns the index into the _has_bits_ field that this field's hasbit
  // resides in, or absl::nullopt if the field does not have a hasbit.
  absl::optional<int> GetHasWordIndex(const FieldDescriptor* field) const;

  // This is the order we layout the message's fields in the class. This is
  // reused to initialize the fields in-order for cache efficiency.
  //
  // This list excludes oneof fields and weak fields.
  absl::Span<const FieldDescriptor* const> optimized_order() const {
    return fields_;
  }

  void PrintHasBitIndicesForSchema(io::Printer* p, size_t& entries) const;

  const SplitMap& split_map() const { return split_map_; }

  bool HasSplitFields() const { return split_map_.HasSplitFields(); }

  // Returns true if the given field is part of the split object.
  bool IsSplit(const FieldDescriptor* field) const;

 private:
  FieldLayout(std::vector<int> has_bit_indices, int max_has_bit_index,
              std::vector<const FieldDescriptor*> fields, SplitMap&& split_map);

  std::vector<int> has_bit_indices_;
  int max_has_bit_index_;

  std::vector<const FieldDescriptor*> fields_;

  SplitMap split_map_;
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_LAYOUT_H__
