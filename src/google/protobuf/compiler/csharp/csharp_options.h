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

#ifndef GOOGLE_PROTOBUF_COMPILER_CSHARP_OPTIONS_H__
#define GOOGLE_PROTOBUF_COMPILER_CSHARP_OPTIONS_H__

#include <string>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

// Generator options (used by csharp_generator.cc):
struct Options {
  Options() :
      file_extension(".cs"),
      base_namespace(""),
      base_namespace_specified(false),
      internal_access(false),
      serializable(false),
      custom_base_class(""),
      keep_original_file_name(false), 
      keep_original_field_name(false), 
      disable_nested_types_container(false) {
  }
  // Extension of the generated file. Defaults to ".cs"
  std::string file_extension;
  // Base namespace to use to create directory hierarchy. Defaults to "".
  // This option allows the simple creation of a conventional C# file layout,
  // where directories are created relative to a project-specific base
  // namespace. For example, in a project with a base namespace of PetShop, a
  // proto of user.proto with a C# namespace of PetShop.Model.Shared would
  // generate Model/Shared/User.cs underneath the specified --csharp_out
  // directory.
  //
  // If no base namespace is specified, all files are generated in the
  // --csharp_out directory, with no subdirectories created automatically.
  std::string base_namespace;
  // Whether the base namespace has been explicitly specified by the user.
  // This is required as the base namespace can be explicitly set to the empty
  // string, meaning "create a full directory hierarchy, starting from the first
  // segment of the namespace."
  bool base_namespace_specified;
  // Whether the generated classes should have accessibility level of "internal".
  // Defaults to false that generates "public" classes.
  bool internal_access;
  // Whether the generated classes should have a global::System.Serializable attribute added
  // Defaults to false
  bool serializable;
  // Custom base class to use for generated classes. Defaults to "".
  // If specified, the base class must be a public class in the current namespace.
  // There is a special support for generic classes, which can be specified by "MyBaseClass<>",
  // "<>" is a placeholder, which will be replaced by the current message class name.
  // For example, for a message named "MyMessage", if "MyBaseClass<>" is specified, 
  // the base class will be "MyBaseClass<MyMessage>".
  std::string custom_base_class;
  // Whether to keep the original file name. Defaults to false.
  // If true, the CamelCase name transformation is skipped, and the original file name of proto file is preserved
  bool keep_original_file_name;
  // Whether to keep the original field name. Defaults to false.
  // If true, the CamelCase name transformation is skipped, and the original field name defined in proto file is preserved
  bool keep_original_field_name;
  // Whether to generate a nested `Types` container class for sub messages.
  // If true, the nested `Types` container class will not be generated. Be ware, name collision may occurs.
  bool disable_nested_types_container;
};

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CSHARP_OPTIONS_H__
