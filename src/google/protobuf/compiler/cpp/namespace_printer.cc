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
  for (const std::string& ns : namespace_components_) {
    p_->Print(absl::Substitute("}  // namespace $0\n", ns));
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
