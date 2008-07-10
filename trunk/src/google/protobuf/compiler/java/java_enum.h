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

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_ENUM_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_ENUM_H__

#include <string>
#include <vector>
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

class EnumGenerator {
 public:
  explicit EnumGenerator(const EnumDescriptor* descriptor);
  ~EnumGenerator();

  void Generate(io::Printer* printer);

 private:
  const EnumDescriptor* descriptor_;

  // The proto language allows multiple enum constants to have the same numeric
  // value.  Java, however, does not allow multiple enum constants to be
  // considered equivalent.  We treat the first defined constant for any
  // given numeric value as "canonical" and the rest as aliases of that
  // canonical value.
  vector<const EnumValueDescriptor*> canonical_values_;

  struct Alias {
    const EnumValueDescriptor* value;
    const EnumValueDescriptor* canonical_value;
  };
  vector<Alias> aliases_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(EnumGenerator);
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_ENUM_H__
