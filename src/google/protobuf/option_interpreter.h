// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_OPTION_INTERPRETER_H__
#define GOOGLE_PROTOBUF_OPTION_INTERPRETER_H__

#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_builder.h"
#include "google/protobuf/text_format.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// OptionInterpreter is a helper class used during the descriptor building
// process to resolve and parse custom options.
//
// During initial parsing of a .proto file, custom options cannot be fully
// parsed because their definitions (which may be extensions defined in other
// files) are not yet known. The parser stores these as raw name-value pairs
// in `UninterpretedOption` messages.
//
// Once the descriptor pool has loaded dependencies and resolved symbols,
// OptionInterpreter is used to "interpret" these options:
// 1. **Name Resolution**: It resolves the path of names in the uninterpreted
//    option (e.g., `(my_extension).sub_field`) to a concrete `FieldDescriptor`.
// 2. **Value Validation**: It validates that the value in the uninterpreted
//    option matches the type of the resolved field.
// 3. **Storage as Unknown Fields**: It stores the interpreted values in the
//    target options message (e.g., `FileOptions`) using `UnknownFieldSet`.
//    This deferred binding is necessary because the options message structure
//    might not yet have the extension fields registered in its reflection.
//    Serializing and deserializing the message later will correctly populate
//    the extension fields once they are known.
// 4. **Aggregate Parsing**: It parses aggregate option values (specified
//    in text format) using `TextFormat` with a custom resolver
//    (`AggregateOptionFinder`) to handle nested extensions and types.
// 5. **Source Code Info Mapping**: It updates `SourceCodeInfo` to map the
//    source locations of the original `UninterpretedOption`s to the
//    resulting interpreted option fields, ensuring correct location reporting
//    for compilers and IDEs.
//
// OptionInterpreter operates in two passes (non-extensions first, then
// extensions) to allow feature resolution to happen in the correct order.
class OptionInterpreter {
 public:
  // Creates an interpreter that operates in the context of the pool of the
  // specified builder, which must not be nullptr. We don't take ownership of
  // the builder. If update_source_code_info is false, path tracking and map
  // population for UpdateSourceCodeInfo are omitted.
  explicit OptionInterpreter(DescriptorBuilder* builder,
                             bool update_source_code_info);
  OptionInterpreter(const OptionInterpreter&) = delete;
  OptionInterpreter& operator=(const OptionInterpreter&) = delete;

  ~OptionInterpreter();

  // Interprets the uninterpreted options in the specified Options messages.
  // On error, calls AddError() on the underlying builder and returns false.
  // Otherwise returns true.
  bool InterpretOptionExtensions(
      absl::Span<OptionsToInterpret> options_to_interpret);

  // Interprets the uninterpreted non-extension options in the specified Options
  // messages. On error, calls AddError() on the underlying builder and returns
  // false. Otherwise returns true.
  bool InterpretNonExtensionOptions(
      absl::Span<OptionsToInterpret> options_to_interpret);

  // Updates the given source code info by re-writing uninterpreted option
  // locations to refer to the corresponding interpreted option.
  void UpdateSourceCodeInfo(SourceCodeInfo* info);

 private:
  class PassInterpreter;

  // Represents the relative source location of a field specified within an
  // aggregate option block (e.g., `option (my_opt) = { field_a: 1 }`).
  struct AggregateFieldLocation {
    // Path to the uninterpreted option in the descriptor (e.g., [options,
    // index]). This is used later during UpdateSourceCodeInfo to find the
    // absolute source location (line and column) where the aggregate option
    // block starts, enabling the calculation of absolute source locations for
    // the nested fields within the block.
    SourceCodePath uninterpreted_path;
    // Destination path for the option field itself.
    SourceCodePath field_dest_path;
    // Marker for the value type (e.g. kPositiveIntValueFieldNumber, etc.)
    int value_marker;
    // Relative start and end line/column span of the field name within the
    // aggregate string.
    TextFormat::ParseLocationRange name_range;
    // Relative start and end line/column span of the field value within the
    // aggregate string.
    TextFormat::ParseLocationRange val_range;
  };

  struct SourceCodeInfoState {
    // Maps the element path of uninterpreted options to the element path
    // of the resulting interpreted option. This is used to modify a file's
    // source code info to account for option interpretation.
    absl::flat_hash_map<SourceCodePath, SourceCodePath> interpreted_paths;

    // Maps the source code path of an uninterpreted dot-notation option to the
    // sequence of destination path prefixes for each component in the
    // dot-separated name, using
    // -UninterpretedOption::kAggregateValueFieldNumber
    // (-8) to separate nested message levels.
    //
    // For example, given `option (foo).bar.baz = "123"` on a message (where
    // `foo` has field number 10101, `bar` has field number 2, and `baz` has
    // field number 3, and `options_path` is `[4, 0, 7]`):
    //   Key (src_path): `[4, 0, 7, 999, 0]` (path to the UninterpretedOption)
    //   Value:
    //     - `[0]` (for `(foo)`): `[4, 0, 7, 10101]`
    //     - `[1]` (for `bar`):   `[4, 0, 7, 10101, -8, 2]`
    //     - `[2]` (for `baz`):   `[4, 0, 7, 10101, -8, 2, -8, 3]`
    //
    // In UpdateSourceCodeInfo(), each prefix `i` is appended with
    // `-UninterpretedOption::kNameFieldNumber` (-2) to record the location of
    // the `i`-th name part, the last prefix (`back()`) is appended with
    // `-uninterpreted_field` (e.g., -7 for string_value) to record the location
    // of the option's value, and the legacy full-option path (`[4, 0, 7, 10101,
    // 2, 3]`) is derived from `back()` by stripping the `-8` markers.
    absl::flat_hash_map<SourceCodePath, std::vector<SourceCodePath>>
        dot_notation_name_paths;

    // Accumulates all sub-field locations gathered during aggregate option
    // interpretation.
    std::vector<AggregateFieldLocation> aggregate_field_locations;
  };

  bool InterpretOptionsPass(absl::Span<OptionsToInterpret> options_to_interpret,
                            bool skip_extensions);

  // Translates relative ParseLocationRange coordinates (from within an
  // aggregate option string) into absolute .proto file coordinates using base
  // line/column.
  static void SetSpan(SourceCodeInfo_Location* loc,
                      const SourceCodeInfo_Location& base_loc,
                      const TextFormat::ParseLocationRange& range);

  // We interpret against this builder's pool. Is never nullptr. We don't own
  // this pointer.
  DescriptorBuilder* builder_;

  // SourceCodeInfo tracking state retained across passes when source code info
  // updates are enabled, or absl::nullopt otherwise.
  absl::optional<SourceCodeInfoState> source_code_info_;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_OPTION_INTERPRETER_H__
