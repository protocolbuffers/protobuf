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
#include <vector>

#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

class MessageSCCAnalyzer;

enum FieldPartition {
  kRepeated,  // Non-split repeated fields.
  kHot,
  kWarm,
  kCold,
  kSplit,
  kMax,
};

using FieldPartitionArray =
    std::array<std::vector<const FieldDescriptor*>, FieldPartition::kMax>;

// Provides an abstract interface to optimize message layout
// by rearranging the fields of a message.
class MessageLayoutHelper {
 public:
  virtual ~MessageLayoutHelper() {}

  virtual void OptimizeLayout(std::vector<const FieldDescriptor*>* fields,
                              const Options& options,
                              MessageSCCAnalyzer* scc_analyzer) = 0;

 protected:
  FieldPartitionArray PartitionFields(
      const std::vector<const FieldDescriptor*>& fields, const Options& options,
      MessageSCCAnalyzer* scc_analyzer) const;

  virtual FieldPartition FieldHotness(
      const FieldDescriptor* field, const Options& options,
      MessageSCCAnalyzer* scc_analyzer) const = 0;
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_LAYOUT_HELPER_H__
