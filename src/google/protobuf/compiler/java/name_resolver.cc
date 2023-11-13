// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/java/name_resolver.h"

#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
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
  std::string result = descriptor->name();
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
  if (message_descriptor == NULL) {
    return descriptor->name();
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

std::string ClassNameResolver::GetFileDefaultImmutableClassName(
    const FileDescriptor* file) {
  std::string basename;
  std::string::size_type last_slash = file->name().find_last_of('/');
  if (last_slash == std::string::npos) {
    basename = file->name();
  } else {
    basename = file->name().substr(last_slash + 1);
  }
  return UnderscoresToCamelCase(StripProto(basename), true);
}

std::string ClassNameResolver::GetFileImmutableClassName(
    const FileDescriptor* file) {
  std::string& class_name = file_immutable_outer_class_names_[file];
  if (class_name.empty()) {
    if (file->options().has_java_outer_classname()) {
      class_name = file->options().java_outer_classname();
    } else {
      class_name = GetFileDefaultImmutableClassName(file);
      if (HasConflictingClassName(file, class_name,
                                  NameEquality::EXACT_EQUAL)) {
        class_name += kOuterClassNameSuffix;
      }
    }
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
  if (options_.opensource_runtime) {
    return GetFileImmutableClassName(file);
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
  std::string result = FileJavaPackage(descriptor, immutable, options_);
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
    result = FileJavaPackage(file, immutable, options_);
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
  return GetClassFullName(
      ClassNameWithoutPackage(descriptor, immutable), descriptor->file(),
      immutable, MultipleJavaFiles(descriptor->file(), immutable), kotlin);
}

std::string ClassNameResolver::GetClassName(const EnumDescriptor* descriptor,
                                            bool immutable) {
  return GetClassName(descriptor, immutable, false);
}

std::string ClassNameResolver::GetClassName(const EnumDescriptor* descriptor,
                                            bool immutable, bool kotlin) {
  return GetClassFullName(
      ClassNameWithoutPackage(descriptor, immutable), descriptor->file(),
      immutable, MultipleJavaFiles(descriptor->file(), immutable), kotlin);
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

// Get the Java Class style full name of a message.
std::string ClassNameResolver::GetJavaClassFullName(
    absl::string_view name_without_package, const FileDescriptor* file,
    bool immutable) {
  return GetJavaClassFullName(name_without_package, file, immutable, false);
}

std::string ClassNameResolver::GetJavaClassFullName(
    absl::string_view name_without_package, const FileDescriptor* file,
    bool immutable, bool kotlin) {
  std::string result;
  if (MultipleJavaFiles(file, immutable)) {
    result = FileJavaPackage(file, immutable, options_);
    if (!result.empty()) result += '.';
  } else {
    result = GetClassName(file, immutable, kotlin);
    if (!result.empty()) result += '$';
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

std::string ClassNameResolver::GetJavaImmutableClassName(
    const Descriptor* descriptor) {
  return GetJavaClassFullName(ClassNameWithoutPackage(descriptor, true),
                              descriptor->file(), true);
}

std::string ClassNameResolver::GetJavaImmutableClassName(
    const EnumDescriptor* descriptor) {
  return GetJavaClassFullName(ClassNameWithoutPackage(descriptor, true),
                              descriptor->file(), true);
}

std::string ClassNameResolver::GetJavaImmutableClassName(
    const ServiceDescriptor* descriptor) {
  return GetJavaClassFullName(ClassNameWithoutPackage(descriptor, true),
                              descriptor->file(), true);
}

std::string ClassNameResolver::GetKotlinExtensionsClassName(
    const Descriptor* descriptor) {
  return GetClassFullName(ClassNameWithoutPackageKotlin(descriptor),
                          descriptor->file(), true, true, true);
}

std::string ClassNameResolver::GetKotlinExtensionsClassNameEscaped(
    const Descriptor* descriptor) {
  std::string name_without_package = ClassNameWithoutPackageKotlin(descriptor);
  std::string full_name = GetClassFullName(
      name_without_package, descriptor->file(), true, true, true);
  std::string name_without_package_suffix =
      absl::StrCat(".", name_without_package, "Kt");
  size_t package_end = full_name.rfind(name_without_package_suffix);
  if (package_end != std::string::npos) {
    return absl::StrCat("`", full_name.substr(0, package_end), "`",
                        name_without_package_suffix);
  }
  return full_name;
}

std::string ClassNameResolver::GetJavaMutableClassName(
    const Descriptor* descriptor) {
  return GetJavaClassFullName(ClassNameWithoutPackage(descriptor, false),
                              descriptor->file(), false);
}

std::string ClassNameResolver::GetJavaMutableClassName(
    const EnumDescriptor* descriptor) {
  return GetJavaClassFullName(ClassNameWithoutPackage(descriptor, false),
                              descriptor->file(), false);
}

std::string ClassNameResolver::GetJavaMutableClassName(
    const ServiceDescriptor* descriptor) {
  return GetJavaClassFullName(ClassNameWithoutPackage(descriptor, false),
                              descriptor->file(), false);
}

std::string ClassNameResolver::GetDowngradedFileClassName(
    const FileDescriptor* file) {
  return absl::StrCat("Downgraded", GetFileClassName(file, false));
}

std::string ClassNameResolver::GetDowngradedClassName(
    const Descriptor* descriptor) {
  return absl::StrCat(FileJavaPackage(descriptor->file(), true, options_), ".",
                      GetDowngradedFileClassName(descriptor->file()), ".",
                      ClassNameWithoutPackage(descriptor, false));
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
