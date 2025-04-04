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

#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

class MessageSCCAnalyzer;

class FieldGroup {
 public:
  FieldGroup()
      : fields_(),
        preferred_location_(0) {
  }

  // A group with a single field.
  FieldGroup(float preferred_location, const FieldDescriptor* field,
             uint64_t num_accesses = 0)
      : fields_(1, field),
        preferred_location_(preferred_location) {
  }

  const std::vector<const FieldDescriptor*>& fields() const { return fields_; }

  // Returns an estimate of the total memory size of the fields in this group,
  // ignoring padding/alignment.
  size_t EstimateMemorySize() const;

  void SetPreferredLocation(double location) { preferred_location_ = location; }

  // Appends the fields in 'other' to this group. Access count is incremented.
  void Append(const FieldGroup& other);

  // Sorts by their preferred location.
  bool operator<(const FieldGroup& other) const;

 private:
  bool UpdatePreferredLocationAndInsertOtherFields(const FieldGroup& other);

  std::vector<const FieldDescriptor*> fields_;
  float preferred_location_;
};

// Provides an abstract interface to optimize message layout
// by rearranging the fields of a message.
class MessageLayoutHelper {
 public:
  explicit MessageLayoutHelper(const Descriptor* descriptor)
      : descriptor_(descriptor) {}
  virtual ~MessageLayoutHelper() = default;

  const Descriptor* descriptor() const { return descriptor_; }

  virtual void OptimizeLayout(std::vector<const FieldDescriptor*>& fields,
                              const Options& options,
                              MessageSCCAnalyzer* scc_analyzer) const {
    return DoOptimizeLayout(fields, options, scc_analyzer);
  }

 protected:
  // TODO: Merge kCold and kSplit once all field types can be
  // split.
  enum FieldHotness {
    kRepeated,  // Non-split repeated fields.
    kHot,
    kWarm,
    kCold,
    kSplit,
    kMaxHotness,
  };

  // Reorder 'fields' so that if the fields are output into a c++ class in the
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
  void DoOptimizeLayout(std::vector<const FieldDescriptor*>& fields,
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

  // Returns true if the message has PDProto data.
  virtual bool HasProfiledData() const = 0;

  virtual FieldHotness GetFieldHotness(
      const FieldDescriptor* field, const Options& options,
      MessageSCCAnalyzer* scc_analyzer) const = 0;

  virtual FieldGroup SingleFieldGroup(const FieldDescriptor* field) const = 0;

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

  const Descriptor* descriptor_;
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_LAYOUT_HELPER_H__
