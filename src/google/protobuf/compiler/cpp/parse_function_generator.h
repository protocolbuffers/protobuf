// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_PARSE_FUNCTION_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_PARSE_FUNCTION_GENERATOR_H__

#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_tctable_gen.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

// Returns the fields of the descriptor ordered by increasing tag number.
std::vector<const FieldDescriptor*> GetOrderedFields(
    const Descriptor* descriptor);

// ParseFunctionGenerator generates the _InternalParse function for a message
// (and any associated supporting members).
class ParseFunctionGenerator {
 public:
  // When presence probability is not present, we're not sure how likely "field"
  // is present. Assign a 50% probability to avoid pessimizing it.
  static constexpr float kUnknownPresenceProbability = 0.5f;

  ParseFunctionGenerator(
      const Descriptor* descriptor, int max_has_bit_index,
      absl::Span<const int> has_bit_indices,
      absl::Span<const int> inlined_string_indices, const Options& options,
      MessageSCCAnalyzer* scc_analyzer,
      const absl::flat_hash_map<absl::string_view, std::string>& vars,
      int index_in_file_messages);

  static std::vector<internal::TailCallTableInfo::FieldOptions>
  BuildFieldOptions(const Descriptor* descriptor,
                    absl::Span<const FieldDescriptor* const> ordered_fields,
                    const Options& options, MessageSCCAnalyzer* scc_analyzer,
                    absl::Span<const int> has_bit_indices,
                    absl::Span<const int> inlined_string_indices);

  static internal::TailCallTableInfo BuildTcTableInfoFromDescriptor(
      const Descriptor* descriptor, const Options& options,
      absl::Span<const internal::TailCallTableInfo::FieldOptions>
          field_options);

  // Emits class-level data member declarations to `printer`:
  void GenerateDataDecls(io::Printer* printer);

  // Emits out-of-class data member definitions to `printer`:
  void GenerateDataDefinitions(io::Printer* printer);

 private:
  friend class TailCallTableInfoTest;

  class GeneratedOptionProvider;

  // Generates the tail-call table definition.
  void GenerateTailCallTable(io::Printer* printer);
  void GenerateFastFieldEntries(Formatter& format);
  void GenerateFieldEntries(io::Printer* p);
  void GenerateFieldNames(Formatter& format);

  const Descriptor* descriptor_;
  MessageSCCAnalyzer* scc_analyzer_;
  const Options& options_;
  absl::flat_hash_map<absl::string_view, std::string> variables_;
  std::unique_ptr<internal::TailCallTableInfo> tc_table_info_;
  std::vector<int> inlined_string_indices_;
  const std::vector<const FieldDescriptor*> ordered_fields_;
  int num_hasbits_;
  int index_in_file_messages_;
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_PARSE_FUNCTION_GENERATOR_H__
