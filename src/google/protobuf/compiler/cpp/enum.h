// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_ENUM_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_ENUM_H__

#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/enum_strategy.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

class EnumGenerator {
 public:
  EnumGenerator() = default;
  EnumGenerator(const EnumDescriptor* descriptor, const Options& options);

  EnumGenerator(const EnumGenerator&) = delete;
  EnumGenerator& operator=(const EnumGenerator&) = delete;

  virtual ~EnumGenerator() = default;

  // Generate (only) the enum definition block: enum { ... }
  inline void GenerateEnumDefinitionBlock(io::Printer* p) const {
    enum_strategy_->GenerateEnumDefinitionBlock(p, GetEnumStrategyContext());
  }

  // Generate the enum helpers:
  //   - enum class name : int { ... }
  //   - extern const uint32_t name_internal_data_[];
  //   - constexpr name name_MIN = ...;
  //   - constexpr name name_MAX = ...;
  //   - bool name_IsValid(int value);
  //   - constexpr int name_ARRAYSIZE = ...;
  //   - const EnumDescriptor* name_descriptor();
  virtual void GenerateEnumHelpers(io::Printer* p) const;

  // Generate header code defining the enum.  This code should be placed
  // within the enum's package namespace, but NOT within any class, even for
  // nested enums.
  virtual void GenerateDefinition(io::Printer* p) const;

  // Generate specialization of GetEnumDescriptor<MyEnum>().
  // Precondition: in ::google::protobuf namespace.
  virtual void GenerateGetEnumDescriptorSpecializations(io::Printer* p);


  // For enums nested within a message, generate code to import all the enum's
  // symbols (e.g. the enum type name, all its values, etc.) into the class's
  // namespace.  This should be placed inside the class definition in the
  // header.
  void GenerateSymbolImports(io::Printer* p) const {
    enum_strategy_->GenerateSymbolImports(p, GetEnumStrategyContext());
  }

  // Generate the `inline` implementation of the _IsValid function.
  virtual void GenerateIsValid(io::Printer* p) const;

  // Source file stuff.

  // Generate non-inline methods related to the enum, such as IsValidValue().
  // Goes in the .cc file. EnumDescriptors are stored in an array, idx is
  // the index in this array that corresponds with this enum.
  virtual void GenerateMethods(int idx, io::Printer* p);

  // Returns true if this enum is defined inside a message.
  inline bool IsNested() const { return enum_->containing_type() != nullptr; }

 private:
  friend class FileGenerator;

  inline EnumStrategyContext GetEnumStrategyContext() const {
    return {
        .enum_ = enum_,
        .options = options_,
        .limits = limits_,
        .enum_vars = enum_vars_,
        .generate_array_size = generate_array_size_,
        .should_cache = should_cache_,
        .has_reflection = has_reflection_,
        .is_nested = IsNested(),
    };
  }

  const EnumDescriptor* enum_;
  Options options_;
  std::vector<int> sorted_unique_values_;

  bool generate_array_size_;
  bool should_cache_;
  bool has_reflection_;
  ValueLimits limits_;
  absl::flat_hash_map<absl::string_view, std::string> enum_vars_;
  std::unique_ptr<EnumStrategy> enum_strategy_;
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_ENUM_H__
