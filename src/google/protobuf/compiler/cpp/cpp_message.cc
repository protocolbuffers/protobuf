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

#include <google/protobuf/compiler/cpp/cpp_message.h>

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <google/protobuf/compiler/cpp/cpp_enum.h>
#include <google/protobuf/compiler/cpp/cpp_extension.h>
#include <google/protobuf/compiler/cpp/cpp_field.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/compiler/cpp/cpp_padding_optimizer.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/map_entry_lite.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>
#include <google/protobuf/stubs/hash.h>


namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

using internal::WireFormat;
using internal::WireFormatLite;

namespace {

static constexpr int kNoHasbit = -1;

// Create an expression that evaluates to
//  "for all i, (_has_bits_[i] & masks[i]) == masks[i]"
// masks is allowed to be shorter than _has_bits_, but at least one element of
// masks must be non-zero.
std::string ConditionalToCheckBitmasks(
    const std::vector<uint32>& masks, bool return_success = true,
    StringPiece has_bits_var = "_has_bits_") {
  std::vector<std::string> parts;
  for (int i = 0; i < masks.size(); i++) {
    if (masks[i] == 0) continue;
    std::string m = StrCat("0x", strings::Hex(masks[i], strings::ZERO_PAD_8));
    // Each xor evaluates to 0 if the expected bits are present.
    parts.push_back(
        StrCat("((", has_bits_var, "[", i, "] & ", m, ") ^ ", m, ")"));
  }
  GOOGLE_CHECK(!parts.empty());
  // If we have multiple parts, each expected to be 0, then bitwise-or them.
  std::string result =
      parts.size() == 1
          ? parts[0]
          : StrCat("(", Join(parts, "\n       | "), ")");
  return result + (return_success ? " == 0" : " != 0");
}

void PrintPresenceCheck(const Formatter& format, const FieldDescriptor* field,
                        const std::vector<int>& has_bit_indices,
                        io::Printer* printer, int* cached_has_word_index) {
  if (!field->options().weak()) {
    int has_bit_index = has_bit_indices[field->index()];
    if (*cached_has_word_index != (has_bit_index / 32)) {
      *cached_has_word_index = (has_bit_index / 32);
      format("cached_has_bits = _has_bits_[$1$];\n", *cached_has_word_index);
    }
    const std::string mask =
        StrCat(strings::Hex(1u << (has_bit_index % 32), strings::ZERO_PAD_8));
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

// Helper for the code that emits the SharedCtor() and InternalSwap() methods.
// Anything that is a POD or a "normal" message (represented by a pointer) can
// be manipulated as raw bytes.
bool CanBeManipulatedAsRawBytes(const FieldDescriptor* field,
                                const Options& options) {
  bool ret = CanInitializeByZeroing(field);

  // Non-repeated, non-lazy message fields are simply raw pointers, so we can
  // swap them or use memset to initialize these in SharedCtor. We cannot use
  // this in Clear, as we need to potentially delete the existing value.
  ret = ret || (!field->is_repeated() && !IsLazy(field, options) &&
                field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE);
  return ret;
}

// Finds runs of fields for which `predicate` is true.
// RunMap maps from fields that start each run to the number of fields in that
// run.  This is optimized for the common case that there are very few runs in
// a message and that most of the eligible fields appear together.
using RunMap = std::unordered_map<const FieldDescriptor*, size_t>;
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
bool EmitFieldNonDefaultCondition(io::Printer* printer,
                                  const std::string& prefix,
                                  const FieldDescriptor* field) {
  GOOGLE_CHECK(!HasHasbit(field));
  Formatter format(printer);
  format.Set("prefix", prefix);
  format.Set("name", FieldName(field));
  // Merge and serialize semantics: primitive fields are merged/serialized only
  // if non-zero (numeric) or non-empty (string).
  if (!field->is_repeated() && !field->containing_oneof()) {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
      format("if ($prefix$$name$().size() > 0) {\n");
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      // Message fields still have has_$name$() methods.
      format("if ($prefix$has_$name$()) {\n");
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_DOUBLE ||
               field->cpp_type() == FieldDescriptor::CPPTYPE_FLOAT) {
      // Handle float comparison to prevent -Wfloat-equal warnings
      format("if (!($prefix$$name$() <= 0 && $prefix$$name$() >= 0)) {\n");
    } else {
      format("if ($prefix$$name$() != 0) {\n");
    }
    format.Indent();
    return true;
  } else if (field->real_containing_oneof()) {
    format("if (_internal_has_$name$()) {\n");
    format.Indent();
    return true;
  }
  return false;
}

// Does the given field have a has_$name$() method?
bool HasHasMethod(const FieldDescriptor* field) {
  if (HasFieldPresence(field->file())) {
    // In proto1/proto2, every field has a has_$name$() method.
    return true;
  }
  // For message types without true field presence, only fields with a message
  // type have a has_$name$() method.
  return field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ||
         field->has_optional_keyword();
}

// Collects map entry message type information.
void CollectMapInfo(const Options& options, const Descriptor* descriptor,
                    std::map<std::string, std::string>* variables) {
  GOOGLE_CHECK(IsMapEntryMessage(descriptor));
  std::map<std::string, std::string>& vars = *variables;
  const FieldDescriptor* key = descriptor->FindFieldByName("key");
  const FieldDescriptor* val = descriptor->FindFieldByName("value");
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
  vars["key_wire_type"] =
      "TYPE_" + ToUpper(DeclaredTypeMethodName(key->type()));
  vars["val_wire_type"] =
      "TYPE_" + ToUpper(DeclaredTypeMethodName(val->type()));
  if (descriptor->file()->syntax() != FileDescriptor::SYNTAX_PROTO3 &&
      val->type() == FieldDescriptor::TYPE_ENUM) {
    const EnumValueDescriptor* default_value = val->default_value_enum();
    vars["default_enum_value"] = Int32ToString(default_value->number());
  } else {
    vars["default_enum_value"] = "0";
  }
}

// Does the given field have a private (internal helper only) has_$name$()
// method?
bool HasPrivateHasMethod(const FieldDescriptor* field) {
  // Only for oneofs in message types with no field presence. has_$name$(),
  // based on the oneof case, is still useful internally for generated code.
  return (!HasFieldPresence(field->file()) && field->real_containing_oneof());
}

// TODO(ckennelly):  Cull these exclusions if/when these protos do not have
// their methods overridden by subclasses.

bool ShouldMarkClassAsFinal(const Descriptor* descriptor,
                            const Options& options) {
  return true;
}

bool ShouldMarkClearAsFinal(const Descriptor* descriptor,
                            const Options& options) {
  static std::set<std::string> exclusions{
  };

  const std::string name = ClassName(descriptor, true);
  return exclusions.find(name) == exclusions.end() ||
         options.opensource_runtime;
}

bool ShouldMarkIsInitializedAsFinal(const Descriptor* descriptor,
                                    const Options& options) {
  static std::set<std::string> exclusions{
  };

  const std::string name = ClassName(descriptor, true);
  return exclusions.find(name) == exclusions.end() ||
         options.opensource_runtime;
}

bool ShouldMarkNewAsFinal(const Descriptor* descriptor,
                          const Options& options) {
  static std::set<std::string> exclusions{
  };

  const std::string name = ClassName(descriptor, true);
  return exclusions.find(name) == exclusions.end() ||
         options.opensource_runtime;
}

bool TableDrivenParsingEnabled(const Descriptor* descriptor,
                               const Options& options) {
  if (!options.table_driven_parsing) {
    return false;
  }

  // Consider table-driven parsing.  We only do this if:
  // - We have has_bits for fields.  This avoids a check on every field we set
  //   when are present (the common case).
  bool has_hasbit = false;
  for (int i = 0; i < descriptor->field_count(); i++) {
    if (HasHasbit(descriptor->field(i))) {
      has_hasbit = true;
      break;
    }
  }

  if (!has_hasbit) return false;

  const double table_sparseness = 0.5;
  int max_field_number = 0;
  for (auto field : FieldRange(descriptor)) {
    if (max_field_number < field->number()) {
      max_field_number = field->number();
    }

    // - There are no weak fields.
    if (IsWeak(field, options)) {
      return false;
    }

    // - There are no lazy fields (they require the non-lite library).
    if (IsLazy(field, options)) {
      return false;
    }
  }

  // - There range of field numbers is "small"
  if (max_field_number >= (2 << 14)) {
    return false;
  }

  // - Field numbers are relatively dense within the actual number of fields.
  //   We check for strictly greater than in the case where there are no fields
  //   (only extensions) so max_field_number == descriptor->field_count() == 0.
  if (max_field_number * table_sparseness > descriptor->field_count()) {
    return false;
  }

  // - This is not a MapEntryMessage.
  if (IsMapEntryMessage(descriptor)) {
    return false;
  }

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
uint32 GenChunkMask(const std::vector<const FieldDescriptor*>& fields,
                    const std::vector<int>& has_bit_indices) {
  GOOGLE_CHECK(!fields.empty());
  int first_index_offset = has_bit_indices[fields.front()->index()] / 32;
  uint32 chunk_mask = 0;
  for (auto field : fields) {
    // "index" defines where in the _has_bits_ the field appears.
    int index = has_bit_indices[field->index()];
    GOOGLE_CHECK_EQ(first_index_offset, index / 32);
    chunk_mask |= static_cast<uint32>(1) << (index % 32);
  }
  GOOGLE_CHECK_NE(0, chunk_mask);
  return chunk_mask;
}

// Return the number of bits set in n, a non-negative integer.
static int popcnt(uint32 n) {
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
      const Options& options,
      const std::vector<std::vector<const FieldDescriptor*>>& chunks,
      const std::vector<int>& has_bit_indices, const double cold_threshold)
      : chunks_(chunks),
        has_bit_indices_(has_bit_indices),
        access_info_map_(options.access_info_map),
        cold_threshold_(cold_threshold) {
    SetCommonVars(options, &variables_);
  }

  // May open an external if check for a batch of cold fields. "from" is the
  // prefix to _has_bits_ to allow MergeFrom to use "from._has_bits_".
  // Otherwise, it should be "".
  void OnStartChunk(int chunk, int cached_has_word_index,
                    const std::string& from, io::Printer* printer);
  bool OnEndChunk(int chunk, io::Printer* printer);

 private:
  bool IsColdChunk(int chunk);

  int HasbitWord(int chunk, int offset) {
    return has_bit_indices_[chunks_[chunk][offset]->index()] / 32;
  }

  const std::vector<std::vector<const FieldDescriptor*>>& chunks_;
  const std::vector<int>& has_bit_indices_;
  const AccessInfoMap* access_info_map_;
  const double cold_threshold_;
  std::map<std::string, std::string> variables_;
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
                                    const std::string& from,
                                    io::Printer* printer) {
  Formatter format(printer, variables_);
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
    uint32 mask = 0;
    int this_word = HasbitWord(chunk, 0);
    // Generate mask for chunks on the same word.
    for (; chunk < limit_chunk_ && HasbitWord(chunk, 0) == this_word; chunk++) {
      for (auto field : chunks_[chunk]) {
        int hasbit_index = has_bit_indices_[field->index()];
        // Fields on a chunk must be in the same word.
        GOOGLE_CHECK_EQ(this_word, hasbit_index / 32);
        mask |= 1 << (hasbit_index % 32);
      }
    }

    if (this_word != first_word) {
      format(" ||\n    ");
    }
    format.Set("mask", strings::Hex(mask, strings::ZERO_PAD_8));
    if (this_word == cached_has_word_index) {
      format("(cached_has_bits & 0x$mask$u) != 0");
    } else {
      format("($1$_has_bits_[$2$] & 0x$mask$u) != 0", from, this_word);
    }
  }
  format(")) {\n");
  format.Indent();
}

bool ColdChunkSkipper::OnEndChunk(int chunk, io::Printer* printer) {
  Formatter format(printer, variables_);
  if (chunk != limit_chunk_ - 1) {
    return false;
  }
  format.Outdent();
  format("}\n");
  return true;
}

}  // anonymous namespace

// ===================================================================

MessageGenerator::MessageGenerator(
    const Descriptor* descriptor,
    const std::map<std::string, std::string>& vars, int index_in_file_messages,
    const Options& options, MessageSCCAnalyzer* scc_analyzer)
    : descriptor_(descriptor),
      index_in_file_messages_(index_in_file_messages),
      classname_(ClassName(descriptor, false)),
      options_(options),
      field_generators_(descriptor, options, scc_analyzer),
      max_has_bit_index_(0),
      num_weak_fields_(0),
      scc_analyzer_(scc_analyzer),
      variables_(vars) {
  if (!message_layout_helper_) {
    message_layout_helper_.reset(new PaddingOptimizer());
  }

  // Variables that apply to this class
  variables_["classname"] = classname_;
  variables_["classtype"] = QualifiedClassName(descriptor_, options);
  variables_["scc_info"] =
      SccInfoSymbol(scc_analyzer_->GetSCC(descriptor_), options_);
  variables_["full_name"] = descriptor_->full_name();
  variables_["superclass"] = SuperClassName(descriptor_, options_);

  // Compute optimized field order to be used for layout and initialization
  // purposes.
  for (auto field : FieldRange(descriptor_)) {
    if (!IsFieldUsed(field, options_)) {
      continue;
    }

    if (IsWeak(field, options_)) {
      num_weak_fields_++;
    } else if (!field->real_containing_oneof()) {
      optimized_order_.push_back(field);
    }
  }

  message_layout_helper_->OptimizeLayout(&optimized_order_, options_);

  // This message has hasbits iff one or more fields need one.
  for (auto field : optimized_order_) {
    if (HasHasbit(field)) {
      if (has_bit_indices_.empty()) {
        has_bit_indices_.resize(descriptor_->field_count(), kNoHasbit);
      }
      has_bit_indices_[field->index()] = max_has_bit_index_++;
    }
  }

  if (!has_bit_indices_.empty()) {
    field_generators_.SetHasBitIndices(has_bit_indices_);
  }

  num_required_fields_ = 0;
  for (int i = 0; i < descriptor->field_count(); i++) {
    if (descriptor->field(i)->is_required()) {
      ++num_required_fields_;
    }
  }

  table_driven_ = TableDrivenParsingEnabled(descriptor_, options_);
}

MessageGenerator::~MessageGenerator() = default;

size_t MessageGenerator::HasBitsSize() const {
  size_t sizeof_has_bits = (max_has_bit_index_ + 31) / 32 * 4;
  if (sizeof_has_bits == 0) {
    // Zero-size arrays aren't technically allowed, and MSVC in particular
    // doesn't like them.  We still need to declare these arrays to make
    // other code compile.  Since this is an uncommon case, we'll just declare
    // them with size 1 and waste some space.  Oh well.
    sizeof_has_bits = 4;
  }

  return sizeof_has_bits;
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
        new EnumGenerator(descriptor_->enum_type(i), variables_, options_));
    enum_generators_.push_back(enum_generators->back().get());
  }
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    extension_generators->emplace_back(
        new ExtensionGenerator(descriptor_->extension(i), options_));
    extension_generators_.push_back(extension_generators->back().get());
  }
}

void MessageGenerator::GenerateFieldAccessorDeclarations(io::Printer* printer) {
  Formatter format(printer, variables_);
  // optimized_fields_ does not contain fields where
  //    field->real_containing_oneof()
  // so we need to iterate over those as well.
  //
  // We place the non-oneof fields in optimized_order_, as that controls the
  // order of the _has_bits_ entries and we want GDB's pretty printers to be
  // able to infer these indices from the k[FIELDNAME]FieldNumber order.
  std::vector<const FieldDescriptor*> ordered_fields;
  ordered_fields.reserve(descriptor_->field_count());

  ordered_fields.insert(ordered_fields.begin(), optimized_order_.begin(),
                        optimized_order_.end());
  for (auto field : FieldRange(descriptor_)) {
    if (!field->real_containing_oneof() && !field->options().weak() &&
        IsFieldUsed(field, options_)) {
      continue;
    }
    ordered_fields.push_back(field);
  }

  if (!ordered_fields.empty()) {
    format("enum : int {\n");
    for (auto field : ordered_fields) {
      Formatter::SaveState save(&format);

      std::map<std::string, std::string> vars;
      SetCommonFieldVariables(field, &vars, options_);
      format.AddMap(vars);
      format("  ${1$$2$$}$ = $number$,\n", field, FieldConstantName(field));
    }
    format("};\n");
  }
  for (auto field : ordered_fields) {
    PrintFieldComment(format, field);

    Formatter::SaveState save(&format);

    std::map<std::string, std::string> vars;
    SetCommonFieldVariables(field, &vars, options_);
    format.AddMap(vars);

    if (field->is_repeated()) {
      format("$deprecated_attr$int ${1$$name$_size$}$() const$2$\n", field,
             IsFieldUsed(field, options_) ? ";" : " {__builtin_trap();}");
      if (IsFieldUsed(field, options_)) {
        format(
            "private:\n"
            "int ${1$_internal_$name$_size$}$() const;\n"
            "public:\n",
            field);
      }
    } else if (HasHasMethod(field)) {
      format("$deprecated_attr$bool ${1$has_$name$$}$() const$2$\n", field,
             IsFieldUsed(field, options_) ? ";" : " {__builtin_trap();}");
      if (IsFieldUsed(field, options_)) {
        format(
            "private:\n"
            "bool _internal_has_$name$() const;\n"
            "public:\n");
      }
    } else if (HasPrivateHasMethod(field)) {
      if (IsFieldUsed(field, options_)) {
        format(
            "private:\n"
            "bool ${1$_internal_has_$name$$}$() const;\n"
            "public:\n",
            field);
      }
    }
    format("$deprecated_attr$void ${1$clear_$name$$}$()$2$\n", field,
           IsFieldUsed(field, options_) ? ";" : "{__builtin_trap();}");

    // Generate type-specific accessor declarations.
    field_generators_.get(field).GenerateAccessorDeclarations(printer);

    format("\n");
  }

  if (descriptor_->extension_range_count() > 0) {
    // Generate accessors for extensions.  We just call a macro located in
    // extension_set.h since the accessors about 80 lines of static code.
    format("$GOOGLE_PROTOBUF$_EXTENSION_ACCESSORS($classname$)\n");
    // Generate MessageSet specific APIs for proto2 MessageSet.
    // For testing purposes we don't check for bridge.MessageSet, so
    // we don't use IsProto2MessageSet
    if (descriptor_->options().message_set_wire_format() &&
        !options_.opensource_runtime && !options_.lite_implicit_weak_fields) {
      // Special-case MessageSet
      format("GOOGLE_PROTOBUF_EXTENSION_MESSAGE_SET_ACCESSORS($classname$)\n");
    }
  }

  for (auto oneof : OneOfRange(descriptor_)) {
    Formatter::SaveState saver(&format);
    format.Set("oneof_name", oneof->name());
    format.Set("camel_oneof_name", UnderscoresToCamelCase(oneof->name(), true));
    format(
        "void ${1$clear_$oneof_name$$}$();\n"
        "$camel_oneof_name$Case $oneof_name$_case() const;\n",
        oneof);
  }
}

void MessageGenerator::GenerateSingularFieldHasBits(
    const FieldDescriptor* field, Formatter format) {
  if (!IsFieldUsed(field, options_)) {
    format(
        "inline bool $classname$::has_$name$() const { "
        "__builtin_trap(); }\n");
    return;
  }
  if (field->options().weak()) {
    format(
        "inline bool $classname$::has_$name$() const {\n"
        "$annotate_accessor$"
        "  return _weak_field_map_.Has($number$);\n"
        "}\n");
    return;
  }
  if (HasHasbit(field)) {
    int has_bit_index = HasBitIndex(field);
    GOOGLE_CHECK_NE(has_bit_index, kNoHasbit);

    format.Set("has_array_index", has_bit_index / 32);
    format.Set("has_mask",
               strings::Hex(1u << (has_bit_index % 32), strings::ZERO_PAD_8));
    format(
        "inline bool $classname$::_internal_has_$name$() const {\n"
        "  bool value = "
        "(_has_bits_[$has_array_index$] & 0x$has_mask$u) != 0;\n");

    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
        !IsLazy(field, options_)) {
      // We maintain the invariant that for a submessage x, has_x() returning
      // true implies that x_ is not null. By giving this information to the
      // compiler, we allow it to eliminate unnecessary null checks later on.
      format("  PROTOBUF_ASSUME(!value || $name$_ != nullptr);\n");
    }

    format(
        "  return value;\n"
        "}\n"
        "inline bool $classname$::has_$name$() const {\n"
        "$annotate_accessor$"
        "  return _internal_has_$name$();\n"
        "}\n");
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    // Message fields have a has_$name$() method.
    if (IsLazy(field, options_)) {
      format(
          "inline bool $classname$::_internal_has_$name$() const {\n"
          "  return !$name$_.IsCleared();\n"
          "}\n");
    } else {
      format(
          "inline bool $classname$::_internal_has_$name$() const {\n"
          "  return this != internal_default_instance() "
          "&& $name$_ != nullptr;\n"
          "}\n");
    }
    format(
        "inline bool $classname$::has_$name$() const {\n"
        "$annotate_accessor$"
        "  return _internal_has_$name$();\n"
        "}\n");
  }
}

void MessageGenerator::GenerateOneofHasBits(io::Printer* printer) {
  Formatter format(printer, variables_);
  for (auto oneof : OneOfRange(descriptor_)) {
    format.Set("oneof_name", oneof->name());
    format.Set("oneof_index", oneof->index());
    format.Set("cap_oneof_name", ToUpper(oneof->name()));
    format(
        "inline bool $classname$::has_$oneof_name$() const {\n"
        "  return $oneof_name$_case() != $cap_oneof_name$_NOT_SET;\n"
        "}\n"
        "inline void $classname$::clear_has_$oneof_name$() {\n"
        "  _oneof_case_[$oneof_index$] = $cap_oneof_name$_NOT_SET;\n"
        "}\n");
  }
}

void MessageGenerator::GenerateOneofMemberHasBits(const FieldDescriptor* field,
                                                  const Formatter& format) {
  if (!IsFieldUsed(field, options_)) {
    if (HasHasMethod(field)) {
      format(
          "inline bool $classname$::has_$name$() const { "
          "__builtin_trap(); }\n");
    }
    format(
        "inline void $classname$::set_has_$name$() { __builtin_trap(); "
        "}\n");
    return;
  }
  // Singular field in a oneof
  // N.B.: Without field presence, we do not use has-bits or generate
  // has_$name$() methods, but oneofs still have set_has_$name$().
  // Oneofs also have has_$name$() but only as a private helper
  // method, so that generated code is slightly cleaner (vs.  comparing
  // _oneof_case_[index] against a constant everywhere).
  //
  // If has_$name$() is private, there is no need to add an internal accessor.
  // Only annotate public accessors.
  if (HasHasMethod(field)) {
    format(
        "inline bool $classname$::_internal_has_$name$() const {\n"
        "  return $oneof_name$_case() == k$field_name$;\n"
        "}\n"
        "inline bool $classname$::has_$name$() const {\n"
        "$annotate_accessor$"
        "  return _internal_has_$name$();\n"
        "}\n");
  } else if (HasPrivateHasMethod(field)) {
    format(
        "inline bool $classname$::_internal_has_$name$() const {\n"
        "  return $oneof_name$_case() == k$field_name$;\n"
        "}\n");
  }
  // set_has_$name$() for oneof fields is always private; hence should not be
  // annotated.
  format(
      "inline void $classname$::set_has_$name$() {\n"
      "  _oneof_case_[$oneof_index$] = k$field_name$;\n"
      "}\n");
}

void MessageGenerator::GenerateFieldClear(const FieldDescriptor* field,
                                          bool is_inline, Formatter format) {
  if (!IsFieldUsed(field, options_)) {
    format("void $classname$::clear_$name$() { __builtin_trap(); }\n");
    return;
  }

  // Generate clear_$name$().
  if (is_inline) {
    format("inline ");
  }
  format(
      "void $classname$::clear_$name$() {\n"
      "$annotate_accessor$");

  format.Indent();

  if (field->real_containing_oneof()) {
    // Clear this field only if it is the active field in this oneof,
    // otherwise ignore
    format("if (_internal_has_$name$()) {\n");
    format.Indent();
    field_generators_.get(field).GenerateClearingCode(format.printer());
    format("clear_has_$oneof_name$();\n");
    format.Outdent();
    format("}\n");
  } else {
    field_generators_.get(field).GenerateClearingCode(format.printer());
    if (HasHasbit(field)) {
      int has_bit_index = HasBitIndex(field);
      format.Set("has_array_index", has_bit_index / 32);
      format.Set("has_mask",
                 strings::Hex(1u << (has_bit_index % 32), strings::ZERO_PAD_8));
      format("_has_bits_[$has_array_index$] &= ~0x$has_mask$u;\n");
    }
  }

  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateFieldAccessorDefinitions(io::Printer* printer) {
  Formatter format(printer, variables_);
  format("// $classname$\n\n");

  for (auto field : FieldRange(descriptor_)) {
    PrintFieldComment(format, field);

    if (!IsFieldUsed(field, options_)) {
      continue;
    }

    std::map<std::string, std::string> vars;
    SetCommonFieldVariables(field, &vars, options_);

    Formatter::SaveState saver(&format);
    format.AddMap(vars);

    // Generate has_$name$() or $name$_size().
    if (field->is_repeated()) {
      if (!IsFieldUsed(field, options_)) {
        format(
            "inline int $classname$::$name$_size() const { "
            "__builtin_trap(); }\n");
      } else {
        format(
            "inline int $classname$::_internal_$name$_size() const {\n"
            "  return $name$_$1$.size();\n"
            "}\n"
            "inline int $classname$::$name$_size() const {\n"
            "$annotate_accessor$"
            "  return _internal_$name$_size();\n"
            "}\n",
            IsImplicitWeakField(field, options_, scc_analyzer_) &&
                    field->message_type()
                ? ".weak"
                : "");
      }
    } else if (field->real_containing_oneof()) {
      format.Set("field_name", UnderscoresToCamelCase(field->name(), true));
      format.Set("oneof_name", field->containing_oneof()->name());
      format.Set("oneof_index",
                 StrCat(field->containing_oneof()->index()));
      GenerateOneofMemberHasBits(field, format);
    } else {
      // Singular field.
      GenerateSingularFieldHasBits(field, format);
    }

    if (!IsCrossFileMaybeMap(field)) {
      GenerateFieldClear(field, true, format);
    }

    // Generate type-specific accessors.
    if (IsFieldUsed(field, options_)) {
      field_generators_.get(field).GenerateInlineAccessorDefinitions(printer);
    }

    format("\n");
  }

  // Generate has_$name$() and clear_has_$name$() functions for oneofs.
  GenerateOneofHasBits(printer);
}

void MessageGenerator::GenerateClassDefinition(io::Printer* printer) {
  Formatter format(printer, variables_);
  format.Set("class_final", ShouldMarkClassAsFinal(descriptor_, options_)
                                ? "PROTOBUF_FINAL"
                                : "");

  if (IsMapEntryMessage(descriptor_)) {
    std::map<std::string, std::string> vars;
    CollectMapInfo(options_, descriptor_, &vars);
    vars["lite"] =
        HasDescriptorMethods(descriptor_->file(), options_) ? "" : "Lite";
    format.AddMap(vars);
    format(
        "class $classname$ : public "
        "::$proto_ns$::internal::MapEntry$lite$<$classname$, \n"
        "    $key_cpp$, $val_cpp$,\n"
        "    ::$proto_ns$::internal::WireFormatLite::$key_wire_type$,\n"
        "    ::$proto_ns$::internal::WireFormatLite::$val_wire_type$,\n"
        "    $default_enum_value$ > {\n"
        "public:\n"
        "  typedef ::$proto_ns$::internal::MapEntry$lite$<$classname$, \n"
        "    $key_cpp$, $val_cpp$,\n"
        "    ::$proto_ns$::internal::WireFormatLite::$key_wire_type$,\n"
        "    ::$proto_ns$::internal::WireFormatLite::$val_wire_type$,\n"
        "    $default_enum_value$ > SuperType;\n"
        "  $classname$();\n"
        "  explicit $classname$(::$proto_ns$::Arena* arena);\n"
        "  void MergeFrom(const $classname$& other);\n"
        "  static const $classname$* internal_default_instance() { return "
        "reinterpret_cast<const "
        "$classname$*>(&_$classname$_default_instance_); }\n");
    auto utf8_check = GetUtf8CheckMode(descriptor_->field(0), options_);
    if (descriptor_->field(0)->type() == FieldDescriptor::TYPE_STRING &&
        utf8_check != NONE) {
      if (utf8_check == STRICT) {
        format(
            "  static bool ValidateKey(std::string* s) {\n"
            "    return ::$proto_ns$::internal::WireFormatLite::"
            "VerifyUtf8String(s->data(), static_cast<int>(s->size()), "
            "::$proto_ns$::internal::WireFormatLite::PARSE, \"$1$\");\n"
            " }\n",
            descriptor_->field(0)->full_name());
      } else {
        GOOGLE_CHECK(utf8_check == VERIFY);
        format(
            "  static bool ValidateKey(std::string* s) {\n"
            "#ifndef NDEBUG\n"
            "    ::$proto_ns$::internal::WireFormatLite::VerifyUtf8String(\n"
            "       s->data(), static_cast<int>(s->size()), "
            "::$proto_ns$::internal::"
            "WireFormatLite::PARSE, \"$1$\");\n"
            "#endif\n"
            "    return true;\n"
            " }\n",
            descriptor_->field(0)->full_name());
      }
    } else {
      format("  static bool ValidateKey(void*) { return true; }\n");
    }
    if (descriptor_->field(1)->type() == FieldDescriptor::TYPE_STRING &&
        utf8_check != NONE) {
      if (utf8_check == STRICT) {
        format(
            "  static bool ValidateValue(std::string* s) {\n"
            "    return ::$proto_ns$::internal::WireFormatLite::"
            "VerifyUtf8String(s->data(), static_cast<int>(s->size()), "
            "::$proto_ns$::internal::WireFormatLite::PARSE, \"$1$\");\n"
            " }\n",
            descriptor_->field(1)->full_name());
      } else {
        GOOGLE_CHECK(utf8_check = VERIFY);
        format(
            "  static bool ValidateValue(std::string* s) {\n"
            "#ifndef NDEBUG\n"
            "    ::$proto_ns$::internal::WireFormatLite::VerifyUtf8String(\n"
            "       s->data(), static_cast<int>(s->size()), "
            "::$proto_ns$::internal::"
            "WireFormatLite::PARSE, \"$1$\");\n"
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
          "  void MergeFrom(const ::$proto_ns$::Message& other) final;\n"
          "  ::$proto_ns$::Metadata GetMetadata() const final;\n"
          "  private:\n"
          "  static ::$proto_ns$::Metadata GetMetadataStatic() {\n"
          "    ::$proto_ns$::internal::AssignDescriptors(&::$desc_table$);\n"
          "    return ::$desc_table$.file_level_metadata[$1$];\n"
          "  }\n"
          "\n"
          "  public:\n"
          "};\n",
          index_in_file_messages_);
    } else {
      format("};\n");
    }
    return;
  }

  format(
      "class $dllexport_decl $${1$$classname$$}$$ class_final$ :\n"
      "    public $superclass$ /* @@protoc_insertion_point("
      "class_definition:$full_name$) */ {\n",
      descriptor_);
  format(" public:\n");
  format.Indent();

  if (SupportsArenas(descriptor_)) {
    format("inline $classname$() : $classname$(nullptr) {}\n");
  } else {
    format("$classname$();\n");
  }

  format(
      "virtual ~$classname$();\n"
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
      "  if (GetArena() == from.GetArena()) {\n"
      "    if (this != &from) InternalSwap(&from);\n"
      "  } else {\n"
      "    CopyFrom(from);\n"
      "  }\n"
      "  return *this;\n"
      "}\n"
      "\n");

  if (options_.table_driven_serialization) {
    format(
        "private:\n"
        "const void* InternalGetTable() const;\n"
        "public:\n"
        "\n");
  }

  std::map<std::string, std::string> vars;
  SetUnknkownFieldsVariable(descriptor_, options_, &vars);
  format.AddMap(vars);
  if (PublicUnknownFieldsAccessors(descriptor_)) {
    format(
        "inline const $unknown_fields_type$& unknown_fields() const {\n"
        "  return $unknown_fields$;\n"
        "}\n"
        "inline $unknown_fields_type$* mutable_unknown_fields() {\n"
        "  return $mutable_unknown_fields$;\n"
        "}\n"
        "\n");
  }

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
        "  return GetMetadataStatic().descriptor;\n"
        "}\n"
        "static const ::$proto_ns$::Reflection* GetReflection() {\n"
        "  return GetMetadataStatic().reflection;\n"
        "}\n");
  }

  format(
      "static const $classname$& default_instance();\n"
      "\n");

  // Generate enum values for every field in oneofs. One list is generated for
  // each oneof with an additional *_NOT_SET value.
  for (auto oneof : OneOfRange(descriptor_)) {
    format("enum $1$Case {\n", UnderscoresToCamelCase(oneof->name(), true));
    format.Indent();
    for (auto field : FieldRange(oneof)) {
      std::string oneof_enum_case_field_name =
          UnderscoresToCamelCase(field->name(), true);
      format("k$1$ = $2$,\n", oneof_enum_case_field_name,  // 1
             field->number());                             // 2
    }
    format("$1$_NOT_SET = 0,\n", ToUpper(oneof->name()));
    format.Outdent();
    format(
        "};\n"
        "\n");
  }

  // TODO(gerbens) make this private, while still granting other protos access.
  format(
      "static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY\n"
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
          "void PackFrom(const ::$proto_ns$::Message& message) {\n"
          "  _any_metadata_.PackFrom(message);\n"
          "}\n"
          "void PackFrom(const ::$proto_ns$::Message& message,\n"
          "              const std::string& type_url_prefix) {\n"
          "  _any_metadata_.PackFrom(message, type_url_prefix);\n"
          "}\n"
          "bool UnpackTo(::$proto_ns$::Message* message) const {\n"
          "  return _any_metadata_.UnpackTo(message);\n"
          "}\n"
          "static bool GetAnyFieldDescriptors(\n"
          "    const ::$proto_ns$::Message& message,\n"
          "    const ::$proto_ns$::FieldDescriptor** type_url_field,\n"
          "    const ::$proto_ns$::FieldDescriptor** value_field);\n"
          "template <typename T, class = typename std::enable_if<"
          "!std::is_convertible<T, const ::$proto_ns$::Message&>"
          "::value>::type>\n"
          "void PackFrom(const T& message) {\n"
          "  _any_metadata_.PackFrom<T>(message);\n"
          "}\n"
          "template <typename T, class = typename std::enable_if<"
          "!std::is_convertible<T, const ::$proto_ns$::Message&>"
          "::value>::type>\n"
          "void PackFrom(const T& message,\n"
          "              const std::string& type_url_prefix) {\n"
          "  _any_metadata_.PackFrom<T>(message, type_url_prefix);"
          "}\n"
          "template <typename T, class = typename std::enable_if<"
          "!std::is_convertible<T, const ::$proto_ns$::Message&>"
          "::value>::type>\n"
          "bool UnpackTo(T* message) const {\n"
          "  return _any_metadata_.UnpackTo<T>(message);\n"
          "}\n");
    } else {
      format(
          "template <typename T>\n"
          "void PackFrom(const T& message) {\n"
          "  _any_metadata_.PackFrom(message);\n"
          "}\n"
          "template <typename T>\n"
          "void PackFrom(const T& message,\n"
          "              const std::string& type_url_prefix) {\n"
          "  _any_metadata_.PackFrom(message, type_url_prefix);\n"
          "}\n"
          "template <typename T>\n"
          "bool UnpackTo(T* message) const {\n"
          "  return _any_metadata_.UnpackTo(message);\n"
          "}\n");
    }
    format(
        "template<typename T> bool Is() const {\n"
        "  return _any_metadata_.Is<T>();\n"
        "}\n"
        "static bool ParseAnyTypeUrl(const string& type_url,\n"
        "                            std::string* full_type_name);\n");
  }

  format.Set("new_final",
             ShouldMarkNewAsFinal(descriptor_, options_) ? "final" : "");

  format(
      "friend void swap($classname$& a, $classname$& b) {\n"
      "  a.Swap(&b);\n"
      "}\n");

  if (SupportsArenas(descriptor_)) {
    format(
        "inline void Swap($classname$* other) {\n"
        "  if (other == this) return;\n"
        "  if (GetArena() == other->GetArena()) {\n"
        "    InternalSwap(other);\n"
        "  } else {\n"
        "    ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);\n"
        "  }\n"
        "}\n"
        "void UnsafeArenaSwap($classname$* other) {\n"
        "  if (other == this) return;\n"
        "  $DCHK$(GetArena() == other->GetArena());\n"
        "  InternalSwap(other);\n"
        "}\n");
  } else {
    format(
        "inline void Swap($classname$* other) {\n"
        "  if (other == this) return;\n"
        "  InternalSwap(other);\n"
        "}\n");
  }

  format(
      "\n"
      "// implements Message ----------------------------------------------\n"
      "\n"
      "inline $classname$* New() const$ new_final$ {\n"
      "  return CreateMaybeMessage<$classname$>(nullptr);\n"
      "}\n"
      "\n"
      "$classname$* New(::$proto_ns$::Arena* arena) const$ new_final$ {\n"
      "  return CreateMaybeMessage<$classname$>(arena);\n"
      "}\n");

  // For instances that derive from Message (rather than MessageLite), some
  // methods are virtual and should be marked as final.
  format.Set("full_final", HasDescriptorMethods(descriptor_->file(), options_)
                               ? "final"
                               : "");

  if (HasGeneratedMethods(descriptor_->file(), options_)) {
    if (HasDescriptorMethods(descriptor_->file(), options_)) {
      format(
          "void CopyFrom(const ::$proto_ns$::Message& from) final;\n"
          "void MergeFrom(const ::$proto_ns$::Message& from) final;\n");
    } else {
      format(
          "void CheckTypeAndMergeFrom(const ::$proto_ns$::MessageLite& from)\n"
          "  final;\n");
    }

    format.Set("clear_final",
               ShouldMarkClearAsFinal(descriptor_, options_) ? "final" : "");
    format.Set(
        "is_initialized_final",
        ShouldMarkIsInitializedAsFinal(descriptor_, options_) ? "final" : "");

    format(
        "void CopyFrom(const $classname$& from);\n"
        "void MergeFrom(const $classname$& from);\n"
        "PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear()$ clear_final$;\n"
        "bool IsInitialized() const$ is_initialized_final$;\n"
        "\n"
        "size_t ByteSizeLong() const final;\n"
        "const char* _InternalParse(const char* ptr, "
        "::$proto_ns$::internal::ParseContext* ctx) final;\n"
        "$uint8$* _InternalSerialize(\n"
        "    $uint8$* target, ::$proto_ns$::io::EpsCopyOutputStream* stream) "
        "const final;\n");

    // DiscardUnknownFields() is implemented in message.cc using reflections. We
    // need to implement this function in generated code for messages.
    if (!UseUnknownFieldSet(descriptor_->file(), options_)) {
      format("void DiscardUnknownFields()$ full_final$;\n");
    }
  }

  format(
      "int GetCachedSize() const final { return _cached_size_.Get(); }"
      "\n\nprivate:\n"
      "inline void SharedCtor();\n"
      "inline void SharedDtor();\n"
      "void SetCachedSize(int size) const$ full_final$;\n"
      "void InternalSwap($classname$* other);\n");

  format(
      // Friend AnyMetadata so that it can call this FullMessageName() method.
      "friend class ::$proto_ns$::internal::AnyMetadata;\n"
      "static $1$ FullMessageName() {\n"
      "  return \"$full_name$\";\n"
      "}\n",
      options_.opensource_runtime ? "::PROTOBUF_NAMESPACE_ID::StringPiece"
                                  : "::StringPiece");

  if (SupportsArenas(descriptor_)) {
    format(
        // TODO(gerbens) Make this private! Currently people are deriving from
        // protos to give access to this constructor, breaking the invariants
        // we rely on.
        "protected:\n"
        "explicit $classname$(::$proto_ns$::Arena* arena);\n"
        "private:\n"
        "static void ArenaDtor(void* object);\n"
        "inline void RegisterArenaDtor(::$proto_ns$::Arena* arena);\n");
  }

  format(
      "public:\n"
      "\n");

  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    format(
        "::$proto_ns$::Metadata GetMetadata() const final;\n"
        "private:\n"
        "static ::$proto_ns$::Metadata GetMetadataStatic() {\n"
        "  ::$proto_ns$::internal::AssignDescriptors(&::$desc_table$);\n"
        "  return ::$desc_table$.file_level_metadata[kIndexInFileMessages];\n"
        "}\n"
        "\n"
        "public:\n"
        "\n");
  } else {
    format(
        "std::string GetTypeName() const final;\n"
        "\n");
  }

  format(
      "// nested types ----------------------------------------------------\n"
      "\n");

  // Import all nested message classes into this class's scope with typedefs.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    const Descriptor* nested_type = descriptor_->nested_type(i);
    if (!IsMapEntryMessage(nested_type)) {
      format.Set("nested_full_name", ClassName(nested_type, false));
      format.Set("nested_name", ResolveKeyword(nested_type->name()));
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
    enum_generators_[i]->GenerateSymbolImports(printer);
    format("\n");
  }

  format(
      "// accessors -------------------------------------------------------\n"
      "\n");

  // Generate accessor methods for all fields.
  GenerateFieldAccessorDeclarations(printer);

  // Declare extension identifiers.
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    extension_generators_[i]->GenerateDeclaration(printer);
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
        "size_t RequiredFieldsByteSizeFallback() const;\n\n");
  }

  // Prepare decls for _cached_size_ and _has_bits_.  Their position in the
  // output will be determined later.

  bool need_to_emit_cached_size = true;
  const std::string cached_size_decl =
      "mutable ::$proto_ns$::internal::CachedSize _cached_size_;\n";

  const size_t sizeof_has_bits = HasBitsSize();
  const std::string has_bits_decl =
      sizeof_has_bits == 0
          ? ""
          : StrCat("::$proto_ns$::internal::HasBits<",
                         sizeof_has_bits / 4, "> _has_bits_;\n");

  // To minimize padding, data members are divided into three sections:
  // (1) members assumed to align to 8 bytes
  // (2) members corresponding to message fields, re-ordered to optimize
  //     alignment.
  // (3) members assumed to align to 4 bytes.

  // Members assumed to align to 8 bytes:

  if (descriptor_->extension_range_count() > 0) {
    format(
        "::$proto_ns$::internal::ExtensionSet _extensions_;\n"
        "\n");
  }

  if (SupportsArenas(descriptor_)) {
    format(
        "template <typename T> friend class "
        "::$proto_ns$::Arena::InternalHelper;\n"
        "typedef void InternalArenaConstructable_;\n"
        "typedef void DestructorSkippable_;\n");
  }

  if (!has_bit_indices_.empty()) {
    // _has_bits_ is frequently accessed, so to reduce code size and improve
    // speed, it should be close to the start of the object. Placing
    // _cached_size_ together with _has_bits_ improves cache locality despite
    // potential alignment padding.
    format(has_bits_decl.c_str());
    format(cached_size_decl.c_str());
    need_to_emit_cached_size = false;
  }

  // Field members:

  // Emit some private and static members
  for (auto field : optimized_order_) {
    const FieldGenerator& generator = field_generators_.get(field);
    generator.GenerateStaticMembers(printer);
    generator.GeneratePrivateMembers(printer);
  }

  // For each oneof generate a union
  for (auto oneof : OneOfRange(descriptor_)) {
    std::string camel_oneof_name = UnderscoresToCamelCase(oneof->name(), true);
    format(
        "union $1$Union {\n"
        // explicit empty constructor is needed when union contains
        // ArenaStringPtr members for string fields.
        "  $1$Union() {}\n",
        camel_oneof_name);
    format.Indent();
    for (auto field : FieldRange(oneof)) {
      if (IsFieldUsed(field, options_)) {
        field_generators_.get(field).GeneratePrivateMembers(printer);
      }
    }
    format.Outdent();
    format("} $1$_;\n", oneof->name());
    for (auto field : FieldRange(oneof)) {
      if (IsFieldUsed(field, options_)) {
        field_generators_.get(field).GenerateStaticMembers(printer);
      }
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

  // The TableStruct struct needs access to the private parts, in order to
  // construct the offsets of all members.
  format("friend struct ::$tablename$;\n");

  format.Outdent();
  format("};");
  GOOGLE_DCHECK(!need_to_emit_cached_size);
}  // NOLINT(readability/fn_size)

void MessageGenerator::GenerateInlineMethods(io::Printer* printer) {
  if (IsMapEntryMessage(descriptor_)) return;
  GenerateFieldAccessorDefinitions(printer);

  // Generate oneof_case() functions.
  for (auto oneof : OneOfRange(descriptor_)) {
    Formatter format(printer, variables_);
    format.Set("camel_oneof_name", UnderscoresToCamelCase(oneof->name(), true));
    format.Set("oneof_name", oneof->name());
    format.Set("oneof_index", oneof->index());
    format(
        "inline $classname$::$camel_oneof_name$Case $classname$::"
        "${1$$oneof_name$_case$}$() const {\n"
        "  return $classname$::$camel_oneof_name$Case("
        "_oneof_case_[$oneof_index$]);\n"
        "}\n",
        oneof);
  }
}

void MessageGenerator::GenerateExtraDefaultFields(io::Printer* printer) {
  // Generate oneof default instance and weak field instances for reflection
  // usage.
  Formatter format(printer, variables_);
  for (auto oneof : OneOfRange(descriptor_)) {
    for (auto field : FieldRange(oneof)) {
      if (!IsFieldUsed(field, options_)) {
        continue;
      }
      if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ||
          (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING &&
           EffectiveStringCType(field, options_) != FieldOptions::STRING)) {
        format("const ");
      }
      field_generators_.get(field).GeneratePrivateMembers(printer);
    }
  }
  for (auto field : FieldRange(descriptor_)) {
    if (field->options().weak() && IsFieldUsed(field, options_)) {
      format("  const ::$proto_ns$::Message* $1$_;\n", FieldName(field));
    }
  }
}

bool MessageGenerator::GenerateParseTable(io::Printer* printer, size_t offset,
                                          size_t aux_offset) {
  Formatter format(printer, variables_);

  if (!table_driven_) {
    format("{ nullptr, nullptr, 0, -1, -1, -1, -1, nullptr, false },\n");
    return false;
  }

  int max_field_number = 0;
  for (auto field : FieldRange(descriptor_)) {
    if (max_field_number < field->number()) {
      max_field_number = field->number();
    }
  }

  format("{\n");
  format.Indent();

  format(
      "$tablename$::entries + $1$,\n"
      "$tablename$::aux + $2$,\n"
      "$3$,\n",
      offset, aux_offset, max_field_number);

  if (has_bit_indices_.empty()) {
    // If no fields have hasbits, then _has_bits_ does not exist.
    format("-1,\n");
  } else {
    format("PROTOBUF_FIELD_OFFSET($classtype$, _has_bits_),\n");
  }

  if (descriptor_->real_oneof_decl_count() > 0) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, _oneof_case_),\n");
  } else {
    format("-1,  // no _oneof_case_\n");
  }

  if (descriptor_->extension_range_count() > 0) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, _extensions_),\n");
  } else {
    format("-1,  // no _extensions_\n");
  }

  // TODO(ckennelly): Consolidate this with the calculation for
  // AuxiliaryParseTableField.
  format(
      "PROTOBUF_FIELD_OFFSET($classtype$, _internal_metadata_),\n"
      "&$package_ns$::_$classname$_default_instance_,\n");

  if (UseUnknownFieldSet(descriptor_->file(), options_)) {
    format("true,\n");
  } else {
    format("false,\n");
  }

  format.Outdent();
  format("},\n");
  return true;
}

void MessageGenerator::GenerateSchema(io::Printer* printer, int offset,
                                      int has_offset) {
  Formatter format(printer, variables_);
  has_offset = !has_bit_indices_.empty() || IsMapEntryMessage(descriptor_)
                   ? offset + has_offset
                   : -1;

  format("{ $1$, $2$, sizeof($classtype$)},\n", offset, has_offset);
}

namespace {

// We need to calculate for each field what function the table driven code
// should use to serialize it. This returns the index in a lookup table.
uint32 CalcFieldNum(const FieldGenerator& generator,
                    const FieldDescriptor* field, const Options& options) {
  bool is_a_map = IsMapEntryMessage(field->containing_type());
  int type = field->type();
  if (type == FieldDescriptor::TYPE_STRING ||
      type == FieldDescriptor::TYPE_BYTES) {
    if (generator.IsInlined()) {
      type = internal::FieldMetadata::kInlinedType;
    }
    // string field
    if (IsCord(field, options)) {
      type = internal::FieldMetadata::kCordType;
    } else if (IsStringPiece(field, options)) {
      type = internal::FieldMetadata::kStringPieceType;
    }
  }

  if (field->real_containing_oneof()) {
    return internal::FieldMetadata::CalculateType(
        type, internal::FieldMetadata::kOneOf);
  } else if (field->is_packed()) {
    return internal::FieldMetadata::CalculateType(
        type, internal::FieldMetadata::kPacked);
  } else if (field->is_repeated()) {
    return internal::FieldMetadata::CalculateType(
        type, internal::FieldMetadata::kRepeated);
  } else if (HasHasbit(field) || field->real_containing_oneof() || is_a_map) {
    return internal::FieldMetadata::CalculateType(
        type, internal::FieldMetadata::kPresence);
  } else {
    return internal::FieldMetadata::CalculateType(
        type, internal::FieldMetadata::kNoPresence);
  }
}

int FindMessageIndexInFile(const Descriptor* descriptor) {
  std::vector<const Descriptor*> flatten =
      FlattenMessagesInFile(descriptor->file());
  return std::find(flatten.begin(), flatten.end(), descriptor) -
         flatten.begin();
}

}  // namespace

int MessageGenerator::GenerateFieldMetadata(io::Printer* printer) {
  Formatter format(printer, variables_);
  if (!options_.table_driven_serialization) {
    return 0;
  }

  std::vector<const FieldDescriptor*> sorted = SortFieldsByNumber(descriptor_);
  if (IsMapEntryMessage(descriptor_)) {
    for (int i = 0; i < 2; i++) {
      const FieldDescriptor* field = sorted[i];
      const FieldGenerator& generator = field_generators_.get(field);

      uint32 tag = internal::WireFormatLite::MakeTag(
          field->number(), WireFormat::WireTypeForFieldType(field->type()));

      std::map<std::string, std::string> vars;
      vars["classtype"] = QualifiedClassName(descriptor_, options_);
      vars["field_name"] = FieldName(field);
      vars["tag"] = StrCat(tag);
      vars["hasbit"] = StrCat(i);
      vars["type"] = StrCat(CalcFieldNum(generator, field, options_));
      vars["ptr"] = "nullptr";
      if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        GOOGLE_CHECK(!IsMapEntryMessage(field->message_type()));
        vars["ptr"] =
            "::" + UniqueName("TableStruct", field->message_type(), options_) +
            "::serialization_table + " +
            StrCat(FindMessageIndexInFile(field->message_type()));
      }
      Formatter::SaveState saver(&format);
      format.AddMap(vars);
      format(
          "{PROTOBUF_FIELD_OFFSET("
          "::$proto_ns$::internal::MapEntryHelper<$classtype$::"
          "SuperType>, $field_name$_), $tag$,"
          "PROTOBUF_FIELD_OFFSET("
          "::$proto_ns$::internal::MapEntryHelper<$classtype$::"
          "SuperType>, _has_bits_) * 8 + $hasbit$, $type$, "
          "$ptr$},\n");
    }
    return 2;
  }
  format(
      "{PROTOBUF_FIELD_OFFSET($classtype$, _cached_size_),"
      " 0, 0, 0, nullptr},\n");
  std::vector<const Descriptor::ExtensionRange*> sorted_extensions;
  sorted_extensions.reserve(descriptor_->extension_range_count());
  for (int i = 0; i < descriptor_->extension_range_count(); ++i) {
    sorted_extensions.push_back(descriptor_->extension_range(i));
  }
  std::sort(sorted_extensions.begin(), sorted_extensions.end(),
            ExtensionRangeSorter());
  for (int i = 0, extension_idx = 0; /* no range */; i++) {
    for (; extension_idx < sorted_extensions.size() &&
           (i == sorted.size() ||
            sorted_extensions[extension_idx]->start < sorted[i]->number());
         extension_idx++) {
      const Descriptor::ExtensionRange* range =
          sorted_extensions[extension_idx];
      format(
          "{PROTOBUF_FIELD_OFFSET($classtype$, _extensions_), "
          "$1$, $2$, ::$proto_ns$::internal::FieldMetadata::kSpecial, "
          "reinterpret_cast<const "
          "void*>(::$proto_ns$::internal::ExtensionSerializer)},\n",
          range->start, range->end);
    }
    if (i == sorted.size()) break;
    const FieldDescriptor* field = sorted[i];

    uint32 tag = internal::WireFormatLite::MakeTag(
        field->number(), WireFormat::WireTypeForFieldType(field->type()));
    if (field->is_packed()) {
      tag = internal::WireFormatLite::MakeTag(
          field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
    }

    std::string classfieldname = FieldName(field);
    if (field->real_containing_oneof()) {
      classfieldname = field->containing_oneof()->name();
    }
    format.Set("field_name", classfieldname);
    std::string ptr = "nullptr";
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (IsMapEntryMessage(field->message_type())) {
        format(
            "{PROTOBUF_FIELD_OFFSET($classtype$, $field_name$_), $1$, $2$, "
            "::$proto_ns$::internal::FieldMetadata::kSpecial, "
            "reinterpret_cast<const void*>(static_cast< "
            "::$proto_ns$::internal::SpecialSerializer>("
            "::$proto_ns$::internal::MapFieldSerializer< "
            "::$proto_ns$::internal::MapEntryToMapField<"
            "$3$>::MapFieldType, "
            "$tablename$::serialization_table>))},\n",
            tag, FindMessageIndexInFile(field->message_type()),
            QualifiedClassName(field->message_type(), options_));
        continue;
      } else if (!field->message_type()->options().message_set_wire_format()) {
        // message_set doesn't have the usual table and we need to
        // dispatch to generated serializer, hence ptr stays zero.
        ptr =
            "::" + UniqueName("TableStruct", field->message_type(), options_) +
            "::serialization_table + " +
            StrCat(FindMessageIndexInFile(field->message_type()));
      }
    }

    const FieldGenerator& generator = field_generators_.get(field);
    int type = CalcFieldNum(generator, field, options_);

    if (IsLazy(field, options_)) {
      type = internal::FieldMetadata::kSpecial;
      ptr = "reinterpret_cast<const void*>(::" + variables_["proto_ns"] +
            "::internal::LazyFieldSerializer";
      if (field->real_containing_oneof()) {
        ptr += "OneOf";
      } else if (!HasHasbit(field)) {
        ptr += "NoPresence";
      }
      ptr += ")";
    }

    if (field->options().weak()) {
      // TODO(gerbens) merge weak fields into ranges
      format(
          "{PROTOBUF_FIELD_OFFSET("
          "$classtype$, _weak_field_map_), $1$, $1$, "
          "::$proto_ns$::internal::FieldMetadata::kSpecial, "
          "reinterpret_cast<const "
          "void*>(::$proto_ns$::internal::WeakFieldSerializer)},\n",
          tag);
    } else if (field->real_containing_oneof()) {
      format.Set("oneofoffset",
                 sizeof(uint32) * field->containing_oneof()->index());
      format(
          "{PROTOBUF_FIELD_OFFSET($classtype$, $field_name$_), $1$,"
          " PROTOBUF_FIELD_OFFSET($classtype$, _oneof_case_) + "
          "$oneofoffset$, $2$, $3$},\n",
          tag, type, ptr);
    } else if (HasHasbit(field)) {
      format.Set("hasbitsoffset", has_bit_indices_[field->index()]);
      format(
          "{PROTOBUF_FIELD_OFFSET($classtype$, $field_name$_), "
          "$1$, PROTOBUF_FIELD_OFFSET($classtype$, _has_bits_) * 8 + "
          "$hasbitsoffset$, $2$, $3$},\n",
          tag, type, ptr);
    } else {
      format(
          "{PROTOBUF_FIELD_OFFSET($classtype$, $field_name$_), "
          "$1$, ~0u, $2$, $3$},\n",
          tag, type, ptr);
    }
  }
  int num_field_metadata = 1 + sorted.size() + sorted_extensions.size();
  num_field_metadata++;
  std::string serializer = UseUnknownFieldSet(descriptor_->file(), options_)
                               ? "UnknownFieldSetSerializer"
                               : "UnknownFieldSerializerLite";
  format(
      "{PROTOBUF_FIELD_OFFSET($classtype$, _internal_metadata_), 0, ~0u, "
      "::$proto_ns$::internal::FieldMetadata::kSpecial, reinterpret_cast<const "
      "void*>(::$proto_ns$::internal::$1$)},\n",
      serializer);
  return num_field_metadata;
}

void MessageGenerator::GenerateFieldDefaultInstances(io::Printer* printer) {
  // Construct the default instances for all fields that need one.
  for (auto field : FieldRange(descriptor_)) {
    field_generators_.get(field).GenerateDefaultInstanceAllocator(printer);
  }
}

void MessageGenerator::GenerateDefaultInstanceInitializer(
    io::Printer* printer) {
  Formatter format(printer, variables_);

  // The default instance needs all of its embedded message pointers
  // cross-linked to other default instances.  We can't do this initialization
  // in the constructor because some other default instances may not have been
  // constructed yet at that time.
  // TODO(kenton):  Maybe all message fields (even for non-default messages)
  //   should be initialized to point at default instances rather than NULL?
  for (auto field : FieldRange(descriptor_)) {
    if (!IsFieldUsed(field, options_)) {
      continue;
    }
    Formatter::SaveState saver(&format);

    if (!field->is_repeated() && !IsLazy(field, options_) &&
        field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
        (!field->real_containing_oneof() ||
         HasDescriptorMethods(descriptor_->file(), options_))) {
      std::string name;
      if (field->real_containing_oneof() || field->options().weak()) {
        name = "_" + classname_ + "_default_instance_.";
      } else {
        name =
            "_" + classname_ + "_default_instance_._instance.get_mutable()->";
      }
      name += FieldName(field);
      format.Set("name", name);
      if (IsWeak(field, options_)) {
        format(
            "$package_ns$::$name$_ = reinterpret_cast<const "
            "::$proto_ns$::Message*>(&$1$);\n"
            "if ($package_ns$::$name$_ == nullptr) {\n"
            "  $package_ns$::$name$_ = "
            "::$proto_ns$::Empty::internal_default_instance();\n"
            "}\n",
            QualifiedDefaultInstanceName(field->message_type(),
                                         options_));  // 1
        continue;
      }
      if (IsImplicitWeakField(field, options_, scc_analyzer_)) {
        format(
            "$package_ns$::$name$_ = reinterpret_cast<$1$*>(\n"
            "    $2$);\n",
            FieldMessageTypeName(field, options_),
            QualifiedDefaultInstancePtr(field->message_type(), options_));
      } else {
        format(
            "$package_ns$::$name$_ = const_cast< $1$*>(\n"
            "    $1$::internal_default_instance());\n",
            FieldMessageTypeName(field, options_));
      }
    } else if (field->real_containing_oneof() &&
               HasDescriptorMethods(descriptor_->file(), options_)) {
      field_generators_.get(field).GenerateConstructorCode(printer);
    }
  }
}

void MessageGenerator::GenerateClassMethods(io::Printer* printer) {
  Formatter format(printer, variables_);
  if (IsMapEntryMessage(descriptor_)) {
    format(
        "$classname$::$classname$() {}\n"
        "$classname$::$classname$(::$proto_ns$::Arena* arena)\n"
        "    : SuperType(arena) {}\n"
        "void $classname$::MergeFrom(const $classname$& other) {\n"
        "  MergeFromInternal(other);\n"
        "}\n");
    if (HasDescriptorMethods(descriptor_->file(), options_)) {
      format(
          "::$proto_ns$::Metadata $classname$::GetMetadata() const {\n"
          "  return GetMetadataStatic();\n"
          "}\n");
      format(
          "void $classname$::MergeFrom(\n"
          "    const ::$proto_ns$::Message& other) {\n"
          "  ::$proto_ns$::Message::MergeFrom(other);\n"
          "}\n"
          "\n");
    }
    return;
  }

  // TODO(gerbens) Remove this function. With a little bit of cleanup and
  // refactoring this is superfluous.
  format("void $classname$::InitAsDefaultInstance() {\n");
  format.Indent();
  GenerateDefaultInstanceInitializer(printer);
  format.Outdent();
  format("}\n");

  if (IsAnyMessage(descriptor_, options_)) {
    if (HasDescriptorMethods(descriptor_->file(), options_)) {
      format(
          "bool $classname$::GetAnyFieldDescriptors(\n"
          "    const ::$proto_ns$::Message& message,\n"
          "    const ::$proto_ns$::FieldDescriptor** type_url_field,\n"
          "    const ::$proto_ns$::FieldDescriptor** value_field) {\n"
          "  return ::$proto_ns$::internal::GetAnyFieldDescriptors(\n"
          "      message, type_url_field, value_field);\n"
          "}\n");
    }
    format(
        "bool $classname$::ParseAnyTypeUrl(const string& type_url,\n"
        "                                  std::string* full_type_name) {\n"
        "  return ::$proto_ns$::internal::ParseAnyTypeUrl(type_url,\n"
        "                                             full_type_name);\n"
        "}\n"
        "\n");
  }

  format(
      "class $classname$::_Internal {\n"
      " public:\n");
  format.Indent();
  if (!has_bit_indices_.empty()) {
    format(
        "using HasBits = decltype(std::declval<$classname$>()._has_bits_);\n");
  }
  for (auto field : FieldRange(descriptor_)) {
    field_generators_.get(field).GenerateInternalAccessorDeclarations(printer);
    if (!IsFieldUsed(field, options_)) {
      continue;
    }
    if (HasHasbit(field)) {
      int has_bit_index = HasBitIndex(field);
      GOOGLE_CHECK_NE(has_bit_index, kNoHasbit) << field->full_name();
      format(
          "static void set_has_$1$(HasBits* has_bits) {\n"
          "  (*has_bits)[$2$] |= $3$u;\n"
          "}\n",
          FieldName(field), has_bit_index / 32, (1u << (has_bit_index % 32)));
    }
  }
  if (num_required_fields_ > 0) {
    const std::vector<uint32> masks_for_has_bits = RequiredFieldsBitMask();
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
    if (IsFieldUsed(field, options_)) {
      field_generators_.get(field).GenerateInternalAccessorDefinitions(printer);
    }
  }

  // Generate non-inline field definitions.
  for (auto field : FieldRange(descriptor_)) {
    if (!IsFieldUsed(field, options_)) {
      continue;
    }
    field_generators_.get(field).GenerateNonInlineAccessorDefinitions(printer);
    if (IsCrossFileMaybeMap(field)) {
      Formatter::SaveState saver(&format);
      std::map<std::string, std::string> vars;
      SetCommonFieldVariables(field, &vars, options_);
      if (field->real_containing_oneof()) {
        SetCommonOneofFieldVariables(field, &vars);
      }
      format.AddMap(vars);
      GenerateFieldClear(field, false, format);
    }
  }

  GenerateStructors(printer);
  format("\n");

  if (descriptor_->real_oneof_decl_count() > 0) {
    GenerateOneofClear(printer);
    format("\n");
  }

  if (HasGeneratedMethods(descriptor_->file(), options_)) {
    GenerateClear(printer);
    format("\n");

    GenerateMergeFromCodedStream(printer);
    format("\n");

    GenerateSerializeWithCachedSizesToArray(printer);
    format("\n");

    GenerateByteSize(printer);
    format("\n");

    GenerateMergeFrom(printer);
    format("\n");

    GenerateClassSpecificMergeFrom(printer);
    format("\n");

    GenerateCopyFrom(printer);
    format("\n");

    GenerateIsInitialized(printer);
    format("\n");
  }

  GenerateSwap(printer);
  format("\n");

  if (options_.table_driven_serialization) {
    format(
        "const void* $classname$::InternalGetTable() const {\n"
        "  return ::$tablename$::serialization_table + $1$;\n"
        "}\n"
        "\n",
        index_in_file_messages_);
  }
  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    format(
        "::$proto_ns$::Metadata $classname$::GetMetadata() const {\n"
        "  return GetMetadataStatic();\n"
        "}\n"
        "\n");
  } else {
    format(
        "std::string $classname$::GetTypeName() const {\n"
        "  return \"$full_name$\";\n"
        "}\n"
        "\n");
  }

}

size_t MessageGenerator::GenerateParseOffsets(io::Printer* printer) {
  Formatter format(printer, variables_);

  if (!table_driven_) {
    return 0;
  }

  // Field "0" is special:  We use it in our switch statement of processing
  // types to handle the successful end tag case.
  format("{0, 0, 0, ::$proto_ns$::internal::kInvalidMask, 0, 0},\n");
  int last_field_number = 1;

  std::vector<const FieldDescriptor*> ordered_fields =
      SortFieldsByNumber(descriptor_);

  for (auto field : ordered_fields) {
    Formatter::SaveState saver(&format);
    GOOGLE_CHECK_GE(field->number(), last_field_number);

    for (; last_field_number < field->number(); last_field_number++) {
      format(
          "{ 0, 0, ::$proto_ns$::internal::kInvalidMask,\n"
          "  ::$proto_ns$::internal::kInvalidMask, 0, 0 },\n");
    }
    last_field_number++;

    unsigned char normal_wiretype, packed_wiretype, processing_type;
    normal_wiretype = WireFormat::WireTypeForFieldType(field->type());

    if (field->is_packable()) {
      packed_wiretype = WireFormatLite::WIRETYPE_LENGTH_DELIMITED;
    } else {
      packed_wiretype = internal::kNotPackedMask;
    }

    processing_type = static_cast<unsigned>(field->type());
    const FieldGenerator& generator = field_generators_.get(field);
    if (field->type() == FieldDescriptor::TYPE_STRING) {
      switch (EffectiveStringCType(field, options_)) {
        case FieldOptions::STRING:
          if (generator.IsInlined()) {
            processing_type = internal::TYPE_STRING_INLINED;
            break;
          }
          break;
        case FieldOptions::CORD:
          processing_type = internal::TYPE_STRING_CORD;
          break;
        case FieldOptions::STRING_PIECE:
          processing_type = internal::TYPE_STRING_STRING_PIECE;
          break;
      }
    } else if (field->type() == FieldDescriptor::TYPE_BYTES) {
      switch (EffectiveStringCType(field, options_)) {
        case FieldOptions::STRING:
          if (generator.IsInlined()) {
            processing_type = internal::TYPE_BYTES_INLINED;
            break;
          }
          break;
        case FieldOptions::CORD:
          processing_type = internal::TYPE_BYTES_CORD;
          break;
        case FieldOptions::STRING_PIECE:
          processing_type = internal::TYPE_BYTES_STRING_PIECE;
          break;
      }
    }

    processing_type |= static_cast<unsigned>(
        field->is_repeated() ? internal::kRepeatedMask : 0);
    processing_type |= static_cast<unsigned>(
        field->real_containing_oneof() ? internal::kOneofMask : 0);

    if (field->is_map()) {
      processing_type = internal::TYPE_MAP;
    }

    const unsigned char tag_size =
        WireFormat::TagSize(field->number(), field->type());

    std::map<std::string, std::string> vars;
    if (field->real_containing_oneof()) {
      vars["name"] = field->containing_oneof()->name();
      vars["presence"] = StrCat(field->containing_oneof()->index());
    } else {
      vars["name"] = FieldName(field);
      vars["presence"] = StrCat(has_bit_indices_[field->index()]);
    }
    vars["nwtype"] = StrCat(normal_wiretype);
    vars["pwtype"] = StrCat(packed_wiretype);
    vars["ptype"] = StrCat(processing_type);
    vars["tag_size"] = StrCat(tag_size);

    format.AddMap(vars);

    format(
        "{\n"
        "  PROTOBUF_FIELD_OFFSET($classtype$, $name$_),\n"
        "  static_cast<$uint32$>($presence$),\n"
        "  $nwtype$, $pwtype$, $ptype$, $tag_size$\n"
        "},\n");
  }

  return last_field_number;
}

size_t MessageGenerator::GenerateParseAuxTable(io::Printer* printer) {
  Formatter format(printer, variables_);

  if (!table_driven_) {
    return 0;
  }

  std::vector<const FieldDescriptor*> ordered_fields =
      SortFieldsByNumber(descriptor_);

  format("::$proto_ns$::internal::AuxiliaryParseTableField(),\n");
  int last_field_number = 1;
  for (auto field : ordered_fields) {
    Formatter::SaveState saver(&format);

    GOOGLE_CHECK_GE(field->number(), last_field_number);
    for (; last_field_number < field->number(); last_field_number++) {
      format("::$proto_ns$::internal::AuxiliaryParseTableField(),\n");
    }

    std::map<std::string, std::string> vars;
    SetCommonFieldVariables(field, &vars, options_);
    format.AddMap(vars);

    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_ENUM:
        if (HasPreservingUnknownEnumSemantics(field)) {
          format(
              "{::$proto_ns$::internal::AuxiliaryParseTableField::enum_aux{"
              "nullptr}},\n");
        } else {
          format(
              "{::$proto_ns$::internal::AuxiliaryParseTableField::enum_aux{"
              "$1$_IsValid}},\n",
              ClassName(field->enum_type(), true));
        }
        last_field_number++;
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE: {
        if (field->is_map()) {
          format(
              "{::$proto_ns$::internal::AuxiliaryParseTableField::map_"
              "aux{&::$proto_ns$::internal::ParseMap<$1$>}},\n",
              QualifiedClassName(field->message_type(), options_));
          last_field_number++;
          break;
        }
        format.Set("field_classname", ClassName(field->message_type(), false));
        format.Set("default_instance", QualifiedDefaultInstanceName(
                                           field->message_type(), options_));

        format(
            "{::$proto_ns$::internal::AuxiliaryParseTableField::message_aux{\n"
            "  &$default_instance$}},\n");
        last_field_number++;
        break;
      }
      case FieldDescriptor::CPPTYPE_STRING: {
        std::string default_val;
        switch (EffectiveStringCType(field, options_)) {
          case FieldOptions::STRING:
            default_val = field->default_value_string().empty()
                              ? "&::" + variables_["proto_ns"] +
                                    "::internal::fixed_address_empty_string"
                              : "&" +
                                    QualifiedClassName(descriptor_, options_) +
                                    "::" + MakeDefaultName(field);
            break;
          case FieldOptions::CORD:
          case FieldOptions::STRING_PIECE:
            default_val =
                "\"" + CEscape(field->default_value_string()) + "\"";
            break;
        }
        format(
            "{::$proto_ns$::internal::AuxiliaryParseTableField::string_aux{\n"
            "  $1$,\n"
            "  \"$2$\"\n"
            "}},\n",
            default_val, field->full_name());
        last_field_number++;
        break;
      }
      default:
        break;
    }
  }

  return last_field_number;
}

std::pair<size_t, size_t> MessageGenerator::GenerateOffsets(
    io::Printer* printer) {
  Formatter format(printer, variables_);

  if (!has_bit_indices_.empty() || IsMapEntryMessage(descriptor_)) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, _has_bits_),\n");
  } else {
    format("~0u,  // no _has_bits_\n");
  }
  format("PROTOBUF_FIELD_OFFSET($classtype$, _internal_metadata_),\n");
  if (descriptor_->extension_range_count() > 0) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, _extensions_),\n");
  } else {
    format("~0u,  // no _extensions_\n");
  }
  if (descriptor_->real_oneof_decl_count() > 0) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, _oneof_case_[0]),\n");
  } else {
    format("~0u,  // no _oneof_case_\n");
  }
  if (num_weak_fields_ > 0) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, _weak_field_map_),\n");
  } else {
    format("~0u,  // no _weak_field_map_\n");
  }
  const int kNumGenericOffsets = 5;  // the number of fixed offsets above
  const size_t offsets = kNumGenericOffsets + descriptor_->field_count() +
                         descriptor_->real_oneof_decl_count();
  size_t entries = offsets;
  for (auto field : FieldRange(descriptor_)) {
    if (!IsFieldUsed(field, options_)) {
      format("~0u,  // stripped\n");
      continue;
    }
    if (field->real_containing_oneof() || field->options().weak()) {
      format("offsetof($classtype$DefaultTypeInternal, $1$_)",
             FieldName(field));
    } else {
      format("PROTOBUF_FIELD_OFFSET($classtype$, $1$_)", FieldName(field));
    }

    uint32 tag = field_generators_.get(field).CalculateFieldTag();
    if (tag != 0) {
      format(" | $1$", tag);
    }

    format(",\n");
  }

  int count = 0;
  for (auto oneof : OneOfRange(descriptor_)) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, $1$_),\n", oneof->name());
    count++;
  }
  GOOGLE_CHECK_EQ(count, descriptor_->real_oneof_decl_count());

  if (IsMapEntryMessage(descriptor_)) {
    entries += 2;
    format(
        "0,\n"
        "1,\n");
  } else if (!has_bit_indices_.empty()) {
    entries += has_bit_indices_.size();
    for (int i = 0; i < has_bit_indices_.size(); i++) {
      const std::string index =
          has_bit_indices_[i] >= 0 ? StrCat(has_bit_indices_[i]) : "~0u";
      format("$1$,\n", index);
    }
  }

  return std::make_pair(entries, offsets);
}

void MessageGenerator::GenerateSharedConstructorCode(io::Printer* printer) {
  Formatter format(printer, variables_);

  format("void $classname$::SharedCtor() {\n");
  if (scc_analyzer_->GetSCCAnalysis(scc_analyzer_->GetSCC(descriptor_))
          .constructor_requires_initialization) {
    format("  ::$proto_ns$::internal::InitSCC(&$scc_info$.base);\n");
  }

  format.Indent();

  std::vector<bool> processed(optimized_order_.size(), false);
  GenerateConstructorBody(printer, processed, false);

  for (auto oneof : OneOfRange(descriptor_)) {
    format("clear_has_$1$();\n", oneof->name());
  }

  format.Outdent();
  format("}\n\n");
}

void MessageGenerator::GenerateSharedDestructorCode(io::Printer* printer) {
  Formatter format(printer, variables_);

  format("void $classname$::SharedDtor() {\n");
  format.Indent();
  if (SupportsArenas(descriptor_)) {
    format("$DCHK$(GetArena() == nullptr);\n");
  }
  // Write the destructors for each field except oneof members.
  // optimized_order_ does not contain oneof fields.
  for (auto field : optimized_order_) {
    field_generators_.get(field).GenerateDestructorCode(printer);
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
    format("_weak_field_map_.ClearAll();\n");
  }
  format.Outdent();
  format(
      "}\n"
      "\n");
}

void MessageGenerator::GenerateArenaDestructorCode(io::Printer* printer) {
  Formatter format(printer, variables_);

  // Generate the ArenaDtor() method. Track whether any fields actually produced
  // code that needs to be called.
  format("void $classname$::ArenaDtor(void* object) {\n");
  format.Indent();

  // This code is placed inside a static method, rather than an ordinary one,
  // since that simplifies Arena's destructor list (ordinary function pointers
  // rather than member function pointers). _this is the object being
  // destructed.
  format(
      "$classname$* _this = reinterpret_cast< $classname$* >(object);\n"
      // avoid an "unused variable" warning in case no fields have dtor code.
      "(void)_this;\n");

  bool need_registration = false;
  // Process non-oneof fields first.
  for (auto field : optimized_order_) {
    if (field_generators_.get(field).GenerateArenaDestructorCode(printer)) {
      need_registration = true;
    }
  }

  // Process oneof fields.
  //
  // Note:  As of 10/5/2016, GenerateArenaDestructorCode does not emit anything
  // and returns false for oneof fields.
  for (auto oneof : OneOfRange(descriptor_)) {
    for (auto field : FieldRange(oneof)) {
      if (IsFieldUsed(field, options_) &&
          field_generators_.get(field).GenerateArenaDestructorCode(printer)) {
        need_registration = true;
      }
    }
  }
  if (num_weak_fields_) {
    // _this is the object being destructed (we are inside a static method
    // here).
    format("_this->_weak_field_map_.ClearAll();\n");
    need_registration = true;
  }

  format.Outdent();
  format("}\n");

  if (need_registration) {
    format(
        "inline void $classname$::RegisterArenaDtor(::$proto_ns$::Arena* "
        "arena) {\n"
        "  if (arena != nullptr) {\n"
        "    arena->OwnCustomDestructor(this, &$classname$::ArenaDtor);\n"
        "  }\n"
        "}\n");
  } else {
    format(
        "void $classname$::RegisterArenaDtor(::$proto_ns$::Arena*) {\n"
        "}\n");
  }
}

void MessageGenerator::GenerateConstructorBody(io::Printer* printer,
                                               std::vector<bool> processed,
                                               bool copy_constructor) const {
  Formatter format(printer, variables_);

  const RunMap runs = FindRuns(
      optimized_order_, [copy_constructor, this](const FieldDescriptor* field) {
        return (copy_constructor && IsPOD(field)) ||
               (!copy_constructor &&
                CanBeManipulatedAsRawBytes(field, options_));
      });

  std::string pod_template;
  if (copy_constructor) {
    pod_template =
        "::memcpy(&$first$_, &from.$first$_,\n"
        "  static_cast<size_t>(reinterpret_cast<char*>(&$last$_) -\n"
        "  reinterpret_cast<char*>(&$first$_)) + sizeof($last$_));\n";
  } else {
    pod_template =
        "::memset(&$first$_, 0, static_cast<size_t>(\n"
        "    reinterpret_cast<char*>(&$last$_) -\n"
        "    reinterpret_cast<char*>(&$first$_)) + sizeof($last$_));\n";
  }

  for (int i = 0; i < optimized_order_.size(); ++i) {
    if (processed[i]) {
      continue;
    }

    const FieldDescriptor* field = optimized_order_[i];
    const auto it = runs.find(field);

    // We only apply the memset technique to runs of more than one field, as
    // assignment is better than memset for generated code clarity.
    if (it != runs.end() && it->second > 1) {
      // Use a memset, then skip run_length fields.
      const size_t run_length = it->second;
      const std::string first_field_name = FieldName(field);
      const std::string last_field_name =
          FieldName(optimized_order_[i + run_length - 1]);

      format.Set("first", first_field_name);
      format.Set("last", last_field_name);

      format(pod_template.c_str());

      i += run_length - 1;
      // ++i at the top of the loop.
    } else {
      if (copy_constructor) {
        field_generators_.get(field).GenerateCopyConstructorCode(printer);
      } else {
        field_generators_.get(field).GenerateConstructorCode(printer);
      }
    }
  }
}

void MessageGenerator::GenerateStructors(io::Printer* printer) {
  Formatter format(printer, variables_);

  std::string superclass;
  superclass = SuperClassName(descriptor_, options_);
  std::string initializer_with_arena = superclass + "(arena)";

  if (descriptor_->extension_range_count() > 0) {
    initializer_with_arena += ",\n  _extensions_(arena)";
  }

  // Initialize member variables with arena constructor.
  for (auto field : optimized_order_) {
    GOOGLE_DCHECK(IsFieldUsed(field, options_));
    bool has_arena_constructor = field->is_repeated();
    if (!field->real_containing_oneof() &&
        (IsLazy(field, options_) || IsStringPiece(field, options_))) {
      has_arena_constructor = true;
    }
    if (has_arena_constructor) {
      initializer_with_arena +=
          std::string(",\n  ") + FieldName(field) + std::string("_(arena)");
    }
  }

  if (IsAnyMessage(descriptor_, options_)) {
    initializer_with_arena += ",\n  _any_metadata_(&type_url_, &value_)";
  }
  if (num_weak_fields_ > 0) {
    initializer_with_arena += ", _weak_field_map_(arena)";
  }

  std::string initializer_null = superclass + "()";
  if (IsAnyMessage(descriptor_, options_)) {
    initializer_null += ", _any_metadata_(&type_url_, &value_)";
  }
  if (num_weak_fields_ > 0) {
    initializer_null += ", _weak_field_map_(nullptr)";
  }

  if (SupportsArenas(descriptor_)) {
    format(
        "$classname$::$classname$(::$proto_ns$::Arena* arena)\n"
        "  : $1$ {\n"
        "  SharedCtor();\n"
        "  RegisterArenaDtor(arena);\n"
        "  // @@protoc_insertion_point(arena_constructor:$full_name$)\n"
        "}\n",
        initializer_with_arena);
  } else {
    format(
        "$classname$::$classname$()\n"
        "  : $1$ {\n"
        "  SharedCtor();\n"
        "  // @@protoc_insertion_point(constructor:$full_name$)\n"
        "}\n",
        initializer_null);
  }

  std::map<std::string, std::string> vars;
  SetUnknkownFieldsVariable(descriptor_, options_, &vars);
  format.AddMap(vars);

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
  } else {
    format(
        "$classname$::$classname$(const $classname$& from)\n"
        "  : $superclass$()");
    format.Indent();
    format.Indent();
    format.Indent();

    if (!has_bit_indices_.empty()) {
      format(",\n_has_bits_(from._has_bits_)");
    }

    std::vector<bool> processed(optimized_order_.size(), false);
    for (int i = 0; i < optimized_order_.size(); i++) {
      auto field = optimized_order_[i];
      if (!(field->is_repeated() && !(field->is_map())) &&
          !IsCord(field, options_)) {
        continue;
      }

      processed[i] = true;
      format(",\n$1$_(from.$1$_)", FieldName(field));
    }

    if (IsAnyMessage(descriptor_, options_)) {
      format(",\n_any_metadata_(&type_url_, &value_)");
    }
    if (num_weak_fields_ > 0) {
      format(",\n_weak_field_map_(from._weak_field_map_)");
    }

    format.Outdent();
    format.Outdent();
    format(" {\n");

    format(
        "_internal_metadata_.MergeFrom<$unknown_fields_type$>(from._internal_"
        "metadata_);\n");

    if (descriptor_->extension_range_count() > 0) {
      format("_extensions_.MergeFrom(from._extensions_);\n");
    }

    GenerateConstructorBody(printer, processed, true);

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
        if (IsFieldUsed(field, options_)) {
          field_generators_.get(field).GenerateMergingCode(printer);
        }
        format("break;\n");
        format.Outdent();
        format("}\n");
      }
      format(
          "case $1$_NOT_SET: {\n"
          "  break;\n"
          "}\n",
          ToUpper(oneof->name()));
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
  GenerateSharedConstructorCode(printer);

  // Generate the destructor.
  format(
      "$classname$::~$classname$() {\n"
      "  // @@protoc_insertion_point(destructor:$full_name$)\n"
      "  SharedDtor();\n"
      "  _internal_metadata_.Delete<$unknown_fields_type$>();\n"
      "}\n"
      "\n");

  // Generate the shared destructor code.
  GenerateSharedDestructorCode(printer);

  // Generate the arena-specific destructor code.
  if (SupportsArenas(descriptor_)) {
    GenerateArenaDestructorCode(printer);
  }

  // Generate SetCachedSize.
  format(
      "void $classname$::SetCachedSize(int size) const {\n"
      "  _cached_size_.Set(size);\n"
      "}\n");

  format(
      "const $classname$& $classname$::default_instance() {\n"
      "  "
      "::$proto_ns$::internal::InitSCC(&::$scc_info$.base)"
      ";\n"
      "  return *internal_default_instance();\n"
      "}\n\n");
}

void MessageGenerator::GenerateSourceInProto2Namespace(io::Printer* printer) {
  Formatter format(printer, variables_);
  format(
      "template<> "
      "PROTOBUF_NOINLINE "
      "$classtype$* Arena::CreateMaybeMessage< $classtype$ >(Arena* arena) {\n"
      "  return Arena::$1$Internal< $classtype$ >(arena);\n"
      "}\n",
      MessageCreateFunction(descriptor_));
}

void MessageGenerator::GenerateClear(io::Printer* printer) {
  Formatter format(printer, variables_);

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
    format("_extensions_.Clear();\n");
  }

  // Collect fields into chunks. Each chunk may have an if() condition that
  // checks all hasbits in the chunk and skips it if none are set.
  int zero_init_bytes = 0;
  for (const auto& field : optimized_order_) {
    if (CanInitializeByZeroing(field)) {
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
        bool same = HasByteIndex(a) == HasByteIndex(b) &&
                    a->is_repeated() == b->is_repeated() &&
                    (CanInitializeByZeroing(a) == CanInitializeByZeroing(b) ||
                     (CanInitializeByZeroing(a) &&
                      (chunk_count == 1 || merge_zero_init)));
        if (!same) chunk_count = 0;
        return same;
      });

  ColdChunkSkipper cold_skipper(options_, chunks, has_bit_indices_, kColdRatio);
  int cached_has_word_index = -1;

  for (int chunk_index = 0; chunk_index < chunks.size(); chunk_index++) {
    std::vector<const FieldDescriptor*>& chunk = chunks[chunk_index];
    cold_skipper.OnStartChunk(chunk_index, cached_has_word_index, "", printer);

    const FieldDescriptor* memset_start = nullptr;
    const FieldDescriptor* memset_end = nullptr;
    bool saw_non_zero_init = false;

    for (const auto& field : chunk) {
      if (CanInitializeByZeroing(field)) {
        GOOGLE_CHECK(!saw_non_zero_init);
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
      uint32 chunk_mask = GenChunkMask(chunk, has_bit_indices_);
      std::string chunk_mask_str =
          StrCat(strings::Hex(chunk_mask, strings::ZERO_PAD_8));

      // Check (up to) 8 has_bits at a time if we have more than one field in
      // this chunk.  Due to field layout ordering, we may check
      // _has_bits_[last_chunk * 8 / 32] multiple times.
      GOOGLE_DCHECK_LE(2, popcnt(chunk_mask));
      GOOGLE_DCHECK_GE(8, popcnt(chunk_mask));

      if (cached_has_word_index != HasWordIndex(chunk.front())) {
        cached_has_word_index = HasWordIndex(chunk.front());
        format("cached_has_bits = _has_bits_[$1$];\n", cached_has_word_index);
      }
      format("if (cached_has_bits & 0x$1$u) {\n", chunk_mask_str);
      format.Indent();
    }

    if (memset_start) {
      if (memset_start == memset_end) {
        // For clarity, do not memset a single field.
        field_generators_.get(memset_start)
            .GenerateMessageClearingCode(printer);
      } else {
        format(
            "::memset(&$1$_, 0, static_cast<size_t>(\n"
            "    reinterpret_cast<char*>(&$2$_) -\n"
            "    reinterpret_cast<char*>(&$1$_)) + sizeof($2$_));\n",
            FieldName(memset_start), FieldName(memset_end));
      }
    }

    // Clear all non-zero-initializable fields in the chunk.
    for (const auto& field : chunk) {
      if (CanInitializeByZeroing(field)) continue;
      // It's faster to just overwrite primitive types, but we should only
      // clear strings and messages if they were set.
      //
      // TODO(kenton):  Let the CppFieldGenerator decide this somehow.
      bool have_enclosing_if =
          HasBitIndex(field) != kNoHasbit &&
          (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ||
           field->cpp_type() == FieldDescriptor::CPPTYPE_STRING);

      if (have_enclosing_if) {
        PrintPresenceCheck(format, field, has_bit_indices_, printer,
                           &cached_has_word_index);
      }

      field_generators_.get(field).GenerateMessageClearingCode(printer);

      if (have_enclosing_if) {
        format.Outdent();
        format("}\n");
      }
    }

    if (have_outer_if) {
      format.Outdent();
      format("}\n");
    }

    if (cold_skipper.OnEndChunk(chunk_index, printer)) {
      // Reset here as it may have been updated in just closed if statement.
      cached_has_word_index = -1;
    }
  }

  // Step 4: Unions.
  for (auto oneof : OneOfRange(descriptor_)) {
    format("clear_$1$();\n", oneof->name());
  }

  if (num_weak_fields_) {
    format("_weak_field_map_.ClearAll();\n");
  }

  if (!has_bit_indices_.empty()) {
    // Step 5: Everything else.
    format("_has_bits_.Clear();\n");
  }

  std::map<std::string, std::string> vars;
  SetUnknkownFieldsVariable(descriptor_, options_, &vars);
  format.AddMap(vars);
  format("_internal_metadata_.Clear<$unknown_fields_type$>();\n");

  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateOneofClear(io::Printer* printer) {
  // Generated function clears the active field and union case (e.g. foo_case_).
  int i = 0;
  for (auto oneof : OneOfRange(descriptor_)) {
    Formatter format(printer, variables_);
    format.Set("oneofname", oneof->name());

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
      if (!IsStringOrMessage(field) || !IsFieldUsed(field, options_)) {
        format("// No need to clear\n");
      } else {
        field_generators_.get(field).GenerateClearingCode(printer);
      }
      format("break;\n");
      format.Outdent();
      format("}\n");
    }
    format(
        "case $1$_NOT_SET: {\n"
        "  break;\n"
        "}\n",
        ToUpper(oneof->name()));
    format.Outdent();
    format(
        "}\n"
        "_oneof_case_[$1$] = $2$_NOT_SET;\n",
        i, ToUpper(oneof->name()));
    format.Outdent();
    format(
        "}\n"
        "\n");
    i++;
  }
}

void MessageGenerator::GenerateSwap(io::Printer* printer) {
  Formatter format(printer, variables_);

  format("void $classname$::InternalSwap($classname$* other) {\n");
  format.Indent();
  format("using std::swap;\n");

  if (HasGeneratedMethods(descriptor_->file(), options_)) {
    if (descriptor_->extension_range_count() > 0) {
      format("_extensions_.Swap(&other->_extensions_);\n");
    }

    std::map<std::string, std::string> vars;
    SetUnknkownFieldsVariable(descriptor_, options_, &vars);
    format.AddMap(vars);
    format(
        "_internal_metadata_.Swap<$unknown_fields_type$>(&other->_internal_"
        "metadata_);\n");

    if (!has_bit_indices_.empty()) {
      for (int i = 0; i < HasBitsSize() / 4; ++i) {
        format("swap(_has_bits_[$1$], other->_has_bits_[$1$]);\n", i);
      }
    }

    // If possible, we swap several fields at once, including padding.
    const RunMap runs =
        FindRuns(optimized_order_, [this](const FieldDescriptor* field) {
          return CanBeManipulatedAsRawBytes(field, options_);
        });

    for (int i = 0; i < optimized_order_.size(); ++i) {
      const FieldDescriptor* field = optimized_order_[i];
      const auto it = runs.find(field);

      // We only apply the memswap technique to runs of more than one field, as
      // `swap(field_, other.field_)` is better than
      // `memswap<...>(&field_, &other.field_)` for generated code readability.
      if (it != runs.end() && it->second > 1) {
        // Use a memswap, then skip run_length fields.
        const size_t run_length = it->second;
        const std::string first_field_name = FieldName(field);
        const std::string last_field_name =
            FieldName(optimized_order_[i + run_length - 1]);

        format.Set("first", first_field_name);
        format.Set("last", last_field_name);

        format(
            "::PROTOBUF_NAMESPACE_ID::internal::memswap<\n"
            "    PROTOBUF_FIELD_OFFSET($classname$, $last$_)\n"
            "    + sizeof($classname$::$last$_)\n"
            "    - PROTOBUF_FIELD_OFFSET($classname$, $first$_)>(\n"
            "        reinterpret_cast<char*>(&$first$_),\n"
            "        reinterpret_cast<char*>(&other->$first$_));\n");

        i += run_length - 1;
        // ++i at the top of the loop.
      } else {
        field_generators_.get(field).GenerateSwappingCode(printer);
      }
    }

    for (auto oneof : OneOfRange(descriptor_)) {
      format("swap($1$_, other->$1$_);\n", oneof->name());
    }

    for (int i = 0; i < descriptor_->real_oneof_decl_count(); i++) {
      format("swap(_oneof_case_[$1$], other->_oneof_case_[$1$]);\n", i);
    }

    if (num_weak_fields_) {
      format("_weak_field_map_.UnsafeArenaSwap(&other->_weak_field_map_);\n");
    }
  } else {
    format("GetReflection()->Swap(this, other);");
  }

  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateMergeFrom(io::Printer* printer) {
  Formatter format(printer, variables_);
  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    // Generate the generalized MergeFrom (aka that which takes in the Message
    // base class as a parameter).
    format(
        "void $classname$::MergeFrom(const ::$proto_ns$::Message& from) {\n"
        "// @@protoc_insertion_point(generalized_merge_from_start:"
        "$full_name$)\n"
        "  $DCHK$_NE(&from, this);\n");
    format.Indent();

    // Cast the message to the proper type. If we find that the message is
    // *not* of the proper type, we can still call Merge via the reflection
    // system, as the GOOGLE_CHECK above ensured that we have the same descriptor
    // for each message.
    format(
        "const $classname$* source =\n"
        "    ::$proto_ns$::DynamicCastToGenerated<$classname$>(\n"
        "        &from);\n"
        "if (source == nullptr) {\n"
        "// @@protoc_insertion_point(generalized_merge_from_cast_fail:"
        "$full_name$)\n"
        "  ::$proto_ns$::internal::ReflectionOps::Merge(from, this);\n"
        "} else {\n"
        "// @@protoc_insertion_point(generalized_merge_from_cast_success:"
        "$full_name$)\n"
        "  MergeFrom(*source);\n"
        "}\n");

    format.Outdent();
    format("}\n");
  } else {
    // Generate CheckTypeAndMergeFrom().
    format(
        "void $classname$::CheckTypeAndMergeFrom(\n"
        "    const ::$proto_ns$::MessageLite& from) {\n"
        "  MergeFrom(*::$proto_ns$::internal::DownCast<const $classname$*>(\n"
        "      &from));\n"
        "}\n");
  }
}

void MessageGenerator::GenerateClassSpecificMergeFrom(io::Printer* printer) {
  // Generate the class-specific MergeFrom, which avoids the GOOGLE_CHECK and cast.
  Formatter format(printer, variables_);
  format(
      "void $classname$::MergeFrom(const $classname$& from) {\n"
      "// @@protoc_insertion_point(class_specific_merge_from_start:"
      "$full_name$)\n"
      "  $DCHK$_NE(&from, this);\n");
  format.Indent();

  if (descriptor_->extension_range_count() > 0) {
    format("_extensions_.MergeFrom(from._extensions_);\n");
  }
  std::map<std::string, std::string> vars;
  SetUnknkownFieldsVariable(descriptor_, options_, &vars);
  format.AddMap(vars);
  format(
      "_internal_metadata_.MergeFrom<$unknown_fields_type$>(from._internal_"
      "metadata_);\n"
      "$uint32$ cached_has_bits = 0;\n"
      "(void) cached_has_bits;\n\n");

  std::vector<std::vector<const FieldDescriptor*>> chunks = CollectFields(
      optimized_order_,
      [&](const FieldDescriptor* a, const FieldDescriptor* b) -> bool {
        return HasByteIndex(a) == HasByteIndex(b);
      });

  ColdChunkSkipper cold_skipper(options_, chunks, has_bit_indices_, kColdRatio);

  // cached_has_word_index maintains that:
  //   cached_has_bits = from._has_bits_[cached_has_word_index]
  // for cached_has_word_index >= 0
  int cached_has_word_index = -1;

  for (int chunk_index = 0; chunk_index < chunks.size(); chunk_index++) {
    const std::vector<const FieldDescriptor*>& chunk = chunks[chunk_index];
    bool have_outer_if =
        chunk.size() > 1 && HasByteIndex(chunk.front()) != kNoHasbit;
    cold_skipper.OnStartChunk(chunk_index, cached_has_word_index, "from.",
                              printer);

    if (have_outer_if) {
      // Emit an if() that will let us skip the whole chunk if none are set.
      uint32 chunk_mask = GenChunkMask(chunk, has_bit_indices_);
      std::string chunk_mask_str =
          StrCat(strings::Hex(chunk_mask, strings::ZERO_PAD_8));

      // Check (up to) 8 has_bits at a time if we have more than one field in
      // this chunk.  Due to field layout ordering, we may check
      // _has_bits_[last_chunk * 8 / 32] multiple times.
      GOOGLE_DCHECK_LE(2, popcnt(chunk_mask));
      GOOGLE_DCHECK_GE(8, popcnt(chunk_mask));

      if (cached_has_word_index != HasWordIndex(chunk.front())) {
        cached_has_word_index = HasWordIndex(chunk.front());
        format("cached_has_bits = from._has_bits_[$1$];\n",
               cached_has_word_index);
      }

      format("if (cached_has_bits & 0x$1$u) {\n", chunk_mask_str);
      format.Indent();
    }

    // Go back and emit merging code for each of the fields we processed.
    bool deferred_has_bit_changes = false;
    for (const auto field : chunk) {
      const FieldGenerator& generator = field_generators_.get(field);

      if (field->is_repeated()) {
        generator.GenerateMergingCode(printer);
      } else if (field->is_optional() && !HasHasbit(field)) {
        // Merge semantics without true field presence: primitive fields are
        // merged only if non-zero (numeric) or non-empty (string).
        bool have_enclosing_if =
            EmitFieldNonDefaultCondition(printer, "from.", field);
        generator.GenerateMergingCode(printer);
        if (have_enclosing_if) {
          format.Outdent();
          format("}\n");
        }
      } else if (field->options().weak() ||
                 cached_has_word_index != HasWordIndex(field)) {
        // Check hasbit, not using cached bits.
        GOOGLE_CHECK(HasHasbit(field));
        format("if (from._internal_has_$1$()) {\n", FieldName(field));
        format.Indent();
        generator.GenerateMergingCode(printer);
        format.Outdent();
        format("}\n");
      } else {
        // Check hasbit, using cached bits.
        GOOGLE_CHECK(HasHasbit(field));
        int has_bit_index = has_bit_indices_[field->index()];
        const std::string mask = StrCat(
            strings::Hex(1u << (has_bit_index % 32), strings::ZERO_PAD_8));
        format("if (cached_has_bits & 0x$1$u) {\n", mask);
        format.Indent();

        if (have_outer_if && IsPOD(field)) {
          // Defer hasbit modification until the end of chunk.
          // This can reduce the number of loads/stores by up to 7 per 8 fields.
          deferred_has_bit_changes = true;
          generator.GenerateCopyConstructorCode(printer);
        } else {
          generator.GenerateMergingCode(printer);
        }

        format.Outdent();
        format("}\n");
      }
    }

    if (have_outer_if) {
      if (deferred_has_bit_changes) {
        // Flush the has bits for the primitives we deferred.
        GOOGLE_CHECK_LE(0, cached_has_word_index);
        format("_has_bits_[$1$] |= cached_has_bits;\n", cached_has_word_index);
      }

      format.Outdent();
      format("}\n");
    }

    if (cold_skipper.OnEndChunk(chunk_index, printer)) {
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
      if (IsFieldUsed(field, options_)) {
        field_generators_.get(field).GenerateMergingCode(printer);
      }
      format("break;\n");
      format.Outdent();
      format("}\n");
    }
    format(
        "case $1$_NOT_SET: {\n"
        "  break;\n"
        "}\n",
        ToUpper(oneof->name()));
    format.Outdent();
    format("}\n");
  }
  if (num_weak_fields_) {
    format("_weak_field_map_.MergeFrom(from._weak_field_map_);\n");
  }

  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateCopyFrom(io::Printer* printer) {
  Formatter format(printer, variables_);
  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    // Generate the generalized CopyFrom (aka that which takes in the Message
    // base class as a parameter).
    format(
        "void $classname$::CopyFrom(const ::$proto_ns$::Message& from) {\n"
        "// @@protoc_insertion_point(generalized_copy_from_start:"
        "$full_name$)\n");
    format.Indent();

    format("if (&from == this) return;\n");

    if (!options_.opensource_runtime) {
      // This check is disabled in the opensource release because we're
      // concerned that many users do not define NDEBUG in their release
      // builds.
      format(
          "#ifndef NDEBUG\n"
          "size_t from_size = from.ByteSizeLong();\n"
          "#endif\n"
          "Clear();\n"
          "#ifndef NDEBUG\n"
          "$CHK$_EQ(from_size, from.ByteSizeLong())\n"
          "  << \"Source of CopyFrom changed when clearing target.  Either \"\n"
          "  << \"source is a nested message in target (not allowed), or \"\n"
          "  << \"another thread is modifying the source.\";\n"
          "#endif\n");
    } else {
      format("Clear();\n");
    }
    format("MergeFrom(from);\n");

    format.Outdent();
    format("}\n\n");
  }

  // Generate the class-specific CopyFrom.
  format(
      "void $classname$::CopyFrom(const $classname$& from) {\n"
      "// @@protoc_insertion_point(class_specific_copy_from_start:"
      "$full_name$)\n");
  format.Indent();

  format("if (&from == this) return;\n");

  if (!options_.opensource_runtime) {
    // This check is disabled in the opensource release because we're
    // concerned that many users do not define NDEBUG in their release builds.
    format(
        "#ifndef NDEBUG\n"
        "size_t from_size = from.ByteSizeLong();\n"
        "#endif\n"
        "Clear();\n"
        "#ifndef NDEBUG\n"
        "$CHK$_EQ(from_size, from.ByteSizeLong())\n"
        "  << \"Source of CopyFrom changed when clearing target.  Either \"\n"
        "  << \"source is a nested message in target (not allowed), or \"\n"
        "  << \"another thread is modifying the source.\";\n"
        "#endif\n");
  } else {
    format("Clear();\n");
  }
  format("MergeFrom(from);\n");

  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateMergeFromCodedStream(io::Printer* printer) {
  std::map<std::string, std::string> vars = variables_;
  SetUnknkownFieldsVariable(descriptor_, options_, &vars);
  Formatter format(printer, vars);
  if (descriptor_->options().message_set_wire_format()) {
    // Special-case MessageSet.
    format(
        "const char* $classname$::_InternalParse(const char* ptr,\n"
        "                  ::$proto_ns$::internal::ParseContext* ctx) {\n"
        "  return _extensions_.ParseMessageSet(ptr, \n"
        "      internal_default_instance(), &_internal_metadata_, ctx);\n"
        "}\n");
    return;
  }
  GenerateParserLoop(descriptor_, max_has_bit_index_, options_, scc_analyzer_,
                     printer);
}

void MessageGenerator::GenerateSerializeOneofFields(
    io::Printer* printer, const std::vector<const FieldDescriptor*>& fields) {
  Formatter format(printer, variables_);
  GOOGLE_CHECK(!fields.empty());
  if (fields.size() == 1) {
    GenerateSerializeOneField(printer, fields[0], -1);
    return;
  }
  // We have multiple mutually exclusive choices.  Emit a switch statement.
  const OneofDescriptor* oneof = fields[0]->containing_oneof();
  format("switch ($1$_case()) {\n", oneof->name());
  format.Indent();
  for (auto field : fields) {
    format("case k$1$: {\n", UnderscoresToCamelCase(field->name(), true));
    format.Indent();
    field_generators_.get(field).GenerateSerializeWithCachedSizesToArray(
        printer);
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

void MessageGenerator::GenerateSerializeOneField(io::Printer* printer,
                                                 const FieldDescriptor* field,
                                                 int cached_has_bits_index) {
  Formatter format(printer, variables_);
  if (!field->options().weak()) {
    // For weakfields, PrintFieldComment is called during iteration.
    PrintFieldComment(format, field);
  }

  bool have_enclosing_if = false;
  if (field->options().weak()) {
  } else if (HasHasbit(field)) {
    // Attempt to use the state of cached_has_bits, if possible.
    int has_bit_index = HasBitIndex(field);
    if (cached_has_bits_index == has_bit_index / 32) {
      const std::string mask =
          StrCat(strings::Hex(1u << (has_bit_index % 32), strings::ZERO_PAD_8));

      format("if (cached_has_bits & 0x$1$u) {\n", mask);
    } else {
      format("if (_internal_has_$1$()) {\n", FieldName(field));
    }

    format.Indent();
    have_enclosing_if = true;
  } else if (field->is_optional() && !HasHasbit(field)) {
    have_enclosing_if = EmitFieldNonDefaultCondition(printer, "this->", field);
  }

  field_generators_.get(field).GenerateSerializeWithCachedSizesToArray(printer);

  if (have_enclosing_if) {
    format.Outdent();
    format("}\n");
  }
  format("\n");
}

void MessageGenerator::GenerateSerializeOneExtensionRange(
    io::Printer* printer, const Descriptor::ExtensionRange* range) {
  std::map<std::string, std::string> vars = variables_;
  vars["start"] = StrCat(range->start);
  vars["end"] = StrCat(range->end);
  Formatter format(printer, vars);
  format("// Extension range [$start$, $end$)\n");
  format(
      "target = _extensions_._InternalSerialize(\n"
      "    $start$, $end$, target, stream);\n\n");
}

void MessageGenerator::GenerateSerializeWithCachedSizesToArray(
    io::Printer* printer) {
  Formatter format(printer, variables_);
  if (descriptor_->options().message_set_wire_format()) {
    // Special-case MessageSet.
    format(
        "$uint8$* $classname$::_InternalSerialize(\n"
        "    $uint8$* target, ::$proto_ns$::io::EpsCopyOutputStream* stream) "
        "const {\n"
        "  target = _extensions_."
        "InternalSerializeMessageSetWithCachedSizesToArray(target, stream);\n");
    std::map<std::string, std::string> vars;
    SetUnknkownFieldsVariable(descriptor_, options_, &vars);
    format.AddMap(vars);
    format(
        "  target = ::$proto_ns$::internal::"
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
      "const {\n");
  format.Indent();

  format("// @@protoc_insertion_point(serialize_to_array_start:$full_name$)\n");

  GenerateSerializeWithCachedSizesBody(printer);

  format("// @@protoc_insertion_point(serialize_to_array_end:$full_name$)\n");

  format.Outdent();
  format(
      "  return target;\n"
      "}\n");
}

void MessageGenerator::GenerateSerializeWithCachedSizesBody(
    io::Printer* printer) {
  Formatter format(printer, variables_);
  // If there are multiple fields in a row from the same oneof then we
  // coalesce them and emit a switch statement.  This is more efficient
  // because it lets the C++ compiler know this is a "at most one can happen"
  // situation. If we emitted "if (has_x()) ...; if (has_y()) ..." the C++
  // compiler's emitted code might check has_y() even when has_x() is true.
  class LazySerializerEmitter {
   public:
    LazySerializerEmitter(MessageGenerator* mg, io::Printer* printer)
        : mg_(mg),
          format_(printer),
          eager_(!HasFieldPresence(mg->descriptor_->file())),
          cached_has_bit_index_(kNoHasbit) {}

    ~LazySerializerEmitter() { Flush(); }

    // If conditions allow, try to accumulate a run of fields from the same
    // oneof, and handle them at the next Flush().
    void Emit(const FieldDescriptor* field) {
      if (eager_ || MustFlush(field)) {
        Flush();
      }
      if (!field->real_containing_oneof()) {
        // TODO(ckennelly): Defer non-oneof fields similarly to oneof fields.

        if (!field->options().weak() && !field->is_repeated() && !eager_) {
          // We speculatively load the entire _has_bits_[index] contents, even
          // if it is for only one field.  Deferring non-oneof emitting would
          // allow us to determine whether this is going to be useful.
          int has_bit_index = mg_->has_bit_indices_[field->index()];
          if (cached_has_bit_index_ != has_bit_index / 32) {
            // Reload.
            int new_index = has_bit_index / 32;

            format_("cached_has_bits = _has_bits_[$1$];\n", new_index);

            cached_has_bit_index_ = new_index;
          }
        }

        mg_->GenerateSerializeOneField(format_.printer(), field,
                                       cached_has_bit_index_);
      } else {
        v_.push_back(field);
      }
    }

    void Flush() {
      if (!v_.empty()) {
        mg_->GenerateSerializeOneofFields(format_.printer(), v_);
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
    Formatter format_;
    const bool eager_;
    std::vector<const FieldDescriptor*> v_;

    // cached_has_bit_index_ maintains that:
    //   cached_has_bits = from._has_bits_[cached_has_bit_index_]
    // for cached_has_bit_index_ >= 0
    int cached_has_bit_index_;
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
        "::$proto_ns$::internal::WeakFieldMap::FieldWriter field_writer("
        "_weak_field_map_);\n");
  }

  format(
      "$uint32$ cached_has_bits = 0;\n"
      "(void) cached_has_bits;\n\n");

  // Merge the fields and the extension ranges, both sorted by field number.
  {
    LazySerializerEmitter e(this, printer);
    const FieldDescriptor* last_weak_field = nullptr;
    int i, j;
    for (i = 0, j = 0;
         i < ordered_fields.size() || j < sorted_extensions.size();) {
      if ((j == sorted_extensions.size()) ||
          (i < descriptor_->field_count() &&
           ordered_fields[i]->number() < sorted_extensions[j]->start)) {
        const FieldDescriptor* field = ordered_fields[i++];
        if (!IsFieldUsed(field, options_)) {
          continue;
        }
        if (field->options().weak()) {
          last_weak_field = field;
          PrintFieldComment(format, field);
        } else {
          if (last_weak_field != nullptr) {
            e.Emit(last_weak_field);
            last_weak_field = nullptr;
          }
          e.Emit(field);
        }
      } else {
        if (last_weak_field != nullptr) {
          e.Emit(last_weak_field);
          last_weak_field = nullptr;
        }
        e.Flush();
        GenerateSerializeOneExtensionRange(printer, sorted_extensions[j++]);
      }
    }
    if (last_weak_field != nullptr) {
      e.Emit(last_weak_field);
    }
  }

  std::map<std::string, std::string> vars;
  SetUnknkownFieldsVariable(descriptor_, options_, &vars);
  format.AddMap(vars);
  format("if (PROTOBUF_PREDICT_FALSE($have_unknown_fields$)) {\n");
  format.Indent();
  if (UseUnknownFieldSet(descriptor_->file(), options_)) {
    format(
        "target = "
        "::$proto_ns$::internal::WireFormat::"
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

std::vector<uint32> MessageGenerator::RequiredFieldsBitMask() const {
  const int array_size = HasBitsSize();
  std::vector<uint32> masks(array_size, 0);

  for (auto field : FieldRange(descriptor_)) {
    if (!field->is_required()) {
      continue;
    }

    const int has_bit_index = has_bit_indices_[field->index()];
    masks[has_bit_index / 32] |= static_cast<uint32>(1) << (has_bit_index % 32);
  }
  return masks;
}

void MessageGenerator::GenerateByteSize(io::Printer* printer) {
  Formatter format(printer, variables_);

  if (descriptor_->options().message_set_wire_format()) {
    // Special-case MessageSet.
    std::map<std::string, std::string> vars;
    SetUnknkownFieldsVariable(descriptor_, options_, &vars);
    format.AddMap(vars);
    format(
        "size_t $classname$::ByteSizeLong() const {\n"
        "// @@protoc_insertion_point(message_set_byte_size_start:$full_name$)\n"
        "  size_t total_size = _extensions_.MessageSetByteSize();\n"
        "  if ($have_unknown_fields$) {\n"
        "    total_size += ::$proto_ns$::internal::\n"
        "        ComputeUnknownMessageSetItemsSize($unknown_fields$);\n"
        "  }\n"
        "  int cached_size = "
        "::$proto_ns$::internal::ToCachedSize(total_size);\n"
        "  SetCachedSize(cached_size);\n"
        "  return total_size;\n"
        "}\n");
    return;
  }

  if (num_required_fields_ > 1) {
    // Emit a function (rarely used, we hope) that handles the required fields
    // by checking for each one individually.
    format(
        "size_t $classname$::RequiredFieldsByteSizeFallback() const {\n"
        "// @@protoc_insertion_point(required_fields_byte_size_fallback_start:"
        "$full_name$)\n");
    format.Indent();
    format("size_t total_size = 0;\n");
    for (auto field : optimized_order_) {
      if (field->is_required()) {
        format(
            "\n"
            "if (_internal_has_$1$()) {\n",
            FieldName(field));
        format.Indent();
        PrintFieldComment(format, field);
        field_generators_.get(field).GenerateByteSize(printer);
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
      "size_t $classname$::ByteSizeLong() const {\n"
      "// @@protoc_insertion_point(message_byte_size_start:$full_name$)\n");
  format.Indent();
  format(
      "size_t total_size = 0;\n"
      "\n");

  if (descriptor_->extension_range_count() > 0) {
    format(
        "total_size += _extensions_.ByteSize();\n"
        "\n");
  }

  std::map<std::string, std::string> vars;
  SetUnknkownFieldsVariable(descriptor_, options_, &vars);
  format.AddMap(vars);

  // Handle required fields (if any).  We expect all of them to be
  // present, so emit one conditional that checks for that.  If they are all
  // present then the fast path executes; otherwise the slow path executes.
  if (num_required_fields_ > 1) {
    // The fast path works if all required fields are present.
    const std::vector<uint32> masks_for_has_bits = RequiredFieldsBitMask();
    format("if ($1$) {  // All required fields are present.\n",
           ConditionalToCheckBitmasks(masks_for_has_bits));
    format.Indent();
    // Oneof fields cannot be required, so optimized_order_ contains all of the
    // fields that we need to potentially emit.
    for (auto field : optimized_order_) {
      if (!field->is_required()) continue;
      PrintFieldComment(format, field);
      field_generators_.get(field).GenerateByteSize(printer);
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
      format("if (_internal_has_$1$()) {\n", FieldName(field));
      format.Indent();
      field_generators_.get(field).GenerateByteSize(printer);
      format.Outdent();
      format("}\n");
    }
  }

  std::vector<std::vector<const FieldDescriptor*>> chunks = CollectFields(
      optimized_order_,
      [&](const FieldDescriptor* a, const FieldDescriptor* b) -> bool {
        return a->label() == b->label() && HasByteIndex(a) == HasByteIndex(b);
      });

  // Remove chunks with required fields.
  chunks.erase(std::remove_if(chunks.begin(), chunks.end(), IsRequired),
               chunks.end());

  ColdChunkSkipper cold_skipper(options_, chunks, has_bit_indices_, kColdRatio);
  int cached_has_word_index = -1;

  format(
      "$uint32$ cached_has_bits = 0;\n"
      "// Prevent compiler warnings about cached_has_bits being unused\n"
      "(void) cached_has_bits;\n\n");

  for (int chunk_index = 0; chunk_index < chunks.size(); chunk_index++) {
    const std::vector<const FieldDescriptor*>& chunk = chunks[chunk_index];
    const bool have_outer_if =
        chunk.size() > 1 && HasWordIndex(chunk[0]) != kNoHasbit;
    cold_skipper.OnStartChunk(chunk_index, cached_has_word_index, "", printer);

    if (have_outer_if) {
      // Emit an if() that will let us skip the whole chunk if none are set.
      uint32 chunk_mask = GenChunkMask(chunk, has_bit_indices_);
      std::string chunk_mask_str =
          StrCat(strings::Hex(chunk_mask, strings::ZERO_PAD_8));

      // Check (up to) 8 has_bits at a time if we have more than one field in
      // this chunk.  Due to field layout ordering, we may check
      // _has_bits_[last_chunk * 8 / 32] multiple times.
      GOOGLE_DCHECK_LE(2, popcnt(chunk_mask));
      GOOGLE_DCHECK_GE(8, popcnt(chunk_mask));

      if (cached_has_word_index != HasWordIndex(chunk.front())) {
        cached_has_word_index = HasWordIndex(chunk.front());
        format("cached_has_bits = _has_bits_[$1$];\n", cached_has_word_index);
      }
      format("if (cached_has_bits & 0x$1$u) {\n", chunk_mask_str);
      format.Indent();
    }

    // Go back and emit checks for each of the fields we processed.
    for (int j = 0; j < chunk.size(); j++) {
      const FieldDescriptor* field = chunk[j];
      const FieldGenerator& generator = field_generators_.get(field);
      bool have_enclosing_if = false;
      bool need_extra_newline = false;

      PrintFieldComment(format, field);

      if (field->is_repeated()) {
        // No presence check is required.
        need_extra_newline = true;
      } else if (HasHasbit(field)) {
        PrintPresenceCheck(format, field, has_bit_indices_, printer,
                           &cached_has_word_index);
        have_enclosing_if = true;
      } else {
        // Without field presence: field is serialized only if it has a
        // non-default value.
        have_enclosing_if =
            EmitFieldNonDefaultCondition(printer, "this->", field);
      }

      generator.GenerateByteSize(printer);

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

    if (cold_skipper.OnEndChunk(chunk_index, printer)) {
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
      if (IsFieldUsed(field, options_)) {
        field_generators_.get(field).GenerateByteSize(printer);
      }
      format("break;\n");
      format.Outdent();
      format("}\n");
    }
    format(
        "case $1$_NOT_SET: {\n"
        "  break;\n"
        "}\n",
        ToUpper(oneof->name()));
    format.Outdent();
    format("}\n");
  }

  if (num_weak_fields_) {
    // TagSize + MessageSize
    format("total_size += _weak_field_map_.ByteSizeLong();\n");
  }

  format("if (PROTOBUF_PREDICT_FALSE($have_unknown_fields$)) {\n");
  if (UseUnknownFieldSet(descriptor_->file(), options_)) {
    // We go out of our way to put the computation of the uncommon path of
    // unknown fields in tail position. This allows for better code generation
    // of this function for simple protos.
    format(
        "  return ::$proto_ns$::internal::ComputeUnknownFieldsSize(\n"
        "      _internal_metadata_, total_size, &_cached_size_);\n");
  } else {
    format("  total_size += $unknown_fields$.size();\n");
  }
  format("}\n");

  // We update _cached_size_ even though this is a const method.  Because
  // const methods might be called concurrently this needs to be atomic
  // operations or the program is undefined.  In practice, since any concurrent
  // writes will be writing the exact same value, normal writes will work on
  // all common processors. We use a dedicated wrapper class to abstract away
  // the underlying atomic. This makes it easier on platforms where even relaxed
  // memory order might have perf impact to replace it with ordinary loads and
  // stores.
  format(
      "int cached_size = ::$proto_ns$::internal::ToCachedSize(total_size);\n"
      "SetCachedSize(cached_size);\n"
      "return total_size;\n");

  format.Outdent();
  format("}\n");
}

void MessageGenerator::GenerateIsInitialized(io::Printer* printer) {
  Formatter format(printer, variables_);
  format("bool $classname$::IsInitialized() const {\n");
  format.Indent();

  if (descriptor_->extension_range_count() > 0) {
    format(
        "if (!_extensions_.IsInitialized()) {\n"
        "  return false;\n"
        "}\n\n");
  }

  if (num_required_fields_ > 0) {
    format(
        "if (_Internal::MissingRequiredFields(_has_bits_))"
        " return false;\n");
  }

  // Now check that all non-oneof embedded messages are initialized.
  for (auto field : optimized_order_) {
    // TODO(ckennelly): Push this down into a generator?
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
        !ShouldIgnoreRequiredFieldCheck(field, options_) &&
        scc_analyzer_->HasRequiredFields(field->message_type())) {
      if (field->is_repeated()) {
        if (IsImplicitWeakField(field, options_, scc_analyzer_)) {
          format(
              "if "
              "(!::$proto_ns$::internal::AllAreInitializedWeak($1$_.weak)"
              ")"
              " return false;\n",
              FieldName(field));
        } else {
          format(
              "if (!::$proto_ns$::internal::AllAreInitialized($1$_))"
              " return false;\n",
              FieldName(field));
        }
      } else if (field->options().weak()) {
        continue;
      } else {
        GOOGLE_CHECK(!field->real_containing_oneof());
        format(
            "if (_internal_has_$1$()) {\n"
            "  if (!$1$_->IsInitialized()) return false;\n"
            "}\n",
            FieldName(field));
      }
    }
  }
  if (num_weak_fields_) {
    // For Weak fields.
    format("if (!_weak_field_map_.IsInitialized()) return false;\n");
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

      if (IsFieldUsed(field, options_) &&
          field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
          !ShouldIgnoreRequiredFieldCheck(field, options_) &&
          scc_analyzer_->HasRequiredFields(field->message_type())) {
        GOOGLE_CHECK(!(field->options().weak() || !field->real_containing_oneof()));
        if (field->options().weak()) {
          // Just skip.
        } else {
          format(
              "if (has_$1$()) {\n"
              "  if (!this->$1$().IsInitialized()) return false;\n"
              "}\n",
              FieldName(field));
        }
      }

      format("break;\n");
      format.Outdent();
      format("}\n");
    }
    format(
        "case $1$_NOT_SET: {\n"
        "  break;\n"
        "}\n",
        ToUpper(oneof->name()));
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
