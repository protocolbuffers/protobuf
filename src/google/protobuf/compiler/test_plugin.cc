// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//
// This is a dummy code generator plugin used by
// command_line_interface_unittest.

#include <stdlib.h>

#include <string>

#include "google/protobuf/compiler/mock_code_generator.h"
#include "google/protobuf/compiler/plugin.h"

namespace google {
namespace protobuf {
namespace compiler {

int ProtobufMain(int argc, char* argv[]) {
  MockCodeGenerator generator("test_plugin");
  return PluginMain(argc, argv, &generator);
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google

int main(int argc, char* argv[]) {
#ifdef _MSC_VER
  // Don't print a silly message or stick a modal dialog box in my face,
  // please.
  _set_abort_behavior(0, ~0);
#endif  // !_MSC_VER
  return google::protobuf::compiler::ProtobufMain(argc, argv);
}
