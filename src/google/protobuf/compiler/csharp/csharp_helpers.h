// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: jonskeet@google.com (Jon Skeet)
//  Following the Java generator by kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_CSHARP_HELPERS_H__
#define GOOGLE_PROTOBUF_COMPILER_CSHARP_HELPERS_H__

#include <string>
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

// Commonly-used separator comments.  Thick is a line of '=', thin is a line
// of '-'.
extern const char kThickSeparator[];
extern const char kThinSeparator[];

// Converts the field's name to camel-case, e.g. "foo_bar_baz" becomes
// "fooBarBaz" or "FooBarBaz", respectively.
string UnderscoresToCamelCase(const FieldDescriptor* field);
string UnderscoresToCapitalizedCamelCase(const FieldDescriptor* field);

// Similar, but for method names.  (Typically, this merely has the effect
// of lower-casing the first letter of the name.)
string UnderscoresToCamelCase(const MethodDescriptor* method);

// Strips ".proto" or ".protodevel" from the end of a filename.
string StripProto(const string& filename);

// Gets the unqualified class name for the file.  Each .proto file becomes a
// single C# class, with extra (possibly nested) classes for messages, enums and services.
string FileClassName(const FileDescriptor* file);

// Returns the file's C# namespace.
string FileCSharpNamespace(const FileDescriptor* file);

// Converts the given fully-qualified name in the proto namespace to its
// fully-qualified name in the C# namespace, given that it is in the given
// file.
string ToCSharpName(const string& full_name, const FileDescriptor* file);

// These return the fully-qualified class name corresponding to the given
// descriptor.
inline string ClassName(const Descriptor* descriptor) {
  return ToCSharpName(descriptor->full_name(), descriptor->file());
}
inline string ClassName(const EnumDescriptor* descriptor) {
  return ToCSharpName(descriptor->full_name(), descriptor->file());
}
inline string ClassName(const ServiceDescriptor* descriptor) {
  return ToCSharpName(descriptor->full_name(), descriptor->file());
}
string ClassName(const FileDescriptor* descriptor);

enum MappedType {
  MAPPEDTYPE_INT,
  MAPPEDTYPE_LONG,
  MAPPEDTYPE_UINT,
  MAPPEDTYPE_ULONG,
  MAPPEDTYPE_FLOAT,
  MAPPEDTYPE_DOUBLE,
  MAPPEDTYPE_BOOLEAN,
  MAPPEDTYPE_STRING,
  MAPPEDTYPE_BYTES,
  MAPPEDTYPE_ENUM,
  MAPPEDTYPE_MESSAGE
};

MappedType GetMappedType(FieldDescriptor::Type field_type);

inline MappedType GetMappedType(const FieldDescriptor* field) {
  return GetMappedType(field->type());
}

// Get the access level for generated classes: public or internal
const char* ClassAccessLevel(const FileDescriptor* file);

// Get the class name for a built-in type (including ByteString).
// Returns NULL for enum and message types.
const char* MappedTypeName(MappedType type);

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_CSHARP_HELPERS_H__
