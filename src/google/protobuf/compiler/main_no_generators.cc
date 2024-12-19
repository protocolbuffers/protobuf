// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/command_line_interface.h"

#include "absl/log/initialize.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {

// This is a version of protoc that has no built-in code generators.
// See go/protobuf-toolchain-protoc
int ProtocMain(int argc, char* argv[]) {
  absl::InitializeLog();

  CommandLineInterface cli;
  cli.AllowPlugins("protoc-");

  return cli.Run(argc, argv);
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google

int main(int argc, char* argv[]) {
  return google::protobuf::compiler::ProtocMain(argc, argv);
}

#include "google/protobuf/port_undef.inc"
