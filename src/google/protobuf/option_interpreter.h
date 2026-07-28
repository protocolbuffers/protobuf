// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_OPTION_INTERPRETER_H__
#define GOOGLE_PROTOBUF_OPTION_INTERPRETER_H__

#include <cstdint>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/functional/function_ref.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_builder.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"
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

  // Interprets the uninterpreted options in the specified Options message.
  // On error, calls AddError() on the underlying builder and returns false.
  // Otherwise returns true.
  bool InterpretOptionExtensions(OptionsToInterpret* options_to_interpret);

  // Interprets the uninterpreted feature options in the specified Options
  // message. On error, calls AddError() on the underlying builder and returns
  // false. Otherwise returns true.
  bool InterpretNonExtensionOptions(OptionsToInterpret* options_to_interpret);

  // Updates the given source code info by re-writing uninterpreted option
  // locations to refer to the corresponding interpreted option.
  void UpdateSourceCodeInfo(SourceCodeInfo* info);

 private:
  bool InterpretOptionsImpl(OptionsToInterpret* options_to_interpret,
                            bool skip_extensions);

  // Interprets uninterpreted_option_ on the specified message, which
  // must be the mutable copy of the original options message to which
  // uninterpreted_option_ belongs. The given src_path is the source
  // location path to the uninterpreted option, and options_path is the
  // source location path to the options message. The location paths are
  // recorded and then used in UpdateSourceCodeInfo.
  // The features boolean controls whether or not we should only interpret
  // feature options or skip them entirely.
  bool InterpretSingleOption(Message* options, const SourceCodePath& src_path,
                             const SourceCodePath& options_path,
                             bool skip_extensions);

  // Adds the uninterpreted_option to the given options message verbatim.
  // Used when AllowUnknownDependencies() is in effect and we can't find
  // the option's definition.
  void AddWithoutInterpreting(const UninterpretedOption& uninterpreted_option,
                              Message* options);

  // A recursive helper function that drills into the intermediate fields
  // in unknown_fields to check if field innermost_field is set on the
  // innermost message. Returns false and sets an error if so.
  bool ExamineIfOptionIsSet(std::vector<const FieldDescriptor*>::const_iterator
                                intermediate_fields_iter,
                            std::vector<const FieldDescriptor*>::const_iterator
                                intermediate_fields_end,
                            const FieldDescriptor* innermost_field,
                            const std::string& debug_msg_name,
                            const UnknownFieldSet& unknown_fields);

  // Validates the value for the option field of the currently interpreted
  // option and then sets it on the unknown_field.
  bool SetOptionValue(const FieldDescriptor* option_field,
                      UnknownFieldSet* unknown_fields, Message* options,
                      const SourceCodePath& src_path,
                      const SourceCodePath& dest_path);

  // Parses an aggregate value for a CPPTYPE_MESSAGE option and
  // saves it into *unknown_fields.
  bool SetAggregateOption(const FieldDescriptor* option_field,
                          UnknownFieldSet* unknown_fields, Message* options,
                          const SourceCodePath& src_path,
                          const SourceCodePath& dest_path);

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

  // Recursively traverses a parsed aggregate option message and its
  // ParseInfoTree to collect relative locations for all populated sub-fields.
  void CollectAggregateFieldLocations(const Message& message,
                                      const TextFormat::ParseInfoTree& tree,
                                      const SourceCodePath& uninterpreted_path,
                                      SourceCodePath& dest_path);

  // Determines the appropriate UninterpretedOption value field number (e.g.,
  // kPositiveIntValueFieldNumber, kStringValueFieldNumber) for a given field.
  int GetValueMarker(const FieldDescriptor* field, const Message& message,
                     int index);

  // Translates relative ParseLocationRange coordinates (from within an
  // aggregate option string) into absolute .proto file coordinates using base
  // line/column.
  void SetSpan(SourceCodeInfo_Location* loc,
               const SourceCodeInfo_Location& base_loc,
               const TextFormat::ParseLocationRange& range);

  // Convenience functions to set an int field the right way, depending on
  // its wire type (a single int CppType can represent multiple wire types).
  void SetInt32(int number, int32_t value, FieldDescriptor::Type type,
                UnknownFieldSet* unknown_fields);
  void SetInt64(int number, int64_t value, FieldDescriptor::Type type,
                UnknownFieldSet* unknown_fields);
  void SetUInt32(int number, uint32_t value, FieldDescriptor::Type type,
                 UnknownFieldSet* unknown_fields);
  void SetUInt64(int number, uint64_t value, FieldDescriptor::Type type,
                 UnknownFieldSet* unknown_fields);

  // A helper function that adds an error at the specified location of the
  // option we're currently interpreting, and returns false.
  bool AddOptionError(DescriptorPool::ErrorCollector::ErrorLocation location,
                      absl::FunctionRef<std::string()> make_error) {
    builder_->AddError(options_to_interpret_->element_name,
                       *uninterpreted_option_, location, make_error);
    return false;
  }

  // A helper function that adds an error at the location of the option name
  // and returns false.
  bool AddNameError(absl::FunctionRef<std::string()> make_error) {
#ifdef PROTOBUF_INTERNAL_IGNORE_FIELD_NAME_ERRORS_
    return true;
#else   // PROTOBUF_INTERNAL_IGNORE_FIELD_NAME_ERRORS_
    return AddOptionError(DescriptorPool::ErrorCollector::OPTION_NAME,
                          make_error);
#endif  // PROTOBUF_INTERNAL_IGNORE_FIELD_NAME_ERRORS_
  }

  // A helper function that adds an error at the location of the option name
  // and returns false.
  bool AddValueError(absl::FunctionRef<std::string()> make_error) {
    return AddOptionError(DescriptorPool::ErrorCollector::OPTION_VALUE,
                          make_error);
  }

  // We interpret against this builder's pool. Is never nullptr. We don't own
  // this pointer.
  DescriptorBuilder* builder_;

  // The options we're currently interpreting, or nullptr if we're not in a
  // call to InterpretOptions.
  const OptionsToInterpret* options_to_interpret_;

  // The option we're currently interpreting within options_to_interpret_, or
  // nullptr if we're not in a call to InterpretOptions(). This points to a
  // submessage of the original option, not the mutable copy. Therefore we
  // can use it to find locations recorded by the parser.
  const UninterpretedOption* uninterpreted_option_;

  // This maps the element path of uninterpreted options to the element path
  // of the resulting interpreted option. This is used to modify a file's
  // source code info to account for option interpretation.
  absl::flat_hash_map<SourceCodePath, SourceCodePath> interpreted_paths_;

  // This maps the path to a repeated option field to the known number of
  // elements the field contains. This is used to track the compute the
  // index portion of the element path when interpreting a single option.
  absl::flat_hash_map<SourceCodePath, int> repeated_option_counts_;

  // Factory used to create the dynamic messages we need to parse
  // any aggregate option values we encounter.
  DynamicMessageFactory dynamic_factory_;

  // Accumulates all sub-field locations gathered during aggregate option
  // interpretation.
  std::vector<AggregateFieldLocation> aggregate_field_locations_;

  // Indicates whether source code info collection is enabled during option
  // interpretation. Source code info requires extra bookkeeping and processing,
  // and for most production use cases, is not necessary.
  bool update_source_code_info_;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_OPTION_INTERPRETER_H__
