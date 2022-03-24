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

#include <google/protobuf/compiler/python/helpers.h>

#include <algorithm>

#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace python {

// Returns the Python module name expected for a given .proto filename.
std::string ModuleName(const std::string& filename) {
  std::string basename = StripProto(filename);
  ReplaceCharacters(&basename, "-", '_');
  ReplaceCharacters(&basename, "/", '.');
  return basename + "_pb2";
}

std::string StrippedModuleName(const std::string& filename) {
  std::string module_name = ModuleName(filename);
  return module_name;
}

// Keywords reserved by the Python language.
const char* const kKeywords[] = {
    "False",  "None",     "True",  "and",    "as",       "assert",
    "async",  "await",    "break", "class",  "continue", "def",
    "del",    "elif",     "else",  "except", "finally",  "for",
    "from",   "global",   "if",    "import", "in",       "is",
    "lambda", "nonlocal", "not",   "or",     "pass",     "raise",
    "return", "try",      "while", "with",   "yield",
};
const char* const* kKeywordsEnd =
    kKeywords + (sizeof(kKeywords) / sizeof(kKeywords[0]));

bool ContainsPythonKeyword(const std::string& module_name) {
  std::vector<std::string> tokens = Split(module_name, ".");
  for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
    if (std::find(kKeywords, kKeywordsEnd, tokens[i]) != kKeywordsEnd) {
      return true;
    }
  }
  return false;
}

bool IsPythonKeyword(const std::string& name) {
  return (std::find(kKeywords, kKeywordsEnd, name) != kKeywordsEnd);
}

std::string ResolveKeyword(const std::string& name) {
  if (IsPythonKeyword(name)) {
    return "globals()['" + name + "']";
  }
  return name;
}

std::string GetFileName(const FileDescriptor* file_des,
                        const std::string& suffix) {
  std::string module_name = ModuleName(file_des->name());
  std::string filename = module_name;
  ReplaceCharacters(&filename, ".", '/');
  filename += suffix;
  return filename;
}

bool HasGenericServices(const FileDescriptor* file) {
  return file->service_count() > 0 && file->options().py_generic_services();
}

template <typename DescriptorT>
std::string NamePrefixedWithNestedTypes(const DescriptorT& descriptor,
                                        const std::string& separator) {
  std::string name = descriptor.name();
  const Descriptor* parent = descriptor.containing_type();
  if (parent != nullptr) {
    std::string prefix = NamePrefixedWithNestedTypes(*parent, separator);
    if (separator == "." && IsPythonKeyword(name)) {
      return "getattr(" + prefix + ", '" + name + "')";
    } else {
      return prefix + separator + name;
    }
  }
  if (separator == ".") {
    name = ResolveKeyword(name);
  }
  return name;
}

template std::string NamePrefixedWithNestedTypes<Descriptor>(
    const Descriptor& descriptor, const std::string& separator);
template std::string NamePrefixedWithNestedTypes<EnumDescriptor>(
    const EnumDescriptor& descriptor, const std::string& separator);

}  // namespace python
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
