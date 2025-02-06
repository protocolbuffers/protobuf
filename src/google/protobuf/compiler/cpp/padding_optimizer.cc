// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/cpp/padding_optimizer.h"

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <vector>

#include "absl/log/absl_log.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/generated_message_tctable_gen.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/wire_format.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {

enum FieldPartition {
  kFastParse,
  kNormal,
  kSplit,
  kMax,
};

void partition_fields(std::vector<const FieldDescriptor*>* fields,
                      const Options& options, MessageSCCAnalyzer* scc_analyzer,
                      std::vector<const FieldDescriptor*> (
                          &field_partitions)[FieldPartition::kMax]) {
  std::bitset<internal::TcParseTableBase::kMaxFastFields> fast_field_idx_used;

  for (const auto* field : *fields) {
    if (ShouldSplit(field, options)) {
      field_partitions[kSplit].push_back(field);
      continue;
    } else if (PaddingOptimizer::IsFieldEligibleForFastParsing(field, options,
                                                               scc_analyzer)) {
      uint32_t fast_field_idx = internal::TcParseTableBase::TagToIdx(
          GetRecodedTagForFastParsing(field));
      if (!fast_field_idx_used.test(fast_field_idx)) {
        fast_field_idx_used.set(fast_field_idx);
        field_partitions[kFastParse].push_back(field);
        continue;
      }
    }

    field_partitions[kNormal].push_back(field);
  }
}

// options.lazy_opt might be on for fields that don't really support lazy, so we
// make sure we only use lazy rep for singular TYPE_MESSAGE fields.
// We can't trust the `lazy=true` annotation.
bool HasLazyRep(const FieldDescriptor* field,
                internal::field_layout::TransformValidation lazy_opt) {
  return field->type() == field->TYPE_MESSAGE && !field->is_repeated() &&
         lazy_opt != 0;
}

// FieldGroup is just a helper for PaddingOptimizer below. It holds a vector of
// fields that are grouped together because they have compatible alignment, and
// a preferred location in the final field ordering.
class FieldGroup {
 public:
  FieldGroup() : preferred_location_(0) {}

  // A group with a single field.
  FieldGroup(double preferred_location, const FieldDescriptor* field)
      : preferred_location_(preferred_location), fields_(1, field) {}

  // Append the fields in 'other' to this group.
  void Append(const FieldGroup& other) {
    if (other.fields_.empty()) {
      return;
    }
    // Preferred location is the average among all the fields, so we weight by
    // the number of fields on each FieldGroup object.
    preferred_location_ = (preferred_location_ * fields_.size() +
                           (other.preferred_location_ * other.fields_.size())) /
                          (fields_.size() + other.fields_.size());
    fields_.insert(fields_.end(), other.fields_.begin(), other.fields_.end());
  }

  void SetPreferredLocation(double location) { preferred_location_ = location; }
  const std::vector<const FieldDescriptor*>& fields() const { return fields_; }

  // FieldGroup objects sort by their preferred location.
  bool operator<(const FieldGroup& other) const {
    return preferred_location_ < other.preferred_location_;
  }

 private:
  // "preferred_location_" is an estimate of where this group should go in the
  // final list of fields.  We compute this by taking the average index of each
  // field in this group in the original ordering of fields.  This is very
  // approximate, but should put this group close to where its member fields
  // originally went.
  double preferred_location_;
  std::vector<const FieldDescriptor*> fields_;
  // We rely on the default copy constructor and operator= so this type can be
  // used in a vector.
};

void OptimizeLayoutHelper(std::vector<const FieldDescriptor*>* fields,
                          const Options& options,
                          MessageSCCAnalyzer* scc_analyzer) {
  if (fields->empty()) return;

  // The sorted numeric order of Family determines the declaration order in the
  // memory layout.
  enum Family {
    REPEATED = 0,
    STRING = 1,
    MESSAGE = 2,
    ZERO_INITIALIZABLE = 3,
    OTHER = 4,
    kMaxFamily
  };

  // First divide fields into those that align to 1 byte, 4 bytes or 8 bytes.
  std::vector<FieldGroup> aligned_to_1[kMaxFamily];
  std::vector<FieldGroup> aligned_to_4[kMaxFamily];
  std::vector<FieldGroup> aligned_to_8[kMaxFamily];
  for (int i = 0; i < fields->size(); ++i) {
    const FieldDescriptor* field = (*fields)[i];

    Family f = OTHER;
    if (field->is_repeated()) {
      f = REPEATED;
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
      f = STRING;
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      f = MESSAGE;
    } else if (CanInitializeByZeroing(field, options, scc_analyzer)) {
      f = ZERO_INITIALIZABLE;
    }

    const int j = field->number();
    switch (EstimateAlignmentSize(field)) {
      case 1:
        aligned_to_1[f].push_back(FieldGroup(j, field));
        break;
      case 4:
        aligned_to_4[f].push_back(FieldGroup(j, field));
        break;
      case 8:
        aligned_to_8[f].push_back(FieldGroup(j, field));
        break;
      default:
        ABSL_LOG(FATAL) << "Unknown alignment size "
                        << EstimateAlignmentSize(field) << "for a field "
                        << field->full_name() << ".";
    }
  }

  // For each family, group fields to optimize padding.
  for (int f = 0; f < kMaxFamily; f++) {
    // Now group fields aligned to 1 byte into sets of 4, and treat those like a
    // single field aligned to 4 bytes.
    for (int i = 0; i < aligned_to_1[f].size(); i += 4) {
      FieldGroup field_group;
      for (int j = i; j < aligned_to_1[f].size() && j < i + 4; ++j) {
        field_group.Append(aligned_to_1[f][j]);
      }
      aligned_to_4[f].push_back(field_group);
    }
    // Sort by preferred location to keep fields as close to their field number
    // order as possible.  Using stable_sort ensures that the output is
    // consistent across runs.
    std::stable_sort(aligned_to_4[f].begin(), aligned_to_4[f].end());

    // Now group fields aligned to 4 bytes (or the 4-field groups created above)
    // into pairs, and treat those like a single field aligned to 8 bytes.
    for (int i = 0; i < aligned_to_4[f].size(); i += 2) {
      FieldGroup field_group;
      for (int j = i; j < aligned_to_4[f].size() && j < i + 2; ++j) {
        field_group.Append(aligned_to_4[f][j]);
      }
      if (i == aligned_to_4[f].size() - 1) {
        if (f == OTHER) {
          // Move incomplete 4-byte block to the beginning.  This is done to
          // pair with the (possible) leftover blocks from the
          // ZERO_INITIALIZABLE family.
          field_group.SetPreferredLocation(-1);
        } else {
          // Move incomplete 4-byte block to the end.
          field_group.SetPreferredLocation(double{FieldDescriptor::kMaxNumber});
        }
      }
      aligned_to_8[f].push_back(field_group);
    }
    // Sort by preferred location.
    std::stable_sort(aligned_to_8[f].begin(), aligned_to_8[f].end());
  }

  // Now pull out all the FieldDescriptors in order.
  fields->clear();
  for (int f = 0; f < kMaxFamily; ++f) {
    for (int i = 0; i < aligned_to_8[f].size(); ++i) {
      fields->insert(fields->end(), aligned_to_8[f][i].fields().begin(),
                     aligned_to_8[f][i].fields().end());
    }
  }
}

}  // namespace

// Reorder 'fields' so that if the fields are output into a c++ class in the new
// order, fields of similar family (see below) are together and within each
// family, alignment padding is minimized.
//
// We try to do this while keeping each field as close as possible to its field
// number order so that we don't reduce cache locality much for function that
// access each field in order.  Originally, OptimizePadding used declaration
// order for its decisions, but generated code minus the serializer/parsers uses
// the output of OptimizePadding as well (stored in
// MessageGenerator::optimized_order_).  Since the serializers use field number
// order, we use that as a tie-breaker.
//
// We classify each field into a particular "family" of fields, that we perform
// the same operation on in our generated functions.
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
void PaddingOptimizer::OptimizeLayout(
    std::vector<const FieldDescriptor*>* fields, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) {
  std::vector<const FieldDescriptor*> field_partitions[FieldPartition::kMax];
  partition_fields(fields, options, scc_analyzer, field_partitions);
  fields->clear();
  for (int partition = 0; partition < FieldPartition::kMax; ++partition) {
    OptimizeLayoutHelper(&field_partitions[partition], options, scc_analyzer);
    fields->insert(fields->end(), field_partitions[partition].begin(),
                   field_partitions[partition].end());
  }
}

bool PaddingOptimizer::IsFieldEligibleForFastParsing(
    const FieldDescriptor* field, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) {
  // Map, oneof, weak, and split fields are not handled on the fast path.
  if (!IsFieldTypeEligibleForFastParsing(field) ||
      IsImplicitWeakField(field, options, scc_analyzer) ||
      ShouldSplit(field, options)) {
    return false;
  }

  auto lazy_opt = GetLazyStyle(field, options, scc_analyzer);
  if (HasLazyRep(field, lazy_opt) &&
      lazy_opt == internal::field_layout::kTvLazy) {
    // We only support eagerly verified lazy fields in the fast path.
    return false;
  }

  return true;
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
