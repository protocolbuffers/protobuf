// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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

// Helper functions for generating ObjectiveC code.

#ifndef GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_NAMES_H__
#define GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_NAMES_H__

#include <string>
#include <vector>

#include "google/protobuf/descriptor.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

// Gets the objc_class_prefix or the prefix made from the proto package.
std::string PROTOC_EXPORT FileClassPrefix(const FileDescriptor* file);

// Gets the path of the file we're going to generate (sans the .pb.h
// extension).  The path will be dependent on the objectivec package
// declared in the proto package.
std::string PROTOC_EXPORT FilePath(const FileDescriptor* file);

// Just like FilePath(), but without the directory part.
std::string PROTOC_EXPORT FilePathBasename(const FileDescriptor* file);

// Gets the name of the root class we'll generate in the file.  This class
// is not meant for external consumption, but instead contains helpers that
// the rest of the classes need
std::string PROTOC_EXPORT FileClassName(const FileDescriptor* file);

// These return the fully-qualified class name corresponding to the given
// descriptor.
std::string PROTOC_EXPORT ClassName(const Descriptor* descriptor);
std::string PROTOC_EXPORT ClassName(const Descriptor* descriptor,
                                    std::string* out_suffix_added);
std::string PROTOC_EXPORT EnumName(const EnumDescriptor* descriptor);

// Returns the fully-qualified name of the enum value corresponding to the
// the descriptor.
std::string PROTOC_EXPORT EnumValueName(const EnumValueDescriptor* descriptor);

// Returns the name of the enum value corresponding to the descriptor.
std::string PROTOC_EXPORT EnumValueShortName(const EnumValueDescriptor* descriptor);

// Reverse what an enum does.
std::string PROTOC_EXPORT UnCamelCaseEnumShortName(const std::string& name);

// Returns the name to use for the extension (used as the method off the file's
// Root class).
std::string PROTOC_EXPORT ExtensionMethodName(const FieldDescriptor* descriptor);

// Returns the transformed field name.
std::string PROTOC_EXPORT FieldName(const FieldDescriptor* field);
std::string PROTOC_EXPORT FieldNameCapitalized(const FieldDescriptor* field);

// Returns the transformed oneof name.
std::string PROTOC_EXPORT OneofEnumName(const OneofDescriptor* descriptor);
std::string PROTOC_EXPORT OneofName(const OneofDescriptor* descriptor);
std::string PROTOC_EXPORT OneofNameCapitalized(const OneofDescriptor* descriptor);

// Returns a symbol that can be used in C code to refer to an Objective C
// class without initializing the class.
std::string PROTOC_EXPORT ObjCClass(const std::string& class_name);

// Declares an Objective C class without initializing the class so that it can
// be refrerred to by ObjCClass.
std::string PROTOC_EXPORT ObjCClassDeclaration(const std::string& class_name);

// The name the commonly used by the library when built as a framework.
// This lines up to the name used in the CocoaPod.
extern PROTOC_EXPORT const char* const ProtobufLibraryFrameworkName;
// Returns the CPP symbol name to use as the gate for framework style imports
// for the given framework name to use.
std::string PROTOC_EXPORT
ProtobufFrameworkImportSymbol(const std::string& framework_name);

// Checks if the file is one of the proto's bundled with the library.
bool PROTOC_EXPORT
IsProtobufLibraryBundledProtoFile(const FileDescriptor* file);

// Generator Prefix Validation Options (see objectivec_generator.cc for a
// description of each):
struct Options {
  Options();
  std::string expected_prefixes_path;
  std::vector<std::string> expected_prefixes_suppressions;
  bool prefixes_must_be_registered;
  bool require_prefixes;
};

// Checks the prefix for the given files and outputs any warnings as needed. If
// there are flat out errors, then out_error is filled in with the first error
// and the result is false.
bool PROTOC_EXPORT ValidateObjCClassPrefixes(
    const std::vector<const FileDescriptor*>& files,
    const Options& validation_options, std::string* out_error);
// Same was the other ValidateObjCClassPrefixes() calls, but the options all
// come from the environment variables.
bool PROTOC_EXPORT ValidateObjCClassPrefixes(
    const std::vector<const FileDescriptor*>& files, std::string* out_error);

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_NAMES_H__
