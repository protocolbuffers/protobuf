// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_HELPERS_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_HELPERS_H__

#include <string>
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

// Commonly-used separator comments.  Thick is a line of '=', thin is a line
// of '-'.
extern const char kThickSeparator[];
extern const char kThinSeparator[];

// Returns the non-nested type name for the given type.  If "qualified" is
// true, prefix the type with the full namespace.  For example, if you had:
//   package foo.bar;
//   message Baz { message Qux {} }
// Then the qualified ClassName for Qux would be:
//   ::foo::bar::Baz_Qux
// While the non-qualified version would be:
//   Baz_Qux
string ClassName(const Descriptor* descriptor, bool qualified);
string ClassName(const EnumDescriptor* enum_descriptor, bool qualified);

// Get the (unqualified) name that should be used for this field in C++ code.
// The name is coerced to lower-case to emulate proto1 behavior.  People
// should be using lowercase-with-underscores style for proto field names
// anyway, so normally this just returns field->name().
string FieldName(const FieldDescriptor* field);

// Get the unqualified name that should be used for a field's field
// number constant.
string FieldConstantName(const FieldDescriptor *field);

// Returns the scope where the field was defined (for extensions, this is
// different from the message type to which the field applies).
inline const Descriptor* FieldScope(const FieldDescriptor* field) {
  return field->is_extension() ?
    field->extension_scope() : field->containing_type();
}

// Strips ".proto" or ".protodevel" from the end of a filename.
string StripProto(const string& filename);

// Get the C++ type name for a primitive type (e.g. "double", "::google::protobuf::int32", etc.).
// Note:  non-built-in type names will be qualified, meaning they will start
// with a ::.  If you are using the type as a template parameter, you will
// need to insure there is a space between the < and the ::, because the
// ridiculous C++ standard defines "<:" to be a synonym for "[".
const char* PrimitiveTypeName(FieldDescriptor::CppType type);

// Get the declared type name in CamelCase format, as is used e.g. for the
// methods of WireFormat.  For example, TYPE_INT32 becomes "Int32".
const char* DeclaredTypeMethodName(FieldDescriptor::Type type);

// Convert a file name into a valid identifier.
string FilenameIdentifier(const string& filename);

// Return the name of the BuildDescriptors() function for a given file.
string GlobalBuildDescriptorsName(const string& filename);

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_HELPERS_H__
