// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/java/name_resolver.h"

#include <cstddef>
#include <string>

#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/java/java_features.pb.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/java/generator.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/names.h"
#include "google/protobuf/descriptor.h"


// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

namespace {
// A suffix that will be appended to the file's outer class name if the name
// conflicts with some other types defined in the file.
const char* kOuterClassNameSuffix = "OuterClass";

// A suffix inserted into the outer class name of an `import public` shim (a
// file that declares no types of its own and only forwards to other files) when
// its default outer class name would collide with the outer class of a file it
// publicly imports. This is what happens when a .proto file is moved and the
// old path is kept as a forwarding shim with the same basename: without
// disambiguation both files would generate the same Java class.
const char* kPublicImportShimSuffix = "Shim";

inline bool UseOldFileClassNameDefault(const FileDescriptor* file) {
  return JavaGenerator::GetResolvedSourceFeatureExtension(*file, pb::java)
      .use_old_outer_classname_default();
}

// foo/bar_baz.proto -> BarBaz
std::string FileBaseClassName(const FileDescriptor* file) {
  absl::string_view basename = file->name();
  std::string::size_type last_slash = basename.find_last_of('/');
  if (last_slash != std::string::npos) {
    basename = basename.substr(last_slash + 1);
  }
  return UnderscoresToCamelCase(StripProto(basename), true);
}

// Returns true if `file` is a pure `import public` shim: it publicly imports
// other files and declares no messages, enums, services or extensions itself.
bool IsPublicImportShim(const FileDescriptor* file) {
  return file->public_dependency_count() > 0 &&
         file->message_type_count() == 0 && file->enum_type_count() == 0 &&
         file->service_count() == 0 && file->extension_count() == 0;
}

// Adds the transitive closure of `file`'s public dependencies to `out`.
void CollectPublicDependencies(
    const FileDescriptor* file,
    absl::flat_hash_set<const FileDescriptor*>* out) {
  for (int i = 0; i < file->public_dependency_count(); ++i) {
    const FileDescriptor* dep = file->public_dependency(i);
    if (out->insert(dep).second) CollectPublicDependencies(dep, out);
  }
}

// Strip package name from a descriptor's full name.
// For example:
//   Full name   : foo.Bar.Baz
//   Package name: foo
//   After strip : Bar.Baz
absl::string_view StripPackageName(absl::string_view full_name,
                                   const FileDescriptor* file) {
  if (file->package().empty()) {
    return full_name;
  } else {
    // Strip package name
    return full_name.substr(file->package().size() + 1);
  }
}

// Get the name of a message's Java class without package name prefix.
std::string ClassNameWithoutPackage(const Descriptor* descriptor,
                                    bool immutable) {
  return std::string(
      StripPackageName(descriptor->full_name(), descriptor->file()));
}

std::string ClassNameWithoutPackageKotlin(const Descriptor* descriptor) {
  std::string result = std::string(descriptor->name());
  const Descriptor* temp = descriptor->containing_type();

  while (temp) {
    result = absl::StrCat(temp->name(), "Kt.", result);
    temp = temp->containing_type();
  }
  return result;
}

// Get the name of an enum's Java class without package name prefix.
std::string ClassNameWithoutPackage(const EnumDescriptor* descriptor,
                                    bool immutable) {
  // Doesn't append "Mutable" for enum type's name.
  const Descriptor* message_descriptor = descriptor->containing_type();
  if (message_descriptor == nullptr) {
    return std::string(descriptor->name());
  } else {
    return absl::StrCat(ClassNameWithoutPackage(message_descriptor, immutable),
                        ".", descriptor->name());
  }
}

// Get the name of a service's Java class without package name prefix.
std::string ClassNameWithoutPackage(const ServiceDescriptor* descriptor,
                                    bool immutable) {
  absl::string_view full_name =
      StripPackageName(descriptor->full_name(), descriptor->file());
  // We don't allow nested service definitions.
  ABSL_CHECK(!absl::StrContains(full_name, '.'));
  return std::string(full_name);
}

// Return true if a and b are equals (case insensitive).
NameEquality CheckNameEquality(absl::string_view a, absl::string_view b) {
  if (absl::AsciiStrToUpper(a) == absl::AsciiStrToUpper(b)) {
    if (a == b) {
      return NameEquality::EXACT_EQUAL;
    }
    return NameEquality::EQUAL_IGNORE_CASE;
  }
  return NameEquality::NO_MATCH;
}

// Check whether a given message or its nested types has the given class name.
bool MessageHasConflictingClassName(const Descriptor* message,
                                    absl::string_view classname,
                                    NameEquality equality_mode) {
  if (CheckNameEquality(message->name(), classname) == equality_mode) {
    return true;
  }
  for (int i = 0; i < message->nested_type_count(); ++i) {
    if (MessageHasConflictingClassName(message->nested_type(i), classname,
                                       equality_mode)) {
      return true;
    }
  }
  for (int i = 0; i < message->enum_type_count(); ++i) {
    if (CheckNameEquality(message->enum_type(i)->name(), classname) ==
        equality_mode) {
      return true;
    }
  }
  return false;
}

}  // namespace

class MemoizeProjection {
 public:
  template <typename Desc, typename Func>
  const auto& operator()(const Desc* descriptor, Func func) {
    return DescriptorPool::MemoizeProjection(descriptor, func);
  }
};

bool ComputeNeedsOuterClassSuffix(const FileDescriptor* file) {
  if (!UseOldFileClassNameDefault(file)) return false;
  return ClassNameResolver::HasConflictingClassName(
      file, ClassNameResolver::GetFileDefaultImmutableClassName(file),
      NameEquality::EXACT_EQUAL);
}

bool NeedsOuterClassSuffix(const FileDescriptor* file) {
  return MemoizeProjection()(file, ComputeNeedsOuterClassSuffix);
}

// Computes the outer class name of an `import public` shim, making sure it
// does not collide with the outer class of any file reachable through its
// public imports that lives in the same Java package. Only the shim is renamed
// (never the file it forwards to), so consumers that already reference the
// moved file's class are unaffected.
std::string ComputePublicImportShimClassName(const FileDescriptor* file) {
  const std::string java_package = FileJavaPackage(file);
  absl::flat_hash_set<const FileDescriptor*> public_deps;
  CollectPublicDependencies(file, &public_deps);
  absl::flat_hash_set<std::string> taken;
  for (const FileDescriptor* dep : public_deps) {
    if (FileJavaPackage(dep) == java_package) {
      taken.insert(ClassNameResolver().GetFileImmutableClassName(dep));
    }
  }

  // Insert the suffix before the "Proto" suffix of the new naming scheme:
  // codes.proto -> CodesShim (old scheme) / CodesShimProto (new scheme).
  const absl::string_view scheme_suffix =
      UseOldFileClassNameDefault(file) ? "" : "Proto";
  std::string base = FileBaseClassName(file);
  std::string class_name = absl::StrCat(base, scheme_suffix);
  while (taken.contains(class_name)) {
    absl::StrAppend(&base, kPublicImportShimSuffix);
    class_name = absl::StrCat(base, scheme_suffix);
  }
  return class_name;
}

const std::string& PublicImportShimClassName(const FileDescriptor* file) {
  return MemoizeProjection()(file, ComputePublicImportShimClassName);
}

std::string ClassNameResolver::GetFileDefaultImmutableClassName(
    const FileDescriptor* file) {
  std::string ret = FileBaseClassName(file);
  return UseOldFileClassNameDefault(file) ? ret : ret + "Proto";
}

std::string ClassNameResolver::GetFileImmutableClassName(
    const FileDescriptor* file) {
  if (file->options().has_java_outer_classname()) {
    return file->options().java_outer_classname();
  }

  // A pure `import public` shim has no types of its own, so it can never need
  // the deprecated "OuterClass" suffix below; it may however collide with the
  // outer class of the file it forwards to.
  if (IsPublicImportShim(file)) {
    return PublicImportShimClassName(file);
  }

  std::string class_name = GetFileDefaultImmutableClassName(file);

  // This disambiguation logic is deprecated and only enabled when using
  // the old default scheme.
  if (NeedsOuterClassSuffix(file)) {
    absl::StrAppend(&class_name, kOuterClassNameSuffix);
  }

  return class_name;
}

std::string ClassNameResolver::GetFileClassName(const FileDescriptor* file,
                                                bool immutable) {
  return GetFileClassName(file, immutable, false);
}

std::string ClassNameResolver::GetFileClassName(const FileDescriptor* file,
                                                bool immutable, bool kotlin) {
  if (kotlin) {
    return absl::StrCat(GetFileImmutableClassName(file), "Kt");
  } else if (immutable) {
    return GetFileImmutableClassName(file);
  } else {
    return absl::StrCat("Mutable", GetFileImmutableClassName(file));
  }
}

// Check whether there is any type defined in the proto file that has
// the given class name.
bool ClassNameResolver::HasConflictingClassName(const FileDescriptor* file,
                                                absl::string_view classname,
                                                NameEquality equality_mode) {
  for (int i = 0; i < file->enum_type_count(); i++) {
    if (CheckNameEquality(file->enum_type(i)->name(), classname) ==
        equality_mode) {
      return true;
    }
  }
  for (int i = 0; i < file->service_count(); i++) {
    if (CheckNameEquality(file->service(i)->name(), classname) ==
        equality_mode) {
      return true;
    }
  }
  for (int i = 0; i < file->message_type_count(); i++) {
    if (MessageHasConflictingClassName(file->message_type(i), classname,
                                       equality_mode)) {
      return true;
    }
  }
  return false;
}

std::string ClassNameResolver::GetDescriptorClassName(
    const FileDescriptor* file) {
  if (google::protobuf::internal::IsOss()) {
    return absl::StrCat(GetFileImmutableClassName(file),
                        ".InternalDescriptors");
  } else {
    return absl::StrCat(GetFileImmutableClassName(file), "InternalDescriptors");
  }
}

std::string ClassNameResolver::GetClassName(const FileDescriptor* descriptor,
                                            bool immutable) {
  return GetClassName(descriptor, immutable, false);
}

std::string ClassNameResolver::GetClassName(const FileDescriptor* descriptor,
                                            bool immutable, bool kotlin) {
  std::string result = GetFileJavaPackage(descriptor, immutable);
  if (!result.empty()) result += '.';
  result += GetFileClassName(descriptor, immutable, kotlin);
  return result;
}

// Get the full name of a Java class by prepending the Java package name
// or outer class name.
std::string ClassNameResolver::GetClassFullName(
    absl::string_view name_without_package, const FileDescriptor* file,
    bool immutable, bool is_own_file) {
  return GetClassFullName(name_without_package, file, immutable, is_own_file,
                          false);
}

std::string ClassNameResolver::GetClassFullName(
    absl::string_view name_without_package, const FileDescriptor* file,
    bool immutable, bool is_own_file, bool kotlin) {
  std::string result;
  if (is_own_file) {
    result = GetFileJavaPackage(file, immutable);
  } else {
    result = GetClassName(file, immutable, kotlin);
  }
  if (!result.empty()) {
    absl::StrAppend(&result, ".");
  }
  absl::StrAppend(&result, name_without_package);
  if (kotlin) absl::StrAppend(&result, "Kt");
  return result;
}

std::string ClassNameResolver::GetClassName(const Descriptor* descriptor,
                                            bool immutable) {
  return GetClassName(descriptor, immutable, false);
}

std::string ClassNameResolver::GetClassName(const Descriptor* descriptor,
                                            bool immutable, bool kotlin) {
  return GetClassFullName(ClassNameWithoutPackage(descriptor, immutable),
                          descriptor->file(), immutable,
                          !NestedInFileClass(*descriptor, immutable), kotlin);
}

std::string ClassNameResolver::GetClassName(const EnumDescriptor* descriptor,
                                            bool immutable) {
  return GetClassName(descriptor, immutable, false);
}

std::string ClassNameResolver::GetClassName(const EnumDescriptor* descriptor,
                                            bool immutable, bool kotlin) {
  return GetClassFullName(ClassNameWithoutPackage(descriptor, immutable),
                          descriptor->file(), immutable,
                          !NestedInFileClass(*descriptor, immutable), kotlin);
}

std::string ClassNameResolver::GetClassName(const ServiceDescriptor* descriptor,
                                            bool immutable) {
  return GetClassName(descriptor, immutable, false);
}

std::string ClassNameResolver::GetClassName(const ServiceDescriptor* descriptor,
                                            bool immutable, bool kotlin) {
  return GetClassFullName(ClassNameWithoutPackage(descriptor, immutable),
                          descriptor->file(), immutable,
                          IsOwnFile(descriptor, immutable), kotlin);
}

template <typename Descriptor>
std::string ClassNameResolver::GetJavaClassFullName(
    absl::string_view name_without_package, const Descriptor& descriptor,
    bool immutable) {
  return GetJavaClassFullName(name_without_package, descriptor, immutable,
                              /*kotlin =*/false);
}

// Get the Java Class style full name of a type.
template <typename Descriptor>
std::string ClassNameResolver::GetJavaClassFullName(
    absl::string_view name_without_package, const Descriptor& descriptor,
    bool immutable, bool kotlin) {
  std::string result;
  auto file = descriptor.file();
  if (NestedInFileClass(descriptor, immutable)) {
    result = GetClassName(file, immutable, kotlin);
    if (!result.empty()) result += '$';
  } else {
    result = GetFileJavaPackage(file, immutable);
    if (!result.empty()) result += '.';
  }
  result += absl::StrReplaceAll(name_without_package, {{".", "$"}});
  return result;
}

std::string ClassNameResolver::GetExtensionIdentifierName(
    const FieldDescriptor* descriptor, bool immutable) {
  return GetExtensionIdentifierName(descriptor, immutable, false);
}

std::string ClassNameResolver::GetExtensionIdentifierName(
    const FieldDescriptor* descriptor, bool immutable, bool kotlin) {
  return absl::StrCat(
      GetClassName(descriptor->containing_type(), immutable, kotlin), ".",
      descriptor->name());
}

std::string ClassNameResolver::GetKotlinFactoryName(
    const Descriptor* descriptor) {
  std::string name = ToCamelCase(descriptor->name(), /* lower_first = */ true);
  return IsForbiddenKotlin(name) ? absl::StrCat(name, "_") : name;
}

std::string ClassNameResolver::GetFullyQualifiedKotlinFactoryName(
    const Descriptor* descriptor) {
  if (descriptor->containing_type() != nullptr) {
    return absl::StrCat(
        GetKotlinExtensionsClassName(descriptor->containing_type()), ".",
        GetKotlinFactoryName(descriptor));
  } else {
    return absl::StrCat(
        GetFileJavaPackage(descriptor->file(), /*immutable=*/true), ".",
        GetKotlinFactoryName(descriptor));
  }
}

std::string ClassNameResolver::GetJavaImmutableClassName(
    const Descriptor* descriptor) {
  return GetJavaClassFullName(ClassNameWithoutPackage(descriptor, true),
                              *descriptor, true);
}

std::string ClassNameResolver::GetJavaImmutableClassName(
    const EnumDescriptor* descriptor) {
  return GetJavaClassFullName(ClassNameWithoutPackage(descriptor, true),
                              *descriptor, true);
}

std::string ClassNameResolver::GetJavaImmutableClassName(
    const ServiceDescriptor* descriptor) {
  return GetJavaClassFullName(ClassNameWithoutPackage(descriptor, true),
                              *descriptor, true);
}

std::string ClassNameResolver::GetKotlinExtensionsClassName(
    const Descriptor* descriptor) {
  return GetClassFullName(ClassNameWithoutPackageKotlin(descriptor),
                          descriptor->file(), /*immutable=*/true,
                          /*is_own_file=*/true,
                          /*kotlin=*/true);
}

std::string ClassNameResolver::GetKotlinExtensionsClassNameEscaped(
    const Descriptor* descriptor) {
  std::string name_without_package = ClassNameWithoutPackageKotlin(descriptor);
  std::string full_name = GetClassFullName(
      name_without_package, descriptor->file(), /*immutable=*/true,
      /*is_own_file=*/true, /*kotlin=*/true);
  std::string name_without_package_suffix =
      absl::StrCat(".", name_without_package, "Kt");
  size_t package_end = full_name.rfind(name_without_package_suffix);
  if (package_end != std::string::npos) {
    return absl::StrCat("`", full_name.substr(0, package_end), "`",
                        name_without_package_suffix);
  }
  return full_name;
}


std::string ClassNameResolver::GetFileJavaPackage(const FileDescriptor* file,
                                                  bool immutable) {
  return FileJavaPackage(file);
}
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
