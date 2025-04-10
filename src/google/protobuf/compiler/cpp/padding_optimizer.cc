// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/cpp/padding_optimizer.h"

#include <vector>

#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/message_layout_helper.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

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
    std::vector<const FieldDescriptor*>& fields, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) {
  auto field_partitions = PartitionFields(fields, options, scc_analyzer);

  fields.clear();
  for (auto& partition : field_partitions) {
    OptimizeLayoutByFamily(
        partition, options,
        {REPEATED, STRING, MESSAGE, ZERO_INITIALIZABLE, OTHER}, scc_analyzer);
    fields.insert(fields.end(), partition.begin(), partition.end());
  }
}

FieldGroup PaddingOptimizer::SingleFieldGroup(
    const FieldDescriptor* field) const {
  return FieldGroup(field->number(), field);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
