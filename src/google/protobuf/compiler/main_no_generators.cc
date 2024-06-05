// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/command_line_interface.h"

#include "absl/log/initialize.h"

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {

// This is a version of protoc that has no built-in code generators.
// See go/protobuf-toolchain-protoc
int ProtocMain(int argc, char* argv[]) {
  ABSL_LOG(ERROR) << "InitializeLog";
  absl::InitializeLog();
  ABSL_LOG(ERROR) << "InitializeLog done";

  CommandLineInterface cli;
  cli.AllowPlugins("protoc-");

  ABSL_LOG(ERROR) << "Running CLI";
  return cli.Run(argc, argv);
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google

int main(int argc, char* argv[]) {
  ABSL_LOG(ERROR) << "Starting protoc_minimal";
  int ret = google::protobuf::compiler::ProtocMain(argc, argv);
  ABSL_CHECK(false);
  return ret;
}
