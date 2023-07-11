// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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

#include "google/protobuf/feature_resolver.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection_ops.h"
#include "google/protobuf/text_format.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#define RETURN_IF_ERROR(expr)                                  \
  do {                                                         \
    const absl::Status _status = (expr);                       \
    if (PROTOBUF_PREDICT_FALSE(!_status.ok())) return _status; \
  } while (0)

namespace google {
namespace protobuf {
namespace {

template <typename... Args>
absl::Status Error(Args... args) {
  return absl::FailedPreconditionError(absl::StrCat(args...));
}

bool IsNonFeatureField(const FieldDescriptor& field) {
  return field.containing_type() &&
         field.containing_type()->full_name() == "google.protobuf.FeatureSet" &&
         field.name() == "raw_features";
}

bool EditionsLessThan(absl::string_view a, absl::string_view b) {
  std::vector<absl::string_view> as = absl::StrSplit(a, '.');
  std::vector<absl::string_view> bs = absl::StrSplit(b, '.');
  size_t min_size = std::min(as.size(), bs.size());
  for (size_t i = 0; i < min_size; ++i) {
    if (as[i].size() != bs[i].size()) {
      return as[i].size() < bs[i].size();
    } else if (as[i] != bs[i]) {
      return as[i] < bs[i];
    }
  }
  // Both strings are equal up until an extra element, which makes that string
  // more recent.
  return as.size() < bs.size();
}

absl::Status ValidateDescriptor(absl::string_view edition,
                                const Descriptor& descriptor) {
  if (descriptor.oneof_decl_count() > 0) {
    return Error("Type ", descriptor.full_name(),
                 " contains unsupported oneof feature fields.");
  }
  for (int i = 0; i < descriptor.field_count(); ++i) {
    const FieldDescriptor& field = *descriptor.field(i);
    if (IsNonFeatureField(field)) continue;

    if (field.is_required()) {
      return Error("Feature field ", field.full_name(),
                   " is an unsupported required field.");
    }
    if (field.is_repeated()) {
      return Error("Feature field ", field.full_name(),
                   " is an unsupported repeated field.");
    }
    if (field.options().targets().empty()) {
      return Error("Feature field ", field.full_name(),
                   " has no target specified.");
    }
  }

  return absl::OkStatus();
}

absl::Status FillDefaults(absl::string_view edition, Message& msg) {
  const Descriptor& descriptor = *msg.GetDescriptor();

  auto comparator = [](const FieldOptions::EditionDefault& a,
                       const FieldOptions::EditionDefault& b) {
    return EditionsLessThan(a.edition(), b.edition());
  };
  FieldOptions::EditionDefault edition_lookup;
  edition_lookup.set_edition(edition);

  for (int i = 0; i < descriptor.field_count(); ++i) {
    const FieldDescriptor& field = *descriptor.field(i);
    if (IsNonFeatureField(field)) continue;

    msg.GetReflection()->ClearField(&msg, &field);
    ABSL_CHECK(!field.is_repeated());

    std::vector<FieldOptions::EditionDefault> defaults{
        field.options().edition_defaults().begin(),
        field.options().edition_defaults().end()};
    absl::c_sort(defaults, comparator);
    auto first_nonmatch =
        absl::c_upper_bound(defaults, edition_lookup, comparator);
    if (first_nonmatch == defaults.begin()) {
      return Error("No valid default found for edition ", edition,
                   " in feature field ", field.full_name());
    }

    if (field.cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      for (auto it = defaults.begin(); it != first_nonmatch; ++it) {
        if (!TextFormat::MergeFromString(
                it->value(),
                msg.GetReflection()->MutableMessage(&msg, &field))) {
          return Error("Parsing error in edition_defaults for feature field ",
                       field.full_name(), ". Could not parse: ", it->value());
        }
      }
    } else {
      const std::string& def = std::prev(first_nonmatch)->value();
      if (!TextFormat::ParseFieldValueFromString(def, &field, &msg)) {
        return Error("Parsing error in edition_defaults for feature field ",
                     field.full_name(), ". Could not parse: ", def);
      }
    }
  }

  return absl::OkStatus();
}

absl::Status ValidateMergedFeatures(const Message& msg) {
  const Descriptor& descriptor = *msg.GetDescriptor();
  const Reflection& reflection = *msg.GetReflection();
  for (int i = 0; i < descriptor.field_count(); ++i) {
    const FieldDescriptor& field = *descriptor.field(i);
    // Validate enum features.
    if (field.enum_type() != nullptr) {
      ABSL_DCHECK(reflection.HasField(msg, &field));
      int int_value = reflection.GetEnumValue(msg, &field);
      const EnumValueDescriptor* value =
          field.enum_type()->FindValueByNumber(int_value);
      ABSL_DCHECK(value != nullptr);
      if (value->number() == 0) {
        return Error("Feature field ", field.full_name(),
                     " must resolve to a known value, found ", value->name());
      }
    }
  }

  return absl::OkStatus();
}

}  // namespace

absl::StatusOr<FeatureResolver> FeatureResolver::Create(
    absl::string_view edition, const Descriptor* descriptor) {
  if (descriptor == nullptr) {
    return Error(
        "Unable to find definition of google.protobuf.FeatureSet in descriptor pool.");
  }

  RETURN_IF_ERROR(ValidateDescriptor(edition, *descriptor));

  auto message_factory = absl::make_unique<DynamicMessageFactory>();
  auto defaults =
      absl::WrapUnique(message_factory->GetPrototype(descriptor)->New());

  RETURN_IF_ERROR(FillDefaults(edition, *defaults));

  return FeatureResolver(edition, *descriptor, std::move(message_factory),
                         std::move(defaults));
}

absl::Status FeatureResolver::RegisterExtension(
    const FieldDescriptor& extension) {
  if (!extension.is_extension() ||
      extension.containing_type() != &descriptor_ ||
      extensions_.contains(&extension)) {
    // These are valid but irrelevant extensions, just return ok.
    return absl::OkStatus();
  }

  ABSL_CHECK(descriptor_.IsExtensionNumber(extension.number()));

  if (extension.message_type() == nullptr) {
    return Error("FeatureSet extension ", extension.full_name(),
                 " is not of message type.  Feature extensions should "
                 "always use messages to allow for evolution.");
  }

  if (extension.is_repeated()) {
    return Error(
        "Only singular features extensions are supported.  Found "
        "repeated extension ",
        extension.full_name());
  }

  if (extension.message_type()->extension_count() > 0 ||
      extension.message_type()->extension_range_count() > 0) {
    return Error("Nested extensions in feature extension ",
                 extension.full_name(), " are not supported.");
  }

  RETURN_IF_ERROR(ValidateDescriptor(edition_, *extension.message_type()));

  Message* msg =
      defaults_->GetReflection()->MutableMessage(defaults_.get(), &extension);
  ABSL_CHECK(msg != nullptr);
  RETURN_IF_ERROR(FillDefaults(edition_, *msg));

  extensions_.insert(&extension);
  return absl::OkStatus();
}

absl::Status FeatureResolver::RegisterExtensions(const FileDescriptor& file) {
  for (int i = 0; i < file.extension_count(); ++i) {
    RETURN_IF_ERROR(RegisterExtension(*file.extension(i)));
  }
  for (int i = 0; i < file.message_type_count(); ++i) {
    RETURN_IF_ERROR(RegisterExtensions(*file.message_type(i)));
  }
  return absl::OkStatus();
}

absl::Status FeatureResolver::RegisterExtensions(const Descriptor& message) {
  for (int i = 0; i < message.extension_count(); ++i) {
    RETURN_IF_ERROR(RegisterExtension(*message.extension(i)));
  }
  for (int i = 0; i < message.nested_type_count(); ++i) {
    RETURN_IF_ERROR(RegisterExtensions(*message.nested_type(i)));
  }
  return absl::OkStatus();
}

absl::StatusOr<FeatureSet> FeatureResolver::MergeFeatures(
    const FeatureSet& merged_parent, const FeatureSet& unmerged_child) const {
  FeatureSet merged;
  ABSL_CHECK(merged.ParseFromString(defaults_->SerializeAsString()));
  merged.MergeFrom(merged_parent);
  merged.MergeFrom(unmerged_child);

  RETURN_IF_ERROR(ValidateMergedFeatures(merged));

  return merged;
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
