// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// Provides a mechanism for mapping a descriptor to the
// fully-qualified name of the corresponding Java class.

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_NAMES_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_NAMES_H__

#include <string>

#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/java/options.h"
#include "google/protobuf/descriptor.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

class Descriptor;
class EnumDescriptor;
class FileDescriptor;
class FieldDescriptor;
class ServiceDescriptor;

namespace compiler {
namespace java {

// Requires:
//   descriptor != NULL
//
// Returns:
//   The fully-qualified Java class name.
std::string ClassName(const Descriptor* descriptor);

// Requires:
//   descriptor != NULL
//
// Returns:
//   The fully-qualified Java class name.
std::string ClassName(const EnumDescriptor* descriptor);

// Requires:
//   descriptor != NULL
//
// Returns:
//   The fully-qualified Java class name.
std::string ClassName(const FileDescriptor* descriptor);

// Requires:
//   descriptor != NULL
//
// Returns:
//   The fully-qualified Java class name.
std::string ClassName(const ServiceDescriptor* descriptor);

// Requires:
//   descriptor != NULL
//
// Returns:
//   Java package name.
std::string FileJavaPackage(const FileDescriptor* descriptor,
                            Options options = {});

// Requires:
//   descriptor != NULL
// Returns:
//   Capitalized camel case field name.
std::string CapitalizedFieldName(const FieldDescriptor* field);

// Requires:
//   descriptor != NULL
// Returns:
//   Capitalized camel case oneof name.
std::string CapitalizedOneofName(const OneofDescriptor* oneof);

// Returns:
//   Converts a name to camel-case. If cap_first_letter is true, capitalize the
//   first letter.
std::string UnderscoresToCamelCase(absl::string_view input,
                                   bool cap_next_letter);
// Requires:
//   field != NULL
// Returns:
//   Converts the field's name to camel-case, e.g. "foo_bar_baz" becomes
//   "fooBarBaz" or "FooBarBaz", respectively.
std::string UnderscoresToCamelCase(const FieldDescriptor* field);

// Requires:
//   method != NULL
// Returns:
//   Similar, but for method names.  (Typically, this merely has the effect
//   of lower-casing the first letter of the name.)
std::string UnderscoresToCamelCase(const MethodDescriptor* method);

// Requires:
//   field != NULL
// Returns:
//   Same as UnderscoresToCamelCase, but checks for reserved keywords
std::string UnderscoresToCamelCaseCheckReserved(const FieldDescriptor* field);


}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_NAMES_H__
