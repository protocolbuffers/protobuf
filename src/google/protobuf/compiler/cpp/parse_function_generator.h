// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_PARSE_FUNCTION_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_PARSE_FUNCTION_GENERATOR_H__

#include <map>
#include <string>
#include <vector>

#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/compiler/cpp/helpers.h>
#include <google/protobuf/compiler/cpp/options.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

// Helper class for generating tailcall parsing functions.
struct TailCallTableInfo {
  TailCallTableInfo(const Descriptor* descriptor, const Options& options,
                    const std::vector<const FieldDescriptor*>& ordered_fields,
                    const std::vector<int>& has_bit_indices,
                    const std::vector<int>& inlined_string_indices,
                    MessageSCCAnalyzer* scc_analyzer);

  // Fields parsed by the table fast-path.
  struct FastFieldInfo {
    std::string func_name;
    const FieldDescriptor* field;
    uint16_t coded_tag;
    uint8_t hasbit_idx;
    uint8_t aux_idx;
  };
  std::vector<FastFieldInfo> fast_path_fields;

  // Fields parsed by mini parsing routines.
  struct FieldEntryInfo {
    const FieldDescriptor* field;
    int hasbit_idx;
    int inlined_string_idx;
    uint16_t aux_idx;
    // True for enums entirely covered by the start/length fields of FieldAux:
    bool is_enum_range;
  };
  std::vector<FieldEntryInfo> field_entries;
  std::vector<std::string> aux_entries;

  // Fields parsed by generated fallback function.
  std::vector<const FieldDescriptor*> fallback_fields;

  // Table size.
  int table_size_log2;
  // Mask for has-bits of required fields.
  uint32_t has_hasbits_required_mask;
  // True if a generated fallback function is required instead of generic.
  bool use_generated_fallback;
};

// ParseFunctionGenerator generates the _InternalParse function for a message
// (and any associated supporting members).
class ParseFunctionGenerator {
 public:
  ParseFunctionGenerator(const Descriptor* descriptor, int max_has_bit_index,
                         const std::vector<int>& has_bit_indices,
                         const std::vector<int>& inlined_string_indices,
                         const Options& options,
                         MessageSCCAnalyzer* scc_analyzer,
                         const std::map<std::string, std::string>& vars);

  // Emits class-level method declarations to `printer`:
  void GenerateMethodDecls(io::Printer* printer);

  // Emits out-of-class method implementation definitions to `printer`:
  void GenerateMethodImpls(io::Printer* printer);

  // Emits class-level data member declarations to `printer`:
  void GenerateDataDecls(io::Printer* printer);

  // Emits out-of-class data member definitions to `printer`:
  void GenerateDataDefinitions(io::Printer* printer);

 private:
  // Returns true if tailcall table code should be generated.
  bool should_generate_tctable() const;

  // Returns true if tailcall table code should be generated, but inside an
  // #ifdef guard.
  bool should_generate_guarded_tctable() const {
    return should_generate_tctable() &&
           options_.tctable_mode == Options::kTCTableGuarded;
  }

  // Generates a tail-calling `_InternalParse` function.
  void GenerateTailcallParseFunction(Formatter& format);

  // Generates a fallback function for tailcall table-based parsing.
  void GenerateTailcallFallbackFunction(Formatter& format);

  // Generates a looping `_InternalParse` function.
  void GenerateLoopingParseFunction(Formatter& format);

  // Generates the tail-call table definition.
  void GenerateTailCallTable(Formatter& format);
  void GenerateFastFieldEntries(Formatter& format);
  void GenerateFieldEntries(Formatter& format);
  int CalculateFieldNamesSize() const;
  void GenerateFieldNames(Formatter& format);

  // Generates parsing code for an `ArenaString` field.
  void GenerateArenaString(Formatter& format, const FieldDescriptor* field);

  // Generates parsing code for a string-typed field.
  void GenerateStrings(Formatter& format, const FieldDescriptor* field,
                       bool check_utf8);

  // Generates parsing code for a length-delimited field (strings, messages,
  // etc.).
  void GenerateLengthDelim(Formatter& format, const FieldDescriptor* field);

  // Generates the parsing code for a known field.
  void GenerateFieldBody(Formatter& format,
                         google::protobuf::internal::WireFormatLite::WireType wiretype,
                         const FieldDescriptor* field);

  // Generates code to parse the next field from the input stream.
  void GenerateParseIterationBody(
      Formatter& format, const Descriptor* descriptor,
      const std::vector<const FieldDescriptor*>& fields);

  // Generates a `switch` statement to parse each of `fields`.
  void GenerateFieldSwitch(Formatter& format,
                           const std::vector<const FieldDescriptor*>& fields);

  const Descriptor* descriptor_;
  MessageSCCAnalyzer* scc_analyzer_;
  const Options& options_;
  std::map<std::string, std::string> variables_;
  std::unique_ptr<TailCallTableInfo> tc_table_info_;
  std::vector<int> inlined_string_indices_;
  const std::vector<const FieldDescriptor*> ordered_fields_;
  int num_hasbits_;
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_PARSE_FUNCTION_GENERATOR_H__
