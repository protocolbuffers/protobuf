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

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_SERVICE_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_SERVICE_H__

#include <map>
#include <string>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {
  namespace io {
    class Printer;             // printer.h
  }
}

namespace protobuf {
namespace compiler {
namespace cpp {

class ServiceGenerator {
 public:
  // See generator.cc for the meaning of dllexport_decl.
  explicit ServiceGenerator(const ServiceDescriptor* descriptor,
                            const string& dllexport_decl);
  ~ServiceGenerator();

  // Header stuff.

  // Generate the class definitions for the service's interface and the
  // stub implementation.
  void GenerateDeclarations(io::Printer* printer);

  // Source file stuff.

  // Generate code that initializes the global variable storing the service's
  // descriptor.
  void GenerateDescriptorInitializer(io::Printer* printer, int index);

  // Generate implementations of everything declared by GenerateDeclarations().
  void GenerateImplementation(io::Printer* printer);

 private:
  enum RequestOrResponse { REQUEST, RESPONSE };
  enum VirtualOrNon { VIRTUAL, NON_VIRTUAL };

  // Header stuff.

  // Generate the service abstract interface.
  void GenerateInterface(io::Printer* printer);

  // Generate the stub class definition.
  void GenerateStubDefinition(io::Printer* printer);

  // Prints signatures for all methods in the
  void GenerateMethodSignatures(VirtualOrNon virtual_or_non,
                                io::Printer* printer);

  // Source file stuff.

  // Generate the default implementations of the service methods, which
  // produce a "not implemented" error.
  void GenerateNotImplementedMethods(io::Printer* printer);

  // Generate the CallMethod() method of the service.
  void GenerateCallMethod(io::Printer* printer);

  // Generate the Get{Request,Response}Prototype() methods.
  void GenerateGetPrototype(RequestOrResponse which, io::Printer* printer);

  // Generate the stub's implementations of the service methods.
  void GenerateStubMethods(io::Printer* printer);

  const ServiceDescriptor* descriptor_;
  map<string, string> vars_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ServiceGenerator);
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_SERVICE_H__
