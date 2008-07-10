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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_FILE_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_FILE_H__

#include <string>
#include <vector>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/cpp/cpp_field.h>

namespace google {
namespace protobuf {
  class FileDescriptor;        // descriptor.h
  namespace io {
    class Printer;             // printer.h
  }
}

namespace protobuf {
namespace compiler {
namespace cpp {

class EnumGenerator;           // enum.h
class MessageGenerator;        // message.h
class ServiceGenerator;        // service.h
class ExtensionGenerator;      // extension.h

class FileGenerator {
 public:
  // See generator.cc for the meaning of dllexport_decl.
  explicit FileGenerator(const FileDescriptor* file,
                         const string& dllexport_decl);
  ~FileGenerator();

  void GenerateHeader(io::Printer* printer);
  void GenerateSource(io::Printer* printer);

 private:
  // Generate the BuildDescriptors() procedure, which builds all descriptors
  // for types defined in the file.
  void GenerateBuildDescriptors(io::Printer* printer);

  void GenerateNamespaceOpeners(io::Printer* printer);
  void GenerateNamespaceClosers(io::Printer* printer);

  const FileDescriptor* file_;

  scoped_array<scoped_ptr<MessageGenerator> > message_generators_;
  scoped_array<scoped_ptr<EnumGenerator> > enum_generators_;
  scoped_array<scoped_ptr<ServiceGenerator> > service_generators_;
  scoped_array<scoped_ptr<ExtensionGenerator> > extension_generators_;

  // E.g. if the package is foo.bar, package_parts_ is {"foo", "bar"}.
  vector<string> package_parts_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FileGenerator);
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_FILE_H__
