// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/cpp/message.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/functional/any_invocable.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/cpp/enum.h"
#include "google/protobuf/compiler/cpp/extension.h"
#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/field_chunk.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/compiler/cpp/padding_optimizer.h"
#include "google/protobuf/compiler/cpp/parse_function_generator.h"
#include "google/protobuf/compiler/cpp/tracker.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/wire_format_lite.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
using ::google::protobuf::internal::WireFormat;
using ::google::protobuf::internal::WireFormatLite;
using ::google::protobuf::internal::cpp::GetFieldHasbitMode;
using ::google::protobuf::internal::cpp::HasbitMode;
using ::google::protobuf::internal::cpp::HasHasbit;
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
  for (size_t i = 0; i < masks.size(); ++i) {
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

template <typename PredicateT>
void DebugAssertUniform(const std::vector<const FieldDescriptor*>& fields,
                        const Options& options, PredicateT&& pred) {
  ABSL_DCHECK(!fields.empty() && absl::c_all_of(fields, [&](const auto* f) {
    return pred(f) == pred(fields.front());
  }));
}

void DebugAssertUniformLikelyPresence(
    const std::vector<const FieldDescriptor*>& fields, const Options& options) {
  DebugAssertUniform(fields, options, [&](const FieldDescriptor* f) {
    return IsLikelyPresent(f, options);
  });
}

// Generates a condition that checks presence of a field. If probability is
// provided, the condition will be wrapped with
// PROTOBUF_EXPECT_TRUE_WITH_PROBABILITY.
//
// If use_cached_has_bits is true, the condition will be generated based on
// cached_has_bits. Otherwise, the condition will be generated based on the
// _has_bits_ array, with has_array_index indicating which element of the array
// to use.
std::string GenerateConditionMaybeWithProbability(
    uint32_t mask, std::optional<float> probability, bool use_cached_has_bits,
    std::optional<int> has_array_index) {
  std::string condition;
  if (use_cached_has_bits) {
    condition = absl::StrFormat("(cached_has_bits & 0x%08xU) != 0", mask);
  } else {
    // We only use has_array_index when use_cached_has_bits is false, make sure
    // we pas a valid index when we need it.
    ABSL_DCHECK(has_array_index.has_value());
    condition = absl::StrFormat("(this_._impl_._has_bits_[%d] & 0x%08xU) != 0",
                                *has_array_index, mask);
  }
  if (probability.has_value()) {
    return absl::StrFormat("PROTOBUF_EXPECT_TRUE_WITH_PROBABILITY(%s, %.3f)",
                           condition, *probability);
  }
  return condition;
}

std::string GenerateConditionMaybeWithProbabilityForField(
    int has_bit_index, const FieldDescriptor* field, const Options& options) {
  auto prob = GetPresenceProbability(field, options);
  return GenerateConditionMaybeWithProbability(
      1u << (has_bit_index % 32), prob,
      /*use_cached_has_bits*/ true,
      /*has_array_index*/ std::nullopt);
}

std::string GenerateConditionMaybeWithProbabilityForGroup(
    uint32_t mask, const std::vector<const FieldDescriptor*>& fields,
    const Options& options) {
  auto prob = GetFieldGroupPresenceProbability(fields, options);
  return GenerateConditionMaybeWithProbability(
      mask, prob,
      /*use_cached_has_bits*/ true,
      /*has_array_index*/ std::nullopt);
}

void PrintPresenceCheck(const FieldDescriptor* field,
                        const std::vector<int>& has_bit_indices, io::Printer* p,
                        int* cached_has_word_index, const Options& options) {
  if (!field->options().weak()) {
    int has_bit_index = has_bit_indices[field->index()];
    if (*cached_has_word_index != (has_bit_index / 32)) {
      *cached_has_word_index = (has_bit_index / 32);
      p->Emit({{"index", *cached_has_word_index}},
              R"cc(
                cached_has_bits = $has_bits$[$index$];
              )cc");
    }
    p->Emit({{"condition", GenerateConditionMaybeWithProbabilityForField(
                               has_bit_index, field, options)}},
            R"cc(
              if ($condition$) {
            )cc");
  } else {
    p->Emit(R"cc(
      if (has_$name$()) {
    )cc");
  }
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
  for (int i = 0; i < descriptor->field_count(); ++i) {
    fields[i] = descriptor->field(i);
  }
  std::sort(fields.begin(), fields.end(), FieldOrderingByNumber());
  return fields;
}

// Functor for sorting extension ranges by their "start" field number.
struct ExtensionRangeSorter {
  bool operator()(const Descriptor::ExtensionRange* left,
                  const Descriptor::ExtensionRange* right) const {
    return left->start_number() < right->start_number();
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

void EmitNonDefaultCheck(io::Printer* p, absl::string_view prefix,
                         const FieldDescriptor* field) {
  ABSL_CHECK(GetFieldHasbitMode(field) != HasbitMode::kTrueHasbit);
  ABSL_CHECK(!field->is_repeated());
  ABSL_CHECK(!field->containing_oneof() || field->real_containing_oneof());

  auto v = p->WithVars({{
      {"prefix", prefix},
      {"name", FieldName(field)},
  }});
  // Merge and serialize semantics: primitive fields are merged/serialized only
  // if non-zero (numeric) or non-empty (string).
  if (!field->containing_oneof()) {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
      p->Emit("!$prefix$_internal_$name$().empty()");
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      // Message fields still have has_$name$() methods.
      p->Emit("$prefix$_internal_has_$name$()");
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_FLOAT) {
      p->Emit("::absl::bit_cast<::uint32_t>($prefix$_internal_$name$()) != 0");
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_DOUBLE) {
      p->Emit("::absl::bit_cast<::uint64_t>($prefix$_internal_$name$()) != 0");
    } else {
      p->Emit("$prefix$_internal_$name$() != 0");
    }
  } else if (field->real_containing_oneof()) {
    p->Emit("$prefix$$has_field$");
  }
}

bool ShouldEmitNonDefaultCheck(const FieldDescriptor* field) {
  if (GetFieldHasbitMode(field) == HasbitMode::kTrueHasbit) {
    return false;
  }
  return !field->is_repeated();
}

void EmitNonDefaultCheckForString(io::Printer* p, absl::string_view prefix,
                                  const FieldDescriptor* field, bool split,
                                  const Options& opts,
                                  absl::AnyInvocable<void()> emit_body) {
  ABSL_DCHECK(field->cpp_type() == FieldDescriptor::CPPTYPE_STRING);
  ABSL_DCHECK(IsArenaStringPtr(field, opts));
  p->Emit(
      {
          {"condition", [&] { EmitNonDefaultCheck(p, prefix, field); }},
          {"emit_body", [&] { emit_body(); }},
          {"set_empty_string",
           [&] {
             p->Emit(
                 {
                     {"prefix", prefix},
                     {"name", FieldName(field)},
                     {"field", FieldMemberName(field, split)},
                 },
                 // The merge semantic is "overwrite if present". This statement
                 // is emitted when hasbit is set and src proto field is
                 // nonpresent (i.e. an empty string). Now, the destination
                 // string can be either empty or nonempty.
                 // - If dst is empty and pointing to the default instance,
                 //   allocate a new empty instance.
                 // - If dst is already pointing to a nondefault instance,
                 //   do nothing.
                 // This will allow destructors and Clear() to be simpler.
                 R"cc(
                   if (_this->$field$.IsDefault()) {
                     _this->_internal_set_$name$("");
                   }
                 )cc");
           }},
      },
      R"cc(
        if ($condition$) {
          $emit_body$;
        } else {
          $set_empty_string$;
        }
      )cc");
}

// Emits an if-statement with a condition that evaluates to true if |field| is
// considered non-default (will be sent over the wire), for message types
// without true field presence. Should only be called if
// !HasHasbit(field).
// If |with_enclosing_braces_always| is set to true, will generate enclosing
// braces even if nondefault check is not emitted -- i.e. code may look like:
// {
//   // code...
// }
// If |with_enclosing_braces_always| is set to false, enclosing braces will not
// be generated if nondefault check is not emitted.
void MayEmitIfNonDefaultCheck(io::Printer* p, absl::string_view prefix,
                              const FieldDescriptor* field,
                              absl::AnyInvocable<void()> emit_body,
                              bool with_enclosing_braces_always) {
  if (ShouldEmitNonDefaultCheck(field)) {
    p->Emit(
        {
            {"condition", [&] { EmitNonDefaultCheck(p, prefix, field); }},
            {"emit_body", [&] { emit_body(); }},
        },
        R"cc(
          if ($condition$) {
            $emit_body$;
          }
        )cc");
    return;
  }

  if (with_enclosing_braces_always) {
    // In repeated fields, the same variable name may be emitted multiple
    // times, hence the need for emitting braces even when the if condition is
    // not necessary, so that the code looks like:
    // {
    //   int tmpvar = ...;
    //   total += tmpvar;
    // }
    // {
    //   int tmpvar = ...;
    //   total += tmpvar;
    // }
    p->Emit({{"emit_body", [&] { emit_body(); }}},
            R"cc(
              {
                //~ Force newline.
                $emit_body$;
              }
            )cc");
    return;
  }

  // If no enclosing braces need to be emitted, just emit the body directly.
  emit_body();
}

void MayEmitMutableIfNonDefaultCheck(io::Printer* p, absl::string_view prefix,
                                     const FieldDescriptor* field, bool split,
                                     const Options& opts,
                                     absl::AnyInvocable<void()> emit_body,
                                     bool with_enclosing_braces_always) {
  if (ShouldEmitNonDefaultCheck(field)) {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING &&
        IsArenaStringPtr(field, opts)) {
      // If a field is backed by std::string, when default initialized it will
      // point to a global empty std::string instance. We prefer to spend some
      // extra cycles here to create a local string instance in the else branch,
      // so that we can get rid of a branch when Clear() is called (if we do
      // this, Clear() can always assume string instance is nonglobal).
      EmitNonDefaultCheckForString(p, prefix, field, split, opts,
                                   std::move(emit_body));
      return;
    }
  }

  // Fall back to the default implementation.
  return MayEmitIfNonDefaultCheck(p, prefix, field, std::move(emit_body),
                                  with_enclosing_braces_always);
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
      vars["val_cpp"] =
          absl::StrCat("::", ProtobufNamespace(options), "::Message");
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

bool HasNonSplitOptionalString(const Descriptor* desc, const Options& options) {
  for (const auto* field : FieldRange(desc)) {
    if (IsString(field) && !field->is_repeated() &&
        !field->real_containing_oneof() && !ShouldSplit(field, options)) {
      return true;
    }
  }
  return false;
}

// Returns true if two chunks may be grouped for hasword check to skip multiple
// cold fields at once. They have to share the following traits:
// - whether they have hasbits
// - whether they are rarely present
// - whether they are split
bool MayGroupChunksForHaswordsCheck(const FieldChunk& a, const FieldChunk& b) {
  return a.has_hasbit == b.has_hasbit &&
         a.is_rarely_present == b.is_rarely_present &&
         a.should_split == b.should_split;
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

// Returns true if it emits conditional check against hasbit words. This is
// useful to skip multiple fields that are unlikely present based on profile
// (go/pdproto). Assumes that each chunk is limited to one has "byte".
bool MaybeEmitHaswordsCheck(ChunkIterator it, ChunkIterator end,
                            const Options& options,
                            const std::vector<int>& has_bit_indices,
                            int cached_has_word_index, const std::string& from,
                            io::Printer* p) {
  if (!it->has_hasbit || !IsProfileDriven(options) ||
      std::distance(it, end) < 2 || !it->is_rarely_present) {
    return false;
  }

  auto hasbit_word = [&has_bit_indices](const FieldDescriptor* field) {
    return has_bit_indices[field->index()] / 32;
  };
  auto is_same_hasword = [&](const FieldChunk& a, const FieldChunk& b) {
    // Empty fields are assumed to have the same haswords.
    if (a.fields.empty() || b.fields.empty()) return true;

    return hasbit_word(a.fields.front()) == hasbit_word(b.fields.front());
  };

  struct HasWordMask {
    int word;
    uint32_t mask;
  };

  std::vector<HasWordMask> hasword_masks;
  while (it != end) {
    auto next = FindNextUnequalChunk(it, end, is_same_hasword);
    for (; it != next; ++it) {
      if (!it->fields.empty()) {
        hasword_masks.push_back({hasbit_word(it->fields.front()),
                                 GenChunkMask(it, next, has_bit_indices)});
        break;
      }
    }
    // Jump to the next batch instead.
    it = next;
  }

  if (hasword_masks.empty()) return false;

  // Emit has_bit check for each has_bit_dword index.
  p->Emit({{"cond",
            [&] {
              int first_word = hasword_masks.front().word;
              for (const auto& m : hasword_masks) {
                uint32_t mask = m.mask;
                int this_word = m.word;
                if (this_word != first_word) {
                  p->Emit(R"cc(
                    ||
                  )cc");
                }
                auto v =
                    p->WithVars({{"mask", absl::StrFormat("0x%08xU", mask)}});
                if (this_word == cached_has_word_index) {
                  p->Emit("(cached_has_bits & $mask$) != 0");
                } else {
                  p->Emit({{"from", from}, {"word", this_word}},
                          "($from$_impl_._has_bits_[$word$] & $mask$) != 0");
                }
              }
            }}},
          R"cc(
            if (ABSL_PREDICT_FALSE($cond$)) {
          )cc");
  p->Indent();
  return true;
}

using Sub = ::google::protobuf::io::Printer::Sub;
std::vector<Sub> ClassVars(const Descriptor* desc, Options opts) {
  std::vector<Sub> vars = {
      {"pkg", Namespace(desc, opts)},
      {"Msg", ClassName(desc, false)},
      {"pkg::Msg", QualifiedClassName(desc, opts)},
      {"pkg.Msg", desc->full_name()},

      // Old-style names, to be removed once all usages are gone in this and
      // other files.
      {"classname", ClassName(desc, false)},
      {"classtype", QualifiedClassName(desc, opts)},
      {"full_name", desc->full_name()},
      {"superclass", SuperClassName(desc, opts)},

      Sub("WeakDescriptorSelfPin",
          UsingImplicitWeakDescriptor(desc->file(), opts)
              ? absl::StrCat(StrongReferenceToType(desc, opts), ";")
              : "")
          .WithSuffix(";"),
  };

  for (auto& pair : MessageVars(desc)) {
    vars.push_back({std::string(pair.first), pair.second});
  }

  for (auto& pair : UnknownFieldsVars(desc, opts)) {
    vars.push_back({std::string(pair.first), pair.second});
  }

  return vars;
}

}  // namespace

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
    message_layout_helper_ = std::make_unique<PaddingOptimizer>(descriptor);
  }

  // Compute optimized field order to be used for layout and initialization
  // purposes.
  num_weak_fields_ = CollectFieldsExcludingWeakAndOneof(descriptor_, options_,
                                                        optimized_order_);
  const size_t initial_size = optimized_order_.size();
  optimized_order_ = message_layout_helper_->OptimizeLayout(
      optimized_order_, options_, scc_analyzer_);
  ABSL_CHECK_EQ(initial_size, optimized_order_.size());
  // Verify that all split fields are placed at the end in the optimized order.
  ABSL_CHECK(std::is_sorted(
      optimized_order_.begin(), optimized_order_.end(),
      [this](const FieldDescriptor* a, const FieldDescriptor* b) {
        return static_cast<int>(ShouldSplit(a, options_)) <
               static_cast<int>(ShouldSplit(b, options_));
      }));

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

  for (int i = 0; i < descriptor->field_count(); ++i) {
    if (descriptor->field(i)->is_required()) {
      ++num_required_fields_;
    }
  }

  parse_function_generator_ = std::make_unique<ParseFunctionGenerator>(
      descriptor_, max_has_bit_index_, has_bit_indices_,
      inlined_string_indices_, options_, scc_analyzer_, variables_,
      index_in_file_messages_);
}

bool MessageGenerator::ShouldGenerateEnclosingIf(
    const FieldDescriptor& field) const {
  return HasBitIndex(&field) != kNoHasbit &&
         (field.is_repeated() ||
          field.cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ||
          field.cpp_type() == FieldDescriptor::CPPTYPE_STRING);
}

size_t MessageGenerator::HasBitsSize() const {
  return (max_has_bit_index_ + 31) / 32;
}

size_t MessageGenerator::InlinedStringDonatedSize() const {
  return (max_inlined_string_index_ + 31) / 32;
}

absl::flat_hash_map<absl::string_view, std::string>
MessageGenerator::HasBitVars(const FieldDescriptor* field) const {
  int has_bit_index = HasBitIndex(field);
  ABSL_CHECK_NE(has_bit_index, kNoHasbit);
  return {
      {"has_array_index", absl::StrCat(has_bit_index / 32)},
      {"has_mask", absl::StrFormat("0x%08xU", 1u << (has_bit_index % 32))},
  };
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
  for (int i = 0; i < descriptor_->enum_type_count(); ++i) {
    enum_generators->emplace_back(
        std::make_unique<EnumGenerator>(descriptor_->enum_type(i), options_));
    enum_generators_.push_back(enum_generators->back().get());
  }
  for (int i = 0; i < descriptor_->extension_count(); ++i) {
    extension_generators->emplace_back(std::make_unique<ExtensionGenerator>(
        descriptor_->extension(i), options_, scc_analyzer_));
    extension_generators_.push_back(extension_generators->back().get());
  }
}

void MessageGenerator::GenerateFieldAccessorDeclarations(io::Printer* p) {
  auto v = p->WithVars(MessageVars(descriptor_));

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
    p->Emit({{"field_comment", FieldComment(field, options_)},
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
                p->Emit({Sub("_internal_has_name",
                             absl::StrCat("_internal_has_", name))
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
                bool _is_packed,
                typename = typename _proto_TypeTraits::Singular>
      PROTOBUF_FUTURE_ADD_NODISCARD inline bool HasExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id) const {
        $WeakDescriptorSelfPin$;
        $annotate_extension_has$;
        return $extensions$.Has(id.number());
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline void ClearExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id) {
        $WeakDescriptorSelfPin$;
        $extensions$.ClearExtension(id.number());
        $annotate_extension_clear$;
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed,
                typename = typename _proto_TypeTraits::Repeated>
      PROTOBUF_FUTURE_ADD_NODISCARD inline int ExtensionSize(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id) const {
        $WeakDescriptorSelfPin$;
        $annotate_extension_repeated_size$;
        return $extensions$.ExtensionSize(id.number());
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed,
                ::std::enable_if_t<!_proto_TypeTraits::kLifetimeBound, int> = 0>
      PROTOBUF_FUTURE_ADD_NODISCARD inline
          typename _proto_TypeTraits::Singular::ConstType
          GetExtension(const $pbi$::ExtensionIdentifier<
                       $Msg$, _proto_TypeTraits, _field_type, _is_packed>& id)
              const {
        $WeakDescriptorSelfPin$;
        $annotate_extension_get$;
        return _proto_TypeTraits::Get(id.number(), $extensions$, id.default_value());
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed,
                ::std::enable_if_t<_proto_TypeTraits::kLifetimeBound, int> = 0>
      PROTOBUF_FUTURE_ADD_NODISCARD inline
          typename _proto_TypeTraits::Singular::ConstType
          GetExtension(const $pbi$::ExtensionIdentifier<
                       $Msg$, _proto_TypeTraits, _field_type, _is_packed>& id)
              const ABSL_ATTRIBUTE_LIFETIME_BOUND {
        $WeakDescriptorSelfPin$;
        $annotate_extension_get$;
        return _proto_TypeTraits::Get(id.number(), $extensions$, id.default_value());
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline typename _proto_TypeTraits::Singular::MutableType MutableExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id)
          ABSL_ATTRIBUTE_LIFETIME_BOUND {
        $WeakDescriptorSelfPin$;
        $annotate_extension_mutable$;
        return _proto_TypeTraits::Mutable(id.number(), _field_type, &$extensions$);
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline void SetExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id,
          typename _proto_TypeTraits::Singular::ConstType value) {
        $WeakDescriptorSelfPin$;
        _proto_TypeTraits::Set(id.number(), _field_type, value, &$extensions$);
        $annotate_extension_set$;
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline void SetAllocatedExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id,
          typename _proto_TypeTraits::Singular::MutableType value) {
        $WeakDescriptorSelfPin$;
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
        $WeakDescriptorSelfPin$;
        _proto_TypeTraits::UnsafeArenaSetAllocated(id.number(), _field_type,
                                                   value, &$extensions$);
        $annotate_extension_set$;
      }
      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      [[nodiscard]] inline typename _proto_TypeTraits::Singular::MutableType
      ReleaseExtension(const $pbi$::ExtensionIdentifier<
                       $Msg$, _proto_TypeTraits, _field_type, _is_packed>& id) {
        $WeakDescriptorSelfPin$;
        $annotate_extension_release$;
        return _proto_TypeTraits::Release(id.number(), _field_type, &$extensions$);
      }
      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline typename _proto_TypeTraits::Singular::MutableType
      UnsafeArenaReleaseExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id) {
        $WeakDescriptorSelfPin$;
        $annotate_extension_release$;
        return _proto_TypeTraits::UnsafeArenaRelease(id.number(), _field_type,
                                                     &$extensions$);
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed,
                ::std::enable_if_t<!_proto_TypeTraits::kLifetimeBound, int> = 0>
      PROTOBUF_FUTURE_ADD_NODISCARD inline
          typename _proto_TypeTraits::Repeated::ConstType
          GetExtension(
              const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                               _field_type, _is_packed>& id,
              int index) const {
        $WeakDescriptorSelfPin$;
        $annotate_repeated_extension_get$;
        return _proto_TypeTraits::Get(id.number(), $extensions$, index);
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed,
                ::std::enable_if_t<_proto_TypeTraits::kLifetimeBound, int> = 0>
      PROTOBUF_FUTURE_ADD_NODISCARD inline
          typename _proto_TypeTraits::Repeated::ConstType
          GetExtension(
              const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                               _field_type, _is_packed>& id,
              int index) const ABSL_ATTRIBUTE_LIFETIME_BOUND {
        $WeakDescriptorSelfPin$;
        $annotate_repeated_extension_get$;
        return _proto_TypeTraits::Get(id.number(), $extensions$, index);
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline typename _proto_TypeTraits::Repeated::MutableType MutableExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id,
          int index) ABSL_ATTRIBUTE_LIFETIME_BOUND {
        $WeakDescriptorSelfPin$;
        $annotate_repeated_extension_mutable$;
        return _proto_TypeTraits::Mutable(id.number(), index, &$extensions$);
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline void SetExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id,
          int index, typename _proto_TypeTraits::Repeated::ConstType value) {
        $WeakDescriptorSelfPin$;
        _proto_TypeTraits::Set(id.number(), index, value, &$extensions$);
        $annotate_repeated_extension_set$;
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline typename _proto_TypeTraits::Repeated::MutableType AddExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id)
          ABSL_ATTRIBUTE_LIFETIME_BOUND {
        $WeakDescriptorSelfPin$;
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
        $WeakDescriptorSelfPin$;
        _proto_TypeTraits::Add(id.number(), _field_type, _is_packed, value,
                               &$extensions$);
        $annotate_repeated_extension_add$;
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      PROTOBUF_FUTURE_ADD_NODISCARD inline const typename _proto_TypeTraits::
          Repeated::RepeatedFieldType&
          GetRepeatedExtension(
              const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                               _field_type, _is_packed>& id)
              const ABSL_ATTRIBUTE_LIFETIME_BOUND {
        $WeakDescriptorSelfPin$;
        $annotate_repeated_extension_list$;
        return _proto_TypeTraits::GetRepeated(id.number(), $extensions$);
      }

      template <typename _proto_TypeTraits, $pbi$::FieldType _field_type,
                bool _is_packed>
      inline typename _proto_TypeTraits::Repeated::RepeatedFieldType* $nonnull$
      MutableRepeatedExtension(
          const $pbi$::ExtensionIdentifier<$Msg$, _proto_TypeTraits,
                                           _field_type, _is_packed>& id)
          ABSL_ATTRIBUTE_LIFETIME_BOUND {
        $WeakDescriptorSelfPin$;
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
    p->Emit({{"oneof_name", oneof->name()},
             Sub{"clear_oneof_name", absl::StrCat("clear_", oneof->name())}
                 .AnnotatedAs({oneof, Semantic::kSet}),
             {"OneOfName", UnderscoresToCamelCase(oneof->name(), true)}},
            R"cc(
              void $clear_oneof_name$();
              $OneOfName$Case $oneof_name$_case() const;
            )cc");
  }
}

void MessageGenerator::GenerateSingularFieldHasBits(
    const FieldDescriptor* field, io::Printer* p) {
  auto t = p->WithVars(MakeTrackerCalls(field, options_));
  if (field->options().weak()) {
    p->Emit(
        R"cc(
          inline bool $classname$::has_$name$() const {
            $WeakDescriptorSelfPin$;
            $annotate_has$;
            return $weak_field_map$.Has($number$);
          }
        )cc");
    return;
  }
  if (GetFieldHasbitMode(field) == HasbitMode::kTrueHasbit) {
    auto v = p->WithVars(HasBitVars(field));
    p->Emit(
        {Sub{"ASSUME",
             [&] {
               if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
                   !IsLazy(field, options_, scc_analyzer_)) {
                 // We maintain the invariant that for a submessage x, has_x()
                 // returning true implies that x_ is not null. By giving this
                 // information to the compiler, we allow it to eliminate
                 // unnecessary null checks later on.
                 p->Emit(
                     R"cc(PROTOBUF_ASSUME(!value || $field$ != nullptr);)cc");
               }
             }}
             .WithSuffix(";")},
        R"cc(
          inline bool $classname$::has_$name$() const {
            $WeakDescriptorSelfPin$;
            $annotate_has$;
            bool value = ($has_bits$[$has_array_index$] & $has_mask$) != 0;
            $ASSUME$;
            return value;
          }
        )cc");
  }
}

void MessageGenerator::GenerateOneofHasBits(io::Printer* p) {
  for (const auto* oneof : OneOfRange(descriptor_)) {
    p->Emit(
        {
            {"oneof_index", oneof->index()},
            {"oneof_name", oneof->name()},
            {"cap_oneof_name", absl::AsciiStrToUpper(oneof->name())},
        },
        R"cc(
          inline bool $classname$::has_$oneof_name$() const {
            return $oneof_name$_case() != $cap_oneof_name$_NOT_SET;
          }
          inline void $classname$::clear_has_$oneof_name$() {
            $oneof_case$[$oneof_index$] = $cap_oneof_name$_NOT_SET;
          }
        )cc");
  }
}

void MessageGenerator::GenerateOneofMemberHasBits(const FieldDescriptor* field,
                                                  io::Printer* p) {
  // Singular field in a oneof
  // N.B.: Without field presence, we do not use has-bits or generate
  // has_$name$() methods, but oneofs still have set_has_$name$().
  // Oneofs also have private _internal_has_$name$() a helper method.
  if (field->has_presence()) {
    auto t = p->WithVars(MakeTrackerCalls(field, options_));
    p->Emit(R"cc(
      inline bool $classname$::has_$name$() const {
        $WeakDescriptorSelfPin$;
        $annotate_has$;
        return $has_field$;
      }
    )cc");
  }
  if (HasInternalHasMethod(field)) {
    p->Emit(R"cc(
      inline bool $classname$::_internal_has_$name_internal$() const {
        return $has_field$;
      }
    )cc");
  }
  // set_has_$name$() for oneof fields is always private; hence should not be
  // annotated.
  p->Emit(R"cc(
    inline void $classname$::set_has_$name_internal$() {
      $oneof_case$[$oneof_index$] = k$field_name$;
    }
  )cc");
}

void MessageGenerator::GenerateFieldClear(const FieldDescriptor* field,
                                          bool is_inline, io::Printer* p) {
  auto t = p->WithVars(MakeTrackerCalls(field, options_));
  p->Emit({{"inline", is_inline ? "inline" : ""},
           {"body",
            [&] {
              if (field->real_containing_oneof()) {
                // Clear this field only if it is the active field in this
                // oneof, otherwise ignore
                p->Emit(
                    {{"clearing_code",
                      [&] {
                        field_generators_.get(field).GenerateClearingCode(p);
                      }}},
                    R"cc(
                      if ($has_field$) {
                        $clearing_code$;
                        clear_has_$oneof_name$();
                      }
                    )cc");
              } else {
                // TODO: figure out if early return breaks tracking
                if (ShouldSplit(field, options_)) {
                  p->Emit(R"cc(
                    if (ABSL_PREDICT_TRUE(IsSplitMessageDefault()))
                      return;
                  )cc");
                }
                field_generators_.get(field).GenerateClearingCode(p);
                if (HasHasbit(field)) {
                  auto v = p->WithVars(HasBitVars(field));
                  p->Emit(R"cc(
                    $has_bits$[$has_array_index$] &= ~$has_mask$;
                  )cc");
                }
              }
            }}},
          R"cc(
            $inline $void $classname$::clear_$name$() {
              $pbi$::TSanWrite(&_impl_);
              $WeakDescriptorSelfPin$;
              $body$;
              $annotate_clear$;
            }
          )cc");
}

void MessageGenerator::GenerateVerifyHasBitConsistency(
    io::Printer* p, absl::string_view prefix) {
  p->Emit({{"prefix", prefix}},
          R"cc(
#ifdef PROTOBUF_ENABLE_VERIFY_HAS_BIT_CONSISTENCY
            $prefix$VerifyHasBitConsistency();
#endif  // PROTOBUF_ENABLE_VERIFY_HAS_BIT_CONSISTENCY
          )cc");
}

namespace {

class AccessorVerifier {
 public:
  using SourceLocation = io::Printer::SourceLocation;

  explicit AccessorVerifier(const FieldDescriptor* field) : field_(field) {}
  ~AccessorVerifier() {
    ABSL_CHECK(!needs_annotate_) << Error(SourceLocation::current());
    ABSL_CHECK(!needs_weak_descriptor_pin_) << Error(SourceLocation::current());
  }

  void operator()(absl::string_view label, io::Printer::SourceLocation loc) {
    if (label == "name" || label == "release_name") {
      // All accessors use $name$ or $release_name$ when constructing the
      // function name. We hook into those to determine that an accessor is
      // starting.
      SetTracker(needs_annotate_, true, loc);
      SetTracker(needs_weak_descriptor_pin_, true, loc);
      loc_ = loc;
    } else if (absl::StartsWith(label, "annotate")) {
      // All annotation labels start with `annotate`. Eg `annotate_get`.
      SetTracker(needs_annotate_, false, loc);
      loc_ = loc;
    } else if (label == "WeakDescriptorSelfPin") {
      // The self pin for weak descriptor types must be on every accessor.
      SetTracker(needs_weak_descriptor_pin_, false, loc);
      loc_ = loc;
    }
  }

 private:
  std::string Error(SourceLocation loc) const {
    return absl::StrFormat("Field %s printed from %s:%d (prev %s:%d)\n",
                           field_->full_name(), loc.file_name(), loc.line(),
                           loc_.file_name(), loc_.line());
  }

  void SetTracker(bool& v, bool new_value, SourceLocation loc) {
    ABSL_CHECK_NE(v, new_value) << Error(loc);
    v = new_value;
  }

  bool needs_annotate_ = false;
  bool needs_weak_descriptor_pin_ = false;
  // We keep these fields for error reporting.
  const FieldDescriptor* field_;
  // On error, we report two locations: the current one and the last one. This
  // can help determine where the bug is. For example, if we see "name" twice in
  // a row, the bug is likely in the "last" one and not the current one because
  // it means the previous accessor didn't add the required code.
  SourceLocation loc_;
};

}  // namespace

template <bool kIsV2>
void MessageGenerator::EmitCheckAndUpdateByteSizeForField(
    const FieldDescriptor* field, io::Printer* p) const {
  absl::AnyInvocable<void()> emit_body = [&] {
    const auto& gen = field_generators_.get(field);
    if constexpr (!kIsV2) {
      gen.GenerateByteSize(p);
    } else {
    }
  };

  if (!HasHasbit(field)) {
    MayEmitIfNonDefaultCheck(p, "this_.", field, std::move(emit_body),
                             /*with_enclosing_braces_always=*/true);
    return;
  }
  if (field->options().weak()) {
    p->Emit({{"emit_body", [&] { emit_body(); }}},
            R"cc(
              if (has_$name$()) {
                $emit_body$;
              }
            )cc");
    return;
  }

  int has_bit_index = has_bit_indices_[field->index()];
  p->Emit({{"condition", GenerateConditionMaybeWithProbabilityForField(
                             has_bit_index, field, options_)},
           {"check_nondefault_and_emit_body",
            [&] {
              // Note that it's possible that the field has explicit presence.
              // In that case, nondefault check will not be emitted but
              // emit_body will still be emitted.
              MayEmitIfNonDefaultCheck(p, "this_.", field, std::move(emit_body),
                                       /*with_enclosing_braces_always=*/false);
            }}},
          R"cc(
            if ($condition$) {
              $check_nondefault_and_emit_body$;
            }
          )cc");
}

void MessageGenerator::MaybeEmitUpdateCachedHasbits(
    const FieldDescriptor* field, io::Printer* p,
    int& cached_has_word_index) const {
  if (!HasHasbit(field) || field->options().weak()) return;

  int has_bit_index = has_bit_indices_[field->index()];

  if (cached_has_word_index == (has_bit_index / 32)) return;

  cached_has_word_index = (has_bit_index / 32);
  p->Emit({{"index", cached_has_word_index}},
          R"cc(
            cached_has_bits = this_.$has_bits$[$index$];
          )cc");
}

void MessageGenerator::EmitUpdateByteSizeForField(
    const FieldDescriptor* field, io::Printer* p,
    int& cached_has_word_index) const {
  p->Emit(
      {{"comment", [&] { PrintFieldComment(Formatter{p}, field, options_); }},
       {"update_cached_has_bits",
        [&] { MaybeEmitUpdateCachedHasbits(field, p, cached_has_word_index); }},
       {"check_and_update_byte_size_for_field",
        [&]() {
          EmitCheckAndUpdateByteSizeForField</*kIsV2=*/false>(field, p);
        }}},
      R"cc(
        $comment$;
        $update_cached_has_bits$;
        $check_and_update_byte_size_for_field$;
      )cc");
}

void MessageGenerator::EmitUpdateByteSizeV2ForNumerics(
    size_t field_size, io::Printer* p, int& cached_has_word_index,
    std::vector<const FieldDescriptor*>&& fields) const {
  if (fields.empty()) return;

  auto v = p->WithVars({{"field_size", field_size}});
  p->Emit(R"cc(
    // fixed size numerics: $field_size$
  )cc");
  for (const auto* f : fields) {
    p->Emit({{"full_name", f->full_name()}},
            R"cc(
              // $full_name$
            )cc");
  }

  p->Emit({{"mask",
            absl::StrFormat("0x%08xU", GenChunkMask(fields, has_bit_indices_))},
           {"size", 1 + 4 + field_size},  // tag + field number + payload
           {"update_cached_has_bits",
            [&] {
              MaybeEmitUpdateCachedHasbits(fields.front(), p,
                                           cached_has_word_index);
            }}},
          R"cc(
            $update_cached_has_bits$;
            total_size += absl::popcount(cached_has_bits & $mask$) * $size$;
          )cc");
}


void MessageGenerator::GenerateFieldAccessorDefinitions(io::Printer* p) {
  p->Emit("// $classname$\n\n");

  for (auto field : FieldRange(descriptor_)) {
    // We use a print listener to verify that the field generators properly add
    // the right annotations. This is only a verification step aimed to prevent
    // bugs where we have lack of test coverage. Note that this will verify the
    // annotations even when the particular feature is not on because we look at
    // the substitution variables, not the substitution result.
    // The check is a state machine that verifies that every substitution for
    // `name` is followed by one and only one for needed annotations. False
    // positives are accessors that are using $name$ for an internal name. For
    // those you can use $name_internal$ which is the same substitution but not
    // tracked by the verifier.
    const auto accessor_verifier =
        p->WithSubstitutionListener(AccessorVerifier(field));

    PrintFieldComment(Formatter{p}, field, options_);

    auto v = p->WithVars(FieldVars(field, options_));
    auto t = p->WithVars(MakeTrackerCalls(field, options_));
    if (field->is_repeated()) {
      p->Emit(R"cc(
        inline int $classname$::_internal_$name_internal$_size() const {
          return _internal_$name_internal$().size();
        }
        inline int $classname$::$name$_size() const {
          $WeakDescriptorSelfPin$;
          $annotate_size$;
          return _internal_$name_internal$_size();
        }
      )cc");
    } else if (field->real_containing_oneof()) {
      GenerateOneofMemberHasBits(field, p);
    } else {
      GenerateSingularFieldHasBits(field, p);
    }

    if (!IsCrossFileMaybeMap(field)) {
      GenerateFieldClear(field, true, p);
    }
    // Generate type-specific accessors.
    field_generators_.get(field).GenerateInlineAccessorDefinitions(p);

    p->Emit("\n");
  }

  GenerateOneofHasBits(p);
}

void MessageGenerator::GenerateVerifyDecl(io::Printer* p) {
}

void MessageGenerator::GenerateAnnotationDecl(io::Printer* p) {
}

void MessageGenerator::GenerateMapEntryClassDefinition(io::Printer* p) {
  Formatter format(p);
  absl::flat_hash_map<absl::string_view, std::string> vars;
  CollectMapInfo(options_, descriptor_, &vars);
  ABSL_CHECK(HasDescriptorMethods(descriptor_->file(), options_));
  auto v = p->WithVars(std::move(vars));
  // Templatize constexpr constructor as a workaround for a bug in gcc 12
  // (warning in gcc 13).
  p->Emit(
      {{"decl_verify_func",
        [&] {
        }},
       {"decl_annotate", [&] { GenerateAnnotationDecl(p); }},
       {"parse_decls",
        [&] {
          parse_function_generator_->GenerateDataDecls(p);
        }}},
      R"cc(
        class $classname$ final
            : public $pbi$::MapEntry<$key_cpp$, $val_cpp$,
                                     $pbi$::WireFormatLite::$key_wire_type$,
                                     $pbi$::WireFormatLite::$val_wire_type$> {
         public:
          using SuperType =
              $pbi$::MapEntry<$key_cpp$, $val_cpp$,
                              $pbi$::WireFormatLite::$key_wire_type$,
                              $pbi$::WireFormatLite::$val_wire_type$>;
          $classname$();
          template <typename = void>
          explicit PROTOBUF_CONSTEXPR $classname$($pbi$::ConstantInitialized);
          explicit $classname$($pb$::Arena* $nullable$ arena);
          static constexpr const void* $nonnull$ internal_default_instance() {
            return &_$classname$_default_instance_;
          }

          $decl_verify_func$;

          static constexpr auto InternalGenerateClassData_();

         private:
          friend class $pb$::MessageLite;
          friend struct ::$tablename$;

          $parse_decls$;
          $decl_annotate$;

          const $pbi$::ClassData* $nonnull$ GetClassData() const PROTOBUF_FINAL;
          static void* $nonnull$ PlacementNew_(
              //~
              const void* $nonnull$, void* $nonnull$ mem,
              $pb$::Arena* $nullable$ arena);
          static constexpr auto InternalNewImpl_();
        };
        $dllexport_decl $extern const $pbi$::ClassDataFull $classname$_class_data_;
      )cc");
}

void MessageGenerator::GenerateImplDefinition(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  // Prepare decls for _cached_size_ and _has_bits_.  Their position in the
  // output will be determined later.
  bool need_to_emit_cached_size = true;
  const size_t sizeof_has_bits = HasBitsSize();

  // To minimize padding, data members are divided into three sections:
  // (1) members assumed to align to 8 bytes
  // (2) members corresponding to message fields, re-ordered to optimize
  //     alignment.
  // (3) members assumed to align to 4 bytes.
  p->Emit(
      {{"extension_set",
        [&] {
          if (descriptor_->extension_range_count() == 0) return;

          p->Emit(R"cc(
            $pbi$::ExtensionSet _extensions_;
          )cc");
        }},
       {"tracker",
        [&] {
          if (!HasTracker(descriptor_, options_)) return;

          p->Emit(R"cc(
            static $pb$::AccessListener<$Msg$> _tracker_;
            static void TrackerOnGetMetadata() { $annotate_reflection$; }
          )cc");
        }},
       {"inlined_string_donated",
        [&] {
          // Generate _inlined_string_donated_ for inlined string type.
          // TODO: To avoid affecting the locality of
          // `_has_bits_`, should this be below or above `_has_bits_`?
          if (inlined_string_indices_.empty()) return;

          p->Emit({{"donated_size", InlinedStringDonatedSize()}},
                  R"cc(
                    $pbi$::HasBits<$donated_size$> _inlined_string_donated_;
                  )cc");
        }},
       {"has_bits",
        [&] {
          if (has_bit_indices_.empty()) return;

          // _has_bits_ is frequently accessed, so to reduce code size and
          // improve speed, it should be close to the start of the object.
          // Placing _cached_size_ together with _has_bits_ improves cache
          // locality despite potential alignment padding.
          p->Emit({{"sizeof_has_bits", sizeof_has_bits}}, R"cc(
            $pbi$::HasBits<$sizeof_has_bits$> _has_bits_;
          )cc");
          if (need_to_emit_cached_size) {
            p->Emit(R"cc(
              $pbi$::CachedSize _cached_size_;
            )cc");
            need_to_emit_cached_size = false;
          }
        }},
       {"field_members",
        [&] {
          // Emit some private and static members
          for (auto field : optimized_order_) {
            field_generators_.get(field).GenerateStaticMembers(p);
            if (!ShouldSplit(field, options_)) {
              field_generators_.get(field).GeneratePrivateMembers(p);
            }
          }
        }},
       {"decl_split",
        [&] {
          if (!ShouldSplit(descriptor_, options_)) return;
          p->Emit({{"split_field",
                    [&] {
                      for (auto field : optimized_order_) {
                        if (!ShouldSplit(field, options_)) continue;
                        field_generators_.get(field).GeneratePrivateMembers(p);
                      }
                    }}},
                  R"cc(
                    struct Split {
                      $split_field$;
                      using InternalArenaConstructable_ = void;
                      using DestructorSkippable_ = void;
                    };
                    static_assert(::std::is_trivially_copy_constructible<Split>::value);
                    static_assert(::std::is_trivially_destructible<Split>::value);
                    Split* $nonnull$ _split_;
                  )cc");
        }},
       {"oneof_members",
        [&] {
          // For each oneof generate a union
          for (auto oneof : OneOfRange(descriptor_)) {
            // explicit empty constructor is needed when union contains
            // ArenaStringPtr members for string fields.
            p->Emit(
                {{"camel_oneof_name",
                  UnderscoresToCamelCase(oneof->name(), true)},
                 {"oneof_name", oneof->name()},
                 {"oneof_fields",
                  [&] {
                    for (auto field : FieldRange(oneof)) {
                      field_generators_.get(field).GeneratePrivateMembers(p);
                    }
                  }}},
                R"cc(
                  union $camel_oneof_name$Union {
                    constexpr $camel_oneof_name$Union() : _constinit_{} {}
                    $pbi$::ConstantInitialized _constinit_;
                    $oneof_fields$;
                  } $oneof_name$_;
                )cc");
            for (auto field : FieldRange(oneof)) {
              field_generators_.get(field).GenerateStaticMembers(p);
            }
          }
        }},
       {"cached_size_if_no_hasbits",
        [&] {
          if (!need_to_emit_cached_size) return;

          need_to_emit_cached_size = false;
          p->Emit(R"cc(
            $pbi$::CachedSize _cached_size_;
          )cc");
        }},
       {"oneof_case",
        [&] {
          // Generate _oneof_case_.
          if (descriptor_->real_oneof_decl_count() == 0) return;

          p->Emit({{"count", descriptor_->real_oneof_decl_count()}},
                  R"cc(
                    $uint32$ _oneof_case_[$count$];
                  )cc");
        }},
       {"weak_field_map",
        [&] {
          if (num_weak_fields_ == 0) return;

          p->Emit(R"cc(
            $pbi$::WeakFieldMap _weak_field_map_;
          )cc");
        }},
       {"union_impl",
        [&] {
          // Only create the _impl_ field if it contains data.
          if (!HasImplData(descriptor_, options_)) return;

          // clang-format off
            p->Emit(R"cc(union { Impl_ _impl_; };)cc");
          // clang-format on
        }}},
      R"cc(
        struct Impl_ {
          //~ TODO: check if/when there is a need for an
          //~ outline dtor.
          inline explicit constexpr Impl_($pbi$::ConstantInitialized) noexcept;
          inline explicit Impl_(
              //~
              $pbi$::InternalVisibility visibility,
              $pb$::Arena* $nullable$ arena);
          inline explicit Impl_(
              //~
              $pbi$::InternalVisibility visibility,
              $pb$::Arena* $nullable$ arena, const Impl_& from,
              const $classname$& from_msg);
          //~ Members assumed to align to 8 bytes:
          $extension_set$;
          $tracker$;
          $inlined_string_donated$;
          $has_bits$;
          //~ Field members:
          $field_members$;
          $decl_split$;
          $oneof_members$;
          //~ Members assumed to align to 4 bytes:
          $cached_size_if_no_hasbits$;
          $oneof_case$;
          $weak_field_map$;
          //~ For detecting when concurrent accessor calls cause races.
          PROTOBUF_TSAN_DECLARE_MEMBER
        };
        $union_impl$;
      )cc");

  ABSL_DCHECK(!need_to_emit_cached_size);
}

void MessageGenerator::GenerateAnyMethodDefinition(io::Printer* p) {
  ABSL_DCHECK(IsAnyMessage(descriptor_));

  p->Emit(
      {{"any_methods",
        [&] {
          if (HasDescriptorMethods(descriptor_->file(), options_)) {
            p->Emit(
                R"cc(
                  bool PackFrom(const $pb$::Message& message) {
                    $DCHK$_NE(&message, this);
                    return $pbi$::InternalPackFrom(message, mutable_type_url(),
                                                   _internal_mutable_value());
                  }
                  bool PackFrom(const $pb$::Message& message,
                                ::absl::string_view type_url_prefix) {
                    $DCHK$_NE(&message, this);
                    return $pbi$::InternalPackFrom(message, type_url_prefix,
                                                   mutable_type_url(),
                                                   _internal_mutable_value());
                  }
                  bool UnpackTo($pb$::Message* $nonnull$ message) const {
                    return $pbi$::InternalUnpackTo(_internal_type_url(),
                                                   _internal_value(), message);
                  }
                  static bool GetAnyFieldDescriptors(
                      const $pb$::Message& message,
                      const $pb$::FieldDescriptor * $nullable$ *
                          $nonnull$ type_url_field,
                      const $pb$::FieldDescriptor * $nullable$ *
                          $nonnull$ value_field);
                  template <
                      typename T,
                      class = typename std::enable_if<!std::is_convertible<
                          T, const $pb$::Message&>::value>::type>
                  bool PackFrom(const T& message) {
                    return $pbi$::InternalPackFrom<T>(
                        message, mutable_type_url(), _internal_mutable_value());
                  }
                  template <
                      typename T,
                      class = typename std::enable_if<!std::is_convertible<
                          T, const $pb$::Message&>::value>::type>
                  bool PackFrom(const T& message,
                                ::absl::string_view type_url_prefix) {
                    return $pbi$::InternalPackFrom<T>(
                        message, type_url_prefix, mutable_type_url(),
                        _internal_mutable_value());
                  }
                  template <
                      typename T,
                      class = typename std::enable_if<!std::is_convertible<
                          T, const $pb$::Message&>::value>::type>
                  bool UnpackTo(T* $nonnull$ message) const {
                    return $pbi$::InternalUnpackTo<T>(
                        _internal_type_url(), _internal_value(), message);
                  }
                )cc");
          } else {
            p->Emit(
                R"cc(
                  template <typename T>
                  bool PackFrom(const T& message) {
                    return $pbi$::InternalPackFrom(message, mutable_type_url(),
                                                   _internal_mutable_value());
                  }
                  template <typename T>
                  bool PackFrom(const T& message,
                                ::absl::string_view type_url_prefix) {
                    return $pbi$::InternalPackFrom(message, type_url_prefix,
                                                   mutable_type_url(),
                                                   _internal_mutable_value());
                  }
                  template <typename T>
                  bool UnpackTo(T* $nonnull$ message) const {
                    return $pbi$::InternalUnpackTo(_internal_type_url(),
                                                   _internal_value(), message);
                  }
                )cc");
          }
        }}},
      R"cc(
        // implements Any
        // -----------------------------------------------

        $any_methods$;

        template <typename T>
        bool Is() const {
          return $pbi$::InternalIs<T>(_internal_type_url());
        }
        static bool ParseAnyTypeUrl(
            //~
            ::absl::string_view type_url,
            std::string* $nonnull$ full_type_name);
      )cc");
}

void MessageGenerator::GenerateClassDefinition(io::Printer* p) {
  if (!ShouldGenerateClass(descriptor_, options_)) return;

  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  Formatter format(p);

  if (IsMapEntryMessage(descriptor_)) {
    GenerateMapEntryClassDefinition(p);
    return;
  }

  auto annotation = p->WithAnnotations({{"classname", descriptor_}});
  p->Emit(
      {{"decl_dtor",
        [&] {
          if (HasSimpleBaseClass(descriptor_, options_)) return;

          p->Emit(R"cc(
            ~$classname$() PROTOBUF_FINAL;
          )cc");
        }},
       {"decl_annotate", [&] { GenerateAnnotationDecl(p); }},
       {"decl_verify_func", [&] { GenerateVerifyDecl(p); }},
       {"descriptor_accessor",
        [&] {
          // Only generate this member if it's not disabled.
          if (!HasDescriptorMethods(descriptor_->file(), options_) ||
              descriptor_->options().no_standard_descriptor_accessor()) {
            return;
          }

          p->Emit(R"cc(
            static const $pb$::Descriptor* $nonnull$ descriptor() {
              return GetDescriptor();
            }
          )cc");
        }},
       {"get_descriptor",
        [&] {
          // These shadow non-static methods of the same names in Message.
          // We redefine them here because calls directly on the generated
          // class can be statically analyzed -- we know what descriptor
          // types are being requested. It also avoids a vtable dispatch.
          //
          // We would eventually like to eliminate the methods in Message,
          // and having this separate also lets us track calls to the base
          // class methods separately.
          if (!HasDescriptorMethods(descriptor_->file(), options_)) return;

          p->Emit(R"cc(
            static const $pb$::Descriptor* $nonnull$ GetDescriptor() {
              return default_instance().GetMetadata().descriptor;
            }
            static const $pb$::Reflection* $nonnull$ GetReflection() {
              return default_instance().GetMetadata().reflection;
            }
          )cc");
        }},
       {"decl_oneof",
        [&] {
          // Generate enum values for every field in oneofs. One list is
          // generated for each oneof with an additional *_NOT_SET value.
          for (auto oneof : OneOfRange(descriptor_)) {
            p->Emit(
                {Sub{"oneof_camel_name",
                     absl::StrCat(UnderscoresToCamelCase(oneof->name(), true),
                                  "Case")}
                     .AnnotatedAs(oneof),
                 {"oneof_field",
                  [&] {
                    for (auto field : FieldRange(oneof)) {
                      p->Emit(
                          {
                              Sub{"oneof_constant",
                                  OneofCaseConstantName(field)}
                                  .AnnotatedAs(field),
                              {"field_number", field->number()},
                          },
                          R"cc(
                            $oneof_constant$ = $field_number$,
                          )cc");
                    }
                  }},
                 {"upper_oneof_name", absl::AsciiStrToUpper(oneof->name())}},
                R"cc(
                  enum $oneof_camel_name$ {
                    $oneof_field$,
                    $upper_oneof_name$_NOT_SET = 0,
                  };
                )cc");
          }
        }},
       {"index_in_file_messages", index_in_file_messages_},
       {"decl_any_methods",
        [&] {
          if (!IsAnyMessage(descriptor_)) return;

          GenerateAnyMethodDefinition(p);
        }},
       {"generated_methods",
        [&] {
          if (!HasGeneratedMethods(descriptor_->file(), options_)) return;

          if (HasDescriptorMethods(descriptor_->file(), options_)) {
            if (!HasSimpleBaseClass(descriptor_, options_)) {
              // Use Message's built-in MergeFrom and CopyFrom when the
              // passed-in argument is a generic Message instance, and
              // only define the custom MergeFrom and CopyFrom
              // instances when the source of the merge/copy is known
              // to be the same class as the destination.
              p->Emit(R"cc(
                using $superclass$::CopyFrom;
                void CopyFrom(const $classname$& from);
                using $superclass$::MergeFrom;
                void MergeFrom(const $classname$& from) { $classname$::MergeImpl(*this, from); }

                private:
                static void MergeImpl($pb$::MessageLite& to_msg,
                                      const $pb$::MessageLite& from_msg);

                public:
              )cc");
            } else {
              p->Emit(R"cc(
                using $superclass$::CopyFrom;
                inline void CopyFrom(const $classname$& from) {
                  $superclass$::CopyImpl(*this, from);
                }
                using $superclass$::MergeFrom;
                void MergeFrom(const $classname$& from) {
                  $superclass$::MergeImpl(*this, from);
                }

                public:
              )cc");
            }
          } else {
            p->Emit(R"cc(
              void CopyFrom(const $classname$& from);
              void MergeFrom(const $classname$& from) { $classname$::MergeImpl(*this, from); }

              private:
              static void MergeImpl($pb$::MessageLite& to_msg,
                                    const $pb$::MessageLite& from_msg);

              public:
            )cc");
          }

          if (NeedsIsInitialized()) {
            p->Emit(R"cc(
              bool IsInitialized() const {
                $WeakDescriptorSelfPin$;
                return IsInitializedImpl(*this);
              }

              private:
              static bool IsInitializedImpl(const MessageLite& msg);

              public:
            )cc");
          } else {
            p->Emit(R"cc(
              bool IsInitialized() const {
                $WeakDescriptorSelfPin$;
                return true;
              }
            )cc");
          }

          if (!HasSimpleBaseClass(descriptor_, options_)) {
            // In custom vtable mode, the functions are implemented as static
            // functions, which are the ones we put in the custom vtable. The
            // non-static functions are small trampolines. In normal mode, the
            // functions implemented are the non-static members which are a
            // virtual overrides. This reduces the number of functions in the
            // binary in both modes.
            p->Emit(R"cc(
              ABSL_ATTRIBUTE_REINITIALIZES void Clear() PROTOBUF_FINAL;
#if defined(PROTOBUF_CUSTOM_VTABLE)
              private:
              static ::size_t ByteSizeLong(const $pb$::MessageLite& msg);
              static $uint8$* $nonnull$ _InternalSerialize(
                  const $pb$::MessageLite& msg, $uint8$* $nonnull$ target,
                  $pb$::io::EpsCopyOutputStream* $nonnull$ stream);

              public:
              ::size_t ByteSizeLong() const { return ByteSizeLong(*this); }
              $uint8$* $nonnull$ _InternalSerialize(
                  $uint8$* $nonnull$ target,
                  $pb$::io::EpsCopyOutputStream* $nonnull$ stream) const {
                return _InternalSerialize(*this, target, stream);
              }
#else   // PROTOBUF_CUSTOM_VTABLE
              ::size_t ByteSizeLong() const final;
              $uint8$* $nonnull$ _InternalSerialize(
                  //~
                  $uint8$* $nonnull$ target,
                  $pb$::io::EpsCopyOutputStream* $nonnull$ stream) const final;
#endif  // PROTOBUF_CUSTOM_VTABLE
            )cc");
          }
        }},
       {"internal_field_number",
        [&] {
          if (!options_.field_listener_options.inject_field_listener_events)
            return;

          p->Emit({{"field_count", descriptor_->field_count()}}, R"cc(
            static constexpr int _kInternalFieldNumber = $field_count$;
          )cc");
        }},
       {"decl_non_simple_base",
        [&] {
          if (HasSimpleBaseClass(descriptor_, options_)) return;
          p->Emit(
              R"cc(
                int GetCachedSize() const { return $cached_size$.Get(); }

                private:
                void SharedCtor($pb$::Arena* $nullable$ arena);
                static void SharedDtor(MessageLite& self);
                void InternalSwap($classname$* $nonnull$ other);
              )cc");
        }},
       {"arena_dtor",
        [&] {
          switch (NeedsArenaDestructor()) {
            case ArenaDtorNeeds::kOnDemand:
              p->Emit(R"cc(
                private:
                static void ArenaDtor(void* $nonnull$ object);
                static void OnDemandRegisterArenaDtor(MessageLite& msg, $pb$::Arena& arena) {
                  auto& this_ = static_cast<$classname$&>(msg);
                  if ((this_.$inlined_string_donated_array$[0] & 0x1u) == 0) {
                    return;
                  }
                  this_.$inlined_string_donated_array$[0] &= 0xFFFFFFFEu;
                  arena.OwnCustomDestructor(&this_, &$classname$::ArenaDtor);
                }
              )cc");
              break;
            case ArenaDtorNeeds::kRequired:
              p->Emit(R"cc(
                private:
                static void ArenaDtor(void* $nonnull$ object);
              )cc");
              break;
            case ArenaDtorNeeds::kNone:
              break;
          }
        }},
       {"get_metadata",
        [&] {
          if (!HasDescriptorMethods(descriptor_->file(), options_)) return;

          p->Emit(R"cc(
            $pb$::Metadata GetMetadata() const;
          )cc");
        }},
       {"decl_split_methods",
        [&] {
          if (!ShouldSplit(descriptor_, options_)) return;
          p->Emit({{"default_name", DefaultInstanceName(descriptor_, options_,
                                                        /*split=*/true)}},
                  R"cc(
                    private:
                    inline bool IsSplitMessageDefault() const {
                      return $split$ == reinterpret_cast<const Impl_::Split*>(&$default_name$);
                    }
                    PROTOBUF_NOINLINE void PrepareSplitMessageForWrite();

                    public:
                  )cc");
        }},
       {"nested_types",
        [&] {
          // Import all nested message classes into this class's scope with
          // typedefs.
          for (int i = 0; i < descriptor_->nested_type_count(); ++i) {
            const Descriptor* nested_type = descriptor_->nested_type(i);
            if (!IsMapEntryMessage(nested_type)) {
              p->Emit(
                  {
                      Sub{"nested_full_name", ClassName(nested_type, false)}
                          .AnnotatedAs(nested_type),
                      Sub{"nested_name",
                          ResolveKnownNameCollisions(nested_type->name(),
                                                     NameContext::kMessage,
                                                     NameKind::kType)}
                          .AnnotatedAs(nested_type),
                  },
                  R"cc(
                    using $nested_name$ = $nested_full_name$;
                  )cc");
            }
          }
        }},
       {"nested_enums",
        [&] {
          // Import all nested enums and their values into this class's
          // scope with typedefs and constants.
          for (int i = 0; i < descriptor_->enum_type_count(); ++i) {
            enum_generators_[i]->GenerateSymbolImports(p);
          }
        }},
       {"decl_field_accessors",
        [&] {
          // Generate accessor methods for all fields.
          GenerateFieldAccessorDeclarations(p);
        }},
       {"decl_extension_ids",
        [&] {
          // Declare extension identifiers.
          for (int i = 0; i < descriptor_->extension_count(); ++i) {
            extension_generators_[i]->GenerateDeclaration(p);
          }
        }},
       {"proto2_message_sets",
        [&] {
        }},
       {"decl_set_has",
        [&] {
          for (auto field : FieldRange(descriptor_)) {
            // set_has_***() generated in all oneofs.
            if (!field->is_repeated() && !field->options().weak() &&
                field->real_containing_oneof()) {
              p->Emit({{"field_name", FieldName(field)}}, R"cc(
                void set_has_$field_name$();
              )cc");
            }
          }
        }},
       {"decl_oneof_has",
        [&] {
          // Generate oneof function declarations
          for (auto oneof : OneOfRange(descriptor_)) {
            p->Emit({{"oneof_name", oneof->name()}}, R"cc(
              inline bool has_$oneof_name$() const;
              inline void clear_has_$oneof_name$();
            )cc");
          }
        }},
       {"decl_data",
        [&] {
          parse_function_generator_->GenerateDataDecls(p);
        }},
       {"post_loop_handler",
        [&] {
          if (!NeedsPostLoopHandler(descriptor_, options_)) return;
          p->Emit(R"cc(
            static const char* $nullable$ PostLoopHandler(
                MessageLite* $nonnull$ msg, const char* $nullable$ ptr,
                $pbi$::ParseContext* $nonnull$ ctx);
          )cc");
        }},
       {"decl_impl", [&] { GenerateImplDefinition(p); }},
       {"classdata_type", ClassDataType(descriptor_, options_)},
       {"split_friend",
        [&] {
          if (!ShouldSplit(descriptor_, options_)) return;

          p->Emit({{"split_default", DefaultInstanceType(descriptor_, options_,
                                                         /*split=*/true)}},
                  R"cc(
                    friend struct $split_default$;
                  )cc");
        }}},
      R"cc(
        class $dllexport_decl $$classname$ final : public $superclass$
        /* @@protoc_insertion_point(class_definition:$full_name$) */ {
         public:
          inline $classname$() : $classname$(nullptr) {}
          $decl_dtor$;

#if defined(PROTOBUF_CUSTOM_VTABLE)
          //~ Define a derived `operator delete` to avoid dynamic dispatch when
          //~ the type is statically known
          void operator delete($classname$* $nonnull$ msg, ::std::destroying_delete_t) {
            SharedDtor(*msg);
            $pbi$::SizedDelete(msg, sizeof($classname$));
          }
#endif

          //~ Templatize constexpr constructor as a workaround for a bug in
          //~ gcc 12 (warning in gcc 13).
          template <typename = void>
          explicit PROTOBUF_CONSTEXPR $classname$($pbi$::ConstantInitialized);

          inline $classname$(const $classname$& from) : $classname$(nullptr, from) {}
          inline $classname$($classname$&& from) noexcept
              : $classname$(nullptr, ::std::move(from)) {}
          inline $classname$& operator=(const $classname$& from) {
            CopyFrom(from);
            return *this;
          }
          inline $classname$& operator=($classname$&& from) noexcept {
            if (this == &from) return *this;
            if ($pbi$::CanMoveWithInternalSwap(GetArena(), from.GetArena())) {
              InternalSwap(&from);
            } else {
              CopyFrom(from);
            }
            return *this;
          }
          $decl_verify_func$;

          inline const $unknown_fields_type$& unknown_fields() const
              ABSL_ATTRIBUTE_LIFETIME_BOUND {
            $annotate_unknown_fields$;
            return $unknown_fields$;
          }
          inline $unknown_fields_type$* $nonnull$ mutable_unknown_fields()
              ABSL_ATTRIBUTE_LIFETIME_BOUND {
            $annotate_mutable_unknown_fields$;
            return $mutable_unknown_fields$;
          }

          $descriptor_accessor$;
          $get_descriptor$;
          static const $classname$& default_instance() {
            return *reinterpret_cast<const $classname$*>(
                &_$classname$_default_instance_);
          }
          $decl_oneof$;
          static constexpr int kIndexInFileMessages = $index_in_file_messages$;
          $decl_any_methods$;
          friend void swap($classname$& a, $classname$& b) { a.Swap(&b); }
          inline void Swap($classname$* $nonnull$ other) {
            if (other == this) return;
            if ($pbi$::CanUseInternalSwap(GetArena(), other->GetArena())) {
              InternalSwap(other);
            } else {
              $pbi$::GenericSwap(this, other);
            }
          }
          void UnsafeArenaSwap($classname$* $nonnull$ other) {
            if (other == this) return;
            $DCHK$(GetArena() == other->GetArena());
            InternalSwap(other);
          }

          // implements Message ----------------------------------------------

          $classname$* $nonnull$ New($pb$::Arena* $nullable$ arena = nullptr) const {
            return $superclass$::DefaultConstruct<$classname$>(arena);
          }
          $generated_methods$;
          $internal_field_number$;
          $decl_non_simple_base$;
          //~ Friend the template function GetAnyMessageName<T>() so that it can
          //~ call this FullMessageName() method.
          //~ NOTE: parentheses around the symbol GetAnyMessageName is required
          //~       for compiler to resolve the symbol correctly and interpret
          //~       it as a function (instead of trying to find the symbol under
          //~       the absl::string_view namespace).
         private:
          template <typename T>
          friend ::absl::string_view($pbi$::GetAnyMessageName)();
          static ::absl::string_view FullMessageName() { return "$full_name$"; }
          $decl_annotate$;

          //~ TODO Make this private! Currently people are
          //~ deriving from protos to give access to this constructor,
          //~ breaking the invariants we rely on.
         protected:
          explicit $classname$($pb$::Arena* $nullable$ arena);
          $classname$($pb$::Arena* $nullable$ arena, const $classname$& from);
          $classname$(
              //~
              $pb$::Arena* $nullable$ arena, $classname$&& from) noexcept
              : $classname$(arena) {
            *this = ::std::move(from);
          }
          $arena_dtor$;
          const $pbi$::ClassData* $nonnull$ GetClassData() const PROTOBUF_FINAL;
          static void* $nonnull$ PlacementNew_(
              //~
              const void* $nonnull$, void* $nonnull$ mem,
              $pb$::Arena* $nullable$ arena);
          static constexpr auto InternalNewImpl_();

         public:
          //~ We need this in the public section to call it from the initializer
          //~ of T_class_data_. However, since it is `constexpr` and has an
          //~ `auto` return type it is not callable from outside the .pb.cc
          //~ without a definition so it is effectively private.
          static constexpr auto InternalGenerateClassData_();

          $get_metadata$;
          $decl_split_methods$;
          // nested types ----------------------------------------------------
          $nested_types$;
          $nested_enums$;

          // accessors -------------------------------------------------------
          $decl_field_accessors$;
          $decl_extension_ids$;
          $proto2_message_sets$;
          // @@protoc_insertion_point(class_scope:$full_name$)
          //~ Generate private members.
         private:
          //~ TODO: Remove hack to track field access and remove
          //~ this class.
          class _Internal;
          $decl_set_has$;
          $decl_oneof_has$;
          $decl_data$;
          $post_loop_handler$;

          friend class $pb$::MessageLite;
          friend class $pb$::Arena;
          template <typename T>
          friend class $pb$::Arena::InternalHelper;
          using InternalArenaConstructable_ = void;
          using DestructorSkippable_ = void;
          $decl_impl$;
          $split_friend$;
          //~ The TableStruct struct needs access to the private parts, in
          //~ order to construct the offsets of all members.
          friend struct ::$tablename$;
        };

        $dllexport_decl $extern const $pbi$::$classdata_type$ $classname$_class_data_;
      )cc");
}  // NOLINT(readability/fn_size)

void MessageGenerator::GenerateInlineMethods(io::Printer* p) {
  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  if (IsMapEntryMessage(descriptor_)) return;
  GenerateFieldAccessorDefinitions(p);

  // Generate oneof_case() functions.
  for (auto oneof : OneOfRange(descriptor_)) {
    p->Emit(
        {
            Sub{"oneof_name", absl::StrCat(oneof->name(), "_case")}.AnnotatedAs(
                oneof),
            {"OneofName",
             absl::StrCat(UnderscoresToCamelCase(oneof->name(), true), "Case")},
            {"oneof_index", oneof->index()},
        },
        R"cc(
          inline $classname$::$OneofName$ $classname$::$oneof_name$() const {
            return $classname$::$OneofName$($oneof_case$[$oneof_index$]);
          }
        )cc");
  }
}

void MessageGenerator::GenerateSchema(io::Printer* p, int offset) {
  auto v = p->WithVars(ClassVars(descriptor_, options_));
  p->Emit(
      {
          {"offset", offset},
      },
      R"cc(
        {$offset$, sizeof($classtype$)},
      )cc");
}

void MessageGenerator::GenerateClassMethods(io::Printer* p) {
  if (!ShouldGenerateClass(descriptor_, options_)) return;

  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));

  if (IsMapEntryMessage(descriptor_)) {
    p->Emit({{"annotate_accessors",
              [&] {
                if (!options_.annotate_accessor) return;
                for (auto f : FieldRange(descriptor_)) {
                  p->Emit({{"field", FieldName(f)}},
                          R"cc(
                            volatile bool $classname$::$field$_AccessedNoStrip;
                          )cc");
                }
              }},
             {"verify",
              [&] {
                // Delegates generating verify function as only a subset of map
                // entry messages need it; i.e. UTF8 string key/value or message
                // type value.
                GenerateVerify(p);
              }},
             {"class_data", [&] { GenerateClassData(p); }}},
            R"cc(
#if defined(PROTOBUF_CUSTOM_VTABLE)
              $classname$::$classname$()
                  : SuperType($classname$_class_data_.base()) {}
              $classname$::$classname$($pb$::Arena* $nullable$ arena)
                  : SuperType(arena, $classname$_class_data_.base()) {}
#else   // PROTOBUF_CUSTOM_VTABLE
              $classname$::$classname$() : SuperType() {}
              $classname$::$classname$($pb$::Arena* $nullable$ arena) : SuperType(arena) {}
#endif  // PROTOBUF_CUSTOM_VTABLE
              $annotate_accessors$;
              $verify$;
              $class_data$;
            )cc");
    parse_function_generator_->GenerateDataDefinitions(p);
    return;
  }
  if (IsAnyMessage(descriptor_)) {
    p->Emit({{"any_field_descriptor",
              [&] {
                if (!HasDescriptorMethods(descriptor_->file(), options_)) {
                  return;
                }
                p->Emit(
                    R"cc(
                      bool $classname$::GetAnyFieldDescriptors(
                          const $pb$::Message& message,
                          const $pb$::FieldDescriptor** type_url_field,
                          const $pb$::FieldDescriptor** value_field) {
                        return ::_pbi::GetAnyFieldDescriptors(message, type_url_field, value_field);
                      }
                    )cc");
              }}},
            R"cc(
              $any_field_descriptor$;
              bool $classname$::ParseAnyTypeUrl(
                  //~
                  ::absl::string_view type_url,
                  std::string* $nonnull$ full_type_name) {
                return ::_pbi::ParseAnyTypeUrl(type_url, full_type_name);
              }
            )cc");
  }
  p->Emit(
      {{"has_bit",
        [&] {
          if (has_bit_indices_.empty()) return;
          p->Emit(
              R"cc(
                using HasBits =
                    decltype(::std::declval<$classname$>().$has_bits$);
                static constexpr ::int32_t kHasBitsOffset =
                    8 * PROTOBUF_FIELD_OFFSET($classname$, _impl_._has_bits_);
              )cc");
        }},
       {"oneof",
        [&] {
          if (descriptor_->real_oneof_decl_count() == 0) return;
          p->Emit(
              R"cc(
                static constexpr ::int32_t kOneofCaseOffset =
                    PROTOBUF_FIELD_OFFSET($classtype$, $oneof_case$);
              )cc");
        }},
       {"required",
        [&] {
          if (num_required_fields_ == 0) return;
          const std::vector<uint32_t> masks_for_has_bits =
              RequiredFieldsBitMask();
          p->Emit(
              {{"check_bit_mask", ConditionalToCheckBitmasks(
                                      masks_for_has_bits, false, "has_bits")}},
              R"cc(
                static bool MissingRequiredFields(const HasBits& has_bits) {
                  return $check_bit_mask$;
                }
              )cc");
        }}},
      R"cc(
        class $classname$::_Internal {
         public:
          $has_bit$;
          $oneof$;
          $required$;
        };
      )cc");
  p->Emit("\n");

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
  p->Emit("\n");

  if (descriptor_->real_oneof_decl_count() > 0) {
    GenerateOneofClear(p);
    p->Emit("\n");
  }

  GenerateClassData(p);
  parse_function_generator_->GenerateDataDefinitions(p);

  if (HasGeneratedMethods(descriptor_->file(), options_)) {
    GenerateClear(p);
    p->Emit("\n");

    GenerateSerializeWithCachedSizesToArray(p);
    p->Emit("\n");

    GenerateByteSize(p);
    p->Emit("\n");


    GenerateClassSpecificMergeImpl(p);
    p->Emit("\n");

    GenerateCopyFrom(p);
    p->Emit("\n");

    GenerateIsInitialized(p);
    p->Emit("\n");
  }

  if (ShouldSplit(descriptor_, options_)) {
    p->Emit({{"split_default",
              DefaultInstanceName(descriptor_, options_, /*split=*/true)},
             {"default",
              DefaultInstanceName(descriptor_, options_, /*split=*/false)}},
            R"cc(
              void $classname$::PrepareSplitMessageForWrite() {
                if (ABSL_PREDICT_TRUE(IsSplitMessageDefault())) {
                  void* chunk = $pbi$::CreateSplitMessageGeneric(
                      GetArena(), &$split_default$, sizeof(Impl_::Split), this,
                      &$default$);
                  $split$ = reinterpret_cast<Impl_::Split*>(chunk);
                }
              }
            )cc");
  }

  GenerateVerify(p);

  GenerateSwap(p);
  p->Emit("\n");

  p->Emit({{"annotate_accessor_definition",
            [&] {
              if (!options_.annotate_accessor) return;
              for (auto f : FieldRange(descriptor_)) {
                p->Emit({{"field", FieldName(f)}},
                        R"cc(
                          volatile bool $classname$::$field$_AccessedNoStrip;
                        )cc");
              }
            }},
           {"get_metadata",
            [&] {
              if (!HasDescriptorMethods(descriptor_->file(), options_)) return;
              // Same as the base class, but it avoids virtual dispatch.
              p->Emit(R"cc(
                $pb$::Metadata $classname$::GetMetadata() const {
                  return $superclass$::GetMetadataImpl(GetClassData()->full());
                }
              )cc");
            }},
           {"post_loop_handler",
            [&] {
              if (!NeedsPostLoopHandler(descriptor_, options_)) return;
              p->Emit({{"required",
                        [&] {
                        }}},
                      R"cc(
                        const char* $nullable$ $classname$::PostLoopHandler(
                            MessageLite* $nonnull$ msg,
                            const char* $nullable$ ptr,
                            ::_pbi::ParseContext* $nonnull$ ctx) {
                          $classname$* _this = static_cast<$classname$*>(msg);
                          $annotate_deserialize$;
                          $required$;
                          return ptr;
                        }
                      )cc");
            }},
           {"message_set_definition",
            [&] {
            }},
           {"tracker_decl",
            [&] {
              if (!HasTracker(descriptor_, options_)) return;
              p->Emit(R"cc(
                $pb$::AccessListener<$classtype$> $classname$::$tracker$(
                    &FullMessageName);
              )cc");
            }}},
          R"cc(
            $annotate_accessor_definition$;
            $get_metadata$;
            $post_loop_handler$;
            $message_set_definition$;
            $tracker_decl$;
          )cc");
}

size_t MessageGenerator::GenerateOffsets(io::Printer* p) {
  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  Formatter format(p);

  int num_generated_indices = 1;
  const auto make_bitmap = [&](auto... bits) {
    uint32_t res = 0;
    int index = 0;
    ((res |= (static_cast<uint32_t>(bits) << index++)), ...);
    ((num_generated_indices += bits), ...);
    return absl::StrCat("0x", absl::Hex(res, absl::kZeroPad3));
  };

  const bool has_has_bits =
      !has_bit_indices_.empty() || IsMapEntryMessage(descriptor_);
  const bool has_extensions = descriptor_->extension_range_count() > 0;
  const bool has_oneofs = descriptor_->real_oneof_decl_count() > 0;
  const bool has_weak_fields = num_weak_fields_ > 0;
  const bool has_inline_strings = !inlined_string_indices_.empty();
  const bool has_split = ShouldSplit(descriptor_, options_);

  format("$1$, // bitmap\n",
         // These conditions have to match exactly the order done below
         make_bitmap(has_has_bits, has_extensions, has_oneofs, has_weak_fields,
                     has_inline_strings, has_split, has_split, has_has_bits,
                     has_inline_strings));

  // The order of these offsets has to match the reading of them in
  // MigrationToReflectionSchema.
  if (has_has_bits) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, $has_bits$),\n");
  }
  if (has_extensions) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, $extensions$),\n");
  }
  if (has_oneofs) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, $oneof_case$[0]),\n");
  }
  if (has_weak_fields) {
    format("PROTOBUF_FIELD_OFFSET($classtype$, $weak_field_map$),\n");
  }
  if (has_inline_strings) {
    format(
        "PROTOBUF_FIELD_OFFSET($classtype$, "
        "$inlined_string_donated_array$),\n");
  }
  if (has_split) {
    format(
        "PROTOBUF_FIELD_OFFSET($classtype$, $split$),\n"
        "sizeof($classtype$::Impl_::Split),\n");
  }
  const size_t offsets = num_generated_indices + descriptor_->field_count() +
                         descriptor_->real_oneof_decl_count();
  if (has_has_bits) {
    format("$1$, // hasbit index offset\n", offsets);
  }
  if (has_inline_strings) {
    format("$1$, // inline string index offset\n",
           offsets + has_bit_indices_.size());
  }
  size_t entries = offsets;
  for (auto field : FieldRange(descriptor_)) {
    // TODO: We should not have an entry in the offset table for fields
    // that do not use them.
    if (field->options().weak()) {
      // Mark the field to prevent unintentional access through reflection.
      // Don't use the top bit because that is for unused fields.
      format("::_pbi::kInvalidFieldOffsetTag");
    } else if (field->real_containing_oneof()) {
      format("PROTOBUF_FIELD_OFFSET($classtype$, _impl_.$1$_)",
             field->real_containing_oneof()->name());
    } else {
      format("PROTOBUF_FIELD_OFFSET($classtype$$1$, $2$)",
             ShouldSplit(field, options_) ? "::Impl_::Split" : "",
             ShouldSplit(field, options_)
                 ? absl::StrCat(FieldName(field), "_")
                 : FieldMemberName(field, /*split=*/false));
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
      format(" | ::_pbi::kSplitFieldOffsetMask");
    }
    if (IsEagerlyVerifiedLazy(field, options_, scc_analyzer_)) {
      format(" | ::_pbi::kLazyMask");
    } else if (IsStringInlined(field, options_)) {
      format(" | ::_pbi::kInlinedMask");
    } else if (IsMicroString(field, options_)) {
      format(" | ::_pbi::kMicroStringMask");
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
    for (size_t i = 0; i < has_bit_indices_.size(); ++i) {
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

  return entries;
}

void MessageGenerator::GenerateZeroInitFields(io::Printer* p) const {
  using Iterator = decltype(optimized_order_.begin());
  const FieldDescriptor* first = nullptr;
  auto emit_pending_zero_fields = [&](Iterator end) {
    if (first != nullptr) {
      const FieldDescriptor* last = end[-1];
      if (first != last ||
          !field_generators_.get(first).has_brace_default_assign()) {
        p->Emit({{"first", FieldName(first)},
                 {"last", FieldName(last)},
                 {"Impl", "Impl_"},
                 {"impl", "_impl_"}},
                R"cc(
                  ::memset(reinterpret_cast<char *>(&$impl$) +
                               offsetof($Impl$, $first$_),
                           0,
                           offsetof($Impl$, $last$_) -
                               offsetof($Impl$, $first$_) +
                               sizeof($Impl$::$last$_));
                )cc");
      } else {
        p->Emit({{"field", FieldMemberName(first, false)}},
                R"cc(
                  $field$ = {};
                )cc");
      }
      first = nullptr;
    }
  };

  auto it = optimized_order_.begin();
  auto end = optimized_order_.end();
  for (; it != end && !ShouldSplit(*it, options_); ++it) {
    auto const& generator = field_generators_.get(*it);
    if (generator.has_trivial_zero_default()) {
      if (first == nullptr) first = *it;
    } else {
      emit_pending_zero_fields(it);
    }
  }
  emit_pending_zero_fields(it);
}

namespace {

class MemberInitSeparator {
 public:
  explicit MemberInitSeparator(io::Printer* printer) : printer_(printer) {}
  MemberInitSeparator(const MemberInitSeparator&) = delete;

  ~MemberInitSeparator() {
    if (separators_) printer_->Outdent();
  }

  void operator()() {
    if (separators_) {
      printer_->Emit(",\n");
    } else {
      printer_->Emit(": ");
      printer_->Indent();
      separators_ = true;
    }
  }

 private:
  bool separators_ = false;
  io::Printer* const printer_;
};

}  // namespace

void MessageGenerator::GenerateImplMemberInit(io::Printer* p,
                                              InitType init_type) {
  ABSL_DCHECK(!HasSimpleBaseClass(descriptor_, options_));

  auto indent = p->WithIndent();
  MemberInitSeparator separator(p);

  auto init_extensions = [&] {
    if (descriptor_->extension_range_count() > 0 &&
        init_type != InitType::kConstexpr) {
      separator();
      p->Emit("_extensions_{visibility, arena}");
    }
  };

  auto init_inlined_string_indices = [&] {
    if (!inlined_string_indices_.empty()) {
      bool dtor_on_demand = NeedsArenaDestructor() == ArenaDtorNeeds::kOnDemand;
      auto values = [&] {
        for (size_t i = 0; i < InlinedStringDonatedSize(); ++i) {
          if (i > 0) {
            p->Emit(", ");
          }
          p->Emit(dtor_on_demand
                      ? "::_pbi::InitDonatingStates()"
                      : "::_pbi::InitDonatingStates() & 0xFFFFFFFEu");
        }
      };
      separator();
      p->Emit({{"values", values}}, "_inlined_string_donated_{$values$}");
    }
  };

  auto init_has_bits = [&] {
    if (!has_bit_indices_.empty()) {
      if (init_type == InitType::kArenaCopy) {
        separator();
        p->Emit("_has_bits_{from._has_bits_}");
      }
      separator();
      p->Emit("_cached_size_{0}");
    }
  };

  auto init_fields = [&] {
    for (auto* field : optimized_order_) {
      if (ShouldSplit(field, options_)) continue;

      auto const& generator = field_generators_.get(field);
      switch (init_type) {
        case InitType::kConstexpr:
          separator();
          generator.GenerateMemberConstexprConstructor(p);
          break;
        case InitType::kArena:
          if (!generator.has_trivial_zero_default()) {
            separator();
            generator.GenerateMemberConstructor(p);
          }
          break;
        case InitType::kArenaCopy:
          if (!generator.has_trivial_value()) {
            separator();
            generator.GenerateMemberCopyConstructor(p);
          }
          break;
      }
    }
  };

  auto init_split = [&] {
    if (ShouldSplit(descriptor_, options_)) {
      separator();
      p->Emit({{"name", DefaultInstanceName(descriptor_, options_, true)}},
              "_split_{const_cast<Split*>(&$name$._instance)}");
    }
  };

  auto init_oneofs = [&] {
    for (auto oneof : OneOfRange(descriptor_)) {
      separator();
      p->Emit({{"name", oneof->name()}}, "$name$_{}");
    }
  };

  auto init_cached_size_if_no_hasbits = [&] {
    if (has_bit_indices_.empty()) {
      separator();
      p->Emit("_cached_size_{0}");
    }
  };

  auto init_oneof_cases = [&] {
    if (int count = descriptor_->real_oneof_decl_count()) {
      separator();
      if (init_type == InitType::kArenaCopy) {
        auto cases = [&] {
          for (int i = 0; i < count; ++i) {
            p->Emit({{"index", i}, {"comma", i ? ", " : ""}},
                    "$comma$from._oneof_case_[$index$]");
          }
        };
        p->Emit({{"cases", cases}}, "_oneof_case_{$cases$}");
      } else {
        p->Emit("_oneof_case_{}");
      }
    }
  };

  auto init_weak_field_map = [&] {
    if (num_weak_fields_ && init_type != InitType::kConstexpr) {
      separator();
      if (init_type == InitType::kArenaCopy) {
        p->Emit("_weak_field_map_{visibility, arena, from._weak_field_map_}");
      } else {
        p->Emit("_weak_field_map_{visibility, arena}");
      }
    }
  };

  // Initialization order of the various fields inside `_impl_(...)`
  init_extensions();
  init_inlined_string_indices();
  init_has_bits();
  init_fields();
  init_split();
  init_oneofs();
  init_cached_size_if_no_hasbits();
  init_oneof_cases();
  init_weak_field_map();
}

void MessageGenerator::GenerateSharedConstructorCode(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;

  // Generate Impl_::Imp_(visibility, Arena*);
  p->Emit({{"init_impl", [&] { GenerateImplMemberInit(p, InitType::kArena); }},
           {"zero_init", [&] { GenerateZeroInitFields(p); }}},
          R"cc(
            PROTOBUF_NDEBUG_INLINE $classname$::Impl_::Impl_(
                [[maybe_unused]] $pbi$::InternalVisibility visibility,
                [[maybe_unused]] $pb$::Arena* $nullable$ arena)
                //~
                $init_impl$ {}

            inline void $classname$::SharedCtor(::_pb::Arena* $nullable$ arena) {
              new (&_impl_) Impl_(internal_visibility(), arena);
              $zero_init$;
            }
          )cc");
}

void MessageGenerator::GenerateInitDefaultSplitInstance(io::Printer* p) {
  if (!ShouldSplit(descriptor_, options_)) return;

  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  p->Emit("\n");
  for (const auto* field : optimized_order_) {
    if (ShouldSplit(field, options_)) {
      field_generators_.get(field).GenerateConstexprAggregateInitializer(p);
    }
  }
}

void MessageGenerator::GenerateSharedDestructorCode(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  auto emit_field_dtors = [&](bool split_fields) {
    // Write the destructors for each field except oneof members.
    // optimized_order_ does not contain oneof fields.
    for (const auto* field : optimized_order_) {
      if (ShouldSplit(field, options_) != split_fields) continue;
      field_generators_.get(field).GenerateDestructorCode(p);
    }
  };
  p->Emit(
      {
          {"has_bit_consistency",
           [&] { GenerateVerifyHasBitConsistency(p, "this_."); }},
          {"field_dtors", [&] { emit_field_dtors(/* split_fields= */ false); }},
          {"split_field_dtors",
           [&] {
             if (!ShouldSplit(descriptor_, options_)) return;
             p->Emit(
                 {
                     {"split_field_dtors_impl",
                      [&] { emit_field_dtors(/* split_fields= */ true); }},
                 },
                 R"cc(
                   if (ABSL_PREDICT_FALSE(!this_.IsSplitMessageDefault())) {
                     auto* $cached_split_ptr$ = this_.$split$;
                     $split_field_dtors_impl$;
                     delete $cached_split_ptr$;
                   }
                 )cc");
           }},
          {"oneof_field_dtors",
           [&] {
             for (const auto* oneof : OneOfRange(descriptor_)) {
               p->Emit({{"name", oneof->name()}},
                       R"cc(
                         if (this_.has_$name$()) {
                           this_.clear_$name$();
                         }
                       )cc");
             }
           }},
          {"weak_fields_dtor",
           [&] {
             if (num_weak_fields_ == 0) return;
             // Generate code to destruct oneofs. Clearing should do the work.
             p->Emit(R"cc(
               this_.$weak_field_map$.ClearAll();
             )cc");
           }},
          {"impl_dtor", [&] { p->Emit("this_._impl_.~Impl_();\n"); }},
      },
      R"cc(
        inline void $classname$::SharedDtor(MessageLite& self) {
          $classname$& this_ = static_cast<$classname$&>(self);
          this_._internal_metadata_.Delete<$unknown_fields_type$>();
          $DCHK$(this_.GetArena() == nullptr);
          $has_bit_consistency$;
          $WeakDescriptorSelfPin$;
          $field_dtors$;
          $split_field_dtors$;
          $oneof_field_dtors$;
          $weak_fields_dtor$;
          $impl_dtor$;
        }
      )cc");
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
  auto emit_field_dtors = [&](bool split_fields) {
    // Write the destructors for each field except oneof members.
    // optimized_order_ does not contain oneof fields.
    for (const auto* field : optimized_order_) {
      if (ShouldSplit(field, options_) != split_fields) continue;
      field_generators_.get(field).GenerateArenaDestructorCode(p);
    }
  };
  bool needs_arena_dtor_split = false;
  for (const auto* field : optimized_order_) {
    if (!ShouldSplit(field, options_)) continue;
    if (field_generators_.get(field).NeedsArenaDestructor() >
        ArenaDtorNeeds::kNone) {
      needs_arena_dtor_split = true;
      break;
    }
  }

  // This code is placed inside a static method, rather than an ordinary one,
  // since that simplifies Arena's destructor list (ordinary function pointers
  // rather than member function pointers). _this is the object being
  // destructed.
  p->Emit(
      {
          {"field_dtors", [&] { emit_field_dtors(/* split_fields= */ false); }},
          {"split_field_dtors",
           [&] {
             if (!ShouldSplit(descriptor_, options_)) return;
             if (!needs_arena_dtor_split) {
               return;
             }
             p->Emit(
                 {
                     {"split_field_dtors_impl",
                      [&] { emit_field_dtors(/* split_fields= */ true); }},
                 },
                 R"cc(
                   if (ABSL_PREDICT_FALSE(!_this->IsSplitMessageDefault())) {
                     $split_field_dtors_impl$;
                   }
                 )cc");
           }},
          {"oneof_field_dtors",
           [&] {
             for (const auto* oneof : OneOfRange(descriptor_)) {
               for (const auto* field : FieldRange(oneof)) {
                 field_generators_.get(field).GenerateArenaDestructorCode(p);
               }
             }
           }},
      },
      R"cc(
        void $classname$::ArenaDtor(void* $nonnull$ object) {
          $classname$* _this = reinterpret_cast<$classname$*>(object);
          $field_dtors$;
          $split_field_dtors$;
          $oneof_field_dtors$;
        }
      )cc");
}

void MessageGenerator::GenerateConstexprConstructor(io::Printer* p) {
  if (!ShouldGenerateClass(descriptor_, options_)) return;

  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  auto c = p->WithVars({{"constexpr", "PROTOBUF_CONSTEXPR"}});
  Formatter format(p);

  if (IsMapEntryMessage(descriptor_) || !HasImplData(descriptor_, options_)) {
    p->Emit({{"base",
              [&] {
                if (IsMapEntryMessage(descriptor_)) {
                  p->Emit("$classname$::MapEntry");
                } else {
                  p->Emit("$superclass$");
                }
              }}},
            R"cc(
              //~ Templatize constexpr constructor as a workaround for a bug in
              //~ gcc 12 (warning in gcc 13).
              template <typename>
              $constexpr$ $classname$::$classname$(::_pbi::ConstantInitialized)
#if defined(PROTOBUF_CUSTOM_VTABLE)
                  : $base$($classname$_class_data_.base()){}
#else   // PROTOBUF_CUSTOM_VTABLE
                  : $base$() {
              }
#endif  // PROTOBUF_CUSTOM_VTABLE
            )cc");
    return;
  }

  // Generate Impl_::Imp_(::_pbi::ConstantInitialized);
  // We use separate p->Emit() calls for LF and #ifdefs as they result in
  // awkward layout and more awkward indenting of the function statement.
  p->Emit("\n");
  p->Emit({{"init", [&] { GenerateImplMemberInit(p, InitType::kConstexpr); }}},
          R"cc(
            inline constexpr $classname$::Impl_::Impl_(
                ::_pbi::ConstantInitialized) noexcept
                //~
                $init$ {}
          )cc");
  p->Emit("\n");

  p->Emit(
      R"cc(
        template <typename>
        $constexpr$ $classname$::$classname$(::_pbi::ConstantInitialized)
#if defined(PROTOBUF_CUSTOM_VTABLE)
            : $superclass$($classname$_class_data_.base()),
#else   // PROTOBUF_CUSTOM_VTABLE
            : $superclass$(),
#endif  // PROTOBUF_CUSTOM_VTABLE
              _impl_(::_pbi::ConstantInitialized()) {
        }
      )cc");
}

bool MessageGenerator::CanUseTrivialCopy() const {
  if (ShouldSplit(descriptor_, options_)) return false;
  if (HasSimpleBaseClass(descriptor_, options_)) return false;
  if (descriptor_->extension_range_count() > 0) return false;
  if (num_weak_fields_ > 0) return false;

  // If all fields are trivially copyable then we can use the trivial copy
  // constructor of Impl_
  for (const auto* field : FieldRange(descriptor_)) {
    if (!field_generators_.get(field).has_trivial_copy()) return false;
  }

  return true;
}

void MessageGenerator::GenerateCopyInitFields(io::Printer* p) const {
  auto begin = optimized_order_.begin();
  auto end = optimized_order_.end();
  const FieldDescriptor* first = nullptr;

  auto emit_pending_copy_fields = [&](decltype(end) itend, bool split) {
    if (first != nullptr) {
      const FieldDescriptor* last = itend[-1];
      if (first != last) {
        p->Emit({{"first", FieldName(first)},
                 {"last", FieldName(last)},
                 {"Impl", split ? "Impl_::Split" : "Impl_"},
                 {"pdst", split ? "_impl_._split_" : "&_impl_"},
                 {"psrc", split ? "from._impl_._split_" : "&from._impl_"}},
                R"cc(
                  ::memcpy(reinterpret_cast<char *>($pdst$) +
                               offsetof($Impl$, $first$_),
                           reinterpret_cast<const char *>($psrc$) +
                               offsetof($Impl$, $first$_),
                           offsetof($Impl$, $last$_) -
                               offsetof($Impl$, $first$_) +
                               sizeof($Impl$::$last$_));
                )cc");
      } else {
        p->Emit({{"field", FieldMemberName(first, split)}},
                R"cc(
                  $field$ = from.$field$;
                )cc");
      }
      first = nullptr;
    }
  };

  int has_bit_word_index = -1;
  auto load_has_bits = [&](const FieldDescriptor* field) {
    if (has_bit_indices_.empty()) return;
    int has_bit_index = has_bit_indices_[field->index()];
    if (has_bit_word_index != has_bit_index / 32) {
      p->Emit({{"declare", has_bit_word_index < 0 ? "::uint32_t " : ""},
               {"index", has_bit_index / 32}},
              "$declare$cached_has_bits = _impl_._has_bits_[$index$];\n");
      has_bit_word_index = has_bit_index / 32;
    }
  };

  auto has_message = [&](const FieldDescriptor* field) {
    if (has_bit_indices_.empty()) {
      p->Emit("from.$field$ != nullptr");
    } else {
      int has_bit_index = has_bit_indices_[field->index()];
      p->Emit({{"condition", GenerateConditionMaybeWithProbabilityForField(
                                 has_bit_index, field, options_)}},
              "$condition$");
    }
  };

  auto emit_copy_message = [&](const FieldDescriptor* field) {
    load_has_bits(field);
    p->Emit({{"has_msg", [&] { has_message(field); }},
             {"submsg", FieldMessageTypeName(field, options_)}},
            R"cc(
              $field$ = ($has_msg$)
                            ? $superclass$::CopyConstruct(arena, *from.$field$)
                            : nullptr;
            )cc");
  };

  auto generate_copy_fields = [&] {
    for (auto it = begin; it != end; ++it) {
      const auto& gen = field_generators_.get(*it);
      auto v = p->WithVars(FieldVars(*it, options_));

      // Non trivial field values are copy constructed
      if (!gen.has_trivial_value() || gen.should_split()) {
        emit_pending_copy_fields(it, false);
        continue;
      }

      if (gen.is_message()) {
        emit_pending_copy_fields(it, false);
        emit_copy_message(*it);
      } else if (first == nullptr) {
        first = *it;
      }
    }
    emit_pending_copy_fields(end, false);
  };

  auto generate_copy_split_fields = [&] {
    for (auto it = begin; it != end; ++it) {
      const auto& gen = field_generators_.get(*it);
      auto v = p->WithVars(FieldVars(*it, options_));

      if (!gen.should_split()) {
        emit_pending_copy_fields(it, true);
        continue;
      }

      if (gen.is_trivial()) {
        if (first == nullptr) first = *it;
      } else {
        emit_pending_copy_fields(it, true);
        gen.GenerateCopyConstructorCode(p);
      }
    }
    emit_pending_copy_fields(end, true);
  };

  auto generate_copy_oneof_fields = [&]() {
    for (const auto* oneof : OneOfRange(descriptor_)) {
      p->Emit(
          {{"name", oneof->name()},
           {"NAME", absl::AsciiStrToUpper(oneof->name())},
           {"cases",
            [&] {
              for (const auto* field : FieldRange(oneof)) {
                p->Emit(
                    {{"Name", UnderscoresToCamelCase(field->name(), true)},
                     {"field", FieldMemberName(field, /*split=*/false)},
                     {"body",
                      [&] {
                        field_generators_.get(field).GenerateOneofCopyConstruct(
                            p);
                      }}},
                    R"cc(
                      case k$Name$:
                        $body$;
                        break;
                    )cc");
              }
            }}},
          R"cc(
            switch ($name$_case()) {
              case $NAME$_NOT_SET:
                break;
                $cases$;
            }
          )cc");
    }
  };

  if (descriptor_->extension_range_count() > 0) {
    p->Emit(R"cc(
      _impl_._extensions_.MergeFrom(this, from._impl_._extensions_);
    )cc");
  }

  p->Emit({{"copy_fields", generate_copy_fields},
           {"copy_oneof_fields", generate_copy_oneof_fields}},
          R"cc(
            $copy_fields$;
            $copy_oneof_fields$;
          )cc");

  if (ShouldSplit(descriptor_, options_)) {
    p->Emit({{"copy_split_fields", generate_copy_split_fields}},
            R"cc(
              if (ABSL_PREDICT_FALSE(!from.IsSplitMessageDefault())) {
                PrepareSplitMessageForWrite();
                $copy_split_fields$;
              }
            )cc");
  }
}

void MessageGenerator::GenerateArenaEnabledCopyConstructor(io::Printer* p) {
  if (!HasSimpleBaseClass(descriptor_, options_)) {
    // Generate Impl_::Imp_(visibility, Arena*, const& from);
    p->Emit(
        {{"init", [&] { GenerateImplMemberInit(p, InitType::kArenaCopy); }}},
        R"cc(
          PROTOBUF_NDEBUG_INLINE $classname$::Impl_::Impl_(
              [[maybe_unused]] $pbi$::InternalVisibility visibility,
              [[maybe_unused]] $pb$::Arena* $nullable$ arena, const Impl_& from,
              [[maybe_unused]] const $classtype$& from_msg)
              //~
              $init$ {}
        )cc");
    p->Emit("\n");
  }

  auto copy_construct_impl = [&] {
    if (!HasSimpleBaseClass(descriptor_, options_)) {
      p->Emit(R"cc(
        new (&_impl_) Impl_(internal_visibility(), arena, from._impl_, from);
      )cc");
    }
  };

  auto force_allocation = [&] {
    if (ShouldForceAllocationOnConstruction(descriptor_, options_)) {
      p->Emit(R"cc(
        if ($pbi$::DebugHardenForceAllocationOnConstruction()) {
          $mutable_unknown_fields$;
        }
      )cc");
    }
  };

  auto maybe_register_arena_dtor = [&] {
    switch (NeedsArenaDestructor()) {
      case ArenaDtorNeeds::kRequired: {
        p->Emit(R"cc(
          if (arena != nullptr) {
            arena->OwnCustomDestructor(this, &$classname$::ArenaDtor);
          }
        )cc");
        break;
      }
      case ArenaDtorNeeds::kOnDemand: {
        p->Emit(R"cc(
          ::_pbi::InternalRegisterArenaDtor(arena, this,
                                            &$classname$::ArenaDtor);
        )cc");
        break;
      }
      case ArenaDtorNeeds::kNone:
        break;
    }
  };

  p->Emit({{"copy_construct_impl", copy_construct_impl},
           {"copy_init_fields", [&] { GenerateCopyInitFields(p); }},
           {"force_allocation", force_allocation},
           {"maybe_register_arena_dtor", maybe_register_arena_dtor}},
          R"cc(
            $classname$::$classname$(
                //~ force alignment
                $pb$::Arena* $nullable$ arena,
                //~ force alignment
                const $classname$& from)
#if defined(PROTOBUF_CUSTOM_VTABLE)
                : $superclass$(arena, $classname$_class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
                : $superclass$(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
              $classname$* const _this = this;
              (void)_this;
              _internal_metadata_.MergeFrom<$unknown_fields_type$>(
                  from._internal_metadata_);
              $copy_construct_impl$;
              $copy_init_fields$;
              $force_allocation$;
              $maybe_register_arena_dtor$;

              // @@protoc_insertion_point(copy_constructor:$full_name$)
            }
          )cc");
}

void MessageGenerator::GenerateStructors(io::Printer* p) {
  p->Emit(
      {
          {"superclass", SuperClassName(descriptor_, options_)},
          {"ctor_body",
           [&] {
             if (HasSimpleBaseClass(descriptor_, options_)) return;
             p->Emit(R"cc(SharedCtor(arena);)cc");
             switch (NeedsArenaDestructor()) {
               case ArenaDtorNeeds::kRequired: {
                 p->Emit(R"cc(
                   if (arena != nullptr) {
                     arena->OwnCustomDestructor(this, &$classname$::ArenaDtor);
                   }
                 )cc");
                 break;
               }
               case ArenaDtorNeeds::kOnDemand: {
                 p->Emit(R"cc(
                   ::_pbi::InternalRegisterArenaDtor(arena, this,
                                                     &$classname$::ArenaDtor);
                 )cc");
                 break;
               }
               case ArenaDtorNeeds::kNone:
                 break;
             }
           }},
      },
      R"cc(
        $classname$::$classname$($pb$::Arena* $nullable$ arena)
#if defined(PROTOBUF_CUSTOM_VTABLE)
            : $superclass$(arena, $classname$_class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
            : $superclass$(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
          $ctor_body$;
          // @@protoc_insertion_point(arena_constructor:$full_name$)
        }
      )cc");

  // Generate the copy constructor.
  if (UsingImplicitWeakFields(descriptor_->file(), options_)) {
    // If we are in lite mode and using implicit weak fields, we generate a
    // one-liner copy constructor that delegates to MergeFrom. This saves some
    // code size and also cuts down on the complexity of implicit weak fields.
    // We might eventually want to do this for all lite protos.
    p->Emit(R"cc(
      $classname$::$classname$(
          //~ Force alignment
          $pb$::Arena* $nullable$ arena, const $classname$& from)
          : $classname$(arena) {
        MergeFrom(from);
      }
    )cc");
  } else if (CanUseTrivialCopy()) {
    p->Emit(R"cc(
      $classname$::$classname$(
          //~ Force alignment
          $pb$::Arena* $nullable$ arena, const $classname$& from)
#if defined(PROTOBUF_CUSTOM_VTABLE)
          : $superclass$(arena, $classname$_class_data_.base()),
#else   // PROTOBUF_CUSTOM_VTABLE
          : $superclass$(arena),
#endif  // PROTOBUF_CUSTOM_VTABLE
            _impl_(from._impl_) {
        _internal_metadata_.MergeFrom<$unknown_fields_type$>(
            from._internal_metadata_);
      }
    )cc");
  } else {
    GenerateArenaEnabledCopyConstructor(p);
  }

  // Generate the shared constructor code.
  GenerateSharedConstructorCode(p);

  // Generate the destructor.
  if (HasSimpleBaseClass(descriptor_, options_)) {
    // For messages using simple base classes, having no destructor
    // allows our vtable to share the same destructor as every other
    // message with a simple base class.  This works only as long as
    // we have no fields needing destruction, of course.  (No strings
    // or extensions)
  } else {
    p->Emit(
        R"cc(
          $classname$::~$classname$() {
            // @@protoc_insertion_point(destructor:$full_name$)
            SharedDtor(*this);
          }
        )cc");
  }

  // Generate the shared destructor code.
  GenerateSharedDestructorCode(p);

  // Generate the arena-specific destructor code.
  if (NeedsArenaDestructor() > ArenaDtorNeeds::kNone) {
    GenerateArenaDestructorCode(p);
  }
}

void MessageGenerator::GenerateSourceInProto2Namespace(io::Printer* p) {
  auto v = p->WithVars(ClassVars(descriptor_, options_));
  auto t = p->WithVars(MakeTrackerCalls(descriptor_, options_));
  Formatter format(p);
  if (ShouldGenerateExternSpecializations(options_) &&
      ShouldGenerateClass(descriptor_, options_)) {
    p->Emit(R"(
      template void* $nonnull$ Arena::DefaultConstruct<$classtype$>(Arena* $nullable$);
    )");
    if (!IsMapEntryMessage(descriptor_)) {
      p->Emit(R"(
        template void* $nonnull$ Arena::CopyConstruct<$classtype$>(Arena* $nullable$, const void* $nonnull$);
      )");
    }
  }
}

void MessageGenerator::GenerateClear(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  Formatter format(p);

  // The maximum number of bytes we will memset to zero without checking their
  // hasbit to see if a zero-init is necessary.
  const int kMaxUnconditionalPrimitiveBytesClear = 4;

  format(
      "PROTOBUF_NOINLINE void $classname$::Clear() {\n"
      "// @@protoc_insertion_point(message_clear_start:$full_name$)\n");
  format.Indent();

  format("$pbi$::TSanWrite(&_impl_);\n");

  format(
      // TODO: It would be better to avoid emitting this if it is not used,
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

  std::vector<FieldChunk> chunks = CollectFields(
      optimized_order_, options_,
      [&](const FieldDescriptor* a, const FieldDescriptor* b) -> bool {
        chunk_count++;
        // This predicate guarantees that there is only a single zero-init
        // (memset) per chunk, and if present it will be at the beginning.
        bool same =
            HasByteIndex(a) == HasByteIndex(b) &&
            IsLikelyPresent(a, options_) == IsLikelyPresent(b, options_) &&
            ShouldSplit(a, options_) == ShouldSplit(b, options_) &&
            (CanClearByZeroing(a) == CanClearByZeroing(b) ||
             (CanClearByZeroing(a) && (chunk_count == 1 || merge_zero_init)));
        if (!same) chunk_count = 0;
        return same;
      });

  auto it = chunks.begin();
  auto end = chunks.end();
  int cached_has_word_index = -1;
  while (it != end) {
    auto next = FindNextUnequalChunk(it, end, MayGroupChunksForHaswordsCheck);
    bool has_haswords_check = MaybeEmitHaswordsCheck(
        it, next, options_, has_bit_indices_, cached_has_word_index, "", p);
    bool has_default_split_check = !it->fields.empty() && it->should_split;
    if (has_default_split_check) {
      // Some fields are cleared without checking has_bit. So we add the
      // condition here to avoid writing to the default split instance.
      format("if (!IsSplitMessageDefault()) {\n");
      format.Indent();
    }
    while (it != next) {
      const std::vector<const FieldDescriptor*>& fields = it->fields;
      bool chunk_is_split = it->should_split;
      ABSL_CHECK_EQ(has_default_split_check, chunk_is_split);

      const FieldDescriptor* memset_start = nullptr;
      const FieldDescriptor* memset_end = nullptr;
      bool saw_non_zero_init = false;

      for (const auto& field : fields) {
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
      const bool check_has_byte =
          HasBitIndex(fields.front()) != kNoHasbit && fields.size() > 1 &&
          !IsLikelyPresent(fields.back(), options_) &&
          (memset_end != fields.back() || merge_zero_init);

      DebugAssertUniformLikelyPresence(fields, options_);

      if (check_has_byte) {
        // Emit an if() that will let us skip the whole chunk if none are set.
        uint32_t chunk_mask = GenChunkMask(fields, has_bit_indices_);

        // Check (up to) 8 has_bits at a time if we have more than one field in
        // this chunk.  Due to field layout ordering, we may check
        // _has_bits_[last_chunk * 8 / 32] multiple times.
        ABSL_DCHECK_LE(2, popcnt(chunk_mask));
        ABSL_DCHECK_GE(8, popcnt(chunk_mask));

        if (cached_has_word_index != HasWordIndex(fields.front())) {
          cached_has_word_index = HasWordIndex(fields.front());
          format("cached_has_bits = $has_bits$[$1$];\n", cached_has_word_index);
        }

        format("if ($1$) {\n", GenerateConditionMaybeWithProbabilityForGroup(
                                   chunk_mask, fields, options_));
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
      for (const auto& field : fields) {
        if (CanClearByZeroing(field)) continue;
        // It's faster to just overwrite primitive types, but we should only
        // clear strings and messages if they were set.
        //
        // TODO:  Let the CppFieldGenerator decide this somehow.
        const bool have_enclosing_if = ShouldGenerateEnclosingIf(*field);

        if (have_enclosing_if) {
          PrintPresenceCheck(field, has_bit_indices_, p, &cached_has_word_index,
                             options_);
          format.Indent();
        }

        field_generators_.get(field).GenerateMessageClearingCode(p);

        if (have_enclosing_if) {
          format.Outdent();
          format("}\n");
        }
      }

      if (check_has_byte) {
        format.Outdent();
        format("}\n");
      }

      // To next chunk.
      ++it;
    }

    if (has_default_split_check) {
      format.Outdent();
      format("}\n");
    }
    if (has_haswords_check) {
      p->Outdent();
      p->Emit(R"cc(
        }
      )cc");

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
    format("$pbi$::TSanWrite(&_impl_);\n");
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
    ++i;
  }
}

void MessageGenerator::GenerateSwap(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  Formatter format(p);

  format(
      "void $classname$::InternalSwap($classname$* PROTOBUF_RESTRICT "
      "$nonnull$ other) {\n");
  format.Indent();
  format("using ::std::swap;\n");
  format("$WeakDescriptorSelfPin$");

  if (HasGeneratedMethods(descriptor_->file(), options_)) {
    if (descriptor_->extension_range_count() > 0) {
      format(
          "$extensions$.InternalSwap(&other->$extensions$);"
          "\n");
    }

    if (HasNonSplitOptionalString(descriptor_, options_)) {
      p->Emit(R"cc(
        auto* arena = GetArena();
        ABSL_DCHECK_EQ(arena, other->GetArena());
      )cc");
    }
    format("_internal_metadata_.InternalSwap(&other->_internal_metadata_);\n");

    if (!has_bit_indices_.empty()) {
      for (size_t i = 0; i < HasBitsSize(); ++i) {
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
            FieldMemberName(field, /*split=*/false);
        const std::string last_field_name = FieldMemberName(
            optimized_order_[i + run_length - 1], /*split=*/false);

        auto v = p->WithVars({
            {"first", first_field_name},
            {"last", last_field_name},
        });

        format(
            "$pbi$::memswap<\n"
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

    for (int i = 0; i < descriptor_->real_oneof_decl_count(); ++i) {
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

MessageGenerator::NewOpRequirements MessageGenerator::GetNewOp(
    io::Printer* arena_emitter) const {
  size_t arena_seeding_count = 0;
  NewOpRequirements op;
  if (IsBootstrapProto(options_, descriptor_->file())) {
    // To simplify bootstrapping we always use a function for these types.
    // It makes it easier to change the ABI of the `MessageCreator` class.
    op.needs_to_run_constructor = true;
    return op;
  }

  if (NeedsArenaDestructor() == ArenaDtorNeeds::kRequired) {
    // We can't skip the ArenaDtor for these messages.
    op.needs_to_run_constructor = true;
  }

  if (descriptor_->extension_range_count() > 0) {
    op.needs_arena_seeding = true;
    ++arena_seeding_count;
    if (arena_emitter) {
      arena_emitter->Emit(R"cc(
        PROTOBUF_FIELD_OFFSET($classname$, $extensions$) +
            decltype($classname$::$extensions$)::InternalGetArenaOffset(
                $superclass$::internal_visibility()),
      )cc");
    }
  }

  if (num_weak_fields_ != 0) {
    op.needs_to_run_constructor = true;
  }

  for (const FieldDescriptor* field : FieldRange(descriptor_)) {
    const auto print_arena_offset = [&](absl::string_view suffix = "") {
      ++arena_seeding_count;
      if (arena_emitter) {
        arena_emitter->Emit(
            {{"field", FieldMemberName(field, false)}, {"suffix", suffix}},
            R"cc(
              PROTOBUF_FIELD_OFFSET($classname$, $field$) +
                  decltype($classname$::$field$)::
                      InternalGetArenaOffset$suffix$(
                          $superclass$::internal_visibility()),
            )cc");
      }
    };
    if (ShouldSplit(field, options_)) {
      op.needs_memcpy = true;
    } else if (field->real_containing_oneof() != nullptr) {
      /* nothing to do */
    } else if (field->is_map()) {
      op.needs_arena_seeding = true;
      // MapField contains an internal vtable pointer we need to copy.
      op.needs_memcpy = true;
      print_arena_offset();
      // Non-lite maps currently have more than one arena pointer in them. Print
      // both.
      if (HasDescriptorMethods(descriptor_->file(), options_)) {
        print_arena_offset("Alt");
      }
    } else if (field->is_repeated()) {
      op.needs_arena_seeding = true;
      print_arena_offset();
    } else {
      const auto& generator = field_generators_.get(field);
      if (generator.has_trivial_zero_default()) {
        /* nothing to do */
      } else {
        switch (field->cpp_type()) {
          case FieldDescriptor::CPPTYPE_INT32:
          case FieldDescriptor::CPPTYPE_INT64:
          case FieldDescriptor::CPPTYPE_UINT32:
          case FieldDescriptor::CPPTYPE_UINT64:
          case FieldDescriptor::CPPTYPE_DOUBLE:
          case FieldDescriptor::CPPTYPE_FLOAT:
          case FieldDescriptor::CPPTYPE_BOOL:
          case FieldDescriptor::CPPTYPE_ENUM:
            op.needs_memcpy = true;
            break;

          case FieldDescriptor::CPPTYPE_STRING:
            switch (field->cpp_string_type()) {
              case FieldDescriptor::CppStringType::kCord:
                // Cord fields are currently rejected above because of ArenaDtor
                // requirements.
                ABSL_CHECK(op.needs_to_run_constructor);
                break;
              case FieldDescriptor::CppStringType::kView:
              case FieldDescriptor::CppStringType::kString:
                op.needs_memcpy = true;
                break;
            }
            break;
          case FieldDescriptor::CPPTYPE_MESSAGE:
            ABSL_LOG(FATAL) << "Message should be zero initializable.";
            break;
        }
      }
    }
  }

  // If we are going to generate too many arena seeding offsets, we can skip the
  // attempt because we know it will fail at compile time and fallback to
  // placement new. The arena seeding code can handle up to an offset of
  // `63 * sizeof(Arena*)`.
  // This prevents generating huge lists that have to be run during constant
  // evaluation to just fail anyway. The actual upper bound is smaller than
  // this, but any reasonable value is enough to prevent long compile times for
  // big messages.
  if (arena_seeding_count >= 64) {
    op.needs_to_run_constructor = true;
  }

  return op;
}

void MessageGenerator::GenerateClassData(io::Printer* p) {
  const auto new_op = GetNewOp(nullptr);
  // Always generate PlacementNew_ because we might need it for different
  // reasons. EnableCustomNewFor<T> might be false in this compiler, or the
  // object might be too large for arena seeding.
  // We mark `inline` to avoid library bloat if the function is unused.
  p->Emit(R"cc(
    inline void* $nonnull$ $classname$::PlacementNew_(
        //~
        const void* $nonnull$, void* $nonnull$ mem,
        $pb$::Arena* $nullable$ arena) {
      return ::new (mem) $classname$(arena);
    }
  )cc");
  if (new_op.needs_to_run_constructor) {
    p->Emit(R"cc(
      constexpr auto $classname$::InternalNewImpl_() {
        return $pbi$::MessageCreator(&$classname$::PlacementNew_,
                                     sizeof($classname$), alignof($classname$));
      }
    )cc");
  } else if (new_op.needs_arena_seeding) {
    p->Emit({{"copy_type", new_op.needs_memcpy ? "CopyInit" : "ZeroInit"},
             {"arena_offsets", [&] { GetNewOp(p); }}},
            R"cc(
              constexpr auto $classname$::InternalNewImpl_() {
                constexpr auto arena_bits = $pbi$::EncodePlacementArenaOffsets({
                    $arena_offsets$,
                });
                if (arena_bits.has_value()) {
                  return $pbi$::MessageCreator::$copy_type$(
                      sizeof($classname$), alignof($classname$), *arena_bits);
                } else {
                  return $pbi$::MessageCreator(&$classname$::PlacementNew_,
                                               sizeof($classname$),
                                               alignof($classname$));
                }
              }
            )cc");
  } else {
    p->Emit({{"copy_type", new_op.needs_memcpy ? "CopyInit" : "ZeroInit"},
             {"arena_offsets", [&] { GetNewOp(p); }}},
            R"cc(
              constexpr auto $classname$::InternalNewImpl_() {
                return $pbi$::MessageCreator::$copy_type$(sizeof($classname$),
                                                          alignof($classname$));
              }
            )cc");
  }

  auto vars = p->WithVars(
      {{"default_instance",
        absl::StrCat("&", DefaultInstanceName(descriptor_, options_),
                     "._instance")}});
  const auto on_demand_register_arena_dtor = [&] {
    if (NeedsArenaDestructor() == ArenaDtorNeeds::kOnDemand) {
      p->Emit(R"cc(
        $classname$::OnDemandRegisterArenaDtor,
      )cc");
    } else {
      p->Emit(R"cc(
        nullptr,  // OnDemandRegisterArenaDtor
      )cc");
    }
  };
  const auto is_initialized = [&] {
    if (NeedsIsInitialized()) {
      p->Emit(R"cc(
        $classname$::IsInitializedImpl,
      )cc");
    } else {
      p->Emit(R"cc(
        nullptr,  // IsInitialized
      )cc");
    }
  };
  const auto custom_vtable_methods = [&] {
    if (HasGeneratedMethods(descriptor_->file(), options_) &&
        !IsMapEntryMessage(descriptor_)) {
      p->Emit(R"cc(
        $superclass$::GetClearImpl<$classname$>(), &$classname$::ByteSizeLong,
            &$classname$::_InternalSerialize,
      )cc");
    } else {
      p->Emit(R"cc(
        static_cast<void ($pb$::MessageLite::*)()>(&$classname$::ClearImpl),
            $superclass$::ByteSizeLongImpl, $superclass$::_InternalSerializeImpl
            ,
      )cc");
    }
  };

  const auto emit_v2_data = [&] {
  };

  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    const auto pin_weak_descriptor = [&] {
      if (!UsingImplicitWeakDescriptor(descriptor_->file(), options_)) return;

      p->Emit({{"pin", StrongReferenceToType(descriptor_, options_)}},
              R"cc(
                $pin$;
              )cc");

      // For CODE_SIZE types, we need to pin the submessages too.
      // SPEED types will pin them via the TcParse table automatically.
      if (HasGeneratedMethods(descriptor_->file(), options_)) return;
      for (int i = 0; i < descriptor_->field_count(); ++i) {
        auto* field = descriptor_->field(i);
        if (field->type() != field->TYPE_MESSAGE) continue;
        p->Emit(
            {{"pin", StrongReferenceToType(field->message_type(), options_)}},
            R"cc(
              $pin$;
            )cc");
      }
    };
    p->Emit(
        {
            {"on_demand_register_arena_dtor", on_demand_register_arena_dtor},
            {"is_initialized", is_initialized},
            {"pin_weak_descriptor", pin_weak_descriptor},
            {"custom_vtable_methods", custom_vtable_methods},
            {"v2_data", emit_v2_data},
            {"tracker_on_get_metadata",
             [&] {
               if (HasTracker(descriptor_, options_)) {
                 p->Emit(R"cc(
                   &Impl_::TrackerOnGetMetadata,
                 )cc");
               } else {
                 p->Emit(R"cc(
                   nullptr,  // tracker
                 )cc");
               }
             }},
        },
        R"cc(
          constexpr auto $classname$::InternalGenerateClassData_() {
            return $pbi$::ClassDataFull{
                $pbi$::ClassData{
                    $default_instance$,
                    &_table_.header,
                    $on_demand_register_arena_dtor$,
                    $is_initialized$,
                    &$classname$::MergeImpl,
                    $superclass$::GetNewImpl<$classname$>(),
#if defined(PROTOBUF_CUSTOM_VTABLE)
                    &$classname$::SharedDtor,
                    $custom_vtable_methods$,
#endif  // PROTOBUF_CUSTOM_VTABLE
                    PROTOBUF_FIELD_OFFSET($classname$, $cached_size$),
                    false,
                    $v2_data$,
                },
                &$classname$::kDescriptorMethods,
                &$desc_table$,
                $tracker_on_get_metadata$,
            };
          }

          PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 const
              $pbi$::ClassDataFull $classname$_class_data_ =
                  $classname$::InternalGenerateClassData_();

          //~ This function needs to be marked as weak to avoid significantly
          //~ slowing down compilation times.  This breaks up LLVM's SCC
          //~ in the .pb.cc translation units. Large translation units see a
          //~ reduction of roughly 50% of walltime for optimized builds.
          //~ Without the weak attribute all the messages in the file, including
          //~ all the vtables and everything they use become part of the same
          //~ SCC.
          PROTOBUF_ATTRIBUTE_WEAK const $pbi$::ClassData* $nonnull$
          $classname$::GetClassData() const {
            $pin_weak_descriptor$;
            $pbi$::PrefetchToLocalCache(&$classname$_class_data_);
            $pbi$::PrefetchToLocalCache($classname$_class_data_.tc_table);
            return $classname$_class_data_.base();
          }
        )cc");
  } else {
    p->Emit(
        {
            {"type_size", descriptor_->full_name().size() + 1},
            {"on_demand_register_arena_dtor", on_demand_register_arena_dtor},
            {"is_initialized", is_initialized},
            {"custom_vtable_methods", custom_vtable_methods},
            {"v2_data", emit_v2_data},
        },
        R"cc(
          constexpr auto $classname$::InternalGenerateClassData_() {
            return $pbi$::ClassDataLite<$type_size$>{
                {
                    $default_instance$,
                    &_table_.header,
                    $on_demand_register_arena_dtor$,
                    $is_initialized$,
                    &$classname$::MergeImpl,
                    $superclass$::GetNewImpl<$classname$>(),
#if defined(PROTOBUF_CUSTOM_VTABLE)
                    &$classname$::SharedDtor,
                    $custom_vtable_methods$,
#endif  // PROTOBUF_CUSTOM_VTABLE
                    PROTOBUF_FIELD_OFFSET($classname$, $cached_size$),
                    true,
                    $v2_data$,
                },
                "$full_name$",
            };
          }

          PROTOBUF_CONSTINIT
          PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
          const $pbi$::ClassDataLite<$type_size$> $classname$_class_data_ =
              $classname$::InternalGenerateClassData_();

          //~ This function needs to be marked as weak to avoid significantly
          //~ slowing down compilation times.  This breaks up LLVM's SCC
          //~ in the .pb.cc translation units. Large translation units see a
          //~ reduction of roughly 50% of walltime for optimized builds.
          //~ Without the weak attribute all the messages in the file, including
          //~ all the vtables and everything they use become part of the same
          //~ SCC.
          PROTOBUF_ATTRIBUTE_WEAK const $pbi$::ClassData* $nonnull$
          $classname$::GetClassData() const {
            return $classname$_class_data_.base();
          }
        )cc");
  }
}

bool MessageGenerator::RequiresArena(GeneratorFunction function) const {
  for (const FieldDescriptor* field : FieldRange(descriptor_)) {
    if (field_generators_.get(field).RequiresArena(function)) {
      return true;
    }
  }
  return false;
}

void MessageGenerator::GenerateClassSpecificMergeImpl(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  // Generate the class-specific MergeFrom, which avoids the ABSL_CHECK and
  // cast.
  Formatter format(p);
  format(
      "void $classname$::MergeImpl($pb$::MessageLite& to_msg, const "
      "$pb$::MessageLite& from_msg) {\n"
      "$WeakDescriptorSelfPin$"
      "  auto* const _this = static_cast<$classname$*>(&to_msg);\n"
      "  auto& from = static_cast<const $classname$&>(from_msg);\n");
  format.Indent();
  GenerateVerifyHasBitConsistency(p, "_this->");
  if (RequiresArena(GeneratorFunction::kMergeFrom)) {
    p->Emit(R"cc(
      $pb$::Arena* arena = _this->GetArena();
    )cc");
  }
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
        "if (ABSL_PREDICT_FALSE(!from.IsSplitMessageDefault())) {\n"
        "  _this->PrepareSplitMessageForWrite();\n"
        "}\n");
  }

  std::vector<FieldChunk> chunks = CollectFields(
      optimized_order_, options_,
      [&](const FieldDescriptor* a, const FieldDescriptor* b) -> bool {
        return HasByteIndex(a) == HasByteIndex(b) &&
               IsLikelyPresent(a, options_) == IsLikelyPresent(b, options_) &&
               ShouldSplit(a, options_) == ShouldSplit(b, options_);
      });

  auto it = chunks.begin();
  auto end = chunks.end();
  // cached_has_word_index maintains that:
  //   cached_has_bits = from._has_bits_[cached_has_word_index]
  // for cached_has_word_index >= 0
  int cached_has_word_index = -1;
  while (it != end) {
    auto next = FindNextUnequalChunk(it, end, MayGroupChunksForHaswordsCheck);
    bool has_haswords_check =
        MaybeEmitHaswordsCheck(it, next, options_, has_bit_indices_,
                               cached_has_word_index, "from.", p);

    while (it != next) {
      const std::vector<const FieldDescriptor*>& fields = it->fields;
      const bool cache_has_bits = HasByteIndex(fields.front()) != kNoHasbit;
      const bool check_has_byte = cache_has_bits && fields.size() > 1 &&
                                  !IsLikelyPresent(fields.back(), options_);

      DebugAssertUniformLikelyPresence(fields, options_);

      if (cache_has_bits &&
          cached_has_word_index != HasWordIndex(fields.front())) {
        cached_has_word_index = HasWordIndex(fields.front());
        format("cached_has_bits = from.$has_bits$[$1$];\n",
               cached_has_word_index);
      }

      if (check_has_byte) {
        // Emit an if() that will let us skip the whole chunk if none are set.
        uint32_t chunk_mask = GenChunkMask(fields, has_bit_indices_);

        // Check (up to) 8 has_bits at a time if we have more than one field in
        // this chunk.  Due to field layout ordering, we may check
        // _has_bits_[last_chunk * 8 / 32] multiple times.
        ABSL_DCHECK_LE(2, popcnt(chunk_mask));
        ABSL_DCHECK_GE(8, popcnt(chunk_mask));

        format("if ($1$) {\n", GenerateConditionMaybeWithProbabilityForGroup(
                                   chunk_mask, fields, options_));
        format.Indent();
      }

      // Go back and emit merging code for each of the fields we processed.
      for (const auto* field : fields) {
        const auto& generator = field_generators_.get(field);

        if (!field->is_required() && !HasHasbit(field)) {
          // Merge semantics without true field presence: primitive fields are
          // merged only if non-zero (numeric) or non-empty (string).
          MayEmitMutableIfNonDefaultCheck(
              p, "from.", field, ShouldSplit(field, options_), options_,
              /*emit_body=*/[&]() { generator.GenerateMergingCode(p); },
              /*with_enclosing_braces_always=*/true);
        } else if (field->options().weak() ||
                   cached_has_word_index != HasWordIndex(field)) {
          // Check hasbit, not using cached bits.
          auto v = p->WithVars(HasBitVars(field));
          format(
              "if ((from.$has_bits$[$has_array_index$] & $has_mask$) != 0) "
              "{\n");
          format.Indent();
          generator.GenerateMergingCode(p);
          format.Outdent();
          format("}\n");
        } else {
          // Check hasbit, using cached bits.
          ABSL_CHECK(HasHasbit(field));
          int has_bit_index = has_bit_indices_[field->index()];
          format("if ($1$) {\n", GenerateConditionMaybeWithProbabilityForField(
                                     has_bit_index, field, options_));
          format.Indent();

          if (GetFieldHasbitMode(field) == HasbitMode::kHintHasbit) {
            // Merge semantics without true field presence: primitive fields are
            // merged only if non-zero (numeric) or non-empty (string).
            MayEmitMutableIfNonDefaultCheck(
                p, "from.", field, ShouldSplit(field, options_), options_,
                /*emit_body=*/[&]() { generator.GenerateMergingCode(p); },
                /*with_enclosing_braces_always=*/false);
          } else {
            ABSL_DCHECK(GetFieldHasbitMode(field) == HasbitMode::kTrueHasbit);
            if (check_has_byte && IsPOD(field)) {
              generator.GenerateCopyConstructorCode(p);
            } else {
              generator.GenerateMergingCode(p);
            }
          }

          format.Outdent();
          format("}\n");
        }
      }

      if (check_has_byte) {
        format.Outdent();
        format("}\n");
      }

      // To next chunk.
      ++it;
    }

    if (has_haswords_check) {
      p->Outdent();
      p->Emit(R"cc(
        }
      )cc");

      // Reset here as it may have been updated in just closed if statement.
      cached_has_word_index = -1;
    }
  }

  if (HasBitsSize() == 1) {
    // Optimization to avoid a load. Assuming that most messages have fewer than
    // 32 fields, this seems useful.
    p->Emit(R"cc(
      _this->$has_bits$[0] |= cached_has_bits;
    )cc");
  } else if (HasBitsSize() > 1) {
    p->Emit(R"cc(
      _this->$has_bits$.Or(from.$has_bits$);
    )cc");
  }

  // Merge oneof fields. Oneof field requires oneof case check.
  for (auto oneof : OneOfRange(descriptor_)) {
    p->Emit({{"name", oneof->name()},
             {"NAME", absl::AsciiStrToUpper(oneof->name())},
             {"index", oneof->index()},
             {"cases",
              [&] {
                for (const auto* field : FieldRange(oneof)) {
                  p->Emit(
                      {{"Label", UnderscoresToCamelCase(field->name(), true)},
                       {"body",
                        [&] {
                          field_generators_.get(field).GenerateMergingCode(p);
                        }}},
                      R"cc(
                        case k$Label$: {
                          $body$;
                          break;
                        }
                      )cc");
                }
              }}},
            R"cc(
              if (const uint32_t oneof_from_case = from.$oneof_case$[$index$]) {
                const uint32_t oneof_to_case = _this->$oneof_case$[$index$];
                const bool oneof_needs_init = oneof_to_case != oneof_from_case;
                if (oneof_needs_init) {
                  if (oneof_to_case != 0) {
                    _this->clear_$name$();
                  }
                  _this->$oneof_case$[$index$] = oneof_from_case;
                }

                switch (oneof_from_case) {
                  $cases$;
                  case $NAME$_NOT_SET:
                    break;
                }
              }
            )cc");
  }
  if (num_weak_fields_) {
    format(
        "_this->$weak_field_map$.MergeFrom(from.$weak_field_map$);"
        "\n");
  }

  // Merging of extensions and unknown fields is done last, to maximize
  // the opportunity for tail calls.
  if (descriptor_->extension_range_count() > 0) {
    p->Emit(R"cc(
      _this->$extensions$.MergeFrom(&default_instance(), from.$extensions$);
    )cc");
  }

  format(
      "_this->_internal_metadata_.MergeFrom<$unknown_fields_type$>("
      "from._internal_metadata_);\n");

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
  ABSL_CHECK(!fields.empty());
  if (fields.size() == 1) {
    GenerateSerializeOneField(p, fields[0], -1);
    return;
  }
  // We have multiple mutually exclusive choices.  Emit a switch statement.
  const OneofDescriptor* oneof = fields[0]->containing_oneof();
  p->Emit({{"name", oneof->name()},
           {"cases",
            [&] {
              for (const auto* field : fields) {
                p->Emit({{"Name", UnderscoresToCamelCase(field->name(), true)},
                         {"body",
                          [&] {
                            field_generators_.get(field)
                                .GenerateSerializeWithCachedSizesToArray(p);
                          }}},
                        R"cc(
                          case k$Name$: {
                            $body$;
                            break;
                          }
                        )cc");
              }
            }}},
          R"cc(
            switch (this_.$name$_case()) {
              $cases$;
              default:
                break;
            }
          )cc");
}

void MessageGenerator::GenerateSerializeOneField(io::Printer* p,
                                                 const FieldDescriptor* field,
                                                 int cached_has_bits_index) {
  auto v = p->WithVars(FieldVars(field, options_));
  auto emit_body = [&] {
    field_generators_.get(field).GenerateSerializeWithCachedSizesToArray(p);
  };

  if (field->options().weak()) {
    emit_body();
    p->Emit("\n");
    return;
  }

  PrintFieldComment(Formatter{p}, field, options_);
  if (HasHasbit(field)) {
    int has_bit_index = HasBitIndex(field);
    int has_word_index = has_bit_index / 32;
    bool use_cached_has_bits = cached_has_bits_index == has_word_index;
    p->Emit(
        {
            {"body",
             [&]() {
               MayEmitIfNonDefaultCheck(p, "this_.", field,
                                        std::move(emit_body),
                                        /*with_enclosing_braces_always=*/false);
             }},
            {"cond", GenerateConditionMaybeWithProbability(
                         1u << (has_bit_index % 32),
                         GetPresenceProbability(field, options_),
                         use_cached_has_bits, has_word_index)},
        },
        R"cc(
          if ($cond$) {
            $body$;
          }
        )cc");
  } else if (!field->is_required() && !field->is_repeated()) {
    MayEmitIfNonDefaultCheck(p, "this_.", field, std::move(emit_body),
                             /*with_enclosing_braces_always=*/true);
  } else {
    emit_body();
  }
  p->Emit("\n");
}

void MessageGenerator::GenerateSerializeOneExtensionRange(io::Printer* p,
                                                          int start, int end) {
  auto v = p->WithVars(variables_);
  p->Emit(  //
      {{"start", start}, {"end", end}},
      R"cc(
        // Extension range [$start$, $end$)
        target = this_.$extensions$._InternalSerialize(
            &default_instance(), $start$, $end$, target, stream);
      )cc");
}

void MessageGenerator::GenerateSerializeAllExtensions(io::Printer* p) {
  auto v = p->WithVars(variables_);
  p->Emit(
      R"cc(
        // All extensions.
        target = this_.$extensions$._InternalSerializeAll(&default_instance(),
                                                          target, stream);
      )cc");
}

void MessageGenerator::GenerateSerializeWithCachedSizesToArray(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  if (descriptor_->options().message_set_wire_format()) {
    // Special-case MessageSet.
    p->Emit(R"cc(
#if defined(PROTOBUF_CUSTOM_VTABLE)
      $uint8$* $nonnull$ $classname$::_InternalSerialize(
          const $pb$::MessageLite& base, $uint8$* $nonnull$ target,
          $pb$::io::EpsCopyOutputStream* $nonnull$ stream) {
        const $classname$& this_ = static_cast<const $classname$&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
      $uint8$* $nonnull$ $classname$::_InternalSerialize(
          $uint8$* $nonnull$ target,
          $pb$::io::EpsCopyOutputStream* $nonnull$ stream) const {
        const $classname$& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
        $annotate_serialize$ target =
            this_.$extensions$
                .InternalSerializeMessageSetWithCachedSizesToArray(
                    &default_instance(), target, stream);
        target = ::_pbi::InternalSerializeUnknownMessageSetItemsToArray(
            this_.$unknown_fields$, target, stream);
        return target;
      }
    )cc");
    return;
  }

  p->Emit(
      {
          {"has_bit_consistency",
           [&] { GenerateVerifyHasBitConsistency(p, "this_."); }},
          {"ndebug", [&] { GenerateSerializeWithCachedSizesBody(p); }},
          {"debug", [&] { GenerateSerializeWithCachedSizesBodyShuffled(p); }},
          {"ifdef",
           [&] {
             if (ShouldSerializeInOrder(descriptor_, options_)) {
               p->Emit("$ndebug$");
             } else {
               p->Emit(R"cc(
                 $has_bit_consistency$;
#ifdef NDEBUG
                 $ndebug$;
#else   // NDEBUG
                 $debug$;
#endif  // !NDEBUG
               )cc");
             }
           }},
      },
      R"cc(
#if defined(PROTOBUF_CUSTOM_VTABLE)
        $uint8$* $nonnull$ $classname$::_InternalSerialize(
            const $pb$::MessageLite& base, $uint8$* $nonnull$ target,
            $pb$::io::EpsCopyOutputStream* $nonnull$ stream) {
          const $classname$& this_ = static_cast<const $classname$&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        $uint8$* $nonnull$ $classname$::_InternalSerialize(
            $uint8$* $nonnull$ target,
            $pb$::io::EpsCopyOutputStream* $nonnull$ stream) const {
          const $classname$& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          $annotate_serialize$;
          // @@protoc_insertion_point(serialize_to_array_start:$full_name$)
          $ifdef$;
          // @@protoc_insertion_point(serialize_to_array_end:$full_name$)
          return target;
        }
      )cc");
}

void MessageGenerator::GenerateSerializeWithCachedSizesBody(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;
  // If there are multiple fields in a row from the same oneof then we
  // coalesce them and emit a switch statement.  This is more efficient
  // because it lets the C++ compiler know this is a "at most one can happen"
  // situation. If we emitted "if (has_x()) ...; if (has_y()) ..." the C++
  // compiler's emitted code might check has_y() even when has_x() is true.
  class LazySerializerEmitter {
   public:
    LazySerializerEmitter(MessageGenerator* mg, io::Printer* p)
        : mg_(mg), p_(p), cached_has_bit_index_(kNoHasbit) {}

    ~LazySerializerEmitter() { Flush(); }

    // If conditions allow, try to accumulate a run of fields from the same
    // oneof, and handle them at the next Flush().
    void Emit(const FieldDescriptor* field) {
      if (!field->has_presence() || MustFlush(field)) {
        Flush();
      }
      if (field->real_containing_oneof()) {
        v_.push_back(field);
      } else {
        // TODO: Defer non-oneof fields similarly to oneof fields.
        if (HasHasbit(field) && !field->real_containing_oneof()) {
          // We speculatively load the entire _has_bits_[index] contents, even
          // if it is for only one field.  Deferring non-oneof emitting would
          // allow us to determine whether this is going to be useful.
          int has_bit_index = mg_->has_bit_indices_[field->index()];
          if (cached_has_bit_index_ != has_bit_index / 32) {
            // Reload.
            int new_index = has_bit_index / 32;
            p_->Emit({{"index", new_index}},
                     R"cc(
                       cached_has_bits = this_._impl_._has_bits_[$index$];
                     )cc");
            cached_has_bit_index_ = new_index;
          }
        }

        mg_->GenerateSerializeOneField(p_, field, cached_has_bit_index_);
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
        min_start_ = range->start_number();
        max_end_ = range->end_number();
        has_current_range_ = true;
      } else {
        min_start_ = std::min(min_start_, range->start_number());
        max_end_ = std::max(max_end_, range->end_number());
      }
    }

    void Flush(bool is_last_range) {
      if (!has_current_range_) {
        return;
      }
      has_current_range_ = false;
      ++range_count_;
      if (is_last_range && range_count_ == 1) {
        mg_->GenerateSerializeAllExtensions(p_);
      } else {
        mg_->GenerateSerializeOneExtensionRange(p_, min_start_, max_end_);
      }
    }

   private:
    MessageGenerator* mg_;
    io::Printer* p_;
    int range_count_ = 0;
    bool has_current_range_ = false;
    int min_start_ = 0;
    int max_end_ = 0;
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
  p->Emit(
      {
          {"handle_weak_fields",
           [&] {
             if (num_weak_fields_ == 0) return;
             p->Emit(R"cc(
               ::_pbi::WeakFieldMap::FieldWriter field_writer(
                   this_.$weak_field_map$);
             )cc");
           }},
          {"handle_lazy_fields",
           [&] {
             // Merge fields and extension ranges, sorted by field number.
             LazySerializerEmitter e(this, p);
             LazyExtensionRangeEmitter re(this, p);
             LargestWeakFieldHolder largest_weak_field;
             size_t i, j;
             for (i = 0, j = 0;
                  i < ordered_fields.size() || j < sorted_extensions.size();) {
               bool no_more_extensions = j == sorted_extensions.size();
               if (no_more_extensions ||
                   (i < static_cast<size_t>(descriptor_->field_count()) &&
                    ordered_fields[i]->number() <
                        sorted_extensions[j]->start_number())) {
                 const FieldDescriptor* field = ordered_fields[i++];
                 re.Flush(no_more_extensions);
                 if (field->options().weak()) {
                   largest_weak_field.ReplaceIfLarger(field);
                   PrintFieldComment(Formatter{p}, field, options_);
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
             re.Flush(/*is_last_range=*/true);
             e.EmitIfNotNull(largest_weak_field.Release());
           }},
          {"handle_unknown_fields",
           [&] {
             if (UseUnknownFieldSet(descriptor_->file(), options_)) {
               p->Emit(R"cc(
                 target =
                     ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
                         this_.$unknown_fields$, target, stream);
               )cc");
             } else {
               p->Emit(R"cc(
                 target = stream->WriteRaw(
                     this_.$unknown_fields$.data(),
                     static_cast<int>(this_.$unknown_fields$.size()), target);
               )cc");
             }
           }},
      },
      R"cc(
        $handle_weak_fields$;
        $uint32$ cached_has_bits = 0;
        (void)cached_has_bits;

        $handle_lazy_fields$;
        if (ABSL_PREDICT_FALSE(this_.$have_unknown_fields$)) {
          $handle_unknown_fields$;
        }
      )cc");
}

void MessageGenerator::GenerateSerializeWithCachedSizesBodyShuffled(
    io::Printer* p) {
  std::vector<const FieldDescriptor*> ordered_fields =
      SortFieldsByNumber(descriptor_);

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
  p->Emit(
      {
          {"last_field", num_fields - 1},
          {"field_writer",
           [&] {
             if (num_weak_fields_ == 0) return;
             p->Emit(R"cc(
               ::_pbi::WeakFieldMap::FieldWriter field_writer(
                   this_.$weak_field_map$);
             )cc");
           }},
          {"ordered_cases",
           [&] {
             size_t index = 0;
             for (const auto* f : ordered_fields) {
               p->Emit({{"index", index++},
                        {"body", [&] { GenerateSerializeOneField(p, f, -1); }}},
                       R"cc(
                         case $index$: {
                           $body$;
                           break;
                         }
                       )cc");
             }
           }},
          {"extension_cases",
           [&] {
             size_t index = ordered_fields.size();
             for (const auto* r : sorted_extensions) {
               p->Emit({{"index", index++},
                        {"body",
                         [&] {
                           GenerateSerializeOneExtensionRange(
                               p, r->start_number(), r->end_number());
                         }}},
                       R"cc(
                         case $index$: {
                           $body$;
                           break;
                         }
                       )cc");
             }
           }},
          {"handle_unknown_fields",
           [&] {
             if (UseUnknownFieldSet(descriptor_->file(), options_)) {
               p->Emit(R"cc(
                 target =
                     ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
                         this_.$unknown_fields$, target, stream);
               )cc");
             } else {
               p->Emit(R"cc(
                 target = stream->WriteRaw(
                     this_.$unknown_fields$.data(),
                     static_cast<int>(this_.$unknown_fields$.size()), target);
               )cc");
             }
           }},
      },
      R"cc(
        $field_writer$;
        for (int i = $last_field$; i >= 0; i--) {
          switch (i) {
            $ordered_cases$;
            $extension_cases$;
            default: {
              $DCHK$(false) << "Unexpected index: " << i;
            }
          }
        }
        if (ABSL_PREDICT_FALSE(this_.$have_unknown_fields$)) {
          $handle_unknown_fields$;
        }
      )cc");
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

static std::optional<int> FixedSize(const FieldDescriptor* field) {
  if (field->is_repeated() || field->real_containing_oneof() ||
      !field->has_presence()) {
    return std::nullopt;
  }

  const size_t tag_size = WireFormat::TagSize(field->number(), field->type());

  switch (field->type()) {
    case FieldDescriptor::TYPE_FIXED32:
      return tag_size + WireFormatLite::kFixed32Size;
    case FieldDescriptor::TYPE_FIXED64:
      return tag_size + WireFormatLite::kFixed64Size;
    case FieldDescriptor::TYPE_SFIXED32:
      return tag_size + WireFormatLite::kSFixed32Size;
    case FieldDescriptor::TYPE_SFIXED64:
      return tag_size + WireFormatLite::kSFixed64Size;
    case FieldDescriptor::TYPE_FLOAT:
      return tag_size + WireFormatLite::kFloatSize;
    case FieldDescriptor::TYPE_DOUBLE:
      return tag_size + WireFormatLite::kDoubleSize;
    case FieldDescriptor::TYPE_BOOL:
      return tag_size + WireFormatLite::kBoolSize;
    default:
      return std::nullopt;
  }
}

void MessageGenerator::GenerateByteSize(io::Printer* p) {
  if (HasSimpleBaseClass(descriptor_, options_)) return;

  if (descriptor_->options().message_set_wire_format()) {
    // Special-case MessageSet.
    p->Emit(
        R"cc(
#if defined(PROTOBUF_CUSTOM_VTABLE)
          ::size_t $classname$::ByteSizeLong(const MessageLite& base) {
            const $classname$& this_ = static_cast<const $classname$&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
          ::size_t $classname$::ByteSizeLong() const {
            const $classname$& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
            $WeakDescriptorSelfPin$;
            $annotate_bytesize$;
            // @@protoc_insertion_point(message_set_byte_size_start:$full_name$)
            ::size_t total_size = this_.$extensions$.MessageSetByteSize();
            if (this_.$have_unknown_fields$) {
              total_size += ::_pbi::ComputeUnknownMessageSetItemsSize(
                  this_.$unknown_fields$);
            }
            this_.$cached_size$.Set(::_pbi::ToCachedSize(total_size));
            return total_size;
          }
        )cc");
    p->Emit("\n");
    return;
  }

  std::vector<const FieldDescriptor*> fixed;
  std::vector<const FieldDescriptor*> rest;
  for (auto* f : optimized_order_) {
    if (FixedSize(f).has_value()) {
      fixed.push_back(f);
    } else {
      rest.push_back(f);
    }
  }

  // Sort the fixed fields to ensure maximum grouping.
  // The layout of the fields is irrelevant because we are not going to read
  // them. We only look at the hasbits.
  const auto fixed_tuple = [&](auto* f) {
    return std::make_tuple(HasWordIndex(f), FixedSize(f));
  };
  absl::c_sort(
      fixed, [&](auto* a, auto* b) { return fixed_tuple(a) < fixed_tuple(b); });
  std::vector<FieldChunk> fixed_chunks =
      CollectFields(fixed, options_, [&](const auto* a, const auto* b) {
        return fixed_tuple(a) == fixed_tuple(b);
      });

  std::vector<FieldChunk> chunks =
      CollectFields(rest, options_, [&](const auto* a, const auto* b) {
        return a->is_required() == b->is_required() &&
               HasByteIndex(a) == HasByteIndex(b) &&
               IsLikelyPresent(a, options_) == IsLikelyPresent(b, options_) &&
               ShouldSplit(a, options_) == ShouldSplit(b, options_);
      });

  // Interleave the fixed chunks in the right place to be able to reuse
  // cached_has_bits if available. Otherwise, add them to the end.
  for (auto& chunk : fixed_chunks) {
    auto it = std::find_if(chunks.begin(), chunks.end(), [&](auto& c) {
      return HasWordIndex(c.fields[0]) == HasWordIndex(chunk.fields[0]);
    });
    chunks.insert(it, std::move(chunk));
  }

  p->Emit(
      {{"handle_extension_set",
        [&] {
          if (descriptor_->extension_range_count() == 0) return;
          p->Emit(R"cc(
            total_size += this_.$extensions$.ByteSize();
          )cc");
        }},
       {"prefetch",
        [&] {
          // See comment in third_party/protobuf/port.h for details,
          // on how much we are prefetching. Only insert prefetch once per
          // function, since advancing is actually slower. We sometimes
          // prefetch more than sizeof(message), because it helps with
          // next message on arena.
          bool generate_prefetch = false;
          // Skip trivial messages with 0 or 1 fields, unless they are
          // repeated, to reduce codesize.
          switch (optimized_order_.size()) {
            case 1:
              generate_prefetch = optimized_order_[0]->is_repeated();
              break;
            case 0:
              break;
            default:
              generate_prefetch = true;
          }
          if (!generate_prefetch || !IsPresentMessage(descriptor_, options_)) {
            return;
          }
          p->Emit(R"cc(
            ::_pbi::Prefetch5LinesFrom7Lines(&this_);
          )cc");
        }},
       {"handle_fields",
        [&] {
          auto it = chunks.begin();
          auto end = chunks.end();
          int cached_has_word_index = -1;
          const auto update_cached_has_bits = [&](auto& fields) {
            if (cached_has_word_index == HasWordIndex(fields.front())) return;

            cached_has_word_index = HasWordIndex(fields.front());
            p->Emit({{"index", cached_has_word_index}},
                    R"cc(
                      cached_has_bits = this_.$has_bits$[$index$];
                    )cc");
          };

          while (it != end) {
            auto next =
                FindNextUnequalChunk(it, end, MayGroupChunksForHaswordsCheck);
            bool has_haswords_check =
                MaybeEmitHaswordsCheck(it, next, options_, has_bit_indices_,
                                       cached_has_word_index, "this_.", p);

            while (it != next) {
              const auto& fields = it->fields;

              // If the chunk is a fixed size singular chunk, use a branchless
              // approach for it.
              if (std::optional<int> fsize = FixedSize(fields[0])) {
                update_cached_has_bits(fields);
                uint32_t mask = GenChunkMask(fields, has_bit_indices_);
                p->Emit({{"mask", absl::StrFormat("0x%08xU", mask)},
                         {"popcount", absl::has_single_bit(mask)
                                          ? "static_cast<bool>"
                                          : "::absl::popcount"},
                         {"fsize", *fsize}},
                        R"cc(
                          //~
                          total_size += $popcount$($mask$ & cached_has_bits) * $fsize$;
                        )cc");
                ++it;
                continue;
              }

              const bool check_has_byte =
                  fields.size() > 1 && HasWordIndex(fields[0]) != kNoHasbit &&
                  !IsLikelyPresent(fields.back(), options_);
              DebugAssertUniformLikelyPresence(fields, options_);
              p->Emit({{"update_byte_size_for_chunk",
                        [&] {
                          // Go back and emit checks for each of the fields we
                          // processed.
                          for (const auto* field : fields) {
                            EmitUpdateByteSizeForField(field, p,
                                                       cached_has_word_index);
                          }
                        }},
                       {"may_update_cached_has_word_index",
                        [&] {
                          if (!check_has_byte) return;
                          update_cached_has_bits(fields);
                        }},
                       {"check_if_chunk_present",
                        [&] {
                          if (!check_has_byte) {
                            return;
                          }

                          // Emit an if() that will let us skip the whole chunk
                          // if none are set.
                          uint32_t chunk_mask =
                              GenChunkMask(fields, has_bit_indices_);

                          // Check (up to) 8 has_bits at a time if we have more
                          // than one field in this chunk.  Due to field layout
                          // ordering, we may check _has_bits_[last_chunk * 8 /
                          // 32] multiple times.
                          ABSL_DCHECK_LE(2, popcnt(chunk_mask));
                          ABSL_DCHECK_GE(8, popcnt(chunk_mask));

                          p->Emit(
                              {{"condition",
                                GenerateConditionMaybeWithProbabilityForGroup(
                                    chunk_mask, fields, options_)}},
                              "if ($condition$)");
                        }}},
                      R"cc(
                        $may_update_cached_has_word_index$;
                        $check_if_chunk_present$ {
                          //~ Force newline.
                          $update_byte_size_for_chunk$;
                        }
                      )cc");

              // To next chunk.
              ++it;
            }

            if (has_haswords_check) {
              p->Emit(R"cc(
                }
              )cc");

              // Reset here as it may have been updated in just closed if
              // statement.
              cached_has_word_index = -1;
            }
          }
        }},
       {"handle_oneof_fields",
        [&] {
          // Fields inside a oneof don't use _has_bits_ so we count them in a
          // separate pass.
          for (auto oneof : OneOfRange(descriptor_)) {
            p->Emit(
                {{"oneof_name", oneof->name()},
                 {"oneof_case_name", absl::AsciiStrToUpper(oneof->name())},
                 {"case_per_field",
                  [&] {
                    for (auto field : FieldRange(oneof)) {
                      PrintFieldComment(Formatter{p}, field, options_);
                      p->Emit(
                          {{"field_name",
                            UnderscoresToCamelCase(field->name(), true)},
                           {"field_byte_size",
                            [&] {
                              field_generators_.get(field).GenerateByteSize(p);
                            }}},
                          R"cc(
                            case k$field_name$: {
                              $field_byte_size$;
                              break;
                            }
                          )cc");
                    }
                  }}},
                R"cc(
                  switch (this_.$oneof_name$_case()) {
                    $case_per_field$;
                    case $oneof_case_name$_NOT_SET: {
                      break;
                    }
                  }
                )cc");
          }
        }},
       {"handle_weak_fields",
        [&] {
          if (num_weak_fields_ == 0) return;
          // TagSize + MessageSize
          p->Emit(R"cc(
            total_size += this_.$weak_field_map$.ByteSizeLong();
          )cc");
        }},
       {"handle_unknown_fields",
        [&] {
          if (UseUnknownFieldSet(descriptor_->file(), options_)) {
            // We go out of our way to put the computation of the uncommon
            // path of unknown fields in tail position. This allows for
            // better code generation of this function for simple protos.
            p->Emit(R"cc(
              return this_.MaybeComputeUnknownFieldsSize(total_size,
                                                         &this_.$cached_size$);
            )cc");
          } else {
            // We update _cached_size_ even though this is a const method.
            // Because const methods might be called concurrently this needs
            // to be atomic operations or the program is undefined.  In
            // practice, since any concurrent writes will be writing the
            // exact same value, normal writes will work on all common
            // processors. We use a dedicated wrapper class to abstract away
            // the underlying atomic. This makes it easier on platforms where
            // even relaxed memory order might have perf impact to replace it
            // with ordinary loads and stores.
            p->Emit(R"cc(
              if (ABSL_PREDICT_FALSE(this_.$have_unknown_fields$)) {
                total_size += this_.$unknown_fields$.size();
              }
              this_.$cached_size$.Set(::_pbi::ToCachedSize(total_size));
              return total_size;
            )cc");
          }
        }}},
      R"cc(
#if defined(PROTOBUF_CUSTOM_VTABLE)
        ::size_t $classname$::ByteSizeLong(const MessageLite& base) {
          const $classname$& this_ = static_cast<const $classname$&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        ::size_t $classname$::ByteSizeLong() const {
          const $classname$& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          $WeakDescriptorSelfPin$;
          $annotate_bytesize$;
          // @@protoc_insertion_point(message_byte_size_start:$full_name$)
          ::size_t total_size = 0;
          $handle_extension_set$;

          $uint32$ cached_has_bits = 0;
          // Prevent compiler warnings about cached_has_bits being unused
          (void)cached_has_bits;

          $prefetch$;
          $handle_fields$;
          $handle_oneof_fields$;
          $handle_weak_fields$;
          $handle_unknown_fields$;
        }
      )cc");
}

template <typename T>
void MessageGenerator::EmitOneofFields(io::Printer* p, const T& emitter) const {
  // Fields inside a oneof don't use _has_bits_ so we count them in a
  // separate pass.
  for (auto oneof : OneOfRange(descriptor_)) {
    p->Emit({{"oneof_name", oneof->name()},
             {"oneof_case_name", absl::AsciiStrToUpper(oneof->name())},
             {"case_per_field",
              [&] {
                for (auto field : FieldRange(oneof)) {
                  PrintFieldComment(Formatter{p}, field, options_);
                  p->Emit({{"field_name",
                            UnderscoresToCamelCase(field->name(), true)},
                           {"field_byte_size", [&] { emitter(p, field); }}},
                          R"cc(
                            case k$field_name$: {
                              $field_byte_size$;
                              break;
                            }
                          )cc");
                }
              }}},
            R"cc(
              switch (this_.$oneof_name$_case()) {
                $case_per_field$;
                case $oneof_case_name$_NOT_SET: {
                  break;
                }
              }
            )cc");
  }
}

void MessageGenerator::GenerateByteSizeV2(io::Printer* p) {
}

void MessageGenerator::EmitCheckAndSerializeField(const FieldDescriptor* field,
                                                  io::Printer* p) const {
  absl::AnyInvocable<void()> emit_body = [&] {
  };
  if (!HasHasbit(field)) {
    MayEmitIfNonDefaultCheck(p, "this_.", field, std::move(emit_body),
                             /*with_enclosing_braces_always=*/true);
    return;
  }

  if (field->options().weak()) {
    p->Emit({{"emit_body", [&] { emit_body(); }}},
            R"cc(
              if (has_$name$()) {
                $emit_body$;
              }
            )cc");
    return;
  }

  int has_bit_index = has_bit_indices_[field->index()];
  p->Emit({{"condition", GenerateConditionMaybeWithProbabilityForField(
                             has_bit_index, field, options_)},
           {"check_nondefault_and_emit_body",
            [&] {
              // Note that it's possible that the field has explicit presence.
              // In that case, nondefault check will not be emitted but
              // emit_body will still be emitted.
              MayEmitIfNonDefaultCheck(p, "this_.", field, std::move(emit_body),
                                       /*with_enclosing_braces_always=*/false);
            }}},
          R"cc(
            if ($condition$) {
              $check_nondefault_and_emit_body$;
            }
          )cc");
}

void MessageGenerator::GenerateSerializeV2(io::Printer* p) {
}

bool MessageGenerator::NeedsIsInitialized() {
  if (HasSimpleBaseClass(descriptor_, options_)) return false;
  if (descriptor_->extension_range_count() != 0) return true;
  if (num_required_fields_ != 0) return true;

  for (const auto* field : optimized_order_) {
    if (field_generators_.get(field).NeedsIsInitialized()) return true;
  }
  if (num_weak_fields_ != 0) return true;

  for (const auto* oneof : OneOfRange(descriptor_)) {
    for (const auto* field : FieldRange(oneof)) {
      if (field_generators_.get(field).NeedsIsInitialized()) return true;
    }
  }

  return false;
}

void MessageGenerator::GenerateIsInitialized(io::Printer* p) {
  if (!NeedsIsInitialized()) return;

  auto has_required_field = [&](const auto* oneof) {
    for (const auto* field : FieldRange(oneof)) {
      if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
          !ShouldIgnoreRequiredFieldCheck(field, options_) &&
          scc_analyzer_->HasRequiredFields(field->message_type())) {
        return true;
      }
    }
    return false;
  };

  p->Emit(
      {
          {"test_extensions",
           [&] {
             if (descriptor_->extension_range_count() == 0) return;
             p->Emit(R"cc(
               if (!this_.$extensions$.IsInitialized(&default_instance())) {
                 return false;
               }
             )cc");
           }},
          {"test_required_fields",
           [&] {
             if (num_required_fields_ == 0) return;
             p->Emit(R"cc(
               if (_Internal::MissingRequiredFields(this_.$has_bits$)) {
                 return false;
               }
             )cc");
           }},
          {"test_ordinary_fields",
           [&] {
             for (const auto* field : optimized_order_) {
               auto& f = field_generators_.get(field);
               // XXX REMOVE? XXX
               const auto needs_verifier =
                   !f.NeedsIsInitialized()
                       ? std::make_optional(p->WithSubstitutionListener(
                             [&](auto label, auto loc) {
                               ABSL_LOG(FATAL)
                                   << "Field generated output but is marked as "
                                      "!NeedsIsInitialized"
                                   << field->full_name();
                             }))
                       : std::nullopt;
               f.GenerateIsInitialized(p);
             }
           }},
          {"test_weak_fields",
           [&] {
             if (num_weak_fields_ == 0) return;
             p->Emit(R"cc(
               if (!this_.$weak_field_map$.IsInitialized())
                 return false;
             )cc");
           }},
          {"test_oneof_fields",
           [&] {
             for (const auto* oneof : OneOfRange(descriptor_)) {
               if (!has_required_field(oneof)) continue;
               p->Emit({{"name", oneof->name()},
                        {"NAME", absl::AsciiStrToUpper(oneof->name())},
                        {"cases",
                         [&] {
                           for (const auto* field : FieldRange(oneof)) {
                             p->Emit({{"Name", UnderscoresToCamelCase(
                                                   field->name(), true)},
                                      {"body",
                                       [&] {
                                         field_generators_.get(field)
                                             .GenerateIsInitialized(p);
                                       }}},
                                     R"cc(
                                       case k$Name$: {
                                         $body$;
                                         break;
                                       }
                                     )cc");
                           }
                         }}},
                       R"cc(
                         switch (this_.$name$_case()) {
                           $cases$;
                           case $NAME$_NOT_SET: {
                             break;
                           }
                         }
                       )cc");
             }
           }},
      },
      R"cc(
        PROTOBUF_NOINLINE bool $classname$::IsInitializedImpl(
            const MessageLite& msg) {
          auto& this_ = static_cast<const $classname$&>(msg);
          $test_extensions$;
          $test_required_fields$;
          $test_ordinary_fields$;
          $test_weak_fields$;
          $test_oneof_fields$;
          return true;
        }
      )cc");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
