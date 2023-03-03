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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/cpp/message.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "google/protobuf/stubs/common.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/map_entry_lite.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/cpp/enum.h"
#include "google/protobuf/compiler/cpp/extension.h"
#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/cpp/padding_optimizer.h"
#include "google/protobuf/compiler/cpp/parse_function_generator.h"
#include "google/protobuf/compiler/cpp/tracker.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/wire_format.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
using ::google::protobuf::internal::WireFormat;
using ::google::protobuf::internal::WireFormatLite;
using ::google::protobuf::internal::cpp::HasHasbit;
using ::google::protobuf::internal::cpp::Utf8CheckMode;
using Semantic = ::google::protobuf::io::AnnotationCollector::Semantic;
using Sub = ::google::protobuf::io::Printer::Sub;

static constexpr int kNoHasbit = -1;

// Create an expression that evaluates to
//  "for all i, (_has_bits_[i] & masks[i]) == masks[i]"
// masks is allowed to be shorter than _has_bits_, but at least one element of
// masks must be non-zero.
std::string ConditionalToCheckBitmasks(
    const std::vector<uint32_t>& masks, bool return_success = true,
    absl::string_view has_bits_var = "_impl_._has_bits_") {
  std::vector<std::string> parts;
  for (int i = 0; i < masks.size(); i++) {
    if (masks[i] == 0) continue;
    std::string m = absl::StrCat("0x", absl::Hex(masks[i], absl::kZeroPad8));
    // Each xor evaluates to 0 if the expected bits are present.
    parts.push_back(
        absl::StrCat("((", has_bits_var, "[", i, "] & ", m, ") ^ ", m, ")"));
  }
  ABSL_CHECK(!parts.empty());
  // If we have multiple parts, each expected to be 0, then bitwise-or them.
  std::string result =
      parts.size() == 1
          ? parts[0]
          : absl::StrCat("(", absl::StrJoin(parts, "\n       | "), ")");
  return result + (return_success ? " == 0" : " != 0");
}

void PrintPresenceCheck(const FieldDescriptor* field,
                        const std::vector<int>& has_bit_indices, io::Printer* p,
                        int* cached_has_word_index) {
  Formatter format(p);
  if (!field->options().weak()) {
    int has_bit_index = has_bit_indices[field->index()];
    if (*cached_has_word_index != (has_bit_index / 32)) {
      *cached_has_word_index = (has_bit_index / 32);
      format("cached_has_bits = $has_bits$[$1$];\n", *cached_has_word_index);
    }
    const std::string mask =
        absl::StrCat(absl::Hex(1u << (has_bit_index % 32), absl::kZeroPad8));
    format("if (cached_has_bits & 0x$1$u) {\n", mask);
  } else {
    format("if (has_$1$()) {\n", FieldName(field));
  }
  format.Indent();
}

struct FieldOrderingByNumber {
  inline bool operator()(const FieldDescriptor* a,
                         const FieldDescriptor* b) const {
    return a->number() < b->number();
  }
};

// Sort the fields of the given Descriptor by number into a new[]'d array
// and return it.
std::vector<const FieldDescriptor*> SortFieldsByNumber(
    const Descriptor* descriptor) {
  std::vector<const FieldDescriptor*> fields(descriptor->field_count());
  for (int i = 0; i < descriptor->field_count(); i++) {
    fields[i] = descriptor->field(i);
  }
  std::sort(fields.begin(), fields.end(), FieldOrderingByNumber());
  return fields;
}

// Functor for sorting extension ranges by their "start" field number.
struct ExtensionRangeSorter {
  bool operator()(const Descriptor::ExtensionRange* left,
                  const Descriptor::ExtensionRange* right) const {
    return left->start < right->start;
  }
};

bool IsPOD(const FieldDescriptor* field) {
  if (field->is_repeated() || field->is_extension()) return false;
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_ENUM:
    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT32:
    case FieldDescriptor::CPPTYPE_UINT64:
    case FieldDescriptor::CPPTYPE_FLOAT:
    case FieldDescriptor::CPPTYPE_DOUBLE:
    case FieldDescriptor::CPPTYPE_BOOL:
      return true;
    case FieldDescriptor::CPPTYPE_STRING:
      return false;
    default:
      return false;
  }
}

// Finds runs of fields for which `predicate` is true.
// RunMap maps from fields that start each run to the number of fields in that
// run.  This is optimized for the common case that there are very few runs in
// a message and that most of the eligible fields appear together.
using RunMap = absl::flat_hash_map<const FieldDescriptor*, size_t>;
RunMap FindRuns(const std::vector<const FieldDescriptor*>& fields,
                const std::function<bool(const FieldDescriptor*)>& predicate) {
  RunMap runs;
  const FieldDescriptor* last_start = nullptr;

  for (auto field : fields) {
    if (predicate(field)) {
      if (last_start == nullptr) {
        last_start = field;
      }

      runs[last_start]++;
    } else {
      last_start = nullptr;
    }
  }
  return runs;
}

// Emits an if-statement with a condition that evaluates to true if |field| is
// considered non-default (will be sent over the wire), for message types
// without true field presence. Should only be called if
// !HasHasbit(field).
bool EmitFieldNonDefaultCondition(io::Printer* p, const std::string& prefix,
                                  const FieldDescriptor* field) {
  ABSL_CHECK(!HasHasbit(field));
  Formatter format(p);
  auto v = p->WithVars({{
      {"prefix", prefix},
      {"name", FieldName(field)},
  }});
  // Merge and serialize semantics: primitive fields are merged/serialized only
  // if non-zero (numeric) or non-empty (string).
  if (!field->is_repeated() && !field->containing_oneof()) {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
      format("if (!$prefix$_internal_$name$().empty()) {\n");
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      // Message fields still have has_$name$() methods.
      format("if ($prefix$_internal_has_$name$()) {\n");
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_FLOAT) {
      format(
          "static_assert(sizeof(::uint32_t) == sizeof(float), \"Code assumes "
          "::uint32_t and float are the same size.\");\n"
          "float tmp_$name$ = $prefix$_internal_$name$();\n"
          "::uint32_t raw_$name$;\n"
          "memcpy(&raw_$name$, &tmp_$name$, sizeof(tmp_$name$));\n"
          "if (raw_$name$ != 0) {\n");
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_DOUBLE) {
      format(
          "static_assert(sizeof(::uint64_t) == sizeof(double), \"Code assumes "
          "::uint64_t and double are the same size.\");\n"
          "double tmp_$name$ = $prefix$_internal_$name$();\n"
          "::uint64_t raw_$name$;\n"
          "memcpy(&raw_$name$, &tmp_$name$, sizeof(tmp_$name$));\n"
          "if (raw_$name$ != 0) {\n");
    } else {
      format("if ($prefix$_internal_$name$() != 0) {\n");
    }
    format.Indent();
    return true;
  } else if (field->real_containing_oneof()) {
    format("if ($has_field$) {\n");
    format.Indent();
    return true;
  }
  return false;
}

bool HasInternalHasMethod(const FieldDescriptor* field) {
  return !HasHasbit(field) &&
         field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE;
}

// Collects map entry message type information.
void CollectMapInfo(
    const Options& options, const Descriptor* descriptor,
    absl::flat_hash_map<absl::string_view, std::string>* variables) {
  ABSL_CHECK(IsMapEntryMessage(descriptor));
  absl::flat_hash_map<absl::string_view, std::string>& vars = *variables;
  const FieldDescriptor* key = descriptor->map_key();
  const FieldDescriptor* val = descriptor->map_value();
  vars["key_cpp"] = PrimitiveTypeName(options, key->cpp_type());
  switch (val->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE:
      vars["val_cpp"] = FieldMessageTypeName(val, options);
      break;
    case FieldDescriptor::CPPTYPE_ENUM:
      vars["val_cpp"] = ClassName(val->enum_type(), true);
      break;
    default:
      vars["val_cpp"] = PrimitiveTypeName(options, val->cpp_type());
  }
  vars["key_wire_type"] = absl::StrCat(
      "TYPE_", absl::AsciiStrToUpper(DeclaredTypeMethodName(key->type())));
  vars["val_wire_type"] = absl::StrCat(
      "TYPE_", absl::AsciiStrToUpper(DeclaredTypeMethodName(val->type())));
}


// Returns true to make the message serialize in order, decided by the following
// factors in the order of precedence.
// --options().message_set_wire_format() == true
// --the message is in the allowlist (true)
// --GOOGLE_PROTOBUF_SHUFFLE_SERIALIZE is defined (false)
// --a ranage of message names that are allowed to stay in order (true)
bool ShouldSerializeInOrder(const Descriptor* descriptor,
                            const Options& options) {
  return true;
}

bool IsCrossFileMapField(const FieldDescriptor* field) {
  if (!field->is_map()) {
    return false;
  }

  const Descriptor* d = field->message_type();
  const FieldDescriptor* value = d->FindFieldByNumber(2);

  return IsCrossFileMessage(value);
}

bool IsCrossFileMaybeMap(const FieldDescriptor* field) {
  if (IsCrossFileMapField(field)) {
    return true;
  }

  return IsCrossFileMessage(field);
}

bool IsRequired(const std::vector<const FieldDescriptor*>& v) {
  return v.front()->is_required();
}

bool HasNonSplitOptionalString(const Descriptor* desc, const Options& options) {
  for (const auto* field : FieldRange(desc)) {
    if (IsString(field, options) && !field->is_repeated() &&
        !field->real_containing_oneof() && !ShouldSplit(field, options)) {
      return true;
    }
  }
  return false;
}

// Collects neighboring fields based on a given criteria (equivalent predicate).
template <typename Predicate>
std::vector<std::vector<const FieldDescriptor*>> CollectFields(
    const std::vector<const FieldDescriptor*>& fields,
    const Predicate& equivalent) {
  std::vector<std::vector<const FieldDescriptor*>> chunks;
  for (auto field : fields) {
    if (chunks.empty() || !equivalent(chunks.back().back(), field)) {
      chunks.emplace_back();
    }
    chunks.back().push_back(field);
  }
  return chunks;
}

// Returns a bit mask based on has_bit index of "fields" that are typically on
// the same chunk. It is used in a group presence check where _has_bits_ is
// masked to tell if any thing in "fields" is present.
uint32_t GenChunkMask(const std::vector<const FieldDescriptor*>& fields,
                      const std::vector<int>& has_bit_indices) {
  ABSL_CHECK(!fields.empty());
  int first_index_offset = has_bit_indices[fields.front()->index()] / 32;
  uint32_t chunk_mask = 0;
  for (auto field : fields) {
    // "index" defines where in the _has_bits_ the field appears.
    int index = has_bit_indices[field->index()];
    ABSL_CHECK_EQ(first_index_offset, index / 32);
    chunk_mask |= static_cast<uint32_t>(1) << (index % 32);
  }
  ABSL_CHECK_NE(0, chunk_mask);
  return chunk_mask;
}

// Return the number of bits set in n, a non-negative integer.
static int popcnt(uint32_t n) {
  int result = 0;
  while (n != 0) {
    result += (n & 1);
    n = n / 2;
  }
  return result;
}

// For a run of cold chunks, opens and closes an external if statement that
// checks multiple has_bits words to skip bulk of cold fields.
class ColdChunkSkipper {
 public:
  ColdChunkSkipper(
      const Descriptor* descriptor, const Options& options,
      const std::vector<std::vector<const FieldDescriptor*>>& chunks,
      const std::vector<int>& has_bit_indices, const double cold_threshold)
      : chunks_(chunks),
        has_bit_indices_(has_bit_indices),
        access_info_map_(options.access_info_map),
        cold_threshold_(cold_threshold) {
    SetCommonMessageDataVariables(descriptor, &variables_);
  }

  // May open an external if check for a batch of cold fields. "from" is the
  // prefix to _has_bits_ to allow MergeFrom to use "from._has_bits_".
  // Otherwise, it should be "".
  void OnStartChunk(int chunk, int cached_has_word_index,
                    const std::string& from, io::Printer* p);
  bool OnEndChunk(int chunk, io::Printer* p);

 private:
  bool IsColdChunk(int chunk);

  int HasbitWord(int chunk, int offset) {
    return has_bit_indices_[chunks_[chunk][offset]->index()] / 32;
  }

  const std::vector<std::vector<const FieldDescriptor*>>& chunks_;
  const std::vector<int>& has_bit_indices_;
  const AccessInfoMap* access_info_map_;
  const double cold_threshold_;
  absl::flat_hash_map<absl::string_view, std::string> variables_;
  int limit_chunk_ = -1;
};

// Tuning parameters for ColdChunkSkipper.
const double kColdRatio = 0.005;

bool ColdChunkSkipper::IsColdChunk(int chunk) {
  // Mark this variable as used until it is actually used
  (void)cold_threshold_;
  return false;
}


void ColdChunkSkipper::OnStartChunk(int chunk, int cached_has_word_index,
                                    const std::string& from, io::Printer* p) {
  Formatter format(p);
  if (!access_info_map_) {
    return;
  } else if (chunk < limit_chunk_) {
    // We are already inside a run of cold chunks.
    return;
  } else if (!IsColdChunk(chunk)) {
    // We can't start a run of cold chunks.
    return;
  }

  // Find the end of consecutive cold chunks.
  limit_chunk_ = chunk;
  while (limit_chunk_ < chunks_.size() && IsColdChunk(limit_chunk_)) {
    limit_chunk_++;
  }

  if (limit_chunk_ <= chunk + 1) {
    // Require at least two chunks to emit external has_bit checks.
    limit_chunk_ = -1;
    return;
  }

  // Emit has_bit check for each has_bit_dword index.
  format("if (PROTOBUF_PREDICT_FALSE(");
  int first_word = HasbitWord(chunk, 0);
  while (chunk < limit_chunk_) {
    uint32_t mask = 0;
    int this_word = HasbitWord(chunk, 0);
    // Generate mask for chunks on the same word.
    for (; chunk < limit_chunk_ && HasbitWord(chunk, 0) == this_word; chunk++) {
      for (auto field : chunks_[chunk]) {
        int hasbit_index = has_bit_indices_[field->index()];
        // Fields on a chunk must be in the same word.
        ABSL_CHECK_EQ(this_word, hasbit_index / 32);
        mask |= 1 << (hasbit_index % 32);
      }
    }

    if (this_word != first_word) {
      format(" ||\n    ");
    }
    auto v = p->WithVars({{"mask", absl::Hex(mask, absl::kZeroPad8)}});
    if (this_word == cached_has_word_index) {
      format("(cached_has_bits & 0x$mask$u) != 0");
    } else {
      format("($1$_impl_._has_bits_[$2$] & 0x$mask$u) != 0", from, this_word);
    }
  }
  format(")) {\n");
  format.Indent();
}

bool ColdChunkSkipper::OnEndChunk(int chunk, io::Printer* p) {
  Formatter format(p);
  if (chunk != limit_chunk_ - 1) {
    return false;
  }
  format.Outdent();
  format("}\n");
  return true;
}

absl::flat_hash_map<absl::string_view, std::string> ClassVars(
    const Descriptor* desc, Options opts) {
  absl::flat_hash_map<absl::string_view, std::string> vars = MessageVars(desc);

  vars.emplace("pkg", Namespace(desc, opts));
  vars.emplace("Msg", ClassName(desc, false));
  vars.emplace("pkg::Msg", QualifiedClassName(desc, opts));
  vars.emplace("pkg.Msg", desc->full_name());

  // Old-style names, to be removed once all usages are gone in this and other
  // files.
  vars.emplace("classname", ClassName(desc, false));
  vars.emplace("classtype", QualifiedClassName(desc, opts));
  vars.emplace("full_name", desc->full_name());
  vars.emplace("superclass", SuperClassName(desc, opts));

  for (auto& pair : UnknownFieldsVars(desc, opts)) {
    vars.emplace(pair);
  }

  return vars;
}

absl::flat_hash_map<absl::string_view, std::string> HasbitVars(
    int has_bit_index) {
  return {
      {"has_array_index", absl::StrCat(has_bit_index / 32)},
      {"has_mask",
       absl::StrCat(
           "0x", absl::Hex(1u << (has_bit_index % 32), absl::kZeroPad8), "u")},
  };
}

}  // anonymous namespace

// ===================================================================

MessageGenerator::MessageGenerator(
    const Descriptor* descriptor,
    const absl::flat_hash_map<absl::string_view, std::string>&,
    int index_in_file_messages, const Options& options,
    MessageSCCAnalyzer* scc_analyzer)
    : descriptor_(descriptor),
      index_in_file_messages_(index_in_file_messages),
      options_(options),
      field_generators_(descriptor),
      scc_analyzer_(scc_analyzer) {

  if (!message_layout_helper_) {
    message_layout_helper_ = std::make_unique<PaddingOptimizer>();
  }

  // Compute optimized field order to be used for layout and initialization
  // purposes.
  for (auto field : FieldRange(descriptor_)) {
    if (IsWeak(field, options_)) {
      ++num_weak_fields_;
      continue;
    }

    if (!field->real_containing_oneof()) {
      optimized_order_.push_back(field);
    }
  }

  message_layout_helper_->OptimizeLayout(&optimized_order_, options_,
                                         scc_analyzer_);

  // This message has hasbits iff one or more fields need one.
  for (auto field : optimized_order_) {
    if (HasHasbit(field)) {
      if (has_bit_indices_.empty()) {
        has_bit_indices_.resize(descriptor_->field_count(), kNoHasbit);
      }
      has_bit_indices_[field->index()] = max_has_bit_index_++;
    }
    if (IsStringInlined(field, options_)) {
      if (inlined_string_indices_.empty()) {
        inlined_string_indices_.resize(descriptor_->field_count(), kNoHasbit);
        // The bitset[0] is for arena dtor tracking. Donating states start from
        // bitset[1];
        ++max_inlined_string_index_;
      }

      inlined_string_indices_[field->index()] = max_inlined_string_index_++;
    }
  }
  field_generators_.Build(options_, scc_analyzer_, has_bit_indices_,
                          inlined_string_indices_);

  for (int i = 0; i < descriptor->field_count(); i++) {
    if (descriptor->field(i)->is_required()) {
      ++num_required_fields_;
    }
  }

  parse_function_generator_ = std::make_unique<ParseFunctionGenerator>(
      descriptor_, max_has_bit_index_, has_bit_indices_,
      inlined_string_indices_, options_, scc_analyzer_, variables_);
}

size_t MessageGenerator::HasBitsSize() const {
  return (max_has_bit_index_ + 31) / 32;
}

size_t MessageGenerator::InlinedStringDonatedSize() const {
  return (max_inlined_string_index_ + 31) / 32;
}

int MessageGenerator::HasBitIndex(const FieldDescriptor* field) const {
  return has_bit_indices_.empty() ? kNoHasbit
                                  : has_bit_indices_[field->index()];
}

int MessageGenerator::HasByteIndex(const FieldDescriptor* field) const {
  int hasbit = HasBitIndex(field);
  return hasbit == kNoHasbit ? kNoHasbit : hasbit / 8;
}

int MessageGenerator::HasWordIndex(const FieldDescriptor* field) const {
  int hasbit = HasBitIndex(field);
  return hasbit == kNoHasbit ? kNoHasbit : hasbit / 32;
}

void MessageGenerator::AddGenerators(
    std::vector<std::unique_ptr<EnumGenerator>>* enum_generators,
    std::vector<std::unique_ptr<ExtensionGenerator>>* extension_generators) {
  for (int i = 0; i < descriptor_->enum_type_count(); i++) {
    enum_generators->emplace_back(
        std::make_unique<EnumGenerator>(descriptor_->enum_type(i), options_));
    enum_generators_.push_back(enum_generators->back().get());
  }
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    extension_generators->emplace_back(std::make_unique<ExtensionGenerator>(
        descriptor_->extension(i), options_, scc_analyzer_));
    extension_generators_.push_back(extension_generators->back().get());
  }
}

void MessageGenerator::GenerateFieldAccessorDeclarations(io::Printer* p) {
  auto v = p->WithVars(MessageVars(descriptor_));
  Formatter format(p);

  // optimized_fields_ does not contain fields where
  //    field->real_containing_oneof()
  // so we need to iterate over those as well.
  //
  // We place the non-oneof fields in optimized_order_, as that controls the
  // order of the _has_bits_ entries and we want GDB's pretty ps to be
  // able to infer these indices from the k[FIELDNAME]FieldNumber order.
  std::vector<const FieldDescriptor*> ordered_fields;
  ordered_fields.reserve(descriptor_->field_count());
  ordered_fields.insert(ordered_fields.begin(), optimized_order_.begin(),
                        optimized_order_.end());

  for (auto field : FieldRange(descriptor_)) {
    if (!field->real_containing_oneof() && !field->options().weak()) {
      continue;
    }
    ordered_fields.push_back(field);
  }

  if (!ordered_fields.empty()) {
    p->Emit({{
                "kFields",
                [&] {
                  for (auto field : ordered_fields) {
                    auto v = p->WithVars(FieldVars(field, options_));
                    p->Emit({Sub("kField", FieldConstantName(field))
                                 .AnnotatedAs(field)},
                            R"cc(
                              $kField$ = $number$,
                            )cc");
                  }
                },
            }},
            R"cc(
              enum : int {
                $kFields$,
              };
            )cc");
  }
  for (auto field : ordered_fields) {
    auto name = FieldName(field);

    auto v = p->WithVars(FieldVars(field, options_));
    auto t = p->WithVars(MakeTrackerCalls(field, options_));
    p->Emit(
        {{"field_comment", FieldComment(field)},
         Sub("const_impl", "const;").WithSuffix(";"),
         Sub("impl", ";").WithSuffix(";"),
         {"sizer",
          [&] {
            if (!field->is_repeated()) return;
            p->Emit({Sub("name_size", absl::StrCat(name, "_size"))
                         .AnnotatedAs(field)},
                    R"cc(
                      $deprecated_attr $int $name_size$() $const_impl$;
                    )cc");

            p->Emit({Sub("_internal_name_size",
                         absl::StrCat("_internal_", name, "_size"))
                         .AnnotatedAs(field)},
                    R"cc(
                      private:
                      int $_internal_name_size$() const;

                      public:
                    )cc");
          }},
         {"hazzer",
          [&] {
            if (!field->has_presence()) return;
            p->Emit({Sub("has_name", absl::StrCat("has_", name))
                         .AnnotatedAs(field)},
                    R"cc(
                      $deprecated_attr $bool $has_name$() $const_impl$;
                    )cc");
          }},
         {"internal_hazzer",
          [&] {
            if (field->is_repeated() || !HasInternalHasMethod(field)) {
              return;
            }
            p->Emit(
                {Sub("_internal_has_name", absl::StrCat("_internal_has_", name))
                     .AnnotatedAs(field)},
                R"cc(
                  private:
                  bool $_internal_has_name$() const;

                  public:
                )cc");
          }},
         {"clearer",
          [&] {
            p->Emit({Sub("clear_name", absl::StrCat("clear_", name))
                         .AnnotatedAs({
                             field,
                             Semantic::kSet,
                         })},
                    R"cc(
                      $deprecated_attr $void $clear_name$() $impl$;
                    )cc");
          }},
         {"accessors",
          [&] {
            field_generators_.get(field).GenerateAccessorDeclarations(p);
          }}},
        R"cc(
          // $field_comment$
          $sizer$;
          $hazzer$;
          $internal_hazzer$;
          $clearer$;
          $accessors$;
        )cc");
  }

  if (descriptor_->extension_range_count() > 0) {
    // Generate accessors for extensions.
    // We use "_proto_TypeTraits" as a type name below because "TypeTraits"
    // causes problems if the class has a nested message or enum type with that
    // name and "_TypeTraits" is technically reserved for the C++ library since
    // it starts with an underscore followed by a capital letter.
    //
    // For similar reason, we use "_field_type" and "_is_packed" as parameter
    // names below, so that "field_type" and "is_packed" can be used as field
    // names.
    p->Emit(R"cc(
      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline bool HasExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id) const {
        $annotate_extension_has$;
        return $extensions$.Has(id.number());
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline void ClearExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id) {
        $extensions$.ClearExtension(id.number());
        $annotate_extension_clear$;
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline int ExtensionSize(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id) const {
        $annotate_extension_repeated_size$;
        return $extensions$.ExtensionSize(id.number());
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline typename _proto_TypeTraits::Singular::ConstType GetExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id) const {
        $annotate_extension_get$;
        return _proto_TypeTraits::Get(id.number(), $extensions$, id.default_value());
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline typename _proto_TypeTraits::Singular::MutableType MutableExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id) {
        $annotate_extension_mutable$;
        return _proto_TypeTraits::Mutable(id.number(), _field_type, &$extensions$);
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline void SetExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id,
          typename _proto_TypeTraits::Singular::ConstType value) {
        _proto_TypeTraits::Set(id.number(), _field_type, value, &$extensions$);
        $annotate_extension_set$;
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline void SetAllocatedExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id,
          typename _proto_TypeTraits::Singular::MutableType value) {
        _proto_TypeTraits::SetAllocated(id.number(), _field_type, value,
                                        &$extensions$);
        $annotate_extension_set$;
      }
      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline void UnsafeArenaSetAllocatedExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id,
          typename _proto_TypeTraits::Singular::MutableType value) {
        _proto_TypeTraits::UnsafeArenaSetAllocated(id.number(), _field_type,
                                                   value, &$extensions$);
        $annotate_extension_set$;
      }
      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      PROTOBUF_NODISCARD inline
          typename _proto_TypeTraits::Singular::MutableType
          ReleaseExtension(
              const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                               _field_type, _is_packed>& id) {
        $annotate_extension_release$;
        return _proto_TypeTraits::Release(id.number(), _field_type, &$extensions$);
      }
      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline typename _proto_TypeTraits::Singular::MutableType
      UnsafeArenaReleaseExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id) {
        $annotate_extension_release$;
        return _proto_TypeTraits::UnsafeArenaRelease(id.number(), _field_type,
                                                     &$extensions$);
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline typename _proto_TypeTraits::Repeated::ConstType GetExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id,
          int index) const {
        $annotate_repeated_extension_get$;
        return _proto_TypeTraits::Get(id.number(), $extensions$, index);
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline typename _proto_TypeTraits::Repeated::MutableType MutableExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id,
          int index) {
        $annotate_repeated_extension_mutable$;
        return _proto_TypeTraits::Mutable(id.number(), index, &$extensions$);
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline void SetExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id,
          int index, typename _proto_TypeTraits::Repeated::ConstType value) {
        _proto_TypeTraits::Set(id.number(), index, value, &$extensions$);
        $annotate_repeated_extension_set$;
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline typename _proto_TypeTraits::Repeated::MutableType AddExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id) {
        typename _proto_TypeTraits::Repeated::MutableType to_add =
            _proto_TypeTraits::Add(id.number(), _field_type, &$extensions$);
        $annotate_repeated_extension_add_mutable$;
        return to_add;
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline void AddExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id,
          typename _proto_TypeTraits::Repeated::ConstType value) {
        _proto_TypeTraits::Add(id.number(), _field_type, _is_packed, value,
                               &$extensions$);
        $annotate_repeated_extension_add$;
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline const typename _proto_TypeTraits::Repeated::RepeatedFieldType&
      GetRepeatedExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id) const {
        $annotate_repeated_extension_list$;
        return _proto_TypeTraits::GetRepeated(id.number(), $extensions$);
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline typename _proto_TypeTraits::Repeated::RepeatedFieldType*
      MutableRepeatedExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id) {
        $annotate_repeated_extension_list_mutable$;
        return _proto_TypeTraits::MutableRepeated(id.number(), _field_type,
                                                  _is_packed, &$extensions$);
      }
    )cc");

    // Generate MessageSet specific APIs for proto2 MessageSet.
    // For testing purposes we don't check for bridge.MessageSet, so
    // we don't use IsProto2MessageSet
    if (descriptor_->options().message_set_wire_format() &&
        !options_.opensource_runtime && !options_.lite_implicit_weak_fields) {
      // Special-case MessageSet.
      p->Emit(R"cc(
        GOOGLE_PROTOBUF_EXTENSION_MESSAGE_SET_ACCESSORS($Msg$);
      )cc");
    }
  }

  for (auto oneof : OneOfRange(descriptor_)) {
    Formatter::SaveState saver(&format);
    auto v = p->WithVars({
        {"oneof_name", oneof->name()},
        {"camel_oneof_name", UnderscoresToCamelCase(oneof->name(), true)},
    });
    format(
        "void ${1$clear_$oneof_name$$}$();\n"
        "$camel_oneof_name$Case $oneof_name$_case() const;\n",
        oneof);
  }
}

void MessageGenerator::GenerateSingularFieldHasBits(
    const FieldDescriptor* field, io::Printer* p) {
  auto t = p->WithVars(MakeTrackerCalls(field, options_));
  Formatter format(p);
  if (field->options().weak()) {
    format(
        "inline bool $classname$::has_$name$() const {\n"
        "$annotate_has$"
        "  return $weak_field_map$.Has($number$);\n"
        "}\n");
    return;
  }
  if (HasHasbit(field)) {
    int has_bit_index = HasBitIndex(field);
    ABSL_CHECK_NE(has_bit_index, kNoHasbit);

    auto v = p->WithVars(HasbitVars(has_bit_index));
    format(
        "inline bool $classname$::has_$name$() const {\n"
        "$annotate_has$"
        "  bool value = ($has_bits$[$has_array_index$] & $has_mask$) != 0;\n");

    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
        !IsLazy(field, options_, scc_analyzer_)) {
      // We maintain the invariant that for a submessage x, has_x() returning
      // true implies that x_ is not null. By giving this information to the
      // compiler, we allow it to eliminate unnecessary null checks later on.
      format("  PROTOBUF_ASSUME(!value || $field$ != nullptr);\n");
    }

    format(
        "  return value;\n"
        "}\n");
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    // Message fields have a has_$name$() method.
    if (IsLazy(field, options_, scc_analyzer_)) {
      format(
          "inline bool $classname$::_internal_has_$name$() const {\n"
          "  return !$field$.IsCleared();\n"
          "}\n");
    } else {
      format(
          "inline bool $classname$::_internal_has_$name$() const {\n"
          "  return this != internal_default_instance() "
          "&& $field$ != nullptr;\n"
          "}\n");
    }
    format(
        "inline bool $classname$::has_$name$() const {\n"
        "$annotate_has$"
        "  return _internal_has_$name$();\n"
        "}\n");
  }
}

void MessageGenerator::GenerateOneofHasBits(io::Printer* p) {
  Formatter format(p);
  for (const auto* oneof : OneOfRange(descriptor_)) {
    auto v = p->WithVars({
        {"oneof_index", oneof->index()},
        {"oneof_name", oneof->name()},
        {"cap_oneof_name", absl::AsciiStrToUpper(oneof->name())},
    });
    format(
        "inline bool $classname$::has_$oneof_name$() const {\n"
        "  return $oneof_name$_case() != $cap_oneof_name$_NOT_SET;\n"
        "}\n"
        "inline void $classname$::clear_has_$oneof_name$() {\n"
        "  $oneof_case$[$oneof_index$] = $cap_oneof_name$_NOT_SET;\n"
        "}\n");
  }
}

void MessageGenerator::GenerateOneofMemberHasBits(const FieldDescriptor* field,
                                                  io::Printer* p) {
  auto t = p->WithVars(MakeTrackerCalls(field, options_));
  Formatter format(p);
  // Singular field in a oneof
  // N.B.: Without field presence, we do not use has-bits or generate
  // has_$name$() methods, but oneofs still have set_has_$name$().
  // Oneofs also have private _internal_has_$name$() a helper method.
  if (field->has_presence()) {
    format(
        "inline bool $classname$::has_$name$() const {\n"
        "$annotate_has$"
        "  return $has_field$;\n"
        "}\n");
  }
  if (HasInternalHasMethod(field)) {
    format(
        "inline bool $classname$::_internal_has_$name$() const {\n"
        "  return $has_field$;\n"
        "}\n");
  }
  // set_has_$name$() for oneof fields is always private; hence should not be
  // annotated.
  format(
      "inline void $classname$::set_has_$name$() {\n"
      "  $oneof_case$[$oneof_index$] = k$field_name$;\n"
      "}\n");
}

void MessageGenerator::GenerateFieldClear(const FieldDescriptor* field,
                                          bool is_inline, io::Printer* p) {
  auto t = p->WithVars(MakeTrackerCalls(field, options_));
  Formatter format(p);

  // Generate clear_$name$().
  if (is_inline) {
    format("inline ");
  }
  format("void $classname$::clear_$name$() {\n");

  format.Indent();

  if (field->real_containing_oneof()) {
    // Clear this field only if it is the active field in this oneof,
    // otherwise ignore
    auto t = p->WithVars(MakeTrackerCalls(field, options_));
    format("if ($has_field$) {\n");
    format.Indent();
    field_generators_.get(field).GenerateClearingCode(p);
    format("clear_has_$oneof_name$();\n");
    format.Outdent();
    format("}\n");
  } else {
    if (ShouldSplit(field, options_)) {
      format("if (IsSplitMessageDefault()) return;\n");
    }
    field_generators_.get(field).GenerateClearingCode(p);
    if (HasHasbit(field)) {
      int has_bit_index = HasBitIndex(field);
      auto v = p->WithVars(HasbitVars(has_bit_index));
      format("$has_bits$[$has_array_index$] &= ~$has_mask$;\n");
    }
  }
  format("$annotate_clear$");
  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateFieldAccessorDefinitions(io::Printer* p) {
  Formatter format(p);
  format("// $classname$\n\n");

  for (auto field : FieldRange(descriptor_)) {
    PrintFieldComment(format, field);

    auto v = p->WithVars(FieldVars(field, options_));
    auto t = p->WithVars(MakeTrackerCalls(field, options_));
    // Generate has_$name$() or $name$_size().
    if (field->is_repeated()) {
      format(
          "inline int $classname$::_internal_$name$_size() const {\n"
          "  return $field$$1$.size();\n"
          "}\n"
          "inline int $classname$::$name$_size() const {\n"
          "$annotate_size$"
          "  return _internal_$name$_size();\n"
          "}\n",
          IsImplicitWeakField(field, options_, scc_analyzer_) &&
                  field->message_type()
              ? ".weak"
              : "");
    } else if (field->real_containing_oneof()) {
      GenerateOneofMemberHasBits(field, p);
    } else {
      // Singular field.
      GenerateSingularFieldHasBits(field, p);
    }

    if (!IsCrossFileMaybeMap(field)) {
      GenerateFieldClear(field, true, p);
    }
    // Generate type-specific accessors.
    field_generators_.get(field).GenerateInlineAccessorDefinitions(p);

    format("\n");
  }

  // Generate has_$name$() and clear_has_$name$() functions for oneofs.
  GenerateOneofHasBits(p);
}

void MessageGenerator::GenerateClassDefinition(io::Printer* p) {
  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  Formatter format(p);

  if (IsMapEntryMessage(descriptor_)) {
    absl::flat_hash_map<absl::string_view, std::string> vars;
    CollectMapInfo(options_, descriptor_, &vars);
    vars["lite"] =
        HasDescriptorMethods(descriptor_->file(), options_) ? "" : "Lite";
    auto v = p->WithVars(std::move(vars));
    format(
        "class $classname$ final : public "
        "::$proto_ns$::internal::MapEntry$lite$<$classname$, \n"
        "    $key_cpp$, $val_cpp$,\n"
        "    ::$proto_ns$::internal::WireFormatLite::$key_wire_type$,\n"
        "    ::$proto_ns$::internal::WireFormatLite::$val_wire_type$> {\n"
        "public:\n"
        "  typedef ::$proto_ns$::internal::MapEntry$lite$<$classname$, \n"
        "    $key_cpp$, $val_cpp$,\n"
        "    ::$proto_ns$::internal::WireFormatLite::$key_wire_type$,\n"
        "    ::$proto_ns$::internal::WireFormatLite::$val_wire_type$> "
        "SuperType;\n"
        "  $classname$();\n"
        "  explicit PROTOBUF_CONSTEXPR $classname$(\n"
        "      ::$proto_ns$::internal::ConstantInitialized);\n"
        "  explicit $classname$(::$proto_ns$::Arena* arena);\n"
        "  void MergeFrom(const $classname$& other);\n"
        "  static const $classname$* internal_default_instance() { return "
        "reinterpret_cast<const "
        "$classname$*>(&_$classname$_default_instance_); }\n");
    auto utf8_check = internal::cpp::GetUtf8CheckMode(
        descriptor_->field(0), GetOptimizeFor(descriptor_->file(), options_) ==
                                   FileOptions::LITE_RUNTIME);
    if (descriptor_->field(0)->type() == FieldDescriptor::TYPE_STRING &&
        utf8_check != Utf8CheckMode::kNone) {
      if (utf8_check == Utf8CheckMode::kStrict) {
        format(
            "  static bool ValidateKey(std::string* s) {\n"
            "    return ::$proto_ns$::internal::WireFormatLite::"
            "VerifyUtf8String(s->data(), static_cast<int>(s->size()), "
            "::$proto_ns$::internal::WireFormatLite::PARSE, \"$1$\");\n"
            " }\n",
            descriptor_->field(0)->full_name());
      } else {
        ABSL_CHECK(utf8_check == Utf8CheckMode::kVerify);
        format(
            "  static bool ValidateKey(std::string* s) {\n"
            "#ifndef NDEBUG\n"
            "    ::$proto_ns$::internal::WireFormatLite::VerifyUtf8String(\n"
            "       s->data(), static_cast<int>(s->size()), "
            "::$proto_ns$::internal::"
            "WireFormatLite::PARSE, \"$1$\");\n"
            "#else\n"
            "    (void) s;\n"
            "#endif\n"
            "    return true;\n"
            " }\n",
            descriptor_->field(0)->full_name());
      }
    } else {
      format("  static bool ValidateKey(void*) { return true; }\n");
    }
    if (descriptor_->field(1)->type() == FieldDescriptor::TYPE_STRING &&
        utf8_check != Utf8CheckMode::kNone) {
      if (utf8_check == Utf8CheckMode::kStrict) {
        format(
            "  static bool ValidateValue(std::string* s) {\n"
            "    return ::$proto_ns$::internal::WireFormatLite::"
            "VerifyUtf8String(s->data(), static_cast<int>(s->size()), "
            "::$proto_ns$::internal::WireFormatLite::PARSE, \"$1$\");\n"
            " }\n",
            descriptor_->field(1)->full_name());
      } else {
        ABSL_CHECK(utf8_check == Utf8CheckMode::kVerify);
        format(
            "  static bool ValidateValue(std::string* s) {\n"
            "#ifndef NDEBUG\n"
            "    ::$proto_ns$::internal::WireFormatLite::VerifyUtf8String(\n"
            "       s->data(), static_cast<int>(s->size()), "
            "::$proto_ns$::internal::"
            "WireFormatLite::PARSE, \"$1$\");\n"
            "#else\n"
            "    (void) s;\n"
            "#endif\n"
            "    return true;\n"
            " }\n",
            descriptor_->field(1)->full_name());
      }
    } else {
      format("  static bool ValidateValue(void*) { return true; }\n");
    }
    if (HasDescriptorMethods(descriptor_->file(), options_)) {
      format(
          "  using ::$proto_ns$::Message::MergeFrom;\n"
          ""
          "  ::$proto_ns$::Metadata GetMetadata() const final;\n");
    }
    format(
        "  friend struct ::$tablename$;\n"
        "};\n");
    return;
  }

  format(
      "class $dllexport_decl $${1$$classname$$}$ final :\n"
      "    public $superclass$ /* @@protoc_insertion_point("
      "class_definition:$full_name$) */ {\n",
      descriptor_);
  format(" public:\n");
  format.Indent();

  format("inline $classname$() : $classname$(nullptr) {}\n");
  if (!HasSimpleBaseClass(descriptor_, options_)) {
    format("~$classname$() override;\n");
  }
  format(
      "explicit PROTOBUF_CONSTEXPR "
      "$classname$(::$proto_ns$::internal::ConstantInitialized);\n"
      "\n"
      "$classname$(const $classname$& from);\n"
      "$classname$($classname$&& from) noexcept\n"
      "  : $classname$() {\n"
      "  *this = ::std::move(from);\n"
      "}\n"
      "\n"
      "inline $classname$& operator=(const $classname$& from) {\n"
      "  CopyFrom(from);\n"
      "  return *this;\n"
      "}\n"
      "inline $classname$& operator=($classname$&& from) noexcept {\n"
      "  if (this == &from) return *this;\n"
      "  if (GetOwningArena() == from.GetOwningArena()\n"
      "#ifdef PROTOBUF_FORCE_COPY_IN_MOVE\n"
      "      && GetOwningArena() != nullptr\n"
      "#endif  // !PROTOBUF_FORCE_COPY_IN_MOVE\n"
      "  ) {\n"
      "    InternalSwap(&from);\n"
      "  } else {\n"
      "    CopyFrom(from);\n"
      "  }\n"
      "  return *this;\n"
      "}\n"
      "\n");

  format(
      "inline const $unknown_fields_type$& unknown_fields() const {\n"
      "  return $unknown_fields$;\n"
      "}\n"
      "inline $unknown_fields_type$* mutable_unknown_fields() {\n"
      "  return $mutable_unknown_fields$;\n"
      "}\n"
      "\n");

  // Only generate this member if it's not disabled.
  if (HasDescriptorMethods(descriptor_->file(), options_) &&
      !descriptor_->options().no_standard_descriptor_accessor()) {
    format(
        "static const ::$proto_ns$::Descriptor* descriptor() {\n"
        "  return GetDescriptor();\n"
        "}\n");
  }

  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    // These shadow non-static methods of the same names in Message.  We
    // redefine them here because calls directly on the generated class can be
    // statically analyzed -- we know what descriptor types are being requested.
    // It also avoids a vtable dispatch.
    //
    // We would eventually like to eliminate the methods in Message, and having
    // this separate also lets us track calls to the base class methods
    // separately.
    format(
        "static const ::$proto_ns$::Descriptor* GetDescriptor() {\n"
        "  return default_instance().GetMetadata().descriptor;\n"
        "}\n"
        "static const ::$proto_ns$::Reflection* GetReflection() {\n"
        "  return default_instance().GetMetadata().reflection;\n"
        "}\n");
  }

  format(
      "static const $classname$& default_instance() {\n"
      "  return *internal_default_instance();\n"
      "}\n");

  // Generate enum values for every field in oneofs. One list is generated for
  // each oneof with an additional *_NOT_SET value.
  for (auto oneof : OneOfRange(descriptor_)) {
    format("enum $1$Case {\n", UnderscoresToCamelCase(oneof->name(), true));
    format.Indent();
    for (auto field : FieldRange(oneof)) {
      format("$1$ = $2$,\n", OneofCaseConstantName(field),  // 1
             field->number());                              // 2
    }
    format("$1$_NOT_SET = 0,\n", absl::AsciiStrToUpper(oneof->name()));
    format.Outdent();
    format(
        "};\n"
        "\n");
  }

  // TODO(gerbens) make this private, while still granting other protos access.
  format(
      "static inline const $classname$* internal_default_instance() {\n"
      "  return reinterpret_cast<const $classname$*>(\n"
      "             &_$classname$_default_instance_);\n"
      "}\n"
      "static constexpr int kIndexInFileMessages =\n"
      "  $1$;\n"
      "\n",
      index_in_file_messages_);

  if (IsAnyMessage(descriptor_, options_)) {
    format(
        "// implements Any -----------------------------------------------\n"
        "\n");
    if (HasDescriptorMethods(descriptor_->file(), options_)) {
      format(
          "bool PackFrom(const ::$proto_ns$::Message& message) {\n"
          "  $DCHK$_NE(&message, this);\n"
          "  return $any_metadata$.PackFrom(GetArena(), message);\n"
          "}\n"
          "bool PackFrom(const ::$proto_ns$::Message& message,\n"
          "              ::absl::string_view type_url_prefix) {\n"
          "  $DCHK$_NE(&message, this);\n"
          "  return $any_metadata$.PackFrom(GetArena(), message, "
          "type_url_prefix);\n"
          "}\n"
          "bool UnpackTo(::$proto_ns$::Message* message) const {\n"
          "  return $any_metadata$.UnpackTo(message);\n"
          "}\n"
          "static bool GetAnyFieldDescriptors(\n"
          "    const ::$proto_ns$::Message& message,\n"
          "    const ::$proto_ns$::FieldDescriptor** type_url_field,\n"
          "    const ::$proto_ns$::FieldDescriptor** value_field);\n"
          "template <typename T, class = typename std::enable_if<"
          "!std::is_convertible<T, const ::$proto_ns$::Message&>"
          "::value>::type>\n"
          "bool PackFrom(const T& message) {\n"
          "  return $any_metadata$.PackFrom<T>(GetArena(), message);\n"
          "}\n"
          "template <typename T, class = typename std::enable_if<"
          "!std::is_convertible<T, const ::$proto_ns$::Message&>"
          "::value>::type>\n"
          "bool PackFrom(const T& message,\n"
          "              ::absl::string_view type_url_prefix) {\n"
          "  return $any_metadata$.PackFrom<T>(GetArena(), message, "
          "type_url_prefix);"
          "}\n"
          "template <typename T, class = typename std::enable_if<"
          "!std::is_convertible<T, const ::$proto_ns$::Message&>"
          "::value>::type>\n"
          "bool UnpackTo(T* message) const {\n"
          "  return $any_metadata$.UnpackTo<T>(message);\n"
          "}\n");
    } else {
      format(
          "template <typename T>\n"
          "bool PackFrom(const T& message) {\n"
          "  return $any_metadata$.PackFrom(GetArena(), message);\n"
          "}\n"
          "template <typename T>\n"
          "bool PackFrom(const T& message,\n"
          "              ::absl::string_view type_url_prefix) {\n"
          "  return $any_metadata$.PackFrom(GetArena(), message, "
          "type_url_prefix);\n"
          "}\n"
          "template <typename T>\n"
          "bool UnpackTo(T* message) const {\n"
          "  return $any_metadata$.UnpackTo(message);\n"
          "}\n");
    }
    format(
        "template<typename T> bool Is() const {\n"
        "  return $any_metadata$.Is<T>();\n"
        "}\n"
        "static bool ParseAnyTypeUrl(::absl::string_view type_url,\n"
        "                            std::string* full_type_name);\n");
  }

  format(
      "friend void swap($classname$& a, $classname$& b) {\n"
      "  a.Swap(&b);\n"
      "}\n"
      "inline void Swap($classname$* other) {\n"
      "  if (other == this) return;\n"
      "#ifdef PROTOBUF_FORCE_COPY_IN_SWAP\n"
      "  if (GetOwningArena() != nullptr &&\n"
      "      GetOwningArena() == other->GetOwningArena()) {\n "
      "#else  // PROTOBUF_FORCE_COPY_IN_SWAP\n"
      "  if (GetOwningArena() == other->GetOwningArena()) {\n"
      "#endif  // !PROTOBUF_FORCE_COPY_IN_SWAP\n"
      "    InternalSwap(other);\n"
      "  } else {\n"
      "    ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);\n"
      "  }\n"
      "}\n"
      "void UnsafeArenaSwap($classname$* other) {\n"
      "  if (other == this) return;\n"
      "  $DCHK$(GetOwningArena() == other->GetOwningArena());\n"
      "  InternalSwap(other);\n"
      "}\n");

  format(
      "\n"
      "// implements Message ----------------------------------------------\n"
      "\n"
      "$classname$* New(::$proto_ns$::Arena* arena = nullptr) const final {\n"
      "  return CreateMaybeMessage<$classname$>(arena);\n"
      "}\n");

  // For instances that derive from Message (rather than MessageLite), some
  // methods are virtual and should be marked as final.
  auto v2 = p->WithVars(
      {{"full_final",
        HasDescriptorMethods(descriptor_->file(), options_) ? "final" : ""}});

  if (HasGeneratedMethods(descriptor_->file(), options_)) {
    if (HasDescriptorMethods(descriptor_->file(), options_)) {
      if (!HasSimpleBaseClass(descriptor_, options_)) {
        format(
            // Use Message's built-in MergeFrom and CopyFrom when the passed-in
            // argument is a generic Message instance, and only define the
            // custom MergeFrom and CopyFrom instances when the source of the
            // merge/copy is known to be the same class as the destination.
            "using $superclass$::CopyFrom;\n"
            "void CopyFrom(const $classname$& from);\n"
            ""
            "using $superclass$::MergeFrom;\n"
            "void MergeFrom("
            " const $classname$& from) {\n"
            "  $classname$::MergeImpl(*this, from);\n"
            "}\n"
            "private:\n"
            "static void MergeImpl(::$proto_ns$::Message& to_msg, const "
            "::$proto_ns$::Message& from_msg);\n"
            "public:\n");
      } else {
        format(
            "using $superclass$::CopyFrom;\n"
            "inline void CopyFrom(const $classname$& from) {\n"
            "  $superclass$::CopyImpl(*this, from);\n"
            "}\n"
            ""
            "using $superclass$::MergeFrom;\n"
            "void MergeFrom(const $classname$& from) {\n"
            "  $superclass$::MergeImpl(*this, from);\n"
            "}\n"
            "public:\n");
      }
    } else {
      format(
          "void CheckTypeAndMergeFrom(const ::$proto_ns$::MessageLite& from)"
          "  final;\n"
          "void CopyFrom(const $classname$& from);\n"
          "void MergeFrom(const $classname$& from);\n");
    }

    if (!HasSimpleBaseClass(descriptor_, options_)) {
      format(
          "PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;\n"
          "bool IsInitialized() const final;\n"
          "\n"
          "::size_t ByteSizeLong() const final;\n");

      parse_function_generator_->GenerateMethodDecls(p);

      format(
          "$uint8$* _InternalSerialize(\n"
          "    $uint8$* target, ::$proto_ns$::io::EpsCopyOutputStream* stream) "
          "const final;\n");
    }
  }

  if (options_.field_listener_options.inject_field_listener_events) {
    format("static constexpr int _kInternalFieldNumber = $1$;\n",
           descriptor_->field_count());
  }

  if (!HasSimpleBaseClass(descriptor_, options_)) {
    format(
        "int GetCachedSize() const final { return "
        "$cached_size$.Get(); }"
        "\n\nprivate:\n"
        "void SharedCtor(::$proto_ns$::Arena* arena);\n"
        "void SharedDtor();\n"
        "void SetCachedSize(int size) const$ full_final$;\n"
        "void InternalSwap($classname$* other);\n");
  }

  format(
      // Friend AnyMetadata so that it can call this FullMessageName() method.
      "\nprivate:\n"
      "friend class ::$proto_ns$::internal::AnyMetadata;\n"
      "static ::absl::string_view FullMessageName() {\n"
      "  return \"$full_name$\";\n"
      "}\n");

  format(
      // TODO(gerbens) Make this private! Currently people are deriving from
      // protos to give access to this constructor, breaking the invariants
      // we rely on.
      "protected:\n"
      "explicit $classname$(::$proto_ns$::Arena* arena);\n");

  switch (NeedsArenaDestructor()) {
    case ArenaDtorNeeds::kOnDemand:
      format(
          "private:\n"
          "static void ArenaDtor(void* object);\n"
          "inline void OnDemandRegisterArenaDtor(::$proto_ns$::Arena* arena) "
          "override {\n"
          "  if (arena == nullptr || ($inlined_string_donated_array$[0] & "
          "0x1u) "
          "== "
          "0) {\n"
          "   return;\n"
          "  }\n"
          "  $inlined_string_donated_array$[0] &= 0xFFFFFFFEu;\n"
          "  arena->OwnCustomDestructor(this, &$classname$::ArenaDtor);\n"
          "}\n");
      break;
    case ArenaDtorNeeds::kRequired:
      format(
          "private:\n"
          "static void ArenaDtor(void* object);\n");
      break;
    case ArenaDtorNeeds::kNone:
      break;
  }

  format(
      "public:\n"
      "\n");

  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    if (HasGeneratedMethods(descriptor_->file(), options_)) {
      format(
          "static const ClassData _class_data_;\n"
          "const ::$proto_ns$::Message::ClassData*"
          "GetClassData() const final;\n"
          "\n");
    }
    format(
        "::$proto_ns$::Metadata GetMetadata() const final;\n"
        "\n");
  } else {
    format(
        "std::string GetTypeName() const final;\n"
        "\n");
  }

  if (ShouldSplit(descriptor_, options_)) {
    format(
        "private:\n"
        "inline bool IsSplitMessageDefault() const {\n"
        "  return $split$ == reinterpret_cast<const Impl_::Split*>(&$1$);\n"
        "}\n"
        "PROTOBUF_NOINLINE void PrepareSplitMessageForWrite();\n"
        "public:\n",
        DefaultInstanceName(descriptor_, options_, /*split=*/true));
  }

  format(
      "// nested types ----------------------------------------------------\n"
      "\n");

  // Import all nested message classes into this class's scope with typedefs.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    const Descriptor* nested_type = descriptor_->nested_type(i);
    if (!IsMapEntryMessage(nested_type)) {
      auto v =
          p->WithVars({{"nested_full_name", ClassName(nested_type, false)},
                       {"nested_name", ResolveKeyword(nested_type->name())}});
      format("typedef ${1$$nested_full_name$$}$ ${1$$nested_name$$}$;\n",
             nested_type);
    }
  }

  if (descriptor_->nested_type_count() > 0) {
    format("\n");
  }

  // Import all nested enums and their values into this class's scope with
  // typedefs and constants.
  for (int i = 0; i < descriptor_->enum_type_count(); i++) {
    enum_generators_[i]->GenerateSymbolImports(p);
    format("\n");
  }

  format(
      "// accessors -------------------------------------------------------\n"
      "\n");

  // Generate accessor methods for all fields.
  GenerateFieldAccessorDeclarations(p);

  // Declare extension identifiers.
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    extension_generators_[i]->GenerateDeclaration(p);
  }


  format("// @@protoc_insertion_point(class_scope:$full_name$)\n");

  // Generate private members.
  format.Outdent();
  format(" private:\n");
  format.Indent();
  // TODO(seongkim): Remove hack to track field access and remove this class.
  format("class _Internal;\n");

  for (auto field : FieldRange(descriptor_)) {
    // set_has_***() generated in all oneofs.
    if (!field->is_repeated() && !field->options().weak() &&
        field->real_containing_oneof()) {
      format("void set_has_$1$();\n", FieldName(field));
    }
  }
  format("\n");

  // Generate oneof function declarations
  for (auto oneof : OneOfRange(descriptor_)) {
    format(
        "inline bool has_$1$() const;\n"
        "inline void clear_has_$1$();\n\n",
        oneof->name());
  }

  if (HasGeneratedMethods(descriptor_->file(), options_) &&
      !descriptor_->options().message_set_wire_format() &&
      num_required_fields_ > 1) {
    format(
        "// helper for ByteSizeLong()\n"
        "::size_t RequiredFieldsByteSizeFallback() const;\n\n");
  }

  if (HasGeneratedMethods(descriptor_->file(), options_)) {
    parse_function_generator_->GenerateDataDecls(p);
  }

  // Prepare decls for _cached_size_ and _has_bits_.  Their position in the
  // output will be determined later.

  bool need_to_emit_cached_size = !HasSimpleBaseClass(descriptor_, options_);
  const std::string cached_size_decl =
      "mutable ::$proto_ns$::internal::CachedSize _cached_size_;\n";

  const size_t sizeof_has_bits = HasBitsSize();
  const std::string has_bits_decl =
      sizeof_has_bits == 0 ? ""
                           : absl::StrCat("::$proto_ns$::internal::HasBits<",
                                          sizeof_has_bits, "> _has_bits_;\n");

  format(
      "template <typename T> friend class "
      "::$proto_ns$::Arena::InternalHelper;\n"
      "typedef void InternalArenaConstructable_;\n"
      "typedef void DestructorSkippable_;\n");

  // To minimize padding, data members are divided into three sections:
  // (1) members assumed to align to 8 bytes
  // (2) members corresponding to message fields, re-ordered to optimize
  //     alignment.
  // (3) members assumed to align to 4 bytes.

  format("struct Impl_ {\n");
  format.Indent();

  // Members assumed to align to 8 bytes:

  if (descriptor_->extension_range_count() > 0) {
    format(
        "::$proto_ns$::internal::ExtensionSet _extensions_;\n"
        "\n");
  }

  if (HasTracker(descriptor_, options_)) {
    format("static ::$proto_ns$::AccessListener<$1$> _tracker_;\n",
           ClassName(descriptor_));
  }

  // Generate _inlined_string_donated_ for inlined string type.
  // TODO(congliu): To avoid affecting the locality of `_has_bits_`, should this
  // be below or above `_has_bits_`?
  if (!inlined_string_indices_.empty()) {
    format("::$proto_ns$::internal::HasBits<$1$> _inlined_string_donated_;\n",
           InlinedStringDonatedSize());
  }

  if (!has_bit_indices_.empty()) {
    // _has_bits_ is frequently accessed, so to reduce code size and improve
    // speed, it should be close to the start of the object. Placing
    // _cached_size_ together with _has_bits_ improves cache locality despite
    // potential alignment padding.
    format(has_bits_decl.c_str());
    if (need_to_emit_cached_size) {
      format(cached_size_decl.c_str());
      need_to_emit_cached_size = false;
    }
  }

  // Field members:

  // Emit some private and static members
  for (auto field : optimized_order_) {
    field_generators_.get(field).GenerateStaticMembers(p);
    if (!ShouldSplit(field, options_)) {
      field_generators_.get(field).GeneratePrivateMembers(p);
    }
  }
  if (ShouldSplit(descriptor_, options_)) {
    format("struct Split {\n");
    format.Indent();
    for (auto field : optimized_order_) {
      if (!ShouldSplit(field, options_)) continue;
      field_generators_.get(field).GeneratePrivateMembers(p);
    }
    format.Outdent();
    format(
        "  typedef void InternalArenaConstructable_;\n"
        "  typedef void DestructorSkippable_;\n"
        "};\n"
        "static_assert(std::is_trivially_copy_constructible<Split>::value);\n"
        "static_assert(std::is_trivially_destructible<Split>::value);\n"
        "Split* _split_;\n");
  }

  // For each oneof generate a union
  for (auto oneof : OneOfRange(descriptor_)) {
    std::string camel_oneof_name = UnderscoresToCamelCase(oneof->name(), true);
    format("union $1$Union {\n", camel_oneof_name);
    format.Indent();
    format(
        // explicit empty constructor is needed when union contains
        // ArenaStringPtr members for string fields.
        "constexpr $1$Union() : _constinit_{} {}\n"
        "  ::$proto_ns$::internal::ConstantInitialized _constinit_;\n",
        camel_oneof_name);
    for (auto field : FieldRange(oneof)) {
      field_generators_.get(field).GeneratePrivateMembers(p);
    }
    format.Outdent();
    format("} $1$_;\n", oneof->name());
    for (auto field : FieldRange(oneof)) {
      field_generators_.get(field).GenerateStaticMembers(p);
    }
  }

  // Members assumed to align to 4 bytes:

  if (need_to_emit_cached_size) {
    format(cached_size_decl.c_str());
    need_to_emit_cached_size = false;
  }

  // Generate _oneof_case_.
  if (descriptor_->real_oneof_decl_count() > 0) {
    format(
        "$uint32$ _oneof_case_[$1$];\n"
        "\n",
        descriptor_->real_oneof_decl_count());
  }

  if (num_weak_fields_) {
    format("::$proto_ns$::internal::WeakFieldMap _weak_field_map_;\n");
  }
  // Generate _any_metadata_ for the Any type.
  if (IsAnyMessage(descriptor_, options_)) {
    format("::$proto_ns$::internal::AnyMetadata _any_metadata_;\n");
  }

  format.Outdent();
  format("};\n");

  // Only create the _impl_ field if it contains data.
  if (HasImplData(descriptor_, options_)) {
    format("union { Impl_ _impl_; };\n");
  }

  if (ShouldSplit(descriptor_, options_)) {
    format("friend struct $1$;\n",
           DefaultInstanceType(descriptor_, options_, /*split=*/true));
  }

  // The TableStruct struct needs access to the private parts, in order to
  // construct the offsets of all members.
  format("friend struct ::$tablename$;\n");

  format.Outdent();
  format("};");
  ABSL_DCHECK(!need_to_emit_cached_size);
}  // NOLINT(readability/fn_size)

void MessageGenerator::GenerateInlineMethods(io::Printer* p) {
  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  if (IsMapEntryMessage(descriptor_)) return;
  GenerateFieldAccessorDefinitions(p);

  // Generate oneof_case() functions.
  for (auto oneof : OneOfRange(descriptor_)) {
    Formatter format(p);
    auto v = p->WithVars({
        {"camel_oneof_name", UnderscoresToCamelCase(oneof->name(), true)},
        {"oneof_name", oneof->name()},
        {"oneof_index", oneof->index()},
    });
    format(
        "inline $classname$::$camel_oneof_name$Case $classname$::"
        "${1$$oneof_name$_case$}$() const {\n"
        "  return $classname$::$camel_oneof_name$Case("
        "$oneof_case$[$oneof_index$]);\n"
        "}\n",
        oneof);
  }
}

void MessageGenerator::GenerateSchema(io::Printer* p, int offset,
                                      int has_offset) {
  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  Formatter format(p);
  has_offset = !has_bit_indices_.empty() || IsMapEntryMessage(descriptor_)
                   ? offset + has_offset
                   : -1;
  int inlined_string_indices_offset;
  if (inlined_string_indices_.empty()) {
    inlined_string_indices_offset = -1;
  } else {
    ABSL_DCHECK_NE(has_offset, -1);
    ABSL_DCHECK(!IsMapEntryMessage(descriptor_));
    inlined_string_indices_offset = has_offset + has_bit_indices_.size();
  }
  format("{ $1$, $2$, $3$, sizeof($classtype$)},\n", offset, has_offset,
         inlined_string_indices_offset);
}

void MessageGenerator::GenerateClassMethods(io::Printer* p) {
  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  Formatter format(p);
  if (IsMapEntryMessage(descriptor_)) {
    format(
        "$classname$::$classname$() {}\n"
        "$classname$::$classname$(::$proto_ns$::Arena* arena)\n"
        "    : SuperType(arena) {}\n"
        "void $classname$::MergeFrom(const $classname$& other) {\n"
        "  MergeFromInternal(other);\n"
        "}\n");
    if (HasDescriptorMethods(descriptor_->file(), options_)) {
      if (!descriptor_->options().map_entry()) {
        format(
            "::$proto_ns$::Metadata $classname$::GetMetadata() const {\n"
            "$annotate_reflection$"
            "  return ::_pbi::AssignDescriptors(\n"
            "      &$desc_table$_getter, &$desc_table$_once,\n"
            "      $file_level_metadata$[$1$]);\n"
            "}\n",
            index_in_file_messages_);
      } else {
        format(
            "::$proto_ns$::Metadata $classname$::GetMetadata() const {\n"
            "  return ::_pbi::AssignDescriptors(\n"
            "      &$desc_table$_getter, &$desc_table$_once,\n"
            "      $file_level_metadata$[$1$]);\n"
            "}\n",
            index_in_file_messages_);
      }
    }
    return;
  }

  if (IsAnyMessage(descriptor_, options_)) {
    if (HasDescriptorMethods(descriptor_->file(), options_)) {
      format(
          "bool $classname$::GetAnyFieldDescriptors(\n"
          "    const ::$proto_ns$::Message& message,\n"
          "    const ::$proto_ns$::FieldDescriptor** type_url_field,\n"
          "    const ::$proto_ns$::FieldDescriptor** value_field) {\n"
          "  return ::_pbi::GetAnyFieldDescriptors(\n"
          "      message, type_url_field, value_field);\n"
          "}\n");
    }
    format(
        "bool $classname$::ParseAnyTypeUrl(\n"
        "    ::absl::string_view type_url,\n"
        "    std::string* full_type_name) {\n"
        "  return ::_pbi::ParseAnyTypeUrl(type_url, full_type_name);\n"
        "}\n"
        "\n");
  }

  format(
      "class $classname$::_Internal {\n"
      " public:\n");
  format.Indent();
  if (!has_bit_indices_.empty()) {
    format(
        "using HasBits = "
        "decltype(std::declval<$classname$>().$has_bits$);\n"
        "static constexpr ::int32_t kHasBitsOffset =\n"
        "  8 * PROTOBUF_FIELD_OFFSET($classname$, _impl_._has_bits_);\n");
  }
  if (descriptor_->real_oneof_decl_count() > 0) {
    format(
        "static constexpr ::int32_t kOneofCaseOffset =\n"
        "  PROTOBUF_FIELD_OFFSET($classtype$, $oneof_case$);\n");
  }
  for (auto field : FieldRange(descriptor_)) {
    auto t = p->WithVars(MakeTrackerCalls(field, options_));
    field_generators_.get(field).GenerateInternalAccessorDeclarations(p);
    if (HasHasbit(field)) {
      int has_bit_index = HasBitIndex(field);
      ABSL_CHECK_NE(has_bit_index, kNoHasbit) << field->full_name();
      format(
          "static void set_has_$1$(HasBits* has_bits) {\n"
          "  (*has_bits)[$2$] |= $3$u;\n"
          "}\n",
          FieldName(field), has_bit_index / 32, (1u << (has_bit_index % 32)));
    }
  }
  if (num_required_fields_ > 0) {
    const std::vector<uint32_t> masks_for_has_bits = RequiredFieldsBitMask();
    format(
        "static bool MissingRequiredFields(const HasBits& has_bits) "
        "{\n"
        "  return $1$;\n"
        "}\n",
        ConditionalToCheckBitmasks(masks_for_has_bits, false, "has_bits"));
  }

  format.Outdent();
  format("};\n\n");
  for (auto field : FieldRange(descriptor_)) {
    field_generators_.get(field).GenerateInternalAccessorDefinitions(p);
  }

  // Generate non-inline field definitions.
  for (auto field : FieldRange(descriptor_)) {
    auto v = p->WithVars(FieldVars(field, options_));
    auto t = p->WithVars(MakeTrackerCalls(field, options_));
    field_generators_.get(field).GenerateNonInlineAccessorDefinitions(p);
    if (IsCrossFileMaybeMap(field)) {
      GenerateFieldClear(field, false, p);
    }
  }

  GenerateStructors(p);
  format("\n");

  if (descriptor_->real_oneof_decl_count() > 0) {
    GenerateOneofClear(p);
    format("\n");
  }

  if (HasGeneratedMethods(descriptor_->file(), options_)) {
    GenerateClear(p);
    format("\n");

    if (!HasSimpleBaseClass(descriptor_, options_)) {
      parse_function_generator_->GenerateMethodImpls(p);
      format("\n");

      parse_function_generator_->GenerateDataDefinitions(p);
    }

    GenerateSerializeWithCachedSizesToArray(p);
    format("\n");

    GenerateByteSize(p);
    format("\n");

    GenerateMergeFrom(p);
    format("\n");

    GenerateClassSpecificMergeImpl(p);
    format("\n");

    GenerateCopyFrom(p);
    format("\n");

    GenerateIsInitialized(p);
    format("\n");
  }

  if (ShouldSplit(descriptor_, options_)) {
    format(
        "void $classname$::PrepareSplitMessageForWrite() {\n"
        "  if (IsSplitMessageDefault()) {\n"
        "    void* chunk = "
        "::PROTOBUF_NAMESPACE_ID::internal::CreateSplitMessageGeneric("
        "GetArenaForAllocation(), &$1$, sizeof(Impl_::Split), this, &$2$);\n"
        "    $split$ = reinterpret_cast<Impl_::Split*>(chunk);\n"
        "  }\n"
        "}\n",
        DefaultInstanceName(descriptor_, options_, /*split=*/true),
        DefaultInstanceName(descriptor_, options_, /*split=*/false));
  }

  GenerateVerify(p);

  GenerateSwap(p);
  format("\n");

  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    if (!descriptor_->options().map_entry()) {
      format(
          "::$proto_ns$::Metadata $classname$::GetMetadata() const {\n"
          "$annotate_reflection$"
          "  return ::_pbi::AssignDescriptors(\n"
          "      &$desc_table$_getter, &$desc_table$_once,\n"
          "      $file_level_metadata$[$1$]);\n"
          "}\n",
          index_in_file_messages_);
    } else {
      format(
          "::$proto_ns$::Metadata $classname$::GetMetadata() const {\n"
          "  return ::_pbi::AssignDescriptors(\n"
          "      &$desc_table$_getter, &$desc_table$_once,\n"
          "      $file_level_metadata$[$1$]);\n"
          "}\n",
          index_in_file_messages_);
    }
  } else {
    format(
        "std::string $classname$::GetTypeName() const {\n"
        "  return \"$full_name$\";\n"
        "}\n"
        "\n");
  }

  if (HasTracker(descriptor_, options_)) {
    format(
        "::$proto_ns$::AccessListener<$classtype$> "
        "$1$::$tracker$(&FullMessageName);\n",
        ClassName(descriptor_));
  }
}

std::pair<size_t, size_t> MessageGenerator::GenerateOffsets(io::Printer* p) {
  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  Formatter format(p);

  if (!has_bit_indices_.empty() || IsMapEntryMessage(descriptor_)) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, $has_bits$),\n");
  } else {
    format("~0u,  // no _has_bits_\n");
  }
  format("PROTOBUF_FIELD_OFFSET($classtype$, _internal_metadata_),\n");
  if (descriptor_->extension_range_count() > 0) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, $extensions$),\n");
  } else {
    format("~0u,  // no _extensions_\n");
  }
  if (descriptor_->real_oneof_decl_count() > 0) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, $oneof_case$[0]),\n");
  } else {
    format("~0u,  // no _oneof_case_\n");
  }
  if (num_weak_fields_ > 0) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, $weak_field_map$),\n");
  } else {
    format("~0u,  // no _weak_field_map_\n");
  }
  if (!inlined_string_indices_.empty()) {
    format(
        "PROTOBUF_FIELD_OFFSET($classtype$, "
        "$inlined_string_donated_array$),\n");
  } else {
    format("~0u,  // no _inlined_string_donated_\n");
  }
  if (ShouldSplit(descriptor_, options_)) {
    format(
        "PROTOBUF_FIELD_OFFSET($classtype$, $split$),\n"
        "sizeof($classtype$::Impl_::Split),\n");
  } else {
    format(
        "~0u,  // no _split_\n"
        "~0u,  // no sizeof(Split)\n");
  }
  const int kNumGenericOffsets = 8;  // the number of fixed offsets above
  const size_t offsets = kNumGenericOffsets + descriptor_->field_count() +
                         descriptor_->real_oneof_decl_count();
  size_t entries = offsets;
  for (auto field : FieldRange(descriptor_)) {
    // TODO(sbenza): We should not have an entry in the offset table for fields
    // that do not use them.
    if (field->options().weak() || field->real_containing_oneof()) {
      // Mark the field to prevent unintentional access through reflection.
      // Don't use the top bit because that is for unused fields.
      format("::_pbi::kInvalidFieldOffsetTag");
    } else {
      format("PROTOBUF_FIELD_OFFSET($classtype$$1$, $2$)",
             ShouldSplit(field, options_) ? "::Impl_::Split" : "",
             ShouldSplit(field, options_)
                 ? absl::StrCat(FieldName(field), "_")
                 : FieldMemberName(field, /*cold=*/false));
    }

    // Some information about a field is in the pdproto profile. The profile is
    // only available at compile time. So we embed such information in the
    // offset of the field, so that the information is available when
    // reflectively accessing the field at run time.
    //
    // We embed whether the field is cold to the MSB of the offset, and whether
    // the field is eagerly verified lazy or inlined string to the LSB of the
    // offset.

    if (ShouldSplit(field, options_)) {
      format(" | ::_pbi::kSplitFieldOffsetMask /*split*/");
    }
    if (IsEagerlyVerifiedLazy(field, options_, scc_analyzer_)) {
      format(" | 0x1u /*eagerly verified lazy*/");
    } else if (IsStringInlined(field, options_)) {
      format(" | 0x1u /*inlined*/");
    }
    format(",\n");
  }

  int count = 0;
  for (auto oneof : OneOfRange(descriptor_)) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, _impl_.$1$_),\n", oneof->name());
    count++;
  }
  ABSL_CHECK_EQ(count, descriptor_->real_oneof_decl_count());

  if (IsMapEntryMessage(descriptor_)) {
    entries += 2;
    format(
        "0,\n"
        "1,\n");
  } else if (!has_bit_indices_.empty()) {
    entries += has_bit_indices_.size();
    for (int i = 0; i < has_bit_indices_.size(); i++) {
      const std::string index =
          has_bit_indices_[i] >= 0 ? absl::StrCat(has_bit_indices_[i]) : "~0u";
      format("$1$,\n", index);
    }
  }
  if (!inlined_string_indices_.empty()) {
    entries += inlined_string_indices_.size();
    for (int inlined_string_index : inlined_string_indices_) {
      const std::string index =
          inlined_string_index >= 0
              ? absl::StrCat(inlined_string_index, ",  // inlined_string_index")
              : "~0u,";
      format("$1$\n", index);
    }
  }

  return std::make_pair(entries, offsets);
}

void MessageGenerator::GenerateSharedConstructorCode(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  Formatter format(p);

  format(
      "inline void $classname$::SharedCtor(::_pb::Arena* arena) {\n"
      "  (void)arena;\n");

  format.Indent();
  // Impl_ _impl_.
  format("new (&_impl_) Impl_{");
  format.Indent();
  const char* field_sep = " ";
  const auto put_sep = [&] {
    format("\n$1$ ", field_sep);
    field_sep = ",";
  };

  // Note: any fields without move/copy constructors can't be explicitly
  // aggregate initialized pre-C++17.
  if (descriptor_->extension_range_count() > 0) {
    put_sep();
    format("/*decltype($extensions$)*/{::_pbi::ArenaInitialized(), arena}");
  }
  if (!inlined_string_indices_.empty()) {
    put_sep();
    format("decltype($inlined_string_donated_array$){}");
  }
  bool need_to_emit_cached_size = !HasSimpleBaseClass(descriptor_, options_);
  if (!has_bit_indices_.empty()) {
    put_sep();
    format("decltype($has_bits$){}");
    if (need_to_emit_cached_size) {
      put_sep();
      format("/*decltype($cached_size$)*/{}");
      need_to_emit_cached_size = false;
    }
  }

  // Initialize member variables with arena constructor.
  for (auto field : optimized_order_) {
    if (ShouldSplit(field, options_)) {
      continue;
    }
    put_sep();
    field_generators_.get(field).GenerateAggregateInitializer(p);
  }
  if (ShouldSplit(descriptor_, options_)) {
    put_sep();
    // We can't assign the default split to this->split without the const_cast
    // because the former is a const. The const_cast is safe because we don't
    // intend to modify the default split through this pointer, and we also
    // expect the default split to be in the rodata section which is protected
    // from mutation.
    format(
        "decltype($split$){const_cast<Impl_::Split*>"
        "(reinterpret_cast<const Impl_::Split*>(&$1$))}",
        DefaultInstanceName(descriptor_, options_, /*split=*/true));
  }
  for (auto oneof : OneOfRange(descriptor_)) {
    put_sep();
    format("decltype(_impl_.$1$_){}", oneof->name());
  }

  if (need_to_emit_cached_size) {
    put_sep();
    format("/*decltype($cached_size$)*/{}");
  }

  if (descriptor_->real_oneof_decl_count() != 0) {
    put_sep();
    format("/*decltype($oneof_case$)*/{}");
  }
  if (num_weak_fields_ > 0) {
    put_sep();
    format("decltype($weak_field_map$){arena}");
  }
  if (IsAnyMessage(descriptor_, options_)) {
    put_sep();
    // AnyMetadata has no move constructor.
    format("/*decltype($any_metadata$)*/{&_impl_.type_url_, &_impl_.value_}");
  }

  format.Outdent();
  format("\n};\n");

  if (!inlined_string_indices_.empty()) {
    // Donate inline string fields.
    format.Indent();
    // The last bit is the tracking bit for registering ArenaDtor. The bit is 1
    // means ArenaDtor is not registered on construction, and on demand register
    // is needed.
    format("if (arena != nullptr) {\n");
    if (NeedsArenaDestructor() == ArenaDtorNeeds::kOnDemand) {
      format("  $inlined_string_donated_array$[0] = ~0u;\n");
    } else {
      format("  $inlined_string_donated_array$[0] = 0xFFFFFFFEu;\n");
    }
    for (size_t i = 1; i < InlinedStringDonatedSize(); ++i) {
      format("  $inlined_string_donated_array$[$1$] = ~0u;\n", i);
    }
    format("}\n");
    format.Outdent();
  }

  for (const FieldDescriptor* field : optimized_order_) {
    if (ShouldSplit(field, options_)) {
      continue;
    }
    field_generators_.get(field).GenerateConstructorCode(p);
  }

  if (ShouldForceAllocationOnConstruction(descriptor_, options_)) {
    format(
        "#ifdef PROTOBUF_FORCE_ALLOCATION_ON_CONSTRUCTION\n"
        "$mutable_unknown_fields$;\n"
        "#endif // PROTOBUF_FORCE_ALLOCATION_ON_CONSTRUCTION\n");
  }

  for (auto oneof : OneOfRange(descriptor_)) {
    format("clear_has_$1$();\n", oneof->name());
  }

  format.Outdent();
  format("}\n\n");
}

void MessageGenerator::GenerateInitDefaultSplitInstance(io::Printer* p) {
  if (!ShouldSplit(descriptor_, options_)) return;

  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  Formatter format(p);
  const char* field_sep = " ";
  const auto put_sep = [&] {
    format("\n$1$ ", field_sep);
    field_sep = ",";
  };
  for (const auto* field : optimized_order_) {
    if (ShouldSplit(field, options_)) {
      put_sep();
      field_generators_.get(field).GenerateConstexprAggregateInitializer(p);
    }
  }
}

void MessageGenerator::GenerateSharedDestructorCode(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  Formatter format(p);

  format("inline void $classname$::SharedDtor() {\n");
  format.Indent();
  format("$DCHK$(GetArenaForAllocation() == nullptr);\n");

  if (descriptor_->extension_range_count() > 0) {
    format("$extensions$.~ExtensionSet();\n");
  }

  // Write the destructors for each field except oneof members.
  // optimized_order_ does not contain oneof fields.
  for (auto field : optimized_order_) {
    if (ShouldSplit(field, options_)) {
      continue;
    }
    field_generators_.get(field).GenerateDestructorCode(p);
  }
  if (ShouldSplit(descriptor_, options_)) {
    format("if (!IsSplitMessageDefault()) {\n");
    format.Indent();
    format("auto* $cached_split_ptr$ = $split$;\n");
    for (auto field : optimized_order_) {
      if (ShouldSplit(field, options_)) {
        field_generators_.get(field).GenerateDestructorCode(p);
      }
    }
    format("delete $cached_split_ptr$;\n");
    format.Outdent();
    format("}\n");
  }

  // Generate code to destruct oneofs. Clearing should do the work.
  for (auto oneof : OneOfRange(descriptor_)) {
    format(
        "if (has_$1$()) {\n"
        "  clear_$1$();\n"
        "}\n",
        oneof->name());
  }

  if (num_weak_fields_) {
    format("$weak_field_map$.ClearAll();\n");
  }

  if (IsAnyMessage(descriptor_, options_)) {
    format("$any_metadata$.~AnyMetadata();\n");
  }

  format.Outdent();
  format(
      "}\n"
      "\n");
}

ArenaDtorNeeds MessageGenerator::NeedsArenaDestructor() const {
  if (HasSimpleBaseClass(descriptor_, options_)) return ArenaDtorNeeds::kNone;
  ArenaDtorNeeds needs = ArenaDtorNeeds::kNone;
  for (const auto* field : FieldRange(descriptor_)) {
    needs =
        std::max(needs, field_generators_.get(field).NeedsArenaDestructor());
  }
  return needs;
}

void MessageGenerator::GenerateArenaDestructorCode(io::Printer* p) {
  ABSL_CHECK(NeedsArenaDestructor() > ArenaDtorNeeds::kNone);

  Formatter format(p);

  // Generate the ArenaDtor() method. Track whether any fields actually produced
  // code that needs to be called.
  format("void $classname$::ArenaDtor(void* object) {\n");
  format.Indent();

  // This code is placed inside a static method, rather than an ordinary one,
  // since that simplifies Arena's destructor list (ordinary function pointers
  // rather than member function pointers). _this is the object being
  // destructed.
  format("$classname$* _this = reinterpret_cast< $classname$* >(object);\n");

  // Process non-oneof fields first.
  for (auto field : optimized_order_) {
    if (ShouldSplit(field, options_)) continue;
    field_generators_.get(field).GenerateArenaDestructorCode(p);
  }
  if (ShouldSplit(descriptor_, options_)) {
    format("if (!_this->IsSplitMessageDefault()) {\n");
    format.Indent();
    for (auto field : optimized_order_) {
      if (!ShouldSplit(field, options_)) continue;
      field_generators_.get(field).GenerateArenaDestructorCode(p);
    }
    format.Outdent();
    format("}\n");
  }

  // Process oneof fields.
  for (auto oneof : OneOfRange(descriptor_)) {
    for (auto field : FieldRange(oneof)) {
      field_generators_.get(field).GenerateArenaDestructorCode(p);
    }
  }

  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateConstexprConstructor(io::Printer* p) {
  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  Formatter format(p);

  if (IsMapEntryMessage(descriptor_) || !HasImplData(descriptor_, options_)) {
    format(
        "PROTOBUF_CONSTEXPR $classname$::$classname$(\n"
        "    ::_pbi::ConstantInitialized) {}\n");
    return;
  }

  format(
      "PROTOBUF_CONSTEXPR $classname$::$classname$(\n"
      "    ::_pbi::ConstantInitialized)");

  bool need_to_emit_cached_size = !HasSimpleBaseClass(descriptor_, options_);
  format(": _impl_{");
  format.Indent();
  const char* field_sep = " ";
  const auto put_sep = [&] {
    format("\n$1$ ", field_sep);
    field_sep = ",";
  };
  if (descriptor_->extension_range_count() > 0) {
    put_sep();
    format("/*decltype($extensions$)*/{}");
  }
  if (!inlined_string_indices_.empty()) {
    put_sep();
    format("/*decltype($inlined_string_donated_array$)*/{}");
  }
  if (!has_bit_indices_.empty()) {
    put_sep();
    format("/*decltype($has_bits$)*/{}");
    if (need_to_emit_cached_size) {
      put_sep();
      format("/*decltype($cached_size$)*/{}");
      need_to_emit_cached_size = false;
    }
  }
  for (auto field : optimized_order_) {
    if (ShouldSplit(field, options_)) {
      continue;
    }
    put_sep();
    field_generators_.get(field).GenerateConstexprAggregateInitializer(p);
  }
  if (ShouldSplit(descriptor_, options_)) {
    put_sep();
    format("/*decltype($split$)*/const_cast<Impl_::Split*>(&$1$._instance)",
           DefaultInstanceName(descriptor_, options_, /*split=*/true));
  }

  for (auto oneof : OneOfRange(descriptor_)) {
    put_sep();
    format("/*decltype(_impl_.$1$_)*/{}", oneof->name());
  }

  if (need_to_emit_cached_size) {
    put_sep();
    format("/*decltype($cached_size$)*/{}");
  }

  if (descriptor_->real_oneof_decl_count() != 0) {
    put_sep();
    format("/*decltype($oneof_case$)*/{}");
  }

  if (num_weak_fields_) {
    put_sep();
    format("/*decltype($weak_field_map$)*/{}");
  }

  if (IsAnyMessage(descriptor_, options_)) {
    put_sep();
    format(
        "/*decltype($any_metadata$)*/{&_impl_.type_url_, "
        "&_impl_.value_}");
  }

  format.Outdent();
  format("} {}\n");
}

void MessageGenerator::GenerateCopyConstructorBody(io::Printer* p) const {
  Formatter format(p);

  const RunMap runs =
      FindRuns(optimized_order_, [this](const FieldDescriptor* field) {
        return IsPOD(field) && !ShouldSplit(field, options_);
      });

  std::string pod_template =
      "::memcpy(&$first$, &from.$first$,\n"
      "  static_cast<::size_t>(reinterpret_cast<char*>(&$last$) -\n"
      "  reinterpret_cast<char*>(&$first$)) + sizeof($last$));\n";

  if (ShouldForceAllocationOnConstruction(descriptor_, options_)) {
    format(
        "#ifdef PROTOBUF_FORCE_ALLOCATION_ON_CONSTRUCTION\n"
        "$mutable_unknown_fields$;\n"
        "#endif // PROTOBUF_FORCE_ALLOCATION_ON_CONSTRUCTION\n");
  }

  for (size_t i = 0; i < optimized_order_.size(); ++i) {
    const FieldDescriptor* field = optimized_order_[i];
    if (ShouldSplit(field, options_)) {
      continue;
    }
    const auto it = runs.find(field);

    // We only apply the memset technique to runs of more than one field, as
    // assignment is better than memset for generated code clarity.
    if (it != runs.end() && it->second > 1) {
      // Use a memset, then skip run_length fields.
      const size_t run_length = it->second;
      const std::string first_field_name =
          FieldMemberName(field, /*cold=*/false);
      const std::string last_field_name =
          FieldMemberName(optimized_order_[i + run_length - 1], /*cold=*/false);

      auto v = p->WithVars({
          {"first", first_field_name},
          {"last", last_field_name},
      });
      format(pod_template.c_str());

      i += run_length - 1;
      // ++i at the top of the loop.
    } else {
      field_generators_.get(field).GenerateCopyConstructorCode(p);
    }
  }

  if (ShouldSplit(descriptor_, options_)) {
    format("if (!from.IsSplitMessageDefault()) {\n");
    format.Indent();
    format("_this->PrepareSplitMessageForWrite();\n");
    // TODO(b/122856539): cache the split pointers.
    for (auto field : optimized_order_) {
      if (ShouldSplit(field, options_)) {
        field_generators_.get(field).GenerateCopyConstructorCode(p);
      }
    }
    format.Outdent();
    format("}\n");
  }
}

void MessageGenerator::GenerateStructors(io::Printer* p) {
  Formatter format(p);

  format(
      "$classname$::$classname$(::$proto_ns$::Arena* arena)\n"
      "  : $1$(arena) {\n",
      SuperClassName(descriptor_, options_));

  if (!HasSimpleBaseClass(descriptor_, options_)) {
    format("  SharedCtor(arena);\n");
    if (NeedsArenaDestructor() == ArenaDtorNeeds::kRequired) {
      format(
          "  if (arena != nullptr) {\n"
          "    arena->OwnCustomDestructor(this, &$classname$::ArenaDtor);\n"
          "  }\n");
    }
  }
  format(
      "  // @@protoc_insertion_point(arena_constructor:$full_name$)\n"
      "}\n");

  // If the message contains only scalar fields (ints and enums),
  // then we can copy the entire impl_ section with a single statement.
  bool copy_construct_impl =
      !ShouldSplit(descriptor_, options_) &&
      !HasSimpleBaseClass(descriptor_, options_) &&
      (descriptor_->extension_range_count() == 0 &&
       descriptor_->real_oneof_decl_count() == 0 && num_weak_fields_ == 0);
  for (const auto& field : optimized_order_) {
    if (!copy_construct_impl) break;
    if (field->is_repeated() || field->is_extension()) {
      copy_construct_impl = false;
    } else if (field->cpp_type() != FieldDescriptor::CPPTYPE_ENUM &&
               field->cpp_type() != FieldDescriptor::CPPTYPE_INT32 &&
               field->cpp_type() != FieldDescriptor::CPPTYPE_INT64 &&
               field->cpp_type() != FieldDescriptor::CPPTYPE_UINT32 &&
               field->cpp_type() != FieldDescriptor::CPPTYPE_UINT64 &&
               field->cpp_type() != FieldDescriptor::CPPTYPE_FLOAT &&
               field->cpp_type() != FieldDescriptor::CPPTYPE_DOUBLE &&
               field->cpp_type() != FieldDescriptor::CPPTYPE_BOOL) {
      copy_construct_impl = false;
    } else {
      // non-repeated integer fields are fine to copy en masse.
    }
  }

  // Generate the copy constructor.
  if (UsingImplicitWeakFields(descriptor_->file(), options_)) {
    // If we are in lite mode and using implicit weak fields, we generate a
    // one-liner copy constructor that delegates to MergeFrom. This saves some
    // code size and also cuts down on the complexity of implicit weak fields.
    // We might eventually want to do this for all lite protos.
    format(
        "$classname$::$classname$(const $classname$& from)\n"
        "  : $classname$() {\n"
        "  MergeFrom(from);\n"
        "}\n");
  } else if (copy_construct_impl) {
    format(
        "$classname$::$classname$(const $classname$& from)\n"
        "  : $superclass$(), _impl_(from._impl_) {\n"
        "  _internal_metadata_.MergeFrom<$unknown_fields_type$>(\n"
        "      from._internal_metadata_);\n");
    format(
        "  // @@protoc_insertion_point(copy_constructor:$full_name$)\n"
        "}\n"
        "\n");
  } else {
    format(
        "$classname$::$classname$(const $classname$& from)\n"
        "  : $superclass$() {\n");
    format.Indent();
    format("$classname$* const _this = this; (void)_this;\n");

    if (HasImplData(descriptor_, options_)) {
      const char* field_sep = " ";
      const auto put_sep = [&] {
        format("\n$1$ ", field_sep);
        field_sep = ",";
      };

      format("new (&_impl_) Impl_{");
      format.Indent();

      if (descriptor_->extension_range_count() > 0) {
        put_sep();
        format("/*decltype($extensions$)*/{}");
      }
      if (!inlined_string_indices_.empty()) {
        // Do not copy inlined_string_donated_, because this is not an arena
        // constructor.
        put_sep();
        format("decltype($inlined_string_donated_array$){}");
      }
      bool need_to_emit_cached_size =
          !HasSimpleBaseClass(descriptor_, options_);
      if (!has_bit_indices_.empty()) {
        put_sep();
        format("decltype($has_bits$){from.$has_bits$}");
        if (need_to_emit_cached_size) {
          put_sep();
          format("/*decltype($cached_size$)*/{}");
          need_to_emit_cached_size = false;
        }
      }

      // Initialize member variables with arena constructor.
      for (auto field : optimized_order_) {
        if (ShouldSplit(field, options_)) {
          continue;
        }
        put_sep();
        field_generators_.get(field).GenerateCopyAggregateInitializer(p);
      }
      if (ShouldSplit(descriptor_, options_)) {
        put_sep();
        format(
            "decltype($split$){const_cast<Impl_::Split*>"
            "(reinterpret_cast<const Impl_::Split*>(&$1$))}",
            DefaultInstanceName(descriptor_, options_, /*split=*/true));
      }
      for (auto oneof : OneOfRange(descriptor_)) {
        put_sep();
        format("decltype(_impl_.$1$_){}", oneof->name());
      }

      if (need_to_emit_cached_size) {
        put_sep();
        format("/*decltype($cached_size$)*/{}");
      }

      if (descriptor_->real_oneof_decl_count() != 0) {
        put_sep();
        format("/*decltype($oneof_case$)*/{}");
      }
      if (num_weak_fields_ > 0) {
        put_sep();
        format("decltype($weak_field_map$){from.$weak_field_map$}");
      }
      if (IsAnyMessage(descriptor_, options_)) {
        put_sep();
        format(
            "/*decltype($any_metadata$)*/{&_impl_.type_url_, &_impl_.value_}");
      }
      format.Outdent();
      format("};\n\n");
    }

    format(
        "_internal_metadata_.MergeFrom<$unknown_fields_type$>(from._internal_"
        "metadata_);\n");

    if (descriptor_->extension_range_count() > 0) {
      format(
          "$extensions$.MergeFrom(internal_default_instance(), "
          "from.$extensions$);\n");
    }

    GenerateCopyConstructorBody(p);

    // Copy oneof fields. Oneof field requires oneof case check.
    for (auto oneof : OneOfRange(descriptor_)) {
      format(
          "clear_has_$1$();\n"
          "switch (from.$1$_case()) {\n",
          oneof->name());
      format.Indent();
      for (auto field : FieldRange(oneof)) {
        format("case k$1$: {\n", UnderscoresToCamelCase(field->name(), true));
        format.Indent();
        field_generators_.get(field).GenerateMergingCode(p);
        format("break;\n");
        format.Outdent();
        format("}\n");
      }
      format(
          "case $1$_NOT_SET: {\n"
          "  break;\n"
          "}\n",
          absl::AsciiStrToUpper(oneof->name()));
      format.Outdent();
      format("}\n");
    }

    format.Outdent();
    format(
        "  // @@protoc_insertion_point(copy_constructor:$full_name$)\n"
        "}\n"
        "\n");
  }

  // Generate the shared constructor code.
  GenerateSharedConstructorCode(p);

  // Generate the destructor.
  if (!HasSimpleBaseClass(descriptor_, options_)) {
    format(
        "$classname$::~$classname$() {\n"
        "  // @@protoc_insertion_point(destructor:$full_name$)\n");
    format(
        "  if (auto *arena = "
        "_internal_metadata_.DeleteReturnArena<$unknown_fields_type$>()) {\n"
        "  (void)arena;\n");
    if (NeedsArenaDestructor() > ArenaDtorNeeds::kNone) {
      format("    ArenaDtor(this);\n");
    }
    format(
        "    return;\n"
        "  }\n");
    format(
        "  SharedDtor();\n"
        "}\n"
        "\n");
  } else {
    // For messages using simple base classes, having no destructor
    // allows our vtable to share the same destructor as every other
    // message with a simple base class.  This works only as long as
    // we have no fields needing destruction, of course.  (No strings
    // or extensions)
  }

  // Generate the shared destructor code.
  GenerateSharedDestructorCode(p);

  // Generate the arena-specific destructor code.
  if (NeedsArenaDestructor() > ArenaDtorNeeds::kNone) {
    GenerateArenaDestructorCode(p);
  }

  if (!HasSimpleBaseClass(descriptor_, options_)) {
    // Generate SetCachedSize.
    format(
        "void $classname$::SetCachedSize(int size) const {\n"
        "  $cached_size$.Set(size);\n"
        "}\n");
  }
}

void MessageGenerator::GenerateSourceInProto2Namespace(io::Printer* p) {
  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  Formatter format(p);
  format(
      "template<> "
      "PROTOBUF_NOINLINE $classtype$*\n"
      "Arena::CreateMaybeMessage< $classtype$ >(Arena* arena) {\n"
      "  return Arena::CreateMessageInternal< $classtype$ >(arena);\n"
      "}\n");
}

void MessageGenerator::GenerateClear(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  Formatter format(p);

  // The maximum number of bytes we will memset to zero without checking their
  // hasbit to see if a zero-init is necessary.
  const int kMaxUnconditionalPrimitiveBytesClear = 4;

  format(
      "void $classname$::Clear() {\n"
      "// @@protoc_insertion_point(message_clear_start:$full_name$)\n");
  format.Indent();

  format(
      // TODO(jwb): It would be better to avoid emitting this if it is not used,
      // rather than emitting a workaround for the resulting warning.
      "$uint32$ cached_has_bits = 0;\n"
      "// Prevent compiler warnings about cached_has_bits being unused\n"
      "(void) cached_has_bits;\n\n");

  if (descriptor_->extension_range_count() > 0) {
    format("$extensions$.Clear();\n");
  }

  // Collect fields into chunks. Each chunk may have an if() condition that
  // checks all hasbits in the chunk and skips it if none are set.
  int zero_init_bytes = 0;
  for (const auto& field : optimized_order_) {
    if (CanClearByZeroing(field)) {
      zero_init_bytes += EstimateAlignmentSize(field);
    }
  }
  bool merge_zero_init = zero_init_bytes > kMaxUnconditionalPrimitiveBytesClear;
  int chunk_count = 0;

  std::vector<std::vector<const FieldDescriptor*>> chunks = CollectFields(
      optimized_order_,
      [&](const FieldDescriptor* a, const FieldDescriptor* b) -> bool {
        chunk_count++;
        // This predicate guarantees that there is only a single zero-init
        // (memset) per chunk, and if present it will be at the beginning.
        bool same =
            HasByteIndex(a) == HasByteIndex(b) &&
            a->is_repeated() == b->is_repeated() &&
            ShouldSplit(a, options_) == ShouldSplit(b, options_) &&
            (CanClearByZeroing(a) == CanClearByZeroing(b) ||
             (CanClearByZeroing(a) && (chunk_count == 1 || merge_zero_init)));
        if (!same) chunk_count = 0;
        return same;
      });

  ColdChunkSkipper cold_skipper(descriptor_, options_, chunks, has_bit_indices_,
                                kColdRatio);
  int cached_has_word_index = -1;
  bool first_split_chunk_processed = false;
  for (size_t chunk_index = 0; chunk_index < chunks.size(); chunk_index++) {
    std::vector<const FieldDescriptor*>& chunk = chunks[chunk_index];
    cold_skipper.OnStartChunk(chunk_index, cached_has_word_index, "", p);

    const FieldDescriptor* memset_start = nullptr;
    const FieldDescriptor* memset_end = nullptr;
    bool saw_non_zero_init = false;
    bool chunk_is_split =
        !chunk.empty() && ShouldSplit(chunk.front(), options_);
    // All chunks after the first split chunk should also be split.
    ABSL_CHECK(!first_split_chunk_processed || chunk_is_split);
    if (chunk_is_split && !first_split_chunk_processed) {
      // Some fields are cleared without checking has_bit. So we add the
      // condition here to avoid writing to the default split instance.
      format("if (!IsSplitMessageDefault()) {\n");
      format.Indent();
      first_split_chunk_processed = true;
    }

    for (const auto& field : chunk) {
      if (CanClearByZeroing(field)) {
        ABSL_CHECK(!saw_non_zero_init);
        if (!memset_start) memset_start = field;
        memset_end = field;
      } else {
        saw_non_zero_init = true;
      }
    }

    // Whether we wrap this chunk in:
    //   if (cached_has_bits & <chunk hasbits) { /* chunk. */ }
    // We can omit the if() for chunk size 1, or if our fields do not have
    // hasbits. I don't understand the rationale for the last part of the
    // condition, but it matches the old logic.
    const bool have_outer_if = HasBitIndex(chunk.front()) != kNoHasbit &&
                               chunk.size() > 1 &&
                               (memset_end != chunk.back() || merge_zero_init);

    if (have_outer_if) {
      // Emit an if() that will let us skip the whole chunk if none are set.
      uint32_t chunk_mask = GenChunkMask(chunk, has_bit_indices_);
      std::string chunk_mask_str =
          absl::StrCat(absl::Hex(chunk_mask, absl::kZeroPad8));

      // Check (up to) 8 has_bits at a time if we have more than one field in
      // this chunk.  Due to field layout ordering, we may check
      // _has_bits_[last_chunk * 8 / 32] multiple times.
      ABSL_DCHECK_LE(2, popcnt(chunk_mask));
      ABSL_DCHECK_GE(8, popcnt(chunk_mask));

      if (cached_has_word_index != HasWordIndex(chunk.front())) {
        cached_has_word_index = HasWordIndex(chunk.front());
        format("cached_has_bits = $has_bits$[$1$];\n", cached_has_word_index);
      }
      format("if (cached_has_bits & 0x$1$u) {\n", chunk_mask_str);
      format.Indent();
    }

    if (memset_start) {
      if (memset_start == memset_end) {
        // For clarity, do not memset a single field.
        field_generators_.get(memset_start).GenerateMessageClearingCode(p);
      } else {
        ABSL_CHECK_EQ(chunk_is_split, ShouldSplit(memset_start, options_));
        ABSL_CHECK_EQ(chunk_is_split, ShouldSplit(memset_end, options_));
        format(
            "::memset(&$1$, 0, static_cast<::size_t>(\n"
            "    reinterpret_cast<char*>(&$2$) -\n"
            "    reinterpret_cast<char*>(&$1$)) + sizeof($2$));\n",
            FieldMemberName(memset_start, chunk_is_split),
            FieldMemberName(memset_end, chunk_is_split));
      }
    }

    // Clear all non-zero-initializable fields in the chunk.
    for (const auto& field : chunk) {
      if (CanClearByZeroing(field)) continue;
      // It's faster to just overwrite primitive types, but we should only
      // clear strings and messages if they were set.
      //
      // TODO(kenton):  Let the CppFieldGenerator decide this somehow.
      bool have_enclosing_if =
          HasBitIndex(field) != kNoHasbit &&
          (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ||
           field->cpp_type() == FieldDescriptor::CPPTYPE_STRING);

      if (have_enclosing_if) {
        PrintPresenceCheck(field, has_bit_indices_, p, &cached_has_word_index);
      }

      field_generators_.get(field).GenerateMessageClearingCode(p);

      if (have_enclosing_if) {
        format.Outdent();
        format("}\n");
      }
    }

    if (have_outer_if) {
      format.Outdent();
      format("}\n");
    }

    if (chunk_index == chunks.size() - 1) {
      if (first_split_chunk_processed) {
        format.Outdent();
        format("}\n");
      }
    }

    if (cold_skipper.OnEndChunk(chunk_index, p)) {
      // Reset here as it may have been updated in just closed if statement.
      cached_has_word_index = -1;
    }
  }

  // Step 4: Unions.
  for (auto oneof : OneOfRange(descriptor_)) {
    format("clear_$1$();\n", oneof->name());
  }

  if (num_weak_fields_) {
    format("$weak_field_map$.ClearAll();\n");
  }

  // We don't clear donated status.

  if (!has_bit_indices_.empty()) {
    // Step 5: Everything else.
    format("$has_bits$.Clear();\n");
  }

  format("_internal_metadata_.Clear<$unknown_fields_type$>();\n");

  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateOneofClear(io::Printer* p) {
  // Generated function clears the active field and union case (e.g. foo_case_).
  int i = 0;
  for (auto oneof : OneOfRange(descriptor_)) {
    Formatter format(p);
    auto v = p->WithVars({{"oneofname", oneof->name()}});

    format(
        "void $classname$::clear_$oneofname$() {\n"
        "// @@protoc_insertion_point(one_of_clear_start:$full_name$)\n");
    format.Indent();
    format("switch ($oneofname$_case()) {\n");
    format.Indent();
    for (auto field : FieldRange(oneof)) {
      format("case k$1$: {\n", UnderscoresToCamelCase(field->name(), true));
      format.Indent();
      // We clear only allocated objects in oneofs
      if (!IsStringOrMessage(field)) {
        format("// No need to clear\n");
      } else {
        field_generators_.get(field).GenerateClearingCode(p);
      }
      format("break;\n");
      format.Outdent();
      format("}\n");
    }
    format(
        "case $1$_NOT_SET: {\n"
        "  break;\n"
        "}\n",
        absl::AsciiStrToUpper(oneof->name()));
    format.Outdent();
    format(
        "}\n"
        "$oneof_case$[$1$] = $2$_NOT_SET;\n",
        i, absl::AsciiStrToUpper(oneof->name()));
    format.Outdent();
    format(
        "}\n"
        "\n");
    i++;
  }
}

void MessageGenerator::GenerateSwap(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  Formatter format(p);

  format("void $classname$::InternalSwap($classname$* other) {\n");
  format.Indent();
  format("using std::swap;\n");

  if (HasGeneratedMethods(descriptor_->file(), options_)) {
    if (descriptor_->extension_range_count() > 0) {
      format(
          "$extensions$.InternalSwap(&other->$extensions$);"
          "\n");
    }

    if (HasNonSplitOptionalString(descriptor_, options_)) {
      format(
          "auto* lhs_arena = GetArenaForAllocation();\n"
          "auto* rhs_arena = other->GetArenaForAllocation();\n");
    }
    format("_internal_metadata_.InternalSwap(&other->_internal_metadata_);\n");

    if (!has_bit_indices_.empty()) {
      for (int i = 0; i < HasBitsSize(); ++i) {
        format("swap($has_bits$[$1$], other->$has_bits$[$1$]);\n", i);
      }
    }

    // If possible, we swap several fields at once, including padding.
    const RunMap runs =
        FindRuns(optimized_order_, [this](const FieldDescriptor* field) {
          return !ShouldSplit(field, options_) &&
                 HasTrivialSwap(field, options_, scc_analyzer_);
        });

    for (size_t i = 0; i < optimized_order_.size(); ++i) {
      const FieldDescriptor* field = optimized_order_[i];
      if (ShouldSplit(field, options_)) {
        continue;
      }
      const auto it = runs.find(field);

      // We only apply the memswap technique to runs of more than one field, as
      // `swap(field_, other.field_)` is better than
      // `memswap<...>(&field_, &other.field_)` for generated code readability.
      if (it != runs.end() && it->second > 1) {
        // Use a memswap, then skip run_length fields.
        const size_t run_length = it->second;
        const std::string first_field_name =
            FieldMemberName(field, /*cold=*/false);
        const std::string last_field_name = FieldMemberName(
            optimized_order_[i + run_length - 1], /*cold=*/false);

        auto v = p->WithVars({
            {"first", first_field_name},
            {"last", last_field_name},
        });

        format(
            "::PROTOBUF_NAMESPACE_ID::internal::memswap<\n"
            "    PROTOBUF_FIELD_OFFSET($classname$, $last$)\n"
            "    + sizeof($classname$::$last$)\n"
            "    - PROTOBUF_FIELD_OFFSET($classname$, $first$)>(\n"
            "        reinterpret_cast<char*>(&$first$),\n"
            "        reinterpret_cast<char*>(&other->$first$));\n");

        i += run_length - 1;
        // ++i at the top of the loop.
      } else {
        field_generators_.get(field).GenerateSwappingCode(p);
      }
    }
    if (ShouldSplit(descriptor_, options_)) {
      format("swap($split$, other->$split$);\n");
    }

    for (auto oneof : OneOfRange(descriptor_)) {
      format("swap(_impl_.$1$_, other->_impl_.$1$_);\n", oneof->name());
    }

    for (int i = 0; i < descriptor_->real_oneof_decl_count(); i++) {
      format("swap($oneof_case$[$1$], other->$oneof_case$[$1$]);\n", i);
    }

    if (num_weak_fields_) {
      format(
          "$weak_field_map$.UnsafeArenaSwap(&other->$weak_field_map$)"
          ";\n");
    }

    if (!inlined_string_indices_.empty()) {
      for (size_t i = 0; i < InlinedStringDonatedSize(); ++i) {
        format(
            "swap($inlined_string_donated_array$[$1$], "
            "other->$inlined_string_donated_array$[$1$]);\n",
            i);
      }
    }
  } else {
    format("GetReflection()->Swap(this, other);");
  }

  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateMergeFrom(io::Printer* p) {
  Formatter format(p);
  if (!HasSimpleBaseClass(descriptor_, options_)) {
    if (HasDescriptorMethods(descriptor_->file(), options_)) {
      // We don't override the generalized MergeFrom (aka that which
      // takes in the Message base class as a parameter); instead we just
      // let the base Message::MergeFrom take care of it.  The base MergeFrom
      // knows how to quickly confirm the types exactly match, and if so, will
      // use GetClassData() to retrieve the address of MergeImpl, which calls
      // the fast MergeFrom overload.  Most callers avoid all this by passing
      // a "from" message that is the same type as the message being merged
      // into, rather than a generic Message.

      format(
          "const ::$proto_ns$::Message::ClassData "
          "$classname$::_class_data_ = {\n"
          "    ::$proto_ns$::Message::CopyWithSourceCheck,\n"
          "    $classname$::MergeImpl\n"
          "};\n"
          "const ::$proto_ns$::Message::ClassData*"
          "$classname$::GetClassData() const { return &_class_data_; }\n"
          "\n");
    } else {
      // Generate CheckTypeAndMergeFrom().
      format(
          "void $classname$::CheckTypeAndMergeFrom(\n"
          "    const ::$proto_ns$::MessageLite& from) {\n"
          "  MergeFrom(*::_pbi::DownCast<const $classname$*>(\n"
          "      &from));\n"
          "}\n");
    }
  } else {
    // In the simple case, we just define ClassData that vectors back to the
    // simple implementation of Copy and Merge.
    format(
        "const ::$proto_ns$::Message::ClassData "
        "$classname$::_class_data_ = {\n"
        "    $superclass$::CopyImpl,\n"
        "    $superclass$::MergeImpl,\n"
        "};\n"
        "const ::$proto_ns$::Message::ClassData*"
        "$classname$::GetClassData() const { return &_class_data_; }\n"
        "\n"
        "\n");
  }
}

void MessageGenerator::GenerateClassSpecificMergeImpl(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  // Generate the class-specific MergeFrom, which avoids the ABSL_CHECK and
  // cast.
  Formatter format(p);
  if (!HasDescriptorMethods(descriptor_->file(), options_)) {
    // For messages that don't inherit from Message, just implement MergeFrom
    // directly.
    format(
        "void $classname$::MergeFrom(const $classname$& from) {\n"
        "  $classname$* const _this = this;\n");
  } else {
    format(
        "void $classname$::MergeImpl(::$proto_ns$::Message& to_msg, const "
        "::$proto_ns$::Message& from_msg) {\n"
        "  auto* const _this = static_cast<$classname$*>(&to_msg);\n"
        "  auto& from = static_cast<const $classname$&>(from_msg);\n");
  }
  format.Indent();
  format(
      "$annotate_mergefrom$"
      "// @@protoc_insertion_point(class_specific_merge_from_start:"
      "$full_name$)\n");
  format("$DCHK$_NE(&from, _this);\n");

  format(
      "$uint32$ cached_has_bits = 0;\n"
      "(void) cached_has_bits;\n\n");

  if (ShouldSplit(descriptor_, options_)) {
    format(
        "if (!from.IsSplitMessageDefault()) {\n"
        "  _this->PrepareSplitMessageForWrite();\n"
        "}\n");
  }

  std::vector<std::vector<const FieldDescriptor*>> chunks = CollectFields(
      optimized_order_,
      [&](const FieldDescriptor* a, const FieldDescriptor* b) -> bool {
        return HasByteIndex(a) == HasByteIndex(b) &&
               ShouldSplit(a, options_) == ShouldSplit(b, options_);
      });

  ColdChunkSkipper cold_skipper(descriptor_, options_, chunks, has_bit_indices_,
                                kColdRatio);

  // cached_has_word_index maintains that:
  //   cached_has_bits = from._has_bits_[cached_has_word_index]
  // for cached_has_word_index >= 0
  int cached_has_word_index = -1;

  for (int chunk_index = 0; chunk_index < chunks.size(); chunk_index++) {
    const std::vector<const FieldDescriptor*>& chunk = chunks[chunk_index];
    bool have_outer_if =
        chunk.size() > 1 && HasByteIndex(chunk.front()) != kNoHasbit;
    cold_skipper.OnStartChunk(chunk_index, cached_has_word_index, "from.", p);

    if (have_outer_if) {
      // Emit an if() that will let us skip the whole chunk if none are set.
      uint32_t chunk_mask = GenChunkMask(chunk, has_bit_indices_);
      std::string chunk_mask_str =
          absl::StrCat(absl::Hex(chunk_mask, absl::kZeroPad8));

      // Check (up to) 8 has_bits at a time if we have more than one field in
      // this chunk.  Due to field layout ordering, we may check
      // _has_bits_[last_chunk * 8 / 32] multiple times.
      ABSL_DCHECK_LE(2, popcnt(chunk_mask));
      ABSL_DCHECK_GE(8, popcnt(chunk_mask));

      if (cached_has_word_index != HasWordIndex(chunk.front())) {
        cached_has_word_index = HasWordIndex(chunk.front());
        format("cached_has_bits = from.$has_bits$[$1$];\n",
               cached_has_word_index);
      }

      format("if (cached_has_bits & 0x$1$u) {\n", chunk_mask_str);
      format.Indent();
    }

    // Go back and emit merging code for each of the fields we processed.
    bool deferred_has_bit_changes = false;
    for (const auto field : chunk) {
      const auto& generator = field_generators_.get(field);

      if (field->is_repeated()) {
        generator.GenerateMergingCode(p);
      } else if (field->is_optional() && !HasHasbit(field)) {
        // Merge semantics without true field presence: primitive fields are
        // merged only if non-zero (numeric) or non-empty (string).
        bool have_enclosing_if =
            EmitFieldNonDefaultCondition(p, "from.", field);
        generator.GenerateMergingCode(p);
        if (have_enclosing_if) {
          format.Outdent();
          format("}\n");
        }
      } else if (field->options().weak() ||
                 cached_has_word_index != HasWordIndex(field)) {
        // Check hasbit, not using cached bits.
        ABSL_CHECK(HasHasbit(field));
        auto v = p->WithVars(HasbitVars(HasBitIndex(field)));
        format(
            "if ((from.$has_bits$[$has_array_index$] & $has_mask$) != 0) {\n");
        format.Indent();
        generator.GenerateMergingCode(p);
        format.Outdent();
        format("}\n");
      } else {
        // Check hasbit, using cached bits.
        ABSL_CHECK(HasHasbit(field));
        int has_bit_index = has_bit_indices_[field->index()];
        const std::string mask = absl::StrCat(
            absl::Hex(1u << (has_bit_index % 32), absl::kZeroPad8));
        format("if (cached_has_bits & 0x$1$u) {\n", mask);
        format.Indent();

        if (have_outer_if && IsPOD(field)) {
          // Defer hasbit modification until the end of chunk.
          // This can reduce the number of loads/stores by up to 7 per 8 fields.
          deferred_has_bit_changes = true;
          generator.GenerateCopyConstructorCode(p);
        } else {
          generator.GenerateMergingCode(p);
        }

        format.Outdent();
        format("}\n");
      }
    }

    if (have_outer_if) {
      if (deferred_has_bit_changes) {
        // Flush the has bits for the primitives we deferred.
        ABSL_CHECK_LE(0, cached_has_word_index);
        format("_this->$has_bits$[$1$] |= cached_has_bits;\n",
               cached_has_word_index);
      }

      format.Outdent();
      format("}\n");
    }

    if (cold_skipper.OnEndChunk(chunk_index, p)) {
      // Reset here as it may have been updated in just closed if statement.
      cached_has_word_index = -1;
    }
  }

  // Merge oneof fields. Oneof field requires oneof case check.
  for (auto oneof : OneOfRange(descriptor_)) {
    format("switch (from.$1$_case()) {\n", oneof->name());
    format.Indent();
    for (auto field : FieldRange(oneof)) {
      format("case k$1$: {\n", UnderscoresToCamelCase(field->name(), true));
      format.Indent();
      field_generators_.get(field).GenerateMergingCode(p);
      format("break;\n");
      format.Outdent();
      format("}\n");
    }
    format(
        "case $1$_NOT_SET: {\n"
        "  break;\n"
        "}\n",
        absl::AsciiStrToUpper(oneof->name()));
    format.Outdent();
    format("}\n");
  }
  if (num_weak_fields_) {
    format(
        "_this->$weak_field_map$.MergeFrom(from.$weak_field_map$);"
        "\n");
  }

  // Merging of extensions and unknown fields is done last, to maximize
  // the opportunity for tail calls.
  if (descriptor_->extension_range_count() > 0) {
    format(
        "_this->$extensions$.MergeFrom(internal_default_instance(), "
        "from.$extensions$);\n");
  }

  format(
      "_this->_internal_metadata_.MergeFrom<$unknown_fields_type$>(from._"
      "internal_"
      "metadata_);\n");

  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateCopyFrom(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  Formatter format(p);
  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    // We don't override the generalized CopyFrom (aka that which
    // takes in the Message base class as a parameter); instead we just
    // let the base Message::CopyFrom take care of it.  The base MergeFrom
    // knows how to quickly confirm the types exactly match, and if so, will
    // use GetClassData() to get the address of Message::CopyWithSourceCheck,
    // which calls Clear() and then MergeFrom(), as well as making sure that
    // clearing the destination message doesn't alter the source, when in debug
    // builds. Most callers avoid this by passing a "from" message that is the
    // same type as the message being merged into, rather than a generic
    // Message.
  }

  // Generate the class-specific CopyFrom.
  format(
      "void $classname$::CopyFrom(const $classname$& from) {\n"
      "// @@protoc_insertion_point(class_specific_copy_from_start:"
      "$full_name$)\n");
  format.Indent();

  format("if (&from == this) return;\n");

  if (!options_.opensource_runtime && HasMessageFieldOrExtension(descriptor_)) {
    // This check is disabled in the opensource release because we're
    // concerned that many users do not define NDEBUG in their release builds.
    // It is also disabled if a message has neither message fields nor
    // extensions, as it's impossible to copy from its descendant.
    //
    // Note that IsDescendant is implemented by reflection and not available for
    // lite runtime. In that case, check if the size of the source has changed
    // after Clear.
    if (HasDescriptorMethods(descriptor_->file(), options_)) {
      format(
          "$DCHK$(!::_pbi::IsDescendant(*this, from))\n"
          "    << \"Source of CopyFrom cannot be a descendant of the "
          "target.\";\n"
          "Clear();\n");
    } else {
      format(
          "#ifndef NDEBUG\n"
          "::size_t from_size = from.ByteSizeLong();\n"
          "#endif\n"
          "Clear();\n"
          "#ifndef NDEBUG\n"
          "$CHK$_EQ(from_size, from.ByteSizeLong())\n"
          "  << \"Source of CopyFrom changed when clearing target.  Either \"\n"
          "     \"source is a nested message in target (not allowed), or \"\n"
          "     \"another thread is modifying the source.\";\n"
          "#endif\n");
    }
  } else {
    format("Clear();\n");
  }
  format("MergeFrom(from);\n");

  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateVerify(io::Printer* p) {
}

void MessageGenerator::GenerateSerializeOneofFields(
    io::Printer* p, const std::vector<const FieldDescriptor*>& fields) {
  Formatter format(p);
  ABSL_CHECK(!fields.empty());
  if (fields.size() == 1) {
    GenerateSerializeOneField(p, fields[0], -1);
    return;
  }
  // We have multiple mutually exclusive choices.  Emit a switch statement.
  const OneofDescriptor* oneof = fields[0]->containing_oneof();
  format("switch ($1$_case()) {\n", oneof->name());
  format.Indent();
  for (auto field : fields) {
    format("case k$1$: {\n", UnderscoresToCamelCase(field->name(), true));
    format.Indent();
    field_generators_.get(field).GenerateSerializeWithCachedSizesToArray(p);
    format("break;\n");
    format.Outdent();
    format("}\n");
  }
  format.Outdent();
  // Doing nothing is an option.
  format(
      "  default: ;\n"
      "}\n");
}

void MessageGenerator::GenerateSerializeOneField(io::Printer* p,
                                                 const FieldDescriptor* field,
                                                 int cached_has_bits_index) {
  auto v = p->WithVars(FieldVars(field, options_));
  Formatter format(p);
  if (!field->options().weak()) {
    // For weakfields, PrintFieldComment is called during iteration.
    PrintFieldComment(format, field);
  }

  bool have_enclosing_if = false;
  if (field->options().weak()) {
  } else if (HasHasbit(field)) {
    // Attempt to use the state of cached_has_bits, if possible.
    int has_bit_index = HasBitIndex(field);
    auto v = p->WithVars(HasbitVars(has_bit_index));
    if (cached_has_bits_index == has_bit_index / 32) {
      format("if (cached_has_bits & $has_mask$) {\n");
    } else {
      field_generators_.get(field).GenerateIfHasField(p);
    }

    format.Indent();
    have_enclosing_if = true;
  } else if (field->is_optional() && !HasHasbit(field)) {
    have_enclosing_if = EmitFieldNonDefaultCondition(p, "this->", field);
  }

  field_generators_.get(field).GenerateSerializeWithCachedSizesToArray(p);

  if (have_enclosing_if) {
    format.Outdent();
    format("}\n");
  }
  format("\n");
}

void MessageGenerator::GenerateSerializeOneExtensionRange(
    io::Printer* p, const Descriptor::ExtensionRange* range) {
  absl::flat_hash_map<absl::string_view, std::string> vars = variables_;
  vars["start"] = absl::StrCat(range->start);
  vars["end"] = absl::StrCat(range->end);
  Formatter format(p, vars);
  format("// Extension range [$start$, $end$)\n");
  format(
      "target = $extensions$._InternalSerialize(\n"
      "internal_default_instance(), $start$, $end$, target, stream);\n\n");
}

void MessageGenerator::GenerateSerializeWithCachedSizesToArray(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  Formatter format(p);
  if (descriptor_->options().message_set_wire_format()) {
    // Special-case MessageSet.
    format(
        "$uint8$* $classname$::_InternalSerialize(\n"
        "    $uint8$* target, ::$proto_ns$::io::EpsCopyOutputStream* stream) "
        "const {\n"
        "$annotate_serialize$"
        "  target = $extensions$."
        "InternalSerializeMessageSetWithCachedSizesToArray(\n"  //
        "internal_default_instance(), target, stream);\n");

    format(
        "  target = ::_pbi::"
        "InternalSerializeUnknownMessageSetItemsToArray(\n"
        "               $unknown_fields$, target, stream);\n");
    format(
        "  return target;\n"
        "}\n");
    return;
  }

  format(
      "$uint8$* $classname$::_InternalSerialize(\n"
      "    $uint8$* target, ::$proto_ns$::io::EpsCopyOutputStream* stream) "
      "const {\n"
      "$annotate_serialize$");
  format.Indent();

  format("// @@protoc_insertion_point(serialize_to_array_start:$full_name$)\n");

  if (!ShouldSerializeInOrder(descriptor_, options_)) {
    format.Outdent();
    format("#ifdef NDEBUG\n");
    format.Indent();
  }

  GenerateSerializeWithCachedSizesBody(p);

  if (!ShouldSerializeInOrder(descriptor_, options_)) {
    format.Outdent();
    format("#else  // NDEBUG\n");
    format.Indent();

    GenerateSerializeWithCachedSizesBodyShuffled(p);

    format.Outdent();
    format("#endif  // !NDEBUG\n");
    format.Indent();
  }

  format("// @@protoc_insertion_point(serialize_to_array_end:$full_name$)\n");

  format.Outdent();
  format(
      "  return target;\n"
      "}\n");
}

void MessageGenerator::GenerateSerializeWithCachedSizesBody(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  Formatter format(p);
  // If there are multiple fields in a row from the same oneof then we
  // coalesce them and emit a switch statement.  This is more efficient
  // because it lets the C++ compiler know this is a "at most one can happen"
  // situation. If we emitted "if (has_x()) ...; if (has_y()) ..." the C++
  // compiler's emitted code might check has_y() even when has_x() is true.
  class LazySerializerEmitter {
   public:
    LazySerializerEmitter(MessageGenerator* mg, io::Printer* p)
        : mg_(mg),
          p_(p),
          cached_has_bit_index_(kNoHasbit) {}

    ~LazySerializerEmitter() { Flush(); }

    // If conditions allow, try to accumulate a run of fields from the same
    // oneof, and handle them at the next Flush().
    void Emit(const FieldDescriptor* field) {
      Formatter format(p_);

      if (!field->has_presence() || MustFlush(field)) {
        Flush();
      }
      if (!field->real_containing_oneof()) {
        // TODO(ckennelly): Defer non-oneof fields similarly to oneof fields.

        if (HasHasbit(field) && field->has_presence()) {
          // We speculatively load the entire _has_bits_[index] contents, even
          // if it is for only one field.  Deferring non-oneof emitting would
          // allow us to determine whether this is going to be useful.
          int has_bit_index = mg_->has_bit_indices_[field->index()];
          if (cached_has_bit_index_ != has_bit_index / 32) {
            // Reload.
            int new_index = has_bit_index / 32;

            format("cached_has_bits = _impl_._has_bits_[$1$];\n", new_index);

            cached_has_bit_index_ = new_index;
          }
        }

        mg_->GenerateSerializeOneField(p_, field, cached_has_bit_index_);
      } else {
        v_.push_back(field);
      }
    }

    void EmitIfNotNull(const FieldDescriptor* field) {
      if (field != nullptr) {
        Emit(field);
      }
    }

    void Flush() {
      if (!v_.empty()) {
        mg_->GenerateSerializeOneofFields(p_, v_);
        v_.clear();
      }
    }

   private:
    // If we have multiple fields in v_ then they all must be from the same
    // oneof.  Would adding field to v_ break that invariant?
    bool MustFlush(const FieldDescriptor* field) {
      return !v_.empty() &&
             v_[0]->containing_oneof() != field->containing_oneof();
    }

    MessageGenerator* mg_;
    io::Printer* p_;
    std::vector<const FieldDescriptor*> v_;

    // cached_has_bit_index_ maintains that:
    //   cached_has_bits = from._has_bits_[cached_has_bit_index_]
    // for cached_has_bit_index_ >= 0
    int cached_has_bit_index_;
  };

  class LazyExtensionRangeEmitter {
   public:
    LazyExtensionRangeEmitter(MessageGenerator* mg, io::Printer* p)
        : mg_(mg), p_(p) {}

    void AddToRange(const Descriptor::ExtensionRange* range) {
      if (!has_current_range_) {
        current_combined_range_ = *range;
        has_current_range_ = true;
      } else {
        current_combined_range_.start =
            std::min(current_combined_range_.start, range->start);
        current_combined_range_.end =
            std::max(current_combined_range_.end, range->end);
      }
    }

    void Flush() {
      if (has_current_range_) {
        mg_->GenerateSerializeOneExtensionRange(p_, &current_combined_range_);
      }
      has_current_range_ = false;
    }

   private:
    MessageGenerator* mg_;
    io::Printer* p_;
    bool has_current_range_ = false;
    Descriptor::ExtensionRange current_combined_range_;
  };

  // We need to track the largest weak field, because weak fields are serialized
  // differently than normal fields.  The WeakFieldMap::FieldWriter will
  // serialize all weak fields that are ordinally between the last serialized
  // weak field and the current field.  In order to guarantee that all weak
  // fields are serialized, we need to make sure to emit the code to serialize
  // the largest weak field present at some point.
  class LargestWeakFieldHolder {
   public:
    const FieldDescriptor* Release() {
      const FieldDescriptor* result = field_;
      field_ = nullptr;
      return result;
    }
    void ReplaceIfLarger(const FieldDescriptor* field) {
      if (field_ == nullptr || field_->number() < field->number()) {
        field_ = field;
      }
    }

   private:
    const FieldDescriptor* field_ = nullptr;
  };

  std::vector<const FieldDescriptor*> ordered_fields =
      SortFieldsByNumber(descriptor_);

  std::vector<const Descriptor::ExtensionRange*> sorted_extensions;
  sorted_extensions.reserve(descriptor_->extension_range_count());
  for (int i = 0; i < descriptor_->extension_range_count(); ++i) {
    sorted_extensions.push_back(descriptor_->extension_range(i));
  }
  std::sort(sorted_extensions.begin(), sorted_extensions.end(),
            ExtensionRangeSorter());
  if (num_weak_fields_) {
    format(
        "::_pbi::WeakFieldMap::FieldWriter field_writer("
        "$weak_field_map$);\n");
  }

  format(
      "$uint32$ cached_has_bits = 0;\n"
      "(void) cached_has_bits;\n\n");

  // Merge the fields and the extension ranges, both sorted by field number.
  {
    LazySerializerEmitter e(this, p);
    LazyExtensionRangeEmitter re(this, p);
    LargestWeakFieldHolder largest_weak_field;
    int i, j;
    for (i = 0, j = 0;
         i < ordered_fields.size() || j < sorted_extensions.size();) {
      if ((j == sorted_extensions.size()) ||
          (i < descriptor_->field_count() &&
           ordered_fields[i]->number() < sorted_extensions[j]->start)) {
        const FieldDescriptor* field = ordered_fields[i++];
        re.Flush();
        if (field->options().weak()) {
          largest_weak_field.ReplaceIfLarger(field);
          PrintFieldComment(format, field);
        } else {
          e.EmitIfNotNull(largest_weak_field.Release());
          e.Emit(field);
        }
      } else {
        e.EmitIfNotNull(largest_weak_field.Release());
        e.Flush();
        re.AddToRange(sorted_extensions[j++]);
      }
    }
    re.Flush();
    e.EmitIfNotNull(largest_weak_field.Release());
  }

  format("if (PROTOBUF_PREDICT_FALSE($have_unknown_fields$)) {\n");
  format.Indent();
  if (UseUnknownFieldSet(descriptor_->file(), options_)) {
    format(
        "target = "
        "::_pbi::WireFormat::"
        "InternalSerializeUnknownFieldsToArray(\n"
        "    $unknown_fields$, target, stream);\n");
  } else {
    format(
        "target = stream->WriteRaw($unknown_fields$.data(),\n"
        "    static_cast<int>($unknown_fields$.size()), target);\n");
  }
  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateSerializeWithCachedSizesBodyShuffled(
    io::Printer* p) {
  Formatter format(p);

  std::vector<const FieldDescriptor*> ordered_fields =
      SortFieldsByNumber(descriptor_);
  ordered_fields.erase(
      std::remove_if(ordered_fields.begin(), ordered_fields.end(),
                     [this](const FieldDescriptor* f) {
                       return !IsFieldUsed(f, options_);
                     }),
      ordered_fields.end());

  std::vector<const Descriptor::ExtensionRange*> sorted_extensions;
  sorted_extensions.reserve(descriptor_->extension_range_count());
  for (int i = 0; i < descriptor_->extension_range_count(); ++i) {
    sorted_extensions.push_back(descriptor_->extension_range(i));
  }
  std::sort(sorted_extensions.begin(), sorted_extensions.end(),
            ExtensionRangeSorter());

  int num_fields = ordered_fields.size() + sorted_extensions.size();
  constexpr int kLargePrime = 1000003;
  ABSL_CHECK_LT(num_fields, kLargePrime)
      << "Prime offset must be greater than the number of fields to ensure "
         "those are coprime.";

  if (num_weak_fields_) {
    format(
        "::_pbi::WeakFieldMap::FieldWriter field_writer("
        "$weak_field_map$);\n");
  }

  format("for (int i = $1$; i >= 0; i-- ) {\n", num_fields - 1);

  format.Indent();
  format("switch(i) {\n");
  format.Indent();

  int index = 0;
  for (const auto* f : ordered_fields) {
    format("case $1$: {\n", index++);
    format.Indent();

    GenerateSerializeOneField(p, f, -1);

    format("break;\n");
    format.Outdent();
    format("}\n");
  }

  for (const auto* r : sorted_extensions) {
    format("case $1$: {\n", index++);
    format.Indent();

    GenerateSerializeOneExtensionRange(p, r);

    format("break;\n");
    format.Outdent();
    format("}\n");
  }

  format(
      "default: {\n"
      "  $DCHK$(false) << \"Unexpected index: \" << i;\n"
      "}\n");
  format.Outdent();
  format("}\n");

  format.Outdent();
  format("}\n");

  format("if (PROTOBUF_PREDICT_FALSE($have_unknown_fields$)) {\n");
  format.Indent();
  if (UseUnknownFieldSet(descriptor_->file(), options_)) {
    format(
        "target = "
        "::_pbi::WireFormat::"
        "InternalSerializeUnknownFieldsToArray(\n"
        "    $unknown_fields$, target, stream);\n");
  } else {
    format(
        "target = stream->WriteRaw($unknown_fields$.data(),\n"
        "    static_cast<int>($unknown_fields$.size()), target);\n");
  }
  format.Outdent();
  format("}\n");
}

std::vector<uint32_t> MessageGenerator::RequiredFieldsBitMask() const {
  const int array_size = HasBitsSize();
  std::vector<uint32_t> masks(array_size, 0);

  for (auto field : FieldRange(descriptor_)) {
    if (!field->is_required()) {
      continue;
    }

    const int has_bit_index = has_bit_indices_[field->index()];
    masks[has_bit_index / 32] |= static_cast<uint32_t>(1)
                                 << (has_bit_index % 32);
  }
  return masks;
}

void MessageGenerator::GenerateByteSize(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  Formatter format(p);

  if (descriptor_->options().message_set_wire_format()) {
    // Special-case MessageSet.
    format(
        "::size_t $classname$::ByteSizeLong() const {\n"
        "$annotate_bytesize$"
        "// @@protoc_insertion_point(message_set_byte_size_start:$full_name$)\n"
        "  ::size_t total_size = $extensions$.MessageSetByteSize();\n"
        "  if ($have_unknown_fields$) {\n"
        "    total_size += ::_pbi::\n"
        "        ComputeUnknownMessageSetItemsSize($unknown_fields$);\n"
        "  }\n"
        "  int cached_size = "
        "::_pbi::ToCachedSize(total_size);\n"
        "  SetCachedSize(cached_size);\n"
        "  return total_size;\n"
        "}\n");
    return;
  }

  if (num_required_fields_ > 1) {
    // Emit a function (rarely used, we hope) that handles the required fields
    // by checking for each one individually.
    format(
        "::size_t $classname$::RequiredFieldsByteSizeFallback() const {\n"
        "// @@protoc_insertion_point(required_fields_byte_size_fallback_start:"
        "$full_name$)\n");
    format.Indent();
    format("::size_t total_size = 0;\n");
    for (auto field : optimized_order_) {
      if (field->is_required()) {
        format("\n");
        field_generators_.get(field).GenerateIfHasField(p);
        format.Indent();
        PrintFieldComment(format, field);
        field_generators_.get(field).GenerateByteSize(p);
        format.Outdent();
        format("}\n");
      }
    }
    format(
        "\n"
        "return total_size;\n");
    format.Outdent();
    format("}\n");
  }

  format(
      "::size_t $classname$::ByteSizeLong() const {\n"
      "$annotate_bytesize$"
      "// @@protoc_insertion_point(message_byte_size_start:$full_name$)\n");
  format.Indent();
  format(
      "::size_t total_size = 0;\n"
      "\n");

  if (descriptor_->extension_range_count() > 0) {
    format(
        "total_size += $extensions$.ByteSize();\n"
        "\n");
  }

  // Handle required fields (if any).  We expect all of them to be
  // present, so emit one conditional that checks for that.  If they are all
  // present then the fast path executes; otherwise the slow path executes.
  if (num_required_fields_ > 1) {
    // The fast path works if all required fields are present.
    const std::vector<uint32_t> masks_for_has_bits = RequiredFieldsBitMask();
    format("if ($1$) {  // All required fields are present.\n",
           ConditionalToCheckBitmasks(masks_for_has_bits));
    format.Indent();
    // Oneof fields cannot be required, so optimized_order_ contains all of the
    // fields that we need to potentially emit.
    for (auto field : optimized_order_) {
      if (!field->is_required()) continue;
      PrintFieldComment(format, field);
      field_generators_.get(field).GenerateByteSize(p);
      format("\n");
    }
    format.Outdent();
    format(
        "} else {\n"  // the slow path
        "  total_size += RequiredFieldsByteSizeFallback();\n"
        "}\n");
  } else {
    // num_required_fields_ <= 1: no need to be tricky
    for (auto field : optimized_order_) {
      if (!field->is_required()) continue;
      PrintFieldComment(format, field);
      field_generators_.get(field).GenerateIfHasField(p);
      format.Indent();
      field_generators_.get(field).GenerateByteSize(p);
      format.Outdent();
      format("}\n");
    }
  }

  std::vector<std::vector<const FieldDescriptor*>> chunks = CollectFields(
      optimized_order_,
      [&](const FieldDescriptor* a, const FieldDescriptor* b) -> bool {
        return a->label() == b->label() && HasByteIndex(a) == HasByteIndex(b) &&
               ShouldSplit(a, options_) == ShouldSplit(b, options_);
      });

  // Remove chunks with required fields.
  chunks.erase(std::remove_if(chunks.begin(), chunks.end(), IsRequired),
               chunks.end());

  ColdChunkSkipper cold_skipper(descriptor_, options_, chunks, has_bit_indices_,
                                kColdRatio);
  int cached_has_word_index = -1;

  format(
      "$uint32$ cached_has_bits = 0;\n"
      "// Prevent compiler warnings about cached_has_bits being unused\n"
      "(void) cached_has_bits;\n\n");

  for (int chunk_index = 0; chunk_index < chunks.size(); chunk_index++) {
    const std::vector<const FieldDescriptor*>& chunk = chunks[chunk_index];
    const bool have_outer_if =
        chunk.size() > 1 && HasWordIndex(chunk[0]) != kNoHasbit;
    cold_skipper.OnStartChunk(chunk_index, cached_has_word_index, "", p);

    if (have_outer_if) {
      // Emit an if() that will let us skip the whole chunk if none are set.
      uint32_t chunk_mask = GenChunkMask(chunk, has_bit_indices_);
      std::string chunk_mask_str =
          absl::StrCat(absl::Hex(chunk_mask, absl::kZeroPad8));

      // Check (up to) 8 has_bits at a time if we have more than one field in
      // this chunk.  Due to field layout ordering, we may check
      // _has_bits_[last_chunk * 8 / 32] multiple times.
      ABSL_DCHECK_LE(2, popcnt(chunk_mask));
      ABSL_DCHECK_GE(8, popcnt(chunk_mask));

      if (cached_has_word_index != HasWordIndex(chunk.front())) {
        cached_has_word_index = HasWordIndex(chunk.front());
        format("cached_has_bits = $has_bits$[$1$];\n", cached_has_word_index);
      }
      format("if (cached_has_bits & 0x$1$u) {\n", chunk_mask_str);
      format.Indent();
    }

    // Go back and emit checks for each of the fields we processed.
    for (int j = 0; j < chunk.size(); j++) {
      const FieldDescriptor* field = chunk[j];
      bool have_enclosing_if = false;
      bool need_extra_newline = false;

      PrintFieldComment(format, field);

      if (field->is_repeated()) {
        // No presence check is required.
        need_extra_newline = true;
      } else if (HasHasbit(field)) {
        PrintPresenceCheck(field, has_bit_indices_, p, &cached_has_word_index);
        have_enclosing_if = true;
      } else {
        // Without field presence: field is serialized only if it has a
        // non-default value.
        have_enclosing_if = EmitFieldNonDefaultCondition(p, "this->", field);
      }

      field_generators_.get(field).GenerateByteSize(p);

      if (have_enclosing_if) {
        format.Outdent();
        format(
            "}\n"
            "\n");
      }
      if (need_extra_newline) {
        format("\n");
      }
    }

    if (have_outer_if) {
      format.Outdent();
      format("}\n");
    }

    if (cold_skipper.OnEndChunk(chunk_index, p)) {
      // Reset here as it may have been updated in just closed if statement.
      cached_has_word_index = -1;
    }
  }

  // Fields inside a oneof don't use _has_bits_ so we count them in a separate
  // pass.
  for (auto oneof : OneOfRange(descriptor_)) {
    format("switch ($1$_case()) {\n", oneof->name());
    format.Indent();
    for (auto field : FieldRange(oneof)) {
      PrintFieldComment(format, field);
      format("case k$1$: {\n", UnderscoresToCamelCase(field->name(), true));
      format.Indent();
      field_generators_.get(field).GenerateByteSize(p);
      format("break;\n");
      format.Outdent();
      format("}\n");
    }
    format(
        "case $1$_NOT_SET: {\n"
        "  break;\n"
        "}\n",
        absl::AsciiStrToUpper(oneof->name()));
    format.Outdent();
    format("}\n");
  }

  if (num_weak_fields_) {
    // TagSize + MessageSize
    format("total_size += $weak_field_map$.ByteSizeLong();\n");
  }

  if (UseUnknownFieldSet(descriptor_->file(), options_)) {
    // We go out of our way to put the computation of the uncommon path of
    // unknown fields in tail position. This allows for better code generation
    // of this function for simple protos.
    format(
        "return MaybeComputeUnknownFieldsSize(total_size, &$cached_size$);\n");
  } else {
    format("if (PROTOBUF_PREDICT_FALSE($have_unknown_fields$)) {\n");
    format("  total_size += $unknown_fields$.size();\n");
    format("}\n");

    // We update _cached_size_ even though this is a const method.  Because
    // const methods might be called concurrently this needs to be atomic
    // operations or the program is undefined.  In practice, since any
    // concurrent writes will be writing the exact same value, normal writes
    // will work on all common processors. We use a dedicated wrapper class to
    // abstract away the underlying atomic. This makes it easier on platforms
    // where even relaxed memory order might have perf impact to replace it with
    // ordinary loads and stores.
    format(
        "int cached_size = ::_pbi::ToCachedSize(total_size);\n"
        "SetCachedSize(cached_size);\n"
        "return total_size;\n");
  }

  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateIsInitialized(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  Formatter format(p);
  format("bool $classname$::IsInitialized() const {\n");
  format.Indent();

  if (descriptor_->extension_range_count() > 0) {
    format(
        "if (!$extensions$.IsInitialized(internal_default_instance())) {\n"
        "  return false;\n"
        "}\n\n");
  }

  if (num_required_fields_ > 0) {
    format(
        "if (_Internal::MissingRequiredFields($has_bits$))"
        " return false;\n");
  }

  // Now check that all non-oneof embedded messages are initialized.
  for (auto field : optimized_order_) {
    field_generators_.get(field).GenerateIsInitialized(p);
  }
  if (num_weak_fields_) {
    // For Weak fields.
    format("if (!$weak_field_map$.IsInitialized()) return false;\n");
  }
  // Go through the oneof fields, emitting a switch if any might have required
  // fields.
  for (auto oneof : OneOfRange(descriptor_)) {
    bool has_required_fields = false;
    for (auto field : FieldRange(oneof)) {
      if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
          !ShouldIgnoreRequiredFieldCheck(field, options_) &&
          scc_analyzer_->HasRequiredFields(field->message_type())) {
        has_required_fields = true;
        break;
      }
    }

    if (!has_required_fields) {
      continue;
    }

    format("switch ($1$_case()) {\n", oneof->name());
    format.Indent();
    for (auto field : FieldRange(oneof)) {
      format("case k$1$: {\n", UnderscoresToCamelCase(field->name(), true));
      format.Indent();
      field_generators_.get(field).GenerateIsInitialized(p);
      format("break;\n");
      format.Outdent();
      format("}\n");
    }
    format(
        "case $1$_NOT_SET: {\n"
        "  break;\n"
        "}\n",
        absl::AsciiStrToUpper(oneof->name()));
    format.Outdent();
    format("}\n");
  }

  format.Outdent();
  format(
      "  return true;\n"
      "}\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
