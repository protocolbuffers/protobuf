// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: seongkim@google.com (Seong Beom Kim)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_PADDING_OPTIMIZER_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_PADDING_OPTIMIZER_H__

#include "google/protobuf/compiler/cpp/message_layout_helper.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

// Rearranges the fields of a message to minimize padding.
// Fields are grouped by the type and the size.
// For example, grouping four boolean fields and one int32
// field results in zero padding overhead. See OptimizeLayout's
// comment for details.
class PaddingOptimizer final : public MessageLayoutHelper {
 public:
  explicit PaddingOptimizer(const Descriptor* descriptor)
      : MessageLayoutHelper(descriptor) {}
  ~PaddingOptimizer() override = default;

 private:
  bool HasProfiledData() const override { return false; }

  FieldHotness GetFieldHotness(
      const FieldDescriptor* field, const Options& options,
      MessageSCCAnalyzer* scc_analyzer) const override {
    // Assume all fields are hot.
    return kHot;
  }

  FieldGroup SingleFieldGroup(const FieldDescriptor* field) const override {
    return FieldGroup(field->number(), field);
  }
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_PADDING_OPTIMIZER_H__
