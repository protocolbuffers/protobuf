// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: seongkim@google.com (Seong Beom Kim)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_LAYOUT_HELPER_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_LAYOUT_HELPER_H__

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_tctable_gen.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

class MessageSCCAnalyzer;

class FieldGroup {
 public:
  FieldGroup()
      : fields_(),
        preferred_location_(0),
        estimated_memory_size_(0) {
  }

  // A group with a single field.
  FieldGroup(float preferred_location, const FieldDescriptor* field,
             uint64_t num_accesses = 0)
      : fields_(1, field),
        preferred_location_(preferred_location),
        estimated_memory_size_(
            static_cast<uint32_t>(EstimateAlignmentSize(field))) {
  }

  const std::vector<const FieldDescriptor*>& fields() const { return fields_; }

  size_t num_fields() const { return fields_.size(); }

  size_t estimated_memory_size() const {
    return static_cast<size_t>(estimated_memory_size_);
  }

  void SetPreferredLocation(double location) { preferred_location_ = location; }

  // Appends the fields in 'other' to this group. Access count is incremented.
  void Append(const FieldGroup& other);

  // Sorts by their preferred location.
  bool operator<(const FieldGroup& other) const;

 private:
  bool UpdatePreferredLocationAndInsertOtherFields(const FieldGroup& other);

  std::vector<const FieldDescriptor*> fields_;
  float preferred_location_;
  // An estimate of the total memory size of the fields in this group, ignoring
  // padding/alignment.
  //
  // Since fields are added to groups in descending size order, and the sizes
  // are all powers of 2, this will be the true size of the group, ignoring the
  // padding at the end. This padding at the end can be filled by other field
  // groups so long as the size of the other group is at most
  // `8 - EstimateMemorySize()`.
  uint32_t estimated_memory_size_;
};

// Provides an abstract interface to optimize message layout
// by rearranging the fields of a message.
class MessageLayoutHelper {
 public:
  using FieldVector = std::vector<const FieldDescriptor*>;

  explicit MessageLayoutHelper(const Descriptor* descriptor)
      : descriptor_(descriptor) {}
  virtual ~MessageLayoutHelper() = default;

  const Descriptor* descriptor() const { return descriptor_; }

  virtual FieldVector OptimizeLayout(const FieldVector& fields,
                                     const Options& options,
                                     MessageSCCAnalyzer* scc_analyzer) const {
    return DoOptimizeLayout(fields, options, scc_analyzer);
  }

 protected:
  // TODO: Merge kCold and kSplit once all field types can be
  // split.
  enum FieldHotness {
    kSplit,
    kCold,
    kWarm,
    kHot,
    kFastParse,  // Fast-parse eligible fields.
    kRepeated,   // Non-split repeated fields.
    kMaxHotness,
  };

  // Reorder 'fields' so that if the fields are output into a C++ class in the
  // new order, fields of similar family (see below) are together and within
  // each family, alignment padding is minimized.
  //
  // We try to do this while keeping each field as close as possible to its
  // field number order (or access count for profiled protos) so that we don't
  // reduce cache locality much for function that access each field in order.
  // Originally, OptimizePadding used declaration order for its decisions, but
  // generated code minus the serializer/parsers uses the output of
  // OptimizePadding as well (stored in MessageGenerator::optimized_order_).
  // Since the serializers use field number order, we use that as a tie-breaker.
  //
  // We classify each field into a particular "family" of fields, that we
  // perform the same operation on in our generated functions.
  //
  // REPEATED is placed first, as the C++ compiler automatically initializes
  // these fields in layout order.
  //
  // STRING is grouped next, as our Clear/SharedCtor/SharedDtor walks it and
  // calls ArenaStringPtr::Destroy on each.
  //
  // MESSAGE is grouped next, as our Clear/SharedDtor code walks it and calls
  // delete on each.  We initialize these fields with a NULL pointer (see
  // MessageFieldGenerator::GenerateConstructorCode), which allows them to be
  // memset.
  //
  // ZERO_INITIALIZABLE is memset in Clear/SharedCtor
  //
  // OTHER these fields are initialized one-by-one.
  //
  // If there are split fields in `fields`, they will be placed at the end. The
  // order within split fields follows the same rule, aka classify and order by
  // "family".
  FieldVector DoOptimizeLayout(const FieldVector& fields,
                               const Options& options,
                               MessageSCCAnalyzer* scc_analyzer) const;

 private:
  enum FieldFamily {
    REPEATED = 0,  // Non-split repeated fields.
    STRING = 1,
    MESSAGE = 2,
    ZERO_INITIALIZABLE = 3,
    OTHER = 4,
    kMaxFamily
  };

  using FieldPartitionArray =
      std::array<std::array<std::vector<FieldGroup>, kMaxHotness>, kMaxFamily>;

  struct FieldAlignmentGroups {
    FieldPartitionArray aligned_to_1;
    FieldPartitionArray aligned_to_4;
    FieldPartitionArray aligned_to_8;
  };

  // Returns true if the message has PDProto data.
  virtual bool HasProfiledData() const = 0;

  virtual FieldHotness GetFieldHotness(
      const FieldDescriptor* field, const Options& options,
      MessageSCCAnalyzer* scc_analyzer) const = 0;

  virtual FieldGroup SingleFieldGroup(const FieldDescriptor* field) const = 0;

  static FieldFamily GetFieldFamily(const FieldDescriptor* field,
                                    const Options& options,
                                    MessageSCCAnalyzer* scc_analyzer);

  // Constructs the fast parse table for the message as it would be generated,
  // ignoring hasbits/inlined string indices as those have not been assigned
  // yet. This is used to determine which fields to prioritize for the fast
  // parse hotness class, which guarantees fast-parse eligibility.
  std::vector<internal::TailCallTableInfo::FastFieldInfo> BuildFastParseTable(
      const Options& options, MessageSCCAnalyzer* scc_analyzer) const;

  static bool IsFastPathField(
      const FieldDescriptor* field,
      const std::vector<internal::TailCallTableInfo::FastFieldInfo>&
          fast_path_fields);

  // Groups fields into alignment equivalence classes (1, 4, and 8). Within
  // each alignment equivalence class, fields are partitioned by `FieldFamily`
  // and `FieldHotness`.
  FieldAlignmentGroups BuildFieldAlignmentGroups(
      const FieldVector& fields, const Options& options,
      MessageSCCAnalyzer* scc_analyzer) const;

  // Consolidates all fields into a single array of field groups, partitioned by
  // `FieldFamily` and `FieldHotness`. Within each partition, fields are
  // consolidated into groups that are aligned to 8 bytes, with some groups
  // potentially containing multiple smaller fields.
  static FieldPartitionArray MergeFieldAlignmentGroups(
      FieldAlignmentGroups&& field_alignment_groups);

  // Builds the final field order from field groups partitioned by both hotness
  // class and family. The partitions are ordered first on `FieldHotness`, then
  // on `FieldFamily` within each hotness span. Repeated fields always appear
  // first (and are considered their own hotness class), followed by hot fields
  // of all other types, then warm/cold/split fields. Within each hotness span,
  // the order is based on `FieldFamily`, arranged in a way to maximize
  // contiguous spans of zero-initializable fields.
  FieldVector BuildFieldDescriptorOrder(FieldPartitionArray&& field_groups,
                                        const Options& options,
                                        MessageSCCAnalyzer* scc_analyzer) const;

  // Consolidate field groups that are aligned to `alignment` into groups that
  // are aligned to `target_alignment`.
  //
  // For example, if `alignment` is 1 and `target_alignment` is 4, then the
  // field groups will be consolidated into groups of 4.
  //
  // Requires `alignment` < `target_alignment`, and each must be a power of 2.
  static std::array<std::vector<FieldGroup>, kMaxHotness>
  ConsolidateAlignedFieldGroups(
      std::array<std::vector<FieldGroup>, kMaxHotness>& field_groups,
      size_t alignment, size_t target_alignment);

  // Moves field groups from `src_partition` into `dst_partition` to fill the
  // extra padding of `dst_partition`.
  //
  // This will not increase the size of `dst_partition` nor break runs of field
  // families. The fields moved to `dst_partition` will be removed from
  // `src_partition`.
  static void FillPaddingFromPartition(std::vector<FieldGroup>& dst_partition,
                                       std::vector<FieldGroup>& src_partition,
                                       size_t alignment);

  // Merges the hot field groups into the fast field groups if the total size of
  // the fast parse group would not exceed 32 fields. This means that every
  // field projected to be placed in the fast parse path will be assigned low
  // enough hasbits to guarantee eligibility.
  static void MaybeMergeHotIntoFast(FieldPartitionArray& field_groups);

  const Descriptor* descriptor_;
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_LAYOUT_HELPER_H__
