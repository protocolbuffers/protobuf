// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/retention.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/match.h"
#include "absl/types/span.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"

namespace google {
namespace protobuf {
namespace compiler {

namespace {

bool IsOptionsProto(const Message& m) {
  const Descriptor* descriptor = m.GetDescriptor();
  return descriptor->file()->name() ==
             DescriptorProto::descriptor()->file()->name() &&
         absl::EndsWith(descriptor->name(), "Options");
}

bool IsEmpty(const Message& m) { return m.ByteSizeLong() == 0; }

// Recursively strips any options with source retention from the message. If
// stripped_paths is not null, then this function will populate it with the
// paths that were stripped, using the path format from
// SourceCodeInfo.Location. The path parameter is used as a stack tracking the
// path to the current location.
void StripMessage(Message& m, std::vector<int>& path,
                  std::vector<std::vector<int>>* stripped_paths) {
  const Reflection* reflection = m.GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(m, &fields);
  for (const FieldDescriptor* field : fields) {
    path.push_back(field->number());
    if (field->options().retention() == FieldOptions::RETENTION_SOURCE) {
      reflection->ClearField(&m, field);
      if (stripped_paths != nullptr) {
        stripped_paths->push_back(path);
      }
    } else if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
      if (field->is_repeated()) {
        int field_size = reflection->FieldSize(m, field);
        for (int i = 0; i < field_size; ++i) {
          path.push_back(i);
          StripMessage(*reflection->MutableRepeatedMessage(&m, field, i), path,
                       stripped_paths);
          path.pop_back();
        }
      } else {
        Message* child = reflection->MutableMessage(&m, field);
        bool was_nonempty_options_proto =
            IsOptionsProto(*child) && !IsEmpty(*child);
        StripMessage(*child, path, stripped_paths);
        // If this is an options message that became empty due to retention
        // stripping, remove it.
        if (was_nonempty_options_proto && IsEmpty(*child)) {
          reflection->ClearField(&m, field);
          if (stripped_paths != nullptr) {
            stripped_paths->push_back(path);
          }
        }
      }
    }
    path.pop_back();
  }
}

// Converts the descriptor to a dynamic message if necessary, and then strips
// out all source-retention options.
//
// The options message may have custom options set on it, and these would
// ordinarily appear as unknown fields since they are not linked into protoc.
// Using a dynamic message allows us to see these custom options. To convert
// back and forth between the generated type and the dynamic message, we have
// to serialize one and parse that into the other.
//
// If stripped_paths is not null, it will be populated with the paths that were
// stripped, using the path format from SourceCodeInfo.Location.
void ConvertToDynamicMessageAndStripOptions(
    Message& m, const DescriptorPool& pool,
    std::vector<std::vector<int>>* stripped_paths = nullptr) {
  // We need to look up the descriptor in the pool so that we can get a
  // descriptor which knows about any custom options that were used in the
  // .proto file.
  const Descriptor* descriptor = pool.FindMessageTypeByName(m.GetTypeName());
  std::vector<int> path;

  if (descriptor == nullptr || &pool == DescriptorPool::generated_pool()) {
    // If the pool does not contain the descriptor, then this proto file does
    // not transitively depend on descriptor.proto, in which case we know there
    // are no custom options to worry about. If we are working with the
    // generated pool, then we can still access any custom options without
    // having to resort to DynamicMessage.
    StripMessage(m, path, stripped_paths);
  } else {
    // To convert to a dynamic message, we need to serialize the original
    // descriptor and parse it back again. This can fail if the descriptor is
    // invalid, so in that case we try to handle it gracefully by stripping the
    // original descriptor without using DynamicMessage. In this situation we
    // will generally not be able to strip custom options, but we can at least
    // strip built-in options.
    DynamicMessageFactory factory;
    std::unique_ptr<Message> dynamic_message(
        factory.GetPrototype(descriptor)->New());
    std::string serialized;
    if (!m.SerializePartialToString(&serialized)) {
      ABSL_LOG_EVERY_N_SEC(ERROR, 1)
          << "Failed to fully strip source-retention options";
      StripMessage(m, path, stripped_paths);
      return;
    }
    if (!dynamic_message->ParsePartialFromString(serialized)) {
      ABSL_LOG_EVERY_N_SEC(ERROR, 1)
          << "Failed to fully strip source-retention options";
      StripMessage(m, path, stripped_paths);
      return;
    }
    StripMessage(*dynamic_message, path, stripped_paths);
    if (!dynamic_message->SerializePartialToString(&serialized)) {
      ABSL_LOG_EVERY_N_SEC(ERROR, 1)
          << "Failed to fully strip source-retention options";
      StripMessage(m, path, stripped_paths);
      return;
    }
    if (!m.ParsePartialFromString(serialized)) {
      ABSL_LOG_EVERY_N_SEC(ERROR, 1)
          << "Failed to fully strip source-retention options";
      StripMessage(m, path, stripped_paths);
      return;
    }
  }
}

// Returns a const reference to the descriptor pool associated with the given
// descriptor.
template <typename DescriptorType>
const google::protobuf::DescriptorPool& GetPool(const DescriptorType& descriptor) {
  return *descriptor.file()->pool();
}

// Specialization for FileDescriptor.
const google::protobuf::DescriptorPool& GetPool(const FileDescriptor& descriptor) {
  return *descriptor.pool();
}

// Returns the options associated with the given descriptor, with all
// source-retention options stripped out.
template <typename DescriptorType>
auto StripLocalOptions(const DescriptorType& descriptor) {
  auto options = descriptor.options();
  ConvertToDynamicMessageAndStripOptions(options, GetPool(descriptor));
  return options;
}

// Returns true if x is a prefix of y.
bool IsPrefix(absl::Span<const int> x, absl::Span<const int> y) {
  return x == y.subspan(0, x.size());
}

// Strips the paths in stripped_paths from the SourceCodeInfo.
void StripSourceCodeInfo(std::vector<std::vector<int>>& stripped_paths,
                         SourceCodeInfo& source_code_info) {
  RepeatedPtrField<SourceCodeInfo::Location>* locations =
      source_code_info.mutable_location();

  // We sort the locations lexicographically by their paths and include an
  // index pointing back to the original location.
  std::vector<std::pair<absl::Span<const int>, int>> sorted_locations;
  sorted_locations.reserve(locations->size());
  for (int i = 0; i < locations->size(); ++i) {
    sorted_locations.emplace_back((*locations)[i].path(), i);
  }
  absl::c_sort(sorted_locations);
  absl::c_sort(stripped_paths);

  // With both arrays sorted, we can efficiently step through them in tandem.
  // If a stripped path is a prefix of any location, then that is a location
  // we need to delete from the SourceCodeInfo.
  absl::flat_hash_set<int> indices_to_delete;
  auto i = stripped_paths.cbegin();
  auto j = sorted_locations.cbegin();
  while (i != stripped_paths.cend() && j != sorted_locations.cend()) {
    if (IsPrefix(*i, j->first)) {
      indices_to_delete.insert(j->second);
      ++j;
    } else if (*i < j->first) {
      ++i;
    } else {
      ++j;
    }
  }

  // We delete the locations in descending order to avoid invalidating
  // indices.
  std::vector<SourceCodeInfo::Location*> old_locations;
  old_locations.resize(locations->size());
  locations->ExtractSubrange(0, locations->size(), old_locations.data());
  locations->Reserve(old_locations.size() - indices_to_delete.size());
  for (size_t i = 0; i < old_locations.size(); ++i) {
    if (indices_to_delete.contains(i)) {
      delete old_locations[i];
    } else {
      locations->AddAllocated(old_locations[i]);
    }
  }
}

}  // namespace

FileDescriptorProto StripSourceRetentionOptions(const FileDescriptor& file,
                                                bool include_source_code_info) {
  FileDescriptorProto file_proto;
  file.CopyTo(&file_proto);
  if (include_source_code_info) {
    file.CopySourceCodeInfoTo(&file_proto);
  }
  StripSourceRetentionOptions(*file.pool(), file_proto);
  return file_proto;
}

void StripSourceRetentionOptions(const DescriptorPool& pool,
                                 FileDescriptorProto& file_proto) {
  std::vector<std::vector<int>> stripped_paths;
  ConvertToDynamicMessageAndStripOptions(file_proto, pool, &stripped_paths);
  if (file_proto.has_source_code_info()) {
    StripSourceCodeInfo(stripped_paths, *file_proto.mutable_source_code_info());
  }
}

DescriptorProto StripSourceRetentionOptions(const Descriptor& message) {
  DescriptorProto message_proto;
  message.CopyTo(&message_proto);
  ConvertToDynamicMessageAndStripOptions(message_proto,
                                         *message.file()->pool());
  return message_proto;
}

DescriptorProto::ExtensionRange StripSourceRetentionOptions(
    const Descriptor& message, const Descriptor::ExtensionRange& range) {
  DescriptorProto::ExtensionRange range_proto;
  range.CopyTo(&range_proto);
  ConvertToDynamicMessageAndStripOptions(range_proto, *message.file()->pool());
  return range_proto;
}

EnumDescriptorProto StripSourceRetentionOptions(const EnumDescriptor& enm) {
  EnumDescriptorProto enm_proto;
  enm.CopyTo(&enm_proto);
  ConvertToDynamicMessageAndStripOptions(enm_proto, *enm.file()->pool());
  return enm_proto;
}

FieldDescriptorProto StripSourceRetentionOptions(const FieldDescriptor& field) {
  FieldDescriptorProto field_proto;
  field.CopyTo(&field_proto);
  ConvertToDynamicMessageAndStripOptions(field_proto, *field.file()->pool());
  return field_proto;
}

OneofDescriptorProto StripSourceRetentionOptions(const OneofDescriptor& oneof) {
  OneofDescriptorProto oneof_proto;
  oneof.CopyTo(&oneof_proto);
  ConvertToDynamicMessageAndStripOptions(oneof_proto, *oneof.file()->pool());
  return oneof_proto;
}

EnumOptions StripLocalSourceRetentionOptions(const EnumDescriptor& descriptor) {
  return StripLocalOptions(descriptor);
}

EnumValueOptions StripLocalSourceRetentionOptions(
    const EnumValueDescriptor& descriptor) {
  return StripLocalOptions(descriptor);
}

FieldOptions StripLocalSourceRetentionOptions(
    const FieldDescriptor& descriptor) {
  return StripLocalOptions(descriptor);
}

FileOptions StripLocalSourceRetentionOptions(const FileDescriptor& descriptor) {
  return StripLocalOptions(descriptor);
}

MessageOptions StripLocalSourceRetentionOptions(const Descriptor& descriptor) {
  return StripLocalOptions(descriptor);
}

ExtensionRangeOptions StripLocalSourceRetentionOptions(
    const Descriptor& descriptor, const Descriptor::ExtensionRange& range) {
  ExtensionRangeOptions options = range.options();
  ConvertToDynamicMessageAndStripOptions(options, GetPool(descriptor));
  return options;
}

MethodOptions StripLocalSourceRetentionOptions(
    const MethodDescriptor& descriptor) {
  return StripLocalOptions(descriptor);
}

OneofOptions StripLocalSourceRetentionOptions(
    const OneofDescriptor& descriptor) {
  return StripLocalOptions(descriptor);
}

ServiceOptions StripLocalSourceRetentionOptions(
    const ServiceDescriptor& descriptor) {
  return StripLocalOptions(descriptor);
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
