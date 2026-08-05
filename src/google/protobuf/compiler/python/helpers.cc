// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/python/helpers.h"

#include <algorithm>
#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace python {

// Returns the Python module name expected for a given .proto filename.
std::string ModuleName(absl::string_view filename) {
  std::string basename = StripProto(filename);
  absl::StrReplaceAll({{"-", "_"}, {"/", "."}}, &basename);
  return absl::StrCat(basename, "_pb2");
}

// Returns whether a module name can be safely emitted into generated Python
// import statements without introducing Python syntax.
bool IsSafePythonModuleName(absl::string_view module_name) {
  for (absl::string_view component : absl::StrSplit(module_name, '.')) {
    if (component.empty()) {
      return false;
    }

    const unsigned char first =
        static_cast<unsigned char>(component.front());
    if (!(absl::ascii_isalpha(first) || first == '_')) {
      return false;
    }

    for (char character : component.substr(1)) {
      const unsigned char value = static_cast<unsigned char>(character);
      if (!(absl::ascii_isalnum(value) || value == '_')) {
        return false;
      }
    }
  }

  return true;
}

namespace {

bool ValidateModuleAndPublicDependencies(
    const FileDescriptor* file,
    absl::flat_hash_set<const FileDescriptor*>* visited,
    std::string* error) {
  if (!visited->insert(file).second) {
    return true;
  }

  const std::string module_name = ModuleName(file->name());
  if (!IsSafePythonModuleName(module_name)) {
    *error = absl::StrCat(
        file->name(),
        ": Cannot generate Python code because the file name produces an "
        "invalid Python module name: ",
        module_name);
    return false;
  }

  for (int i = 0; i < file->public_dependency_count(); ++i) {
    if (!ValidateModuleAndPublicDependencies(
            file->public_dependency(i), visited, error)) {
      return false;
    }
  }

  return true;
}

}  // namespace

bool ValidatePythonModuleNames(const FileDescriptor* file, std::string* error) {
  absl::flat_hash_set<const FileDescriptor*> visited;

  if (!ValidateModuleAndPublicDependencies(file, &visited, error)) {
    return false;
  }

  for (int i = 0; i < file->dependency_count(); ++i) {
    if (!ValidateModuleAndPublicDependencies(
            file->dependency(i), &visited, error)) {
      return false;
    }
  }

  return true;
}

std::string StrippedModuleName(absl::string_view filename) {
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

bool ContainsPythonKeyword(absl::string_view module_name) {
  std::vector<absl::string_view> tokens = absl::StrSplit(module_name, '.');
  for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
    if (std::find(kKeywords, kKeywordsEnd, tokens[i]) != kKeywordsEnd) {
      return true;
    }
  }
  return false;
}

bool IsPythonKeyword(absl::string_view name) {
  return (std::find(kKeywords, kKeywordsEnd, name) != kKeywordsEnd);
}

std::string ResolveKeyword(absl::string_view name) {
  if (IsPythonKeyword(name)) {
    return absl::StrCat("globals()['", name, "']");
  }
  return std::string(name);
}

std::string GetFileName(const FileDescriptor* file_des,
                        absl::string_view suffix) {
  std::string module_name = ModuleName(file_des->name());
  std::string filename = module_name;
  absl::StrReplaceAll({{".", "/"}}, &filename);
  absl::StrAppend(&filename, suffix);
  return filename;
}

bool HasGenericServices(const FileDescriptor* file) {
  return file->service_count() > 0 && file->options().py_generic_services();
}

std::string GeneratedCodeToBase64(const GeneratedCodeInfo& annotations) {
  std::string result;
  result = absl::Base64Escape(annotations.SerializeAsString());
  return result;
}

template <typename DescriptorT>
std::string NamePrefixedWithNestedTypes(const DescriptorT& descriptor,
                                        absl::string_view separator) {
  std::string name = std::string(descriptor.name());
  const Descriptor* parent = descriptor.containing_type();
  if (parent != nullptr) {
    std::string prefix = NamePrefixedWithNestedTypes(*parent, separator);
    if (separator == "." && IsPythonKeyword(name)) {
      return absl::StrCat("getattr(", prefix, ", '", name, "')");
    } else {
      return absl::StrCat(prefix, separator, name);
    }
  }
  if (separator == ".") {
    name = ResolveKeyword(name);
  }
  return name;
}

template std::string NamePrefixedWithNestedTypes<Descriptor>(
    const Descriptor& descriptor, absl::string_view separator);
template std::string NamePrefixedWithNestedTypes<EnumDescriptor>(
    const EnumDescriptor& descriptor, absl::string_view separator);

}  // namespace python
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
