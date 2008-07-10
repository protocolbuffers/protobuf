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

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_SERVICE_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_SERVICE_H__

#include <map>
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {
  namespace io {
    class Printer;             // printer.h
  }
}

namespace protobuf {
namespace compiler {
namespace java {

class ServiceGenerator {
 public:
  explicit ServiceGenerator(const ServiceDescriptor* descriptor);
  ~ServiceGenerator();

  void Generate(io::Printer* printer);

 private:
  // Generate the implementation of Service.callMethod().
  void GenerateCallMethod(io::Printer* printer);

  // Generate the implementations of Service.get{Request,Response}Prototype().
  enum RequestOrResponse { REQUEST, RESPONSE };
  void GenerateGetPrototype(RequestOrResponse which, io::Printer* printer);

  // Generate a stub implementation of the service.
  void GenerateStub(io::Printer* printer);

  const ServiceDescriptor* descriptor_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ServiceGenerator);
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf

#endif  // NET_PROTO2_COMPILER_JAVA_SERVICE_H__
}  // namespace google
