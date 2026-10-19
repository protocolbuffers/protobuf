// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/cpp/namespace_printer.h"

#include <string>
#include <utility>
#include <vector>

#include "absl/log/die_if_null.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

NamespacePrinter::NamespacePrinter(
    google::protobuf::io::Printer* const p, std::vector<std::string> namespace_components)
    : p_(ABSL_DIE_IF_NULL(p)),
      namespace_components_(std::move(namespace_components)) {
  // Open the namespace.
  for (const std::string& ns : namespace_components_) {
    p_->Print(absl::Substitute("namespace $0 {\n", ns));
  }
  p_->Print("\n");
}

NamespacePrinter::~NamespacePrinter() {
  // Close the namespace.
  for (auto it = namespace_components_.rbegin();
       it != namespace_components_.rend(); ++it) {
    p_->Print(absl::Substitute("}  // namespace $0\n", *it));
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
