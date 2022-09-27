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

#include "google/protobuf/compiler/csharp/names.h"

#include <string>

#include "absl/strings/str_replace.h"
#include "google/protobuf/compiler/csharp/names.h"
#include "google/protobuf/descriptor.pb.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

namespace {

std::string StripDotProto(const std::string& proto_file) {
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

std::string ToCSharpName(const std::string& name, const FileDescriptor* file) {
  std::string result = GetFileNamespace(file);
  if (!result.empty()) {
    result += '.';
  }
  std::string classname;
  if (file->package().empty()) {
    classname = name;
  } else {
    // Strip the proto package from full_name since we've replaced it with
    // the C# namespace.
    classname = name.substr(file->package().size() + 1);
  }
  result += absl::StrReplaceAll(classname, {{".", ".Types."}});
  return "global::" + result;
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
  return GetFileNameBase(descriptor) + "Reflection";
}

std::string GetReflectionClassName(const FileDescriptor* descriptor) {
  std::string result = GetFileNamespace(descriptor);
  if (!result.empty()) {
    result += '.';
  }
  result += GetReflectionClassUnqualifiedName(descriptor);
  return "global::" + result;
}

std::string GetExtensionClassUnqualifiedName(const FileDescriptor* descriptor) {
  // TODO: Detect collisions with existing messages,
  // and append an underscore if necessary.
  return GetFileNameBase(descriptor) + "Extensions";
}

std::string GetOutputFile(const FileDescriptor* descriptor,
                          const std::string file_extension,
                          const bool generate_directories,
                          const std::string base_namespace,
                          std::string* error) {
  std::string relative_filename = GetFileNameBase(descriptor) + file_extension;
  if (!generate_directories) {
    return relative_filename;
  }
  std::string ns = GetFileNamespace(descriptor);
  std::string namespace_suffix = ns;
  if (!base_namespace.empty()) {
    // Check that the base_namespace is either equal to or a leading part of
    // the file namespace. This isn't just a simple prefix; "Foo.B" shouldn't
    // be regarded as a prefix of "Foo.Bar". The simplest option is to add "."
    // to both.
    std::string extended_ns = ns + ".";
    if (extended_ns.find(base_namespace + ".") != 0) {
      *error = "Namespace " + ns + " is not a prefix namespace of base namespace " + base_namespace;
      return ""; // This will be ignored, because we've set an error.
    }
    namespace_suffix = ns.substr(base_namespace.length());
    if (namespace_suffix.find('.') == 0) {
      namespace_suffix = namespace_suffix.substr(1);
    }
  }

  std::string namespace_dir =
      absl::StrReplaceAll(namespace_suffix, {{".", "/"}});
  if (!namespace_dir.empty()) {
    namespace_dir += "/";
  }
  return namespace_dir + relative_filename;
}

std::string UnderscoresToPascalCase(const std::string& input) {
  return UnderscoresToCamelCase(input, true);
}

// TODO(jtattermusch): can we reuse a utility function?
std::string UnderscoresToCamelCase(const std::string& input,
                                   bool cap_next_letter,
                                   bool preserve_period) {
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
