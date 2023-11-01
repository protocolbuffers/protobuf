// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/java/message_serialization.h"

#include "absl/strings/str_cat.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

void GenerateSerializeExtensionRange(io::Printer* printer,
                                     const Descriptor::ExtensionRange* range) {
  printer->Print("extensionWriter.writeUntil($end$, output);\n", "end",
                 absl::StrCat(range->end_number()));
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
