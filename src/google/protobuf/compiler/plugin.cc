// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)

#include "google/protobuf/compiler/plugin.h"

#include <iostream>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#else
#include <unistd.h>
#endif

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/plugin.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/io_win32.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"


namespace google {
namespace protobuf {
namespace compiler {

#if defined(_WIN32)
// DO NOT include <io.h>, instead create functions in io_win32.{h,cc} and import
// them like we do below.
using google::protobuf::io::win32::setmode;
#endif

class GeneratorResponseContext : public GeneratorContext {
 public:
  GeneratorResponseContext(
      const Version& compiler_version, CodeGeneratorResponse* response,
      const std::vector<const FileDescriptor*>& parsed_files)
      : compiler_version_(compiler_version),
        response_(response),
        parsed_files_(parsed_files) {}
  ~GeneratorResponseContext() override {}

  // implements GeneratorContext --------------------------------------

  io::ZeroCopyOutputStream* Open(const std::string& filename) override {
    CodeGeneratorResponse::File* file = response_->add_file();
    file->set_name(filename);
    return new io::StringOutputStream(file->mutable_content());
  }

  io::ZeroCopyOutputStream* OpenForInsert(
      const std::string& filename,
      const std::string& insertion_point) override {
    CodeGeneratorResponse::File* file = response_->add_file();
    file->set_name(filename);
    file->set_insertion_point(insertion_point);
    return new io::StringOutputStream(file->mutable_content());
  }

  io::ZeroCopyOutputStream* OpenForInsertWithGeneratedCodeInfo(
      const std::string& filename, const std::string& insertion_point,
      const google::protobuf::GeneratedCodeInfo& info) override {
    CodeGeneratorResponse::File* file = response_->add_file();
    file->set_name(filename);
    file->set_insertion_point(insertion_point);
    *file->mutable_generated_code_info() = info;
    return new io::StringOutputStream(file->mutable_content());
  }

  void ListParsedFiles(std::vector<const FileDescriptor*>* output) override {
    *output = parsed_files_;
  }

  void GetCompilerVersion(Version* version) const override {
    *version = compiler_version_;
  }

 private:
  Version compiler_version_;
  CodeGeneratorResponse* response_;
  const std::vector<const FileDescriptor*>& parsed_files_;
};

bool GenerateCode(const CodeGeneratorRequest& request,
                  const CodeGenerator& generator,
                  CodeGeneratorResponse* response, std::string* error_msg) {
  DescriptorPool pool;

  // Initialize feature set default mapping.
  absl::StatusOr<FeatureSetDefaults> defaults =
      generator.BuildFeatureSetDefaults();
  if (!defaults.ok()) {
    *error_msg = absl::StrCat("error generating feature defaults: ",
                              defaults.status().message());
    return false;
  }
  absl::Status status = pool.SetFeatureSetDefaults(std::move(defaults).value());
  ABSL_CHECK(status.ok()) << status.message();

  for (int i = 0; i < request.proto_file_size(); i++) {
    const FileDescriptor* file = pool.BuildFile(request.proto_file(i));
    if (file == nullptr) {
      // BuildFile() already wrote an error message.
      return false;
    }
  }

  std::vector<const FileDescriptor*> parsed_files;
  for (int i = 0; i < request.file_to_generate_size(); i++) {
    parsed_files.push_back(pool.FindFileByName(request.file_to_generate(i)));
    if (parsed_files.back() == nullptr) {
      *error_msg = absl::StrCat(
          "protoc asked plugin to generate a file but "
          "did not provide a descriptor for the file: ",
          request.file_to_generate(i));
      return false;
    }
  }

  GeneratorResponseContext context(request.compiler_version(), response,
                                   parsed_files);


  std::string error;
  bool succeeded = generator.GenerateAll(parsed_files, request.parameter(),
                                         &context, &error);

  response->set_supported_features(generator.GetSupportedFeatures());
  response->set_minimum_edition(
      static_cast<int>(generator.GetMinimumEdition()));
  response->set_maximum_edition(
      static_cast<int>(generator.GetMaximumEdition()));

  if (!succeeded && error.empty()) {
    error =
        "Code generator returned false but provided no error "
        "description.";
  }
  if (!error.empty()) {
    response->set_error(error);
  }

  return true;
}

int PluginMain(int argc, char* argv[], const CodeGenerator* generator) {

  if (argc > 1) {
    std::cerr << argv[0] << ": Unknown option: " << argv[1] << std::endl;
    return 1;
  }

#ifdef _WIN32
  setmode(STDIN_FILENO, _O_BINARY);
  setmode(STDOUT_FILENO, _O_BINARY);
#endif

  CodeGeneratorRequest request;
  if (!request.ParseFromFileDescriptor(STDIN_FILENO)) {
    std::cerr << argv[0] << ": protoc sent unparseable request to plugin."
              << std::endl;
    return 1;
  }


  std::string error_msg;
  CodeGeneratorResponse response;

  if (GenerateCode(request, *generator, &response, &error_msg)) {
    if (!response.SerializeToFileDescriptor(STDOUT_FILENO)) {
      std::cerr << argv[0] << ": Error writing to stdout." << std::endl;
      return 1;
    }
  } else {
    if (!error_msg.empty()) {
      std::cerr << argv[0] << ": " << error_msg << std::endl;
    }
    return 1;
  }

  return 0;
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
