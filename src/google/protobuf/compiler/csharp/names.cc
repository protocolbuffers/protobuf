// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/csharp/names.h"

#include <string>

#include "absl/strings/match.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/compiler/csharp/names.h"
#include "google/protobuf/descriptor.pb.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

namespace {

absl::string_view StripDotProto(absl::string_view proto_file) {
  int lastindex = proto_file.find_last_of('.');
  return proto_file.substr(0, lastindex);
}

// Returns the Pascal-cased last part of the proto file. For example,
// input of "google/protobuf/foo_bar.proto" would result in "FooBar".
std::string GetFileNameBase(const FileDescriptor* descriptor) {
    std::string proto_file = descriptor->name();
    int lastslash = proto_file.find_last_of('/');
    std::string base = proto_file.substr(lastslash + 1);
    return UnderscoresToPascalCase(StripDotProto(base));
}

std::string ToCSharpName(absl::string_view name, const FileDescriptor* file) {
    std::string result = GetFileNamespace(file);
    if (!result.empty()) {
      result += '.';
    }
    absl::string_view classname;
    if (file->package().empty()) {
      classname = name;
    } else {
      // Strip the proto package from full_name since we've replaced it with
      // the C# namespace.
      classname = name.substr(file->package().size() + 1);
    }
    return absl::StrCat("global::", result,
                        absl::StrReplaceAll(classname, {{".", ".Types."}}));
}

}  // namespace

std::string GetFileNamespace(const FileDescriptor* descriptor) {
  if (descriptor->options().has_csharp_namespace()) {
    return descriptor->options().csharp_namespace();
  }
  return UnderscoresToCamelCase(descriptor->package(), true, true);
}

std::string GetClassName(const Descriptor* descriptor) {
  return ToCSharpName(descriptor->full_name(), descriptor->file());
}

std::string GetClassName(const EnumDescriptor* descriptor) {
  return ToCSharpName(descriptor->full_name(), descriptor->file());
}

std::string GetReflectionClassUnqualifiedName(const FileDescriptor* descriptor) {
  // TODO: Detect collisions with existing messages,
  // and append an underscore if necessary.
  return absl::StrCat(GetFileNameBase(descriptor), "Reflection");
}

std::string GetReflectionClassName(const FileDescriptor* descriptor) {
  std::string result = GetFileNamespace(descriptor);
  if (!result.empty()) {
    result += '.';
  }
  return absl::StrCat("global::", result,
                      GetReflectionClassUnqualifiedName(descriptor));
}

std::string GetExtensionClassUnqualifiedName(const FileDescriptor* descriptor) {
  // TODO: Detect collisions with existing messages,
  // and append an underscore if necessary.
  return absl::StrCat(GetFileNameBase(descriptor), "Extensions");
}

std::string GetOutputFile(const FileDescriptor* descriptor,
                          absl::string_view file_extension,
                          bool generate_directories,
                          absl::string_view base_namespace,
                          std::string* error) {
  std::string relative_filename =
      absl::StrCat(GetFileNameBase(descriptor), file_extension);
  if (!generate_directories) {
    return relative_filename;
  }
  std::string ns = GetFileNamespace(descriptor);
  absl::string_view namespace_suffix = ns;
  if (!base_namespace.empty()) {
    // Check that the base_namespace is either equal to or a leading part of
    // the file namespace. This isn't just a simple prefix; "Foo.B" shouldn't
    // be regarded as a prefix of "Foo.Bar". The simplest option is to add "."
    // to both.
    if (!absl::ConsumePrefix(&namespace_suffix, base_namespace) ||
        (!namespace_suffix.empty() &&
         !absl::ConsumePrefix(&namespace_suffix, "."))) {
      *error = absl::StrCat("Namespace ", ns,
                            " is not a prefix namespace of base namespace ",
                            base_namespace);
      return "";  // This will be ignored, because we've set an error.
    }
  }

  return absl::StrCat(absl::StrReplaceAll(namespace_suffix, {{".", "/"}}),
                      namespace_suffix.empty() ? "" : "/", relative_filename);
}

std::string UnderscoresToPascalCase(absl::string_view input) {
  return UnderscoresToCamelCase(input, true);
}

// TODO: can we reuse a utility function?
std::string UnderscoresToCamelCase(absl::string_view input,
                                   bool cap_next_letter, bool preserve_period) {
  std::string result;

  // Note:  I distrust ctype.h due to locales.
  for (int i = 0; i < input.size(); i++) {
    if ('a' <= input[i] && input[i] <= 'z') {
      if (cap_next_letter) {
        result += input[i] + ('A' - 'a');
      } else {
        result += input[i];
      }
      cap_next_letter = false;
    } else if ('A' <= input[i] && input[i] <= 'Z') {
      if (i == 0 && !cap_next_letter) {
        // Force first letter to lower-case unless explicitly told to
        // capitalize it.
        result += input[i] + ('a' - 'A');
      } else {
        // Capital letters after the first are left as-is.
        result += input[i];
      }
      cap_next_letter = false;
    } else if ('0' <= input[i] && input[i] <= '9') {
      result += input[i];
      cap_next_letter = true;
    } else {
      cap_next_letter = true;
      if (input[i] == '.' && preserve_period) {
        result += '.';
      }
    }
  }
  // Add a trailing "_" if the name should be altered.
  if (input.size() > 0 && input[input.size() - 1] == '#') {
    result += '_';
  }

  // https://github.com/protocolbuffers/protobuf/issues/8101
  // To avoid generating invalid identifiers - if the input string
  // starts with _<digit> (or multiple underscores then digit) then
  // we need to preserve the underscore as an identifier cannot start
  // with a digit.
  // This check is being done after the loop rather than before
  // to handle the case where there are multiple underscores before the
  // first digit. We let them all be consumed so we can see if we would
  // start with a digit.
  // Note: not preserving leading underscores for all otherwise valid identifiers
  // so as to not break anything that relies on the existing behaviour
  if (result.size() > 0 && ('0' <= result[0] && result[0] <= '9')
      && input.size() > 0 && input[0] == '_')
  {
      result.insert(0, 1, '_');
  }
  return result;
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
