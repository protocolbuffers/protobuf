// Protocol Buffers - Google's data interchange format
// Copyright 2018 Oleksii Pylypenko.  All rights reserved.
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

// Author: oleksiy.pylypenko@gmail.com

#ifndef GOOGLE_PROTOBUF_COMPILER_KOTLIN_FILE_H__
#define GOOGLE_PROTOBUF_COMPILER_KOTLIN_FILE_H__

#include <memory>
#include <string>
#include <vector>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/kotlin/kotlin_options.h>

namespace google {
namespace protobuf {
class FileDescriptor;          // descriptor.h
namespace io {
  class Printer;               // printer.h
}
namespace compiler {
  class GeneratorContext;      // code_generator.h
  namespace kotlin {
    class Context;             // context.h
  }
  namespace java {
    class ClassNameResolver;   // name_resolver.h
  }
}
}  // namespace protobuf
}  // namespace google

namespace google {
namespace protobuf {
namespace compiler {
namespace kotlin {

class FileGenerator {
 public:
  FileGenerator(const FileDescriptor* file, const Options& options,
                bool immutable_api = true);
  ~FileGenerator();

  // Checks for problems that would otherwise lead to cryptic compile errors.
  // Returns true if there are no problems, or writes an error description to
  // the given string and returns false otherwise.
  bool Validate(std::string* error);

  void Generate(io::Printer* printer);

  const std::string& java_package() { return java_package_; }
  const std::string& classname() { return classname_; }

 private:

  const FileDescriptor* file_;
  std::string java_package_;
  std::string classname_;

  java::ClassNameResolver* name_resolver_;
  const Options options_;
  bool immutable_api_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FileGenerator);
};

}  // namespace kotlin
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_KOTLIN_FILE_H__
