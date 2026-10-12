#include "google/protobuf/compiler/python/names.h"

#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/compiler/code_generator.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace python {

// Returns the Python module name expected for a given .proto filename.
std::string ModuleName(const FileDescriptor* file) {
  std::string basename = StripProto(file->name());
  absl::StrReplaceAll({{"-", "_"}, {"/", "."}}, &basename);
  return absl::StrCat(basename, "_pb2");
}

std::string StrippedModuleName(const FileDescriptor* file) {
  std::string module_name = ModuleName(file);
  return module_name;
}

// Returns the alias we assign to the module of the given .proto filename
// when importing. See testPackageInitializationImport in
// third_party/py/google/protobuf/internal/reflection_test.py
// to see why we need the alias.
std::string ModuleAlias(const FileDescriptor* file) {
  std::string module_name = ModuleName(file);
  // We can't have dots in the module name, so we replace each with _dot_.
  // But that could lead to a collision between a.b and a_dot_b, so we also
  // duplicate each underscore.
  absl::StrReplaceAll({{"_", "__"}}, &module_name);
  absl::StrReplaceAll({{".", "_dot_"}}, &module_name);
  return module_name;
}

}  // namespace python
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
