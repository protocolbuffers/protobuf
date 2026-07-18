// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/option_interpreter.h"

#include <fcntl.h>
#include <limits.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <new>  // IWYU pragma: keep
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/functional/function_ref.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/any.h"
#include "google/protobuf/cpp_features.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor_builder.h"
#include "google/protobuf/descriptor_lite.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/json_enumvalue_options.pb.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/symbol.h"
#include "google/protobuf/symbol_checker.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unknown_field_set.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

OptionInterpreter::OptionInterpreter(DescriptorBuilder* builder)
    : builder_(builder) {
  ABSL_CHECK(builder_);
}

OptionInterpreter::~OptionInterpreter() = default;

bool OptionInterpreter::InterpretOptionExtensions(
    OptionsToInterpret* options_to_interpret) {
  return InterpretOptionsImpl(options_to_interpret, /*skip_extensions=*/false);
}
bool OptionInterpreter::InterpretNonExtensionOptions(
    OptionsToInterpret* options_to_interpret) {
  return InterpretOptionsImpl(options_to_interpret, /*skip_extensions=*/true);
}
bool OptionInterpreter::InterpretOptionsImpl(
    OptionsToInterpret* options_to_interpret, bool skip_extensions) {
  // Note that these may be in different pools, so we can't use the same
  // descriptor and reflection objects on both.
  Message* options = options_to_interpret->options;
  const Message* original_options = options_to_interpret->original_options;

  bool failed = false;
  options_to_interpret_ = options_to_interpret;

  // Find the uninterpreted_option field in the mutable copy of the options
  // and clear them, since we're about to interpret them.
  const FieldDescriptor* uninterpreted_options_field =
      options->GetDescriptor()->FindFieldByName("uninterpreted_option");
  ABSL_CHECK(uninterpreted_options_field != nullptr)
      << "No field named \"uninterpreted_option\" in the Options proto.";
  options->GetReflection()->ClearField(options, uninterpreted_options_field);

  SourceCodePath src_path = options_to_interpret->element_path;
  src_path.push_back(uninterpreted_options_field->number());

  // Find the uninterpreted_option field in the original options.
  const FieldDescriptor* original_uninterpreted_options_field =
      original_options->GetDescriptor()->FindFieldByName(
          "uninterpreted_option");
  ABSL_CHECK(original_uninterpreted_options_field != nullptr)
      << "No field named \"uninterpreted_option\" in the Options proto.";

  const int num_uninterpreted_options =
      original_options->GetReflection()->FieldSize(
          *original_options, original_uninterpreted_options_field);
  for (int i = 0; i < num_uninterpreted_options; ++i) {
    src_path.push_back(i);
    uninterpreted_option_ = DownCastMessage<UninterpretedOption>(
        &original_options->GetReflection()->GetRepeatedMessage(
            *original_options, original_uninterpreted_options_field, i));
    if (!InterpretSingleOption(options, src_path,
                               options_to_interpret->element_path,
                               skip_extensions)) {
      // Error already added by InterpretSingleOption().
      failed = true;
      break;
    }
    src_path.pop_back();
  }
  // Reset these, so we don't have any dangling pointers.
  uninterpreted_option_ = nullptr;
  options_to_interpret_ = nullptr;

  if (!failed) {
    // InterpretSingleOption() added the interpreted options in the
    // UnknownFieldSet, in case the option isn't yet known to us.  Now we
    // serialize the options message and deserialize it back.  That way, any
    // option fields that we do happen to know about will get moved from the
    // UnknownFieldSet into the real fields, and thus be available right away.
    // If they are not known, that's OK too. They will get reparsed into the
    // UnknownFieldSet and wait there until the message is parsed by something
    // that does know about the options.

    // Keep the unparsed options around in case the reparsing fails.
    std::unique_ptr<Message> unparsed_options(options->New());
    options->GetReflection()->Swap(unparsed_options.get(), options);

    std::string buf;
    if (!unparsed_options->AppendToString(&buf) ||
        !options->ParseFromString(buf)) {
      builder_->AddError(
          options_to_interpret->element_name, *original_options,
          DescriptorPool::ErrorCollector::OTHER, [&] {
            return absl::StrCat(
                "Some options could not be correctly parsed using the proto "
                "descriptors compiled into this binary.\n"
                "Unparsed options: ",
                unparsed_options->ShortDebugString(),
                "\n"
                "Parsing attempt:  ",
                options->ShortDebugString());
          });
      // Restore the unparsed options.
      options->GetReflection()->Swap(unparsed_options.get(), options);
    }
  }

  return !failed;
}

bool OptionInterpreter::InterpretSingleOption(
    Message* options, const SourceCodePath& src_path,
    const SourceCodePath& options_path, bool skip_extensions) {
  // First do some basic validation.
  if (uninterpreted_option_->name_size() == 0) {
    // This should never happen unless the parser has gone seriously awry or
    // someone has manually created the uninterpreted option badly.
    if (skip_extensions) {
      // Come back to it later.
      return true;
    }
    return AddNameError(
        []() -> std::string { return "Option must have a name."; });
  }
  if (uninterpreted_option_->name(0).name_part() == "uninterpreted_option") {
    if (skip_extensions) {
      // Come back to it later.
      return true;
    }
    return AddNameError([]() -> std::string {
      return "Option must not use reserved name \"uninterpreted_option\".";
    });
  }

  if (skip_extensions == uninterpreted_option_->name(0).is_extension()) {
    // Allow feature and option interpretation to occur in two phases.  This is
    // necessary because features *are* options and need to be interpreted
    // before resolving them.  However, options can also *have* features
    // attached to them.
    return true;
  }

  const Descriptor* options_descriptor = nullptr;
  // Get the options message's descriptor from the builder's pool, so that we
  // get the version that knows about any extension options declared in the file
  // we're currently building. The descriptor should be there as long as the
  // file we're building imported descriptor.proto.

  // Note that we use internal::DescriptorBuilder::FindSymbolNotEnforcingDeps(),
  // not DescriptorPool::FindMessageTypeByName() because we're already holding
  // the pool's mutex, and the latter method locks it again.  We don't use
  // FindSymbol() because files that use custom options only need to depend on
  // the file that defines the option, not descriptor.proto itself.
  Symbol symbol = builder_->FindSymbolNotEnforcingDeps(
      options->GetDescriptor()->full_name());
  options_descriptor = symbol.descriptor();
  if (options_descriptor == nullptr) {
    // The options message's descriptor was not in the builder's pool, so use
    // the standard version from the generated pool. We're not holding the
    // generated pool's mutex, so we can search it the straightforward way.
    options_descriptor = options->GetDescriptor();
  }
  ABSL_CHECK(options_descriptor);

  // We iterate over the name parts to drill into the submessages until we find
  // the leaf field for the option. As we drill down we remember the current
  // submessage's descriptor in |descriptor| and the next field in that
  // submessage in |field|. We also track the fields we're drilling down
  // through in |intermediate_fields|. As we go, we reconstruct the full option
  // name in |debug_msg_name|, for use in error messages.
  const Descriptor* descriptor = options_descriptor;
  const FieldDescriptor* field = nullptr;
  std::vector<const FieldDescriptor*> intermediate_fields;
  std::string debug_msg_name = "";

  SourceCodePath dest_path = options_path;

  for (int i = 0; i < uninterpreted_option_->name_size(); ++i) {
    builder_->undefine_resolved_name_.clear();
    const std::string& name_part = uninterpreted_option_->name(i).name_part();
    if (!debug_msg_name.empty()) {
      absl::StrAppend(&debug_msg_name, ".");
    }
    if (uninterpreted_option_->name(i).is_extension()) {
      absl::StrAppend(&debug_msg_name, "(", name_part, ")");
      // Search for the extension's descriptor as an extension in the builder's
      // pool. Note that we use internal::DescriptorBuilder::LookupSymbol(), not
      // DescriptorPool::FindExtensionByName(), for two reasons: 1) It allows
      // relative lookups, and 2) because we're already holding the pool's
      // mutex, and the latter method locks it again.
      symbol =
          builder_->LookupSymbol(name_part, options_to_interpret_->name_scope);
      field = symbol.field_descriptor();
      // If we don't find the field then the field's descriptor was not in the
      // builder's pool, but there's no point in looking in the generated
      // pool. We require that you import the file that defines any extensions
      // you use, so they must be present in the builder's pool.
    } else {
      absl::StrAppend(&debug_msg_name, name_part);
      // Search for the field's descriptor as a regular field.
      field = descriptor->FindFieldByName(name_part);
    }

    if (field == nullptr) {
      if (DescriptorBuilder::get_allow_unknown(builder_->pool_)) {
        // We can't find the option, but AllowUnknownDependencies() is enabled,
        // so we will just leave it as uninterpreted.
        AddWithoutInterpreting(*uninterpreted_option_, options);
        return true;
      } else if (!(builder_->undefine_resolved_name_).empty()) {
        // Option is resolved to a name which is not defined.
        return AddNameError([&] {
          return absl::StrCat(
              "Option \"", debug_msg_name, "\" is resolved to \"(",
              builder_->undefine_resolved_name_,
              ")\", which is not defined. The innermost scope is searched "
              "first "
              "in name resolution. Consider using a leading '.'(i.e., \"(.",
              debug_msg_name.substr(1),
              "\") to start from the outermost scope.");
        });
      } else {
        return AddNameError([&] {
          return absl::StrCat("Option \"", debug_msg_name,
                              "\" unknown. Ensure that your proto",
                              " definition file imports the proto which "
                              "defines the option (i.e. via import option "
                              "after edition 2024).");
        });
      }
    } else if (field->containing_type() != descriptor) {
      if (DescriptorBuilder::get_is_placeholder(field->containing_type())) {
        // The field is an extension of a placeholder type, so we can't
        // reliably verify whether it is a valid extension to use here (e.g.
        // we don't know if it is an extension of the correct *Options message,
        // or if it has a valid field number, etc.).  Just leave it as
        // uninterpreted instead.
        AddWithoutInterpreting(*uninterpreted_option_, options);
        return true;
      } else {
        // This can only happen if, due to some insane misconfiguration of the
        // pools, we find the options message in one pool but the field in
        // another. This would probably imply a hefty bug somewhere.
        return AddNameError([&] {
          return absl::StrCat("Option field \"", debug_msg_name,
                              "\" is not a field or extension of message \"",
                              descriptor->name(), "\".");
        });
      }
    } else {
      // accumulate field numbers to form path to interpreted option
      dest_path.push_back(field->number());

      // Special handling to prevent feature use in the same file as the
      // definition.
      // TODO Add proper support for cases where this can work.
      if (field->file() == builder_->file_ &&
          uninterpreted_option_->name(0).name_part() == "features" &&
          !uninterpreted_option_->name(0).is_extension()) {
        return AddNameError([&] {
          return absl::StrCat(
              "Feature \"", debug_msg_name,
              "\" can't be used in the same file it's defined in.");
        });
      }

      if (i < uninterpreted_option_->name_size() - 1) {
        if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
          return AddNameError([&] {
            return absl::StrCat("Option \"", debug_msg_name,
                                "\" is an atomic type, not a message.");
          });
        } else if (field->is_repeated()) {
          return AddNameError([&] {
            return absl::StrCat("Option field \"", debug_msg_name,
                                "\" is a repeated message. Repeated message "
                                "options must be initialized using an "
                                "aggregate value.");
          });
        } else {
          // Drill down into the submessage.
          intermediate_fields.push_back(field);
          descriptor = field->message_type();
        }
      }
    }
  }

  // We've found the leaf field. Now we use UnknownFieldSets to set its value
  // on the options message. We do so because the message may not yet know
  // about its extension fields, so we may not be able to set the fields
  // directly. But the UnknownFieldSets will serialize to the same wire-format
  // message, so reading that message back in once the extension fields are
  // known will populate them correctly.

  // First see if the option is already set.
  if (!field->is_repeated() &&
      !ExamineIfOptionIsSet(
          intermediate_fields.begin(), intermediate_fields.end(), field,
          debug_msg_name,
          options->GetReflection()->GetUnknownFields(*options))) {
    return false;  // ExamineIfOptionIsSet() already added the error.
  }

  // First set the value on the UnknownFieldSet corresponding to the
  // innermost message.
  std::unique_ptr<UnknownFieldSet> unknown_fields =
      std::make_unique<UnknownFieldSet>();
  if (!SetOptionValue(field, unknown_fields.get(), options)) {
    return false;  // SetOptionValue() already added the error.
  }

  // Now wrap the UnknownFieldSet with UnknownFieldSets corresponding to all
  // the intermediate messages.
  for (std::vector<const FieldDescriptor*>::reverse_iterator iter =
           intermediate_fields.rbegin();
       iter != intermediate_fields.rend(); ++iter) {
    std::unique_ptr<UnknownFieldSet> parent_unknown_fields =
        std::make_unique<UnknownFieldSet>();
    switch ((*iter)->type()) {
      case FieldDescriptor::TYPE_MESSAGE: {
        std::string outstr;
        ABSL_CHECK(unknown_fields->SerializeToString(&outstr))
            << "Unexpected failure while serializing option submessage "
            << debug_msg_name << "\".";
        parent_unknown_fields->AddLengthDelimited((*iter)->number(),
                                                  std::move(outstr));
        break;
      }

      case FieldDescriptor::TYPE_GROUP: {
        parent_unknown_fields->AddGroup((*iter)->number())
            ->MergeFrom(*unknown_fields);
        break;
      }

      default:
        ABSL_LOG(FATAL) << "Invalid wire type for CPPTYPE_MESSAGE: "
                        << (*iter)->type();
        return false;
    }
    unknown_fields = std::move(parent_unknown_fields);
  }

  // Now merge the UnknownFieldSet corresponding to the top-level message into
  // the options message.
  options->GetReflection()->MutableUnknownFields(options)->MergeFrom(
      *unknown_fields);

  // record the element path of the interpreted option
  if (field->is_repeated()) {
    int index = repeated_option_counts_[dest_path]++;
    dest_path.push_back(index);
  }
  interpreted_paths_[src_path] = dest_path;

  return true;
}

void OptionInterpreter::UpdateSourceCodeInfo(SourceCodeInfo* info) {
  if (interpreted_paths_.empty()) {
    // nothing to do!
    return;
  }

  // We find locations that match keys in interpreted_paths_ and
  // 1) replace the path with the corresponding value in interpreted_paths_
  // 2) remove any subsequent sub-locations (sub-location is one whose path
  //    has the parent path as a prefix), except for direct children (like
  //    option name and value) which are mapped to the interpreted path.
  //
  // To avoid quadratic behavior of removing interior rows as we go,
  // we keep a copy. But we don't actually copy anything until we've
  // found the first match (so if the source code info has no locations
  // that need to be changed, there is zero copy overhead).

  // The original repeated field of source code locations in the file.
  RepeatedPtrField<SourceCodeInfo_Location>* locs = info->mutable_location();

  // The new repeated field of source code locations being built to replace
  // locs.
  RepeatedPtrField<SourceCodeInfo_Location> new_locs;

  // Indicates whether we have started copying locations to new_locs. To avoid
  // unnecessary copying overhead when no locations need modification, copying
  // remains false until the first matching uninterpreted option location is
  // found.
  bool copying = false;

  // The uninterpreted option path (e.g., [options, index]) currently being
  // matched and replaced.
  SourceCodePath match_src;

  // The interpreted option path (e.g., [options, custom_option_tag]) that
  // replaces match_src.
  SourceCodePath match_dest;

  // Indicates whether we are currently traversing child locations of an
  // uninterpreted option that was matched in a previous iteration. When true,
  // child sub-locations are inspected and either remapped or removed.
  bool matched = false;

  for (RepeatedPtrField<SourceCodeInfo_Location>::iterator loc = locs->begin();
       loc != locs->end(); loc++) {
    if (matched) {
      // see if this location is in the range to remove
      bool loc_matches = true;
      if (loc->path_size() < static_cast<int64_t>(match_src.size())) {
        loc_matches = false;
      } else {
        for (size_t j = 0; j < match_src.size(); j++) {
          if (loc->path(j) != match_src[j]) {
            loc_matches = false;
            break;
          }
        }
      }

      if (loc_matches) {
        if (loc->path_size() == static_cast<int64_t>(match_src.size() + 1)) {
          int uninterpreted_field = loc->path(match_src.size());

          SourceCodeInfo_Location* mapped_loc = new_locs.Add();
          *mapped_loc = *loc;
          mapped_loc->mutable_path()->Assign(match_dest.begin(),
                                             match_dest.end());
          mapped_loc->add_path(uninterpreted_field);

          // TODO: b/168903973 - recursively process options with aggregate
          // values and add locations. Example: [(my_opt) = {a: 1, b: 2}]
          // Locations of `a` and `b` are not added yet.
        }
        // don't copy this row since it is a sub-location that we're removing
        // (or we already mapped it if it's a direct child)
        continue;
      }

      matched = false;
    }

    SourceCodePath curr_path(loc->path().begin(), loc->path().end());
    auto entry = interpreted_paths_.find(curr_path);

    if (entry == interpreted_paths_.end()) {
      // not a match
      if (copying) {
        *new_locs.Add() = *loc;
      }
      continue;
    }

    matched = true;
    match_src = std::move(curr_path);
    match_dest = entry->second;

    if (!copying) {
      // initialize the copy we are building
      copying = true;
      new_locs.Reserve(locs->size());
      // Copy all the locations we've seen so far
      new_locs.Add(locs->begin(), loc);
    }

    // add replacement and update its path
    SourceCodeInfo_Location* replacement = new_locs.Add();
    *replacement = *loc;
    replacement->mutable_path()->Assign(entry->second.begin(),
                                        entry->second.end());
  }

  // if we made a changed copy, put it in place
  if (copying) {
    *locs = std::move(new_locs);
  }
}

void OptionInterpreter::AddWithoutInterpreting(
    const UninterpretedOption& uninterpreted_option, Message* options) {
  const FieldDescriptor* field =
      options->GetDescriptor()->FindFieldByName("uninterpreted_option");
  ABSL_CHECK(field != nullptr);

  options->GetReflection()
      ->AddMessage(options, field)
      ->CopyFrom(uninterpreted_option);
}

bool OptionInterpreter::ExamineIfOptionIsSet(
    std::vector<const FieldDescriptor*>::const_iterator
        intermediate_fields_iter,
    std::vector<const FieldDescriptor*>::const_iterator intermediate_fields_end,
    const FieldDescriptor* innermost_field, const std::string& debug_msg_name,
    const UnknownFieldSet& unknown_fields) {
  // We do linear searches of the UnknownFieldSet and its sub-groups.  This
  // should be fine since it's unlikely that any one options structure will
  // contain more than a handful of options.

  if (intermediate_fields_iter == intermediate_fields_end) {
    // We're at the innermost submessage.
    for (int i = 0; i < unknown_fields.field_count(); i++) {
      if (unknown_fields.field(i).number() == innermost_field->number()) {
        return AddNameError([&] {
          return absl::StrCat("Option \"", debug_msg_name,
                              "\" was already set.");
        });
      }
    }
    return true;
  }

  for (int i = 0; i < unknown_fields.field_count(); i++) {
    if (unknown_fields.field(i).number() ==
        (*intermediate_fields_iter)->number()) {
      const UnknownField* unknown_field = &unknown_fields.field(i);
      FieldDescriptor::Type type = (*intermediate_fields_iter)->type();
      // Recurse into the next submessage.
      switch (type) {
        case FieldDescriptor::TYPE_MESSAGE:
          if (unknown_field->type() == UnknownField::TYPE_LENGTH_DELIMITED) {
            UnknownFieldSet intermediate_unknown_fields;
            if (intermediate_unknown_fields.ParseFromString(
                    unknown_field->length_delimited()) &&
                !ExamineIfOptionIsSet(intermediate_fields_iter + 1,
                                      intermediate_fields_end, innermost_field,
                                      debug_msg_name,
                                      intermediate_unknown_fields)) {
              return false;  // Error already added.
            }
          }
          break;

        case FieldDescriptor::TYPE_GROUP:
          if (unknown_field->type() == UnknownField::TYPE_GROUP) {
            if (!ExamineIfOptionIsSet(intermediate_fields_iter + 1,
                                      intermediate_fields_end, innermost_field,
                                      debug_msg_name, unknown_field->group())) {
              return false;  // Error already added.
            }
          }
          break;

        default:
          ABSL_LOG(FATAL) << "Invalid wire type for CPPTYPE_MESSAGE: " << type;
          return false;
      }
    }
  }
  return true;
}

namespace {
// Helpers for method below

template <typename T>
std::string ValueOutOfRange(absl::string_view type_name,
                            absl::string_view option_name) {
  return absl::StrFormat("Value out of range, %d to %d, for %s option \"%s\".",
                         std::numeric_limits<T>::min(),
                         std::numeric_limits<T>::max(), type_name, option_name);
}

template <typename T>
std::string ValueMustBeInt(absl::string_view type_name,
                           absl::string_view option_name) {
  return absl::StrFormat(
      "Value must be integer, from %d to %d, for %s option \"%s\".",
      std::numeric_limits<T>::min(), std::numeric_limits<T>::max(), type_name,
      option_name);
}

}  // namespace

bool OptionInterpreter::SetOptionValue(const FieldDescriptor* option_field,
                                       UnknownFieldSet* unknown_fields,
                                       Message* options) {
  // We switch on the CppType to validate.
  switch (option_field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      if (uninterpreted_option_->has_positive_int_value()) {
        if (uninterpreted_option_->positive_int_value() >
            static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
          return AddValueError([&] {
            return ValueOutOfRange<int32_t>("int32", option_field->full_name());
          });
        } else {
          SetInt32(option_field->number(),
                   uninterpreted_option_->positive_int_value(),
                   option_field->type(), unknown_fields);
        }
      } else if (uninterpreted_option_->has_negative_int_value()) {
        if (uninterpreted_option_->negative_int_value() <
            static_cast<int64_t>(std::numeric_limits<int32_t>::min())) {
          return AddValueError([&] {
            return ValueOutOfRange<int32_t>("int32", option_field->full_name());
          });
        } else {
          SetInt32(option_field->number(),
                   uninterpreted_option_->negative_int_value(),
                   option_field->type(), unknown_fields);
        }
      } else {
        return AddValueError([&] {
          return ValueMustBeInt<int32_t>("int32", option_field->full_name());
        });
      }
      break;

    case FieldDescriptor::CPPTYPE_INT64:
      if (uninterpreted_option_->has_positive_int_value()) {
        if (uninterpreted_option_->positive_int_value() >
            static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
          return AddValueError([&] {
            return ValueOutOfRange<int64_t>("int64", option_field->full_name());
          });
        } else {
          SetInt64(option_field->number(),
                   uninterpreted_option_->positive_int_value(),
                   option_field->type(), unknown_fields);
        }
      } else if (uninterpreted_option_->has_negative_int_value()) {
        SetInt64(option_field->number(),
                 uninterpreted_option_->negative_int_value(),
                 option_field->type(), unknown_fields);
      } else {
        return AddValueError([&] {
          return ValueMustBeInt<int64_t>("int64", option_field->full_name());
        });
      }
      break;

    case FieldDescriptor::CPPTYPE_UINT32:
      if (uninterpreted_option_->has_positive_int_value()) {
        if (uninterpreted_option_->positive_int_value() >
            std::numeric_limits<uint32_t>::max()) {
          return AddValueError([&] {
            return ValueOutOfRange<uint32_t>("uint32",
                                             option_field->full_name());
          });
        } else {
          SetUInt32(option_field->number(),
                    uninterpreted_option_->positive_int_value(),
                    option_field->type(), unknown_fields);
        }
      } else {
        return AddValueError([&] {
          return ValueMustBeInt<uint32_t>("uint32", option_field->full_name());
        });
      }
      break;

    case FieldDescriptor::CPPTYPE_UINT64:
      if (uninterpreted_option_->has_positive_int_value()) {
        SetUInt64(option_field->number(),
                  uninterpreted_option_->positive_int_value(),
                  option_field->type(), unknown_fields);
      } else {
        return AddValueError([&] {
          return ValueMustBeInt<uint64_t>("uint64", option_field->full_name());
        });
      }
      break;

    case FieldDescriptor::CPPTYPE_FLOAT: {
      float value;
      if (uninterpreted_option_->has_double_value()) {
        value = uninterpreted_option_->double_value();
      } else if (uninterpreted_option_->has_positive_int_value()) {
        value = uninterpreted_option_->positive_int_value();
      } else if (uninterpreted_option_->has_negative_int_value()) {
        value = uninterpreted_option_->negative_int_value();
      } else if (uninterpreted_option_->identifier_value() == "inf") {
        value = std::numeric_limits<float>::infinity();
      } else if (uninterpreted_option_->identifier_value() == "nan") {
        value = std::numeric_limits<float>::quiet_NaN();
      } else {
        return AddValueError([&] {
          return absl::StrCat("Value must be number for float option \"",
                              option_field->full_name(), "\".");
        });
      }
      unknown_fields->AddFixed32(option_field->number(),
                                 internal::WireFormatLite::EncodeFloat(value));
      break;
    }

    case FieldDescriptor::CPPTYPE_DOUBLE: {
      double value;
      if (uninterpreted_option_->has_double_value()) {
        value = uninterpreted_option_->double_value();
      } else if (uninterpreted_option_->has_positive_int_value()) {
        value = uninterpreted_option_->positive_int_value();
      } else if (uninterpreted_option_->has_negative_int_value()) {
        value = uninterpreted_option_->negative_int_value();
      } else if (uninterpreted_option_->identifier_value() == "inf") {
        value = std::numeric_limits<double>::infinity();
      } else if (uninterpreted_option_->identifier_value() == "nan") {
        value = std::numeric_limits<double>::quiet_NaN();
      } else {
        return AddValueError([&] {
          return absl::StrCat("Value must be number for double option \"",
                              option_field->full_name(), "\".");
        });
      }
      unknown_fields->AddFixed64(option_field->number(),
                                 internal::WireFormatLite::EncodeDouble(value));
      break;
    }

    case FieldDescriptor::CPPTYPE_BOOL:
      uint64_t value;
      if (!uninterpreted_option_->has_identifier_value()) {
        return AddValueError([&] {
          return absl::StrCat("Value must be identifier for boolean option \"",
                              option_field->full_name(), "\".");
        });
      }
      if (uninterpreted_option_->identifier_value() == "true") {
        value = 1;
      } else if (uninterpreted_option_->identifier_value() == "false") {
        value = 0;
      } else {
        return AddValueError([&] {
          return absl::StrCat(
              "Value must be \"true\" or \"false\" for boolean option \"",
              option_field->full_name(), "\".");
        });
      }
      unknown_fields->AddVarint(option_field->number(), value);
      break;

    case FieldDescriptor::CPPTYPE_ENUM: {
      if (!uninterpreted_option_->has_identifier_value()) {
        return AddValueError([&] {
          return absl::StrCat(
              "Value must be identifier for enum-valued option \"",
              option_field->full_name(), "\".");
        });
      }
      const EnumDescriptor* enum_type = option_field->enum_type();
      const std::string& value_name = uninterpreted_option_->identifier_value();
      const EnumValueDescriptor* enum_value = nullptr;

      if (enum_type->file()->pool() != DescriptorPool::generated_pool()) {
        // Note that the enum value's fully-qualified name is a sibling of the
        // enum's name, not a child of it.
        std::string fully_qualified_name = std::string(enum_type->full_name());
        fully_qualified_name.resize(fully_qualified_name.size() -
                                    enum_type->name().size());
        fully_qualified_name += value_name;

        // Search for the enum value's descriptor in the builder's pool. Note
        // that we use
        // internal::DescriptorBuilder::FindSymbolNotEnforcingDeps(), not
        // DescriptorPool::FindEnumValueByName() because we're already holding
        // the pool's mutex, and the latter method locks it again.
        Symbol symbol =
            builder_->FindSymbolNotEnforcingDeps(fully_qualified_name);
        if (auto* candidate_descriptor = symbol.enum_value_descriptor()) {
          if (candidate_descriptor->type() != enum_type) {
            return AddValueError([&] {
              return absl::StrCat(
                  "Enum type \"", enum_type->full_name(),
                  "\" has no value named \"", value_name, "\" for option \"",
                  option_field->full_name(),
                  "\". This appears to be a value from a sibling type.");
            });
          } else {
            enum_value = candidate_descriptor;
          }
        }
      } else {
        // The enum type is in the generated pool, so we can search for the
        // value there.
        enum_value = enum_type->FindValueByName(value_name);
      }

      if (enum_value == nullptr) {
        return AddValueError([&] {
          return absl::StrCat(
              "Enum type \"", option_field->enum_type()->full_name(),
              "\" has no value named \"", value_name, "\" for option \"",
              option_field->full_name(), "\".");
        });
      } else {
        // Sign-extension is not a problem, since we cast directly from int32_t
        // to uint64_t, without first going through uint32_t.
        unknown_fields->AddVarint(
            option_field->number(),
            static_cast<uint64_t>(static_cast<int64_t>(enum_value->number())));
      }
      break;
    }

    case FieldDescriptor::CPPTYPE_STRING:
      if (!uninterpreted_option_->has_string_value()) {
        return AddValueError([&] {
          return absl::StrCat(
              "Value must be quoted string for string option \"",
              option_field->full_name(), "\".");
        });
      }
      // The string has already been unquoted and unescaped by the parser.
      unknown_fields->AddLengthDelimited(option_field->number(),
                                         uninterpreted_option_->string_value());
      break;
    case FieldDescriptor::CPPTYPE_MESSAGE:
      if (!SetAggregateOption(option_field, unknown_fields, options)) {
        return false;
      }
  }

  return true;
}

class AggregateOptionFinder : public TextFormat::Finder {
 public:
  DescriptorBuilder* builder_;

  const Descriptor* FindAnyType(const Message& /*message*/,
                                const std::string& prefix,
                                const std::string& name) const override {
    if (prefix != internal::kTypeGoogleApisComPrefix &&
        prefix != internal::kTypeGoogleProdComPrefix) {
      return nullptr;
    }
    DescriptorBuilder::assert_mutex_held(builder_->pool_);
    return builder_->FindSymbol(name).descriptor();
  }

  const FieldDescriptor* FindExtension(Message* message,
                                       const std::string& name) const override {
    DescriptorBuilder::assert_mutex_held(builder_->pool_);
    const Descriptor* descriptor = message->GetDescriptor();
    Symbol result =
        builder_->LookupSymbolNoPlaceholder(name, descriptor->full_name());
    if (auto* field = result.field_descriptor()) {
      return field;
    } else if (result.type() == Symbol::MESSAGE &&
               descriptor->options().message_set_wire_format()) {
      const Descriptor* foreign_type = result.descriptor();
      // The text format allows MessageSet items to be specified using
      // the type name, rather than the extension identifier. If the symbol
      // lookup returned a Message, and the enclosing Message has
      // message_set_wire_format = true, then return the message set
      // extension, if one exists.
      for (int i = 0; i < foreign_type->extension_count(); i++) {
        const FieldDescriptor* extension = foreign_type->extension(i);
        if (extension->containing_type() == descriptor &&
            extension->type() == FieldDescriptor::TYPE_MESSAGE &&
            extension->label_ == FieldDescriptor::LABEL_OPTIONAL &&
            extension->message_type() == foreign_type) {
          // Found it.
          return extension;
        }
      }
    }
    return nullptr;
  }
};

// A custom error collector to record any text-format parsing errors
namespace {
class AggregateErrorCollector : public io::ErrorCollector {
 public:
  std::string error_;

  void RecordError(int /* line */, int /* column */,
                   const absl::string_view message) override {
    if (!error_.empty()) {
      absl::StrAppend(&error_, "; ");
    }
    absl::StrAppend(&error_, message);
  }

  void RecordWarning(int /* line */, int /* column */,
                     const absl::string_view /* message */) override {
    // Ignore warnings
  }
};
}  // namespace

// We construct a dynamic message of the type corresponding to
// option_field, parse the supplied text-format string into this
// message, and serialize the resulting message to produce the value.
bool OptionInterpreter::SetAggregateOption(const FieldDescriptor* option_field,
                                           UnknownFieldSet* unknown_fields,
                                           Message* options) {
  if (!uninterpreted_option_->has_aggregate_value()) {
    return AddValueError([&] {
      return absl::StrCat("Option \"", option_field->full_name(),
                          "\" is a message. "
                          "To set the entire message, use syntax like \"",
                          option_field->name(),
                          " = { <proto text format> }\". "
                          "To set fields within it, use syntax like \"",
                          option_field->name(), ".foo = value\".");
    });
  }

  const Descriptor* type = option_field->message_type();
  std::unique_ptr<Message> dynamic(dynamic_factory_.GetPrototype(type)->New());
  ABSL_CHECK(dynamic.get() != nullptr)
      << "Could not create an instance of " << option_field->DebugString();

  AggregateErrorCollector collector;
  AggregateOptionFinder finder;
  finder.builder_ = builder_;
  TextFormat::Parser parser;
  parser.RecordErrorsTo(&collector);
  parser.SetFinder(&finder);
  if (!parser.ParseFromString(uninterpreted_option_->aggregate_value(),
                              dynamic.get())) {
    if (DescriptorBuilder::get_allow_unknown(builder_->pool_)) {
      // We can't interpret the option, but AllowUnknownDependencies() is
      // enabled, so we will just leave it as uninterpreted.
      AddWithoutInterpreting(*uninterpreted_option_, options);
      return true;
    } else {
      AddValueError([&] {
        return absl::StrCat("Error while parsing option value for \"",
                            option_field->name(), "\": ", collector.error_);
      });
      return false;
    }
  } else {
    std::string serial;
    ABSL_CHECK(dynamic->SerializeToString(&serial));  // Never fails
    if (option_field->type() == FieldDescriptor::TYPE_MESSAGE) {
      unknown_fields->AddLengthDelimited(option_field->number(), serial);
    } else {
      ABSL_CHECK_EQ(option_field->type(), FieldDescriptor::TYPE_GROUP);
      UnknownFieldSet* group = unknown_fields->AddGroup(option_field->number());
      // TODO: Remove this suppression.
      (void)group->ParseFromString(serial);
    }
    return true;
  }
}

void OptionInterpreter::SetInt32(int number, int32_t value,
                                 FieldDescriptor::Type type,
                                 UnknownFieldSet* unknown_fields) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32:
      unknown_fields->AddVarint(
          number, static_cast<uint64_t>(static_cast<int64_t>(value)));
      break;

    case FieldDescriptor::TYPE_SFIXED32:
      unknown_fields->AddFixed32(number, static_cast<uint32_t>(value));
      break;

    case FieldDescriptor::TYPE_SINT32:
      unknown_fields->AddVarint(
          number, internal::WireFormatLite::ZigZagEncode32(value));
      break;

    default:
      ABSL_LOG(FATAL) << "Invalid wire type for CPPTYPE_INT32: " << type;
      break;
  }
}

void OptionInterpreter::SetInt64(int number, int64_t value,
                                 FieldDescriptor::Type type,
                                 UnknownFieldSet* unknown_fields) {
  switch (type) {
    case FieldDescriptor::TYPE_INT64:
      unknown_fields->AddVarint(number, static_cast<uint64_t>(value));
      break;

    case FieldDescriptor::TYPE_SFIXED64:
      unknown_fields->AddFixed64(number, static_cast<uint64_t>(value));
      break;

    case FieldDescriptor::TYPE_SINT64:
      unknown_fields->AddVarint(
          number, internal::WireFormatLite::ZigZagEncode64(value));
      break;

    default:
      ABSL_LOG(FATAL) << "Invalid wire type for CPPTYPE_INT64: " << type;
      break;
  }
}

void OptionInterpreter::SetUInt32(int number, uint32_t value,
                                  FieldDescriptor::Type type,
                                  UnknownFieldSet* unknown_fields) {
  switch (type) {
    case FieldDescriptor::TYPE_UINT32:
      unknown_fields->AddVarint(number, static_cast<uint64_t>(value));
      break;

    case FieldDescriptor::TYPE_FIXED32:
      unknown_fields->AddFixed32(number, static_cast<uint32_t>(value));
      break;

    default:
      ABSL_LOG(FATAL) << "Invalid wire type for CPPTYPE_UINT32: " << type;
      break;
  }
}

void OptionInterpreter::SetUInt64(int number, uint64_t value,
                                  FieldDescriptor::Type type,
                                  UnknownFieldSet* unknown_fields) {
  switch (type) {
    case FieldDescriptor::TYPE_UINT64:
      unknown_fields->AddVarint(number, value);
      break;

    case FieldDescriptor::TYPE_FIXED64:
      unknown_fields->AddFixed64(number, value);
      break;

    default:
      ABSL_LOG(FATAL) << "Invalid wire type for CPPTYPE_UINT64: " << type;
      break;
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
