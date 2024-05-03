// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/code_generator.h"

#include <utility>

#include "absl/log/absl_log.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/compiler/plugin.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/feature_resolver.h"

namespace google {
namespace protobuf {
namespace compiler {

CodeGenerator::~CodeGenerator() {}

bool CodeGenerator::GenerateAll(const std::vector<const FileDescriptor*>& files,
                                const std::string& parameter,
                                GeneratorContext* generator_context,
                                std::string* error) const {
  // Default implementation is just to call the per file method, and prefix any
  // error string with the file to provide context.
  bool succeeded = true;
  for (int i = 0; i < files.size(); i++) {
    const FileDescriptor* file = files[i];
    succeeded = Generate(file, parameter, generator_context, error);
    if (!succeeded && error && error->empty()) {
      *error =
          "Code generator returned false but provided no error "
          "description.";
    }
    if (error && !error->empty()) {
      *error = absl::StrCat(file->name(), ": ", *error);
      break;
    }
    if (!succeeded) {
      break;
    }
  }
  return succeeded;
}

absl::StatusOr<FeatureSetDefaults> CodeGenerator::BuildFeatureSetDefaults()
    const {
  return FeatureResolver::CompileDefaults(
      FeatureSet::descriptor(), GetFeatureExtensions(), GetMinimumEdition(),
      GetMaximumEdition());
}

GeneratorContext::~GeneratorContext() {}

io::ZeroCopyOutputStream* GeneratorContext::OpenForAppend(
    const std::string& filename) {
  return nullptr;
}

io::ZeroCopyOutputStream* GeneratorContext::OpenForInsert(
    const std::string& filename, const std::string& insertion_point) {
  ABSL_LOG(FATAL) << "This GeneratorContext does not support insertion.";
  return nullptr;  // make compiler happy
}

io::ZeroCopyOutputStream* GeneratorContext::OpenForInsertWithGeneratedCodeInfo(
    const std::string& filename, const std::string& insertion_point,
    const google::protobuf::GeneratedCodeInfo& /*info*/) {
  return OpenForInsert(filename, insertion_point);
}

void GeneratorContext::ListParsedFiles(
    std::vector<const FileDescriptor*>* output) {
  ABSL_LOG(FATAL) << "This GeneratorContext does not support ListParsedFiles";
}

void GeneratorContext::GetCompilerVersion(Version* version) const {
  version->set_major(GOOGLE_PROTOBUF_VERSION / 1000000);
  version->set_minor(GOOGLE_PROTOBUF_VERSION / 1000 % 1000);
  version->set_patch(GOOGLE_PROTOBUF_VERSION % 1000);
  version->set_suffix(GOOGLE_PROTOBUF_VERSION_SUFFIX);
}

// Parses a set of comma-delimited name/value pairs.
void ParseGeneratorParameter(
    absl::string_view text,
    std::vector<std::pair<std::string, std::string> >* output) {
  std::vector<absl::string_view> parts =
      absl::StrSplit(text, ',', absl::SkipEmpty());

  for (absl::string_view part : parts) {
    auto equals_pos = part.find_first_of('=');
    if (equals_pos == absl::string_view::npos) {
      output->emplace_back(part, "");
    } else {
      output->emplace_back(part.substr(0, equals_pos),
                           part.substr(equals_pos + 1));
    }
  }
}

// Strips ".proto" or ".protodevel" from the end of a filename.
std::string StripProto(absl::string_view filename) {
  if (absl::EndsWith(filename, ".protodevel")) {
    return std::string(absl::StripSuffix(filename, ".protodevel"));
  } else {
    return std::string(absl::StripSuffix(filename, ".proto"));
  }
}

bool IsKnownFeatureProto(absl::string_view filename) {
  if (filename == "google/protobuf/cpp_features.proto" ||
      filename == "google/protobuf/java_features.proto") {
    return true;
  }
  return false;
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
