// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifdef _WIN32
#include <fcntl.h>
#else
#include <unistd.h>
#endif

#include "absl/log/absl_check.h"
#include "absl/strings/escaping.h"
#include "google/protobuf/compiler/plugin.pb.h"
#include "google/protobuf/io/io_win32.h"

using google::protobuf::compiler::CodeGeneratorRequest;
using google::protobuf::compiler::CodeGeneratorResponse;

// This fake protoc plugin does nothing but write out the CodeGeneratorRequest
// in base64. This is not very useful except that it gives us a way to make
// assertions in tests about the contents of requests that protoc sends to
// plugins.
int main(int argc, char* argv[]) {

#ifdef _WIN32
  google::protobuf::io::win32::setmode(STDIN_FILENO, _O_BINARY);
  google::protobuf::io::win32::setmode(STDOUT_FILENO, _O_BINARY);
#endif

  CodeGeneratorRequest request;
  ABSL_CHECK(request.ParseFromFileDescriptor(STDIN_FILENO));
  ABSL_CHECK(!request.file_to_generate().empty());
  CodeGeneratorResponse response;
  response.set_supported_features(
      CodeGeneratorResponse::FEATURE_SUPPORTS_EDITIONS);
  response.add_file()->set_name(
      absl::StrCat(request.file_to_generate(0), ".request"));
  response.mutable_file(0)->set_content(
      absl::Base64Escape(request.SerializeAsString()));
  ABSL_CHECK(response.SerializeToFileDescriptor(STDOUT_FILENO));
  return 0;
}
